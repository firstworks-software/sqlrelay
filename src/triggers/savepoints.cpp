// Copyright (c) David Muse
// All rights reserved

#include <sqlrelay/sqlrserver.h>

// owns spname
class savepointentry {
	public:
		savepointentry(uint16_t cursorid, char *spname);
		~savepointentry();

		uint16_t	cursorid;
		char		*spname;
};

savepointentry::savepointentry(uint16_t cursorid, char *spname) {
	this->cursorid=cursorid;
	this->spname=spname;
}

savepointentry::~savepointentry() {
	delete[] spname;
}


class SQLRSERVER_DLLSPEC sqlrtrigger_savepoints : public sqlrtrigger {
	public:
		sqlrtrigger_savepoints(sqlrservercontroller *cont,
						domnode *parameters);

		bool	runBeforePrepare(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runAfterPrepare(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runBeforeExecute(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runAfterExecute(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);

		void	endTransaction(bool commit);
		void	endSession();

	private:
		bool	shouldSkip(sqlrservercursor *sqlrcur);

		// Returns the live savepoint name for "sqlrcur", or NULL if
		// there isn't one.  An entry left behind by a deleted cursor
		// that happened to be allocated at the same address is
		// detected by its id and dropped.
		const char	*getSavepointName(sqlrservercursor *sqlrcur);

		void	buildSavepointSql(const char *format,
						const char *spname,
						stringbuffer *out);
		bool	runQuery(const char *format, const char *spname);
		bool	createSavepoint(sqlrservercursor *sqlrcur);
		void	finishSavepoint(sqlrservercursor *sqlrcur, bool error);

		const char	*savepointquery;
		const char	*rollbackquery;
		const char	*releasequery;
		bool		dorelease;

		const char	*prefix;

		// Shared across cursors on purpose.  A per-cursor counter
		// would give two cursors' savepoints the same name, and
		// databases resolve a rollback or release against the most
		// recent savepoint of that name, so one cursor would roll
		// back to the other cursor's savepoint.
		uint64_t	spcounter;

		// name of the live savepoint, keyed by cursor - this module is
		// instantiated once per connection, but each cursor gets its
		// own savepoint, and an entry exists only between successful
		// savepoint creation and its rollback/release
		dictionary< sqlrservercursor *, savepointentry * >	spnames;

		stringbuffer	spsql;
};

sqlrtrigger_savepoints::sqlrtrigger_savepoints(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrtrigger(cont,parameters) {

	// sql templates - "%s" is replaced with the savepoint name
	savepointquery=parameters->getAttributeValue("savepointquery");
	if (charstring::isNullOrEmpty(savepointquery)) {
		savepointquery="savepoint %s";
	}
	rollbackquery=parameters->getAttributeValue("rollbackquery");
	if (charstring::isNullOrEmpty(rollbackquery)) {
		rollbackquery="rollback to savepoint %s";
	}
	releasequery=parameters->getAttributeValue("releasequery");
	if (charstring::isNullOrEmpty(releasequery)) {
		releasequery="release savepoint %s";
	}

	// whether to release the savepoint after a successful query
	const char	*r=parameters->getAttributeValue("release");
	dorelease=charstring::isNullOrEmpty(r) || !charstring::isNo(r);

	// prefix for the auto-generated savepoint names
	prefix=parameters->getAttributeValue("prefix");
	if (charstring::isNullOrEmpty(prefix)) {
		prefix="sqlrsp";
	}

	spcounter=0;

	// let the dictionary delete each entry on remove/clear/destruction,
	// which deletes the name it owns too
	spnames.setManageValues(true);
}

const char *sqlrtrigger_savepoints::getSavepointName(
					sqlrservercursor *sqlrcur) {

	savepointentry	*spe=spnames.getValue(sqlrcur);
	if (!spe) {
		return NULL;
	}

	// A cursor can be deleted without its savepoint being finished, and
	// newCursor() can hand the same address back later with a different
	// id.  The entry then belongs to the earlier cursor, and its
	// savepoint no longer exists, so drop it rather than reuse it.
	if (spe->cursorid!=sqlrcur->getId()) {
		debugWrite("dropping stale savepoint %s (cursor id %d, "
					"entry id %d)",spe->spname,
					(int)sqlrcur->getId(),
					(int)spe->cursorid);
		spnames.remove(sqlrcur);
		return NULL;
	}

	return spe->spname;
}

bool sqlrtrigger_savepoints::shouldSkip(sqlrservercursor *sqlrcur) {

	// bail if the query was suppressed
	if (cont->getQuerySuppressed(sqlrcur)) {
		debugWrite("skip: query suppressed");
		return true;
	}

	// savepoints only have meaning inside a transaction
	// (in autocommit mode, any savepoint we create would be destroyed
	// before runAfterExecute could roll back to it)
	if (!cont->getInTransaction()) {
		debugWrite("skip: not in a transaction");
		return true;
	}

	// skip transaction-control queries
	sqlrquerytype_t	querytype=sqlrcur->getQueryType();
	if (querytype==SQLRQUERYTYPE_BEGIN ||
		querytype==SQLRQUERYTYPE_COMMIT ||
		querytype==SQLRQUERYTYPE_ROLLBACK ||
		querytype==SQLRQUERYTYPE_AUTOCOMMIT_ON ||
		querytype==SQLRQUERYTYPE_AUTOCOMMIT_OFF ||
		querytype==SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_ON ||
		querytype==SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_OFF) {
		debugWrite("skip: transaction-control query (type %d)",
								(int)querytype);
		return true;
	}

	debugWrite("not skipping");
	return false;
}

bool sqlrtrigger_savepoints::runBeforePrepare(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	debugStart("savepoints runBeforePrepare");

	// reset state
	spnames.remove(sqlrcur);

	if (shouldSkip(sqlrcur)) {
		debugEnd();
		return true;
	}

	// Take the savepoint before prepare.  Some databases (notably
	// postgresql) parse and resolve references during prepare, so
	// reference-related failures occur here rather than during
	// execute.
	createSavepoint(sqlrcur);

	debugEnd();

	return true;
}

bool sqlrtrigger_savepoints::runAfterPrepare(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// bail if we never created a savepoint
	if (!getSavepointName(sqlrcur)) {
		return true;
	}

	// did the prepare error?
	bool	error=(cont->getErrorSize(sqlrcur) ||
				cont->getErrorNumber(sqlrcur));

	// leave the savepoint in place to bracket the execute
	if (!error) {
		return true;
	}

	debugStart("savepoints runAfterPrepare");
	finishSavepoint(sqlrcur,true);
	debugEnd();

	// preserve the user's error
	return true;
}

bool sqlrtrigger_savepoints::runBeforeExecute(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// reuse the savepoint created by runBeforePrepare
	if (getSavepointName(sqlrcur)) {
		return true;
	}

	debugStart("savepoints runBeforeExecute");

	// no savepoint live, create one to bracket the execute
	if (shouldSkip(sqlrcur)) {
		debugEnd();
		return true;
	}

	createSavepoint(sqlrcur);

	debugEnd();

	return true;
}

bool sqlrtrigger_savepoints::runAfterExecute(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// bail if no savepoint is live
	if (!getSavepointName(sqlrcur)) {
		return true;
	}

	debugStart("savepoints runAfterExecute");

	// did the user query error?
	bool	error=(cont->getErrorSize(sqlrcur) ||
				cont->getErrorNumber(sqlrcur));

	finishSavepoint(sqlrcur,error);

	debugEnd();

	// preserve the user's error
	return true;
}

void sqlrtrigger_savepoints::endTransaction(bool commit) {

	debugStart("savepoints endTransaction");

	// a commit or rollback destroys every savepoint
	debugWrite("dropping %lld savepoints",
			(long long)spnames.getCount());
	spnames.clear();

	debugEnd();
}

void sqlrtrigger_savepoints::endSession() {

	debugStart("savepoints endSession");

	debugWrite("dropping %lld savepoints",
			(long long)spnames.getCount());
	spnames.clear();

	debugEnd();
}

bool sqlrtrigger_savepoints::createSavepoint(sqlrservercursor *sqlrcur) {

	// build a unique savepoint name
	stringbuffer	spname;
	spname.append(prefix)->append(spcounter++);

	// if this fails, log it but let the user query run anyway
	if (runQuery(savepointquery,spname.getString())) {

		// setValue() would overwrite, rather than delete, an entry
		// that's somehow still here
		spnames.remove(sqlrcur);

		// the entry takes ownership of the name
		spnames.setValue(sqlrcur,
				new savepointentry(sqlrcur->getId(),
						spname.detachString()));
		return true;
	}
	debugWrite("failed to create savepoint %s",spname.getString());
	return false;
}

void sqlrtrigger_savepoints::finishSavepoint(sqlrservercursor *sqlrcur,
								bool error) {

	const char	*livespname=getSavepointName(sqlrcur);
	if (!livespname) {
		return;
	}

	// Take our own copy of the name and drop the entry before running
	// anything.  The rollback and release queries go through
	// interceptQuery(), so a configured query that classifies as a
	// commit, rollback or autocommit change reaches endTransaction(),
	// which clears the map and would free the name mid-use.
	char	*spname=charstring::duplicate(livespname);
	spnames.remove(sqlrcur);

	if (error) {

		// undo any partial effects of the failed query
		if (!runQuery(rollbackquery,spname)) {
			debugWrite("failed to roll back to savepoint %s",
									spname);
		}

		// release the savepoint after rolling back, so the stack
		// doesn't grow
		if (dorelease) {
			if (!runQuery(releasequery,spname)) {
				debugWrite("failed to release savepoint %s",
									spname);
			}
		}

	} else if (dorelease) {

		// release the savepoint so the stack doesn't grow
		if (!runQuery(releasequery,spname)) {
			debugWrite("failed to release savepoint %s",spname);
		}
	}

	delete[] spname;
}

void sqlrtrigger_savepoints::buildSavepointSql(const char *format,
						const char *spname,
						stringbuffer *out) {

	// replace "%s" with the savepoint name
	const char	*pct=charstring::findFirst(format,"%s");
	if (pct) {
		out->append(format,pct-format);
		out->append(spname);
		out->append(pct+2);
	} else {
		out->append(format);
	}
}

bool sqlrtrigger_savepoints::runQuery(const char *format, const char *spname) {

	spsql.clear();
	buildSavepointSql(format,spname,&spsql);

	debugWrite("%.*s",(int)spsql.getSize(),spsql.getString());

	// use a dedicated cursor
	sqlrservercursor	*spcur=cont->newCursor();
	if (!spcur) {
		return false;
	}
	if (!cont->open(spcur)) {
		cont->deleteCursor(spcur);
		return false;
	}

	// run the query with directives, translations, filters, and
	// triggers disabled, so this trigger doesn't recurse into itself
	bool	success=cont->prepareQuery(spcur,
					spsql.getString(),spsql.getSize(),
					false,false,false,false) &&
			cont->executeQuery(spcur,false,false,false,false);

	cont->closeResultSet(spcur);
	cont->close(spcur);
	cont->deleteCursor(spcur);

	return success;
}

extern "C" {
	SQLRSERVER_DLLSPEC
	sqlrtrigger	*new_sqlrtrigger_savepoints(sqlrservercontroller *cont,
						domnode *parameters) {

		return new sqlrtrigger_savepoints(cont,parameters);
	}
}

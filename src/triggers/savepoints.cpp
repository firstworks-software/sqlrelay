// Copyright (c) David Muse
// All rights reserved

#include <sqlrelay/sqlrserver.h>

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

	private:
		bool	shouldSkip(sqlrservercursor *sqlrcur);
		void	buildSavepointSql(const char *format,
						stringbuffer *out);
		bool	runQuery(const char *format);
		bool	createSavepoint();
		void	finishSavepoint(bool error);

		const char	*savepointquery;
		const char	*rollbackquery;
		const char	*releasequery;
		bool		dorelease;

		const char	*prefix;
		uint64_t	spcounter;
		stringbuffer	spname;

		// true between successful savepoint creation and its
		// rollback/release
		bool		spactive;

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
	spactive=false;
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
	spactive=false;

	if (shouldSkip(sqlrcur)) {
		debugEnd();
		return true;
	}

	// Take the savepoint before prepare.  Some databases (notably
	// postgresql) parse and resolve references during prepare, so
	// reference-related failures occur here rather than during
	// execute.
	createSavepoint();

	debugEnd();

	return true;
}

bool sqlrtrigger_savepoints::runAfterPrepare(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// bail if we never created a savepoint
	if (!spactive) {
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
	finishSavepoint(true);
	debugEnd();

	// preserve the user's error
	return true;
}

bool sqlrtrigger_savepoints::runBeforeExecute(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// reuse the savepoint created by runBeforePrepare
	if (spactive) {
		return true;
	}

	debugStart("savepoints runBeforeExecute");

	// no savepoint live, create one to bracket the execute
	if (shouldSkip(sqlrcur)) {
		debugEnd();
		return true;
	}

	createSavepoint();

	debugEnd();

	return true;
}

bool sqlrtrigger_savepoints::runAfterExecute(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// bail if no savepoint is live
	if (!spactive) {
		return true;
	}

	debugStart("savepoints runAfterExecute");

	// did the user query error?
	bool	error=(cont->getErrorSize(sqlrcur) ||
				cont->getErrorNumber(sqlrcur));

	finishSavepoint(error);

	debugEnd();

	// preserve the user's error
	return true;
}

bool sqlrtrigger_savepoints::createSavepoint() {

	// build a unique savepoint name
	spname.clear();
	spname.append(prefix)->append(spcounter++);

	// if this fails, log it but let the user query run anyway
	if (runQuery(savepointquery)) {
		spactive=true;
		return true;
	}
	debugWrite("failed to create savepoint %s",spname.getString());
	return false;
}

void sqlrtrigger_savepoints::finishSavepoint(bool error) {

	spactive=false;

	if (error) {

		// undo any partial effects of the failed query
		if (!runQuery(rollbackquery)) {
			debugWrite("failed to roll back to savepoint %s",
							spname.getString());
		}

		// release the savepoint after rolling back, so the stack
		// doesn't grow
		if (dorelease) {
			if (!runQuery(releasequery)) {
				debugWrite("failed to release savepoint %s",
							spname.getString());
			}
		}

	} else if (dorelease) {

		// release the savepoint so the stack doesn't grow
		if (!runQuery(releasequery)) {
			debugWrite("failed to release savepoint %s",
							spname.getString());
		}
	}
}

void sqlrtrigger_savepoints::buildSavepointSql(const char *format,
						stringbuffer *out) {

	// replace "%s" with the savepoint name
	const char	*pct=charstring::findFirst(format,"%s");
	if (pct) {
		out->append(format,pct-format);
		out->append(spname.getString(),spname.getSize());
		out->append(pct+2);
	} else {
		out->append(format);
	}
}

bool sqlrtrigger_savepoints::runQuery(const char *format) {

	spsql.clear();
	buildSavepointSql(format,&spsql);

	debugWrite("%.*s",spsql.getSize(),spsql.getString());

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

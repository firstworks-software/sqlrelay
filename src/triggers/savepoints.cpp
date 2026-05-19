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

		// True between successful savepoint creation and its
		// rollback/release.  Used both to decide whether to do
		// cleanup and to skip redundant savepoint creation in
		// runBeforeExecute when one is already live from
		// runBeforePrepare.
		bool		spactive;

		stringbuffer	spsql;
};

sqlrtrigger_savepoints::sqlrtrigger_savepoints(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrtrigger(cont,parameters) {

	// SQL templates - "%s" is replaced with the savepoint name.
	// Defaults are standard SQL (works for postgresql, mysql, oracle,
	// db2, firebird, etc.).  For sap/sybase/mssql, configure as
	// savepointquery="save tran %s" rollbackquery="rollback tran %s".
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

	// whether to release the savepoint after a successful query.
	// Keeps the savepoint stack from growing for long-running
	// transactions.  Set release="no" for databases that don't
	// support releasing savepoints (eg. sap/sybase/mssql).
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

	// nothing to do if the query was suppressed
	if (cont->getQuerySuppressed(sqlrcur)) {
		debugWrite("skip: query suppressed");
		return true;
	}

	// savepoints only have meaning inside a transaction.  In
	// autocommit mode each statement is its own transaction and any
	// savepoint we create would be destroyed before runAfterExecute
	// could roll back to it.
	if (!cont->getInTransaction()) {
		debugWrite("skip: not in a transaction");
		return true;
	}

	// skip transaction-control queries.  Creating a savepoint inside
	// a commit/rollback/autocommit-toggle is either nonsensical or
	// would be wiped out by the user's query.
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

	// Reset state.  If we bail or fail to create a savepoint,
	// runAfterPrepare must not try to roll back or release.
	spactive=false;

	if (shouldSkip(sqlrcur)) {
		debugEnd();
		return true;
	}

	// Take the savepoint before prepare.  Some databases (notably
	// postgresql) parse and resolve references during prepare, so
	// reference-related failures occur here rather than during
	// execute.  Without a savepoint bracketing the prepare, those
	// failures would poison the transaction.
	createSavepoint();

	debugEnd();

	return true;
}

bool sqlrtrigger_savepoints::runAfterPrepare(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// nothing to do if we never created a savepoint for this prepare
	if (!spactive) {
		return true;
	}

	// did the prepare error?
	bool	error=(cont->getErrorSize(sqlrcur) ||
				cont->getErrorNumber(sqlrcur));

	// If the prepare succeeded, leave the savepoint in place so it
	// can bracket the upcoming execute.  runAfterExecute will release
	// it.
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

	// If runBeforePrepare already created a savepoint for this query
	// (the typical prepare-then-execute case), reuse it.
	if (spactive) {
		return true;
	}

	debugStart("savepoints runBeforeExecute");

	// No savepoint live - we're in a re-execute of a previously
	// prepared statement, or the prepare phase was skipped/intercepted.
	// Create one now to bracket the execute.
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

	// preserve the user's error - returning true here doesn't clear
	// it, the caller still sees the failure
	return true;
}

bool sqlrtrigger_savepoints::createSavepoint() {

	// build a unique savepoint name for this query.  The counter is
	// per-trigger-instance which means per-sqlr-connection, so there's
	// no contention with other sessions.
	spname.clear();
	spname.append(prefix)->append(spcounter++);

	// If this fails, log it but let the user query run anyway - the
	// only consequence is that we won't be able to roll back if the
	// user query fails.
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

		// also release the savepoint after rolling back, so the
		// stack doesn't grow.  Allowed by standard SQL after a
		// "rollback to savepoint".
		if (dorelease) {
			if (!runQuery(releasequery)) {
				debugWrite("failed to release savepoint %s",
							spname.getString());
			}
		}

	} else if (dorelease) {

		// release the savepoint so the stack doesn't grow without
		// bound across many queries in a long-running transaction
		if (!runQuery(releasequery)) {
			debugWrite("failed to release savepoint %s",
							spname.getString());
		}
	}
}

void sqlrtrigger_savepoints::buildSavepointSql(const char *format,
						stringbuffer *out) {

	// replace the first occurrence of "%s" in format with the
	// savepoint name
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

	// Use a dedicated cursor so we don't disturb the user cursor's
	// query buffer, binds, result set, etc.
	sqlrservercursor	*spcur=cont->newCursor();
	if (!spcur) {
		return false;
	}
	if (!cont->open(spcur)) {
		cont->deleteCursor(spcur);
		return false;
	}

	// Run the savepoint query with directives, translations, filters,
	// and triggers all disabled.  Disabling triggers in particular
	// prevents this trigger from recursing into itself - both at
	// prepare time and at execute time.
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

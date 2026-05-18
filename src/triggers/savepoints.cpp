// Copyright (c) David Muse
// All rights reserved

#include <sqlrelay/sqlrserver.h>

class SQLRSERVER_DLLSPEC sqlrtrigger_savepoints : public sqlrtrigger {
	public:
		sqlrtrigger_savepoints(sqlrservercontroller *cont,
						domnode *parameters);

		bool	runBefore(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runAfter(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);

	private:
		bool	shouldSkip(sqlrservercursor *sqlrcur);
		void	buildSavepointSql(const char *format,
						stringbuffer *out);
		bool	runQuery(const char *format);

		const char	*savepointquery;
		const char	*rollbackquery;
		const char	*releasequery;
		bool		dorelease;

		const char	*prefix;
		uint64_t	spcounter;
		stringbuffer	spname;

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
		return true;
	}

	// savepoints only have meaning inside a transaction.  In
	// autocommit mode each statement is its own transaction and any
	// savepoint we create would be destroyed before runAfter could
	// roll back to it.
	if (!cont->getInTransaction()) {
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
		return true;
	}

	return false;
}

bool sqlrtrigger_savepoints::runBefore(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// reset state - if we bail out below, runAfter must not try to
	// roll back to or release a savepoint we didn't actually create
	spactive=false;

	if (shouldSkip(sqlrcur)) {
		return true;
	}

	debugStart("savepoints runBefore");

	// build a unique savepoint name for this query.  The counter is
	// per-trigger-instance which means per-sqlr-connection, so there's
	// no contention with other sessions.
	spname.clear();
	spname.append(prefix)->append(spcounter++);

	// create the savepoint.  If this fails, log it but let the user
	// query run anyway - the only consequence is that we won't be able
	// to roll back if the user query fails.
	if (runQuery(savepointquery)) {
		spactive=true;
	} else {
		debugWrite("failed to create savepoint %s",spname.getString());
	}

	debugEnd();

	return true;
}

bool sqlrtrigger_savepoints::runAfter(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	// bail if runBefore didn't create a savepoint for this query
	if (!spactive) {
		return true;
	}
	spactive=false;

	debugStart("savepoints runAfter");

	// did the user query error?  Match the same conditions other
	// triggers use - either a non-empty error message or a non-zero
	// error number.
	bool	error=(cont->getErrorSize(sqlrcur) ||
				cont->getErrorNumber(sqlrcur));

	if (error) {

		// undo any partial effects of the failed query
		if (!runQuery(rollbackquery)) {
			debugWrite("failed to roll back to savepoint %s",
							spname.getString());
		}

	} else if (dorelease) {

		// release the savepoint so the stack doesn't grow without
		// bound across many queries in a long-running transaction
		if (!runQuery(releasequery)) {
			debugWrite("failed to release savepoint %s",
							spname.getString());
		}
	}

	debugEnd();

	// preserve the user's error - returning true here doesn't clear
	// it, the caller still sees the failure
	return true;
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
	// prevents this trigger from recursing into itself.
	bool	success=cont->prepareQuery(spcur,
					spsql.getString(),spsql.getSize(),
					false,false,false) &&
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

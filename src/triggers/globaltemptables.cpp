// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/regularexpression.h>
#include <rudiments/sensitivevalue.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/dynamicarray.h>


class table_t {
	public:
		table_t();
		~table_t();

		void	setName(const char *name);
		void	attachCreateStatement(char *cs, uint64_t cslen);
		void	setCreated(bool created);

		const char	*getName();
		const char	*getCreateStatement();
		uint64_t	getCreateStatementLength();
		bool		getCreated();

		bool	match(const char *q);
	private:
		const char		*name;
		char			*createstatement;
		uint64_t		createstatementlength;
		regularexpression	re;
		bool			created;
};

table_t::table_t() {
	name=NULL;
	createstatement=NULL;
	createstatementlength=0;
	created=false;
}

table_t::~table_t() {
	delete[] createstatement;
}

void table_t::setName(const char *name) {

	this->name=name;

	// build a regex that matches the table name, as a whole word,
	// anywhere in the query
	// (the query is assumed to be normalized)
	// The boundaries are spelled out as an explicit "not an identifier
	// character" class rather than \b.  Without pcre, rudiments'
	// regularexpression falls back to posix regcomp, which has no \b.
	stringbuffer	pattern;
	pattern.append("(^|[^a-zA-Z0-9_$#])");
	pattern.append(name);
	pattern.append("([^a-zA-Z0-9_$#]|$)");

	re.setPattern(pattern.getString());
	re.study();
}

void table_t::attachCreateStatement(char *cs, uint64_t cslen) {
	delete[] createstatement;
	createstatement=cs;
	createstatementlength=cslen;
}

void table_t::setCreated(bool created) {
	this->created=created;
}

const char *table_t::getName() {
	return name;
}

const char *table_t::getCreateStatement() {
	return createstatement;
}

uint64_t table_t::getCreateStatementLength() {
	return createstatementlength;
}

bool table_t::getCreated() {
	return created;
}

bool table_t::match(const char *q) {
	return re.match(q);
}


class SQLRSERVER_DLLSPEC sqlrtrigger_globaltemptables : public sqlrtrigger {
	public:
		sqlrtrigger_globaltemptables(sqlrservercontroller *cont,
						domnode *parameters);

		bool	runBeforePrepare(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		void	endTransaction(bool commit);
		void	endSession();
	private:
		bool	createTable(uint64_t i, sqlrservercursor *usercur);
		bool	isAlreadyExistsError(sqlrservercursor *ccur);

		dynamicarray<table_t>	tables;

		int64_t		alreadyexistsnum;
		const char	*alreadyexistsstr;
};

sqlrtrigger_globaltemptables::sqlrtrigger_globaltemptables(
					sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrtrigger(cont,parameters) {

	debugStart("globaltemptables config");


	// parse <alreadyexists error="..."/> tag...
	alreadyexistsnum=0;
	alreadyexistsstr=NULL;
	const char	*ae=parameters->getFirstTagChild("alreadyexists")->
						getAttributeValue("error");
	if (charstring::isInteger(ae)) {
		alreadyexistsnum=charstring::convertToInteger(ae);
		debugWrite("alreadyexists: %lld",(long long)alreadyexistsnum);
	} else if (!charstring::isNullOrEmpty(ae)) {
		alreadyexistsstr=ae;
		debugWrite("alreadyexists: \"%s\"",alreadyexistsstr);
	}


	// parse <table> tags...

	// FIXME: we may want to use a different path here...
	sensitivevalue	sv;
	sv.setPath(cont->getConfig()->getPasswordPath());

	uint64_t	i=0;
	for (domnode *t=parameters->getFirstTagChild("table");
				!t->isNullNode();
				t=t->getNextTagSibling("table")) {

		// set the table name
		tables[i].setName(t->getAttributeValue("name"));

		// attach the create statement (support [...filename...])
		sv.parse(t->getFirstChild("text")->getValue());
		uint64_t	cslen=sv.getTextValueLength();
		tables[i].attachCreateStatement(sv.detachTextValue(),cslen);

		debugWrite("table: %s",tables[i].getName());
		debugWrite("create: %s",tables[i].getCreateStatement());

		i++;
	}

	debugEnd();
}

bool sqlrtrigger_globaltemptables::runBeforePrepare(
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur) {

	debugStart("globaltemptables runBeforePrepare");

	// bail if the query was suppressed
	if (cont->getQuerySuppressed(sqlrcur)) {
		return true;
	}

	// bail if there are no configured tables
	if (!tables.getCount()) {
		return true;
	}

	// get the query and bail if there is no query
	const char	*query=cont->getQueryBuffer(sqlrcur);
	if (!query) {
		return true;
	}

	// skip past any leading whitespace and comments
	const char	*q=cont->skipWhitespaceAndComments(query);

	// run through the tables...
	for (uint64_t i=0; i<tables.getCount(); i++) {

		// loop back if we don't have a match
		if (!tables[i].match(q)) {
			continue;
		}

		debugWrite("matched %s",tables[i].getName());

		// loop back if we've already created this table
		// (a query may reference more than one configured table now
		// that the match isn't limited to a single leading insert,
		// so don't stop at the first match)
		if (tables[i].getCreated()) {
			debugWrite("already created");
			continue;
		}

		// create the table
		bool	created=createTable(i,sqlrcur);
		tables[i].setCreated(created);
		if (!created) {
			debugEnd();
			return false;
		}
	}

	debugEnd();

	return true;
}

void sqlrtrigger_globaltemptables::endTransaction(bool commit) {

	// A rollback can undo a "create temp table" issued earlier in the
	// same transaction (eg. postgresql's DDL is transactional) while
	// this trigger still believes the table exists - so a later
	// reference skips the create and fails against a table that isn't
	// there. A commit doesn't undo anything, so only a rollback needs
	// to clear the flags here; runBeforePrepare() re-creates on demand.
	if (commit) {
		return;
	}
	debugStart("globaltemptables endTransaction (rollback)");
	for (uint64_t i=0; i<tables.getCount(); i++) {
		tables[i].setCreated(false);
	}
	debugEnd();
}

void sqlrtrigger_globaltemptables::endSession() {

	// The "created" flag models a permanent, once-created object (which
	// is what a real global temporary table is, on databases like
	// Oracle/DB2 - only the DATA is session-scoped there). But this
	// trigger's own object outlives any one client session - it lives
	// for as long as the sqlr-connection process holds the pooled
	// backend connection, and that connection is reused by many client
	// sessions in turn. On a database where the create statement
	// actually makes a session-scoped OBJECT (eg. postgresql's
	// "create temp table"), the object is gone once the backend session
	// that created it ends, even though this trigger's "created" flag
	// says otherwise. So forget every table here, at the end of each
	// client session, and let runBeforePrepare() re-create on demand.
	//
	// If a table's create statement really does target a permanent
	// object (eg. a real Oracle/DB2 global temporary table), this just
	// costs one extra create-and-catch-"already exists" per table per
	// session - createTable()'s isAlreadyExistsError() handling already
	// treats that as success, so nothing breaks, it's just not free.
	debugStart("globaltemptables endSession");
	for (uint64_t i=0; i<tables.getCount(); i++) {
		tables[i].setCreated(false);
	}
	debugEnd();
}

bool sqlrtrigger_globaltemptables::isAlreadyExistsError(
						sqlrservercursor *ccur) {

	// match by error substring, if one was configured
	if (alreadyexistsstr) {

		// the error buffer may not be terminated, but contains below
		// needs a terminated string, so make a copy of it here
		stringbuffer	err;
		err.append(cont->getErrorBuffer(ccur),cont->getErrorSize(ccur));

		return charstring::contains(err.getString(),alreadyexistsstr);
	}

	// otherwise match by error number
	return cont->getErrorNumber(ccur)==alreadyexistsnum;
}

bool sqlrtrigger_globaltemptables::createTable(uint64_t i,
						sqlrservercursor *usercur) {

	debugWrite("creating table %s",tables[i].getName());
	debugWrite("%s",tables[i].getCreateStatement());

	// Create a separate cursor to run the create rather than reusing
	// the cursor that is about to run the user's query.  This preserves
	// the user cursor's buffers, binds, etc.
	sqlrservercursor	*ccur=cont->newCursor();
	if (!ccur) {
		return false;
	}

	// open the create cursor
	if (!cont->open(ccur)) {
		cont->deleteCursor(ccur);
		return false;
	}

	// prepare and execute the create query
	bool	success=cont->prepareQuery(ccur,
				tables[i].getCreateStatement(),
				tables[i].getCreateStatementLength(),
				false,false,false,false) &&
			cont->executeQuery(ccur,
				false,false,false,false);

	if (!success) {

		debugWrite("create failed: %d - %.*s",
				cont->getErrorNumber(ccur),
				cont->getErrorSize(ccur),
				cont->getErrorBuffer(ccur));

		if (isAlreadyExistsError(ccur)) {

			// handle table-already-exists as success
			debugWrite("table already exists - "
						"treating as success");
			success=true;

		} else {

			// report any other error
			const char	*errorstring;
			uint32_t	errorsize;
			int64_t		errnum;
			bool		liveconnection;
			cont->getError(ccur,&errorstring,&errorsize,
						&errnum,&liveconnection);
			cont->setError(usercur,errorstring,errorsize,
						errnum,liveconnection);
		}
	}

	// clean up
	cont->closeResultSet(ccur);
	cont->close(ccur);
	cont->deleteCursor(ccur);

	return success;
}

extern "C" {
	SQLRSERVER_DLLSPEC
	sqlrtrigger	*new_sqlrtrigger_globaltemptables(
						sqlrservercontroller *cont,
						domnode *parameters) {

		return new sqlrtrigger_globaltemptables(cont,parameters);
	}
}

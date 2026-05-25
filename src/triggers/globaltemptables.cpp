// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/regularexpression.h>
#include <rudiments/sensitivevalue.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/dynamicarray.h>

// Simulates global temporary tables in a database that only supports
// session-local temporary tables.  For each configured table, watches for
// inserts targeting that table at prepare-time and lazily issues the
// configured create statement on the backend session the first time the
// table is referenced.  Once created, the table persists for the life of
// the backend session, which (for the sqlr-connection process model) means
// it appears to behave like a global temporary table across all client
// sessions multiplexed onto that backend session.
//
// The trigger is fired before prepare so that the create statement runs
// before the database tries to parse a reference to a not-yet-created
// table during prepare.

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

	// build an insert regex that handles mysql's weird options
	// (the query is assumed to be normalized)
	stringbuffer	pattern;
	pattern.append("^insert "
			"((low_priority|delayed|high_priority) )?"
			"(ignore )?"
			"(into )?");
	pattern.append(name);
	pattern.append("[ (]");

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

		debugWrite("matched insert into %s",tables[i].getName());

		// bail if we've already created this table
		if (tables[i].getCreated()) {
			debugWrite("already created");
			break;
		}

		// create the table
		bool	created=createTable(i,sqlrcur);
		tables[i].setCreated(created);
		if (!created) {
			debugEnd();
			return false;
		}
		break;
	}

	debugEnd();

	return true;
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
	// the cursor that is about to run the user's insert.  This preserves
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

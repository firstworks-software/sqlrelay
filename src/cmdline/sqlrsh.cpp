// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrclient.h>
#include <sqlrelay/sqlrutil.h>
#include <rudiments/file.h>
#include <rudiments/permissions.h>
#include <rudiments/filesystem.h>
#include <rudiments/filedescriptor.h>
#include <rudiments/process.h>
#include <rudiments/environment.h>
#include <rudiments/datetime.h>
#include <rudiments/signalclasses.h>
#include <rudiments/xmldom.h>
#include <rudiments/stdio.h>
#include <rudiments/character.h>
#include <rudiments/bytestring.h>
#include <rudiments/memorypool.h>
#include <rudiments/prompt.h>
#include <rudiments/locale.h>
#include <rudiments/sensitivevalue.h>
#include <config.h>
#include <defaults.h>
#define NEED_IS_BIT_TYPE_CHAR 1
#define NEED_IS_BOOL_TYPE_CHAR 1
#define NEED_IS_NUMBER_TYPE_CHAR 1
#define NEED_IS_FLOAT_TYPE_CHAR 1
#define NEED_IS_NONSCALE_FLOAT_TYPE_CHAR 1
#include <datatypes.h>
#include <defines.h>
#include <version.h>
#include <math.h>

class sqlrshbindvalue {
	public:
		// A union in C++98 can't hold anything with a constructor, so
		// every member here is a plain type.
		union {
			// string, blob and clob values carry their own
			// length, so an embedded null isn't cut short
			struct {
				char		*value;
				uint32_t	length;
			} stringval;
			int64_t	integerval;
			struct {
				double		value;
				uint32_t	precision;
				uint32_t	scale;
			} doubleval;
			struct {
				int16_t		year;
				int16_t		month;
				int16_t		day;
				int16_t		hour;
				int16_t		minute;
				int16_t		second;
				int32_t		microsecond;
				const char	*tz;
				bool		isnegative;
			} dateval;
		};
		sqlrclientbindvartype_t	type;
		uint32_t		outputstringbindlength;

		void	print() {}
};

static char *duplicateBytes(const char *value, uint32_t length) {

	if (!value) {
		return NULL;
	}

	char	*copy=new char[length+1];
	bytestring::copy(copy,value,length);
	copy[length]='\0';
	return copy;
}

static char *poolCopy(memorypool *pool, const char *value, size_t length) {

	char	*copy=(char *)pool->allocate(length+1);
	bytestring::copy(copy,value,length);
	copy[length]='\0';
	return copy;
}

static void setStringValue(sqlrshbindvalue *bv,
				const char *value,
				uint32_t valuelength,
				uint32_t length) {

	// cut off or null-pad to fit
	char		*buffer=new char[length+1];
	uint32_t	copylength=(valuelength<length)?valuelength:length;
	bytestring::copy(buffer,value,copylength);
	if (copylength<length) {
		bytestring::zero(buffer+copylength,length-copylength);
	}
	buffer[length]='\0';

	bv->type=SQLRCLIENTBINDVARTYPE_STRING;
	bv->stringval.value=buffer;
	bv->stringval.length=length;
}

static void deleteBindValue(sqlrshbindvalue *bv) {

	if (!bv) {
		return;
	}

	if (bv->type==SQLRCLIENTBINDVARTYPE_STRING ||
		bv->type==SQLRCLIENTBINDVARTYPE_BLOB ||
		bv->type==SQLRCLIENTBINDVARTYPE_CLOB) {
		delete[] bv->stringval.value;
	}
	delete bv;
}

enum sqlrshformat {
	SQLRSH_FORMAT_PLAIN=0,
	SQLRSH_FORMAT_CSV,
	SQLRSH_FORMAT_JSON,
	SQLRSH_FORMAT_JSONL
};

// the format names, in one place
struct sqlrshformatname {
	const char	*name;
	sqlrshformat	format;
};

static const sqlrshformatname	sqlrshformatnames[]={
	{"plain",SQLRSH_FORMAT_PLAIN},
	{"csv",SQLRSH_FORMAT_CSV},
	{"json",SQLRSH_FORMAT_JSON},
	{"jsonl",SQLRSH_FORMAT_JSONL},
	{NULL,SQLRSH_FORMAT_PLAIN}
};

static bool formatFromName(const char *name, sqlrshformat *format) {

	if (charstring::isNullOrEmpty(name)) {
		return false;
	}

	for (const sqlrshformatname *fn=sqlrshformatnames; fn->name; fn++) {
		if (!charstring::compareIgnoringCase(name,fn->name)) {
			*format=fn->format;
			return true;
		}
	}
	return false;
}

static void badFormatName(const char *name) {

	stderror.printf("unrecognized format \"%s\", expected ",
					(name)?name:"");
	for (const sqlrshformatname *fn=sqlrshformatnames; fn->name; fn++) {
		if (fn!=sqlrshformatnames) {
			stderror.write('|');
		}
		stderror.write(fn->name);
	}
	stderror.write('\n');
}

// which getFieldAs...() method the result set is fetched with
enum sqlrshfieldsas {
	SQLRSH_FIELDSAS_RAW=0,
	SQLRSH_FIELDSAS_NUMBER,
	SQLRSH_FIELDSAS_BOOLEAN,
	SQLRSH_FIELDSAS_DATE
};

// the mode names, in one place
struct sqlrshfieldsasname {
	const char	*name;
	sqlrshfieldsas	fieldsas;
};

static const sqlrshfieldsasname	sqlrshfieldsasnames[]={
	{"raw",SQLRSH_FIELDSAS_RAW},
	{"number",SQLRSH_FIELDSAS_NUMBER},
	{"boolean",SQLRSH_FIELDSAS_BOOLEAN},
	{"date",SQLRSH_FIELDSAS_DATE},
	{NULL,SQLRSH_FIELDSAS_RAW}
};

static bool fieldsAsFromName(const char *name, sqlrshfieldsas *fieldsas) {

	if (charstring::isNullOrEmpty(name)) {
		return false;
	}

	for (const sqlrshfieldsasname *fn=sqlrshfieldsasnames; fn->name; fn++) {
		if (!charstring::compareIgnoringCase(name,fn->name)) {
			*fieldsas=fn->fieldsas;
			return true;
		}
	}
	return false;
}

static void badFieldsAsName(const char *name) {

	stderror.printf("unrecognized fieldsas mode \"%s\", expected ",
						(name)?name:"");
	for (const sqlrshfieldsasname *fn=sqlrshfieldsasnames; fn->name; fn++) {
		if (fn!=sqlrshfieldsasnames) {
			stderror.write('|');
		}
		stderror.write(fn->name);
	}
	stderror.write('\n');
}

// the json type a converted field goes out as
enum sqlrshjsontype {
	SQLRSH_JSONTYPE_STRING=0,
	SQLRSH_JSONTYPE_NUMBER,
	SQLRSH_JSONTYPE_BOOLEAN
};

static char *commandArgument(const char *args) {

	char	*arg=charstring::duplicate(args);
	charstring::bothTrim(arg);
	if (charstring::isNullOrEmpty(arg)) {
		delete[] arg;
		return NULL;
	}
	return arg;
}

// These are part of the interface.  Add to them, but don't renumber them.
enum sqlrshexitcode {
	SQLRSH_EXIT_SUCCESS=0,
	SQLRSH_EXIT_USAGE=1,
	SQLRSH_EXIT_SCRIPT=2,
	SQLRSH_EXIT_CONNECTION=3,
	SQLRSH_EXIT_QUERY=4
};

class sqlrshenv {
	public:
			sqlrshenv();
			~sqlrshenv();
		void	 clearbinds(
			dictionary<char *, sqlrshbindvalue *> *binds);
		// removes one bind variable.  Returns false if the list
		// didn't have it.
		bool	 clearbind(
			dictionary<char *, sqlrshbindvalue *> *binds,
			const char *variable);
		void	 clearsubstitutions();

		bool		headers;
		bool		divider;
		bool		stats;
		uint64_t	rsbs;
		bool		final;
		bool		autocommit;
		bool		lazyfetch;
		char		delimiter;
		dictionary<char *, sqlrshbindvalue *>	inputbinds;
		memorypool	inbindpool;
		dictionary<char *, sqlrshbindvalue *>	outputbinds;
		dictionary<char *, sqlrshbindvalue *>	inputoutputbinds;
		dictionary<char *, sqlrshbindvalue *>	substitutions;
		// The cursor keeps the names and values it's handed rather
		// than copying them, so a substitution has to outlive the
		// clearsubstitutions that dropped it.
		memorypool	subpool;
		bool		validatebinds;
		char		*cacheto;
		// The connection keeps the socket it's handed rather than
		// copying it, so the one a resumesession was given has to
		// outlive the command that ran it.
		char		*resumesocket;
		sqlrshformat	format;
		sqlrshfieldsas	fieldsas;
		bool		noelapsed;
		bool		nextresultset;
		bool		txqueries;
		bool		continueonerror;
		// the number of statements that failed in the script or the
		// -command list
		uint64_t	errorcount;
		// stdin piped in with no -script, -command or -query: no
		// banner, no prompts, and a failed statement counts like one
		// does in -script or -command
		bool		batch;
};

sqlrshenv::sqlrshenv() {
	headers=true;
	divider=true;
	stats=true;
	rsbs=100;
	final=false;
	autocommit=false;
	lazyfetch=false;
	delimiter=';';
	validatebinds=false;
	cacheto=NULL;
	resumesocket=NULL;
	format=SQLRSH_FORMAT_PLAIN;
	fieldsas=SQLRSH_FIELDSAS_RAW;
	noelapsed=false;
	nextresultset=false;
	txqueries=false;
	continueonerror=false;
	errorcount=0;
	batch=false;
	inputbinds.setManageArrayKeys(true);
	outputbinds.setManageArrayKeys(true);
	inputoutputbinds.setManageArrayKeys(true);
}

sqlrshenv::~sqlrshenv() {
	clearbinds(&inputbinds);
	clearbinds(&outputbinds);
	clearbinds(&inputoutputbinds);
	clearsubstitutions();
	delete[] cacheto;
	delete[] resumesocket;
}

void sqlrshenv::clearbinds(dictionary<char *, sqlrshbindvalue *> *binds) {

	for (listnode<char *> *node=binds->getKeys()->getFirst();
						node; node=node->getNext()) {
		deleteBindValue(binds->getValue(node->getValue()));
	}
	binds->clear();
	inbindpool.clear();
}

bool sqlrshenv::clearbind(dictionary<char *, sqlrshbindvalue *> *binds,
						const char *variable) {

	sqlrshbindvalue	*bv=NULL;
	if (!binds->getValue((char *)variable,&bv)) {
		return false;
	}

	// the list frees the name, but not the value
	deleteBindValue(bv);
	return binds->remove((char *)variable);
}

void sqlrshenv::clearsubstitutions() {

	// Only the values are freed here.  The names and the strings they
	// point at came out of subpool, which the cursor is still holding
	// references into.
	for (listnode<char *> *node=substitutions.getKeys()->getFirst();
						node; node=node->getNext()) {
		delete substitutions.getValue(node->getValue());
	}
	substitutions.clear();
}

enum querytype_t {
	SHOW_DATABASES_QUERY=0,
	SHOW_TABLES_QUERY,
	SHOW_COLUMNS_QUERY,
	SHOW_PRIMARY_KEYS_QUERY,
	DESCRIBE_QUERY
};

class	sqlrsh {
	public:
			sqlrsh();
			~sqlrsh();
		// returns one of the sqlrshexitcode's
		int32_t	execute(int argc, const char **argv);
	private:
		// returns the on/off value of command line option "arg", or
		// "defaultvalue" if the option wasn't given.  An option given
		// without a value means on.
		bool	onOffOption(const char *arg, bool defaultvalue);
		void	startupMessage(sqlrshenv *env,
					const char *host,
					uint16_t port,
					const char *user);
		void	userRcFile(sqlrconnection *sqlrcon, 
					sqlrcursor *sqlrcur, 
					sqlrshenv *env);
		// counts a command that failed, unless a script nested
		// inside it already counted what failed in there.
		// "errorcount" is env's count from before the command ran.
		void	countError(sqlrshenv *env, uint64_t errorcount);
		// returns true if the run should carry on past a command
		// that failed
		bool	continueOnError(sqlrconnection *sqlrcon,
					sqlrshenv *env);
		// writes the number of failed statements to stderr, unless
		// nothing failed or the run stopped at its first failure,
		// and makes a run with any of them exit SQLRSH_EXIT_QUERY
		void	reportErrorCount(sqlrshenv *env, int32_t *exitcode);
		// returns SQLRSH_EXIT_SUCCESS, SQLRSH_EXIT_SCRIPT if the
		// file couldn't be opened, or SQLRSH_EXIT_QUERY if one of
		// the commands in it failed
		int32_t	runScript(sqlrconnection *sqlrcon,
					sqlrcursor *sqlrcur,
					sqlrshenv *env,
					const char *filename,
					bool displayerror);
		bool	runCommands(sqlrconnection *sqlrcon,
					sqlrcursor *sqlrcur,
					sqlrshenv *env, 
					const char *commands,
					bool *exitprogram);
		bool	getCommandFromFileOrString(file *fl,
					const char *string,
					const char **stringpos,
					stringbuffer *cmdbuffer,
					sqlrshenv *env);
		bool	runCommand(sqlrconnection *sqlrcon, 
					sqlrcursor *sqlrcur, 
					sqlrshenv *env,
					const char *command,
					bool *exitprogram);
		int	commandType(const char *command);
		bool	internalCommand(sqlrconnection *sqlrcon,
					sqlrcursor *sqlrcur,
					sqlrshenv *env,
					const char *command);
		bool	externalCommand(sqlrconnection *sqlrcon,
					sqlrcursor *sqlrcur,
					sqlrshenv *env, 
					const char *command);
		void	executeQuery(sqlrcursor *sqlrcur,
					sqlrshenv *env);
		char	*getWild(const char *command);
		char	*getTable(const char *command, bool in);
		char	*getProcedure(const char *command);
		char	*getType(const char *command);
		void	initStats(sqlrshenv *env);
		void	displayError(sqlrshenv *env,
					const char *message,
					const char *error,
					int64_t errornumber);
		void	displayHeader(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	plainDisplayHeader(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	csvDisplayHeader(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	jsonDisplayHeader(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	csvWriteField(const char *field, uint32_t length);
		bool	csvFieldNeedsQuotes(const char *field,
						uint32_t length);
		void	csvEscapeField(const char *field, uint32_t length);
		// writes "str" as a json string, in double quotes, escaped.
		// A NULL "str" is written as an empty string.  These take
		// the destination because errors go to stderr and everything
		// else goes to stdout.
		void	jsonWriteString(filedescriptor *fd,
						const char *str,
						uint32_t length);
		void	jsonEscapeString(filedescriptor *fd,
						const char *str,
						uint32_t length);
		// writes "field" as a json value.  A NULL "field" is a
		// database null, and is written as the json null literal.
		// SQLRSH_JSONTYPE_NUMBER asks for a bare json number, which
		// it gets if it really looks like one, and a json string if
		// it doesn't.
		void	jsonWriteValue(filedescriptor *fd,
						const char *field,
						uint32_t length,
						sqlrshjsontype jsontype);
		// fetches field "col" of row "row", applying the fieldsas
		// conversion.  Sets "*length" to the field's length and
		// "*jsontype" to the json type the result goes out as.
		// Returns NULL for a database null.  "convbuffer" is scratch
		// space the conversion writes into, so it has to outlive the
		// returned pointer.
		const char *getFieldForDisplay(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						uint64_t row, uint32_t col,
						uint32_t *length,
						char *convbuffer,
						size_t convbuffersize,
						sqlrshjsontype *jsontype);
		// writes the broken-down date "getFieldAsDate" handed back
		// into "buffer", as [-]YYYY-MM-DD HH:MM:SS[.uuuuuu], leaving
		// out the parts the field didn't carry.  Returns false if it
		// carried neither a date nor a time.
		bool	formatFieldAsDate(char *buffer, size_t buffersize,
						int16_t year, int16_t month,
						int16_t day, int16_t hour,
						int16_t minute, int16_t second,
						int32_t microsecond,
						bool isnegative);
		// returns the width plain format pads column "col" to, which
		// the header, the divider and the rows all have to agree on
		uint32_t	plainColumnWidth(sqlrcursor *sqlrcur,
						sqlrshenv *env, uint32_t col);
		void	displayResultSet(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	plainDisplayResultSet(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	csvDisplayResultSet(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	jsonDisplayResultSet(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	displayStats(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		// writes the stats as json object members, without the
		// enclosing braces, so json can put them in the document and
		// jsonl in an object of its own
		void	jsonWriteStats(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		// writes the header, the result set and the stats for the
		// result set the cursor is holding
		void	displayCurrentResultSet(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		// writes every piece of column metadata the client api
		// reports about the current result set
		void	columninfo(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	plainColumnInfo(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	csvColumnInfo(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		void	jsonColumnInfo(sqlrcursor *sqlrcur,
						sqlrshenv *env);
		// writes the result of an internal command that produces a
		// single value.  plain and csv write the value by itself,
		// json and jsonl write a one line object keyed by "name".
		void	writeScalar(sqlrshenv *env,
						const char *name,
						const char *value);
		void	writeScalarNumber(sqlrshenv *env,
						const char *name,
						int64_t value);
		void	writeScalarBoolean(sqlrshenv *env,
						const char *name,
						bool value);
		// writes a timeout, as seconds and microseconds, or as -1
		// when it's disabled
		void	writeTimeout(sqlrshenv *env,
						const char *name,
						int32_t sec,
						int32_t usec);
		// writes what a timeout-setting command produces, in the
		// same seconds and microseconds form writeTimeout() uses.
		// "label" heads the plain sentence, "name" keys the json
		// object.
		void	writeTimeoutSet(sqlrshenv *env,
						const char *name,
						const char *label,
						uint32_t sec,
						uint32_t usec);
		// writes the result of a command that asked the connection
		// for a string.  A NULL with an error behind it is the
		// error, a NULL without one is a null.
		bool	writeConnectionString(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *name,
						const char *value);
		bool	ping(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	identify(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	dbversion(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	dbhostname(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	dbipaddress(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	bindformat(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	nextvalformat(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	getisolationlevel(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	usecatalog(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		bool	useschema(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		bool	resumesession(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		bool	bindvariabledelimiters(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		bool	bindvariabledelimitersupported(
						sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		bool	databasefeature(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		// with an argument it turns column info on or off, without
		// one it dumps the metadata
		bool	columninfocommand(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *args);
		bool	columncase(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *args);
		bool	resumeresultset(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *args);
		bool	resumecachedresultset(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *args);
		void	clientversion(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	serverversion(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		bool	lastinsertid(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		// writes the usage for a command that was given bad or
		// missing arguments.  plain and csv get the lines on stderr,
		// the way they always have.  json and jsonl get an error
		// object, the same shape a failed query gives them.  Always
		// returns false, so a caller can return what this returns.
		bool	usageError(sqlrshenv *env, const char *usage);
		bool	inputbind(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command);
		// the body of the inputbindblob and inputbindclob commands.
		// "name" is the command word, for the usage message, and
		// "type" is the bind type it defines.
		bool	inputbindlob(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command,
						const char *name,
						sqlrclientbindvartype_t type);
		bool	outputbind(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command);
		bool	inputoutputbind(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command);
		// fetches from a cursor that came back as an output bind and
		// writes the result set
		bool	fetchfrombindcursor(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *args);
		// with an argument it turns bind validation on or off,
		// without one it turns it on
		bool	validatebinds(sqlrshenv *env, const char *args);
		bool	validbind(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *args);
		bool	substitution(sqlrshenv *env, const char *args);
		// prepares the query in a file, and runs it too if "execute"
		// is set, with the binds and substitutions applied
		bool	filequery(sqlrconnection *sqlrcon,
						sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *args,
						bool execute);
		void	printbinds(const char *type,
				dictionary<char *, sqlrshbindvalue *> *binds);
		// writes one bind variable list in the selected format.
		// "type" heads the plain block, "key" keys the json object.
		void	printbindlist(sqlrshenv *env,
				const char *type,
				const char *key,
				dictionary<char *, sqlrshbindvalue *> *binds);
		// writes one member of the printbinds object, "key" mapped
		// to an object keyed by bind variable name
		void	jsonPrintBinds(const char *key,
				dictionary<char *, sqlrshbindvalue *> *binds);
		void	clearbinds(
				dictionary<char *, sqlrshbindvalue *> *binds);
		// clears the bind variable named in "args", or the whole
		// list if "args" doesn't name one.  "name" is the command
		// word, for the error message.
		bool	clearbindcommand(sqlrshenv *env,
				const char *name,
				dictionary<char *, sqlrshbindvalue *> *binds,
				const char *args);
		void	setclientinfo(sqlrconnection *sqlrcon,
						const char *command);
		void	getclientinfo(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		void	delimiter(sqlrshenv *env);
		void	autocommit(sqlrshenv *env, bool on);
		bool	connectTimeout(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		bool	responseTimeout(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		bool	cache(sqlrshenv *env, sqlrcursor *sqlrcur,
							const char *command);
		bool	openCache(sqlrshenv *env, sqlrcursor *sqlrcur,
							const char *command);
		void	displayHelp(sqlrshenv *env);
		void	interactWithUser(sqlrconnection *sqlrcon,
						sqlrcursor *sqlrcur,
						sqlrshenv *env);

		sqlrcmdline	*cmdline;
		sqlrpaths	*sqlrpth;

		datetime	start;

		prompt		pr;
};

sqlrsh::sqlrsh() {
	cmdline=NULL;
	sqlrpth=NULL;
}

sqlrsh::~sqlrsh() {
	delete cmdline;
	delete sqlrpth;
}

void sqlrsh::userRcFile(sqlrconnection *sqlrcon, sqlrcursor *sqlrcur, 
						sqlrshenv *env) {

	// get user's home directory
	const char	*home=environment::getValue("HOME");
	if (!home) {
		home="~";
	}

	// build rcfilename
	size_t	userrcfilelen=charstring::getLength(home)+10+1;
	char	*userrcfile=new char[userrcfilelen];
	charstring::copy(userrcfile,home);
	charstring::append(userrcfile,"/.sqlrshrc");

	// process the file
	// (a failing command in the rc file doesn't change the exit status,
	// so it doesn't count toward the failure count either)
	uint64_t	errorcount=env->errorcount;
	runScript(sqlrcon,sqlrcur,env,userrcfile,false);
	env->errorcount=errorcount;
	delete[] userrcfile;
}

void sqlrsh::countError(sqlrshenv *env, uint64_t errorcount) {

	// the run command runs a script, and that script already counted
	// whatever failed inside it
	if (env->errorcount>errorcount) {
		return;
	}
	env->errorcount++;
}

bool sqlrsh::continueOnError(sqlrconnection *sqlrcon, sqlrshenv *env) {

	if (!env->continueonerror) {
		return false;
	}

	// a statement that took the session down with it stops the run anyway
	// (ping() reopens the session, so a false here means the server is
	// gone, not that the last statement closed the session)
	return sqlrcon->ping();
}

void sqlrsh::reportErrorCount(sqlrshenv *env, int32_t *exitcode) {

	if (!env->errorcount) {
		return;
	}

	// a statement that failed is a failed run, whatever the loop that ran
	// it handed back
	*exitcode=SQLRSH_EXIT_QUERY;

	// a run that stopped at its first failure has a count of 1, which the
	// exit code already said
	// (a higher count with continueonerror off means it was on earlier
	// and the run carried on past a failure)
	if (!env->continueonerror && env->errorcount<2) {
		return;
	}
	stderror.printf("statements failed: %lld\n",
				(long long)env->errorcount);
}

int32_t sqlrsh::runScript(sqlrconnection *sqlrcon, sqlrcursor *sqlrcur,
			sqlrshenv *env, const char *filename,
			bool displayerror) {

	int32_t	exitcode=SQLRSH_EXIT_SUCCESS;

	char	*trimmedfilename=charstring::duplicate(filename);
	charstring::bothTrim(trimmedfilename);

	// open the file
	file	scriptfile;
	if (scriptfile.open(trimmedfilename,O_RDONLY)) {

		// optimize
		scriptfile.setReadBufferSize(
			filesystem::getOptimumTransferBlockSize(
							trimmedfilename));

		for (;;) {

			// get a command
			// getCommandFromFileOrString() returns false at the
			// end of the file, which is not an error
			stringbuffer	command;
			if (!getCommandFromFileOrString(
					&scriptfile,NULL,NULL,&command,env)) {
				break;
			}

			// run the command
			uint64_t	errorcount=env->errorcount;
			if (!runCommand(sqlrcon,sqlrcur,env,
						command.getString(),
						NULL)) {
				exitcode=SQLRSH_EXIT_QUERY;
				countError(env,errorcount);
				if (!continueOnError(sqlrcon,env)) {
					break;
				}
			}
		}

		// close the file
		scriptfile.close();

	} else {

		// error message
		if (displayerror) {
			stderror.printf("Couldn't open file: %s\n\n",
							trimmedfilename);
		}
		exitcode=SQLRSH_EXIT_SCRIPT;
	}

	delete[] trimmedfilename;

	return exitcode;
}

bool sqlrsh::runCommands(sqlrconnection *sqlrcon,
				sqlrcursor *sqlrcur, 
				sqlrshenv *env,
				const char *commands,
				bool *exitprogram) {

	bool		retval=true;
	const char	*nextcommand=commands;
	for (;;) {
		stringbuffer	command;
		if (!getCommandFromFileOrString(NULL,
						nextcommand,
						&nextcommand,
						&command,
						env)) {
			break;
		}
		uint64_t	errorcount=env->errorcount;
		if (!runCommand(sqlrcon,sqlrcur,env,
					command.getString(),
					exitprogram)) {
			retval=false;
			countError(env,errorcount);
			if (!continueOnError(sqlrcon,env)) {
				break;
			}
		}
	}
	return retval;
}

bool sqlrsh::getCommandFromFileOrString(file *fl,
					const char *string,
					const char **stringpos,
					stringbuffer *cmdbuffer,
					sqlrshenv *env) {

	bool	ininitialwhitespace=true;
	bool	insinglequotes=false;
	bool	indoublequotes=false;
	char	ch;
	
	for (;;) {

		// get a character from the file or string
		if (fl) {
			if (fl->read(&ch)!=sizeof(ch)) {
				// end of the command...
				// only return false if we're at the
				// beginning, prior to any actual command
				return !ininitialwhitespace;
			}
		} else {
			if (!*string) {
				// end of the command...
				// only return false if we're at the
				// beginning, prior to any actual command
				if (stringpos) {
					*stringpos=string;
				}
				return !ininitialwhitespace;
			}
			ch=*string;
			string++;
		}

		// skip whitespace at the beginning
		if (ininitialwhitespace) {
			if (character::isWhitespace(ch)) {
				continue;
			}
			ininitialwhitespace=false;
		}

		// handle single-quoted strings, with escaping
		if (ch=='\'') {
			if (insinglequotes) {
				cmdbuffer->append(ch);
				if (fl) {
					if (fl->read(&ch)!=sizeof(ch)) {
						return true;
					}
				} else {
					if (!*string) {
						if (stringpos) {
							*stringpos=string;
						}
						return true;
					}
					ch=*string;
					string++;
				}
				// if we didn't get 2 single-quotes in a row
				// while already inside of single-quotes, then
				// we're no longer inside of single-quotes
				if (ch!='\'') {
					insinglequotes=false;
				}
			} else {
				insinglequotes=true;
			}
		}

		// handle double-quoted strings, with escaping
		if (ch=='"') {
			if (indoublequotes) {
				cmdbuffer->append(ch);
				if (fl) {
					if (fl->read(&ch)!=sizeof(ch)) {
						return true;
					}
				} else {
					if (!*string) {
						if (stringpos) {
							*stringpos=string;
						}
						return true;
					}
					ch=*string;
					string++;
				}
				// if we didn't get 2 double-quotes in a row
				// while already inside of double-quotes, then
				// we're no longer inside of double-quotes
				if (ch!='"') {
					indoublequotes=false;
				}
			} else {
				indoublequotes=true;
			}
		}

		// look for an end of command delimiter
		if (!insinglequotes && !indoublequotes && ch==env->delimiter) {
			if (string && stringpos) {
				*stringpos=string;
			}
			return true;
		}

		// write character to buffer and move on
		cmdbuffer->append(ch);
	}
}

bool sqlrsh::runCommand(sqlrconnection *sqlrcon,
					sqlrcursor *sqlrcur, 
					sqlrshenv *env,
					const char *command,
					bool *exitprogram) {

	int	cmdtype=commandType(command);
	if (exitprogram) {
		*exitprogram=false;
	}

	// init stats
	initStats(env);

	if (cmdtype>0) {
		// if the command an internal command, run it as one
		return internalCommand(sqlrcon,sqlrcur,env,command);
	} else if (cmdtype==0) {
		// if the command is not an internal command, 
		// execute it as a query and display the result set
		return externalCommand(sqlrcon,sqlrcur,env,command);
	}

	// exit
	if (exitprogram) {
		*exitprogram=true;
	}
	return true;
}

int sqlrsh::commandType(const char *command) {

	// skip white space
	char	*ptr=(char *)command;
	while (*ptr==' ' || *ptr=='	' || *ptr=='\n') {
		ptr++;
	}

	// compare to known internal commands
	if (!charstring::compareIgnoringCase(ptr,"headers",7) ||
		!charstring::compareIgnoringCase(ptr,"divider",7) ||
		!charstring::compareIgnoringCase(ptr,"stats",5) ||
		!charstring::compareIgnoringCase(ptr,"format",6) ||
		!charstring::compareIgnoringCase(ptr,"debug",5) ||
		!charstring::compareIgnoringCase(ptr,"nullsasnulls",12) ||
		!charstring::compareIgnoringCase(ptr,"autocommit",10) ||
		!charstring::compareIgnoringCase(ptr,"final",5) ||
		!charstring::compareIgnoringCase(ptr,"getasnumber",11) ||
		!charstring::compareIgnoringCase(ptr,"fieldsas",8) ||
		!charstring::compareIgnoringCase(ptr,"noelapsed",9) ||
		!charstring::compareIgnoringCase(ptr,"nextresultset",13) ||
		!charstring::compareIgnoringCase(ptr,"quiet",5) ||
		!charstring::compareIgnoringCase(ptr,"help") ||
		!charstring::compareIgnoringCase(ptr,"ping") ||
		!charstring::compareIgnoringCase(ptr,"identify") ||
		!charstring::compareIgnoringCase(ptr,"dbversion") ||
		!charstring::compareIgnoringCase(ptr,"dbhostname") ||
		!charstring::compareIgnoringCase(ptr,"dbipaddress") ||
		!charstring::compareIgnoringCase(ptr,"bindformat") ||
		!charstring::compareIgnoringCase(ptr,"nextvalformat") ||
		!charstring::compareIgnoringCase(ptr,"isolationlevel",14) ||
		!charstring::compareIgnoringCase(ptr,"clientversion") ||
		!charstring::compareIgnoringCase(ptr,"serverversion") ||
		!charstring::compareIgnoringCase(ptr,"use ",4) ||
		!charstring::compareIgnoringCase(ptr,"usecatalog",10) ||
		!charstring::compareIgnoringCase(ptr,"useschema",9) ||
		!charstring::compareIgnoringCase(ptr,"currentdb") ||
		!charstring::compareIgnoringCase(ptr,"currentcatalog") ||
		!charstring::compareIgnoringCase(ptr,"currentschema") ||
		!charstring::compareIgnoringCase(ptr,"currentuser") ||
		!charstring::compareIgnoringCase(ptr,"databaseisschema") ||
		!charstring::compareIgnoringCase(ptr,"databasefeature",15) ||
		!charstring::compareIgnoringCase(ptr,"getautocommit") ||
		!charstring::compareIgnoringCase(ptr,"intransaction") ||
		!charstring::compareIgnoringCase(ptr,"transactionmodel",16) ||
		!charstring::compareIgnoringCase(
					ptr,"defaulttransactionmodel") ||
		!charstring::compareIgnoringCase(
					ptr,"defaultisolationlevel") ||
		!charstring::compareIgnoringCase(ptr,"suspendsession") ||
		!charstring::compareIgnoringCase(ptr,"resumesession",13) ||
		!charstring::compareIgnoringCase(ptr,"connectionport") ||
		!charstring::compareIgnoringCase(ptr,"connectionsocket") ||
		!charstring::compareIgnoringCase(ptr,"connect timeout",15) ||
		!charstring::compareIgnoringCase(ptr,"getconnecttimeout") ||
		!charstring::compareIgnoringCase(ptr,"getresponsetimeout") ||
		// this one covers bindvariabledelimitersupported too
		!charstring::compareIgnoringCase(
					ptr,"bindvariabledelimiters",22) ||
		!charstring::compareIgnoringCase(ptr,"getdebug") ||
		// this one covers "columninfo on|off" too
		!charstring::compareIgnoringCase(ptr,"columninfo",10) ||
		!charstring::compareIgnoringCase(ptr,"columncase",10) ||
		!charstring::compareIgnoringCase(ptr,"suspendresultset") ||
		!charstring::compareIgnoringCase(ptr,"resultsetid") ||
		!charstring::compareIgnoringCase(
					ptr,"resumecachedresultset",21) ||
		!charstring::compareIgnoringCase(ptr,"resumeresultset",15) ||
		!charstring::compareIgnoringCase(ptr,"closeresultset") ||
		!charstring::compareIgnoringCase(ptr,"cacheoff") ||
		!charstring::compareIgnoringCase(ptr,"cachefilename") ||
		!charstring::compareIgnoringCase(ptr,"totalrows") ||
		!charstring::compareIgnoringCase(ptr,"firstrowindex") ||
		!charstring::compareIgnoringCase(ptr,"endofresultset") ||
		!charstring::compareIgnoringCase(ptr,"run",3) ||
		!charstring::compareIgnoringCase(ptr,"@",1) ||
		!charstring::compareIgnoringCase(ptr,"delimiter",9) ||
		!charstring::compareIgnoringCase(ptr,"delimeter",9) ||
		!charstring::compareIgnoringCase(ptr,"inputbind ",10) ||
		!charstring::compareIgnoringCase(ptr,"inputbindblob ",14) ||
		!charstring::compareIgnoringCase(ptr,"inputbindclob ",14) ||
		!charstring::compareIgnoringCase(ptr,"outputbind ",11) ||
		!charstring::compareIgnoringCase(ptr,"inputoutputbind ",16) ||
		// the bind commands take a trailing space above, so these
		// catch the command word on its own
		!charstring::compareIgnoringCase(ptr,"inputbind") ||
		!charstring::compareIgnoringCase(ptr,"inputbindblob") ||
		!charstring::compareIgnoringCase(ptr,"inputbindclob") ||
		!charstring::compareIgnoringCase(ptr,"outputbind") ||
		!charstring::compareIgnoringCase(ptr,"inputoutputbind") ||
		!charstring::compareIgnoringCase(
					ptr,"fetchfrombindcursor",19) ||
		!charstring::compareIgnoringCase(ptr,"countbindvariables") ||
		!charstring::compareIgnoringCase(ptr,"validatebinds",13) ||
		!charstring::compareIgnoringCase(ptr,"validbind",9) ||
		!charstring::compareIgnoringCase(ptr,"substitution",12) ||
		!charstring::compareIgnoringCase(ptr,"clearsubstitutions") ||
		!charstring::compareIgnoringCase(ptr,"preparefilequery",16) ||
		!charstring::compareIgnoringCase(ptr,"filequery",9) ||
		!charstring::compareIgnoringCase(ptr,"printinputbind",14) ||
		!charstring::compareIgnoringCase(ptr,"printoutputbind",15) ||
		!charstring::compareIgnoringCase(
					ptr,"printinputoutputbind",20) ||
		!charstring::compareIgnoringCase(ptr,"printbinds") ||
		!charstring::compareIgnoringCase(ptr,"clearinputbind",14) ||
		!charstring::compareIgnoringCase(ptr,"clearoutputbind",15) ||
		!charstring::compareIgnoringCase(
					ptr,"clearinputoutputbind",20) ||
		!charstring::compareIgnoringCase(ptr,"clearbinds") ||
		!charstring::compareIgnoringCase(ptr,"lastinsertid") ||
		!charstring::compareIgnoringCase(ptr,"setclientinfo ",14) ||
		!charstring::compareIgnoringCase(ptr,"getclientinfo") ||
		!charstring::compareIgnoringCase(ptr,
					"setresultsetbuffersize ",23) ||
		!charstring::compareIgnoringCase(ptr,
					"getresultsetbuffersize") ||
		!charstring::compareIgnoringCase(ptr,"lazyfetch",9) ||
		!charstring::compareIgnoringCase(ptr,"endsession") ||
		!charstring::compareIgnoringCase(ptr,"querytree") ||
		!charstring::compareIgnoringCase(ptr,"translatedquery") ||
		!charstring::compareIgnoringCase(ptr,"response timeout",16) ||
		!charstring::compareIgnoringCase(ptr,"cache ",6) ||
		!charstring::compareIgnoringCase(ptr,"opencache ",10) ||
		!charstring::compareIgnoringCase(ptr,"txqueries",9) ||
		!charstring::compareIgnoringCase(ptr,"continueonerror",15)) {

		// return value of 1 is internal command
		return 1;
	}

	// look for an exit command
	if (!charstring::compareIgnoringCase(ptr,"quit",4) ||
		!charstring::compareIgnoringCase(ptr,"exit",4)) {
		return -1;
	}

	// return value of 0 is external command
	return 0;
}

bool sqlrsh::internalCommand(sqlrconnection *sqlrcon, sqlrcursor *sqlrcur,
					sqlrshenv *env, const char *command) {

	// skip white space
	char	*ptr=(char *)command;
	while (*ptr==' ' || *ptr=='	' || *ptr=='\n') {
		ptr++;
	}

	// compare to known internal commands
	int	cmdtype=0;
	if (!charstring::compareIgnoringCase(ptr,"headers",7)) {
		ptr=ptr+7;
		cmdtype=2;
	} else if (!charstring::compareIgnoringCase(ptr,"divider",7)) {
		ptr=ptr+7;
		cmdtype=11;
	} else if (!charstring::compareIgnoringCase(ptr,"stats",5)) {	
		ptr=ptr+5;
		cmdtype=3;
	} else if (!charstring::compareIgnoringCase(ptr,"format",6)) {	
		ptr=ptr+6;
		cmdtype=10;
	} else if (!charstring::compareIgnoringCase(ptr,"debug",5)) {	
		ptr=ptr+5;
		cmdtype=4;
	} else if (!charstring::compareIgnoringCase(ptr,"nullsasnulls",12)) {	
		ptr=ptr+12;
		cmdtype=9;
	} else if (!charstring::compareIgnoringCase(ptr,"autocommit",10)) {	
		ptr=ptr+10;
		cmdtype=8;
	} else if (!charstring::compareIgnoringCase(ptr,"final",5)) {	
		ptr=ptr+5;
		cmdtype=5;
	} else if (!charstring::compareIgnoringCase(ptr,"getasnumber",11)) {
		ptr=ptr+11;
		cmdtype=14;
	} else if (!charstring::compareIgnoringCase(ptr,"fieldsas",8)) {
		ptr=ptr+8;
		cmdtype=19;
	} else if (!charstring::compareIgnoringCase(ptr,"noelapsed",9)) {
		ptr=ptr+9;
		cmdtype=15;
	} else if (!charstring::compareIgnoringCase(ptr,"nextresultset",13)) {
		ptr=ptr+13;
		cmdtype=16;
	} else if (!charstring::compareIgnoringCase(ptr,"quiet",5)) {
		ptr=ptr+5;
		cmdtype=17;
	} else if (!charstring::compareIgnoringCase(ptr,"help")) {	
		displayHelp(env);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"ping")) {	
		return ping(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"use ",4)) {	
		if (!sqlrcon->selectDatabase(ptr+4)) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"currentdb")) {
		const char	*currentdb=sqlrcon->getCurrentDatabase();
		if (!currentdb && sqlrcon->errorMessage()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		writeScalar(env,"currentdb",currentdb);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"currentschema")) {
		const char	*currentschema=sqlrcon->getCurrentSchema();
		if (!currentschema && sqlrcon->errorMessage()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		writeScalar(env,"currentschema",currentschema);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"currentuser")) {
		const char	*currentuser=sqlrcon->getCurrentUser();
		if (!currentuser && sqlrcon->errorMessage()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		writeScalar(env,"currentuser",currentuser);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"usecatalog",10)) {
		return usecatalog(sqlrcon,env,ptr+10);
	} else if (!charstring::compareIgnoringCase(ptr,"useschema",9)) {
		return useschema(sqlrcon,env,ptr+9);
	} else if (!charstring::compareIgnoringCase(ptr,"currentcatalog")) {
		return writeConnectionString(sqlrcon,env,"currentcatalog",
					sqlrcon->getCurrentCatalog());
	} else if (!charstring::compareIgnoringCase(ptr,"databaseisschema")) {
		writeScalarBoolean(env,"databaseisschema",
					sqlrcon->getDatabaseIsSchema());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"databasefeature",15)) {
		return databasefeature(sqlrcon,env,ptr+15);
	} else if (!charstring::compareIgnoringCase(ptr,"getautocommit")) {
		writeScalarBoolean(env,"getautocommit",
					sqlrcon->getAutoCommit());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"intransaction")) {
		writeScalarBoolean(env,"intransaction",
					sqlrcon->getInTransaction());
		return true;
	} else if (!charstring::compareIgnoringCase(
					ptr,"transactionmodel ",17)) {
		if (!sqlrcon->setTransactionModel(ptr+17)) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"transactionmodel")) {
		return writeConnectionString(sqlrcon,env,"transactionmodel",
					sqlrcon->getTransactionModel());
	} else if (!charstring::compareIgnoringCase(
					ptr,"defaulttransactionmodel")) {
		return writeConnectionString(sqlrcon,env,
					"defaulttransactionmodel",
					sqlrcon->getDefaultTransactionModel());
	} else if (!charstring::compareIgnoringCase(
					ptr,"defaultisolationlevel")) {
		return writeConnectionString(sqlrcon,env,
					"defaultisolationlevel",
					sqlrcon->getDefaultIsolationLevel());
	} else if (!charstring::compareIgnoringCase(ptr,"suspendsession")) {
		if (!sqlrcon->suspendSession()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"resumesession",13)) {
		return resumesession(sqlrcon,env,ptr+13);
	} else if (!charstring::compareIgnoringCase(ptr,"connectionport")) {
		writeScalarNumber(env,"connectionport",
				(int64_t)sqlrcon->getConnectionPort());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"connectionsocket")) {
		return writeConnectionString(sqlrcon,env,"connectionsocket",
					sqlrcon->getConnectionSocket());
	} else if (!charstring::compareIgnoringCase(ptr,"connect timeout",15)) {
		return connectTimeout(sqlrcon,env,ptr+15);
	} else if (!charstring::compareIgnoringCase(ptr,"getconnecttimeout")) {
		writeTimeout(env,"getconnecttimeout",
				sqlrcon->getConnectTimeoutSeconds(),
				sqlrcon->getConnectTimeoutMicroseconds());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"getresponsetimeout")) {
		writeTimeout(env,"getresponsetimeout",
				sqlrcon->getResponseTimeoutSeconds(),
				sqlrcon->getResponseTimeoutMicroseconds());
		return true;
	} else if (!charstring::compareIgnoringCase(
				ptr,"bindvariabledelimitersupported",30)) {
		// this one comes first, because the command below is a
		// prefix of it
		return bindvariabledelimitersupported(sqlrcon,env,ptr+30);
	} else if (!charstring::compareIgnoringCase(
					ptr,"bindvariabledelimiters",22)) {
		return bindvariabledelimiters(sqlrcon,env,ptr+22);
	} else if (!charstring::compareIgnoringCase(ptr,"getdebug")) {
		writeScalarBoolean(env,"getdebug",sqlrcon->getDebug());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"columninfo",10)) {
		return columninfocommand(sqlrcur,env,ptr+10);
	} else if (!charstring::compareIgnoringCase(ptr,"columncase",10)) {
		return columncase(sqlrcur,env,ptr+10);
	} else if (!charstring::compareIgnoringCase(ptr,"suspendresultset")) {
		sqlrcur->suspendResultSet();
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"resultsetid")) {
		writeScalarNumber(env,"resultsetid",
				(int64_t)sqlrcur->getResultSetId());
		return true;
	} else if (!charstring::compareIgnoringCase(
					ptr,"resumecachedresultset",21)) {
		return resumecachedresultset(sqlrcur,env,ptr+21);
	} else if (!charstring::compareIgnoringCase(
					ptr,"resumeresultset",15)) {
		return resumeresultset(sqlrcur,env,ptr+15);
	} else if (!charstring::compareIgnoringCase(ptr,"closeresultset")) {
		sqlrcur->closeResultSet();
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"cacheoff")) {
		// env->cacheto is deliberately left alone.  cacheOff() only
		// clears the flag, so the cachefilename command would read
		// freed memory if this deleted it.
		sqlrcur->cacheOff();
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"cachefilename")) {
		writeScalar(env,"cachefilename",sqlrcur->getCacheFileName());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"totalrows")) {
		writeScalarNumber(env,"totalrows",
					(int64_t)sqlrcur->totalRows());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"firstrowindex")) {
		writeScalarNumber(env,"firstrowindex",
					(int64_t)sqlrcur->firstRowIndex());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"endofresultset")) {
		writeScalarBoolean(env,"endofresultset",
					sqlrcur->endOfResultSet());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"run",3)) {
		ptr=ptr+3;
		cmdtype=6;
	} else if (!charstring::compareIgnoringCase(ptr,"@",1)) {	
		ptr=ptr+1;
		cmdtype=6;
	} else if (!charstring::compareIgnoringCase(ptr,"delimiter",9) ||
			!charstring::compareIgnoringCase(ptr,"delimeter",9)) {	
		ptr=ptr+9;
		cmdtype=7;
	} else if (!charstring::compareIgnoringCase(ptr,"identify")) {	
		return identify(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"dbversion")) {	
		return dbversion(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"dbhostname")) {	
		return dbhostname(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"dbipaddress")) {	
		return dbipaddress(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"bindformat")) {	
		return bindformat(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"nextvalformat")) {	
		return nextvalformat(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(
					ptr,"isolationlevel ",15)) {
		if (!sqlrcon->setIsolationLevel(ptr+15)) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"isolationlevel")) {
		return getisolationlevel(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"clientversion")) {	
		clientversion(sqlrcon,env);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"serverversion")) {	
		return serverversion(sqlrcon,env);
	} else if (!charstring::compareIgnoringCase(ptr,"inputbind ",10)) {	
		return inputbind(sqlrcur,env,command);
	} else if (!charstring::compareIgnoringCase(ptr,"inputbindblob ",14)) {
		return inputbindlob(sqlrcur,env,command,"inputbindblob",
					SQLRCLIENTBINDVARTYPE_BLOB);
	} else if (!charstring::compareIgnoringCase(ptr,"inputbindclob ",14)) {
		return inputbindlob(sqlrcur,env,command,"inputbindclob",
					SQLRCLIENTBINDVARTYPE_CLOB);
	} else if (!charstring::compareIgnoringCase(ptr,"outputbind ",11)) {
		return outputbind(sqlrcur,env,command);
	} else if (!charstring::compareIgnoringCase(
						ptr,"inputoutputbind ",16)) {	
		return inputoutputbind(sqlrcur,env,command);
	// these catch the command word on its own
	} else if (!charstring::compareIgnoringCase(ptr,"inputbind")) {
		return inputbind(sqlrcur,env,command);
	} else if (!charstring::compareIgnoringCase(ptr,"inputbindblob")) {
		return inputbindlob(sqlrcur,env,command,"inputbindblob",
					SQLRCLIENTBINDVARTYPE_BLOB);
	} else if (!charstring::compareIgnoringCase(ptr,"inputbindclob")) {
		return inputbindlob(sqlrcur,env,command,"inputbindclob",
					SQLRCLIENTBINDVARTYPE_CLOB);
	} else if (!charstring::compareIgnoringCase(ptr,"outputbind")) {
		return outputbind(sqlrcur,env,command);
	} else if (!charstring::compareIgnoringCase(ptr,"inputoutputbind")) {
		return inputoutputbind(sqlrcur,env,command);
	} else if (!charstring::compareIgnoringCase(
					ptr,"fetchfrombindcursor",19)) {
		return fetchfrombindcursor(sqlrcur,env,ptr+19);
	} else if (!charstring::compareIgnoringCase(
					ptr,"countbindvariables")) {
		// this parses the query that was prepared last
		writeScalarNumber(env,"countbindvariables",
				(int64_t)sqlrcur->countBindVariables());
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"validatebinds",13)) {
		return validatebinds(env,ptr+13);
	} else if (!charstring::compareIgnoringCase(ptr,"validbind",9)) {
		return validbind(sqlrcur,env,ptr+9);
	} else if (!charstring::compareIgnoringCase(ptr,"substitution",12)) {
		return substitution(env,ptr+12);
	} else if (!charstring::compareIgnoringCase(
					ptr,"clearsubstitutions")) {
		env->clearsubstitutions();
		return true;
	} else if (!charstring::compareIgnoringCase(
					ptr,"preparefilequery",16)) {
		return filequery(sqlrcon,sqlrcur,env,ptr+16,false);
	} else if (!charstring::compareIgnoringCase(ptr,"filequery",9)) {
		return filequery(sqlrcon,sqlrcur,env,ptr+9,true);
	} else if (!charstring::compareIgnoringCase(ptr,"printbinds")) {
		switch (env->format) {
			case SQLRSH_FORMAT_PLAIN:
			case SQLRSH_FORMAT_CSV:
				printbinds("Input",&env->inputbinds);
				stdoutput.printf("\n");
				printbinds("Output",&env->outputbinds);
				stdoutput.printf("\n");
				printbinds("Input/Output",
						&env->inputoutputbinds);
				break;
			case SQLRSH_FORMAT_JSON:
			case SQLRSH_FORMAT_JSONL:
				// one object, one line
				stdoutput.write('{');
				jsonPrintBinds("input",&env->inputbinds);
				stdoutput.write(',');
				jsonPrintBinds("output",&env->outputbinds);
				stdoutput.write(',');
				jsonPrintBinds("inputoutput",
						&env->inputoutputbinds);
				stdoutput.write("}\n");
				break;
		}
		return true;
	} else if (!charstring::compareIgnoringCase(
					ptr,"printinputoutputbind",20)) {
		printbindlist(env,"Input/Output","inputoutput",
					&env->inputoutputbinds);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"printinputbind",14)) {
		printbindlist(env,"Input","input",&env->inputbinds);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"printoutputbind",15)) {
		printbindlist(env,"Output","output",&env->outputbinds);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"clearinputbind",14)) {
		return clearbindcommand(env,"clearinputbind",
						&env->inputbinds,ptr+14);
	} else if (!charstring::compareIgnoringCase(ptr,"clearoutputbind",15)) {
		return clearbindcommand(env,"clearoutputbind",
						&env->outputbinds,ptr+15);
	} else if (!charstring::compareIgnoringCase(ptr,
						"clearinputoutputbind",20)) {
		return clearbindcommand(env,"clearinputoutputbind",
						&env->inputoutputbinds,ptr+20);
	} else if (!charstring::compareIgnoringCase(ptr,"clearbinds")) {
		env->clearbinds(&env->inputbinds);
		env->clearbinds(&env->outputbinds);
		env->clearbinds(&env->inputoutputbinds);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"lastinsertid")) {	
		if (!lastinsertid(sqlrcon,env)) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			return false;
		}
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"setclientinfo ",14)) {	
		setclientinfo(sqlrcon,command);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"getclientinfo")) {
		getclientinfo(sqlrcon,env);
		return true;
	} else if (!charstring::compareIgnoringCase(
					ptr,"setresultsetbuffersize ",23)) {	
		ptr=ptr+23;
		env->rsbs=charstring::convertToInteger(ptr);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"lazyfetch",9)) {
		ptr=ptr+9;
		cmdtype=12;
	} else if (!charstring::compareIgnoringCase(
					ptr,"getresultsetbuffersize")) {
		writeScalarNumber(env,"getresultsetbuffersize",
						(int64_t)env->rsbs);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"endsession")) {	
		sqlrcon->endSession();
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"querytree")) {
		// json and jsonl get the xml as a string, because a query
		// tree isn't json
		switch (env->format) {
			case SQLRSH_FORMAT_PLAIN:
			case SQLRSH_FORMAT_CSV:
				{
					xmldom	xmld;
					if (xmld.parseString(
						sqlrcur->getQueryTree())) {
						xmld.getRootNode()->
							write(&stdoutput,true);
					}
				}
				break;
			case SQLRSH_FORMAT_JSON:
			case SQLRSH_FORMAT_JSONL:
				writeScalar(env,"querytree",
						sqlrcur->getQueryTree());
				break;
		}
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"translatedquery")) {
		writeScalar(env,"translatedquery",
					sqlrcur->getTranslatedQuery());
		return true;
	} else if (!charstring::compareIgnoringCase(
					ptr,"response timeout",16)) {
		return responseTimeout(sqlrcon,env,ptr+16);
	} else if (!charstring::compareIgnoringCase(ptr,"cache ",6)) {
		return cache(env,sqlrcur,command);
	} else if (!charstring::compareIgnoringCase(ptr,"opencache ",10)) {
		return openCache(env,sqlrcur,command);
	} else if (!charstring::compareIgnoringCase(ptr,"txqueries",9)) {
		ptr=ptr+9;
		cmdtype=13;
	} else if (!charstring::compareIgnoringCase(
					ptr,"continueonerror",15)) {
		ptr=ptr+15;
		cmdtype=18;
	} else {
		return false;
	}

	// skip white space
	while (*ptr==' ' || *ptr=='	' || *ptr=='\n') {
		ptr++;
	}

	// handle scripts
	// A script that "run" couldn't open is just a failed command as far
	// as the caller is concerned.  Code 2 is for the -script file.
	if (cmdtype==6) {
		return (runScript(sqlrcon,sqlrcur,env,ptr,true)==
						SQLRSH_EXIT_SUCCESS);
	}

	// handle debug
	if (cmdtype==4) {
		if (!charstring::compareIgnoringCase(ptr,"on",2)) {
			sqlrcon->debugOn();
			sqlrcon->setDebugFile(NULL);
		} else if (!charstring::compareIgnoringCase(ptr,"off",3)) {
			sqlrcon->debugOff();
			sqlrcon->setDebugFile(NULL);
		} else {
			sqlrcon->debugOn();
			sqlrcon->setDebugFile(ptr);
		}
		return true;
	}

	// handle format
	// An unrecognized name used to quietly mean plain, so "format jsonl"
	// on a build without jsonl looked like it had worked.  It's a failed
	// command now.
	if (cmdtype==10) {
		char	*name=charstring::duplicate(ptr);
		charstring::bothTrim(name);
		sqlrshformat	format;
		bool		valid=formatFromName(name,&format);
		if (valid) {
			env->format=format;
		} else {
			badFormatName(name);
		}
		delete[] name;
		return valid;
	}

	// handle fieldsas
	if (cmdtype==19) {
		char	*name=charstring::duplicate(ptr);
		charstring::bothTrim(name);
		sqlrshfieldsas	fieldsas;
		bool		valid=fieldsAsFromName(name,&fieldsas);
		if (valid) {
			env->fieldsas=fieldsas;
		} else {
			badFieldsAsName(name);
		}
		delete[] name;
		return valid;
	}

	// on or off?
	bool	toggle=false;
	if (!charstring::compareIgnoringCase(ptr,"on",2)) {
		toggle=true;
	}

	// set parameter
	switch (cmdtype) {
		case 2:
			env->headers=toggle;
			break;
		case 11:
			env->divider=toggle;
			break;
		case 3:
			env->stats=toggle;
			break;
		case 5:
			env->final=toggle;
			break;
		case 7:
			env->delimiter=ptr[0];
			delimiter(env);
			break;
		case 8:
			if (toggle) {
				if (!sqlrcon->autoCommitOn()) {
					displayError(env,NULL,
						sqlrcon->errorMessage(),
						sqlrcon->errorNumber());
					return false;
				}
			} else {
				if (!sqlrcon->autoCommitOff()) {
					displayError(env,NULL,
						sqlrcon->errorMessage(),
						sqlrcon->errorNumber());
					return false;
				}
			}
			env->autocommit=toggle;
			autocommit(env,toggle);
			break;
		case 9:
			if (toggle) {
				sqlrcur->getNullsAsNulls();
			} else {
				sqlrcur->getNullsAsEmptyStrings();
			}
			break;
		case 12:
			env->lazyfetch=toggle;
			break;
		case 13:
			env->txqueries=toggle;
			break;
		// getasnumber is an alias for two of the fieldsas modes
		case 14:
			env->fieldsas=(toggle)?SQLRSH_FIELDSAS_NUMBER:
						SQLRSH_FIELDSAS_RAW;
			break;
		case 15:
			env->noelapsed=toggle;
			break;
		case 16:
			env->nextresultset=toggle;
			break;
		// quiet is shorthand for headers and stats
		case 17:
			env->headers=!toggle;
			env->stats=!toggle;
			break;
		case 18:
			env->continueonerror=toggle;
			break;
	}
	return true;
}

bool sqlrsh::externalCommand(sqlrconnection *sqlrcon,
				sqlrcursor *sqlrcur, sqlrshenv *env, 
				const char *command) {

	bool	retval=true;

	// handle begin, commit and rollback
	if (!env->txqueries &&
		!charstring::compareIgnoringCase(command,"begin")) {

		if (!sqlrcon->begin()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			retval=false;
		}

	} else if (!env->txqueries &&
		!charstring::compareIgnoringCase(command,"commit")) {

		if (!sqlrcon->commit()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			retval=false;
		}

	} else if (!env->txqueries &&
		!charstring::compareIgnoringCase(command,"rollback")) {

		if (!sqlrcon->rollback()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
			retval=false;
		}

	} else if (!charstring::compareIgnoringCase(command,"fields ",7)) {

		char	*table=getTable(command,false);
		sqlrcur->getColumnList(table,NULL);
		delete[] table;

		// write the column names on one line
		uint64_t	namecount=sqlrcur->rowCount();
		switch (env->format) {
			case SQLRSH_FORMAT_PLAIN:
				for (uint64_t j=0; j<namecount; j++) {
					if (j>0) {
						stdoutput.printf(",");
					}
					stdoutput.printf("%s",
						sqlrcur->getField(
							j,(uint32_t)0));
				}
				stdoutput.printf("\n");
				break;
			case SQLRSH_FORMAT_CSV:
				for (uint64_t j=0; j<namecount; j++) {
					if (j>0) {
						stdoutput.printf(",");
					}
					csvWriteField(
						sqlrcur->getField(
							j,(uint32_t)0),
						sqlrcur->getFieldLength(
							j,(uint32_t)0));
				}
				stdoutput.printf("\n");
				break;
			case SQLRSH_FORMAT_JSON:
			case SQLRSH_FORMAT_JSONL:
				stdoutput.write("{\"fields\":[");
				for (uint64_t j=0; j<namecount; j++) {
					if (j>0) {
						stdoutput.write(',');
					}
					jsonWriteString(&stdoutput,
						sqlrcur->getField(
							j,(uint32_t)0),
						sqlrcur->getFieldLength(
							j,(uint32_t)0));
				}
				stdoutput.write("]}\n");
				break;
		}

		if (env->final) {
			sqlrcon->endSession();
		}

	} else {

		sqlrcur->setResultSetBufferSize(env->rsbs);

		if (env->lazyfetch) {
			sqlrcur->lazyFetch();
		} else {
			sqlrcur->dontLazyFetch();
		}

		// send the query
		if (!charstring::compareIgnoringCase(command,
						"show databases mysql",20)) {
			char	*wild=getWild(command);
			sqlrcur->getDatabaseList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show databases odbc",19)) {
			char	*wild=getWild(command);
			sqlrcur->getDatabaseList(wild,
					SQLRCLIENTLISTFORMAT_ODBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show databases jdbc",19)) {
			char	*wild=getWild(command);
			sqlrcur->getDatabaseList(wild,
					SQLRCLIENTLISTFORMAT_JDBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show databases",14)) {
			char	*wild=getWild(command);
			sqlrcur->getDatabaseList(wild);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show catalogs mysql",19)) {
			char	*wild=getWild(command);
			sqlrcur->getCatalogList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show catalogs odbc",18)) {
			char	*wild=getWild(command);
			sqlrcur->getCatalogList(wild,
					SQLRCLIENTLISTFORMAT_ODBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show catalogs jdbc",18)) {
			char	*wild=getWild(command);
			sqlrcur->getCatalogList(wild,
					SQLRCLIENTLISTFORMAT_JDBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show catalogs",13)) {
			char	*wild=getWild(command);
			sqlrcur->getCatalogList(wild);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show schemas mysql",18)) {
			char	*wild=getWild(command);
			sqlrcur->getSchemaList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show schemas odbc",17)) {
			char	*wild=getWild(command);
			sqlrcur->getSchemaList(wild,
					SQLRCLIENTLISTFORMAT_ODBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show schemas jdbc",17)) {
			char	*wild=getWild(command);
			sqlrcur->getSchemaList(wild,
					SQLRCLIENTLISTFORMAT_JDBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show schemas",12)) {
			char	*wild=getWild(command);
			sqlrcur->getSchemaList(wild);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show tables mysql",17)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_TABLE|
					DB_OBJECT_VIEW|
					DB_OBJECT_ALIAS|
					DB_OBJECT_SYNONYM);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show tables odbc",16)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_ODBC,
					DB_OBJECT_TABLE|
					DB_OBJECT_VIEW|
					DB_OBJECT_ALIAS|
					DB_OBJECT_SYNONYM);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show tables jdbc",16)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_JDBC,
					DB_OBJECT_TABLE|
					DB_OBJECT_VIEW|
					DB_OBJECT_ALIAS|
					DB_OBJECT_SYNONYM);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only tables mysql",22)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_TABLE);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only tables odbc",21)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_ODBC,
					DB_OBJECT_TABLE);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only tables jdbc",21)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_JDBC,
					DB_OBJECT_TABLE);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only tables",16)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_TABLE);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only views mysql",21)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_VIEW);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only views odbc",20)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_ODBC,
					DB_OBJECT_VIEW);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only views jdbc",20)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_JDBC,
					DB_OBJECT_VIEW);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only views",15)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_VIEW);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only aliases mysql",23)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_ALIAS);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only aliases odbc",22)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_ODBC,
					DB_OBJECT_ALIAS);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only aliases jdbc",22)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_JDBC,
					DB_OBJECT_ALIAS);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only aliases",17)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_ALIAS);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
					"show only synonyms mysql",24)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_SYNONYM);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only synonyms odbc",23)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_ODBC,
					DB_OBJECT_SYNONYM);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only synonyms jdbc",23)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_JDBC,
					DB_OBJECT_SYNONYM);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show only synonyms",18)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL,
					DB_OBJECT_SYNONYM);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
							"show tables",11)) {
			char	*wild=getWild(command);
			sqlrcur->getTableList(wild);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show table types mysql",22)) {
			sqlrcur->getTableTypeList(
					SQLRCLIENTLISTFORMAT_MYSQL);
		} else if (!charstring::compareIgnoringCase(command,
						"show table types odbc",21)) {
			sqlrcur->getTableTypeList(
					SQLRCLIENTLISTFORMAT_ODBC);
		} else if (!charstring::compareIgnoringCase(command,
						"show table types jdbc",21)) {
			sqlrcur->getTableTypeList(
					SQLRCLIENTLISTFORMAT_JDBC);
		} else if (!charstring::compareIgnoringCase(command,
						"show table types",16)) {
			sqlrcur->getTableTypeList();
		} else if (!charstring::compareIgnoringCase(command,
						"show columns mysql",18)) {
			char	*table=getTable(command,true);
			char	*wild=getWild(command);
			sqlrcur->getColumnList(table,wild,
					SQLRCLIENTLISTFORMAT_MYSQL);
			delete[] table;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show columns odbc",17)) {
			char	*table=getTable(command,true);
			char	*wild=getWild(command);
			sqlrcur->getColumnList(table,wild,
					SQLRCLIENTLISTFORMAT_ODBC);
			delete[] table;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show columns jdbc",17)) {
			char	*table=getTable(command,true);
			char	*wild=getWild(command);
			sqlrcur->getColumnList(table,wild,
					SQLRCLIENTLISTFORMAT_JDBC);
			delete[] table;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
							"show columns",12)) {
			char	*table=getTable(command,true);
			char	*wild=getWild(command);
			sqlrcur->getColumnList(table,wild);
			delete[] table;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
							"describe ",9)) {
			char	*table=getTable(command,false);
			char	*wild=getWild(command);
			sqlrcur->getColumnList(table,wild);
			delete[] table;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show primary keys",17)) {
			char	*table=getTable(command,true);
			char	*wild=getWild(command);
			sqlrcur->getPrimaryKeysList(table,wild);
			delete[] table;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show keys and indexes",21)) {
			char	*table=getTable(command,true);
			char	*wild=getWild(command);
			sqlrcur->getKeyAndIndexList(table,wild);
			delete[] table;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
					"show procedure parameters",25)) {
			char	*procedure=getProcedure(command);
			char	*wild=getWild(command);
			sqlrcur->getProcedureParameterList(procedure,wild);
			delete[] procedure;
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show type info mysql",20)) {
			char	*type=getType(command);
			sqlrcur->getTypeInfoList(type,
					SQLRCLIENTLISTFORMAT_MYSQL);
			delete[] type;
		} else if (!charstring::compareIgnoringCase(command,
						"show type info odbc",19)) {
			char	*type=getType(command);
			sqlrcur->getTypeInfoList(type,
					SQLRCLIENTLISTFORMAT_ODBC);
			delete[] type;
		} else if (!charstring::compareIgnoringCase(command,
						"show type info jdbc",19)) {
			char	*type=getType(command);
			sqlrcur->getTypeInfoList(type,
					SQLRCLIENTLISTFORMAT_JDBC);
			delete[] type;
		} else if (!charstring::compareIgnoringCase(command,
						"show type info",14)) {
			char	*type=getType(command);
			sqlrcur->getTypeInfoList(type);
			delete[] type;
		} else if (!charstring::compareIgnoringCase(command,
						"show procedures mysql",21)) {
			char	*wild=getWild(command);
			sqlrcur->getProcedureList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show procedures odbc",20)) {
			char	*wild=getWild(command);
			sqlrcur->getProcedureList(wild,
					SQLRCLIENTLISTFORMAT_ODBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show procedures jdbc",20)) {
			char	*wild=getWild(command);
			sqlrcur->getProcedureList(wild,
					SQLRCLIENTLISTFORMAT_JDBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show procedures",15)) {
			char	*wild=getWild(command);
			sqlrcur->getProcedureList(wild);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show lastinsertid",17)) {
			sqlrcur->getLastInsertIdList();
		} else if (!charstring::compareIgnoringCase(command,
							"reexecute")) {	
			executeQuery(sqlrcur,env);
		} else {
			sqlrcur->prepareQuery(command);
			executeQuery(sqlrcur,env);
		}

		// look for an error
		if (sqlrcur->errorMessage()) {

			// display the error
			displayError(env,NULL,
					sqlrcur->errorMessage(),
					sqlrcur->errorNumber());
			retval=false;

		} else if (env->nextresultset) {

			do {

				// display the header
				displayHeader(sqlrcur,env);

				// display the result set
				displayResultSet(sqlrcur,env);

				// display any errors
				if (sqlrcur->errorMessage()) {
					displayError(env,NULL,
						sqlrcur->errorMessage(),
						sqlrcur->errorNumber());
					retval=false;
				}

			} while (sqlrcur->nextResultSet());

		} else {

			// display the header
			displayHeader(sqlrcur,env);

			// display the result set
			displayResultSet(sqlrcur,env);
		}

		if (env->final) {
			sqlrcon->endSession();
		}
	}

	// display statistics
	displayStats(sqlrcur,env);

	return retval;
}

void sqlrsh::executeQuery(sqlrcursor *sqlrcur, sqlrshenv *env) {

	sqlrcur->clearBinds();

	if (env->inputbinds.getCount()) {

		for (listnode<char *> *node=
				env->inputbinds.getKeys()->getFirst();
				node; node=node->getNext()) {

			const char	*name=node->getValue();
			sqlrshbindvalue	*bv=
				env->inputbinds.getValue(node->getValue());
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				// the length is always explicit, so a value
				// with an embedded null goes out whole
				sqlrcur->inputBind(name,
						bv->stringval.value,
						bv->stringval.length);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
				sqlrcur->inputBind(name,bv->integerval);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
				sqlrcur->inputBind(name,bv->doubleval.value,
							bv->doubleval.precision,
							bv->doubleval.scale);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
				sqlrcur->inputBind(name,
						bv->dateval.year,
						bv->dateval.month,
						bv->dateval.day,
						bv->dateval.hour,
						bv->dateval.minute,
						bv->dateval.second,
						bv->dateval.microsecond,
						bv->dateval.tz,
						bv->dateval.isnegative);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_BLOB) {
				sqlrcur->inputBindBlob(name,
						bv->stringval.value,
						bv->stringval.length);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_CLOB) {
				sqlrcur->inputBindClob(name,
						bv->stringval.value,
						bv->stringval.length);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_NULL) {
				sqlrcur->inputBind(name,(const char *)NULL);
			}
		}
	}

	if (env->outputbinds.getCount()) {

		for (listnode<char *> *node=
				env->outputbinds.getKeys()->getFirst();
				node; node=node->getNext()) {

			const char	*name=node->getValue();
			sqlrshbindvalue	*bv=
				env->outputbinds.getValue(node->getValue());
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				sqlrcur->defineOutputBindString(name,
						bv->outputstringbindlength);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
				sqlrcur->defineOutputBindInteger(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
				sqlrcur->defineOutputBindDouble(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
				sqlrcur->defineOutputBindDate(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_BLOB) {
				sqlrcur->defineOutputBindBlob(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_CLOB) {
				sqlrcur->defineOutputBindClob(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_CURSOR) {
				sqlrcur->defineOutputBindCursor(name);
			}
		}
	}

	if (env->inputoutputbinds.getCount()) {

		for (listnode<char *> *node=
				env->inputoutputbinds.getKeys()->getFirst();
				node; node=node->getNext()) {

			const char	*name=node->getValue();
			sqlrshbindvalue	*bv=
				env->inputoutputbinds.getValue(
						node->getValue());
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				sqlrcur->defineInputOutputBindString(name,
						bv->stringval.value,
						bv->outputstringbindlength);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
				sqlrcur->defineInputOutputBindInteger(name,
						bv->integerval);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
				sqlrcur->defineInputOutputBindDouble(name,
						bv->doubleval.value,
						bv->doubleval.precision,
						bv->doubleval.scale);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
				sqlrcur->defineInputOutputBindDate(name,
						bv->dateval.year,
						bv->dateval.month,
						bv->dateval.day,
						bv->dateval.hour,
						bv->dateval.minute,
						bv->dateval.second,
						bv->dateval.microsecond,
						bv->dateval.tz,
						bv->dateval.isnegative);
			}
		}
	}

	// apply substitutions
	// prepareQuery() and prepareFileQuery() clear the ones the cursor is
	// holding, so they have to go on again for every query.
	if (env->substitutions.getCount()) {

		for (listnode<char *> *node=
				env->substitutions.getKeys()->getFirst();
				node; node=node->getNext()) {

			const char	*name=node->getValue();
			sqlrshbindvalue	*bv=
				env->substitutions.getValue(node->getValue());
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				sqlrcur->substitution(name,
						bv->stringval.value);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
				sqlrcur->substitution(name,bv->integerval);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
				sqlrcur->substitution(name,
						bv->doubleval.value,
						bv->doubleval.precision,
						bv->doubleval.scale);
			}
		}
	}

	// prepareQuery() clears this too, so it also goes on for every query
	if (env->validatebinds) {
		sqlrcur->validateBinds();
	}

	sqlrcur->executeQuery();

	if (env->outputbinds.getCount()) {

		for (listnode<char *> *node=
				env->outputbinds.getKeys()->getFirst();
				node; node=node->getNext()) {

			const char	*name=node->getValue();
			sqlrshbindvalue	*bv=
				env->outputbinds.getValue(node->getValue());
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				const char	*str=
					sqlrcur->getOutputBindString(name);
				delete[] bv->stringval.value;
				bv->stringval.value=
					charstring::duplicate(str);
				bv->stringval.length=
					charstring::getLength(str);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_BLOB ||
					bv->type==SQLRCLIENTBINDVARTYPE_CLOB) {
				// a lob's length is the only thing that says
				// where it ends - it can hold nulls
				const char	*lob=
					(bv->type==SQLRCLIENTBINDVARTYPE_BLOB)?
					sqlrcur->getOutputBindBlob(name):
					sqlrcur->getOutputBindClob(name);
				uint32_t	loblen=
					sqlrcur->getOutputBindLength(name);
				delete[] bv->stringval.value;
				bv->stringval.value=
					duplicateBytes(lob,loblen);
				bv->stringval.length=(lob)?loblen:0;
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
				bv->integerval=
					sqlrcur->getOutputBindInteger(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
				bv->doubleval.value=
					sqlrcur->getOutputBindDouble(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
				sqlrcur->getOutputBindDate(name,
						&(bv->dateval.year),
						&(bv->dateval.month),
						&(bv->dateval.day),
						&(bv->dateval.hour),
						&(bv->dateval.minute),
						&(bv->dateval.second),
						&(bv->dateval.microsecond),
						&(bv->dateval.tz),
						&(bv->dateval.isnegative));
			}
		}
	}

	if (env->inputoutputbinds.getCount()) {

		for (listnode<char *> *node=
				env->inputoutputbinds.getKeys()->getFirst();
				node; node=node->getNext()) {

			const char	*name=node->getValue();
			sqlrshbindvalue	*bv=
				env->inputoutputbinds.getValue(
						node->getValue());
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				const char	*str=
					sqlrcur->getInputOutputBindString(
									name);
				delete[] bv->stringval.value;
				bv->stringval.value=
					charstring::duplicate(str);
				bv->stringval.length=
					charstring::getLength(str);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
				bv->integerval=
				sqlrcur->getInputOutputBindInteger(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
				bv->doubleval.value=
				sqlrcur->getInputOutputBindDouble(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
				sqlrcur->getInputOutputBindDate(name,
						&(bv->dateval.year),
						&(bv->dateval.month),
						&(bv->dateval.day),
						&(bv->dateval.hour),
						&(bv->dateval.minute),
						&(bv->dateval.second),
						&(bv->dateval.microsecond),
						&(bv->dateval.tz),
						&(bv->dateval.isnegative));
			}
		}
	}
}

char *sqlrsh::getWild(const char *command) {
	const char	*wildptr=charstring::findFirst(command,"'");
	if (!wildptr) {
		return NULL;
	}
	wildptr++;
	const char	*endptr=charstring::findLast(wildptr,"'");
	if (!endptr) {
		return NULL;
	}

	// unescape single quotes
	stringbuffer	output;
	for (const char *ch=wildptr; ch<endptr; ch++) {
		if (*ch=='\'' && *(ch+1)=='\'') {
			ch++;
		}
		output.append(*ch);
	}

	return output.detachString();
}

char *sqlrsh::getTable(const char *command, bool in) {
	const char	*tableptr=NULL;
	if (in) {
		tableptr=charstring::findFirst(command," in ");
		if (!tableptr) {
			return NULL;
		}
		tableptr=tableptr+4;
		const char	*endptr=charstring::findFirst(tableptr," ");
		if (!endptr) {
			return charstring::duplicate(tableptr);
		}
		return charstring::duplicate(tableptr,endptr-tableptr);
	} else {
		tableptr=charstring::findFirst(command," ");
		if (!tableptr) {
			return NULL;
		}
		return charstring::duplicate(tableptr+1);
	}
	return NULL;
}

char *sqlrsh::getProcedure(const char *command) {
	const char	*procptr=charstring::findFirst(command," in ");
	if (!procptr) {
		return NULL;
	}
	procptr=procptr+4;
	const char	*endptr=charstring::findFirst(procptr," ");
	if (!endptr) {
		return charstring::duplicate(procptr);
	}
	return charstring::duplicate(procptr,endptr-procptr);
}

char *sqlrsh::getType(const char *command) {
	const char	*procptr=charstring::findFirst(command," for ");
	if (!procptr) {
		return charstring::duplicate("*");
	}
	procptr=procptr+5;
	const char	*endptr=charstring::findFirst(procptr," ");
	if (!endptr) {
		return charstring::duplicate(procptr);
	}
	return charstring::duplicate(procptr,endptr-procptr);
}

void sqlrsh::initStats(sqlrshenv *env) {

	if (!env->stats) {
		return;
	}

	start.initFromSystemDateTime();
}

void sqlrsh::displayError(sqlrshenv *env,
				const char *message,
				const char *error,
				int64_t errornumber) {
	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			if (!charstring::isNullOrEmpty(message)) {
				stderror.printf("%s\n",message);
			}
			stderror.printf("%lld:\n",(long long)errornumber);
			if (!charstring::isNullOrEmpty(error)) {
				stderror.printf("%s\n\n",error);
			}
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			// one object, one line, on stderr, where it can't
			// corrupt the document on stdout
			stderror.write("{\"error\":{");
			if (!charstring::isNullOrEmpty(message)) {
				stderror.write("\"context\":");
				jsonWriteString(&stderror,message,
					charstring::getLength(message));
				stderror.write(',');
			}
			stderror.printf("\"number\":%lld,\"message\":",
						(long long)errornumber);
			jsonWriteString(&stderror,error,
					charstring::getLength(error));
			stderror.write("}}\n");
			break;
	}
}

void sqlrsh::displayHeader(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// no default label, so -Wswitch points here when a format is added
	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
			plainDisplayHeader(sqlrcur,env);
			break;
		case SQLRSH_FORMAT_CSV:
			csvDisplayHeader(sqlrcur,env);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			jsonDisplayHeader(sqlrcur,env);
			break;
	}
}

uint32_t sqlrsh::plainColumnWidth(sqlrcursor *sqlrcur, sqlrshenv *env,
							uint32_t col) {

	// getLongest() measures the raw fields, so a conversion that widens
	// them runs past it - a boolean is the one whose width is known
	// before the rows arrive
	uint32_t	longest=sqlrcur->getLongest(col);
	if (env->fieldsas==SQLRSH_FIELDSAS_BOOLEAN && longest<5) {
		const char	*fieldtype=sqlrcur->getColumnType(col);
		if (isBoolTypeChar(fieldtype) ||
			isBitTypeChar(fieldtype) ||
			isNumberTypeChar(fieldtype)) {
			longest=5;
		}
	}

	if (env->headers) {
		uint32_t	namelen=charstring::getLength(
						sqlrcur->getColumnName(col));
		if (namelen>longest) {
			longest=namelen;
		}
	}
	return longest;
}

void sqlrsh::plainDisplayHeader(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// The headers toggle is plain format only.  Every other format is
	// parsed, and the names are the only way a reader learns the columns.
	if (!env->headers) {
		return;
	}

	uint32_t	colcount=sqlrcur->colCount();
	if (!colcount) {
		return;
	}

	// display column names
	uint32_t	charcount=0;
	for (uint32_t ci=0; ci<colcount; ci++) {

		// put an extra space between field names
		if (ci) {
			stdoutput.write(' ');
			charcount=charcount+1;
		}

		// write the column name
		const char	*name=sqlrcur->getColumnName(ci);
		uint32_t	namelen=charstring::getLength(name);
		stdoutput.write(name);

		// space-pad after the name, if necessary
		uint32_t	longest=plainColumnWidth(sqlrcur,env,ci);
		charcount=charcount+longest;
		for (uint32_t j=namelen; j<longest; j++) {
			stdoutput.write(' ');
		}
	}
	stdoutput.printf("\n");

	// display divider
	if (env->divider) {
		for (uint32_t i=0; i<charcount; i++) {
			stdoutput.printf("=");
		}
		stdoutput.printf("\n");
	}
}

void sqlrsh::csvDisplayHeader(sqlrcursor *sqlrcur, sqlrshenv *env) {

	uint32_t	colcount=sqlrcur->colCount();
	if (!colcount) {
		return;
	}

	for (uint32_t ci=0; ci<colcount; ci++) {

		// put a comma between field names
		if (ci) {
			stdoutput.write(',');
		}

		// write the column name
		const char	*name=sqlrcur->getColumnName(ci);
		csvWriteField(name,charstring::getLength(name));
	}
	stdoutput.printf("\n");
}

void sqlrsh::jsonDisplayHeader(sqlrcursor *sqlrcur, sqlrshenv *env) {

	bool	jsonl=(env->format==SQLRSH_FORMAT_JSONL);

	// json opens the one document for this result set here, and
	// jsonDisplayResultSet() closes it
	if (jsonl) {
		stdoutput.write("{\"type\":\"columns\",\"columns\":[");
	} else {
		stdoutput.write("{\"columns\":[");
	}

	uint32_t	colcount=sqlrcur->colCount();
	for (uint32_t ci=0; ci<colcount; ci++) {

		if (ci) {
			stdoutput.write(',');
		}

		const char	*name=sqlrcur->getColumnName(ci);
		const char	*type=sqlrcur->getColumnType(ci);
		stdoutput.write("{\"name\":");
		jsonWriteString(&stdoutput,name,charstring::getLength(name));
		stdoutput.write(",\"type\":");
		jsonWriteString(&stdoutput,type,charstring::getLength(type));
		stdoutput.write('}');
	}

	if (jsonl) {
		stdoutput.write("]}\n");
	} else {
		stdoutput.write("],\"rows\":[");
	}
}

void sqlrsh::csvWriteField(const char *field, uint32_t length) {

	// A null is written as an unquoted empty field.  The empty string
	// isn't a number, so it gets quoted, and the two can be told apart.
	if (!field) {
		return;
	}

	bool	quote=csvFieldNeedsQuotes(field,length);

	if (quote) {
		stdoutput.write('"');
	}
	csvEscapeField(field,length);
	if (quote) {
		stdoutput.write('"');
	}
}

bool sqlrsh::csvFieldNeedsQuotes(const char *field, uint32_t length) {

	// quote fields containing anything that would otherwise
	// break the field, the row, or the file apart
	for (uint32_t index=0; index<length; index++) {
		char	ch=field[index];
		if (ch=='"' || ch==',' || ch=='\n' || ch=='\r' ||
						ch=='\t' || !ch) {
			return true;
		}
	}

	// quote the field if it's not a number, or if it is a number,
	// but has more than 12 digits.  Excel (and presumably other
	// spreadsheet apps) likes to convert 12+ digit numbers to
	// scientific notation.
	return (!charstring::isNumber(field,(int32_t)length) || length>=12);
}

void sqlrsh::csvEscapeField(const char *field, uint32_t length) {

	// write the field through unchanged, doubling embedded
	// double quotes, one run of bytes at a time
	uint32_t	start=0;
	for (uint32_t index=0; index<length; index++) {
		if (field[index]=='"') {
			stdoutput.write(field+start,index-start+1);
			stdoutput.write('"');
			start=index+1;
		}
	}
	stdoutput.write(field+start,length-start);
}

void sqlrsh::jsonWriteString(filedescriptor *fd,
				const char *str, uint32_t length) {

	fd->write('"');
	if (str) {
		jsonEscapeString(fd,str,length);
	}
	fd->write('"');
}

void sqlrsh::jsonEscapeString(filedescriptor *fd,
				const char *str, uint32_t length) {

	// Note the unsigned char: with a plain char every utf-8 continuation
	// byte is negative and would read as a control character.
	uint32_t	start=0;
	for (uint32_t index=0; index<length; index++) {

		unsigned char	ch=(unsigned char)str[index];
		const char	*escape=NULL;
		char		escapebuffer[7];

		switch (ch) {
			case '"':
				escape="\\\"";
				break;
			case '\\':
				escape="\\\\";
				break;
			case '\b':
				escape="\\b";
				break;
			case '\f':
				escape="\\f";
				break;
			case '\n':
				escape="\\n";
				break;
			case '\r':
				escape="\\r";
				break;
			case '\t':
				escape="\\t";
				break;
			default:
				// json requires an escape for every byte
				// below 0x20 and has no short form for the
				// rest of them
				if (ch<0x20) {
					charstring::printf(escapebuffer,
							sizeof(escapebuffer),
							"\\u%04x",(int)ch);
					escape=escapebuffer;
				}
				break;
		}

		if (escape) {
			fd->write(str+start,(size_t)(index-start));
			fd->write(escape);
			start=index+1;
		}
	}
	fd->write(str+start,(size_t)(length-start));
}

// a json number:
//	-? (0 | [1-9][0-9]*) (\.[0-9]+)? ([eE][-+]?[0-9]+)?
static bool jsonIsNumber(const char *field, uint32_t length) {

	uint32_t	index=0;

	// sign
	if (index<length && field[index]=='-') {
		index++;
	}

	// integer part
	if (index>=length || !character::isDigit(field[index])) {
		return false;
	}
	if (field[index]=='0') {
		index++;
	} else {
		while (index<length && character::isDigit(field[index])) {
			index++;
		}
	}

	// fraction
	if (index<length && field[index]=='.') {
		index++;
		if (index>=length || !character::isDigit(field[index])) {
			return false;
		}
		while (index<length && character::isDigit(field[index])) {
			index++;
		}
	}

	// exponent
	if (index<length && (field[index]=='e' || field[index]=='E')) {
		index++;
		if (index<length && (field[index]=='-' || field[index]=='+')) {
			index++;
		}
		if (index>=length || !character::isDigit(field[index])) {
			return false;
		}
		while (index<length && character::isDigit(field[index])) {
			index++;
		}
	}

	return (index==length);
}

void sqlrsh::jsonWriteValue(filedescriptor *fd, const char *field,
					uint32_t length,
					sqlrshjsontype jsontype) {

	// a database null is the json null literal
	if (!field) {
		fd->write("null");
		return;
	}

	// A number only goes out bare if it really is a json number.
	// getFieldAsDouble() can hand back inf or nan, and neither is one.
	if (jsontype==SQLRSH_JSONTYPE_NUMBER && jsonIsNumber(field,length)) {
		fd->write(field,(size_t)length);
		return;
	}

	// a real json boolean, so a reader doesn't have to match a word
	if (jsontype==SQLRSH_JSONTYPE_BOOLEAN) {
		fd->write(field,(size_t)length);
		return;
	}

	jsonWriteString(fd,field,length);
}

bool sqlrsh::formatFieldAsDate(char *buffer, size_t buffersize,
				int16_t year, int16_t month, int16_t day,
				int16_t hour, int16_t minute, int16_t second,
				int32_t microsecond, bool isnegative) {

	// a month or a day that's there but zero is mysql's 0000-00-00, so
	// the whole field passes through rather than being rendered
	if (!month || !day) {
		return false;
	}

	// a time on its own comes back with the month and the day unset, but
	// with the year adjusted, so the date part is decided on the month
	// and the day
	bool	hasdate=(month>0 && day>0);
	bool	hastime=(hour>=0);
	if (!hasdate && !hastime) {
		return false;
	}

	stringbuffer	date;
	if (isnegative) {
		date.append('-');
	}
	char	part[32];
	if (hasdate) {
		charstring::printf(part,sizeof(part),"%04d-%02d-%02d",
					(int)year,(int)month,(int)day);
		date.append(part);
	}
	if (hastime) {
		if (hasdate) {
			date.append(' ');
		}
		charstring::printf(part,sizeof(part),"%02d:%02d:%02d",
					(int)hour,
					(int)((minute>=0)?minute:0),
					(int)((second>=0)?second:0));
		date.append(part);
		// the parse can't tell a fraction that wasn't there from one
		// that was zero, so a zero one is left off
		if (microsecond>0) {
			charstring::printf(part,sizeof(part),".%06d",
						(int)microsecond);
			date.append(part);
		}
	}
	charstring::printf(buffer,buffersize,"%s",date.getString());
	return true;
}

const char *sqlrsh::getFieldForDisplay(sqlrcursor *sqlrcur, sqlrshenv *env,
					uint64_t row, uint32_t col,
					uint32_t *length,
					char *convbuffer,
					size_t convbuffersize,
					sqlrshjsontype *jsontype) {

	const char	*field=sqlrcur->getField(row,col);
	const char	*fieldtype=sqlrcur->getColumnType(col);
	*length=sqlrcur->getFieldLength(row,col);
	*jsontype=SQLRSH_JSONTYPE_STRING;

	if (!field) {
		return field;
	}

	// FIXME: move this down below the end-of-rs check?
	// The purpose of this is to verify the functionality
	// of the getFieldAsXXX() methods.
	if (env->fieldsas==SQLRSH_FIELDSAS_NUMBER &&
		(isBitTypeChar(fieldtype) ||
			isNumberTypeChar(fieldtype))) {

		if (isFloatTypeChar(fieldtype)) {
			double	fd=sqlrcur->getFieldAsDouble(row,col);
			if (isNonScaleFloatTypeChar(fieldtype)) {
				int32_t	precision=sqlrcur->getColumnPrecision(col);
				// here precision is a number of bits, but printf %g wants digits.
				// FIXME: precision should actually be the number of digits, not bits...
				int32_t	digits=(int32_t)(ceil(precision/3.33));
				charstring::printf(convbuffer,convbuffersize,"%.*g",digits,fd);
			} else {
				int	scale=sqlrcur->getColumnScale(col);
				// NOTE: we are not using the precision to format the number to a string.
				charstring::printf(convbuffer,convbuffersize,"%.*f",scale,fd);
			}
		} else {
			int64_t fi = sqlrcur->getFieldAsInteger(row,col);
			charstring::printf(convbuffer, convbuffersize, "%ld", fi);
		}
		field=convbuffer;
		*length=charstring::getLength(field);
		*jsontype=SQLRSH_JSONTYPE_NUMBER;

	} else if (env->fieldsas==SQLRSH_FIELDSAS_BOOLEAN &&
			(isBoolTypeChar(fieldtype) ||
				isBitTypeChar(fieldtype) ||
				isNumberTypeChar(fieldtype))) {

		// getFieldAsBoolean() answers for a field of any type at all,
		// so only the column type can say whether the answer means
		// anything
		field=(sqlrcur->getFieldAsBoolean(row,col))?"true":"false";
		*length=charstring::getLength(field);
		*jsontype=SQLRSH_JSONTYPE_BOOLEAN;

	} else if (env->fieldsas==SQLRSH_FIELDSAS_DATE) {

		// unlike getFieldAsBoolean(), this one reports whether it
		// could read the field, so its own answer is the guard - a
		// column type test would make the mode do nothing at all for
		// sqlite, which calls a date column STRING
		int16_t	year;
		int16_t	month;
		int16_t	day;
		int16_t	hour;
		int16_t	minute;
		int16_t	second;
		int32_t	microsecond;
		bool	isnegative;
		if (sqlrcur->getFieldAsDate(row,col,&year,&month,&day,
						&hour,&minute,&second,
						&microsecond,&isnegative) &&
			formatFieldAsDate(convbuffer,convbuffersize,
						year,month,day,
						hour,minute,second,
						microsecond,isnegative)) {
			field=convbuffer;
			*length=charstring::getLength(field);
		}
	}

	return field;
}

void sqlrsh::displayResultSet(sqlrcursor *sqlrcur, sqlrshenv *env) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
			plainDisplayResultSet(sqlrcur,env);
			break;
		case SQLRSH_FORMAT_CSV:
			csvDisplayResultSet(sqlrcur,env);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			jsonDisplayResultSet(sqlrcur,env);
			break;
	}
}

void sqlrsh::plainDisplayResultSet(sqlrcursor *sqlrcur, sqlrshenv *env) {

	uint32_t	colcount=sqlrcur->colCount();
	if (!colcount) {
		return;
	}

	char		convfieldbuffer[256];

	bool		done=false;
	for (uint64_t row=0; !done; row++) {

		if (row) {
			stdoutput.write('\n');
		}

		for (uint32_t col=0; col<colcount; col++) {

			// put an extra space between fields
			if (col) {
				stdoutput.write(' ');
			}

			// get the field
			uint32_t	fieldlength;
			sqlrshjsontype	jsontype;
			const char	*field=getFieldForDisplay(sqlrcur,env,
						row,col,&fieldlength,
						convfieldbuffer,
						sizeof(convfieldbuffer),
						&jsontype);

			// check for end-of-result-set condition
			// (since nullsasnulls might be set, we have to do
			// a bit more than just check for a NULL)
			if (!col && !field &&
				sqlrcur->endOfResultSet() &&
				row==sqlrcur->rowCount()) {
				done=true;
				break;
			}

			// handle nulls
			if (!field) {
				field="NULL";
				fieldlength=4;
			}

			// write the field
			stdoutput.write(field,fieldlength);

			// space-pad after the field, if necessary
			uint32_t	longest=plainColumnWidth(
							sqlrcur,env,col);
			for (uint32_t i=fieldlength; i<longest; i++) {
				stdoutput.write(' ');
			}
		}
	}
}

void sqlrsh::csvDisplayResultSet(sqlrcursor *sqlrcur, sqlrshenv *env) {

	uint32_t	colcount=sqlrcur->colCount();
	if (!colcount) {
		return;
	}

	char		convfieldbuffer[256];

	bool		done=false;
	for (uint64_t row=0; !done; row++) {

		if (row) {
			stdoutput.write('\n');
		}

		for (uint32_t col=0; col<colcount; col++) {

			// put a comma between fields
			if (col) {
				stdoutput.write(',');
			}

			// get the field
			uint32_t	fieldlength;
			sqlrshjsontype	jsontype;
			const char	*field=getFieldForDisplay(sqlrcur,env,
						row,col,&fieldlength,
						convfieldbuffer,
						sizeof(convfieldbuffer),
						&jsontype);

			// check for end-of-result-set condition
			// (since nullsasnulls might be set, we have to do
			// a bit more than just check for a NULL)
			if (!col && !field &&
				sqlrcur->endOfResultSet() &&
				row==sqlrcur->rowCount()) {
				done=true;
				break;
			}

			// handle nulls
			// leave the field null, csvWriteField() writes an
			// unquoted empty field
			if (!field) {
				fieldlength=0;
			}

			// write the field
			csvWriteField(field,fieldlength);
		}
	}
}

void sqlrsh::jsonDisplayResultSet(sqlrcursor *sqlrcur, sqlrshenv *env) {

	bool		jsonl=(env->format==SQLRSH_FORMAT_JSONL);

	uint32_t	colcount=sqlrcur->colCount();

	char		convfieldbuffer[256];

	// A statement with no result set never reaches the end-of-result-set
	// test, because that lives in the column loop.
	bool		done=!colcount;
	for (uint64_t row=0; !done; row++) {

		for (uint32_t col=0; col<colcount; col++) {

			// get the field
			uint32_t	fieldlength;
			sqlrshjsontype	jsontype;
			const char	*field=getFieldForDisplay(sqlrcur,env,
						row,col,&fieldlength,
						convfieldbuffer,
						sizeof(convfieldbuffer),
						&jsontype);

			// check for end-of-result-set condition
			// (since nullsasnulls might be set, we have to do
			// a bit more than just check for a NULL)
			if (!col && !field &&
				sqlrcur->endOfResultSet() &&
				row==sqlrcur->rowCount()) {
				done=true;
				break;
			}

			// open the row
			if (!col) {
				if (jsonl) {
					stdoutput.write(
						"{\"type\":\"row\",\"row\":{");
				} else {
					if (row) {
						stdoutput.write(',');
					}
					stdoutput.write('[');
				}
			} else {
				stdoutput.write(',');
			}

			// write the field
			if (jsonl) {
				const char	*name=
						sqlrcur->getColumnName(col);
				jsonWriteString(&stdoutput,name,
						charstring::getLength(name));
				stdoutput.write(':');
			}
			jsonWriteValue(&stdoutput,field,fieldlength,jsontype);
		}

		// close the row
		if (!done) {
			if (jsonl) {
				stdoutput.write("}}\n");
			} else {
				stdoutput.write(']');
			}
		}
	}

	// close the result set
	// The stats go here rather than in displayStats(), because
	// -nextresultset calls this once per result set.
	if (jsonl) {
		if (env->stats) {
			stdoutput.write("{\"type\":\"stats\",");
			jsonWriteStats(sqlrcur,env);
			stdoutput.write("}\n");
		}
	} else {
		stdoutput.write(']');
		if (env->stats) {
			stdoutput.write(',');
			jsonWriteStats(sqlrcur,env);
		}
		stdoutput.write("}\n");
	}
}

void sqlrsh::displayStats(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// The stats block is plain format only.  json and jsonl carry the
	// same numbers inside the document, which jsonDisplayResultSet()
	// writes.
	if (env->format==SQLRSH_FORMAT_PLAIN && env->stats) {

		// calculate elapsed time
		datetime	end;
		end.initFromSystemDateTime();
		uint64_t	startusec=start.getEpoch()*1000000+
						start.getMicrosecond();
		uint64_t	endusec=end.getEpoch()*1000000+
						end.getMicrosecond();
		double		time=((double)(endusec-startusec))/1000000;

		// display stats
		stdoutput.write('\n');
		stdoutput.printf("	Affected Rows   : ");
		stdoutput.printf("%lld\n",(long long)sqlrcur->affectedRows());
		stdoutput.printf("	Rows Returned   : ");
		stdoutput.printf("%lld\n",(long long)sqlrcur->rowCount());
		stdoutput.printf("	Fields Returned : ");
		stdoutput.printf("%lld\n",
			(long long)sqlrcur->rowCount()*sqlrcur->colCount());
		if (!env->noelapsed) {
			stdoutput.printf("	Elapsed Time    : ");
			stdoutput.printf("%.6f sec\n",time);
		}
		stdoutput.printf("\n");
	}
}

void sqlrsh::jsonWriteStats(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// calculate elapsed time
	datetime	end;
	end.initFromSystemDateTime();
	uint64_t	startusec=start.getEpoch()*1000000+
					start.getMicrosecond();
	uint64_t	endusec=end.getEpoch()*1000000+
					end.getMicrosecond();
	double		time=((double)(endusec-startusec))/1000000;

	stdoutput.printf("\"affectedrows\":%lld,"
				"\"rowsreturned\":%lld,"
				"\"fieldsreturned\":%lld",
				(long long)sqlrcur->affectedRows(),
				(long long)sqlrcur->rowCount(),
				(long long)sqlrcur->rowCount()*
						sqlrcur->colCount());
	if (!env->noelapsed) {
		stdoutput.printf(",\"elapsed\":%.6f",time);
	}
}

void sqlrsh::displayCurrentResultSet(sqlrcursor *sqlrcur, sqlrshenv *env) {
	displayHeader(sqlrcur,env);
	displayResultSet(sqlrcur,env);
	displayStats(sqlrcur,env);
}

// everything the client api reports about one column of a result set
struct sqlrshcolumninfo {
	const char	*name;
	const char	*type;
	uint32_t	length;
	uint32_t	precision;
	uint32_t	scale;
	bool		nullable;
	bool		primarykey;
	bool		unique;
	bool		partofkey;
	// "unsigned" is a keyword
	bool		isunsigned;
	bool		zerofilled;
	bool		binary;
	bool		autoincrement;
};

static void fillColumnInfo(sqlrcursor *sqlrcur, uint32_t col,
					sqlrshcolumninfo *ci) {
	ci->name=sqlrcur->getColumnName(col);
	ci->type=sqlrcur->getColumnType(col);
	ci->length=sqlrcur->getColumnLength(col);
	ci->precision=sqlrcur->getColumnPrecision(col);
	ci->scale=sqlrcur->getColumnScale(col);
	ci->nullable=sqlrcur->getColumnIsNullable(col);
	ci->primarykey=sqlrcur->getColumnIsPrimaryKey(col);
	ci->unique=sqlrcur->getColumnIsUnique(col);
	ci->partofkey=sqlrcur->getColumnIsPartOfKey(col);
	ci->isunsigned=sqlrcur->getColumnIsUnsigned(col);
	ci->zerofilled=sqlrcur->getColumnIsZeroFilled(col);
	ci->binary=sqlrcur->getColumnIsBinary(col);
	ci->autoincrement=sqlrcur->getColumnIsAutoIncrement(col);
}

// the columninfo field names, in the order the writers below use them
static const char * const sqlrshcolumninfokeys[]={
	"name","type","length","precision","scale","nullable","primarykey",
	"unique","partofkey","unsigned","zerofilled","binary","autoincrement",
	NULL
};

void sqlrsh::columninfo(sqlrcursor *sqlrcur, sqlrshenv *env) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
			plainColumnInfo(sqlrcur,env);
			break;
		case SQLRSH_FORMAT_CSV:
			csvColumnInfo(sqlrcur,env);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			jsonColumnInfo(sqlrcur,env);
			break;
	}
}

void sqlrsh::plainColumnInfo(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// a block of tab indented labels per column
	uint32_t	colcount=sqlrcur->colCount();
	for (uint32_t col=0; col<colcount; col++) {

		sqlrshcolumninfo	ci;
		fillColumnInfo(sqlrcur,col,&ci);

		stdoutput.printf("	Name           : %s\n",
					(ci.name)?ci.name:"");
		stdoutput.printf("	Type           : %s\n",
					(ci.type)?ci.type:"");
		stdoutput.printf("	Length         : %lld\n",
					(long long)ci.length);
		stdoutput.printf("	Precision      : %lld\n",
					(long long)ci.precision);
		stdoutput.printf("	Scale          : %lld\n",
					(long long)ci.scale);
		stdoutput.printf("	Nullable       : %s\n",
					(ci.nullable)?"true":"false");
		stdoutput.printf("	Primary Key    : %s\n",
					(ci.primarykey)?"true":"false");
		stdoutput.printf("	Unique         : %s\n",
					(ci.unique)?"true":"false");
		stdoutput.printf("	Part Of Key    : %s\n",
					(ci.partofkey)?"true":"false");
		stdoutput.printf("	Unsigned       : %s\n",
					(ci.isunsigned)?"true":"false");
		stdoutput.printf("	Zero Filled    : %s\n",
					(ci.zerofilled)?"true":"false");
		stdoutput.printf("	Binary         : %s\n",
					(ci.binary)?"true":"false");
		stdoutput.printf("	Auto Increment : %s\n",
					(ci.autoincrement)?"true":"false");
		stdoutput.write('\n');
	}
}

void sqlrsh::csvColumnInfo(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// one row per column, with a header row
	for (const char * const *key=sqlrshcolumninfokeys; *key; key++) {
		if (key!=sqlrshcolumninfokeys) {
			stdoutput.write(',');
		}
		csvWriteField(*key,charstring::getLength(*key));
	}
	stdoutput.write('\n');

	uint32_t	colcount=sqlrcur->colCount();
	for (uint32_t col=0; col<colcount; col++) {

		sqlrshcolumninfo	ci;
		fillColumnInfo(sqlrcur,col,&ci);

		csvWriteField(ci.name,charstring::getLength(ci.name));
		stdoutput.write(',');
		csvWriteField(ci.type,charstring::getLength(ci.type));
		stdoutput.printf(",%lld,%lld,%lld",
					(long long)ci.length,
					(long long)ci.precision,
					(long long)ci.scale);
		stdoutput.write((ci.nullable)?",true":",false");
		stdoutput.write((ci.primarykey)?",true":",false");
		stdoutput.write((ci.unique)?",true":",false");
		stdoutput.write((ci.partofkey)?",true":",false");
		stdoutput.write((ci.isunsigned)?",true":",false");
		stdoutput.write((ci.zerofilled)?",true":",false");
		stdoutput.write((ci.binary)?",true":",false");
		stdoutput.write((ci.autoincrement)?",true":",false");
		stdoutput.write('\n');
	}
}

void sqlrsh::jsonColumnInfo(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// json and jsonl agree here.  A column list is one object on one
	// line either way, so there's nothing to stream.
	stdoutput.write("{\"columninfo\":[");

	uint32_t	colcount=sqlrcur->colCount();
	for (uint32_t col=0; col<colcount; col++) {

		if (col) {
			stdoutput.write(',');
		}

		sqlrshcolumninfo	ci;
		fillColumnInfo(sqlrcur,col,&ci);

		stdoutput.write("{\"name\":");
		jsonWriteString(&stdoutput,ci.name,
					charstring::getLength(ci.name));
		stdoutput.write(",\"type\":");
		jsonWriteString(&stdoutput,ci.type,
					charstring::getLength(ci.type));
		stdoutput.printf(",\"length\":%lld,"
					"\"precision\":%lld,"
					"\"scale\":%lld",
					(long long)ci.length,
					(long long)ci.precision,
					(long long)ci.scale);
		// real json booleans, so a reader never has to match a word
		stdoutput.write((ci.nullable)?
				",\"nullable\":true":",\"nullable\":false");
		stdoutput.write((ci.primarykey)?
				",\"primarykey\":true":",\"primarykey\":false");
		stdoutput.write((ci.unique)?
				",\"unique\":true":",\"unique\":false");
		stdoutput.write((ci.partofkey)?
				",\"partofkey\":true":",\"partofkey\":false");
		stdoutput.write((ci.isunsigned)?
				",\"unsigned\":true":",\"unsigned\":false");
		stdoutput.write((ci.zerofilled)?
				",\"zerofilled\":true":",\"zerofilled\":false");
		stdoutput.write((ci.binary)?
				",\"binary\":true":",\"binary\":false");
		stdoutput.write((ci.autoincrement)?
				",\"autoincrement\":true":
				",\"autoincrement\":false");
		stdoutput.write('}');
	}

	stdoutput.write("]}\n");
}

void sqlrsh::writeScalar(sqlrshenv *env,
				const char *name, const char *value) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.printf("%s\n",(value)?value:"");
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			// one object, one line, keyed by the command name
			stdoutput.write('{');
			jsonWriteString(&stdoutput,name,
					charstring::getLength(name));
			stdoutput.write(':');
			if (value) {
				jsonWriteString(&stdoutput,value,
						charstring::getLength(value));
			} else {
				stdoutput.write("null");
			}
			stdoutput.write("}\n");
			break;
	}
}

void sqlrsh::writeScalarNumber(sqlrshenv *env,
				const char *name, int64_t value) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.printf("%lld\n",(long long)value);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			stdoutput.write('{');
			jsonWriteString(&stdoutput,name,
					charstring::getLength(name));
			stdoutput.printf(":%lld}\n",(long long)value);
			break;
	}
}

void sqlrsh::writeScalarBoolean(sqlrshenv *env,
				const char *name, bool value) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.write((value)?"true\n":"false\n");
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			// a json boolean, so a reader doesn't have to match
			// a word
			stdoutput.write('{');
			jsonWriteString(&stdoutput,name,
					charstring::getLength(name));
			stdoutput.write((value)?":true}\n":":false}\n");
			break;
	}
}

void sqlrsh::writeTimeout(sqlrshenv *env, const char *name,
					int32_t sec, int32_t usec) {

	// Either half negative means the timeout is off, so it goes out as a
	// plain -1 rather than as arithmetic on two negative numbers.
	char	value[64];
	if (sec<0 || usec<0) {
		charstring::copy(value,"-1");
	} else {
		charstring::printf(value,sizeof(value),
					"%d.%06d",sec,usec);
	}
	writeScalar(env,name,value);
}

void sqlrsh::writeTimeoutSet(sqlrshenv *env, const char *name,
					const char *label,
					uint32_t sec, uint32_t usec) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.printf("%s set to %d.%06d seconds\n",
							label,sec,usec);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			{
				// same seconds and microseconds form the
				// matching get command reports
				char	value[64];
				charstring::printf(value,sizeof(value),
							"%d.%06d",sec,usec);
				writeScalar(env,name,value);
			}
			break;
	}
}

bool sqlrsh::writeConnectionString(sqlrconnection *sqlrcon, sqlrshenv *env,
					const char *name, const char *value) {

	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,name,value);
	return true;
}

bool sqlrsh::ping(sqlrconnection *sqlrcon, sqlrshenv *env) {
	bool	result=sqlrcon->ping();
	if (!result && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			if (result) {
				stdoutput.printf("	The database is up.\n");
			} else {
				stdoutput.printf(
					"	The database is down.\n");
			}
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			// a boolean, so a reader doesn't have to match a
			// sentence
			stdoutput.write((result)?"{\"ping\":true}\n":
						"{\"ping\":false}\n");
			break;
	}
	return true;
}

bool sqlrsh::lastinsertid(sqlrconnection *sqlrcon, sqlrshenv *env) {
	bool		retval=false;
	uint64_t	id=sqlrcon->getLastInsertId();
	if (id!=0 || !sqlrcon->errorMessage()) {
		writeScalarNumber(env,"lastinsertid",(int64_t)id);
		retval=true;
	}
	return retval;
}

bool sqlrsh::identify(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->identify();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"identify",value);
	return true;
}

bool sqlrsh::dbversion(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->dbVersion();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"dbversion",value);
	return true;
}

bool sqlrsh::dbhostname(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->dbHostName();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"dbhostname",value);
	return true;
}

bool sqlrsh::dbipaddress(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->dbIpAddress();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"dbipaddress",value);
	return true;
}

bool sqlrsh::bindformat(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->bindFormat();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"bindformat",value);
	return true;
}

bool sqlrsh::nextvalformat(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->nextvalFormat();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"nextvalformat",value);
	return true;
}

bool sqlrsh::getisolationlevel(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->getIsolationLevel();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"isolationlevel",value);
	return true;
}

bool sqlrsh::usecatalog(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	char	*catalog=commandArgument(args);
	if (!catalog) {
		displayError(env,NULL,"usecatalog needs a catalog name",0);
		return false;
	}

	bool	success=sqlrcon->selectCatalog(catalog);
	delete[] catalog;

	if (!success) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	return true;
}

bool sqlrsh::useschema(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	char	*schema=commandArgument(args);
	if (!schema) {
		displayError(env,NULL,"useschema needs a schema name",0);
		return false;
	}

	bool	success=sqlrcon->selectSchema(schema);
	delete[] schema;

	if (!success) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	return true;
}

bool sqlrsh::resumesession(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	char	*arg=commandArgument(args);
	if (!arg || !character::isDigit(arg[0])) {
		delete[] arg;
		displayError(env,NULL,
			"resumesession needs a port, "
			"and a socket for a unix socket session",0);
		return false;
	}

	uint16_t	port=(uint16_t)charstring::convertToInteger(arg);

	// the socket is the rest of the argument, and is optional
	const char	*socket=arg;
	while (*socket && !character::isWhitespace(*socket)) {
		socket++;
	}
	while (character::isWhitespace(*socket)) {
		socket++;
	}

	// The connection keeps this pointer rather than copying it, so env
	// owns it for the rest of the run.
	delete[] env->resumesocket;
	env->resumesocket=charstring::duplicate(socket);
	delete[] arg;

	bool	success=sqlrcon->resumeSession(port,env->resumesocket);

	if (!success) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	return true;
}

bool sqlrsh::bindvariabledelimiters(sqlrconnection *sqlrcon,
					sqlrshenv *env, const char *args) {

	// with an argument it sets the delimiters
	char	*delimiters=commandArgument(args);
	if (delimiters) {
		sqlrcon->setBindVariableDelimiters(delimiters);
		delete[] delimiters;
		return true;
	}

	// Without one it reports them.  There's no
	// getBindVariableDelimiters(), so the answer is assembled out of the
	// four supported() methods.
	char	current[5];
	uint8_t	index=0;
	if (sqlrcon->getBindVariableDelimiterQuestionMarkSupported()) {
		current[index++]='?';
	}
	if (sqlrcon->getBindVariableDelimiterColonSupported()) {
		current[index++]=':';
	}
	if (sqlrcon->getBindVariableDelimiterAtSignSupported()) {
		current[index++]='@';
	}
	if (sqlrcon->getBindVariableDelimiterDollarSignSupported()) {
		current[index++]='$';
	}
	current[index]='\0';

	writeScalar(env,"bindvariabledelimiters",current);
	return true;
}

bool sqlrsh::bindvariabledelimitersupported(sqlrconnection *sqlrcon,
					sqlrshenv *env, const char *args) {

	char	*delimiter=commandArgument(args);

	bool	supported=false;
	bool	valid=true;
	switch ((delimiter)?delimiter[0]:'\0') {
		case '?':
			supported=sqlrcon->
			    getBindVariableDelimiterQuestionMarkSupported();
			break;
		case ':':
			supported=sqlrcon->
			    getBindVariableDelimiterColonSupported();
			break;
		case '@':
			supported=sqlrcon->
			    getBindVariableDelimiterAtSignSupported();
			break;
		case '$':
			supported=sqlrcon->
			    getBindVariableDelimiterDollarSignSupported();
			break;
		default:
			valid=false;
			break;
	}
	delete[] delimiter;

	if (!valid) {
		displayError(env,NULL,
			"bindvariabledelimitersupported needs "
			"one of ? : @ $",0);
		return false;
	}

	writeScalarBoolean(env,"bindvariabledelimitersupported",supported);
	return true;
}

bool sqlrsh::databasefeature(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	char	*feature=commandArgument(args);
	if (!feature) {
		displayError(env,NULL,
			"databasefeature needs a feature name",0);
		return false;
	}

	const char	*value=sqlrcon->getDatabaseFeature(feature);
	delete[] feature;

	// A NULL means the fetch failed or the feature name wasn't one the
	// database knows.  Either way the command failed.
	if (!value) {
		if (sqlrcon->errorMessage()) {
			displayError(env,NULL,
					sqlrcon->errorMessage(),
					sqlrcon->errorNumber());
		} else {
			displayError(env,NULL,
				"databasefeature was given a feature "
				"the database doesn't have",0);
		}
		return false;
	}

	writeScalar(env,"databasefeature",value);
	return true;
}

bool sqlrsh::columninfocommand(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *args) {

	// with an argument it turns column info on or off, without one it
	// dumps the metadata
	char	*arg=commandArgument(args);
	if (!arg) {
		columninfo(sqlrcur,env);
		return true;
	}

	// A bad argument is a failed command here, rather than quietly
	// meaning off, the way the older toggles read one.
	bool	on=false;
	bool	valid=false;
	if (!charstring::compareIgnoringCase(arg,"on")) {
		on=true;
		valid=true;
	} else if (!charstring::compareIgnoringCase(arg,"off")) {
		valid=true;
	}
	delete[] arg;

	if (!valid) {
		displayError(env,NULL,"columninfo needs on or off",0);
		return false;
	}

	if (on) {
		sqlrcur->getColumnInfo();
	} else {
		sqlrcur->dontGetColumnInfo();
	}
	return true;
}

bool sqlrsh::columncase(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *args) {

	char	*arg=commandArgument(args);
	bool	valid=true;
	if (!arg) {
		valid=false;
	} else if (!charstring::compareIgnoringCase(arg,"mixed")) {
		sqlrcur->mixedCaseColumnNames();
	} else if (!charstring::compareIgnoringCase(arg,"upper")) {
		sqlrcur->upperCaseColumnNames();
	} else if (!charstring::compareIgnoringCase(arg,"lower")) {
		sqlrcur->lowerCaseColumnNames();
	} else {
		valid=false;
	}
	delete[] arg;

	if (!valid) {
		displayError(env,NULL,
			"columncase needs mixed, upper, or lower",0);
		return false;
	}
	return true;
}

bool sqlrsh::resumeresultset(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *args) {

	// the id is optional - without one the cursor's own id is used
	char	*arg=commandArgument(args);
	if (arg && !character::isDigit(arg[0])) {
		delete[] arg;
		displayError(env,NULL,
			"resumeresultset needs a result set id, "
			"or no argument to resume its own",0);
		return false;
	}

	uint16_t	id=(arg)?
				(uint16_t)charstring::convertToInteger(arg):
				sqlrcur->getResultSetId();
	delete[] arg;

	if (!sqlrcur->resumeResultSet(id)) {
		const char	*error=sqlrcur->errorMessage();
		if (charstring::isNullOrEmpty(error)) {
			error="Couldn't resume the result set.";
		}
		displayError(env,NULL,error,sqlrcur->errorNumber());
		return false;
	}

	displayCurrentResultSet(sqlrcur,env);
	return true;
}

bool sqlrsh::resumecachedresultset(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *args) {

	char	*arg=commandArgument(args);
	if (arg && !character::isDigit(arg[0])) {
		delete[] arg;
		displayError(env,NULL,
			"resumecachedresultset needs a result set id, "
			"and optionally a file name to keep caching to",0);
		return false;
	}

	uint16_t	id=(arg)?
				(uint16_t)charstring::convertToInteger(arg):
				sqlrcur->getResultSetId();

	// the file name is the rest of the argument, and is optional
	char	*newcacheto=NULL;
	if (arg) {
		const char	*rest=arg;
		while (*rest && !character::isWhitespace(*rest)) {
			rest++;
		}
		while (character::isWhitespace(*rest)) {
			rest++;
		}
		if (*rest) {
			stringbuffer	fn;
			fn.append(sqlrpth->getCacheDir())->append(rest);
			newcacheto=fn.detachString();
		}
	}
	delete[] arg;

	bool	success=sqlrcur->resumeCachedResultSet(id,newcacheto);

	// The cursor doesn't copy the name, so whichever one it's holding now
	// has to stay alive.  Asking it is the only way to know - it can
	// return before it gets as far as taking the new one.
	if (sqlrcur->getCacheFileName()==newcacheto) {
		delete[] env->cacheto;
		env->cacheto=newcacheto;
	} else {
		delete[] newcacheto;
	}

	if (!success) {
		const char	*error=sqlrcur->errorMessage();
		if (charstring::isNullOrEmpty(error)) {
			error="Couldn't resume the cached result set.";
		}
		displayError(env,NULL,error,sqlrcur->errorNumber());
		return false;
	}

	displayCurrentResultSet(sqlrcur,env);
	return true;
}

void sqlrsh::clientversion(sqlrconnection *sqlrcon, sqlrshenv *env) {
	writeScalar(env,"clientversion",sqlrcon->clientVersion());
}

bool sqlrsh::serverversion(sqlrconnection *sqlrcon, sqlrshenv *env) {
	const char	*value=sqlrcon->serverVersion();
	if (!value && sqlrcon->errorMessage()) {
		displayError(env,NULL,
				sqlrcon->errorMessage(),
				sqlrcon->errorNumber());
		return false;
	}
	writeScalar(env,"serverversion",value);
	return true;
}

bool sqlrsh::usageError(sqlrshenv *env, const char *usage) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stderror.printf("%s\n",usage);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			// an error object, on stderr, the same shape a failed
			// query gives them
			displayError(env,NULL,usage,0);
			break;
	}
	return false;
}

// every form of the inputbind command, in one place
static const char	*inputbindusage=
	"usage: inputbind [variable] = [value]\n"
	"       inputbind [variable] is null\n"
	"       inputbind [variable] string [length] = [value]";

bool sqlrsh::inputbind(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *command) {

	// the command word on its own
	if (!command[9]) {
		return usageError(env,inputbindusage);
	}

	// sanity check
	const char	*ptr=command+10;
	const char	*space=charstring::findFirst(ptr,' ');
	if (!space) {
		return usageError(env,
				"usage: inputbind [variable] = [value]");
	}

	// get the variable name
	char	*variable=charstring::duplicate(ptr,space-ptr);

	// an explicit length comes between the variable and the =
	bool		haslength=false;
	uint32_t	length=0;
	ptr=space;
	if (!charstring::compareIgnoringCase(ptr+1,"string ",7)) {
		const char	*lenptr=ptr+8;
		const char	*lenend=charstring::findFirst(lenptr,' ');
		char		*len=(lenend)?
				charstring::duplicate(lenptr,lenend-lenptr):
				NULL;
		if (!len || !charstring::isInteger(len)) {
			delete[] len;
			delete[] variable;
			return usageError(env,
				"usage: inputbind [variable] string "
				"[length] = [value]");
		}
		length=(uint32_t)charstring::convertToInteger(len);
		delete[] len;
		haslength=true;
		ptr=lenend;
	}

	// move on
	if (*(ptr+1)=='=' && *(ptr+2)==' ') {
		ptr=ptr+3;
	} else if (!charstring::compareIgnoringCase(ptr+1,"is null")) {
		ptr=NULL;
	} else {
		delete[] variable;
		return usageError(env,inputbindusage);
	}

	// get the value
	char	*value=charstring::duplicate(ptr);
	charstring::bothTrim(value);
	size_t	valuelen=charstring::getLength(value);

	// if the bind variable is already defined, clear it...
	sqlrshbindvalue	*bv=NULL;
	bool		predefined=env->inputbinds.getValue(variable,&bv);
	if (predefined) {
		deleteBindValue(bv);
	}

	// define the variable
	bv=new sqlrshbindvalue;

	// a value in quotes is a string
	// (it takes at least two characters to have both of them)
	bool	quoted=(valuelen>=2 &&
			((value[0]=='\'' && value[valuelen-1]=='\'') ||
			(value[0]=='"' && value[valuelen-1]=='"')));
	if (quoted) {

		// trim off the quotes and unescape what's between them
		char		*unescaped=NULL;
		uint64_t	unescapedlen=0;
		charstring::unescape(value+1,valuelen-2,
					&unescaped,&unescapedlen);
		delete[] value;
		value=unescaped;
		valuelen=(size_t)unescapedlen;
	}

	// first handle nulls, then a value given with an explicit length,
	// then quoted values, which are strings...
	// if it's unquoted, check to see if it's an integer, float or date
	// if it's not, then it's a string
	if (!value) {
		bv->type=SQLRCLIENTBINDVARTYPE_NULL;
	} else if (haslength) {
		setStringValue(bv,value,(uint32_t)valuelen,length);
		delete[] value;
	} else if (quoted) {
		bv->type=SQLRCLIENTBINDVARTYPE_STRING;
		bv->stringval.value=value;
		bv->stringval.length=(uint32_t)valuelen;
	} else if (charstring::contains(value,"/") &&
			charstring::contains(value,":")) {
		int16_t	year;
		int16_t	month;
		int16_t	day;
		int16_t	hour;
		int16_t	minute;
		int16_t	second;
		int32_t	microsecond;
		bool	isnegative;
		datetime::parse(value,false,false,"/",
					&year,&month,&day,
					&hour,&minute,&second,
					&microsecond,&isnegative);
		bv->type=SQLRCLIENTBINDVARTYPE_DATE;
		bv->dateval.year=year;
		bv->dateval.month=month;
		bv->dateval.day=day;
		bv->dateval.hour=hour;
		bv->dateval.minute=minute;
		bv->dateval.second=second;
		bv->dateval.microsecond=microsecond;
		bv->dateval.tz="";
		bv->dateval.isnegative=isnegative;
		delete[] value;
	} else if (charstring::isInteger(value)) {
		bv->type=SQLRCLIENTBINDVARTYPE_INTEGER;
		bv->integerval=charstring::convertToInteger(value);
		delete[] value;
	} else if (charstring::isNumber(value)) {
		bv->type=SQLRCLIENTBINDVARTYPE_DOUBLE;
		bv->doubleval.value=charstring::convertToFloatC(value);
		bv->doubleval.precision=valuelen-((value[0]=='-')?2:1);
		bv->doubleval.scale=
			charstring::findFirst(value,'.')-value+
			((value[0]=='-')?0:1);
		delete[] value;
	} else {
		bv->type=SQLRCLIENTBINDVARTYPE_STRING;
		bv->stringval.value=value;
		bv->stringval.length=(uint32_t)valuelen;
	}

	// put the bind variable in the list
	// (the list keeps the name it already has, so a redefinition has to
	// free the one it just made)
	env->inputbinds.setValue(variable,bv);
	if (predefined) {
		delete[] variable;
	}

	return true;
}

bool sqlrsh::inputbindlob(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *command,
				const char *name,
				sqlrclientbindvartype_t type) {

	char		usage[128];
	const char	*ptr=command+charstring::getLength(name);

	// the command word on its own
	if (!*ptr) {
		charstring::printf(usage,sizeof(usage),
				"usage: %s [variable] = [value]\n"
				"       %s [variable] is null",name,name);
		return usageError(env,usage);
	}

	// sanity check
	ptr++;
	const char	*space=charstring::findFirst(ptr,' ');
	if (!space) {
		charstring::printf(usage,sizeof(usage),
				"usage: %s [variable] = [value]",name);
		return usageError(env,usage);
	}

	// get the variable name
	char	*variable=charstring::duplicate(ptr,space-ptr);

	// move on
	ptr=space;
	if (*(ptr+1)=='=' && *(ptr+2)==' ') {
		ptr=ptr+3;
	} else if (!charstring::compareIgnoringCase(ptr+1,"is null")) {
		ptr=NULL;
	} else {
		charstring::printf(usage,sizeof(usage),
				"usage: %s [variable] = [value]\n"
				"       %s [variable] is null",name,name);
		delete[] variable;
		return usageError(env,usage);
	}

	// get the value
	char	*value=charstring::duplicate(ptr);
	charstring::bothTrim(value);
	size_t	valuelen=charstring::getLength(value);

	// if the bind variable is already defined, clear it...
	sqlrshbindvalue	*bv=NULL;
	bool		predefined=env->inputbinds.getValue(variable,&bv);
	if (predefined) {
		deleteBindValue(bv);
	}

	// define the variable
	bv=new sqlrshbindvalue;

	// a value in quotes gets them trimmed off and what's between them
	// unescaped, an unquoted value is taken as it stands
	if (!value) {
		bv->type=SQLRCLIENTBINDVARTYPE_NULL;
	} else if (valuelen>=2 &&
			((value[0]=='\'' && value[valuelen-1]=='\'') ||
			(value[0]=='"' && value[valuelen-1]=='"'))) {

		char		*unescaped=NULL;
		uint64_t	unescapedlen=0;
		charstring::unescape(value+1,valuelen-2,
					&unescaped,&unescapedlen);
		delete[] value;

		bv->type=type;
		bv->stringval.value=unescaped;
		bv->stringval.length=(uint32_t)unescapedlen;

	} else {
		bv->type=type;
		bv->stringval.value=value;
		bv->stringval.length=(uint32_t)valuelen;
	}

	// put the bind variable in the list
	// (the list keeps the name it already has, so a redefinition has to
	// free the one it just made)
	env->inputbinds.setValue(variable,bv);
	if (predefined) {
		delete[] variable;
	}

	return true;
}

// every form of the outputbind command, in one place
static const char	*outputbindusage=
	"usage: outputbind [variable] string [length]\n"
	"       outputbind [variable] integer\n"
	"       outputbind [variable] double [precision] [scale]\n"
	"       outputbind [variable] date\n"
	"       outputbind [variable] blob\n"
	"       outputbind [variable] clob\n"
	"       outputbind [variable] cursor";

bool sqlrsh::outputbind(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *command) {

	// split the command on ' '
	char		**parts;
	uint64_t	partcount;
	charstring::split(command," ",true,&parts,&partcount);

	// sanity check...
	bool	sane=true;
	bool	predefined=false;
	if (partcount>2 && !charstring::compare(parts[0],"outputbind")) {

		// if the bind variable is already defined, clear it...
		sqlrshbindvalue	*bv=NULL;
		predefined=env->outputbinds.getValue(parts[1],&bv);
		if (predefined) {
			deleteBindValue(bv);
		}

		// define the variable
		bv=new sqlrshbindvalue;

		if (!charstring::compareIgnoringCase(
						parts[2],"string") &&
						partcount==4) {
			bv->type=SQLRCLIENTBINDVARTYPE_STRING;
			bv->stringval.value=NULL;
			bv->stringval.length=0;
			bv->outputstringbindlength=
				charstring::convertToInteger(parts[3]);
		} else if (!charstring::compareIgnoringCase(
						parts[2],"integer") &&
						partcount==3) {
			bv->type=SQLRCLIENTBINDVARTYPE_INTEGER;
			bv->integerval=0;
		} else if (!charstring::compareIgnoringCase(
						parts[2],"double") &&
						partcount==5) {
			bv->type=SQLRCLIENTBINDVARTYPE_DOUBLE;
			bv->doubleval.value=0.0;
			bv->doubleval.precision=
				charstring::convertToInteger(parts[3]);
			bv->doubleval.scale=
				charstring::convertToInteger(parts[4]);
		} else if (!charstring::compareIgnoringCase(
						parts[2],"date") &&
						partcount==3) {
			bv->type=SQLRCLIENTBINDVARTYPE_DATE;
			bv->dateval.year=0;
			bv->dateval.month=0;
			bv->dateval.day=0;
			bv->dateval.hour=0;
			bv->dateval.minute=0;
			bv->dateval.second=0;
			bv->dateval.microsecond=0;
			bv->dateval.tz="";
			bv->dateval.isnegative=false;
		} else if (!charstring::compareIgnoringCase(
						parts[2],"blob") &&
						partcount==3) {
			bv->type=SQLRCLIENTBINDVARTYPE_BLOB;
			bv->stringval.value=NULL;
			bv->stringval.length=0;
		} else if (!charstring::compareIgnoringCase(
						parts[2],"clob") &&
						partcount==3) {
			bv->type=SQLRCLIENTBINDVARTYPE_CLOB;
			bv->stringval.value=NULL;
			bv->stringval.length=0;
		} else if (!charstring::compareIgnoringCase(
						parts[2],"cursor") &&
						partcount==3) {
			// a cursor bind has no value of its own
			bv->type=SQLRCLIENTBINDVARTYPE_CURSOR;
		} else {
			delete bv;
			sane=false;
		}

		// put the bind variable in the list
		if (sane) {
			env->outputbinds.setValue(parts[1],bv);
		}

	} else {
		sane=false;
	}

	// clean up
	// (the list takes parts[1] as its key, unless it already had one by
	// that name, in which case this one has to go too)
	if (sane) {
		delete[] parts[0];
		if (predefined) {
			delete[] parts[1];
		}
		for (uint64_t i=2; i<partcount; i++) {
			delete[] parts[i];
		}
	} else {
		usageError(env,outputbindusage);
		for (uint64_t i=0; i<partcount; i++) {
			delete[] parts[i];
		}
	}
	delete[] parts;

	return sane;
}

// every form of the inputoutputbind command, in one place
static const char	*inputoutputbindusage=
	"usage: inputoutputbind [variable] string [length] = [value]\n"
	"       inputoutputbind [variable] integer = [value]\n"
	"       inputoutputbind [variable] double "
					"[precision] [scale] = [value]\n"
	"       inputoutputbind [variable] date = [value]\n"
	"       inputoutputbind [variable] [type] ... is null";

bool sqlrsh::inputoutputbind(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *command) {

	// get the value
	char		*value=NULL;
	const char	*equals=charstring::findFirst(command,'=');
	if (equals) {
		value=charstring::duplicate(equals+1);
		charstring::bothTrim(value);
		charstring::bothTrim(value,'\'');
	} else if (charstring::compare(
			command+charstring::getLength(command)-8," is null")) {
		return usageError(env,inputoutputbindusage);
	}

	// split the command on ' '
	char		**parts;
	uint64_t	partcount;
	charstring::split(command," ",true,&parts,&partcount);

	// sanity check...
	bool	sane=true;
	bool	predefined=false;
	if (partcount>=5 && !charstring::compare(parts[0],"inputoutputbind")) {

		// if the bind variable is already defined, clear it...
		sqlrshbindvalue	*bv=NULL;
		predefined=env->inputoutputbinds.getValue(parts[1],&bv);
		if (predefined) {
			deleteBindValue(bv);
		}

		// define the variable
		bv=new sqlrshbindvalue;

		if (!charstring::compareIgnoringCase(
						parts[2],"string") &&
						partcount>=6) {
			// inputoutputbind 1 string length = 'string'
			bv->type=SQLRCLIENTBINDVARTYPE_STRING;
			bv->outputstringbindlength=
				charstring::convertToInteger(parts[3]);
			char		*unescaped=NULL;
			uint64_t	unescapedlen=0;
			charstring::unescape(value,
					charstring::getLength(value),
					&unescaped,&unescapedlen);
			bv->stringval.value=unescaped;
			bv->stringval.length=(uint32_t)unescapedlen;
		} else if (!charstring::compareIgnoringCase(
						parts[2],"integer") &&
						partcount==5) {
			// inputoutputbind 1 integer = value
			bv->type=SQLRCLIENTBINDVARTYPE_INTEGER;
			bv->integerval=charstring::convertToInteger(value);
		} else if (!charstring::compareIgnoringCase(
						parts[2],"double") &&
						partcount==7) {
			// inputoutputbind 1 double prec scale = value
			bv->type=SQLRCLIENTBINDVARTYPE_DOUBLE;
			bv->doubleval.value=charstring::convertToFloatC(value);
			bv->doubleval.precision=
				charstring::convertToInteger(parts[3]);
			bv->doubleval.scale=
				charstring::convertToInteger(parts[4]);
		} else if (!charstring::compareIgnoringCase(
						parts[2],"date") &&
						partcount>=5) {
			// inputoutputbind 1 date = '...'
			int16_t	year;
			int16_t	month;
			int16_t	day;
			int16_t	hour;
			int16_t	minute;
			int16_t	second;
			int32_t	microsecond;
			bool	isnegative;
			datetime::parse(value,false,false,"/",
						&year,&month,&day,
						&hour,&minute,&second,
						&microsecond,&isnegative);
			bv->type=SQLRCLIENTBINDVARTYPE_DATE;
			bv->dateval.year=year;
			bv->dateval.month=month;
			bv->dateval.day=day;
			bv->dateval.hour=hour;
			bv->dateval.minute=minute;
			bv->dateval.second=second;
			bv->dateval.microsecond=microsecond;
			bv->dateval.tz="";
			bv->dateval.isnegative=isnegative;
		} else {
			delete bv;
			sane=false;
		}

		// put the bind variable in the list
		if (sane) {
			env->inputoutputbinds.setValue(parts[1],bv);
		}

	} else {
		sane=false;
	}

	// clean up
	// (the list takes parts[1] as its key, unless it already had one by
	// that name, in which case this one has to go too)
	if (sane) {
		delete[] parts[0];
		if (predefined) {
			delete[] parts[1];
		}
		for (uint64_t i=2; i<partcount; i++) {
			delete[] parts[i];
		}
	} else {
		usageError(env,inputoutputbindusage);
		for (uint64_t i=0; i<partcount; i++) {
			delete[] parts[i];
		}
	}
	delete[] parts;
	delete[] value;

	return sane;
}

static void writeBindString(sqlrshbindvalue *bv) {

	if (!bv->stringval.value) {
		stdoutput.write("(null)");
		return;
	}
	stdoutput.write(bv->stringval.value,(size_t)bv->stringval.length);
}

static void writeBindLob(sqlrshbindvalue *bv) {

	if (!bv->stringval.value) {
		stdoutput.write("(null)");
		return;
	}
	stdoutput.safePrint(bv->stringval.value,bv->stringval.length);
}

void sqlrsh::printbinds(const char *type,
			dictionary<char *, sqlrshbindvalue *> *binds) {

	stdoutput.printf("%s bind variables:\n",type);

	for (listnode<char *> *node=binds->getKeys()->getFirst();
						node; node=node->getNext()) {

		stdoutput.printf("    %s ",node->getValue());
		sqlrshbindvalue	*bv=binds->getValue(node->getValue());
		if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
			stdoutput.printf("(STRING) = ");
			writeBindString(bv);
			stdoutput.printf("\n");
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
			stdoutput.printf("(INTEGER) = %lld\n",
						(long long)bv->integerval);
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
			stdoutput.printf("(DOUBLE %d,%d) = %*.*f\n",
						bv->doubleval.precision,
						bv->doubleval.scale,
						(int)bv->doubleval.precision,
						(int)bv->doubleval.scale,
						bv->doubleval.value);
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
			stdoutput.printf("(DATE) = %02d/%02d/%04d "
						"%s%02d:%02d:%02d.%06d %s\n",
						bv->dateval.month,
						bv->dateval.day,
						bv->dateval.year,
						(bv->dateval.isnegative)?"-":"",
						bv->dateval.hour,
						bv->dateval.minute,
						bv->dateval.second,
						bv->dateval.microsecond,
						bv->dateval.tz);
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_BLOB) {
			stdoutput.printf("(BLOB) = ");
			writeBindLob(bv);
			stdoutput.printf("\n");
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_CLOB) {
			stdoutput.printf("(CLOB) = ");
			writeBindLob(bv);
			stdoutput.printf("\n");
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_CURSOR) {
			// a cursor bind has no value to write
			stdoutput.printf("(CURSOR)\n");
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_NULL) {
			stdoutput.printf("NULL\n");
		}
	}
}

void sqlrsh::jsonPrintBinds(const char *key,
			dictionary<char *, sqlrshbindvalue *> *binds) {

	jsonWriteString(&stdoutput,key,charstring::getLength(key));
	stdoutput.write(":{");

	char	buffer[256];

	bool	first=true;
	for (listnode<char *> *node=binds->getKeys()->getFirst();
						node; node=node->getNext()) {

		if (!first) {
			stdoutput.write(',');
		}
		first=false;

		const char	*name=node->getValue();
		jsonWriteString(&stdoutput,name,charstring::getLength(name));
		stdoutput.write(':');

		// the value goes out as the json type that matches
		// (a cursor is a null - it has no value json can carry)
		sqlrshbindvalue	*bv=binds->getValue(node->getValue());
		if (bv->type==SQLRCLIENTBINDVARTYPE_STRING ||
				bv->type==SQLRCLIENTBINDVARTYPE_BLOB ||
				bv->type==SQLRCLIENTBINDVARTYPE_CLOB) {
			jsonWriteString(&stdoutput,bv->stringval.value,
						bv->stringval.length);
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
			stdoutput.printf("%lld",(long long)bv->integerval);
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
			charstring::printf(buffer,sizeof(buffer),"%*.*f",
						(int)bv->doubleval.precision,
						(int)bv->doubleval.scale,
						bv->doubleval.value);
			charstring::bothTrim(buffer);
			jsonWriteValue(&stdoutput,buffer,
					charstring::getLength(buffer),
					SQLRSH_JSONTYPE_NUMBER);
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
			charstring::printf(buffer,sizeof(buffer),
					"%02d/%02d/%04d %s%02d:%02d:%02d.%06d %s",
					bv->dateval.month,
					bv->dateval.day,
					bv->dateval.year,
					(bv->dateval.isnegative)?"-":"",
					bv->dateval.hour,
					bv->dateval.minute,
					bv->dateval.second,
					bv->dateval.microsecond,
					bv->dateval.tz);
			jsonWriteString(&stdoutput,buffer,
					charstring::getLength(buffer));
		} else {
			stdoutput.write("null");
		}
	}

	stdoutput.write('}');
}

void sqlrsh::printbindlist(sqlrshenv *env,
			const char *type,
			const char *key,
			dictionary<char *, sqlrshbindvalue *> *binds) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			printbinds(type,binds);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			// one object, one line
			stdoutput.write('{');
			jsonPrintBinds(key,binds);
			stdoutput.write("}\n");
			break;
	}
}

bool sqlrsh::clearbindcommand(sqlrshenv *env,
			const char *name,
			dictionary<char *, sqlrshbindvalue *> *binds,
			const char *args) {

	// no variable clears the whole list
	char	*variable=commandArgument(args);
	if (!variable) {
		env->clearbinds(binds);
		return true;
	}

	bool	cleared=env->clearbind(binds,variable);
	delete[] variable;

	// A variable that isn't in the list is a failed command, rather than
	// a quiet no-op, the way a bad argument is elsewhere.
	if (!cleared) {
		char	error[128];
		charstring::printf(error,sizeof(error),
				"%s was given a variable "
				"that isn't in the list",name);
		displayError(env,NULL,error,0);
		return false;
	}

	return true;
}

bool sqlrsh::fetchfrombindcursor(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *args) {

	char	*variable=commandArgument(args);
	if (!variable) {
		return usageError(env,
				"usage: fetchfrombindcursor [variable]");
	}

	// the cursor belongs to this method from here on
	sqlrcursor	*bindcur=sqlrcur->getOutputBindCursor(variable);
	delete[] variable;
	if (!bindcur) {
		displayError(env,NULL,
			"fetchfrombindcursor needs a cursor output bind "
			"of the query that just ran",0);
		return false;
	}

	bindcur->setResultSetBufferSize(env->rsbs);
	if (env->lazyfetch) {
		bindcur->lazyFetch();
	} else {
		bindcur->dontLazyFetch();
	}

	bool	success=bindcur->fetchFromBindCursor();
	if (success) {
		// the rows are in hand by the time it returns
		displayCurrentResultSet(bindcur,env);
	} else {
		const char	*error=bindcur->errorMessage();
		if (charstring::isNullOrEmpty(error)) {
			error="Couldn't fetch from the bind cursor.";
		}
		displayError(env,NULL,error,bindcur->errorNumber());
	}

	delete bindcur;

	return success;
}

bool sqlrsh::validatebinds(sqlrshenv *env, const char *args) {

	// no argument at all means on
	bool	on=true;
	char	*arg=commandArgument(args);
	if (arg) {
		if (!charstring::compareIgnoringCase(arg,"off")) {
			on=false;
		} else if (charstring::compareIgnoringCase(arg,"on")) {
			delete[] arg;
			return usageError(env,"usage: validatebinds [on|off]");
		}
	}
	delete[] arg;

	// prepareQuery() clears the flag on the cursor, so this is kept here
	// and put back on for every query, the way the binds are
	env->validatebinds=on;

	return true;
}

bool sqlrsh::validbind(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *args) {

	char	*variable=commandArgument(args);
	if (!variable) {
		return usageError(env,"usage: validbind [variable]");
	}

	writeScalarBoolean(env,"validbind",sqlrcur->validBind(variable));

	delete[] variable;

	return true;
}

bool sqlrsh::substitution(sqlrshenv *env, const char *args) {

	static const char	*usage=
			"usage: substitution [variable] = [value]";

	char	*arg=commandArgument(args);
	if (!arg) {
		return usageError(env,usage);
	}

	// split the variable from the value
	char	*space=charstring::findFirst(arg,' ');
	if (!space || space[1]!='=' || space[2]!=' ') {
		delete[] arg;
		return usageError(env,usage);
	}
	*space='\0';

	char	*value=charstring::duplicate(space+3);
	charstring::bothTrim(value);
	size_t	valuelen=charstring::getLength(value);

	// the name comes out of the pool
	char	*name=poolCopy(&env->subpool,arg,charstring::getLength(arg));
	delete[] arg;

	// if the substitution variable is already defined, clear it...
	// Only the value is freed - what it points at belongs to the pool.
	sqlrshbindvalue	*bv=NULL;
	if (env->substitutions.getValue(name,&bv)) {
		delete bv;
	}

	// define the variable
	bv=new sqlrshbindvalue;

	// There is no date or null substitution - the api has no call for
	// either.
	if (valuelen>=2 &&
		((value[0]=='\'' && value[valuelen-1]=='\'') ||
		(value[0]=='"' && value[valuelen-1]=='"'))) {

		char		*unescaped=NULL;
		uint64_t	unescapedlen=0;
		charstring::unescape(value+1,valuelen-2,
					&unescaped,&unescapedlen);
		bv->type=SQLRCLIENTBINDVARTYPE_STRING;
		bv->stringval.value=poolCopy(&env->subpool,
					unescaped,(size_t)unescapedlen);
		bv->stringval.length=(uint32_t)unescapedlen;
		delete[] unescaped;

	} else if (charstring::isInteger(value)) {
		bv->type=SQLRCLIENTBINDVARTYPE_INTEGER;
		bv->integerval=charstring::convertToInteger(value);
	} else if (charstring::isNumber(value)) {
		bv->type=SQLRCLIENTBINDVARTYPE_DOUBLE;
		bv->doubleval.value=charstring::convertToFloatC(value);
		bv->doubleval.precision=valuelen-((value[0]=='-')?2:1);
		bv->doubleval.scale=
			charstring::findFirst(value,'.')-value+
			((value[0]=='-')?0:1);
	} else {
		bv->type=SQLRCLIENTBINDVARTYPE_STRING;
		bv->stringval.value=poolCopy(&env->subpool,value,valuelen);
		bv->stringval.length=(uint32_t)valuelen;
	}

	delete[] value;

	// put the substitution variable in the list
	env->substitutions.setValue(name,bv);

	return true;
}

bool sqlrsh::filequery(sqlrconnection *sqlrcon, sqlrcursor *sqlrcur,
			sqlrshenv *env, const char *args, bool execute) {

	const char	*usage=(execute)?
			"usage: filequery [path] [filename]\n"
			"       filequery [filename]":
			"usage: preparefilequery [path] [filename]\n"
			"       preparefilequery [filename]";

	char	*arg=commandArgument(args);
	if (!arg) {
		return usageError(env,usage);
	}

	// the path is optional, so one argument is a file name on its own
	char		**parts;
	uint64_t	partcount;
	charstring::split(arg," ",true,&parts,&partcount);
	delete[] arg;

	bool	prepared=false;
	if (partcount==1) {
		prepared=sqlrcur->prepareFileQuery(NULL,parts[0]);
	} else if (partcount==2) {
		prepared=sqlrcur->prepareFileQuery(parts[0],parts[1]);
	}

	if (!prepared) {
		if (partcount==1 || partcount==2) {
			displayError(env,NULL,
					sqlrcur->errorMessage(),
					sqlrcur->errorNumber());
		} else {
			usageError(env,usage);
		}
	}

	for (uint64_t i=0; i<partcount; i++) {
		delete[] parts[i];
	}
	delete[] parts;

	// preparefilequery stops here
	if (!prepared || !execute) {
		return prepared;
	}

	executeQuery(sqlrcur,env);

	if (sqlrcur->errorMessage()) {
		displayError(env,NULL,
				sqlrcur->errorMessage(),
				sqlrcur->errorNumber());
		return false;
	}

	displayCurrentResultSet(sqlrcur,env);

	if (env->final) {
		sqlrcon->endSession();
	}

	return true;
}

void sqlrsh::setclientinfo(sqlrconnection *sqlrcon, const char *command) {
	sqlrcon->setClientInfo(command+14);
}

void sqlrsh::getclientinfo(sqlrconnection *sqlrcon, sqlrshenv *env) {
	writeScalar(env,"getclientinfo",sqlrcon->getClientInfo());
}

void sqlrsh::delimiter(sqlrshenv *env) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.printf("Delimiter set to %c\n",
							env->delimiter);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			{
				char	value[2];
				value[0]=env->delimiter;
				value[1]='\0';
				writeScalar(env,"delimiter",value);
			}
			break;
	}
}

void sqlrsh::autocommit(sqlrshenv *env, bool on) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.write((on)?"Autocommit set on\n":
						"Autocommit set off\n");
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			writeScalarBoolean(env,"autocommit",on);
			break;
	}
}

static bool parseTimeout(const char *args, uint32_t *sec, uint32_t *usec) {

	// skip to the timeout itself
	while (character::isWhitespace(*args)) {
		args++;
	}

	// get seconds
	// (the api takes these as an int32_t and treats a negative as no
	// timeout at all, so a value past its max is refused rather than
	// wrapped around into one)
	if (!character::isDigit(*args)) {
		return false;
	}
	*sec=0;
	while (character::isDigit(*args)) {
		if (*sec>214748364U) {
			return false;
		}
		*sec=(*sec)*10+(uint32_t)(*args-'0');
		if (*sec>2147483647U) {
			return false;
		}
		args++;
	}

	// get the fraction of a second, as microseconds
	// (a decimal fraction, the way the c++ api reads the
	// SQLR_CLIENT_*_TIMEOUT environment variables)
	*usec=0;
	if (*args=='.') {
		args++;
		if (!character::isDigit(*args)) {
			return false;
		}
		uint32_t	scale=100000;
		bool		rounded=false;
		while (character::isDigit(*args)) {
			if (scale) {
				*usec=(*usec)+
					(uint32_t)(*args-'0')*scale;
				scale=scale/10;
			} else if (!rounded) {
				// a seventh digit rounds the sixth rather
				// than being dropped, and any digit past
				// that can't change the result
				if (*args>='5') {
					(*usec)++;
				}
				rounded=true;
			}
			args++;
		}

		// rounding .9999995 or more up carries into the seconds
		if (*usec>999999U) {
			*usec=0;
			(*sec)++;
			if (*sec>2147483647U) {
				return false;
			}
		}
	}

	// nothing but trailing whitespace may follow
	while (character::isWhitespace(*args)) {
		args++;
	}
	return !(*args);
}

bool sqlrsh::connectTimeout(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	uint32_t	sec;
	uint32_t	usec;
	if (!parseTimeout(args,&sec,&usec)) {
		displayError(env,NULL,
			"connect timeout needs a whole number of seconds, "
			"optionally followed by a dot and a decimal "
			"fraction of a second",0);
		return false;
	}

	// This applies to the next connect, not to the session sqlrsh is
	// already in.
	sqlrcon->setConnectTimeout((int32_t)sec,(int32_t)usec);

	writeTimeoutSet(env,"connecttimeout","Connect Timeout",sec,usec);
	return true;
}

bool sqlrsh::responseTimeout(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	uint32_t	sec;
	uint32_t	usec;
	if (!parseTimeout(args,&sec,&usec)) {
		displayError(env,NULL,
			"response timeout needs a whole number of seconds, "
			"optionally followed by a dot and a decimal "
			"fraction of a second",0);
		return false;
	}

	sqlrcon->setResponseTimeout((int32_t)sec,(int32_t)usec);

	writeTimeoutSet(env,"responsetimeout","Response Timeout",sec,usec);
	return true;
}

bool sqlrsh::cache(sqlrshenv *env, sqlrcursor *sqlrcur, const char *command) {

	// move to file name
	const char	*ptr=command+6;

	// skip whitespace
	while (*ptr==' ') {
		ptr++;
	}

	// bail if no file name was given
	if (!*ptr) {
		stderror.printf("	No file name given\n\n");
		return false;
	}

	// build filename
	stringbuffer	fn;
	fn.append(sqlrpth->getCacheDir());
	bool	inquotes=false;
	while (*ptr) {
		if (*ptr=='"') {
			inquotes=!inquotes;
		}
		if (*ptr==' ' && !inquotes) {
			break;
		}
		fn.append(*ptr);
		ptr++;
	}
	delete[] env->cacheto;
	env->cacheto=fn.detachString();

	// find ttl
	while (*ptr==' ') {
		ptr++;
	}
	uint32_t	cachettl=600;
	if (*ptr) {
		cachettl=charstring::convertToInteger(ptr);
	}

	// the banner is plain format only
	if (env->format==SQLRSH_FORMAT_PLAIN) {
		stdoutput.printf("	Caching To       : %s\n",env->cacheto);
		stdoutput.printf("	Cache TTL Set To : %lld seconds\n\n",
							(long long)cachettl);
	}

	// begin caching
	sqlrcur->cacheToFile(env->cacheto);
	sqlrcur->setCacheTtl(cachettl);

	return true;
}

bool sqlrsh::openCache(sqlrshenv *env,
			sqlrcursor *sqlrcur, const char *command) {

	// move to file name
	command=command+10;

	// skip whitespace
	while (*command==' ') {
		command++;
	}

	// bail if no file name was given
	if (!*command) {
		stderror.printf("	No file name given\n\n");
		return false;
	}

	// if the file name starts with a slash then use it as-is, otherwise
	// prepend the default cache directory.
	stringbuffer	fn;
	fn.append(sqlrpth->getCacheDir())->append(command);

	// open the cached result set
	if (!sqlrcur->openCachedResultSet(fn.getString())) {
		stderror.printf("	Cannot open cache file\n\n");
		return false;
	}

	// display the header
	displayHeader(sqlrcur,env);

	// display the result set
	displayResultSet(sqlrcur,env);

	// display statistics
	displayStats(sqlrcur,env);

	return true;
}

void sqlrsh::displayHelp(sqlrshenv *env) {

	// The text is written rather than printf'd.  It contains like
	// patterns such as 'a%' that printf would read as conversions.
	stdoutput.write(
"\n"
"  To run a query, type it at the prompt, followed by the delimiter.  A\n"
"  query may be split over as many lines as it takes.  Everything below is\n"
"  a command, and ends with the delimiter too.\n"
"\n"
"  Run \"sqlrsh -help\" for the command line options, the output formats,\n"
"  and the exit codes.\n"
"\n"
"  Queries, scripts and files:\n"
"\n"
"    reexecute               runs the previous query again\n"
"    filequery [path] [file]\n"
"    filequery [file]        runs the whole file as one query\n"
"    preparefilequery [path] [file]\n"
"    preparefilequery [file]\n"
"                            prepares the whole file as one query, but\n"
"                            does not run it.  reexecute runs it\n"
"    run [file]              runs the sqlrsh commands in the file\n"
"    @ [file]                same as run\n"
"    continueonerror on|off  carries on past a statement that failed,\n"
"                            rather than stopping at the first one\n"
"    help                    this text\n"
"    exit                    exits\n"
"    quit                    same as exit\n"
"\n"
"  Output settings:\n"
"\n"
"    format plain|csv|json|jsonl\n"
"                            sets the output format\n"
"    headers on|off          the column names above the result set\n"
"                            (plain format only)\n"
"    divider on|off          the line of = signs under the column names\n"
"                            (plain format only)\n"
"    stats on|off            the statistics below the result set\n"
"    quiet on|off            shorthand - quiet on turns headers and stats\n"
"                            off, quiet off turns them back on\n"
"    noelapsed on|off        leaves the elapsed time out of the stats\n"
"    fieldsas raw|number|boolean|date\n"
"                            fetches fields with the matching getFieldAs\n"
"                            method, where the column suits it.  raw is\n"
"                            the field as the database sent it\n"
"    getasnumber on|off      an alias - on is fieldsas number, off is\n"
"                            fieldsas raw\n"
"    nullsasnulls on|off     gets nulls as nulls, rather than as empty\n"
"                            strings\n"
"    delimiter [character]   sets the delimiter, and echoes it back.\n"
"                            delimeter is accepted as a spelling of it\n"
"    debug on|off            client library debug messages, to stdout\n"
"    debug [file]            client library debug messages, to the file\n"
"    getdebug                whether debug is on\n"
"\n"
"  Result sets:\n"
"\n"
"    setresultsetbuffersize [rows]\n"
"                            fetches rows at a time, rather than all at\n"
"                            once.  0 means all at once.  100 to start\n"
"                            with\n"
"    getresultsetbuffersize  the number of rows fetched at a time\n"
"    lazyfetch on|off        fetches rows as they are asked for, rather\n"
"                            than when the query runs\n"
"    nextresultset on|off    fetches every result set a query returns,\n"
"                            rather than just the first\n"
"    columninfo              all of the column metadata of the current\n"
"                            result set\n"
"    columninfo on|off       whether column metadata is fetched at all.\n"
"                            off leaves names and types empty\n"
"    columncase mixed|upper|lower\n"
"                            the case of the column names\n"
"    totalrows               the total row count, if the database gives\n"
"                            one, and 0 if it does not\n"
"    firstrowindex           the index of the first buffered row\n"
"    endofresultset          whether the whole result set has arrived\n"
"    suspendresultset        leaves the result set open for another\n"
"                            session to resume\n"
"    resultsetid             the id of the suspended result set\n"
"    resumeresultset [id]    resumes the result set, and writes the rest\n"
"                            of it.  No id means this cursor's own\n"
"    closeresultset          closes the result set\n"
"    querytree               the parsed query, as xml\n"
"    translatedquery         the query, after translation\n"
"    lastinsertid            the value of the most recently updated\n"
"                            auto-increment or identity column, if the\n"
"                            database has one\n"
"\n"
"  Caching:\n"
"\n"
"    cache [file] [ttl]      caches the next result set to the file, with\n"
"                            a time-to-live of ttl seconds.  600 if left\n"
"                            out\n"
"    cacheoff                stops caching\n"
"    cachefilename           the file the result set is being cached to\n"
"    opencache [file]        opens a cached result set and writes it\n"
"    resumecachedresultset [id] [file]\n"
"                            resumes a suspended result set and keeps\n"
"                            caching it to the file.  A resume appends,\n"
"                            so the name should be the one the original\n"
"                            session was already caching to\n"
"\n"
"    A cache file name is taken relative to the cache directory.\n"
"\n"
"  Bind variables:\n"
"\n"
"    inputbind [variable] = [value]\n"
"    inputbind [variable] is null\n"
"    inputbind [variable] string [length] = [value]\n"
"                            defines an input bind variable.  A quoted\n"
"                            value is a string, an unquoted one is a\n"
"                            number if it looks like one, and a date if\n"
"                            it is MM/DD/YYYY HH24:MI:SS:uS TZN.  The\n"
"                            string form binds exactly length bytes,\n"
"                            cutting off or null-padding to fit\n"
"    inputbindblob [variable] = [value]\n"
"    inputbindblob [variable] is null\n"
"    inputbindclob [variable] = [value]\n"
"    inputbindclob [variable] is null\n"
"                            defines a blob or clob input bind variable\n"
"    outputbind [variable] string [length]\n"
"    outputbind [variable] integer\n"
"    outputbind [variable] double [precision] [scale]\n"
"    outputbind [variable] date\n"
"    outputbind [variable] blob\n"
"    outputbind [variable] clob\n"
"    outputbind [variable] cursor\n"
"                            defines an output bind variable\n"
"    inputoutputbind [variable] string [length] = [value]\n"
"    inputoutputbind [variable] integer = [value]\n"
"    inputoutputbind [variable] double [precision] [scale] = [value]\n"
"    inputoutputbind [variable] date = [value]\n"
"    inputoutputbind [variable] [type] ... is null\n"
"                            defines an input/output bind variable\n"
"    fetchfrombindcursor [variable]\n"
"                            writes the result set of a cursor output\n"
"                            bind of the query that just ran\n"
"    printbinds              all three bind variable lists\n"
"    printinputbind          the input bind variable list\n"
"    printoutputbind         the output bind variable list\n"
"    printinputoutputbind    the input/output bind variable list\n"
"    clearinputbind [variable]\n"
"    clearoutputbind [variable]\n"
"    clearinputoutputbind [variable]\n"
"                            clears one bind variable, or the whole list\n"
"                            when no variable is given.  A variable that\n"
"                            is not in the list is an error\n"
"    clearbinds              clears all three lists\n"
"    countbindvariables      the number of bind variables in the query\n"
"                            that was prepared last\n"
"    validatebinds on|off    ignores bind variables the query does not\n"
"                            have, rather than failing.  No value means\n"
"                            on\n"
"    validbind [variable]    whether the variable really was a bind\n"
"                            variable of the query that just ran\n"
"    bindformat              the bind variable format of the database\n"
"    bindvariabledelimiters  the bind variable delimiters in use\n"
"    bindvariabledelimiters [delimiters]\n"
"                            sets them, from the set ? : @ $\n"
"    bindvariabledelimitersupported [delimiter]\n"
"                            whether one of ? : @ $ is supported\n"
"\n"
"  Substitutions:\n"
"\n"
"    substitution [variable] = [value]\n"
"                            replaces $(variable) in the query text when\n"
"                            the query is prepared.  A substitution is\n"
"                            not a bind variable\n"
"    clearsubstitutions      clears the substitution list\n"
"\n"
"  Session and database information:\n"
"\n"
"    ping                    pings the database\n"
"    identify                the type of database\n"
"    dbversion               the version of the database\n"
"    dbhostname              the host name of the database\n"
"    dbipaddress             the ip address of the database\n"
"    clientversion           the version of the client library\n"
"    serverversion           the version of the server\n"
"    databasefeature [feature]\n"
"                            whether the database has the feature\n"
"    nextvalformat           the sequence next-value format of the\n"
"                            database\n"
"    setclientinfo [info]    sets the client info string\n"
"    getclientinfo           the client info string\n"
"    endsession              ends the session.  The next command starts a\n"
"                            new one\n"
"    final on|off            one session per query\n"
"    suspendsession          leaves the session open for another client\n"
"                            to resume\n"
"    connectionport          the port of the suspended session\n"
"    connectionsocket        the unix socket of the suspended session\n"
"    resumesession [port]\n"
"    resumesession [port] [socket]\n"
"                            resumes a suspended session\n"
"    connect timeout [seconds[.fraction]]\n"
"                            the timeout for the next connect, which is\n"
"                            what a resumesession or a reconnect after an\n"
"                            endsession uses\n"
"    getconnecttimeout       the connect timeout, or -1 when it is off\n"
"    response timeout [seconds[.fraction]]\n"
"                            the timeout for a response from the server\n"
"    getresponsetimeout      the response timeout, or -1 when it is off\n"
"\n"
"    A timeout is decimal seconds, so 5.25 sets five and a quarter\n"
"    seconds and 0.5 sets half a second.  Leave the dot out for a whole\n"
"    number of seconds.  The smallest unit is the microsecond, so a\n"
"    seventh digit past the dot only rounds the sixth.  The get commands\n"
"    report a timeout in the same form, padded to 6 places, so what they\n"
"    print can be given straight back to the setter unchanged.  The\n"
"    SQLR_CLIENT_CONNECT_TIMEOUT and SQLR_CLIENT_RESPONSE_TIMEOUT\n"
"    environment variables use this same form.\n"
"\n"
"  Databases, catalogs and schemas:\n"
"\n"
"    use [database]          changes the current database or schema\n"
"    currentdb               the current database or schema\n"
"    usecatalog [catalog]    changes the current catalog\n"
"    currentcatalog          the current catalog\n"
"    useschema [schema]      changes the current schema\n"
"    currentschema           the current schema\n"
"    currentuser             the current user\n"
"    databaseisschema        whether the database is what other databases\n"
"                            would call a schema\n"
"\n"
"  Transactions:\n"
"\n"
"    autocommit on|off       commits after every query\n"
"    getautocommit           whether autocommit is on\n"
"    begin                   begins a transaction\n"
"    commit                  commits\n"
"    rollback                rolls back\n"
"    txqueries on|off        sends begin, commit and rollback to the\n"
"                            database as queries, rather than running\n"
"                            them as commands\n"
"    intransaction           whether a transaction is open\n"
"    isolationlevel          the current isolation level\n"
"    isolationlevel [level]  sets it\n"
"    defaultisolationlevel   the isolation level the session starts with\n"
"    transactionmodel        the current transaction model\n"
"    transactionmodel [model]\n"
"                            sets it\n"
"    defaulttransactionmodel the transaction model the session starts\n"
"                            with\n"
"\n"
"  Metadata:\n"
"\n"
"    show databases [like 'pattern']\n"
"    show catalogs [like 'pattern']\n"
"    show schemas [like 'pattern']\n"
"    show tables [like 'pattern']\n"
"    show table types\n"
"    show columns in [table] [like 'pattern']\n"
"    describe [table]\n"
"    fields [table]\n"
"    show primary keys in [table] [like 'pattern']\n"
"    show keys and indexes in [table] [like 'pattern']\n"
"    show procedures [like 'pattern']\n"
"    show procedure parameters in [procedure] [like 'pattern']\n"
"    show type info [for type]\n"
"    show lastinsertid\n"
"    show only tables [like 'pattern']\n"
"    show only views [like 'pattern']\n"
"    show only aliases [like 'pattern']\n"
"    show only synonyms [like 'pattern']\n"
"\n"
"    describe writes the same column metadata that show columns does.\n"
"    fields writes just the column names, on one line.\n"
"\n"
"    A pattern goes in single quotes, and a single quote inside one is\n"
"    doubled.  The wildcards are the database's own, usually % and _.\n"
"\n"
"    databases, catalogs, schemas, tables, table types, columns,\n"
"    procedures, type info and the show only forms each take an\n"
"    optional list format, right after the category:\n"
"\n"
"      show tables mysql\n"
"      show tables odbc like 'a%'\n"
"      show columns jdbc in mytable\n"
"      show only views odbc\n"
"\n"
"    The list format decides the columns of the result set: mysql gives\n"
"    what a MySQL client would see, odbc what an ODBC client would see,\n"
"    and jdbc what a JDBC client would see.  Left out, the columns are\n"
"    whatever the database itself returns.\n"
"\n"
"  Notes:\n"
"\n"
"    An on|off command with no value means off, so \"headers;\" turns\n"
"    headers off.  Any value that does not begin with \"on\" also means\n"
"    off, and nothing is said about it.\n"
"\n"
"    A command that sets something writes nothing when it works.\n"
"\n"
"    A command that fails writes to stderr and, in a script or a\n"
"    -command list, stops the run and exits 4.\n"
"\n");
	stdoutput.printf(
"    All commands must be followed by the delimiter: %c\n\n",
								env->delimiter);
}

void sqlrsh::startupMessage(sqlrshenv *env, const char *host,
					uint16_t port, const char *user) {

	// no banner in batch mode - it would land in the piped-out data
	if (env->batch) {
		return;
	}

	// no banner for json or jsonl - a greeting isn't part of either
	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.printf("%ssh - ",SQLR);
			stdoutput.printf("Version %s\n",SQLR_VERSION);
			stdoutput.printf("	Connected to: ");
			stdoutput.printf("%s:%d as %s\n\n",host,port,user);
			stdoutput.printf("	type help; for help.\n\n");
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			break;
	}
}

void sqlrsh::interactWithUser(sqlrconnection *sqlrcon, sqlrcursor *sqlrcur, 
							sqlrshenv *env) {

	// init some variables
	stringbuffer	command;
	stringbuffer	prmpt;
	bool		exitprogram=false;
	uint32_t	promptcount;

	// Blocking mode is apparently not the default on some systems
	// (Syllable for sure, maybe others) and this causes hilariously
	// odd behavior when reading standard input.
	stdinput.setNonBlockingMode(false);

	while (!exitprogram) {

		// prompt the user
		promptcount=0;
		
		// get the command
		bool	done=false;
		while (!done) {

			// The prompt is written to the same stream the
			// output goes to, so json and jsonl don't get one,
			// and neither does batch mode, for the same reason.
			if (!env->batch) {
				switch (env->format) {
					case SQLRSH_FORMAT_PLAIN:
					case SQLRSH_FORMAT_CSV:
						prmpt.append(promptcount);
						prmpt.append("> ");
						break;
					case SQLRSH_FORMAT_JSON:
					case SQLRSH_FORMAT_JSONL:
						break;
				}
			}
			// an empty stringbuffer can hand back a NULL
			const char	*promptstring=prmpt.getString();
			pr.setPrompt((promptstring)?promptstring:"");
			prmpt.clear();

			char	*cmd=pr.read();

			// cmd is NULL if you hit ctrl-D
			if (!cmd) {
				return;
			}

			size_t	len=charstring::getLength(cmd);

			// len=0 and cmd="" if you just hit return
			if (len) {
				command.append(cmd);
				done=(cmd[len-1]==env->delimiter);
			}

			if (!done) {
				promptcount++;
				command.append('\n');
			}
		}

		char	*cmd=command.detachString();

		// run the command
		runCommands(sqlrcon,sqlrcur,env,cmd,&exitprogram);

		// clean up
		delete[] cmd;
	}
}

bool sqlrsh::onOffOption(const char *arg, bool defaultvalue) {

	if (!cmdline->isFound(arg)) {
		return defaultvalue;
	}

	// getValue() returns an empty string for an option with no value,
	// for an option followed by another option, and for an option that
	// isn't there at all, so isFound() above is what decides presence
	const char	*value=cmdline->getValue(arg);
	if (charstring::isNullOrEmpty(value)) {
		return true;
	}

	// same test the on/off commands use
	return !charstring::compareIgnoringCase(value,"on",2);
}

// the options that require a value, beyond the connection options that
// sqlrcmdline already knows about
// (the on/off options aren't here, and neither are -krb, -tls and -debug,
// because a bare one of those is legal and means on)
static const char * const	valueoptions[]={
	"bindvariabledelimiters",
	"script",
	"command",
	"query",
	"delimiter",
	"delimeter",
	"format",
	"fieldsas",
	"resultsetbuffersize",
	"locale",
	NULL
};

int32_t sqlrsh::execute(int argc, const char **argv) {

	cmdline=new sqlrcmdline(argc,argv);

	// an option that requires a value has to have one
	// (getValue() hands back an empty string for one that doesn't, which
	// would mean 0 for the numeric options)
	const char	*mvo=cmdline->missingValueOption(valueoptions);
	if (mvo) {
		stderror.printf("usage: -%s requires a value.  "
				"Use --%s=value if the value "
				"begins with a dash.\n",mvo,mvo);
		return SQLRSH_EXIT_USAGE;
	}

	sqlrpth=new sqlrpaths(cmdline);
	sqlrconfigs	sqlrcfgs(sqlrpth);
	sqlrconfig	*cfg=NULL;

	// get command-line options
	const char	*configurl=sqlrpth->getConfigUrl();
	const char	*id=cmdline->getValue("id");
	const char	*host=cmdline->getValue("host");
	uint16_t	port=charstring::convertToInteger(
				(cmdline->isFound("port"))?
				cmdline->getValue("port"):DEFAULT_PORT);
	const char	*socket=cmdline->getValue("socket");
	const char	*user=cmdline->getValue("user");
	const char	*password=cmdline->getValue("password");
	const char	*passwordencryptionid=NULL;
	char		*defaultpassword=NULL;
	char		*decryptedpassword=NULL;
	bool		usekrb=cmdline->isFound("krb");
	const char	*krbservice=cmdline->getValue("krbservice");
	const char	*krbmech=cmdline->getValue("krbmech");
	const char	*krbflags=cmdline->getValue("krbflags");
	bool		usetls=cmdline->isFound("tls");
	const char	*tlsversion=cmdline->getValue("tlsversion");
	const char	*tlscert=cmdline->getValue("tlscert");
	const char	*tlspassword=cmdline->getValue("tlspassword");
	const char	*tlsciphers=cmdline->getValue("tlsciphers");
	const char	*tlsvalidate="no";
	if (cmdline->isFound("tlsvalidate")) {
		tlsvalidate=cmdline->getValue("tlsvalidate");
	}
	const char	*tlsca=cmdline->getValue("tlsca");
	uint16_t	tlsdepth=charstring::convertToUnsignedInteger(
					cmdline->getValue("tlsdepth"));
	const char	*localeargument=cmdline->getValue("locale");
	const char	*script=cmdline->getValue("script");
	const char	*command=cmdline->getValue("command");
	const char	*query=cmdline->getValue("query");

	// at least id, host, or socket is required
	if (charstring::isNullOrEmpty(id) &&
		charstring::isNullOrEmpty(host) &&
		charstring::isNullOrEmpty(socket)) {

		stderror.printf("usage:\n"
			" %ssh -host host [-port port]\n"
			"        [-user user] [-password password]\n"
			"        [-krb] [-krbservice svc] [-krbmech mech] "
			"[-krbflags flags]\n"
			"        [-tls] [-tlsversion version]\n"
			"        [-tlscert certfile] [-tlspassword password]\n"
			"        [-tlsciphers cipherlist]\n"
			"        [-tlsvalidate (no|ca|ca+domain|ca+host)] "
			"[-tlsca ca] [-tlsdepth depth]\n"
			"        [options]\n"
			"  or\n"
			" %ssh -socket socket [-user user] "
			"[-password password]\n"
			"        [options]\n"
			"  or\n"
			" %ssh [-config config] -id id [options]\n"
			"\n"
			" options:\n"
			"        [-script script | -command commands | "
			"-query query]\n"
			"        [-batch] [-agent] "
			"[-format (plain|csv|json|jsonl)]\n"
			"        [-quiet (on|off)] [-headers (on|off)] "
			"[-divider (on|off)]\n"
			"        [-stats (on|off)] [-noelapsed (on|off)]\n"
			"        [-fieldsas (raw|number|boolean|date)]\n"
			"        [-getasnumber (on|off)] "
			"[-nextresultset (on|off)]\n"
			"        [-lazyfetch (on|off)] [-txqueries (on|off)]\n"
			"        [-continueonerror (on|off)] "
			"[-nullsasnulls (on|off)]\n"
			"        [-final (on|off)] [-autocommit (on|off)]\n"
			"        [-validatebinds (on|off)]\n"
			"        [-bindvariabledelimiters delimiters]\n"
			"        [-debug (on|off|file)]\n"
			"        [-resultsetbuffersize rows] "
			"[-delimiter char]\n"
			"        [-locale (env|name)] [-localstatedir dir]\n"
			"\n"
			" exit codes:\n"
			"        0  success\n"
			"        1  usage error\n"
			"        2  the -script file could not be opened\n"
			"        3  connection or authentication failure\n"
			"        4  at least one statement failed\n"
			"\n"
			" Run \"%ssh -help\" for the options in full, and "
			"\"help;\" at the\n"
			" %ssh prompt for the commands.\n",
			SQLR,SQLR,SQLR,SQLR,SQLR);
		return SQLRSH_EXIT_USAGE;
	}

	// if an id was specified, then get various values from the config file
	if (!charstring::isNullOrEmpty(id)) {
		cfg=sqlrcfgs.load(configurl,id);
		if (cfg) {
			if (!cmdline->isFound("host")) {
				host="localhost";
			}
			if (!cmdline->isFound("port")) {
				port=cfg->getDefaultPort();
			}
			if (!cmdline->isFound("socket")) {
				socket=cfg->getDefaultSocket();
			}
			if (!cmdline->isFound("krb")) {
				usekrb=cfg->getDefaultKrb();
			}
			if (!cmdline->isFound("krbservice")) {
				krbservice=cfg->getDefaultKrbService();
			}
			if (!cmdline->isFound("krbmech")) {
				krbmech=cfg->getDefaultKrbMech();
			}
			if (!cmdline->isFound("krbflags")) {
				krbflags=cfg->getDefaultKrbFlags();
			}
			if (!cmdline->isFound("tls")) {
				usetls=cfg->getDefaultTls();
			}
			if (!cmdline->isFound("tlsciphers")) {
				tlsciphers=cfg->getDefaultTlsCiphers();
			}
			if (!cmdline->isFound("user")) {

				// get the default user
				user=cfg->getDefaultUser();

				// get the default password and
				// de-sensitiveize it if necessary
				sensitivevalue	passwordvalue;
				passwordvalue.setPath(cfg->getPasswordPath());
				passwordvalue.parse(cfg->getDefaultPassword());
				defaultpassword=
					passwordvalue.detachTextValue();
				password=defaultpassword;

				// decrypt the password if necessary
				passwordencryptionid=
					cfg->getDefaultPasswordEncryptionId();
				if (passwordencryptionid) {

					// get the module
					sqlrpwdencs	sqlrpe(
						sqlrpth,false,
						cfg->getPasswordEncryptions());
					sqlrpe.load();
					sqlrpwdenc	*pe=
						sqlrpe.
						getPasswordEncryptionById(
							passwordencryptionid);
					if (!pe) {
						stderror.printf(
							"password encryption "
							"id %s not found\n",
							passwordencryptionid);
						delete[] defaultpassword;
						delete[] decryptedpassword;
						return SQLRSH_EXIT_USAGE;
					}

					// a one-way encryption can encrypt
					// the password but not decrypt it
					if (pe->oneWay()) {
						stderror.printf(
							"password encryption "
							"%s is one-way, so the "
							"password in the "
							"configuration file "
							"cannot be decrypted."
							"\nUse -user and "
							"-password along with "
							"-id.\n",
							passwordencryptionid);
						delete[] defaultpassword;
						delete[] decryptedpassword;
						return SQLRSH_EXIT_USAGE;
					}

					// decrypt the password
					decryptedpassword=
						pe->decrypt(password);
					password=decryptedpassword;
				}
			}
		}
	}

	if (!charstring::isNullOrEmpty(localeargument)) {
		// This is useful for making sure that decimals still work
		// when the locale is changed to say, de_DE that has different
		// number formats.
		if (!locale::setAll(
			(!charstring::compare(localeargument,"env"))?
							"":localeargument)) {
			stderror.printf("ERROR: set locale failed\n");
			delete[] defaultpassword;
			delete[] decryptedpassword;
			return SQLRSH_EXIT_USAGE;
		}
	}

	// configure sql relay connection
	sqlrconnection	sqlrcon(host,port,socket,user,password,0,1);
	sqlrcursor	sqlrcur(&sqlrcon);

	// configure kerberos/tls
	if (usekrb) {
		sqlrcon.enableKerberos(krbservice,krbmech,krbflags);
	} else if (usetls) {
		sqlrcon.enableTls(tlsversion,tlscert,tlspassword,tlsciphers,
						tlsvalidate,tlsca,tlsdepth);
	}

	// handle debug
	// (runs before the connection is checked below, so a connection that
	// fails can be debugged too)
	if (cmdline->isFound("debug")) {
		const char	*debugvalue=cmdline->getValue("debug");
		if (charstring::isNullOrEmpty(debugvalue) ||
			!charstring::compareIgnoringCase(debugvalue,"on",2)) {
			sqlrcon.debugOn();
			sqlrcon.setDebugFile(NULL);
		} else if (!charstring::compareIgnoringCase(
						debugvalue,"off",3)) {
			sqlrcon.debugOff();
			sqlrcon.setDebugFile(NULL);
		} else {
			sqlrcon.debugOn();
			sqlrcon.setDebugFile(debugvalue);
		}
	}

	// handle the bind variable delimiters
	if (cmdline->isFound("bindvariabledelimiters")) {
		sqlrcon.setBindVariableDelimiters(
			cmdline->getValue("bindvariabledelimiters"));
	}

	// set up an sqlrshenv
	sqlrshenv	env;

	// -agent is shorthand for the settings a script or agent wants:
	// quiet, format jsonl, nullsasnulls on and continueonerror on.  It
	// runs first, so any of those given explicitly still win, the same
	// way -quiet's shorthand does relative to -headers and -stats below.
	bool	agentnullsasnulls=false;
	if (cmdline->isFound("agent")) {
		env.headers=false;
		env.stats=false;
		env.format=SQLRSH_FORMAT_JSONL;
		env.continueonerror=true;
		agentnullsasnulls=true;
	}

	// quiet runs before the others, so an explicit -headers or -stats
	// wins over the shorthand
	if (cmdline->isFound("quiet")) {
		bool	quiet=onOffOption("quiet",false);
		env.headers=!quiet;
		env.stats=!quiet;
	}

	// handle the result set format
	// An unrecognized name used to quietly fall back to plain.  It's a
	// usage error now.
	if (cmdline->isFound("format")) {
		const char	*formatname=cmdline->getValue("format");
		if (!formatFromName(formatname,&env.format)) {
			badFormatName(formatname);
			delete[] defaultpassword;
			delete[] decryptedpassword;
			return SQLRSH_EXIT_USAGE;
		}
	}

	// handle the getFieldAs...() mode
	// (-getasnumber is an alias for two of the modes, and runs first, so
	// the more specific -fieldsas wins when both are given)
	if (cmdline->isFound("getasnumber")) {
		env.fieldsas=(onOffOption("getasnumber",false))?
						SQLRSH_FIELDSAS_NUMBER:
						SQLRSH_FIELDSAS_RAW;
	}
	if (cmdline->isFound("fieldsas")) {
		const char	*fieldsasname=cmdline->getValue("fieldsas");
		if (!fieldsAsFromName(fieldsasname,&env.fieldsas)) {
			badFieldsAsName(fieldsasname);
			delete[] defaultpassword;
			delete[] decryptedpassword;
			return SQLRSH_EXIT_USAGE;
		}
	}

	// handle the result set buffer size
	if (cmdline->isFound("resultsetbuffersize")) {
		env.rsbs=charstring::convertToInteger(
				cmdline->getValue("resultsetbuffersize"));
	}

	// handle the on/off settings
	env.headers=onOffOption("headers",env.headers);
	env.divider=onOffOption("divider",env.divider);
	env.stats=onOffOption("stats",env.stats);
	env.lazyfetch=onOffOption("lazyfetch",env.lazyfetch);
	env.txqueries=onOffOption("txqueries",env.txqueries);
	env.noelapsed=onOffOption("noelapsed",env.noelapsed);
	env.nextresultset=onOffOption("nextresultset",env.nextresultset);
	env.continueonerror=onOffOption("continueonerror",env.continueonerror);
	env.final=onOffOption("final",env.final);
	env.validatebinds=onOffOption("validatebinds",env.validatebinds);

	// handle nullsasnulls
	// (the client library hands back empty strings unless it's told
	// otherwise, so only an explicit option, or -agent, changes it)
	if (cmdline->isFound("nullsasnulls")) {
		if (onOffOption("nullsasnulls",false)) {
			sqlrcur.getNullsAsNulls();
		} else {
			sqlrcur.getNullsAsEmptyStrings();
		}
	} else if (agentnullsasnulls) {
		sqlrcur.getNullsAsNulls();
	}

	// handle the delimiter
	// The delimeter misspelling is accepted as an option because the
	// command has always accepted it.
	const char	*delimiter=cmdline->getValue("delimiter");
	if (charstring::isNullOrEmpty(delimiter)) {
		delimiter=cmdline->getValue("delimeter");
	}
	if (!charstring::isNullOrEmpty(delimiter)) {
		env.delimiter=delimiter[0];
	}

	// check the connection
	// sqlrconnection doesn't connect until something needs the server, so
	// without this a bad host, port, or password would first show up as a
	// failed command.
	if (!sqlrcon.ping()) {
		const char	*error=sqlrcon.errorMessage();
		if (charstring::isNullOrEmpty(error)) {
			error="The database is down.";
		}
		displayError(&env,NULL,error,sqlrcon.errorNumber());
		delete[] defaultpassword;
		delete[] decryptedpassword;
		return SQLRSH_EXIT_CONNECTION;
	}

	// handle autocommit
	// (the server decides it otherwise, so only an explicit option
	// changes it, and no banner is written)
	if (cmdline->isFound("autocommit")) {
		env.autocommit=onOffOption("autocommit",env.autocommit);
		bool	success=(env.autocommit)?sqlrcon.autoCommitOn():
						sqlrcon.autoCommitOff();
		if (!success) {
			displayError(&env,NULL,sqlrcon.errorMessage(),
						sqlrcon.errorNumber());
			delete[] defaultpassword;
			delete[] decryptedpassword;
			return SQLRSH_EXIT_CONNECTION;
		}
	}

	// process RC files
	userRcFile(&sqlrcon,&sqlrcur,&env);


	// handle the history file
	const char	*home=environment::getValue("HOME");
	if (!charstring::isNullOrEmpty(home)) {
		char	*filename=new char[charstring::getLength(home)+16+1];
		charstring::copy(filename,home);
		charstring::append(filename,"/.sqlrsh_history");
		pr.setHistoryFile(filename);
		pr.setMaxHistoryLines(100);
		delete[] filename;
	}

	int32_t	exitcode=SQLRSH_EXIT_SUCCESS;

	if (!charstring::isNullOrEmpty(script)) {
		// if a script was specified, run it
		exitcode=runScript(&sqlrcon,&sqlrcur,&env,script,true);
		reportErrorCount(&env,&exitcode);
	} else if (!charstring::isNullOrEmpty(command)) {
		// if a command was specified, run it
		if (!runCommands(&sqlrcon,&sqlrcur,&env,command,NULL)) {
			exitcode=SQLRSH_EXIT_QUERY;
		}
		reportErrorCount(&env,&exitcode);
	} else if (!charstring::isNullOrEmpty(query)) {
		// if a query was specified, run it as a single statement
		// (no delimiter scan, so an embedded delimiter can't split it,
		// but a trailing one has to come off)
		char	*trimmedquery=charstring::duplicate(query);
		charstring::rightTrim(trimmedquery);
		size_t	querylen=charstring::getLength(trimmedquery);
		if (querylen && trimmedquery[querylen-1]==env.delimiter) {
			trimmedquery[querylen-1]='\0';
		}
		if (!runCommand(&sqlrcon,&sqlrcur,&env,trimmedquery,NULL)) {
			exitcode=SQLRSH_EXIT_QUERY;
		}
		delete[] trimmedquery;
	} else {
		// otherwise read commands from the prompt, same as always,
		// but a -batch run skips the banner and the prompts, since
		// they'd land in the piped-out data, and counts failures like
		// -script and -command do, since a run reading piped-in SQL
		// is a batch even though it's still reading a command at a
		// time from stdin (a genuinely interactive session isn't a
		// batch, so a failed query at the prompt still isn't a failed
		// run there)
		env.batch=cmdline->isFound("batch");
		startupMessage(&env,host,port,user);
		interactWithUser(&sqlrcon,&sqlrcur,&env);
		if (env.batch) {
			reportErrorCount(&env,&exitcode);
		}
	}

	// clean up
	pr.flushHistory();
	delete[] defaultpassword;
	delete[] decryptedpassword;

	return exitcode;
}

static void helpmessage(const char *progname) {
	stdoutput.printf(
		"%s is the %s command line database shell.\n"
		"\n"
		"It can be used interactively, or non-interactively to run queries directly from the command line, or scripts containing queries.\n"
		"\n"
		"Usage: %s [OPTIONS]\n"
		"\n"
		"Options:\n"
		"\n"
		CONNECTIONOPTIONS
		"\n"
		"Presets:\n"
		"	-agent			Shorthand for -quiet -format jsonl -nullsasnulls on\n"
		"				-continueonerror on: the settings a script or an\n"
		"				agent wants for output that's quiet, one json\n"
		"				object per line, nulls distinguishable from empty\n"
		"				strings, and a run that keeps going past a failed\n"
		"				statement.  Any of those given explicitly, such as\n"
		"				-format csv, still wins.\n"
		"\n"
		"Command options:\n"
		"	-script filename	Run the specified script which contains	commands\n"
		"				or queries that could otherwise be run at the\n"
		"				%s prompt.\n"
		"\n"
		"	-command \"commands\"	Run the provided string which contains commands\n"
		"				or queries that could otherwise be run at the\n"
		"				%s prompt.\n"
		"\n"
		"	-query \"query\"		Run the provided string as a single statement.\n"
		"				Unlike -command, the string is not scanned for\n"
		"				the delimiter character and no trailing\n"
		"				delimiter is needed.\n"
		"\n"
		"	-batch			With none of -script, -command or -query given,\n"
		"				read and run commands from stdin like an\n"
		"				interactive session, but without the banner or\n"
		"				the prompts, so piped-in SQL doesn't come back\n"
		"				mixed in with the output.  Unlike a real\n"
		"				interactive session, a failed statement counts,\n"
		"				and the exit code reflects it, the same as\n"
		"				-script and -command.\n"
		"\n"
		"	-delimiter char		End each command or query with the specified\n"
		"				character.  Defaults to a semicolon.\n"
		"				-delimeter is accepted as a spelling of it.\n"
		"\n"
		"	-continueonerror on|off\n"
		"				Carry on past a statement that failed, rather\n"
		"				than stopping at the first one.  Defaults to\n"
		"				off.  See Continuing past an error below.\n"
		"\n"
		"Output options:\n"
		"	-format plain|csv|json|jsonl\n"
		"				Format the output as specified.\n"
		"				Defaults to plain.  See Output formats below.\n"
		"\n"
		"	-quiet on|off		Shorthand for -headers off -stats off.\n"
		"				Defaults to off.\n"
		"\n"
		"	-headers on|off		Write the column names above the result set.\n"
		"				plain format only.  Defaults to on.\n"
		"\n"
		"	-divider on|off		Write a line of = signs under the column names.\n"
		"				plain format only.  Defaults to on.\n"
		"\n"
		"	-stats on|off		Write the statistics after the result set.\n"
		"				plain writes a block of labels, json and jsonl\n"
		"				carry the same numbers inside the document,\n"
		"				and csv has no statistics at all.\n"
		"				Defaults to on.\n"
		"\n"
		"	-noelapsed on|off	Leave the elapsed time out of the statistics.\n"
		"				Defaults to off.\n"
		"\n"
		"	-fieldsas raw|number|boolean|date\n"
		"				Fetch each field with the matching getFieldAs\n"
		"				method, where the column suits it.  Defaults to\n"
		"				raw, the field as the database sent it.  See\n"
		"				Fetching fields as a type below.\n"
		"\n"
		"	-getasnumber on|off	calls getFieldAs(Integer|Double) as appropriate.\n"
		"				An alias - on is -fieldsas number and off is\n"
		"				-fieldsas raw.  Defaults to off.\n"
		"\n"
		"	-nullsasnulls on|off	Get nulls as nulls, rather than as empty\n"
		"				strings.  csv, json and jsonl can only tell the\n"
		"				two apart while this is on.  Defaults to off.\n"
		"\n"
		"	-debug on|off		Write the client library debug messages to\n"
		"				stdout.  Defaults to off.\n"
		"\n"
		"	-debug filename		Write them to the named file instead.\n"
		"\n"
		"Fetch options:\n"
		"	-resultsetbuffersize rows\n"
		"				Fetch result sets using the specified number of\n"
		"				rows at once.  0 means fetch the whole result\n"
		"				set at once.  Defaults to 100.\n"
		"\n"
		"	-lazyfetch on|off	Fetch rows as they are asked for, rather than\n"
		"				when the query runs.  Defaults to off.\n"
		"\n"
		"	-nextresultset on|off	Fetch every result set the query returns,\n"
		"				rather than just the first.  Defaults to off.\n"
		"\n"
		"	-txqueries on|off	Send begin, commit and rollback to the database\n"
		"				as queries, rather than running them as\n"
		"				commands.  Defaults to off.\n"
		"\n"
		"Bind variable options:\n"
		"	-validatebinds on|off	Ignore bind variables the query does not have,\n"
		"				rather than failing.  Defaults to off.\n"
		"\n"
		"	-bindvariabledelimiters delimiters\n"
		"				Recognize the given bind variable delimiters,\n"
		"				from the set ? : @ $.\n"
		"\n"
		"Session options:\n"
		"	-final on|off		End the session after every query, so each one\n"
		"				gets a session of its own.  Defaults to off.\n"
		"\n"
		"	-autocommit on|off	Commit after every query.  Without this the\n"
		"				server decides, so there is no default here.\n"
		"\n"
		"Other options:\n"
		"	-locale env|locale_name	calls setlocale(LC_ALL, locale_name).\n"
		"				env means use LC variables.\n"
		"\n"
		"	-localstatedir dir	Override the default directory for keeping\n"
		"				working or stateful files, the result set\n"
		"				cache among them, with the specified\n"
		"				directory.\n"
		"\n"
		"	-help			Show this help and exit.  Must be the only\n"
		"				option.\n"
		"\n"
		"	-version		Show the version and exit.  Must be the only\n"
		"				option.\n"
		"\n"
		"Option values:\n"
		"\n"
		"An option that takes a value has to be given one, so -resultsetbuffersize by\n"
		"itself exits 1.  A value that begins with a dash has to be given as\n"
		"--option=value, because \"-option -value\" reads the dash as the next option.\n"
		"\n"
		"On and off:\n"
		"\n"
		"An on|off option with no value means on, so -headers by itself turns headers\n"
		"on.  Any value that does not begin with \"on\" means off, and nothing is said\n"
		"about it.  -quiet is applied before the others, so \"-quiet -headers on\" gives\n"
		"headers on and stats off.\n"
		"\n"
		"-debug is the exception.  A value that is neither on nor off is a file name,\n"
		"the same as it is for the debug command.\n"
		"\n"
		"Every one of these settings is also a command, but a command with no value\n"
		"means off, not on, so \"headers;\" turns headers off.  Some settings are\n"
		"commands only, cache, opencache and the bind variable commands among them.\n"
		"Run \"help;\" for the full list.\n"
		"\n"
		"Output formats:\n"
		"\n"
		"plain	Columns are space padded, the column names go above the result set,\n"
		"	a line of = signs goes under the names, and a block of tab indented\n"
		"	labels goes under the rows.  A null is written as the word NULL,\n"
		"	which is indistinguishable from the four character string NULL.\n"
		"	-headers, -divider and the statistics block are this format only.\n"
		"\n"
		"csv	One header row of column names, then one row per row of the result\n"
		"	set, and nothing else.  The header row is always written, whatever\n"
		"	-headers says, because it is the only way a reader learns what the\n"
		"	columns are.\n"
		"	A field is quoted when it contains a double quote, a comma, a\n"
		"	newline, a carriage return, a tab, or a null byte, and also when it\n"
		"	is not a number, or is a number of 12 or more digits.  An embedded\n"
		"	double quote is doubled.  Every other byte passes through unchanged,\n"
		"	so utf-8 survives.  A database null is written as an unquoted empty\n"
		"	field and an empty string as a quoted empty field, that is, as two\n"
		"	double quotes.\n"
		"\n"
		"json	One compact document per command, all on one line, so a run of\n"
		"	several commands is still readable a line at a time.  Wrapped here\n"
		"	to fit the page, but written as one line:\n"
		"\n"
		"	{\"columns\":[{\"name\":\"id\",\"type\":\"INTEGER\"}],\"rows\":[[1]],\n"
		"	\"affectedrows\":0,\"rowsreturned\":1,\"fieldsreturned\":1,\n"
		"	\"elapsed\":0.001234}\n"
		"\n"
		"	Rows are arrays, since the names are already in the column list.\n"
		"	Best for a small result set, where the reader wants the whole thing.\n"
		"\n"
		"jsonl	One json object per line, each typed by a \"type\" member.  The\n"
		"	stats object is wrapped here too, and is really one line:\n"
		"\n"
		"	{\"type\":\"columns\",\"columns\":[{\"name\":\"id\",\"type\":\"INTEGER\"}]}\n"
		"	{\"type\":\"row\",\"row\":{\"id\":1}}\n"
		"	{\"type\":\"stats\",\"affectedrows\":0,\"rowsreturned\":1,\n"
		"		\"fieldsreturned\":1,\"elapsed\":0.001234}\n"
		"\n"
		"	Rows are objects keyed by column name, so a line stands on its own.\n"
		"	Best for a large result set - it streams, so head, grep and wc work\n"
		"	on it, and a partial result set is still usable.\n"
		"\n"
		"json and jsonl both carry the column types, which neither plain nor csv does.\n"
		"A database null is the json null literal, never \"\" and never \"NULL\".  A\n"
		"converted field is typed, so -fieldsas number gives a bare json number and\n"
		"-fieldsas boolean a bare json true or false.  Everything else, a converted\n"
		"date included, is a json string.  A value is never truncated.  The column\n"
		"list is always written and no divider is written, for the same reason csv\n"
		"always writes its header row.  The statistics are part of the document and\n"
		"are left out when stats are off, and -noelapsed drops just the elapsed\n"
		"member.  A statement with no result set still produces a document, with an\n"
		"empty column list, an empty row list, and the affected row count in the\n"
		"statistics.  The startup banner and the prompt are not written.  An error is\n"
		"one object on one line on stderr, and stdout gets nothing:\n"
		"\n"
		"	{\"error\":{\"number\":1,\"message\":\"no such table: nosuchtable\"}}\n"
		"\n"
		"A command that produces a single value writes a one line object keyed by the\n"
		"command name, so a reader gets the same shape from every one of them:\n"
		"{\"identify\":\"sqlite\"}, {\"ping\":true}, {\"getresultsetbuffersize\":100}.\n"
		"printbinds writes one object holding the three bind lists, and the fields\n"
		"command writes {\"fields\":[\"id\",\"descr\"]}.\n"
		"\n"
		"csv, json and jsonl can only tell a null from an empty string while\n"
		"nullsasnulls is on.  With it off, which is the default, the client library\n"
		"hands back an empty string for a null and %s cannot tell the two apart.\n"
		"\n"
		"A format name is matched ignoring case.  An unrecognized name is an error:\n"
		"-format bogus exits 1, and \"format bogus;\" is a failed command.\n"
		"\n"
		"Fetching fields as a type:\n"
		"\n"
		"-fieldsas, and the fieldsas command, choose which getFieldAs method every\n"
		"field of every result set is fetched with.  There is one setting, shared by\n"
		"all four formats, since all four are renderings of the same rows.  A mode\n"
		"only converts a field where the conversion means something, and writes the\n"
		"field unchanged where it doesn't, so a mode never turns a name column into\n"
		"a number, a true or a date.\n"
		"\n"
		"raw	The field as the database sent it.  The default.\n"
		"\n"
		"number	getFieldAsInteger or getFieldAsDouble, whichever the column type\n"
		"	calls for, on a numeric or bit column.  Every other column passes\n"
		"	through.\n"
		"\n"
		"boolean	getFieldAsBoolean, on a boolean, bit or numeric column.  Every\n"
		"	other column passes through.  Numeric columns are included because a\n"
		"	database with no boolean type stores a boolean as 0 or 1.  The result\n"
		"	is the word true or false, and a bare json true or false in json and\n"
		"	jsonl.\n"
		"\n"
		"date	getFieldAsDate, on any column whose value reads as a date.  A field\n"
		"	it cannot read passes through, so no column type test is needed.  The\n"
		"	result is [-]YYYY-MM-DD HH:MM:SS[.uuuuuu], carrying only the parts the\n"
		"	field had, so a date column gives a date, a time column a time, and a\n"
		"	timestamp both.  A fraction of a second is written when it is not\n"
		"	zero, since a fraction that was not there and one that was zero read\n"
		"	back the same.  A zero month or day, which is how mysql stores\n"
		"	0000-00-00, is not a date, so the whole field passes through.  The\n"
		"	result is a json string, not an object of parts.\n"
		"\n"
		"getasnumber is an alias: on is fieldsas number and off is fieldsas raw.  When\n"
		"-getasnumber and -fieldsas are both given, -fieldsas wins.  A mode name is\n"
		"matched ignoring case, and an unrecognized one is an error: -fieldsas bogus\n"
		"exits 1, and \"fieldsas bogus;\" is a failed command.\n"
		"\n"
		"Exit codes:\n"
		"\n"
		"0	Success.  A genuinely interactive session always exits 0, even if a\n"
		"	query at the prompt failed - it isn't a batch.  -batch is, so a\n"
		"	failed statement there is a 4, the same as -script and -command.\n"
		"\n"
		"1	Usage error.  A run with no -id, no -host and no -socket, an option\n"
		"	that takes a value given without one, an unrecognized -format or\n"
		"	-fieldsas name, a -locale value that setlocale rejects, or an -id\n"
		"	whose configured password encryption is missing or one-way.\n"
		"\n"
		"2	The file named by -script could not be opened.  A file that the run\n"
		"	command could not open is a failed command in the outer script, so\n"
		"	that is a 4, not a 2.\n"
		"\n"
		"3	Connection or authentication failure.  The connection is checked at\n"
		"	startup, so this is reported before any command runs, in every mode.\n"
		"\n"
		"4	At least one statement in a script, a -command list, a -query, or a\n"
		"	-batch run failed, not necessarily all of them.  Without\n"
		"	-continueonerror the run stopped at that statement, and with it the\n"
		"	run went on.\n"
		"\n"
		"Continuing past an error:\n"
		"\n"
		"-continueonerror, and the continueonerror command, make a script, a\n"
		"-command list, or a -batch run go through every statement rather than\n"
		"stopping at the first one that failed.  A statement rejected before it\n"
		"reached the database and one the database rejected count the same, and\n"
		"each failed statement still writes its own error.\n"
		"\n"
		"A failure that takes the session down with it stops the run anyway, because\n"
		"the rest of the statements have nothing left to run against.\n"
		"\n"
		"At the end of a run with anything failed, the number of failed statements is\n"
		"written to stderr as \"statements failed: N\", and the exit code is 4.  With\n"
		"continueonerror off the run stopped at its first failure, so the count would\n"
		"always be 1 and none is written.  A failing command in ~/.sqlrshrc is not\n"
		"counted and does not change the exit code.\n"
		"\n"
		"Commands:\n"
		"\n"
		"Run \"help;\" at the %s prompt, or\n"
		"\"%s -id myinst -command 'help;'\", for the command reference.\n"
		"\n"
		"Examples:\n"
		"\n"
		"Interactive session with server at svr:9000 as usr/pwd.\n"
		"\n"
		"	%s -host svr -port 9000 -user usr -password pwd\n"
		"\n"
		"Interactive session with local server on socket /tmp/svr.sock as usr/pwd.\n"
		"\n"
		"	%s -socket /tmp/svr.sock -user usr -password pwd\n"
		"\n"
		"Interactive session using connection info and credentials from an instance\n"
		"defined in the default configuration.\n"
		"\n"
		"	%s -id myinst\n"
		"\n"
		"Interactive session using connection info and credentials from an instance\n"
		"defined in the config file ./myconfig.conf\n"
		"\n"
		"	%s -config ./myconfig.conf -id myinst\n"
		"\n"
		"Non-interactive session, running commands from ./script.sql\n"
		"\n"
		"	%s -id myinst -script ./script.sql\n"
		"\n"
		"Non-interactive session, running query \"select * from mytable\" with csv\n"
		"output.\n"
		"\n"
		"	%s -id myinst -command \"select * from mytable\" -quiet -format csv\n"
		"\n"
		"Non-interactive session, running a single query with no trailing delimiter\n"
		"needed.\n"
		"\n"
		"	%s -id myinst -query \"select * from mytable\"\n"
		"\n"
		"Non-interactive session, piping commands into stdin, with no banner or\n"
		"prompts mixed into the output.\n"
		"\n"
		"	echo \"select 1;\" | %s -id myinst -batch\n"
		"	%s -id myinst -batch < script.sql\n"
		"\n"
		"Non-interactive session, one json document per command, on stdout, with\n"
		"errors on stderr.  nullsasnulls is what makes a null come out as the\n"
		"json null literal.\n"
		"\n"
		"	%s -id myinst -format json \\\n"
		"		-command \"nullsasnulls on; select * from mytable\"\n"
		"\n"
		"Non-interactive session, one jsonl object per line, piped to a reader that\n"
		"takes a line at a time.\n"
		"\n"
		"	%s -id myinst -format jsonl -command \"select * from mybigtable\" | myreader\n"
		"\n"
		"Non-interactive session, the machine-readable preset, running a script\n"
		"that keeps going past a failed statement.\n"
		"\n"
		"	%s -id myinst -agent -script ./script.sql\n"
		"\n"
		"Non-interactive session, checking the exit code.\n"
		"\n"
		"	%s -id myinst -script ./script.sql || echo \"failed with $?\"\n"
		"\n",
		progname,SQL_RELAY,progname,progname,progname,
		progname,progname,progname,progname,progname,
		progname,progname,progname,progname,progname,
		progname,progname,progname,progname,progname,
		progname);
}

int main(int argc, const char **argv) {

	version(argc,argv);
	help(argc,argv);

	#ifdef SIGPIPE
	// ignore SIGPIPE
	signalset	set;
	set.removeAllSignals();
	set.addSignal(SIGPIPE);
	signalmanager::ignoreSignals(&set);
	#endif

	int32_t	exitcode=SQLRSH_EXIT_SUCCESS;
	{
		sqlrsh	s;
		exitcode=s.execute(argc,argv);
	}
	process::exit(exitcode);
}

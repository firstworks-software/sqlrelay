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
#include <rudiments/memorypool.h>
#include <rudiments/prompt.h>
#include <rudiments/locale.h>
#include <rudiments/sensitivevalue.h>
#include <config.h>
#include <defaults.h>
#define NEED_IS_BIT_TYPE_CHAR 1
#define NEED_IS_NUMBER_TYPE_CHAR 1
#define NEED_IS_FLOAT_TYPE_CHAR 1
#define NEED_IS_NONSCALE_FLOAT_TYPE_CHAR 1
#include <datatypes.h>
#include <defines.h>
#include <version.h>
#include <math.h>

class sqlrshbindvalue {
	public:
		union {
			char	*stringval;
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

// These are part of the interface too.  Every site that acts on the format
// switches on it, with no default label, so -Wswitch makes the compiler point
// at each one of them when a format is added here.
enum sqlrshformat {
	SQLRSH_FORMAT_PLAIN=0,
	SQLRSH_FORMAT_CSV,
	SQLRSH_FORMAT_JSON,
	SQLRSH_FORMAT_JSONL
};

// the format names, in one place, so the format command and the -format
// option can't drift apart
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

// looks "name" up in the list above, ignoring case.  Returns true and sets
// "format" if it's a format name, or false if it isn't.
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

// writes "name isn't a format, here are the ones that are" to stderr
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

// Returns the argument that follows a command word, trimmed, or NULL if the
// command was given without one.  The caller owns the string.
static char *commandArgument(const char *args) {

	char	*arg=charstring::duplicate(args);
	charstring::bothTrim(arg);
	if (charstring::isNullOrEmpty(arg)) {
		delete[] arg;
		return NULL;
	}
	return arg;
}

// These are what a program driving sqlrsh non-interactively has to go on.
// They are part of the interface, like the output formats are.  Add to them,
// but don't renumber them and don't change what one means.
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
		char		*cacheto;
		// The connection keeps the socket it's handed rather than
		// copying it, so the one a resumesession was given has to
		// outlive the command that ran it.
		char		*resumesocket;
		sqlrshformat	format;
		bool		getasnumber;
		bool		noelapsed;
		bool		nextresultset;
		bool		txqueries;
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
	cacheto=NULL;
	resumesocket=NULL;
	format=SQLRSH_FORMAT_PLAIN;
	getasnumber=false;
	noelapsed=false;
	nextresultset=false;
	txqueries=false;
	inputbinds.setManageArrayKeys(true);
	outputbinds.setManageArrayKeys(true);
	inputoutputbinds.setManageArrayKeys(true);
}

sqlrshenv::~sqlrshenv() {
	clearbinds(&inputbinds);
	clearbinds(&outputbinds);
	clearbinds(&inputoutputbinds);
	delete[] cacheto;
	delete[] resumesocket;
}

void sqlrshenv::clearbinds(dictionary<char *, sqlrshbindvalue *> *binds) {

	for (listnode<char *> *node=binds->getKeys()->getFirst();
						node; node=node->getNext()) {

		sqlrshbindvalue	*bv=binds->getValue(node->getValue());
		if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
			delete[] bv->stringval;
		}
		delete bv;
	}
	binds->clear();
	inbindpool.clear();
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
		// "asnumber" asks for a bare json number, which it gets if
		// it really looks like one, and a json string if it doesn't.
		void	jsonWriteValue(filedescriptor *fd,
						const char *field,
						uint32_t length,
						bool asnumber);
		// fetches field "col" of row "row", applying the getasnumber
		// conversion.  Sets "*length" to the field's length and
		// "*asnumber" to whether the conversion happened.  Returns
		// NULL for a database null.  "numberbuffer" is scratch space
		// the conversion writes into, so it has to outlive the
		// returned pointer.
		const char *getFieldForDisplay(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						uint64_t row, uint32_t col,
						uint32_t *length,
						char *numberbuffer,
						size_t numberbuffersize,
						bool *asnumber);
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
		// writes what a timeout-setting command produces.  "label"
		// heads the plain sentence, "name" keys the json object.
		void	writeTimeoutSet(sqlrshenv *env,
						const char *name,
						const char *label,
						uint32_t sec,
						uint32_t msec);
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
		bool	inputbind(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command);
		bool	inputbindblob(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command);
		bool	outputbind(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command);
		bool	inputoutputbind(sqlrcursor *sqlrcur,
						sqlrshenv *env,
						const char *command);
		void	printbinds(const char *type,
				dictionary<char *, sqlrshbindvalue *> *binds);
		// writes one member of the printbinds object, "key" mapped
		// to an object keyed by bind variable name
		void	jsonPrintBinds(const char *key,
				dictionary<char *, sqlrshbindvalue *> *binds);
		void	clearbinds(
				dictionary<char *, sqlrshbindvalue *> *binds);
		void	setclientinfo(sqlrconnection *sqlrcon,
						const char *command);
		void	getclientinfo(sqlrconnection *sqlrcon,
						sqlrshenv *env);
		void	delimiter(sqlrshenv *env);
		void	autocommit(sqlrshenv *env, bool on);
		bool	connectTimeout(sqlrconnection *sqlrcon,
						sqlrshenv *env,
						const char *args);
		void	responseTimeout(sqlrconnection *sqlrcon,
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
	runScript(sqlrcon,sqlrcur,env,userrcfile,false);
	delete[] userrcfile;
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
			if (!runCommand(sqlrcon,sqlrcur,env,
						command.getString(),
						NULL)) {
				exitcode=SQLRSH_EXIT_QUERY;
				break;
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
		if (!runCommand(sqlrcon,sqlrcur,env,
					command.getString(),
					exitprogram)) {
			return false;
		}
	}
	return true;
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
		!charstring::compareIgnoringCase(ptr,"outputbind ",11) ||
		!charstring::compareIgnoringCase(ptr,"inputoutputbind ",16) ||
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
		!charstring::compareIgnoringCase(ptr,"lazyfetch ",10) ||
		!charstring::compareIgnoringCase(ptr,"endsession") ||
		!charstring::compareIgnoringCase(ptr,"querytree") ||
		!charstring::compareIgnoringCase(ptr,"translatedquery") ||
		!charstring::compareIgnoringCase(ptr,"response timeout",16) ||
		!charstring::compareIgnoringCase(ptr,"cache ",6) ||
		!charstring::compareIgnoringCase(ptr,"opencache ",10) ||
		!charstring::compareIgnoringCase(ptr,"txqueries ",10)) {

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
		ptr=ptr+13;
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
		// Nothing is written on success, the way the use command
		// writes nothing.  What the caller needs next is the port and
		// the socket, and those are commands of their own.
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
		// Nothing is written on success, the way the use command
		// writes nothing.  What the caller needs next is the id, and
		// that's a command of its own.
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
		// clears the flag, so getCacheFileName() still hands back the
		// pointer it was given, and the cachefilename command would
		// read freed memory if this deleted it.
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
		return inputbindblob(sqlrcur,env,command);
	} else if (!charstring::compareIgnoringCase(ptr,"outputbind ",11)) {	
		return outputbind(sqlrcur,env,command);
	} else if (!charstring::compareIgnoringCase(
						ptr,"inputoutputbind ",16)) {	
		return inputoutputbind(sqlrcur,env,command);
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
				// one object, one line, with the three
				// lists as members, each keyed by bind
				// variable name
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
	} else if (!charstring::compareIgnoringCase(ptr,"clearinputbind",14)) {	
		env->clearbinds(&env->inputbinds);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"clearoutputbind",15)) {
		env->clearbinds(&env->outputbinds);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,
						"clearinputoutputbind",20)) {
		env->clearbinds(&env->inputoutputbinds);
		return true;
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
	} else if (!charstring::compareIgnoringCase(ptr,"lazyfetch ",10)) {
		ptr=ptr+10;
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
		// plain and csv get the tree pretty printed as xml, the way
		// they always have.  json and jsonl get the same xml, as a
		// string, because a query tree isn't json and giving it a
		// json shape of its own is a ticket in itself.
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
		responseTimeout(sqlrcon,env,ptr+16);
		return true;
	} else if (!charstring::compareIgnoringCase(ptr,"cache ",6)) {
		return cache(env,sqlrcur,command);
	} else if (!charstring::compareIgnoringCase(ptr,"opencache ",10)) {
		return openCache(env,sqlrcur,command);
	} else if (!charstring::compareIgnoringCase(ptr,"txqueries ",10)) {
		ptr=ptr+10;
		cmdtype=13;
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

	// handle nullsasnulls
	if (cmdtype==9) {
		if (!charstring::compareIgnoringCase(ptr,"on",2)) {
			sqlrcur->getNullsAsNulls();
		} else if (!charstring::compareIgnoringCase(ptr,"off",3)) {
			sqlrcur->getNullsAsEmptyStrings();
		}
		return true;
	}

	// handle format
	// The name goes through the same list the -format option uses.  An
	// unrecognized name used to quietly mean plain, so "format jsonl" on
	// a build without jsonl looked like it had worked.  It's a failed
	// command now, which a script stops on and exits SQLRSH_EXIT_QUERY.
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
			autocommit(env,toggle);
			break;
		case 12:
			env->lazyfetch=toggle;
			break;
		case 13:
			env->txqueries=toggle;
			break;
		case 14:
			env->getasnumber=toggle;
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
		// One arm per format, for the reason displayHeader() gives.
		// Names go through the format's own field writer, so one
		// containing a comma or a double quote can't break the line
		// apart.
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
						"show catalogs mysql",20)) {
			char	*wild=getWild(command);
			sqlrcur->getCatalogList(wild,
					SQLRCLIENTLISTFORMAT_MYSQL);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show catalogs odbc",19)) {
			char	*wild=getWild(command);
			sqlrcur->getCatalogList(wild,
					SQLRCLIENTLISTFORMAT_ODBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show catalogs jdbc",19)) {
			char	*wild=getWild(command);
			sqlrcur->getCatalogList(wild,
					SQLRCLIENTLISTFORMAT_JDBC);
			delete[] wild;
		} else if (!charstring::compareIgnoringCase(command,
						"show catalogs",14)) {
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
				sqlrcur->inputBind(name,bv->stringval);
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
				sqlrcur->inputBindBlob(name,bv->stringval,
					charstring::getLength(bv->stringval));
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
				// FIXME: make buffer length variable
				sqlrcur->defineOutputBindString(name,
						bv->outputstringbindlength);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
				sqlrcur->defineOutputBindInteger(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
				sqlrcur->defineOutputBindDouble(name);
			} else if (bv->type==SQLRCLIENTBINDVARTYPE_DATE) {
				sqlrcur->defineOutputBindDate(name);
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
						bv->stringval,
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

	sqlrcur->executeQuery();

	if (env->outputbinds.getCount()) {

		for (listnode<char *> *node=
				env->outputbinds.getKeys()->getFirst();
				node; node=node->getNext()) {

			const char	*name=node->getValue();
			sqlrshbindvalue	*bv=
				env->outputbinds.getValue(node->getValue());
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				delete[] bv->stringval;
				bv->stringval=charstring::duplicate(
					sqlrcur->getOutputBindString(name));
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
				delete[] bv->stringval;
				bv->stringval=charstring::duplicate(
				sqlrcur->getInputOutputBindString(name));
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
			// One object, one line, on stderr, where it can't
			// corrupt the document on stdout.  jsonl reads it a
			// line at a time like the rest of the stream.
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

	// One arm per format, rather than one arm for plain and an else that
	// silently means every other format.  There's no default label, so
	// -Wswitch makes the compiler point right here when a format is
	// added to the enum.
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

void sqlrsh::plainDisplayHeader(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// The headers toggle applies to the plain format.  There the column
	// names are a convenience for the person reading the output, so they
	// can be turned off.  Every other format is handed to a parser, and
	// the names are the only way it can learn what the columns are, so
	// they're part of the data there and always written.
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
		uint32_t	longest=sqlrcur->getLongest(ci);
		if (namelen>longest) {
			longest=namelen;
		}
		charcount=charcount+longest;
		for (uint32_t j=namelen; j<longest; j++) {
			stdoutput.write(' ');
		}
	}
	stdoutput.printf("\n");

	// display divider
	// Only the plain format gets one.  It underlines the column names
	// there.  Every other format is meant to be parsed, and a row of
	// equals signs isn't part of any of them.
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
	// jsonDisplayResultSet() closes it.  jsonl writes a standalone
	// columns object and every later line stands alone too.
	// A statement with no result set - an insert, say - still gets a
	// document, with an empty column list, so a reader gets exactly one
	// per statement and never has to guess whether another is coming.
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

	// Everything json doesn't require an escape for goes through
	// unchanged, one run of bytes at a time, so utf-8 and every other
	// high byte comes out the way it went in.  Note the unsigned char:
	// with a plain char every utf-8 continuation byte is negative and
	// would read as a control character.
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

// Returns whether "field" is a number the way json defines one:
//	-? (0 | [1-9][0-9]*) (\.[0-9]+)? ([eE][-+]?[0-9]+)?
// Anything else - inf, nan, a leading plus, a leading zero, a bare leading
// dot - has to go out as a string instead, or the document won't parse.
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
					uint32_t length, bool asnumber) {

	// A database null is the json null literal.  json has a real one, so
	// there's no reason to make a reader guess at an empty string, and no
	// way to confuse it with the string "NULL" either.
	if (!field) {
		fd->write("null");
		return;
	}

	// A number only goes out bare if it really is a json number.
	// getFieldAsDouble() can hand back inf or nan, and neither is one.
	if (asnumber && jsonIsNumber(field,length)) {
		fd->write(field,(size_t)length);
		return;
	}

	jsonWriteString(fd,field,length);
}

const char *sqlrsh::getFieldForDisplay(sqlrcursor *sqlrcur, sqlrshenv *env,
					uint64_t row, uint32_t col,
					uint32_t *length,
					char *numberbuffer,
					size_t numberbuffersize,
					bool *asnumber) {

	const char	*field=sqlrcur->getField(row,col);
	const char	*fieldtype=sqlrcur->getColumnType(col);
	*length=sqlrcur->getFieldLength(row,col);
	*asnumber=false;

	// FIXME: move this down below the end-of-rs check?
	// The purpose of this is to verify the functionality
	// of the getFieldAsXXX() methods.
	if (field && env->getasnumber &&
		(isBitTypeChar(fieldtype) ||
			isNumberTypeChar(fieldtype))) {

		if (isFloatTypeChar(fieldtype)) {
			double	fd=sqlrcur->getFieldAsDouble(row,col);
			if (isNonScaleFloatTypeChar(fieldtype)) {
				int32_t	precision=sqlrcur->getColumnPrecision(col);
				// here precision is a number of bits, but printf %g wants digits.
				// FIXME: precision should actually be the number of digits, not bits...
				int32_t	digits=(int32_t)(ceil(precision/3.33));
				charstring::printf(numberbuffer,numberbuffersize,"%.*g",digits,fd);
			} else {
				int	scale=sqlrcur->getColumnScale(col);
				// NOTE: we are not using the precision to format the number to a string.
				charstring::printf(numberbuffer,numberbuffersize,"%.*f",scale,fd);
			}
		} else {
			int64_t fi = sqlrcur->getFieldAsInteger(row,col);
			charstring::printf(numberbuffer, numberbuffersize, "%ld", fi);
		}
		field=numberbuffer;
		*length=charstring::getLength(field);
		*asnumber=true;
	}

	return field;
}

void sqlrsh::displayResultSet(sqlrcursor *sqlrcur, sqlrshenv *env) {

	// One arm per format, for the reason displayHeader() gives.
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

	char		numberfieldbuffer[256];

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
			bool		asnumber;
			const char	*field=getFieldForDisplay(sqlrcur,env,
						row,col,&fieldlength,
						numberfieldbuffer,
						sizeof(numberfieldbuffer),
						&asnumber);

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
			uint32_t	longest=sqlrcur->getLongest(col);
			if (env->headers) {
				uint32_t	namelen=charstring::getLength(
						sqlrcur->getColumnName(col));
				if (namelen>longest) {
					longest=namelen;
				}
			}
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

	char		numberfieldbuffer[256];

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
			bool		asnumber;
			const char	*field=getFieldForDisplay(sqlrcur,env,
						row,col,&fieldlength,
						numberfieldbuffer,
						sizeof(numberfieldbuffer),
						&asnumber);

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

	char		numberfieldbuffer[256];

	// A statement with no result set has no rows to look for, and
	// looking anyway would never reach the end-of-result-set test,
	// because that lives in the column loop.
	bool		done=!colcount;
	for (uint64_t row=0; !done; row++) {

		for (uint32_t col=0; col<colcount; col++) {

			// get the field
			uint32_t	fieldlength;
			bool		asnumber;
			const char	*field=getFieldForDisplay(sqlrcur,env,
						row,col,&fieldlength,
						numberfieldbuffer,
						sizeof(numberfieldbuffer),
						&asnumber);

			// check for end-of-result-set condition
			// (since nullsasnulls might be set, we have to do
			// a bit more than just check for a NULL)
			// This runs before anything is written for the row,
			// so the row that isn't there leaves no trace.
			if (!col && !field &&
				sqlrcur->endOfResultSet() &&
				row==sqlrcur->rowCount()) {
				done=true;
				break;
			}

			// open the row
			// jsonl rows are objects, keyed by column name, so
			// each line carries its own names and stands alone.
			// json rows are arrays, since the names are already
			// in the columns list at the top of the document.
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
			jsonWriteValue(&stdoutput,field,fieldlength,asnumber);
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
	// -nextresultset calls this once per result set and each one gets
	// its own document, or its own group of lines.
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

	// This block of tab indented labels is plain format only.  It's a
	// note to the person who ran the query, and plain is the only format
	// a person reads directly.  csv has no place for it.  json and jsonl
	// carry the same numbers, but as part of the document, which
	// jsonDisplayResultSet() writes, because with -nextresultset there
	// is a set of them per result set and this runs once per statement.
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

	// One arm per format, for the reason displayHeader() gives.
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

	// A block of tab indented labels per column, the way displayStats()
	// writes its block.  Thirteen values across a row would be too wide to
	// read, and plain is the format a person reads directly.
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

	// One row per column, with a header row, the way csvDisplayHeader()
	// always writes one.  The names are the only way a csv reader can
	// learn what the fields are, so they're data rather than decoration.
	for (const char * const *key=sqlrshcolumninfokeys; *key; key++) {
		if (key!=sqlrshcolumninfokeys) {
			stdoutput.write(',');
		}
		csvWriteField(*key,charstring::getLength(*key));
	}
	stdoutput.write('\n');

	// The names and types go through csvWriteField(), so one containing a
	// comma or a double quote can't break the row apart.  The numbers and
	// the booleans are written bare, the way writeScalarNumber() and
	// writeScalarBoolean() write them in csv.
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

	// json and jsonl agree here.  A column list is one object on one line
	// either way, so there's nothing to stream and nothing to differ
	// about, the way the fields command already works.  The keys are the
	// ones sqlrshcolumninfokeys lists.
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
			// One object on one line, keyed by the command name,
			// so a reader gets the same thing from every one of
			// these commands and never has to know which it ran.
			// json and jsonl agree here - there's nothing to
			// stream, so there's nothing to differ about.
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

	// Either half negative means the timeout is off, which is what
	// setConnectTimeout() and setResponseTimeout() say a negative value
	// does, so it goes out as a plain -1 rather than as arithmetic on two
	// negative numbers.
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
					uint32_t sec, uint32_t msec) {

	switch (env->format) {
		case SQLRSH_FORMAT_PLAIN:
		case SQLRSH_FORMAT_CSV:
			stdoutput.printf("%s set to %d.%04d seconds\n",
							label,sec,msec);
			break;
		case SQLRSH_FORMAT_JSON:
		case SQLRSH_FORMAT_JSONL:
			{
				// the value the command was given, echoed
				// back the way the command spells one
				char	value[64];
				charstring::printf(value,sizeof(value),
							"%d.%04d",sec,msec);
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
		// A command that needed an argument and didn't get one
		// failed, the way a query with a syntax error failed, so it
		// goes out as an error and exits SQLRSH_EXIT_QUERY.
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

	// The socket is the rest of the argument, and an inet session doesn't
	// have one, so it's optional.
	const char	*socket=arg;
	while (*socket && !character::isWhitespace(*socket)) {
		socket++;
	}
	while (character::isWhitespace(*socket)) {
		socket++;
	}

	// The connection keeps this pointer rather than copying it, and
	// getConnectionSocket() hands it back later, so env owns it for the
	// rest of the run, the way env owns cacheto.
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
	// four supported() methods, in the order the default "?:@$" lists
	// them.
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

	// This switch is on a delimiter, not on the output format, so it does
	// have a default label - an unrecognized delimiter is a failed
	// command.
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
	// database knows.  Either way the command failed, so it doesn't write
	// a null the way a command that really can have one does.
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

	// With an argument it turns column info on or off, without one it
	// dumps the metadata, the way isolationlevel sets with an argument and
	// reports without one.
	char	*arg=commandArgument(args);
	if (!arg) {
		columninfo(sqlrcur,env);
		return true;
	}

	// The argument is validated rather than run through the loose test the
	// older toggles use, where anything that isn't "on" quietly means off.
	// A bad argument is a failed command here.
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

	// The id is optional.  Without one the cursor's own id is used, which
	// is the one a suspendresultset in this same sqlrsh left behind.
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

	// The rows have been fetched by now, and sqlrsh has no command that
	// writes the result set it's holding, so they go out here, the way the
	// opencache command writes the result set it just opened.
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

	// The file name is the rest of the argument, and continuing to cache
	// is optional, so the name is too.
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

	// The cursor doesn't copy references, so whichever name it's holding
	// now is the one that has to stay alive, and the other one is free to
	// go.  Asking it is the only way to know: it hands the name to
	// cacheToFile() on the way through, but it can also return before
	// getting that far, and then it's still holding the old one.
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

bool sqlrsh::inputbind(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *command) {

	// sanity check
	const char	*ptr=command+10;
	const char	*space=charstring::findFirst(ptr,' ');
	if (!space) {
		stderror.printf("usage: inputbind [variable] = [value]\n");
		return false;
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
		stderror.printf("usage: inputbind [variable] = [value]\n");
		stderror.printf("       inputbind [variable] is null\n");
		return false;
	}
		
	// get the value
	char	*value=charstring::duplicate(ptr);
	charstring::bothTrim(value);
	size_t	valuelen=charstring::getLength(value);

	// if the bind variable is already defined, clear it...
	sqlrshbindvalue	*bv=NULL;
	if (env->inputbinds.getValue(variable,&bv)) {
		if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
			delete[] bv->stringval;
		}
		delete bv;
	}

	// define the variable
	bv=new sqlrshbindvalue;

	// first handle nulls, then...
	// anything enclosed in quotes is a string
	// if it's unquoted, check to see if it's an integer, float or date
	// if it's not, then it's a string
	if (!value) {
		bv->type=SQLRCLIENTBINDVARTYPE_NULL;
	} else if ((value[0]=='\'' && value[valuelen-1]=='\'') ||
			(value[0]=='"' && value[valuelen-1]=='"')) {

		bv->type=SQLRCLIENTBINDVARTYPE_STRING;

		// trim off quotes
		char	*newvalue=charstring::duplicate(value+1);
		newvalue[valuelen-2]='\0';
		delete[] value;

		// unescape the string
		bv->stringval=charstring::unescape(newvalue);
		delete[] newvalue;

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
		bv->stringval=value;
	}

	// put the bind variable in the list
	env->inputbinds.setValue(variable,bv);

	return true;
}

bool sqlrsh::inputbindblob(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *command) {

	// sanity check
	const char	*ptr=command+14;
	const char	*space=charstring::findFirst(ptr,' ');
	if (!space) {
		stderror.printf("usage: inputbindblob [variable] = [value]\n");
		return false;
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
		stderror.printf("usage: inputbindblob [variable] = [value]\n");
		stderror.printf("       inputbindblob [variable] is null\n");
		return false;
	}
		
	// get the value
	char	*value=charstring::duplicate(ptr);
	charstring::bothTrim(value);
	size_t	valuelen=charstring::getLength(value);

	// if the bind variable is already defined, clear it...
	sqlrshbindvalue	*bv=NULL;
	if (env->inputbinds.getValue(variable,&bv)) {
		if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
			delete[] bv->stringval;
		}
		delete bv;
	}

	// define the variable
	bv=new sqlrshbindvalue;

	// first handle nulls, then...
	// anything enclosed in quotes is a string
	// if it's unquoted, check to see if it's an integer, float or date
	// if it's not, then it's a string
	if (!value) {
		bv->type=SQLRCLIENTBINDVARTYPE_NULL;
	} else if ((value[0]=='\'' && value[valuelen-1]=='\'') ||
			(value[0]=='"' && value[valuelen-1]=='"')) {

		bv->type=SQLRCLIENTBINDVARTYPE_BLOB;

		// trim off quotes
		char	*newvalue=charstring::duplicate(value+1);
		newvalue[valuelen-2]='\0';
		delete[] value;

		// unescape the string
		bv->stringval=charstring::unescape(newvalue);
		delete[] newvalue;

	} else {
		bv->type=SQLRCLIENTBINDVARTYPE_BLOB;
		bv->stringval=value;
	}

	// put the bind variable in the list
	env->inputbinds.setValue(variable,bv);

	return true;
}

bool sqlrsh::outputbind(sqlrcursor *sqlrcur,
				sqlrshenv *env, const char *command) {

	// split the command on ' '
	char		**parts;
	uint64_t	partcount;
	charstring::split(command," ",true,&parts,&partcount);

	// sanity check...
	bool	sane=true;
	if (partcount>2 && !charstring::compare(parts[0],"outputbind")) {

		// if the bind variable is already defined, clear it...
		sqlrshbindvalue	*bv=NULL;
		if (env->outputbinds.getValue(parts[1],&bv)) {
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				delete[] bv->stringval;
			}
			delete bv;
		}

		// define the variable
		bv=new sqlrshbindvalue;

		if (!charstring::compareIgnoringCase(
						parts[2],"string") &&
						partcount==4) {
			bv->type=SQLRCLIENTBINDVARTYPE_STRING;
			bv->stringval=NULL;
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
		} else {
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
	if (sane) {
		delete[] parts[0];
	} else {
		stderror.printf("usage: outputbind "
				// FIXME: not entirely accurate
				"[variable] [type] [length] [scale]\n");
		for (uint64_t i=0; i<partcount; i++) {
			delete[] parts[i];
		}
	}
	delete[] parts;

	return sane;
}

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
		// FIXME: usage...
		return false;
	}

	// split the command on ' '
	char		**parts;
	uint64_t	partcount;
	charstring::split(command," ",true,&parts,&partcount);

	// sanity check...
	bool	sane=true;
	if (partcount>=5 && !charstring::compare(parts[0],"inputoutputbind")) {

		// if the bind variable is already defined, clear it...
		sqlrshbindvalue	*bv=NULL;
		if (env->inputoutputbinds.getValue(parts[1],&bv)) {
			if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
				delete[] bv->stringval;
			}
			delete bv;
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
			bv->stringval=charstring::unescape(value);
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
	if (sane) {
		delete[] parts[0];
	} else {
		stderror.printf("usage: inputoutputbind "
				// FIXME: not entirely accurate
				"[variable] [type] [length] [scale]\n");
		for (uint64_t i=0; i<partcount; i++) {
			delete[] parts[i];
		}
	}
	delete[] parts;
	delete[] value;

	return sane;
}

void sqlrsh::printbinds(const char *type,
			dictionary<char *, sqlrshbindvalue *> *binds) {

	stdoutput.printf("%s bind variables:\n",type);

	for (listnode<char *> *node=binds->getKeys()->getFirst();
						node; node=node->getNext()) {

		stdoutput.printf("    %s ",node->getValue());
		sqlrshbindvalue	*bv=binds->getValue(node->getValue());
		if (bv->type==SQLRCLIENTBINDVARTYPE_STRING) {
			stdoutput.printf("(STRING) = %s\n",bv->stringval);
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
			stdoutput.safePrint(bv->stringval,
					charstring::getLength(bv->stringval));
			stdoutput.printf("\n");
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

		// The value carries its own type, so it goes out as the
		// json type that matches: a number for a number, a string
		// for a string, a date, or a blob, and null for a null.
		sqlrshbindvalue	*bv=binds->getValue(node->getValue());
		if (bv->type==SQLRCLIENTBINDVARTYPE_STRING ||
				bv->type==SQLRCLIENTBINDVARTYPE_BLOB) {
			jsonWriteString(&stdoutput,bv->stringval,
					charstring::getLength(bv->stringval));
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_INTEGER) {
			stdoutput.printf("%lld",(long long)bv->integerval);
		} else if (bv->type==SQLRCLIENTBINDVARTYPE_DOUBLE) {
			charstring::printf(buffer,sizeof(buffer),"%*.*f",
						(int)bv->doubleval.precision,
						(int)bv->doubleval.scale,
						bv->doubleval.value);
			charstring::bothTrim(buffer);
			jsonWriteValue(&stdoutput,buffer,
					charstring::getLength(buffer),true);
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
				// a one character string, because a
				// delimiter is a character
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

// Parses a timeout the way the response timeout command has always spelled
// one: whole seconds, then an optional dot and up to 4 more digits.  Those
// digits are handed to the connection as microseconds, which is what this
// command has always done.
static void parseTimeout(const char *args, uint32_t *sec, uint32_t *msec) {

	// skip to the timeout itself
	while (character::isWhitespace(*args)) {
		args++;
	}

	// get seconds
	*sec=charstring::convertToInteger(args);

	// get milliseconds
	char	msecbuf[5];
	bytestring::set(msecbuf,'0',4);
	msecbuf[4]='\0';
	const char	*dot=charstring::findFirst(args,'.');
	if (dot) {
		args=dot+1;
		for (uint8_t i=0; i<4 && *args; i++) {
			msecbuf[i]=*args;
			args++;
		}
	}
	*msec=charstring::convertToInteger(msecbuf);
}

bool sqlrsh::connectTimeout(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	char	*arg=commandArgument(args);
	if (!arg || !character::isDigit(arg[0])) {
		delete[] arg;
		displayError(env,NULL,
			"connect timeout needs a timeout in seconds, "
			"with an optional fraction",0);
		return false;
	}

	uint32_t	sec;
	uint32_t	msec;
	parseTimeout(arg,&sec,&msec);
	delete[] arg;

	// This applies to the next connect, not to the session sqlrsh is
	// already in, so it matters for a resumesession or for the reconnect
	// that follows an endsession.
	sqlrcon->setConnectTimeout((int32_t)sec,(int32_t)msec);

	writeTimeoutSet(env,"connecttimeout","Connect Timeout",sec,msec);
	return true;
}

void sqlrsh::responseTimeout(sqlrconnection *sqlrcon,
				sqlrshenv *env, const char *args) {

	uint32_t	sec;
	uint32_t	msec;
	parseTimeout(args,&sec,&msec);

	sqlrcon->setResponseTimeout((int32_t)sec,(int32_t)msec);

	writeTimeoutSet(env,"responsetimeout","Response Timeout",sec,msec);
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

	// The banner is plain format only.  What this command really produces
	// is the next result set, so in any other format the banner is two
	// stray lines at the top of the stream.
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

	stdoutput.printf("\n");
	stdoutput.printf("	To run a query, simply type it at the prompt,\n"
			"	followed by a semicolon.  Queries may be \n"
			"	split over multiple lines.\n\n");
	stdoutput.printf("	ping			- ");
	stdoutput.printf("pings the database\n");
	stdoutput.printf("	identify		- ");
	stdoutput.printf("returns the type of database\n");
	stdoutput.printf("	dbversion		- ");
	stdoutput.printf("returns the version of the database\n");
	stdoutput.printf("	dbhostname		- ");
	stdoutput.printf("returns the host name of the database\n");
	stdoutput.printf("	dbipaddress		- ");
	stdoutput.printf("returns the ip address of the database\n");
	stdoutput.printf("	clientversion		- ");
	stdoutput.printf("returns the version of the client library\n");
	stdoutput.printf("	serverversion		- ");
	stdoutput.printf("returns the version of the server\n");
	stdoutput.printf("	use [database]		- ");
	stdoutput.printf("change the current database/schema\n");
	stdoutput.printf("	currentdb		- ");
	stdoutput.printf("shows the current database/schema\n");
	stdoutput.printf("	run script		- ");
	stdoutput.printf("runs commands contained in file \"script\"\n");
	stdoutput.printf("	headers on|off		- ");
	stdoutput.printf("toggles column descriptions before result set\n");
	stdoutput.printf("	divider on|off		- ");
	stdoutput.printf("toggles the divider before the result set\n");
	stdoutput.printf("	stats on|off		- ");
	stdoutput.printf("toggles statistics after result set\n");
	stdoutput.printf("	format plain|csv	- ");
	stdoutput.printf("sets output format to plain or csv\n");
	stdoutput.printf("	debug on|off		- ");
	stdoutput.printf("toggles debug messages\n");
	stdoutput.printf("	nullsasnulls on|off	- ");
	stdoutput.printf("toggles getting nulls as nulls\n"
			"					"
			"(rather than as empty strings)\n");
	stdoutput.printf("	autocommit on|off	- ");
	stdoutput.printf("toggles autocommit\n");
	stdoutput.printf("	final on|off		- ");
	stdoutput.printf("toggles use of one session per query\n");
	stdoutput.printf("	delimiter [character]	- ");
	stdoutput.printf("sets delimiter character to [character]\n\n");
	stdoutput.printf("	response timeout [sec.msec]   - ");
	stdoutput.printf("sets response timeout to [sec.msec]\n\n");
	stdoutput.printf("	inputbind ...                 - ");
	stdoutput.printf("defines an input bind variable\n");
	stdoutput.printf("		inputbind [variable] is null\n");
	stdoutput.printf("		inputbind [variable] = [stringvalue]\n");
	stdoutput.printf("		inputbind [variable] = [integervalue]\n");
	stdoutput.printf("		inputbind [variable] = [doublevalue]\n");
	stdoutput.printf("		inputbind [variable] = [MM/DD/YYYY HH24:MM:SS:uS TZN]\n");
	stdoutput.printf("		inputbindblob [variable] = [value]\n");
	stdoutput.printf("	outputbind ...                 - ");
	stdoutput.printf("defines an output bind variable\n");
	stdoutput.printf("		outputbind [variable] string [length]\n");
	stdoutput.printf("		outputbind [variable] integer\n");
	stdoutput.printf("		outputbind [variable] double [precision] [scale}\n");
	stdoutput.printf("		outputbind [variable] date\n");
	stdoutput.printf("	printbinds                     - ");
	stdoutput.printf("prints all bind variables\n");
	stdoutput.printf("	clearinputbind [variable]      - ");
	stdoutput.printf("clears an input bind variable\n");
	stdoutput.printf("	clearoutputbind [variable]     - ");
	stdoutput.printf("clears an output bind variable\n");
	stdoutput.printf("	clearbinds                     - ");
	stdoutput.printf("clears all bind variables\n");
	stdoutput.printf("	reexecute                      - ");
	stdoutput.printf("reexecutes the previous query\n\n");
	stdoutput.printf("	lastinsertid                   - ");
	stdoutput.printf("returns the value of the most recently\n");
	stdoutput.printf("\t\t\t\t\t updated auto-increment or identity\n");
	stdoutput.printf("\t\t\t\t\t column, if the database supports it\n\n");
	stdoutput.printf("	show databases [like pattern]		-\n");
	stdoutput.printf("		returns a list of known databases/schemas\n");
	stdoutput.printf("	show tables [like pattern]		-\n");
	stdoutput.printf("		returns a list of known tables\n");
	stdoutput.printf("	show columns in table [like pattern]	-\n");
	stdoutput.printf("		returns a list of column metadata for the table \"table\"\n");
	stdoutput.printf("	describe table				-\n");
	stdoutput.printf("		returns a list of column metadata for the table \"table\"\n");
	stdoutput.printf("	fields table				-\n");
	stdoutput.printf("		returns a list of column names for the table \"table\"\n\n");
	stdoutput.printf("	setclientinfo info	- sets the client info\n");
	stdoutput.printf("	getclientinfo		- displays the client info\n\n");
	stdoutput.printf("	setresultsetbuffersize size	- fetch size rows at a time\n");
	stdoutput.printf("	getresultsetbuffersize 		- shows rows fetched at a time\n\n");
	stdoutput.printf("	endsession		- ends the current session\n\n");
	stdoutput.printf("	cache [filename] [ttl]	- caches the next result set to \"filename\"\n	                      	  with ttl of \"ttl\"\n");
	stdoutput.printf("	opencache [filename] 	- opens and displays cached result set \n				  in \"filename\"\n\n");
	stdoutput.printf("	exit/quit		- ");
	stdoutput.printf("exits\n\n");
	stdoutput.printf("	All commands must be followed by the delimiter: %c\n",
								env->delimiter);
}

void sqlrsh::startupMessage(sqlrshenv *env, const char *host,
					uint16_t port, const char *user) {

	// The banner greets a person.  json and jsonl are handed to a
	// parser, and a greeting isn't part of either, so it's left out of
	// the stream entirely rather than written and hoped over.
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
			// output goes to, so json and jsonl don't get one.
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

int32_t sqlrsh::execute(int argc, const char **argv) {

	cmdline=new sqlrcmdline(argc,argv);
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
	
	// at least id, host, or socket is required
	if (charstring::isNullOrEmpty(id) &&
		charstring::isNullOrEmpty(host) &&
		charstring::isNullOrEmpty(socket)) {

		stderror.printf("usage:\n"
			" %ssh -host host -port port -socket socket\n"
			"        [-user user] [-password password]\n"
			"        [-krb] [-krbservice svc] [-krbmech mech] "
			"[-krbflags flags]\n"
			"        [-tls] [-tlsversion version]\n"
			"        [-tlscert certfile] [-tlspassword password]\n"
			"        [-tlsciphers cipherlist]\n"
			"        [-tlsvalidate (no|ca|ca+domain|ca+host)] "
			"[-tlsca ca] [-tlsdepth depth]\n"
			"        [-script script | -command command]\n"
			"        [-quiet]\n"
			"        [-format (plain|csv)]\n"
			"        [-locale (env|name)]\n"
			"        [-getasnumber]\n"
			"        [-noelapsed]\n"
			"        [-nextresultset]\n"
			"        [-resultsetbuffersize rows]\n"
			"  or\n"
			" %ssh [-config config] -id id\n"
			"        [-script script | -command command]\n"
			"        [-quiet]\n"
			"        [-format (plain|csv)]\n"
			"        [-locale (env|name)]\n"
			"        [-getasnumber]\n"
			"        [-noelapsed]\n"
			"        [-nextresultset]\n"
			"        [-resultsetbuffersize rows]\n",
			SQLR,SQLR);
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
			if (!cmdline->getValue("tlsciphers")) {
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
				password=passwordvalue.detachTextValue();

				// decrypt the password if necessary
				passwordencryptionid=
					cfg->getDefaultPasswordEncryptionId();
				if (passwordencryptionid) {
					sqlrpwdencs	sqlrpe(
						sqlrpth,false,
						cfg->getPasswordEncryptions());
					sqlrpe.load();
					decryptedpassword=
						sqlrpe.
						getPasswordEncryptionById(
							passwordencryptionid)->
							decrypt(password);
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

	// set up an sqlrshenv
	sqlrshenv	env;

	// Quiet is shorthand for headers and stats.  It runs before them so
	// that an explicit -headers or -stats wins over the shorthand.
	if (cmdline->isFound("quiet")) {
		bool	quiet=onOffOption("quiet",false);
		env.headers=!quiet;
		env.stats=!quiet;
	}

	// handle the result set format
	// The name goes through the same list the format command uses.  An
	// unrecognized name used to quietly fall back to plain, so
	// "-format jsonl" on a build without jsonl emitted plain text and
	// said nothing.  It's a usage error now.
	if (cmdline->isFound("format")) {
		const char	*formatname=cmdline->getValue("format");
		if (!formatFromName(formatname,&env.format)) {
			badFormatName(formatname);
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
	env.getasnumber=onOffOption("getasnumber",env.getasnumber);
	env.noelapsed=onOffOption("noelapsed",env.noelapsed);
	env.nextresultset=onOffOption("nextresultset",env.nextresultset);

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
	// failed command, and the caller couldn't tell it from a bad query.
	// ping() opens the session, so this costs a round trip, not a session.
	if (!sqlrcon.ping()) {
		const char	*error=sqlrcon.errorMessage();
		if (charstring::isNullOrEmpty(error)) {
			error="The database is down.";
		}
		displayError(&env,NULL,error,sqlrcon.errorNumber());
		delete[] decryptedpassword;
		return SQLRSH_EXIT_CONNECTION;
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
	}

	int32_t	exitcode=SQLRSH_EXIT_SUCCESS;

	if (!charstring::isNullOrEmpty(script)) {
		// if a script was specified, run it
		exitcode=runScript(&sqlrcon,&sqlrcur,&env,script,true);
	} else if (!charstring::isNullOrEmpty(command)) {
		// if a command was specified, run it
		if (!runCommands(&sqlrcon,&sqlrcur,&env,command,NULL)) {
			exitcode=SQLRSH_EXIT_QUERY;
		}
	} else {
		// otherwise go into interactive mode
		// An interactive session isn't a batch, so a failed query at
		// the prompt isn't a failed run.  This always succeeds.
		startupMessage(&env,host,port,user);
		interactWithUser(&sqlrcon,&sqlrcur,&env);
	}

	// clean up
	pr.flushHistory();
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
		"Command options:\n"
		"	-script filename	Run the specified script which contains	commands\n"
		"				or queries that could otherwise be run at the\n"
		"				%s prompt.\n"
		"\n"
		"	-command \"commands\"	Run the provided string which contains commands\n"
		"				or queries that could otherwise be run at the\n"
		"				%s prompt.\n"
		"\n"
		"	-quiet			Omit headers and stats in output.\n"
		"\n"
		"	-format plain|csv	Format the output as specified.\n"
		"				Defaults to plain.\n"
		"\n"
		"	-locale env|locale_name	calls setlocale(LC_ALL, locale_name).\n"
		"				env means use LC variables.\n"
		"\n"
		"	-getasnumber		calls getFieldAs(Integer|Double) as appropriate\n"
		"\n"
		"	-noelapsed		do not print elapsed time\n"
		"\n"
		"	-nextresultset		attempt to fetch multiple resultsets\n"
		"\n"
		"	-resultsetbuffersize rows\n"
		"				Fetch result sets using the specified number of\n"
		"				rows at once.\n"
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
		"Non-interactive session, running query \"select * from mytable\" with csv output.\n"
		"\n"
		"	%s -id myinst -command \"select * from mytable\" -quiet -format csv\n"
		"\n",
		progname,SQL_RELAY,progname,progname,progname,progname,
		progname,progname,progname,progname,progname);
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

// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/permissions.h>
#include <rudiments/logger.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

class SQLRSERVER_DLLSPEC sqlrlogger_debug : public sqlrlogger {
	public:
		sqlrlogger_debug(domnode *parameters);
		~sqlrlogger_debug();

		bool	init(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon);
		bool	start(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrloglevel_t level,
					sqlrevent_t event,
					const char *info);
		bool	write(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrloglevel_t level,
					sqlrevent_t event,
					const char *info);
		bool	end(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrloglevel_t level,
					sqlrevent_t event);
	private:
		bool	openDebug();
		void	closeDebug();

		bool			logtostdout;
		bool			logtostderr;
		bool			logtosyslog;
		bool			logtofile;

		bool			loglistener;
		bool			logconnection;

		logger			*debuglogger;

		stdoutdestination	*stdoutdest;
		stderrdestination	*stderrdest;
		syslogdestination	*syslogdest;
		filedestination		*filedest;

		const char		*debugdir;
		mode_t			debugfileperms;

		const char		*processname;

		uint16_t		indent;
};

sqlrlogger_debug::sqlrlogger_debug(domnode *parameters) :
					sqlrlogger(parameters) {

	logtostdout=charstring::isYes(parameters->getAttributeValue("stdout"));
	logtostderr=charstring::isYes(parameters->getAttributeValue("stderr"));
	logtosyslog=charstring::isYes(parameters->getAttributeValue("syslog"));

	// If file="..." isn't specified at all, then enable it, if nothing
	// else was enabled.  Otherwise, only enable it if it's set to "yes".
	const char	*fileattr=parameters->getAttributeValue("file");
	if (charstring::isNullOrEmpty(fileattr)) {
		logtofile=!logtostdout && !logtostderr && !logtosyslog;
	} else {
		logtofile=charstring::isYes(
				parameters->getAttributeValue("file"));
	}

	const char	*permstring=parameters->getAttributeValue("perms");
	if (!charstring::getLength(permstring)) {
		permstring="rw-------";
	}

	loglistener=!charstring::isNo(
			parameters->getAttributeValue("listener"));
	logconnection=!charstring::isNo(
			parameters->getAttributeValue("connection"));

	debuglogger=NULL;

	stdoutdest=NULL;
	stderrdest=NULL;
	syslogdest=NULL;
	filedest=NULL;

	debugdir=NULL;
	debugfileperms=permissions::parsePermString(permstring);

	processname=NULL;

	indent=0;
}

sqlrlogger_debug::~sqlrlogger_debug() {
	closeDebug();
}

bool sqlrlogger_debug::init(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon) {

	closeDebug();

	// Log listener or connection.
	// Log both by default, but either can be disabled.
	if (sqlrl && !loglistener) {
		return true;
	}
	if (sqlrcon && !logconnection) {
		return true;
	}
	processname=(sqlrl)?("sqlr-listener"):("sqlr-connection");
	debugdir=(sqlrcon)?sqlrcon->cont->getPaths()->getDebugDir():
					sqlrl->getPaths()->getDebugDir();
	return true;
}

bool sqlrlogger_debug::start(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event,
				const char *info) {
	if (sqlrl && !loglistener) {
		return true;
	}
	if (sqlrcon && !logconnection) {
		return true;
	}
	if (!debuglogger && !openDebug()) {
		return false;
	}
	if (charstring::isNullOrEmpty(info)) {
		return true;
	}
	debuglogger->start(0,NULL,indent,info);
	indent++;
	return true;
}

bool sqlrlogger_debug::write(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event,
				const char *info) {
	if (sqlrl && !loglistener) {
		return true;
	}
	if (sqlrcon && !logconnection) {
		return true;
	}
	if (!debuglogger && !openDebug()) {
		return false;
	}
	if (charstring::isNullOrEmpty(info)) {
		return true;
	}
	debuglogger->write(0,NULL,indent,info);
	return true;
}

bool sqlrlogger_debug::end(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event) {
	if (sqlrl && !loglistener) {
		return true;
	}
	if (sqlrcon && !logconnection) {
		return true;
	}
	if (!debuglogger && !openDebug()) {
		return false;
	}
	indent--;
	debuglogger->end(0,(const char *)NULL,indent);
	return true;
}

bool sqlrlogger_debug::openDebug() {

	bool	retval=true;

	debuglogger=new logger();

	if (logtostdout) {
		stdoutdest=new stdoutdestination();
		stdoutput.printf("Debugging to: stdout\n");
		debuglogger->addLogDestination(stdoutdest);
	}
	
	if (logtostderr) {
		stderrdest=new stderrdestination();
		stdoutput.printf("Debugging to: stderr\n");
		debuglogger->addLogDestination(stderrdest);
	}
	
	if (logtosyslog) {
		syslogdest=new syslogdestination();
		stdoutput.printf("Debugging to: syslog\n");
		syslogdest->open(processname,LOG_CONS,LOG_USER,LOG_DEBUG);
		debuglogger->addLogDestination(syslogdest);
	}

	if (logtofile) {

		// build the debug file name
		char	*debugfilename;
		charstring::printf(&debugfilename,"%s/%s.%ld",
			debugdir,processname,(long)process::getProcessId());

		// open the file destination
		filedest=new filedestination();
		if (filedest->open(debugfilename,debugfileperms)) {
			stdoutput.printf("Debugging to: %s\n",debugfilename);
			debuglogger->addLogDestination(filedest);
		} else {
			stderror.printf("Couldn't open debug file: %s\n",
								debugfilename);
			filedest->close();
			delete filedest;
			filedest=NULL;
			retval=false;
		}

		// clean up
		delete[] debugfilename;
	}
	return retval;
}

void sqlrlogger_debug::closeDebug() {

	delete stdoutdest;
	stdoutdest=NULL;

	delete stderrdest;
	stderrdest=NULL;

	if (syslogdest) {
		syslogdest->close();
		delete syslogdest;
		syslogdest=NULL;
	}

	if (filedest) {
		filedest->close();
		delete filedest;
		filedest=NULL;
	}

	delete debuglogger;
	debuglogger=NULL;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrlogger *new_sqlrlogger_debug(
						domnode *parameters) {
		return new sqlrlogger_debug(parameters);
	}
}

// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/logger.h>
#include <rudiments/process.h>

class sqlrserverbaseprivate {
	friend class sqlrserverbase;
	private:
		logger			_lg;
		stdoutdestination	_sod;
		bool			_debug;
		uint16_t		_indent;
		stringbuffer		_logbuffer;
};

sqlrserverbase::sqlrserverbase() {
	pvt=new sqlrserverbaseprivate;
	pvt->_debug=false;
	pvt->_indent=0;
	pvt->_lg.setLogLevel(1);
	pvt->_lg.addLogDestination(&pvt->_sod);
}

sqlrserverbase::~sqlrserverbase() {
	delete pvt;
}

void sqlrserverbase::setDebug(bool debug) {
	pvt->_debug=debug;
}

bool sqlrserverbase::getDebug() {
	return pvt->_debug;
}

void sqlrserverbase::debugStart(const char *title, ...) {
	if (!pvt->_debug) {
		return;
	}
	pvt->_logbuffer.clear();
	if (!pvt->_indent) {
		pvt->_logbuffer.append(process::getProcessId());
		pvt->_logbuffer.append(": ");
	}
	va_list	argp;
	va_start(argp,title);
	pvt->_logbuffer.printf(title,&argp);
	va_end(argp);
	pvt->_lg.start(1,NULL,pvt->_indent,pvt->_logbuffer.getString());
	pvt->_indent++;
}

void sqlrserverbase::debugWrite(const char *string, ...) {
	if (!pvt->_debug) {
		return;
	}
	va_list	argp;
	va_start(argp,string);
	pvt->_lg.write(1,NULL,pvt->_indent,string,&argp);
	va_end(argp);
}

void sqlrserverbase::debugHexDump(const byte_t *data, uint64_t size) {
	if (!pvt->_debug) {
		return;
	}
	pvt->_logbuffer.clear();
	pvt->_logbuffer.printHex(data,size,0);
	pvt->_lg.write(1,NULL,0,pvt->_logbuffer.getString());
}

void sqlrserverbase::debugEnd() {
	if (!pvt->_debug) {
		return;
	}
	pvt->_indent--;
	pvt->_lg.end(1,(const char *)NULL,pvt->_indent);
}

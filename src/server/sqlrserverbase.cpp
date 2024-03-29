// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/stdio.h>
#include <rudiments/process.h>

class sqlrserverbaseprivate {
	friend class sqlrserverbase;
	private:
		bool	_debug;
};

sqlrserverbase::sqlrserverbase() {
	pvt=new sqlrserverbaseprivate;
	pvt->_debug=false;
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

void sqlrserverbase::debugStart(const char *title) {
	debugStart(title,0);
}

void sqlrserverbase::debugStart(const char *title, uint16_t indent) {
	if (pvt->_debug) {
		for (uint16_t i=0; i<indent; i++) {
			stdoutput.write('	');
		}
		if (!indent) {
			stdoutput.printf("%d: ",process::getProcessId());
		}
		stdoutput.write(title);
		stdoutput.write(" {\n");
	}
}

void sqlrserverbase::debugHexDump(const byte_t *data, uint64_t size) {
	debugHexDump(data,size,1);
}

void sqlrserverbase::debugHexDump(const byte_t *data,
						uint64_t size,
						uint16_t indent) {
	if (!pvt->_debug) {
		return;
	}
	stdoutput.printHex(data,size,indent);
}

void sqlrserverbase::debugEnd() {
	debugEnd(0);
}

void sqlrserverbase::debugEnd(uint16_t indent) {
	if (pvt->_debug) {
		for (uint16_t i=0; i<indent; i++) {
			stdoutput.write('	');
		}
		stdoutput.write("}\n");
	}
}

// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrparserprivate {
	friend class sqlrparser;
	private:
};

sqlrparser::sqlrparser(sqlrservercontroller *cont, domnode *parameters) :
					sqlrservermodule(cont,parameters) {
	pvt=new sqlrparserprivate;
}

sqlrparser::~sqlrparser() {
	delete pvt;
}

bool sqlrparser::parse(const char *query) {
	// by default, do nothing...
	return false;
}

void sqlrparser::setTree(xmldom *tree) {
	// by default, do nothing...
}

xmldom *sqlrparser::getTree() {
	// by default, do nothing...
	return NULL;
}

xmldom *sqlrparser::detachTree() {
	// by default, do nothing...
	return NULL;
}

bool sqlrparser::write(stringbuffer *output) {
	// by default, do nothing...
	return false;
}

bool sqlrparser::write(domnode *node,
				stringbuffer *output,
				bool omitsiblings) {
	// by default, do nothing...
	return false;
}

bool sqlrparser::write(domnode *node, stringbuffer *output) {
	// by default, do nothing...
	return false;
}

void sqlrparser::getMetaData(domnode *node) {
	// by default, do nothing...
}

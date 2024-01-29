// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexportfile.h>

sqlrexportfile::sqlrexportfile() : sqlrexport() {
	filename=NULL;
}

sqlrexportfile::~sqlrexportfile() {
}

void sqlrexportfile::setFileName(const char *filename) {
	this->filename=filename;
}

const char *sqlrexportfile::getFileName() {
	return filename;
}

void sqlrexportfile::setFileDescriptor(filedescriptor *fd) {
	this->fd=fd;
}

filedescriptor *sqlrexportfile::getFileDescriptor() {
	return fd;
}

bool sqlrexportfile::exportData() {
	clearOutput();
	return true;
}

void sqlrexportfile::clearOutput() {
	sqlrexport::clearOutput();
	fd=NULL;
}

// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportfile.h>

sqlrimportfile::sqlrimportfile() : sqlrimport() {
	filename=NULL;
	fd=NULL;
}

sqlrimportfile::~sqlrimportfile() {
}

void sqlrimportfile::setFileName(const char *filename) {
	this->filename=filename;
}

const char *sqlrimportfile::getFileName() {
	return filename;
}

void sqlrimportfile::setFileDescriptor(filedescriptor *fd) {
	this->fd=fd;
}

filedescriptor *sqlrimportfile::getFileDescriptor() {
	return fd;
}

bool sqlrimportfile::importData() {
	fd=NULL;
	return true;
}

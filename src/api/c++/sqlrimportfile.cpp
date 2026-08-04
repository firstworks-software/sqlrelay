// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportfile.h>

#include <rudiments/file.h>
#include <rudiments/stdio.h>

sqlrimportfile::sqlrimportfile() : sqlrimport() {
	filename=NULL;
	fd=NULL;
	extension=NULL;
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

void sqlrimportfile::setExtension(const char *extension) {
	this->extension=extension;
}

const char *sqlrimportfile::getExtension() {
	return extension;
}

bool sqlrimportfile::importData() {

	fd=NULL;

	// set the table name from the file name, if it wasn't already set
	if (!getObjectName()) {

		// there's no file name when importing from standard input
		if (charstring::isNullOrEmpty(filename)) {
			stderror.printf("no table or sequence name was "
					"given, and none could be derived, "
					"because the import is from "
					"standard input\n");
			return false;
		}

		char	*objectname=file::getBaseName(filename);

		// truncate the extension, but only if the file name has it
		// (case-insensitive, like the importer choice in sqlr-import)
		size_t	objectnamelen=charstring::getLength(objectname);
		size_t	extensionlen=charstring::getLength(extension);
		if (extensionlen && extensionlen<=objectnamelen &&
			!charstring::compareIgnoringCase(
					objectname+objectnamelen-extensionlen,
					extension)) {
			objectname[objectnamelen-extensionlen]='\0';
		}

		// the file name has to leave something behind
		if (charstring::isNullOrEmpty(objectname)) {
			stderror.printf("no table or sequence name was "
					"given, and none could be derived "
					"from the file name \"%s\"\n",filename);
			delete[] objectname;
			return false;
		}

		setObjectName(objectname);
		delete[] objectname;
	}

	return true;
}

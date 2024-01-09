// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORTXML_H
#define SQLREXPORTXML_H

#include <sqlrelay/private/sqlrexportxmlincludes.h>

/** The sqlrexportxml class implements sqlrexport for XML files. */
class SQLRCLIENT_DLLSPEC sqlrexportxml : public sqlrexport {
	public:
		/** Creates an instance of the sqlrexportxml class. */
		sqlrexportxml();

		/** Destroys this instance of the sqlrexportxml class. */
		~sqlrexportxml();

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the file
		 *  "filename" in XML format.
		 *
		 *  If "table" is non-null then the table name is embedded in
		 *  the XML file in the <table/> tag.
		 *
		 *  Returns true on success and false if an error occurred. */
		bool	exportToFile(const char *filename, const char *table);

	#include <sqlrelay/private/sqlrexportxml.h>
};

#endif

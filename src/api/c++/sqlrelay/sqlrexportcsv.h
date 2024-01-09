// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORTCSV_H
#define SQLREXPORTCSV_H

#include <sqlrelay/private/sqlrexportcsvincludes.h>

/** The sqlrexportcsv class implements sqlrexport for CSV files. */
class SQLRCLIENT_DLLSPEC sqlrexportcsv : public sqlrexport {
	public:
		/** Creates an instance of the sqlrexportcsv class. */
		sqlrexportcsv();

		/** Destroys this instance of the sqlrexportcsv class. */
		~sqlrexportcsv();

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to file
		 *  "filename" in CSV format.
		 *
		 *  The "table" argument is ignored in this implementation.
		 *
		 *  Returns true on success and false if an error occurred. */
		bool	exportToFile(const char *filename,
						const char *table);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  JSON domnode "jsondomnode".
		 *
		 *  The "table" argument is ignored in this implementation.
		 *
		 *  Returns true on success and false if an error occurred. */
		bool	exportToJsonDomNode(domnode *jsondomnode,
						const char *table);

	#include <sqlrelay/private/sqlrexportcsv.h>
};

#endif

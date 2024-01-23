// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORTTABLE_H
#define SQLREXPORTTABLE_H

#include <sqlrelay/private/sqlrexporttableincludes.h>

/** The sqlrexporttable class implements sqlrexport for database tables. */
class SQLRCLIENT_DLLSPEC sqlrexporttable : public sqlrexport {
	public:
		/** Creates an instance of the sqlrexporttable class. */
		sqlrexporttable();

		/** Destroys this instance of the sqlrexporttable class. */
		~sqlrexporttable();

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  database table "table" using "sqlrcon" and "sqlrcur".
		 *  A commit is called every "commitcount" rows.  No commit is
		 *  called if "commitcount" is set to 0.
		 *
		 *  Returns true on success and false if an error occurred. */
		bool	exportToTable(sqlrconnection *sqlrcon,
						sqlrcursor *sqlrcur,
						const char *table,
						uint64_t commitcount);
};

#endif

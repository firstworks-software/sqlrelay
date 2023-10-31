// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORTTABLE_H
#define SQLREXPORTTABLE_H

#include <sqlrelay/private/sqlrexporttableincludes.h>

class SQLRCLIENT_DLLSPEC sqlrexporttable : public sqlrexport {
	public:
			sqlrexporttable();
			~sqlrexporttable();

		bool	exportToTable(sqlrconnection *sqlrcon,
						const char *table);
};

#endif

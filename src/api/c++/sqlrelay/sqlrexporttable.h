// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORTTABLE_H
#define SQLREXPORTTABLE_H

#include <sqlrelay/private/sqlrexporttableincludes.h>

class SQLRCLIENT_DLLSPEC sqlrexporttable : public sqlrexport {
	public:
			sqlrexporttable();
			~sqlrexporttable();

		void	setCommitCount(uint64_t commitcount);
		uint64_t	getCommitCount();

		bool	exportToTable(sqlrconnection *sqlrcon,
						const char *table);

	#include <sqlrelay/private/sqlrexporttable.h>
};

#endif

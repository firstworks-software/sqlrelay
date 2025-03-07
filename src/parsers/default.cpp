// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

extern "C" {
	SQLRSERVER_DLLSPEC sqlrparser *new_sqlrparser_default(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrparser(cont,parameters);
	}
}

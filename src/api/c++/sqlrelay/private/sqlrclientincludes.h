// Copyright (c) David Muse
// See the COPYING file for more information.

#include <rudiments/object.h>

#include <rudiments/dynamicarray.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/socketclient.h>

#ifndef SQLRCLIENT_DLLSPEC
	#ifdef _WIN32
		#ifdef SQLRCLIENT_EXPORTS
			#define SQLRCLIENT_DLLSPEC __declspec(dllexport)
		#else
			#define SQLRCLIENT_DLLSPEC __declspec(dllimport)
		#endif
	#else
		#define SQLRCLIENT_DLLSPEC
	#endif
#endif

enum sqlrclientbindvartype_t {
	SQLRCLIENTBINDVARTYPE_NULL=0,
	SQLRCLIENTBINDVARTYPE_STRING,
	SQLRCLIENTBINDVARTYPE_INTEGER,
	SQLRCLIENTBINDVARTYPE_DOUBLE,
	SQLRCLIENTBINDVARTYPE_BLOB,
	SQLRCLIENTBINDVARTYPE_CLOB,
	SQLRCLIENTBINDVARTYPE_CURSOR,
	SQLRCLIENTBINDVARTYPE_DATE,

	// These values are sent over the wire as-is and must match the
	// corresponding sqlrserverbindvartype_t values.  8 and 9 are
	// skipped because they are server-side bulk-load-only types
	// (DELIMITER and NEWLINE) that the client never sends.
	SQLRCLIENTBINDVARTYPE_ARRAY=10,

	// A null lob bind keeps its lob-ness rather than collapsing to
	// NULL, so the server can route it to the connection module's
	// inputBindBlob()/inputBindClob() rather than to its plain
	// character inputBind().  Only a protocol version 4 or newer
	// client sends these.
	SQLRCLIENTBINDVARTYPE_NULLBLOB=11,
	SQLRCLIENTBINDVARTYPE_NULLCLOB=12
};

enum sqlrclientlistformat_t {
	SQLRCLIENTLISTFORMAT_NULL=0,
	SQLRCLIENTLISTFORMAT_MYSQL,
	SQLRCLIENTLISTFORMAT_POSTGRESQL,
	SQLRCLIENTLISTFORMAT_ODBC,
	SQLRCLIENTLISTFORMAT_JDBC
};

enum sqlrclientisolationlevelformat_t {
	SQLRCLIENTISOLATIONLEVELFORMAT_NULL=0,
	SQLRCLIENTISOLATIONLEVELFORMAT_NATIVE,
	SQLRCLIENTISOLATIONLEVELFORMAT_ODBC,
	SQLRCLIENTISOLATIONLEVELFORMAT_JDBC
};

class sqlrconnectionprivate;
class sqlrcursor;
class sqlrcursorprivate;
class sqlrclientcolumn;
class sqlrclientbindvar;

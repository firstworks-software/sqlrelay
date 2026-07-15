/* Copyright (c) 2000  Adam Kropielnicki
   See the file COPYING for more information */

// Some versions of PHP 7.0 need INT64_MIN/MAX and (U)INT*_C to be defined.
// These gyrations are necessary when using C++.
#include <rudiments/private/config.h>
#ifdef RUDIMENTS_HAVE_STDINT_H
	#define __STDC_LIMIT_MACROS
	#define __STDC_CONSTANT_MACROS
	#include <stdint.h>
#endif

#include <config.h>

#include <sqlrelay/sqlrclient.h>

// The various define/undef games below play havoc with inttypes.h
// on some platforms (openbsd 5.7, for example).  Including it here
// prevents it from being included later after the games.
// We'll borrow a macro from rudiments to detect it's existence.
#ifdef RUDIMENTS_HAVE_INTTYPES_H
	#include <inttypes.h>
#endif

#ifdef WIN32
	#undef uid_t
	#undef gid_t
	#undef ssize_t
	#undef socklen_t
	#undef pid_t
	#undef mode_t
	#define PHP_WIN32
	#define ZEND_WIN32
	#define ZEND_DEBUG 0
	#define ZTS 1
#endif

extern "C" {
	#ifndef WIN32

		// On some platforms (solaris), stdio.h must be included prior
		// to including math.h or FILE will get redefined.
		#include <stdio.h>

		// php.h ultimately includes math.h and on some platforms,
		// __cplusplus to be defined when including it.  Manually
		// including it prior to including php.h solves this problem.
		#include <math.h>

		#ifdef __cplusplus
			#if PHPMAJORVERSION < 8
				#undef __cplusplus
				#define cpluspluswasdefined
			#endif
		#endif
		#ifndef HAVE_SOCKLEN_T
			#define HAVE_SOCKLEN_T
		#endif
		#ifndef _WCHAR_T_DECLARED
			#define _WCHAR_T_DECLARED
		#endif
		#ifndef _WCHAR_T_DEFINED_
			#define _WCHAR_T_DEFINED_
		#endif
	#endif
	// On some versions of Mac OS X (10.4), php.h ultimately includes
	// dyld.h, but somehow _Bool avoids getting defined by stdbool.h
	// Define it here.
	#ifdef RUDIMENTS_HAVE_MACH_O_DYLD_H
		typedef int     _Bool;
	#endif
	#include <php.h>
	#ifndef WIN32
		#ifdef cpluspluswasdefined
			#define __cplusplus
		#endif
	#endif
}

#include <config.h>

#if PHP_MAJOR_VERSION >= 7

	#define ZVAL zval*

	#define zend_rsrc_list_entry zend_resource
	#define ZEND_REGISTER_RESOURCE(a,b,c) \
			RETURN_RES(zend_register_resource(b,c))
	#define ZEND_FETCH_RESOURCE(a,b,c,d,e,f) \
		if ((a=(b)zend_fetch_resource( \
			Z_RES_P((zval *)c),e,f))==NULL) { \
			RETURN_FALSE; \
		}
	#define ZEND_LIST_DELETE(a) zend_list_delete(Z_RES_P(a));

	#define GET_PARAMETERS zend_parse_parameters
	#define PARAMS(a) a,

	#define SVAL(a) Z_STRVAL_P(a)
	#define LVAL(a) Z_LVAL_P(a)
	#define DVAL(a) Z_DVAL_P(a)
	#define ARRVAL(a) Z_ARRVAL_P(a)
	#define TYPE(a) Z_TYPE_P(a)

	#define RET_STRING(a,b) \
		RETURN_STR(zend_string_init(a,charstring::getLength(a),0))
	#define RET_STRINGL(a,b,c) \
		RETURN_STR(zend_string_init(a,b,0))


	#define ADD_ASSOC_STRINGL(a,b,c,d,e) \
		add_assoc_stringl(a,b,zend_string_init(c,d,0)->val,d)
	#define ADD_NEXT_INDEX_STRING(a,b,c) \
		add_next_index_string( \
			a,zend_string_init(b,charstring::getLength(b),0)->val)
	#define ADD_NEXT_INDEX_STRINGL(a,b,c,d) \
		add_next_index_stringl(a,zend_string_init(b,c,0)->val,c)

	#define HASH_INDEX_FIND(a,b,c) c=zend_hash_index_find(a,b)

	// for 7.2 and greater...
	#if PHP_MAJOR_VERSION > 7 || PHP_MINOR_VERSION > 2
		#define ARRAY_INIT_CANT_FAIL 1
	#endif

	// for earlier than 7.4...
	#if PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION < 4
		#define ADD_ASSOC_NULL(a,b) add_assoc_unset(a,b)
		#define	ADD_NEXT_INDEX_NULL(a) add_next_index_unset(a)
	#else
		#define ADD_ASSOC_NULL(a,b) add_assoc_null(a,b)
		#define	ADD_NEXT_INDEX_NULL(a) add_next_index_null(a)
	#endif

#else

	#define ZVAL zval**

	#define ZEND_LIST_DELETE(a) zend_list_delete(LVAL(a));

	#define GET_PARAMETERS zend_get_parameters_ex
	#define PARAMS(a)

	// apparently, sufficiently old PHP doesn't support Z_*VAL(a)...
	#define SVAL(a) (*a)->value.str.val
	#define LVAL(a) (*a)->value.lval
	#define DVAL(a) (*a)->value.dval
	#define ARRVAL(a) (*a)->value.ht
	#define TYPE(a) Z_TYPE_PP(a)

	#define RET_STRING RETURN_STRING
	#define RET_STRINGL RETURN_STRINGL

	#define ADD_ASSOC_STRINGL(a,b,c,d,e) add_assoc_stringl(a,b,c,d,e)
	#define ADD_ASSOC_NULL(a,b) add_assoc_unset(a,b)
	#define ADD_NEXT_INDEX_STRING(a,b,c) add_next_index_string(a,b,c)
	#define ADD_NEXT_INDEX_STRINGL(a,b,c,d) add_next_index_stringl(a,b,c,d)
	#define	ADD_NEXT_INDEX_NULL(a) add_next_index_unset(a)

	#define HASH_INDEX_FIND(a,b,c) zend_hash_index_find(a,b,(void **)&c)
#endif

#if PHP_MAJOR_VERSION >= 5
	#define ARGINFO(a) a
#else
	#define ARGINFO(a) NULL
	#define ZEND_BEGIN_ARG_INFO_EX(a,b,c,d)
	#define ZEND_END_ARG_INFO()
#endif

// old enough versions of PHP don't support TSRMLS macros
#ifndef TSRMLS_DC
	#define TSRMLS_DC
#endif
#ifndef TSRMLS_CC
	#define TSRMLS_CC
#endif

extern "C" {

#ifdef _WIN32
#include <windows.h>
#define DLEXPORT __declspec(dllexport)
#else
#define DLEXPORT
#endif

static int sqlrelay_connection;
static int sqlrelay_cursor;

#ifdef ZEND_MODULE_STARTUP_D
static void sqlrcon_cleanup(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	sqlrconnection	*connection=(sqlrconnection *)rsrc->ptr;
	delete connection;
}

static void sqlrcur_cleanup(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	sqlrcursor	*cursor=(sqlrcursor *)rsrc->ptr;
	delete cursor;
}

ZEND_MODULE_STARTUP_D(sqlrelay) {
	sqlrelay_connection=zend_register_list_destructors_ex(sqlrcon_cleanup,
				NULL,"sqlrelay connection",module_number);
	sqlrelay_cursor=zend_register_list_destructors_ex(sqlrcur_cleanup,
				NULL,"sqlrelay cursor",module_number);
	return SUCCESS;
}
#endif

/**
 *  call-seq:
 *  sqlrcon_alloc($server, $port, $socket, $user, $password, $retrytime, $tries)
 *
 *  Initiates a connection to "server" on "port" or to the unix "socket" on
 *  the local machine and auths with "user" and "password".  Failed
 *  connections will be retried for "tries" times, waiting "retrytime" seconds
 *  between each try.  If "tries" is 0 then retries will continue forever.  If
 *  "retrytime" is 0 then retries will be attempted on a default interval.
 *
 *  If "server" is a comma-separated list of hosts, then an attempt will be
 *  made to connect to each until the attempt succeeds, or there are no more
 *  hosts left to try.
 *
 *  If the "socket" parameter is neither NULL nor "" then an attempt will be
 *  made to connect through it before attempting to connect to "server" on
 *  "port".
 *  If it is NULL or "" then no attempt will be made to connect through the
 *  socket. */
DLEXPORT ZEND_FUNCTION(sqlrcon_alloc) {
	ZVAL server;
	ZVAL port;
	ZVAL socket;
	ZVAL user;
	ZVAL password;
	ZVAL retrytime;
	ZVAL tries;
	sqlrconnection *connection=NULL;
	if (ZEND_NUM_ARGS() != 7 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzzz")
				&server,
				&port,
				&socket,
				&user,
				&password,
				&retrytime,
				&tries) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(server);
	convert_to_long_ex(port);
	convert_to_string_ex(socket);
	convert_to_string_ex(user);
	convert_to_string_ex(password);
	convert_to_long_ex(retrytime);
	convert_to_long_ex(tries);
	connection=new sqlrconnection(
			SVAL(server),
			LVAL(port),
			SVAL(socket),
			SVAL(user),
			SVAL(password),
			LVAL(retrytime),
			LVAL(tries),
			true);
	connection->debugPrintFunction((int (*)(const char *,...))zend_printf);
	ZEND_REGISTER_RESOURCE(return_value,connection,sqlrelay_connection);
}

/**
 *  call-seq:
 *  sqlrcon_free($sqlrconref)
 *
 *  Disconnects and ends the session if it hasn't been ended already. */
DLEXPORT ZEND_FUNCTION(sqlrcon_free) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	ZEND_LIST_DELETE(sqlrcon);
}

/**
 *  call-seq:
 *  sqlrcon_setConnectTimeout($sqlrconref, $timeoutsec, $timeoutusec)
 *
 *  Sets the server connect timeout in seconds and
 *  microseconds.  Setting either parameter to -1 disables the
 *  timeout.  You can also set this timeout using the
 *  SQLR_CLIENT_CONNECT_TIMEOUT environment variable. */
DLEXPORT ZEND_FUNCTION(sqlrcon_setconnecttimeout) {
	ZVAL sqlrcon;
	ZVAL timeoutsec;
	ZVAL timeoutusec;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcon,
				&timeoutsec,
				&timeoutusec) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(timeoutsec);
	convert_to_long_ex(timeoutusec);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->setConnectTimeout(LVAL(timeoutsec),
						LVAL(timeoutusec));
	}
}

/**
 *  call-seq:
 *  sqlrcon_getConnectTimeoutSeconds($sqlrconref)
 *
 *  Gets the server connect timeout in seconds. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getconnecttimeoutseconds) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		RETURN_LONG(connection->getConnectTimeoutSeconds());
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getConnectTimeoutMicroseconds($sqlrconref)
 *
 *  Gets the server connect timeout in microseconds. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getconnecttimeoutmicroseconds) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		RETURN_LONG(connection->getConnectTimeoutMicroseconds());
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_setResponseTimeout($sqlrconref, $timeoutsec, $timeoutusec)
 *
 *  Sets the response timeout (for queries, commits, rollbacks,
 *  pings, etc.) in seconds and microseconds.  Setting either
 *  parameter to -1 disables the timeout.  You can also set
 *  this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
 *  environment variable. */
DLEXPORT ZEND_FUNCTION(sqlrcon_setresponsetimeout) {
	ZVAL sqlrcon;
	ZVAL timeoutsec;
	ZVAL timeoutusec;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcon,
				&timeoutsec,
				&timeoutusec) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(timeoutsec);
	convert_to_long_ex(timeoutusec);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->setResponseTimeout(LVAL(timeoutsec),
						LVAL(timeoutusec));
	}
}

/**
 *  call-seq:
 *  sqlrcon_getResponseTimeoutSeconds($sqlrconref)
 *
 *  Gets the response timeout in seconds. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getresponsetimeoutseconds) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		RETURN_LONG(connection->getResponseTimeoutSeconds());
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getResponseTimeoutMicroseconds($sqlrconref)
 *
 *  Gets the response timeout in microseconds. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getresponsetimeoutmicroseconds) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		RETURN_LONG(connection->getResponseTimeoutMicroseconds());
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_setBindVariableDelimiters($sqlrconref, $delimiters)
 *
 *  Sets which delimiters are used to identify bind variables
 *  in countBindVariables() and validateBinds().  Valid
 *  delimiters include ?,:,@, and $.  Defaults to "?:@$" */
DLEXPORT ZEND_FUNCTION(sqlrcon_setbindvariabledelimiters) {
	ZVAL sqlrcon;
	ZVAL delimiters;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&delimiters) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(delimiters);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->setBindVariableDelimiters(SVAL(delimiters));
	}
}

/**
 *  call-seq:
 *  sqlrcon_getBindVariableDelimiterQuestionMarkSupported($sqlrconref)
 *
 *  Returns true if question marks (?) are considered to be
 *  valid bind variable delimiters. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getbindvariabledelimiterquestionmarksupported) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getBindVariableDelimiterQuestionMarkSupported();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getBindVariableDelimiterColonSupported($sqlrconref)
 *
 *  Returns true if colons (:) are considered to be
 *  valid bind variable delimiters. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getbindvariabledelimitercolonsupported) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getBindVariableDelimiterColonSupported();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getBindVariableDelimiterAtSignSupported($sqlrconref)
 *
 *  Returns true if at-signs (@) are considered to be
 *  valid bind variable delimiters. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getbindvariabledelimiteratsignsupported) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getBindVariableDelimiterAtSignSupported();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getBindVariableDelimiterDollarSignSupported($sqlrconref)
 *
 *  Returns true if dollar signs ($) are considered to be
 *  valid bind variable delimiters. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getbindvariabledelimiterdollarsignsupported) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getBindVariableDelimiterDollarSignSupported();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_enableKerberos($sqlrconref, $service, $mech, $flags)
 *
 *  Enables Kerberos authentication and encryption.
 *
 *  "service" indicates the Kerberos service name of the
 *  SQL Relay server.  If left empty or NULL then the service
 *  name "sqlrelay" will be used. "sqlrelay" is the default
 *  service name of the SQL Relay server.  Note that on Windows
 *  platforms the service name must be fully qualified,
 *  including the host and realm name.  For example:
 *  "sqlrelay/sqlrserver.firstworks.com@AD.FIRSTWORKS.COM".
 *
 *  "mech" indicates the specific Kerberos mechanism to use.
 *  On Linux/Unix platforms, this should be a string
 *  representation of the mechnaism's OID, such as:
 *      { 1 2 840 113554 1 2 2 }
 *  On Windows platforms, this should be a string like:
 *      Kerberos
 *  If left empty or NULL then the default mechanism will be
 *  used.  Only set this if you know that you have a good
 *  reason to.
 *
 *  "flags" indicates what Kerberos flags to use.  Multiple
 *  flags may be specified, separated by commas.  If left
 *  empty or NULL then a defalt set of flags will be used.
 *  Only set this if you know that you have a good reason to.
 *
 *  Valid flags include:
 *   * GSS_C_MUTUAL_FLAG
 *   * GSS_C_REPLAY_FLAG
 *   * GSS_C_SEQUENCE_FLAG
 *   * GSS_C_CONF_FLAG
 *   * GSS_C_INTEG_FLAG
 *
 *  For a full list of flags, consult the GSSAPI documentation,
 *  though note that only the flags listed above are supported
 *  on Windows. */
DLEXPORT ZEND_FUNCTION(sqlrcon_enablekerberos) {
	ZVAL sqlrcon;
	ZVAL service;
	ZVAL mech;
	ZVAL flags;
	if (ZEND_NUM_ARGS() != 4 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzz")
				&sqlrcon,
				&service,
				&mech,
				&flags) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(service);
	convert_to_string_ex(mech);
	convert_to_string_ex(flags);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->enableKerberos(SVAL(service),
						SVAL(mech),
						SVAL(flags));
	}
}

/**
 *  call-seq:
 *  sqlrcon_enableTls($sqlrconref, $version, $cert, $password, $ciphers, $validate, $ca, $depth)
 *
 *  Enables TLS/SSL encryption, and optionally authentication.
 *
 *  "version" specifies the TLS/SSL protocol version that the
 *  client will attempt to use.  Valid values include SSL2,
 *  SSL3, TLS1, TLS1.1, TLS1.2 or any more recent version of
 *  TLS, as supported by and enabled in the underlying TLS/SSL
 *  library.  If left blank or empty then the highest supported
 *  version will be negotiated.
 *
 *  "cert" is the file name of the certificate chain file to
 *  send to the SQL Relay server.  This is only necessary if
 *  the SQL Relay server is configured to authenticate and
 *  authorize clients by certificate.
 *
 *  If "cert" contains a password-protected private key, then
 *  "password" may be supplied to access it.  If the private
 *  key is not password-protected, then this argument is
 *  ignored, and may be left empty or NULL.
 *
 *  "ciphers" is a list of ciphers to allow.  Ciphers may be
 *  separated by spaces, commas, or colons.  If "ciphers" is
 *  empty or NULL then a default set is used.  Only set this if
 *  you know that you have a good reason to.
 *
 *  For a list of valid ciphers on Linux/Unix platforms, see:
 *      man ciphers
 *
 *  For a list of valid ciphers on Windows platforms, see:
 *      https://msdn.microsoft.com/en-us/library/windows/desktop/aa375549%28v=vs.85%29.aspx
 *  On Windows platforms, the ciphers (alg_id's) should omit
 *  CALG_ and may be given with underscores or dashes.
 *  For example: 3DES_112
 *
 *  "validate" indicates whether to validate the SQL Relay's
 *  server certificate, and may be set to one of the following:
 *      "no" - Don't validate the server's certificate.
 *      "ca" - Validate that the server's certificate was
 *             signed by a trusted certificate authority.
 *      "ca+host" - Perform "ca" validation and also validate
 *             that one of the subject altenate names (or the
 *             common name if no SANs are present) in the
 *             certificate matches the host parameter.
 *             (Falls back to "ca" validation when a unix
 *             socket is used.)
 *      "ca+domain" - Perform "ca" validation and also validate
 *             that the domain name of one of the subject
 *             alternate names (or the common name if no SANs
 *             are present) in the certificate matches the
 *             domain name of the host parameter.  (Falls back
 *             to "ca" validation when a unix socket is used.)
 *
 *  "ca" is the location of a certificate authority file to
 *  use, in addition to the system's root certificates, when
 *  validating the SQL Relay server's certificate.  This is
 *  useful if the SQL Relay server's certificate is self-signed.
 *
 *  On Windows, "ca" must be a file name.
 *
 *  On non-Windows systems, "ca" can be either a file or
 *  directory name.  If it is a directory name, then all
 *  certificate authority files found in that directory will be
 *  used.  If it a file name, then only that file will be used.
 *
 *
 *  Note that the supported "cert" and "ca" file formats may
 *  vary between platforms.  A variety of file formats are
 *  generally supported on Linux/Unix platfoms (.pem, .pfx,
 *  etc.) but only the .pfx format is currently supported on
 *  Windows. */
DLEXPORT ZEND_FUNCTION(sqlrcon_enabletls) {
	ZVAL sqlrcon;
	ZVAL version;
	ZVAL cert;
	ZVAL password;
	ZVAL ciphers;
	ZVAL validate;
	ZVAL ca;
	ZVAL depth;
	if (ZEND_NUM_ARGS() != 8 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzzzz")
				&sqlrcon,
				&version,
				&cert,
				&password,
				&ciphers,
				&validate,
				&ca,
				&depth) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(version);
	convert_to_string_ex(cert);
	convert_to_string_ex(password);
	convert_to_string_ex(ciphers);
	convert_to_string_ex(validate);
	convert_to_string_ex(ca);
	convert_to_long_ex(depth);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->enableTls(SVAL(version),
					SVAL(cert),
					SVAL(password),
					SVAL(ciphers),
					SVAL(validate),
					SVAL(ca),
					LVAL(depth));
	}
}

/**
 *  call-seq:
 *  sqlrcon_disableEncryption($sqlrconref)
 *
 *  Disables encryption. */
DLEXPORT ZEND_FUNCTION(sqlrcon_disableencryption) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->disableEncryption();
	}
}

/**
 *  call-seq:
 *  sqlrcon_endSession($sqlrconref)
 *
 *  Ends the session. */
DLEXPORT ZEND_FUNCTION(sqlrcon_endsession) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->endSession();
	}
}

/**
 *  call-seq:
 *  sqlrcon_suspendSession($sqlrconref)
 *
 *  Disconnects this connection from the current
 *  session but leaves the session open so
 *  that another connection can connect to it
 *  using resumeSession(). */
DLEXPORT ZEND_FUNCTION(sqlrcon_suspendsession) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->suspendSession();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getConnectionPort($sqlrconref)
 *
 *  Returns the inet port that the connection is communicating over.  This
 *  parameter may be passed to another connection for use in the
 *  resumeSession() method.  Note: The value this method returns
 *  is only valid after a call to suspendSession(). */
DLEXPORT ZEND_FUNCTION(sqlrcon_getconnectionport) {
	ZVAL sqlrcon;
	uint16_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getConnectionPort();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getConnectionSocket($sqlrconref)
 *
 *  Returns the unix socket that the connection is communicating over.  This
 *  parameter may be passed to another connection for use in the
 *  resumeSession() method.  Note: The value this method returns
 *  is only valid after a call to suspendSession(). */
DLEXPORT ZEND_FUNCTION(sqlrcon_getconnectionsocket) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getConnectionSocket();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_resumeSession($sqlrconref, $port, $socket)
 *
 *  Resumes a session previously left open
 *  using suspendSession().
 *  Returns 1 on success and 0 on failure. */
DLEXPORT ZEND_FUNCTION(sqlrcon_resumesession) {
	ZVAL sqlrcon;
	ZVAL port;
	ZVAL socket;
	bool r;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcon,
				&port,
				&socket) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(port);
	convert_to_string_ex(socket);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->resumeSession(LVAL(port),
						SVAL(socket));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_ping($sqlrconref)
 *
 *  Returns 1 if the database is up and 0 if it's down. */
DLEXPORT ZEND_FUNCTION(sqlrcon_ping) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->ping();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_identify($sqlrconref)
 *
 *  Returns the type of database: oracle, postgresql, mysql, etc. */
DLEXPORT ZEND_FUNCTION(sqlrcon_identify) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->identify();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_dbVersion($sqlrconref)
 *
 *  Returns the version of the database */
DLEXPORT ZEND_FUNCTION(sqlrcon_dbversion) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->dbVersion();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_dbHostName($sqlrconref)
 *
 *  Returns the host name of the database */
DLEXPORT ZEND_FUNCTION(sqlrcon_dbhostname) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->dbHostName();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_dbIpAddress($sqlrconref)
 *
 *  Returns the ip address of the database */
DLEXPORT ZEND_FUNCTION(sqlrcon_dbipaddress) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->dbIpAddress();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_serverVersion($sqlrconref)
 *
 *  Returns the version of the sqlrelay server software. */
DLEXPORT ZEND_FUNCTION(sqlrcon_serverversion) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->serverVersion();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_clientVersion($sqlrconref)
 *
 *  Returns the version of the sqlrelay client software. */
DLEXPORT ZEND_FUNCTION(sqlrcon_clientversion) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->clientVersion();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_bindFormat($sqlrconref)
 *
 *  Returns a string representing the bind variable format used
 *  by the database.  For example:
 *
 *  ?  - database uses a ? to represent a bind variable
 *  @* - database uses a @ followed by any characters to
 *       represent a bind variable
 *  $1 - database uses a $ followed by a number to represent a
 *       bind variable
 *  :* - database uses a : followed by any characters to
 *       represent a bind variable */
DLEXPORT ZEND_FUNCTION(sqlrcon_bindformat) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->bindFormat();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_nextvalFormat($sqlrconref)
 *
 *  Returns a string representing the format of the sequence
 *  nextval command used in the database.  The format will
 *  contain a %s in place of the sequence name.  For example:
 *
 *  (nextval for %s)
 *  next value for %s
 *  nextval('%s')
 *  %s.nextval
 *
 *  Returns an empty string if the database does not support
 *  sequences. */
DLEXPORT ZEND_FUNCTION(sqlrcon_nextvalformat) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->nextvalFormat();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_selectDatabase($sqlrconref, $database)
 *
 *  Sets the current database to "database".
 *
 *  May set the current catalog or schema, depending on
 *  whether the backend database equates "database" with
 *  catalog or schema.
 *
 *  See getDatabaseIsSchema(). */
DLEXPORT ZEND_FUNCTION(sqlrcon_selectdatabase) {
	ZVAL sqlrcon;
	ZVAL database;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&database) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(database);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->selectDatabase(SVAL(database));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getCurrentDatabase($sqlrconref)
 *
 *  Returns the database that is currently in use.
 *
 *  May return the current catalog or schema, depending on
 *  whether the backend database equates "database" with
 *  catalog or schema.
 *
 *  See getDatabaseIsSchema(). */
DLEXPORT ZEND_FUNCTION(sqlrcon_getcurrentdatabase) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getCurrentDatabase();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_selectCatalog($sqlrconref, $catalog)
 *
 *  Sets the current catalog to "catalog" */
DLEXPORT ZEND_FUNCTION(sqlrcon_selectcatalog) {
	ZVAL sqlrcon;
	ZVAL catalog;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&catalog) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(catalog);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->selectCatalog(SVAL(catalog));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getCurrentCatalog($sqlrconref)
 *
 *  Returns the catalog that is currently in use. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getcurrentcatalog) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getCurrentCatalog();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_selectSchema($sqlrconref, $schema)
 *
 *  Sets the current schema to "schema" */
DLEXPORT ZEND_FUNCTION(sqlrcon_selectschema) {
	ZVAL sqlrcon;
	ZVAL schema;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&schema) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(schema);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->selectSchema(SVAL(schema));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getCurrentSchema($sqlrconref)
 *
 *  Returns the schema that is currently in use. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getcurrentschema) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getCurrentSchema();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_getDatabaseIsSchema($sqlrconref)
 *
 *  Returns true if the backend database equates "database" with
 *  "schema", and false if it equates "database" with "catalog". */
DLEXPORT ZEND_FUNCTION(sqlrcon_getdatabaseisschema) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		RETURN_LONG(connection->getDatabaseIsSchema());
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getCurrentUser($sqlrconref)
 *
 *  Returns the user that sqlrelay is currently logged in to
 *  the database as, or NULL if no user could be determined
 *  or if an error occurred. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getcurrentuser) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getCurrentUser();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_getLastInsertId($sqlrconref)
 *
 *  Returns the value of the autoincrement column for the last insert */
DLEXPORT ZEND_FUNCTION(sqlrcon_getlastinsertid) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		RETURN_LONG(connection->getLastInsertId());
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_autoCommitOn($sqlrconref)
 *
 *  Instructs the database to perform a commit after every successful query. */
DLEXPORT ZEND_FUNCTION(sqlrcon_autocommiton) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->autoCommitOn();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_autoCommitOff($sqlrconref)
 *
 *  Instructs the database to wait for the client to tell it when to commit. */
DLEXPORT ZEND_FUNCTION(sqlrcon_autocommitoff) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->autoCommitOff();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getAutoCommit($sqlrconref)
 *
 *  Returns 1 if auto-commit is currently on, 0 otherwise. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getautocommit) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getAutoCommit();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_begin($sqlrconref)
 *
 *  Begins a transaction.  Returns 1 if the begin succeeded, 0 if it failed.
 *  If the database automatically begins a new transaction when a commit or
 *  rollback is issued then this doesn't do anything unless SQL Relay is faking
 *  transaction blocks. */
DLEXPORT ZEND_FUNCTION(sqlrcon_begin) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->begin();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_commit($sqlrconref)
 *
 *  Commits a transaction.  Returns 1 if the commit succeeded, 0 if it
 *  failed. */
DLEXPORT ZEND_FUNCTION(sqlrcon_commit) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->commit();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_rollback($sqlrconref)
 *
 *  Rolls back a transaction.  Returns 1 if the rollback succeeded, 0 if it
 *  failed. */
DLEXPORT ZEND_FUNCTION(sqlrcon_rollback) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->rollback();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getInTransaction($sqlrconref)
 *
 *  Returns 1 if the session is currently inside a transaction,
 *  0 otherwise. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getInTransaction) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getInTransaction();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getDefaultTransactionModel($sqlrconref)
 *
 *  Returns the database's native transaction model.  See
 *  setTranscationModel() for a list of potential return
 *  values.  Returns NULL if an error occurred. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getDefaultTransactionModel) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getDefaultTransactionModel();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_setTransactionModel($sqlrconref, $txmodel)
 *
 *  Sets the current transaction model to "txmodel" which should be one of:
 *
 *  * native - the database's native transaction model
 *  * none - no transactions
 *  * "implicit"
 *      * in a transaction when the session begins
 *      * commit/rollback implicitly starts a new transcaction
 *      * autocommit on/off take effect immediately
 *  * "explicit"
 *      * not in a transaction when the session begins
 *      * begin required to start a new transaction
 *      * commit/rollback does not start a new transcaction
 *      * autocommit on/off take effect immediately
 *  * "explicit-deferred"
 *      * not in a transaction when the session begins
 *      * begin required to start a new transaction
 *      * commit/rollback does not start a new transcaction
 *      * while in a begin-initiated transaction, autocommit
 *        on takes effect at next commit/rollback (deferred)
 *      * while in an autocommit-off-initiated transaction,
 *        autocommit on takes effect immediately
 *  * "explicit-error"
 *      * not in a transaction when the session begins
 *      * begin required to start a new transaction
 *      * commit/rollback does not start a new transcaction
 *      * while in a transaction, autocommit on/off throw error
 *
 *  Returns 1 on success and 0 on failure. */
DLEXPORT ZEND_FUNCTION(sqlrcon_setTransactionModel) {
	ZVAL sqlrcon;
	ZVAL txmodel;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&txmodel) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(txmodel);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->setTransactionModel(SVAL(txmodel));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getTransactionModel($sqlrconref)
 *
 *  Returns the current transaction model.  See
 *  setTranscationModel() for a list of potential return
 *  values.  Returns NULL if an error occurred. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getTransactionModel) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getTransactionModel();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_getDefaultIsolationLevel($sqlrconref)
 *
 *  Returns the database-specific default isolation level,
 *  or NULL if an error occurred. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getDefaultIsolationLevel) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getDefaultIsolationLevel();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_setIsolationLevel($sqlrconref, $isolationlevel)
 *
 *  Sets the transaction isolation level to "isolationlevel".  The string
 *  is the database-specific (native) name and is matched
 *  case-insensitively.
 *
 *  Valid isolation levels include:
 *
 *  For PostgreSQL:
 *  * READ UNCOMMITTED
 *  * READ COMMITTED (default)
 *  * REPEATABLE READ
 *  * SERIALIZABLE
 *
 *  For MySQL/MariaDB:
 *  * READ-UNCOMMITTED
 *  * READ-COMMITTED
 *  * REPEATABLE-READ (default)
 *  * SERIALIZABLE
 *
 *  For Oracle:
 *  * READ COMMITTED (default)
 *  * SERIALIZABLE
 *
 *  For DB2:
 *  * UR  (uncommitted read)
 *  * CS  (cursor stability, default)
 *  * RS  (read stability)
 *  * RR  (repeatable read)
 *
 *  For MS SQL Server (via FreeTDS):
 *  * READ UNCOMMITTED
 *  * READ COMMITTED (default)
 *  * REPEATABLE READ
 *  * SERIALIZABLE
 *  * SNAPSHOT
 *
 *  For SAP ASE (Sybase):
 *  * 0  (read uncommitted)
 *  * 1  (read committed, default)
 *  * 2  (repeatable read)
 *  * 3  (serializable)
 *
 *  For Informix:
 *  * dirty read
 *  * committed read (default)
 *  * cursor stability
 *  * repeatable read
 *
 *  For Firebird:
 *  * read committed (default)
 *  * read committed no record version
 *  * read consistency
 *  * snapshot
 *  * snapshot table stability
 *
 *  For SQLite:
 *  * 0  (serializable, default)
 *  * 1  (read uncommitted)
 *
 *  For ODBC:
 *  * SQL_TXN_READ_UNCOMMITTED
 *  * SQL_TXN_READ_COMMITTED
 *  * SQL_TXN_REPEATABLE_READ
 *  * SQL_TXN_SERIALIZABLE
 *
 *  (whether a given level is actually supported depends on
 *  the underlying ODBC driver and target database).  The
 *  generic ODBC backend also accepts the database-specific
 *  native names listed above for any of the other backends,
 *  as well as the JDBC TRANSACTION_* names, and maps them
 *  to the closest of the four ODBC levels above.
 *
 *  For other databases, the string is passed through to the
 *  backend as the argument to "set transaction isolation
 *  level".
 *
 *  Returns 1 if setting the isolation level succeeded, 0 if it
 *  failed. */
DLEXPORT ZEND_FUNCTION(sqlrcon_setIsolationLevel) {
	ZVAL sqlrcon;
	ZVAL isolationlevel;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&isolationlevel) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(isolationlevel);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->setIsolationLevel(SVAL(isolationlevel));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_getIsolationLevel($sqlrconref)
 *
 *  Returns the database-specific isolation level, "unknown" if the isolation
 *  level is unknown, or NULL if an error occurred. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getIsolationLevel) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getIsolationLevel();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_getDatabaseFeature($sqlrconref,$feature)
 *
 *  Returns the value of the specified database "feature".
 *
 *  Valid features include:
 *  * aggregate_functions
 *   * list - ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM
 *  * all_procedures_are_callable
 *   * true/false
 *  * all_tables_are_selectable
 *   * true/false
 *  * alter_domain_clauses
 *   * list - ADD_DOMAIN_CONSTRAINT,ADD_DOMAIN_DEFAULT,...
 *  * alter_table_operations
 *   * list - ADD_COLUMN,DROP_COLUMN
 *  * ansi92_sql_levels
 *   * list - ENTRY_LEVEL,FULL,INTERMEDIATE
 *  * auto_commit_failure_closes_all_result_sets
 *   * true/false
 *  * batch_operations
 *   * list - SELECT_EXPLICIT,ROW_COUNT_EXPLICIT,SELECT_PROC,ROW_COUNT_PROC
 *  * batch_row_counts
 *   * list - PROCEDURES,EXPLICIT,ROLLED_UP
 *  * catalog_separator
 *   * string
 *  * catalog_term
 *   * string
 *  * catalog_usage
 *   * list - DATA_MANIPULATION,INDEX_DEFINITIONS,...
 *  * collation_seq
 *   * string
 *  * create_assertion_clauses
 *   * list - CREATE_ASSERTION,CONSTRAINT_INITIALLY_DEFERRED,...
 *  * create_character_set_clauses
 *   * list - CREATE_CHARACTER_SET,COLLATE_CLAUSE,...
 *  * create_collation_clauses
 *   * list - CREATE_COLLATION
 *  * create_domain_clauses
 *   * list - CREATE_DOMAIN,CONSTRAINT_NAME_DEFINITION,...
 *  * create_schema_clauses
 *   * list - CREATE_SCHEMA,AUTHORIZATION,DEFAULT_CHARACTER_SET
 *  * create_table_clauses
 *   * list - CREATE_TABLE,TABLE_CONSTRAINT,...
 *  * create_translation_clauses
 *   * list - CREATE_TRANSLATION
 *  * create_view_clauses
 *   * list - CREATE_VIEW,CHECK_OPTION,CASCADED,LOCAL
 *  * data_definition_transaction_behavior
 *   * list - CAUSES_COMMIT,IGNORED_IN_TRANSACTIONS
 *  * ddl_index_operations
 *   * list - CREATE_INDEX,DROP_INDEX
 *   * string
 *  * default_result_set_holdability
 *   * string
 *  * deletes_are_detected
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * does_max_row_size_include_blobs
 *   * true/false
 *  * drop_assertion_clauses
 *   * list - DROP_ASSERTION
 *  * drop_character_set_clauses
 *   * list - DROP_CHARACTER_SET
 *  * drop_collation_clauses
 *   * list - DROP_COLLATION
 *  * drop_domain_clauses
 *   * list - DROP_DOMAIN,CASCADE,RESTRICT
 *  * drop_schema_clauses
 *   * list - DROP_SCHEMA,CASCADE,RESTRICT
 *  * drop_table_clauses
 *   * list - DROP_TABLE,CASCADE,RESTRICT
 *  * drop_translation_clauses
 *   * list - DROP_TRANSLATION
 *  * drop_view_clauses
 *   * list - DROP_VIEW,CASCADE,RESTRICT
 *  * extra_name_characters
 *   * string
 *  * foreign_key_delete_rules
 *   * list - CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL
 *  * foreign_key_update_rules
 *   * list - CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL
 *  * forward_only_cursor_attributes
 *   * list - NEXT,ABSOLUTE,RELATIVE,BOOKMARK,...
 *  * generated_key_always_returned
 *   * true/false
 *  * grant_clauses
 *   * list - DELETE_TABLE,INSERT_COLUMN,INSERT_TABLE,...
 *  * group_by_clauses
 *   * list - BASIC,BEYOND_SELECT,UNRELATED
 *  * identifier_case_storage
 *   * list - LOWER,MIXED,SENSITIVE,UPPER
 *  * identifier_quote_string
 *   * string
 *  * index_keywords
 *   * list - ASC,DESC
 *  * info_schema_views
 *   * list - ASSERTIONS,CHARACTER_SETS,CHECK_CONSTRAINTS,...
 *  * insert_operations
 *   * list - INSERT_LITERALS,INSERT_SEARCHED,SELECT_INTO
 *  * inserts_are_detected
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * is_catalog_at_start
 *   * true/false
 *  * isolation_levels
 *   * list - READ_UNCOMMITTED,READ_COMMITTED,...
 *  * local_file_usage
 *   * list - LOCAL_FILE_PER_TABLE,LOCAL_FILES
 *  * locators_update_copy
 *   * true/false
 *  * lock_types
 *   * list - NO_CHANGE,EXCLUSIVE,UNLOCK
 *  * max_binary_literal_length
 *   * number
 *  * max_catalog_name_length
 *   * number
 *  * max_char_literal_length
 *   * number
 *  * max_column_name_length
 *   * number
 *  * max_columns_in_group_by
 *   * number
 *  * max_columns_in_index
 *   * number
 *  * max_columns_in_order_by
 *   * number
 *  * max_columns_in_select
 *   * number
 *  * max_columns_in_table
 *   * number
 *  * max_connections
 *   * number
 *  * max_cursor_name_length
 *   * number
 *  * max_identifier_length
 *   * number
 *  * max_index_length
 *   * number
 *  * max_procedure_name_length
 *   * number
 *  * max_row_size
 *   * number
 *  * max_schema_name_length
 *   * number
 *  * max_statement_length
 *   * number
 *  * max_statements
 *   * number
 *  * max_table_name_length
 *   * number
 *  * max_tables_in_select
 *   * number
 *  * max_user_name_length
 *   * number
 *  * need_long_data_length
 *   * true/false
 *  * null_plus_non_null_is_null
 *   * true/false
 *  * null_sort_order
 *   * list - AT_END,AT_START,HIGH,LOW
 *  * numeric_functions
 *   * list - ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,COT,...
 *  * open_cursors_across
 *   * list - COMMIT,ROLLBACK
 *  * open_statements_across
 *   * list - COMMIT,ROLLBACK
 *  * others_deletes_are_visible
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * others_inserts_are_visible
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * others_updates_are_visible
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * outer_joins
 *   * list - BASIC,FULL,LIMITED
 *  * own_deletes_are_visible
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * own_inserts_are_visible
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * own_updates_are_visible
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * predicates
 *   * list - BETWEEN,COMPARISON,EXISTS,IN,ISNOTNULL,ISNULL,...
 *  * procedure_term
 *   * string
 *  * quoted_identifier_case_storage
 *   * list - LOWER,MIXED,SENSITIVE,UPPER
 *  * relational_join_operators
 *   * list - CORRESPONDING_CLAUSE,CROSS_JOIN,EXCEPT_JOIN,...
 *  * result_set_concurrencies
 *   * list - FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,...
 *  * result_set_holdabilities
 *   * list - CLOSE_CURSORS_AT_COMMIT,HOLD_CURSORS_OVER_COMMIT
 *  * result_set_types
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * revoke_clauses
 *   * list - CASCADE,DELETE_TABLE,GRANT_OPTION_FOR,...
 *  * row_id_lifetime
 *   * string
 *  * row_value_constructor_expressions
 *   * list - VALUE_EXPRESSION,NULL,DEFAULT,ROW_SUBQUERY
 *  * schema_term
 *   * string
 *  * schema_usage
 *   * list - DATA_MANIPULATION,INDEX_DEFINITIONS,...
 *  * scroll_concurrencies
 *   * list - READ_ONLY,LOCK,OPT_ROWVER,OPT_VALUES
 *  * search_string_escape
 *   * string
 *  * sql_grammar_levels
 *   * list - CORE,EXTENDED,MINIMUM
 *  * sql_keywords
 *   * list - ACCESS,ADD,ALTER,AUDIT,CLUSTER,COLUMN,COMMENT,...
 *  * sql_state_type
 *   * number
 *  * static_cursor_attributes
 *   * list - NEXT,ABSOLUTE,RELATIVE,BOOKMARK,...
 *  * stored_programs
 *   * list - FUNCTIONS,PROCEDURES
 *  * string_functions
 *   * list - CONCAT,INSERT,LEFT,LTRIM,LENGTH,LOCATE,LCASE,...
 *  * subquery_usage
 *   * list - COMPARISONS,EXISTS,INS,QUANTIFIEDS
 *  * supports_batch_updates
 *   * true/false
 *  * supports_column_aliasing
 *   * true/false
 *  * supports_convert
 *   * true/false
 *  * supports_correlated_subqueries
 *   * true/false
 *  * supports_describe_parameter
 *   * true/false
 *  * supports_expressions_in_order_by
 *   * true/false
 *  * supports_get_generated_keys
 *   * true/false
 *  * supports_integrity_enhancement_facility
 *   * true/false
 *  * supports_like_escape_clause
 *   * true/false
 *  * supports_multiple_result_sets
 *   * true/false
 *  * supports_multiple_transactions
 *   * true/false
 *  * supports_named_parameters
 *   * true/false
 *  * supports_non_nullable_columns
 *   * true/false
 *  * supports_order_by_unrelated
 *   * true/false
 *  * supports_savepoints
 *   * true/false
 *  * supports_select_for_update
 *   * true/false
 *  * supports_transactions
 *   * true/false
 *  * system_functions
 *   * list - USER,DBNAME,IFNULL
 *  * table_correlation_names
 *   * list - BASIC,DIFFERENT
 *  * table_term
 *   * string
 *  * time_date_add_intervals
 *   * list - FRAC_SECOND,SECOND,MINUTE,HOUR,DAY,WEEK,MONTH,...
 *  * time_date_diff_intervals
 *   * list - FRAC_SECOND,SECOND,MINUTE,HOUR,DAY,WEEK,MONTH,...
 *  * time_date_functions
 *   * list - NOW,CURDATE,DAYOFMONTH,DAYOFWEEK,DAYOFYEAR,...
 *  * time_date_literals
 *   * list - DATE,TIME,TIMESTAMP,INTERVAL_YEAR,...
 *  * transaction_ddl_dml
 *   * list - DDL_AND_DML,DML_ONLY
 *  * union_clauses
 *   * list - UNION,UNION_ALL
 *  * updates_are_detected
 *   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
 *  * value_expressions
 *   * list - CASE,CAST,COALESCE,NULLIF
 *  * where_current_of_operations
 *   * list - DELETE,UPDATE
 *
 *  Returns the value of the feature as a string, or NULL if
 *  an error occurred or an invalid feature was requested. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getDatabaseFeature) {
	ZVAL sqlrcon;
	ZVAL feature;
	const char *r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&feature) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(feature);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getDatabaseFeature(SVAL(feature));
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_errorMessage($sqlrconref)
 *
 *  If an operation failed and generated an error, the error message is
 *  available here.  If there is no error then this method returns NULL. */
DLEXPORT ZEND_FUNCTION(sqlrcon_errormessage) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->errorMessage();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcon_errorNumber($sqlrconref)
 *
 *  If an operation failed and generated an error, the error number is
 *  available here.  If there is no error then this method returns 0. */
DLEXPORT ZEND_FUNCTION(sqlrcon_errornumber) {
	ZVAL sqlrcon;
	int64_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->errorNumber();
		if (r) {
			RETURN_LONG(r);
		}
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcon_debugOn($sqlrconref)
 *
 *  Causes verbose debugging information to be sent to standard output.
 *  Another way to do this is to start a query with "-- debug\n".
 *  Yet another way is to set the environment variable SQLR_CLIENT_DEBUG
 *  to "ON" */
DLEXPORT ZEND_FUNCTION(sqlrcon_debugon) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->debugOn();
	}
}

/**
 *  call-seq:
 *  sqlrcon_debugOff($sqlrconref)
 *
 *  Turns debugging off. */
DLEXPORT ZEND_FUNCTION(sqlrcon_debugoff) {
	ZVAL sqlrcon;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->debugOff();
	}
}

/**
 *  call-seq:
 *  sqlrcon_getDebug($sqlrconref)
 *
 *  Returns 0 if debugging is off and 1 if debugging is on. */
DLEXPORT ZEND_FUNCTION(sqlrcon_getdebug) {
	ZVAL sqlrcon;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getDebug();
		if (r) {
			RETURN_TRUE;
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcon_setDebugFile($sqlrconref, $filename)
 *
 *  Allows you to specify a file to write debug to.
 *  Setting "filename" to NULL or an empty string causes debug
 *  to be written to standard output (the default). */
DLEXPORT ZEND_FUNCTION(sqlrcon_setdebugfile) {
	ZVAL sqlrcon;
	ZVAL filename;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(filename);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->setDebugFile(SVAL(filename));
	}
}

/**
 *  call-seq:
 *  sqlrcon_setClientInfo($sqlrconref, $clientinfo)
 *
 *  Allows you to set a string that will be passed to the
 *  server and ultimately included in server-side logging
 *  along with queries that were run by this instance of
 *  the client. */
DLEXPORT ZEND_FUNCTION(sqlrcon_setclientinfo) {
	ZVAL sqlrcon;
	ZVAL clientinfo;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcon,
				&clientinfo) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(clientinfo);
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		connection->setClientInfo(SVAL(clientinfo));
	}
}

/**
 *  call-seq:
 *  sqlrcon_getClientInfo($sqlrconref)
 *
 *  Returns the string that was set by setClientInfo(). */
DLEXPORT ZEND_FUNCTION(sqlrcon_getclientinfo) {
	ZVAL sqlrcon;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (connection) {
		r=connection->getClientInfo();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcur_alloc($sqlrconref)
 *
 *  Creates a cursor to run queries and fetch
 *  result sets using connection "sqlrconref" */
DLEXPORT ZEND_FUNCTION(sqlrcur_alloc) {
	zval	**sqlrcon;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrconnection *connection=NULL;
	ZEND_FETCH_RESOURCE(connection,
				sqlrconnection *,
				sqlrcon,
				-1,
				"sqlrelay connection",
				sqlrelay_connection);
	if (!connection) {
		RETURN_LONG(0);
	}
	sqlrcursor	*cursor=new sqlrcursor(connection,true);
	ZEND_REGISTER_RESOURCE(return_value,cursor,sqlrelay_cursor);
}

/**
 *  call-seq:
 *  sqlrcur_free($sqlrcurref)
 *
 *  Destroys the cursor and cleans up all associated result set data. */
DLEXPORT ZEND_FUNCTION(sqlrcur_free) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	ZEND_LIST_DELETE(sqlrcur);
}

/**
 *  call-seq:
 *  sqlrcur_setResultSetBufferSize($sqlrcurref, $rows)
 *
 *  Sets the number of rows of the result set to buffer at a time.
 *  0 (the default) means buffer the entire result set. */
DLEXPORT ZEND_FUNCTION(sqlrcur_setresultsetbuffersize) {
	ZVAL sqlrcur;
	ZVAL rows;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,&rows) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(rows);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->setResultSetBufferSize(LVAL(rows));
	}
}

/**
 *  call-seq:
 *  sqlrcur_getResultSetBufferSize($sqlrcurref)
 *
 *  Returns the number of result set rows that will be buffered at a time or
 *  0 for the entire result set. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getresultsetbuffersize) {
	ZVAL sqlrcur;
	uint64_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getResultSetBufferSize();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_dontGetColumnInfo($sqlrcurref)
 *
 *  Tells the server not to send any column info (names, types, sizes).  If
 *  you don't need that info, you should call this function to improve
 *  performance. */
DLEXPORT ZEND_FUNCTION(sqlrcur_dontgetcolumninfo) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->dontGetColumnInfo();
	}
}

/**
 *  call-seq:
 *  sqlrcur_getColumnInfo($sqlrcurref)
 *
 *  Tells the server to send column info. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumninfo) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->getColumnInfo();
	}
}

/**
 *  call-seq:
 *  sqlrcur_mixedCaseColumnNames($sqlrcurref)
 *
 *  Columns names are returned in the same case as they are defined in the
 *  database.  This is the default. */
DLEXPORT ZEND_FUNCTION(sqlrcur_mixedcasecolumnnames) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->mixedCaseColumnNames();
	}
}

/**
 *  call-seq:
 *  sqlrcur_upperCaseColumnNames($sqlrcurref)
 *
 *  Columns names are converted to upper case. */
DLEXPORT ZEND_FUNCTION(sqlrcur_uppercasecolumnnames) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->upperCaseColumnNames();
	}
}

/**
 *  call-seq:
 *  sqlrcur_lowerCaseColumnNames($sqlrcurref)
 *
 *  Columns names are converted to lower case. */
DLEXPORT ZEND_FUNCTION(sqlrcur_lowercasecolumnnames) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->lowerCaseColumnNames();
	}
}

/**
 *  call-seq:
 *  sqlrcur_cacheToFile($sqlrcurref, $filename)
 *
 *  Sets query caching on.  Future queries
 *  will be cached to the file "filename".
 *
 *  A default time-to-live of 10 minutes is
 *  also set.
 *
 *  Note that once cacheToFile() is called,
 *  the result sets of all future queries will
 *  be cached to that file until another call
 *  to cacheToFile() changes which file to
 *  cache to or a call to cacheOff() turns off
 *  caching. */
DLEXPORT ZEND_FUNCTION(sqlrcur_cachetofile) {
	ZVAL sqlrcur;
	ZVAL filename;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(filename);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->cacheToFile(SVAL(filename));
	}
}

/**
 *  call-seq:
 *  sqlrcur_setCacheTtl($sqlrcurref, $ttl)
 *
 *  Sets the time-to-live for cached result sets. The sqlr-cachemanger will
 *  remove each cached result set "ttl" seconds after it's created, provided
 *  it's scanning the directory containing the cache files. */
DLEXPORT ZEND_FUNCTION(sqlrcur_setcachettl) {
	ZVAL sqlrcur;
	ZVAL ttl;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&ttl) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(ttl);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->setCacheTtl(LVAL(ttl));
	}
}

/**
 *  call-seq:
 *  sqlrcur_getCacheFileName($sqlrcurref)
 *
 *  Returns the name of the file containing the
 *  cached result set. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcachefilename) {
	ZVAL sqlrcur;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getCacheFileName();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcur_cacheOff($sqlrcurref)
 *
 *  Sets query caching off. */
DLEXPORT ZEND_FUNCTION(sqlrcur_cacheoff) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->cacheOff();
	}
}

/**
 *  call-seq:
 *  sqlrcur_getDatabaseList($sqlrcurref, $wild)
 *
 *  Generates a result set containing databases that match the
 *  pattern "databases".
 *
 *  The result set will contain the following columns:
 *  * Database
 *
 *  If "databases" is empty or NULL then a result set
 *  containing all databases will be returned.
 *
 *  May actually return a result set of catalogs or schemas,
 *  depending on whether the backend database equates
 *  "database" with catalog or schema.
 *
 *  See getDatabaseIsSchema().
 *
 *  If SQL Relay doesn't support getting a list of databases
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getdatabaselist) {
	ZVAL sqlrcur;
	ZVAL databases;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&databases) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(databases);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getDatabaseList(SVAL(databases));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getCatalogList($sqlrcurref, $wild)
 *
 *  Generates a result set containing catalogs that match the
 *  pattern "catalog".
 *
 *  The result set will contain the following columns:
 *  * Database
 *
 *  If "catalog" is empty or NULL then a result set containing
 *  all catalogs will be returned.
 *
 *  If SQL Relay doesn't support getting a list of catalogs
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcataloglist) {
	ZVAL sqlrcur;
	ZVAL catalogs;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&catalogs) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(catalogs);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getCatalogList(SVAL(catalogs));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getSchemaList($sqlrcurref, $wild)
 *
 *  Generates a result set containing schemas that match the
 *  pattern "schemas".
 *
 *  The result set will contain the following columns:
 *  * Database
 *
 *  (The column name is a bit of a misnomer, the results are
 *  schemas, not databases.)
 *
 *  If "schemas" is empty or NULL then a result set containing
 *  all schemas in the current database will be returned.
 *
 *  If SQL Relay doesn't support getting a list of schemas
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getschemalist) {
	ZVAL sqlrcur;
	ZVAL schemas;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&schemas) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(schemas);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getSchemaList(SVAL(schemas));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getTableTypeList($sqlrcurref)
 *
 *  Generates a result set containing supported table types.
 *
 *  The result set will contain the following columns:
 *  * table_type
 *
 *  If SQL Relay doesn't support getting a list of table types
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_gettabletypelist) {
	ZVAL sqlrcur;
	bool r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getTableTypeList();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getTableList($sqlrcurref, $wild)
 *
 *  Generates a result set containing the tables in the current
 *  database and schema that match the pattern "tables".
 *
 *  The result set will contain the following columns:
 *  * Tables_in_xxx
 *
 *  If "tables" is empty or NULL then a result set containing
 *  all tables in the current database/schema will be returned.
 *
 *  If SQL Relay doesn't support getting a list of tables
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_gettablelist) {
	ZVAL sqlrcur;
	ZVAL tables;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&tables) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(tables);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getTableList(SVAL(tables));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getTypeInfoList($sqlrcurref, $type)
 *
 *  Generates a result set containing data type information for
 *  "type".
 *
 *  The result set will contain the following columns:
 *  * type_name
 *  * data_type
 *  * precision
 *  * literal_prefix
 *  * literal_suffix
 *  * create_params
 *  * nullable
 *  * case_sensitive
 *  * searchable
 *  * unsigned_attribute
 *  * fixed_prec_scale
 *  * auto_increment
 *  * local_type_name
 *  * minumum_scale
 *  * maxiumm_scale
 *  * sql_data_type
 *  * sql_datetime_sub
 *  * num_prec_radix
 *  * interval_precision
 *
 *  If "type" is empty or NULL then a result set containing
 *  all data types in the current databas/schema will be
 *  returned.
 *
 *  If SQL Relay doesn't support getting type info
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_gettypeinfolist) {
	ZVAL sqlrcur;
	ZVAL type;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&type) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(type);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getTypeInfoList(SVAL(type));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnList($sqlrcurref, $table, $wild)
 *
 *  Generates a result set containing the columns of "table",
 *  which match the pattern "columns".
 *
 *  The result set will contain the following columns:
 *  * column_name
 *  * data_type
 *  * character_maximum_length
 *  * numeric_precision
 *  * numeric_scale
 *  * is_nullable
 *  * column_key
 *  * column_default
 *  * extra
 *
 *  If "columns" is empty or NULL then a list of all columns
 *  of "table" will be returned.
 *
 *  If SQL Relay doesn't support getting a list of columns
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnlist) {
	ZVAL sqlrcur;
	ZVAL table;
	ZVAL columns;
	bool r;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&table,
				&columns) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(table);
	convert_to_string_ex(columns);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getColumnList(SVAL(table),
					SVAL(columns));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getPrimaryKeysList($sqlrcurref, $table, $wild)
 *
 *  Generates a result set containing the primary keys of
 *  "table", which match the pattern "columns".
 *
 *  The result set will contain the following columns:
 *  * table
 *  * non_unique
 *  * key_name
 *  * seq_in_index
 *  * column_name
 *  * collation
 *  * cardinality
 *  * sub_part
 *  * packed
 *  * null
 *  * index_type
 *  * comment
 *  * index_comment
 *
 *  If "columns" is empty or NULL then a result set containing
 *  all primary keys of "table" will be returned.
 *
 *  If SQL Relay doesn't support getting a list of primary keys
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getprimarykeyslist) {
	ZVAL sqlrcur;
	ZVAL table;
	ZVAL columns;
	bool r;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&table,
				&columns) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(table);
	convert_to_string_ex(columns);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getPrimaryKeysList(SVAL(table),
					SVAL(columns));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getKeyAndIndexList($sqlrcurref, $table, $wild)
 *
 *  Generates a result set containing the keys and indexes of
 *  "table", which match the pattern "qualifier".
 *
 *  The result set will contain the following columns:
 *  * table
 *  * non_unique
 *  * key_name
 *  * seq_in_index
 *  * column_name
 *  * collation
 *  * cardinality
 *  * sub_part
 *  * packed
 *  * null
 *  * index_type
 *  * comment
 *  * index_comment
 *
 *  If "qualifier" is empty or NULL then a result set
 *  containing all keys and indexes of "table" will be
 *  returned.
 *
 *  If SQL Relay doesn't support getting a list of keys and
 *  indexes for the current database backend (or the database
 *  doesn't) then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getkeyandindexlist) {
	ZVAL sqlrcur;
	ZVAL table;
	ZVAL qualifier;
	bool r;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&table,
				&qualifier) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(table);
	convert_to_string_ex(qualifier);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getKeyAndIndexList(SVAL(table),
					SVAL(qualifier));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getProcedureList($sqlrcurref, $wild)
 *
 *  Generates a result set containing procedures that match the
 *  pattern "procedures".
 *
 *  The result set will contain the following columns:
 *  * routine_catalog
 *  * routine_schema
 *  * routine_name
 *  * data_type
 *
 *  If "procedures" is empty or NULL then a result set
 *  containing all procedures in the current database/schema
 *  will be returned.
 *
 *  If SQL Relay doesn't support getting a list of procedures
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getprocedurelist) {
	ZVAL sqlrcur;
	ZVAL procedures;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&procedures) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(procedures);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getProcedureList(SVAL(procedures));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getProcedureParameterList($sqlrcurref, $procedure, $wild)
 *
 *  Generates a result set containing the parameters of
 *  "procedure", which match the pattern "parameters".
 *
 *  The result set will contain the following columns:
 *  * parameter_name
 *  * parameter_mode
 *  * data_type
 *  * character_maximum_length
 *  * ordinal_position
 *
 *  If "parameters" is empty or NULL then a result set
 *  containing all parameters of "procedure" will be returned.
 *
 *  If SQL Relay doesn't support getting a list of procedure
 *  parameters for the current database backend (or the
 *  database doesn't) then an empty result set will be
 *  returned. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getprocedureparameterlist) {
	ZVAL sqlrcur;
	ZVAL procedure;
	ZVAL parameters;
	bool r;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&procedure,
				&parameters) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(procedure);
	convert_to_string_ex(parameters);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getProcedureParameterList(SVAL(procedure),
					SVAL(parameters));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_sendQuery($sqlrcurref, $query)
 *
 *  Sends "query" directly and gets a result set. */
DLEXPORT ZEND_FUNCTION(sqlrcur_sendquery) {
	ZVAL sqlrcur;
	ZVAL query;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&query) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(query);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->sendQuery(SVAL(query));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_sendQueryWithLength($sqlrcurref, $query, $length)
 *
 *  Sends "query" with length "length" directly and gets a result set. This
 *  function must be used if the query contains binary data. */
DLEXPORT ZEND_FUNCTION(sqlrcur_sendquerywithlength) {
	ZVAL sqlrcur;
	ZVAL query;
	ZVAL length;
	bool r;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&query,
				&length) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(query);
	convert_to_long_ex(length);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->sendQuery(SVAL(query),
					LVAL(length));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_sendFileQuery($sqlrcurref, $path, $filename)
 *
 *  Sends the query in file "path"/"filename" directly and gets a result set. */
DLEXPORT ZEND_FUNCTION(sqlrcur_sendfilequery) {
	ZVAL sqlrcur;
	ZVAL path;
	ZVAL filename;
	bool r;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&path,
				&filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(path);
	convert_to_string_ex(filename);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=((sqlrcursor *)LVAL(sqlrcur))->
				sendFileQuery(SVAL(path),
						SVAL(filename));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_prepareQuery($sqlrcurref, $query)
 *
 *  Prepare to execute "query". */
DLEXPORT ZEND_FUNCTION(sqlrcur_preparequery) {
	ZVAL sqlrcur;
	ZVAL query;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&query) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(query);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->prepareQuery(SVAL(query));
	}
}

/**
 *  call-seq:
 *  sqlrcur_prepareQueryWithLength($sqlrcurref, $query, $length)
 *
 *  Prepare to execute "query" with length "length".  This function must be
 *  used if the query contains binary data. */
DLEXPORT ZEND_FUNCTION(sqlrcur_preparequerywithlength) {
	ZVAL sqlrcur;
	ZVAL query;
	ZVAL length;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&query,
				&length) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(query);
	convert_to_long_ex(length);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->prepareQuery(SVAL(query),
					LVAL(length));
	}
}

/**
 *  call-seq:
 *  sqlrcur_prepareFileQuery($sqlrcurref, $path, $filename)
 *
 *  Prepare to execute the contents of "path"/"filename".  Returns false if the
 *  file couldn't be opened. */
DLEXPORT ZEND_FUNCTION(sqlrcur_preparefilequery) {
	ZVAL sqlrcur;
	ZVAL path;
	ZVAL filename;
	bool r;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&path,
				&filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(path);
	convert_to_string_ex(filename);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->prepareFileQuery(SVAL(path),
						SVAL(filename));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_substitution($sqlrcurref, $variable, $value, $precision, $scale)
 *
 *  Defines a substitution variable.  The value may be a string,
 *  integer or decimal.  If it is a decimal, then precision and scale may
 *  also be specified */
DLEXPORT ZEND_FUNCTION(sqlrcur_substitution) {
	ZVAL sqlrcur;
	ZVAL variable;
	ZVAL value;
	ZVAL precision;
	ZVAL scale;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&variable,
				&value) == FAILURE) {
		if (ZEND_NUM_ARGS() != 5 || 
			GET_PARAMETERS(
					ZEND_NUM_ARGS() TSRMLS_CC,
					PARAMS("zzzzz")
					&sqlrcur,
					&variable,
					&value,
					&precision,
					&scale)== FAILURE) {
			WRONG_PARAM_COUNT;
		}
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(value)==IS_STRING) {
			convert_to_string_ex(value);
			cursor->substitution(SVAL(variable),
						SVAL(value));
			RETURN_LONG(1);
		} else if (TYPE(value)==IS_LONG) {
			convert_to_long_ex(value);
			cursor->substitution(SVAL(variable),
						LVAL(value));
			RETURN_LONG(1);
		} else if (ZEND_NUM_ARGS()==5 && TYPE(value)==IS_DOUBLE) {
			convert_to_double_ex(value);
			cursor->substitution(
				SVAL(variable),
				DVAL(value),
				(unsigned short)LVAL(precision),
				(unsigned short)LVAL(scale));
			RETURN_LONG(1);
		} else if (TYPE(value)==IS_NULL) {
			cursor->substitution(
				SVAL(variable),
				(const char *)NULL);
			RETURN_LONG(1);
		}
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_substitutions($sqlrcurref, $variables, $values, $precisions, $scales)
 *
 *  Defines an array of substitution variables.  The values may be
 *  strings, integers or decimals.  If they are decimals, then precisions and
 *  scales may also be specified */
DLEXPORT ZEND_FUNCTION(sqlrcur_substitutions) {
	ZVAL sqlrcur;
	ZVAL variables;
	ZVAL values;
	ZVAL precisions;
	ZVAL scales;
	ZVAL var;
	ZVAL val;
	ZVAL precision;
	ZVAL scale;
	unsigned int i;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&variables,
				&values) == FAILURE) {
		if (ZEND_NUM_ARGS() != 5 || 
			GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
					PARAMS("zzzzz")
					&sqlrcur,
					&variables,
					&values,
					&precisions,
					&scales)== FAILURE) {
			WRONG_PARAM_COUNT;
		} else {
			convert_to_array_ex(precisions);
			convert_to_array_ex(scales);
		}
	}
	convert_to_array_ex(variables);
	convert_to_array_ex(values);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	long	success=1;
	if (cursor) {
		for (i=0; i<ARRVAL(variables)->nNumOfElements; i++) {
			HASH_INDEX_FIND(ARRVAL(variables),i,var);
			HASH_INDEX_FIND(ARRVAL(values),i,val);
			if (TYPE(val)==IS_STRING) {
				convert_to_string_ex(val);
				cursor->substitution(SVAL(var),
							SVAL(val));
			} else if (TYPE(val)==IS_LONG) {
				convert_to_long_ex(val);
				cursor->substitution(SVAL(var),
							LVAL(val));
			} else if (ZEND_NUM_ARGS()==5 &&
						TYPE(val)==IS_DOUBLE) {
				HASH_INDEX_FIND(ARRVAL(precisions),i,precision);
				HASH_INDEX_FIND(ARRVAL(scales),i,scale);
				convert_to_double_ex(val);
				convert_to_long_ex(precision);
				convert_to_long_ex(scale);
				cursor->substitution(
					SVAL(var),
					DVAL(val),
					(unsigned short)LVAL(precision),
					(unsigned short)LVAL(scale));
			} else if (TYPE(val)==IS_NULL) {
				cursor->substitution(SVAL(var),
							(const char *)NULL);
			} else {
				success=0;
			}
		}
	}
	RETURN_LONG(success);
}

/**
 *  call-seq:
 *  sqlrcur_inputBind($sqlrcurref, $variable, $value, $precision, $scale)
 *
 *  Defines an input bind variable.  The value may be a string,
 *  integer or decimal.  If the value is a decimal, then precision and scale may
 *  also be specified.  If you don't have the precision and scale then set them
 *  both to 0.  However in that case you may get unexpected rounding behavior
 *  if the server is faking binds. */
DLEXPORT ZEND_FUNCTION(sqlrcur_inputbind) {
	ZVAL sqlrcur;
	ZVAL variable;
	ZVAL value;
	ZVAL precision;
	ZVAL scale;
	ZVAL length;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&variable,
				&value) == FAILURE) {
		if (ZEND_NUM_ARGS() != 4 || 
			GET_PARAMETERS(
					ZEND_NUM_ARGS() TSRMLS_CC,
					PARAMS("zzzz")
					&sqlrcur,
					&variable,
					&value,
					&length)== FAILURE) {
			if (ZEND_NUM_ARGS() != 5 || 
				GET_PARAMETERS(
						ZEND_NUM_ARGS() TSRMLS_CC,
						PARAMS("zzzzz")
						&sqlrcur,
						&variable,
						&value,
						&precision,
						&scale)== FAILURE) {
				WRONG_PARAM_COUNT;
			}
		}
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(value)==IS_STRING) {
			convert_to_string_ex(value);
			if (ZEND_NUM_ARGS() == 4 && LVAL(length)>0) {
				cursor->inputBind(
					SVAL(variable),
					SVAL(value),
					LVAL(length));
			} else {
				cursor->inputBind(
					SVAL(variable),
					SVAL(value));
			}
			RETURN_LONG(1);
		} else if (TYPE(value)==IS_LONG) {
			convert_to_long_ex(value);
			cursor->inputBind(SVAL(variable),
						LVAL(value));
			RETURN_LONG(1);
		} else if (ZEND_NUM_ARGS()==5 && TYPE(value)==IS_DOUBLE) {
			convert_to_double_ex(value);
			cursor->inputBind(
				SVAL(variable),
				DVAL(value),
				(unsigned short)LVAL(precision),
				(unsigned short)LVAL(scale));
			RETURN_LONG(1);
		} else if (TYPE(value)==IS_NULL) {
			cursor->inputBind(
				SVAL(variable),
				(const char *)NULL);
			RETURN_LONG(1);
		}
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_inputBinds($sqlrcurref, $variables, $values, $precisions, $scales)
 *
 *  Defines an array of input bind variables.  The values may be
 *  strings, integers or decimals.  If they are decimals, then precisions and
 *  scales may also be specified */
DLEXPORT ZEND_FUNCTION(sqlrcur_inputbinds) {
	ZVAL sqlrcur;
	ZVAL variables;
	ZVAL values;
	ZVAL precisions;
	ZVAL scales;
	ZVAL var;
	ZVAL val;
	ZVAL precision;
	ZVAL scale;
	int i;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&variables,
				&values) == FAILURE) {
		if (ZEND_NUM_ARGS() != 5 || 
			GET_PARAMETERS(
					ZEND_NUM_ARGS() TSRMLS_CC,
					PARAMS("zzzzz")
					&sqlrcur,
					&variables,
					&values,
					&precisions,
					&scales)== FAILURE) {
			WRONG_PARAM_COUNT;
		} else {
			convert_to_array_ex(precisions);
			convert_to_array_ex(scales);
		}
	}
	convert_to_array_ex(variables);
	convert_to_array_ex(values);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	long	success=1;
	if (cursor) {
		for (i=0; i<zend_hash_num_elements(ARRVAL(variables)); i++) {
			HASH_INDEX_FIND(ARRVAL(variables),i,var);
			HASH_INDEX_FIND(ARRVAL(values),i,val);
			if (TYPE(val)==IS_STRING) {
				convert_to_string_ex(val);
				cursor->inputBind(SVAL(var),
							SVAL(val));
			} else if (TYPE(val)==IS_LONG) {
				convert_to_long_ex(val);
				cursor->inputBind(SVAL(var),
							LVAL(val));
			} else if (ZEND_NUM_ARGS()==5 &&
						TYPE(val)==IS_DOUBLE) {
				HASH_INDEX_FIND(ARRVAL(precisions),i,precision);
				HASH_INDEX_FIND(ARRVAL(scales),i,scale);
				convert_to_long_ex(precision);
				convert_to_long_ex(scale);
				convert_to_double_ex(val);
				cursor->inputBind(
					SVAL(var),
					DVAL(val),
					(unsigned short)LVAL(precision),
					(unsigned short)LVAL(scale));
			} else if (TYPE(val)==IS_NULL) {
				cursor->inputBind(SVAL(var),
							(const char *)NULL);
			} else {
				success=0;
			}
		}
	}
	RETURN_LONG(success);
}

/**
 *  call-seq:
 *  sqlrcur_inputBindDate($sqlrcurref, $variable, $year, $month, $day, $hour, $minute, $second, $microsecond, $tz, $isnegative)
 *
 *  Defines a date input bind variable.  "day" and "month"
 *  are 1-based.
 *
 *  Some databases distinguish between date, time, and
 *  datetime types.  For those databases...
 *
 *  * The input bind variable will be interpreted as a time type
 *  if year and/or month are negative.
 *
 *  * The input bind variable will be interpreted as a date type
 *  if hour, minute, second, and/or microsecond are negative.
 *
 *  * The input bind variable will be interpreted as a datetime
 *  type if all parts are positive.
 *
 *  "tz" is the timezone abbreviation, and may be left NULL.
 *  Most databases ignore "tz".
 *
 *  Set "isnegative" may be set to true to represent a negative
 *  time interval.  However, few databases support negative
 *  time intervals and ignore "isnegative". */
DLEXPORT ZEND_FUNCTION(sqlrcur_inputbinddate) {
	ZVAL sqlrcur;
	ZVAL variable;
	ZVAL year;
	ZVAL month;
	ZVAL day;
	ZVAL hour;
	ZVAL minute;
	ZVAL second;
	ZVAL microsecond;
	ZVAL tz;
	ZVAL isnegative;
	if (ZEND_NUM_ARGS() != 11 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzzzzzzz")
				&sqlrcur,
				&variable,
				&year,
				&month,
				&day,
				&hour,
				&minute,
				&second,
				&microsecond,
				&tz,
				&isnegative) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	convert_to_long_ex(year);
	convert_to_long_ex(month);
	convert_to_long_ex(day);
	convert_to_long_ex(hour);
	convert_to_long_ex(minute);
	convert_to_long_ex(second);
	convert_to_long_ex(microsecond);
	convert_to_string_ex(tz);
	convert_to_long_ex(isnegative);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->inputBind(SVAL(variable),
					(int16_t)LVAL(year),
					(int16_t)LVAL(month),
					(int16_t)LVAL(day),
					(int16_t)LVAL(hour),
					(int16_t)LVAL(minute),
					(int16_t)LVAL(second),
					(int32_t)LVAL(microsecond),
					SVAL(tz),
					(bool)LVAL(isnegative));
		RETURN_LONG(1);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_inputBindBlob($sqlrcurref, $variable, $value, $size)
 *
 *  Defines a binary lob input bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_inputbindblob) {
	ZVAL sqlrcur;
	ZVAL variable;
	ZVAL value;
	ZVAL size;
	if (ZEND_NUM_ARGS() != 4 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzz")
				&sqlrcur,
				&variable,
				&value,
				&size) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	bool	valueisnull=(TYPE(value)==IS_NULL);
	if (!valueisnull) {
		convert_to_string_ex(value);
	}
	convert_to_long_ex(size);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->inputBindBlob(SVAL(variable),
					(valueisnull)?NULL:SVAL(value),
					LVAL(size));
		RETURN_LONG(1);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_inputBindClob($sqlrcurref, $variable, $value, $size)
 *
 *  Defines a character lob input bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_inputbindclob) {
	ZVAL sqlrcur;
	ZVAL variable;
	ZVAL value;
	ZVAL size;
	if (ZEND_NUM_ARGS() != 4 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzz")
				&sqlrcur,
				&variable,
				&value,
				&size) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	bool	valueisnull=(TYPE(value)==IS_NULL);
	if (!valueisnull) {
		convert_to_string_ex(value);
	}
	convert_to_long_ex(size);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->inputBindClob(SVAL(variable),
					(valueisnull)?NULL:SVAL(value),
					LVAL(size));
		RETURN_LONG(1);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_defineOutputBindString($sqlrcurref, $variable, $length)
 *
 *  Defines an output bind variable.
 *  "bufferlength" bytes will be reserved
 *  to store the value. */
DLEXPORT ZEND_FUNCTION(sqlrcur_defineoutputbindstring) {
	ZVAL sqlrcur;
	ZVAL variable;
	ZVAL length;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&variable,
				&length) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	convert_to_long_ex(length);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->defineOutputBindString(
					SVAL(variable),
					LVAL(length));
	}
}

/**
 *  call-seq:
 *  sqlrcur_defineOutputBindInteger($sqlrcurref, $variable)
 *
 *  Defines an integer output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_defineoutputbindinteger) {
	ZVAL sqlrcur;
	ZVAL variable;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->defineOutputBindInteger(SVAL(variable));
	}
}

/**
 *  call-seq:
 *  sqlrcur_defineOutputBindDouble($sqlrcurref, $variable)
 *
 *  Defines a decimal output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_defineoutputbinddouble) {
	ZVAL sqlrcur;
	ZVAL variable;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->defineOutputBindDouble(SVAL(variable));
	}
}

/**
 *  call-seq:
 *  sqlrcur_defineOutputBindDate($sqlrcurref, $variable)
 *
 *  Defines a date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_defineoutputbinddate) {
	ZVAL sqlrcur;
	ZVAL variable;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->defineOutputBindDate(SVAL(variable));
	}
}

/**
 *  call-seq:
 *  sqlrcur_defineOutputBindBlob($sqlrcurref, $variable)
 *
 *  Defines a binary lob output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_defineoutputbindblob) {
	ZVAL sqlrcur;
	ZVAL variable;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->defineOutputBindBlob(SVAL(variable));
	}
}

/**
 *  call-seq:
 *  sqlrcur_defineOutputBindClob($sqlrcurref, $variable)
 *
 *  Defines a character lob output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_defineoutputbindclob) {
	ZVAL sqlrcur;
	ZVAL variable;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->defineOutputBindClob(SVAL(variable));
	}
}

/**
 *  call-seq:
 *  sqlrcur_defineOutputBindCursor($sqlrcurref, $variable)
 *
 *  Defines a cursor output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_defineoutputbindcursor) {
	ZVAL sqlrcur;
	ZVAL variable;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->defineOutputBindCursor(SVAL(variable));
	}
}

/**
 *  call-seq:
 *  sqlrcur_clearBinds($sqlrcurref)
 *
 *  Clears all bind variables. */
DLEXPORT ZEND_FUNCTION(sqlrcur_clearbinds) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->clearBinds();
	}
}

/**
 *  call-seq:
 *  sqlrcur_countBindVariables($sqlrcurref)
 *
 *  Parses the previously prepared query, counts the number of bind variables
 *  defined in it and returns that number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_countbindvariables) {
	ZVAL sqlrcur;
	uint16_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->countBindVariables();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_validateBinds($sqlrcurref)
 *
 *  If you are binding to any variables that might not actually be in your
 *  query, call this to ensure that the database won't try to bind them unless
 *  they really are in the query.  There is a performance penalty for calling
 *  this function */
DLEXPORT ZEND_FUNCTION(sqlrcur_validatebinds) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->validateBinds();
	}
}

/**
 *  call-seq:
 *  sqlrcur_validBind($sqlrcurref, $variable)
 *
 *  Returns true if "variable" was a valid bind variable of the query. */
DLEXPORT ZEND_FUNCTION(sqlrcur_validbind) {
	ZVAL sqlrcur;
	ZVAL variable;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->validBind(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_executeQuery($sqlrcurref)
 *
 *  Execute the query that was previously prepared and bound. */
DLEXPORT ZEND_FUNCTION(sqlrcur_executequery) {
	ZVAL sqlrcur;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->executeQuery();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_fetchFromBindCursor($sqlrcurref)
 *
 *  Fetch from a cursor that was returned as an output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_fetchfrombindcursor) {
	ZVAL sqlrcur;
	bool r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->fetchFromBindCursor();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindString($sqlrcurref, $variable)
 *
 *  Get the value stored in a previously defined
 *  string output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbindstring) {
	ZVAL sqlrcur;
	ZVAL variable;
	const char *r;
	uint32_t rl;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindString(SVAL(variable));
		rl=cursor->getOutputBindLength(SVAL(variable));
		if (r) {
			RET_STRINGL(const_cast<char *>(r),rl,1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindInteger($sqlrcurref, $variable)
 *
 *  Get the value stored in a previously defined
 *  integer output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbindinteger) {
	ZVAL sqlrcur;
	ZVAL variable;
	int64_t r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindInteger(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDouble($sqlrcurref, $variable)
 *
 *  Get the value stored in a previously defined
 *  decimal output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddouble) {
	ZVAL sqlrcur;
	ZVAL variable;
	double r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDouble(SVAL(variable));
		RETURN_DOUBLE(r);
	}
	RETURN_DOUBLE(0.0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindBlob($sqlrcurref, $variable)
 *
 *  Get the value stored in a previously defined
 *  binary lob output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbindblob) {
	ZVAL sqlrcur;
	ZVAL variable;
	const char *r;
	uint32_t rl;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindBlob(SVAL(variable));
		rl=cursor->getOutputBindLength(SVAL(variable));
		if (r) {
			RET_STRINGL(const_cast<char *>(r),rl,1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindClob($sqlrcurref, $variable)
 *
 *  Get the value stored in a previously defined
 *  character lob output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbindclob) {
	ZVAL sqlrcur;
	ZVAL variable;
	const char *r;
	uint32_t rl;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindClob(SVAL(variable));
		rl=cursor->getOutputBindLength(SVAL(variable));
		if (r) {
			RET_STRINGL(const_cast<char *>(r),rl,1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindLength($sqlrcurref, $variable)
 *
 *  Get the length of the value stored in a previously
 *  defined output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbindlength) {
	ZVAL sqlrcur;
	ZVAL variable;
	uint32_t r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindLength(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindCursor($sqlrcurref, $variable)
 *
 *  Get the cursor associated with a previously defined output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbindcursor) {
	ZVAL sqlrcur;
	ZVAL variable;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (!cursor) {
		RETURN_LONG(0);
	}
	sqlrcursor	*s=cursor->getOutputBindCursor(
					SVAL(variable),
					true);
	ZEND_REGISTER_RESOURCE(return_value,s,sqlrelay_cursor);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateYear($sqlrcurref, $variable)
 *
 *  Get the year from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddateyear) {
	ZVAL sqlrcur;
	ZVAL variable;
	int16_t r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateYear(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateMonth($sqlrcurref, $variable)
 *
 *  Get the month from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddatemonth) {
	ZVAL sqlrcur;
	ZVAL variable;
	int16_t r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateMonth(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateDay($sqlrcurref, $variable)
 *
 *  Get the day from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddateday) {
	ZVAL sqlrcur;
	ZVAL variable;
	int16_t r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateDay(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateHour($sqlrcurref, $variable)
 *
 *  Get the hour from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddatehour) {
	ZVAL sqlrcur;
	ZVAL variable;
	int16_t r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateHour(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateMinute($sqlrcurref, $variable)
 *
 *  Get the minute from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddateminute) {
	ZVAL sqlrcur;
	ZVAL variable;
	int16_t r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateMinute(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateSecond($sqlrcurref, $variable)
 *
 *  Get the second from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddatesecond) {
	ZVAL sqlrcur;
	ZVAL variable;
	int16_t r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateSecond(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateMicrosecond($sqlrcurref, $variable)
 *
 *  Get the microsecond from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddatemicrosecond) {
	ZVAL sqlrcur;
	ZVAL variable;
	int32_t r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateMicrosecond(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateTz($sqlrcurref, $variable)
 *
 *  Get the time zone from a previously defined
 *  date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddatetz) {
	ZVAL sqlrcur;
	ZVAL variable;
	const char *r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateTz(SVAL(variable));
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcur_getOutputBindDateIsNegative($sqlrcurref, $variable)
 *
 *  Get whether the value is negative from a
 *  previously defined date output bind variable. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getoutputbinddateisnegative) {
	ZVAL sqlrcur;
	ZVAL variable;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&variable) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(variable);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getOutputBindDateIsNegative(SVAL(variable));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_openCachedResultSet($sqlrcurref, $filename)
 *
 *  Opens a cached result set.  Returns 1 on success and 0 on failure. */
DLEXPORT ZEND_FUNCTION(sqlrcur_opencachedresultset) {
	ZVAL sqlrcur;
	ZVAL filename;
	bool r;
	if (ZEND_NUM_ARGS() != 2 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(filename);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->openCachedResultSet(SVAL(filename));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_colCount($sqlrcurref)
 *
 *  Returns the number of columns in the current result set. */
DLEXPORT ZEND_FUNCTION(sqlrcur_colcount) {
	ZVAL sqlrcur;
	uint32_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->colCount();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_rowCount($sqlrcurref)
 *
 *  Returns the number of rows in the current result set (if the result set is
 *  being stepped through, this returns the number of rows processed so far). */
DLEXPORT ZEND_FUNCTION(sqlrcur_rowcount) {
	ZVAL sqlrcur;
	uint64_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->rowCount();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_totalRows($sqlrcurref)
 *
 *  Returns the total number of rows that will be returned in the result set.
 *  Not all databases support this call.  Don't use it for applications which
 *  are designed to be portable across databases.  0 is returned by databases
 *  which don't support this option. */
DLEXPORT ZEND_FUNCTION(sqlrcur_totalrows) {
	ZVAL sqlrcur;
	uint64_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->totalRows();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_affectedRows($sqlrcurref)
 *
 *  Returns the number of rows that were updated, inserted or deleted by the
 *  query.  Not all databases support this call.  Don't use it for applications
 *  which are designed to be portable across databases.  0 is returned by
 *  databases which don't support this option. */
DLEXPORT ZEND_FUNCTION(sqlrcur_affectedrows) {
	ZVAL sqlrcur;
	uint64_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->affectedRows();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_firstRowIndex($sqlrcurref)
 *
 *  Returns the index of the first buffered row.  This is useful when buffering
 *  only part of the result set at a time. */
DLEXPORT ZEND_FUNCTION(sqlrcur_firstrowindex) {
	ZVAL sqlrcur;
	uint64_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->firstRowIndex();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_endOfResultSet($sqlrcurref)
 *
 *  Returns 0 if part of the result set is still pending on the server and 1 if
 *  not.  This function can only return 0 if setResultSetBufferSize() has been
 *  called with a parameter other than 0. */
DLEXPORT ZEND_FUNCTION(sqlrcur_endofresultset) {
	ZVAL sqlrcur;
	bool r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->endOfResultSet();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_nextResultSet($sqlrcurref)
 *
 *  Returns true and acts like executeQuery() when there is another result set
 *  available from the server. */
DLEXPORT ZEND_FUNCTION(sqlrcur_nextresultset) {
	ZVAL sqlrcur;
	bool r;
	if (ZEND_NUM_ARGS() != 1 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->nextResultSet();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_errorMessage($sqlrcurref)
 *
 *  If a query failed and generated an error, the error message is available
 *  here.  If the query succeeded then this method returns NULL. */
DLEXPORT ZEND_FUNCTION(sqlrcur_errormessage) {
	ZVAL sqlrcur;
	const char *r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->errorMessage();
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcur_errorNumber($sqlrcurref)
 *
 *  If a query failed and generated an error, the error number is available
 *  here.  If there is no error then this method returns 0. */
DLEXPORT ZEND_FUNCTION(sqlrcur_errornumber) {
	ZVAL sqlrcur;
	int64_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->errorNumber();
		if (r) {
			RETURN_LONG(r);
		}
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getNullsAsEmptyStrings($sqlrcurref)
 *
 *  Tells the connection to return NULL fields and output bind variables as
 *  empty strings.  This is the default. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getnullsasemptystrings) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->getNullsAsEmptyStrings();
	}
}

/**
 *  call-seq:
 *  sqlrcur_getNullsAsNulls($sqlrcurref)
 *
 *  Tells the connection to return NULL fields
 *  and output bind variables as NULL's rather
 *  than as empty strings. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getnullsasnulls) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->getNullsAsNulls();
	}
}

/**
 *  call-seq:
 *  sqlrcur_getField($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as a string. "col" may be specified as the
 *  column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfield) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	const char *r=NULL;
	uint32_t rl;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getField(LVAL(row),LVAL(col));
			rl=cursor->getFieldLength(LVAL(row),LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getField(LVAL(row),
						SVAL(col));
			rl=cursor->getFieldLength(LVAL(row),
						SVAL(col));
		}
		if (r) {
			RET_STRINGL(const_cast<char *>(r),rl,1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcur_getFieldIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as a string, ignoring the case of "col".
 *  "col" must be a column name. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	const char *r=NULL;
	uint32_t rl;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldIgnoringCase(LVAL(row),
						SVAL(col));
			rl=cursor->getFieldLength(LVAL(row),
						SVAL(col));
		}
		if (r) {
			RET_STRINGL(const_cast<char *>(r),rl,1);
		}
	}
	RETURN_NULL();
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsInteger($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as an integer. "col" may be specified as the
 *  column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasinteger) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	int64_t r=0;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getFieldAsInteger(LVAL(row),
							LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldAsInteger(LVAL(row),
							SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDouble($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as a decimal. "col" may be specified as the
 *  column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdouble) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	double r=0.0;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getFieldAsDouble(LVAL(row),
							LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldAsDouble(LVAL(row),
							SVAL(col));
		}
		RETURN_DOUBLE(r);
	}
	RETURN_DOUBLE(0.0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsBoolean($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as a boolean. "col" may be specified as the
 *  column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasboolean) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	bool r=false;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getFieldAsBoolean(LVAL(row),
							LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldAsBoolean(LVAL(row),
							SVAL(col));
		}
		RETURN_BOOL(r);
	}
	RETURN_BOOL(false);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateYear($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the year component.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdateyear) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateYear(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateYear(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateYear(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateYear(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateMonth($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the month component.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatemonth) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateMonth(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMonth(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateMonth(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMonth(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateDay($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the day component.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdateday) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateDay(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateDay(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateDay(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateDay(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateHour($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the hour component.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatehour) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateHour(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateHour(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateHour(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateHour(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateMinute($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the minute component.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdateminute) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateMinute(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMinute(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateMinute(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMinute(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateSecond($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the second component.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatesecond) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateSecond(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateSecond(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateSecond(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateSecond(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateMicrosecond($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the microsecond
 *  component.  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatemicrosecond) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int32_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateMicrosecond(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMicrosecond(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateMicrosecond(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMicrosecond(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateIsNegative($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns whether the hour
 *  component is negative.  "col" may be specified as the column name or
 *  number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdateisnegative) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	bool r=false;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateIsNegative(
						LVAL(row),
						LVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateIsNegative(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_LONG) {
				convert_to_long_ex(col);
				r=cursor->getFieldAsDateIsNegative(
						LVAL(row),
						LVAL(col));
			} else if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateIsNegative(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsIntegerIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as an integer, ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasintegerignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	int64_t r=0;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldAsIntegerIgnoringCase(LVAL(row),
							SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDoubleIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as a decimal, ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdoubleignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	double r=0.0;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldAsDoubleIgnoringCase(LVAL(row),
							SVAL(col));
		}
		RETURN_DOUBLE(r);
	}
	RETURN_DOUBLE(0.0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsBooleanIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Returns the specified field as a boolean, ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasbooleanignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	bool r=false;
	if (ZEND_NUM_ARGS() != 3 ||
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldAsBooleanIgnoringCase(LVAL(row),
							SVAL(col));
		}
		RETURN_BOOL(r);
	}
	RETURN_BOOL(false);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateYearIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the year component,
 *  ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdateyearignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateYearIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateYearIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateMonthIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the month component,
 *  ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatemonthignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMonthIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMonthIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateDayIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the day component,
 *  ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatedayignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateDayIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateDayIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateHourIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the hour component,
 *  ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatehourignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateHourIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateHourIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateMinuteIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the minute component,
 *  ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdateminuteignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMinuteIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMinuteIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateSecondIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the second component,
 *  ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatesecondignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int16_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateSecondIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateSecondIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateMicrosecondIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns the microsecond
 *  component, ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdatemicrosecondignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	int32_t r=0;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMicrosecondIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateMicrosecondIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldAsDateIsNegativeIgnoringCase($sqlrcurref, $row, $col)
 *
 *  Interprets the specified field as a date and returns whether the hour
 *  component is negative, ignoring case of "col". */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldasdateisnegativeignoringcase) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	ZVAL ddmm;
	ZVAL yyyyddmm;
	ZVAL datedelimiters;
	bool r=false;
	if (ZEND_NUM_ARGS()==6 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzzzzz")
				&sqlrcur,
				&row,
				&col,
				&ddmm,
				&yyyyddmm,
				&datedelimiters) != FAILURE) {
		convert_to_long_ex(row);
		convert_to_long_ex(ddmm);
		convert_to_long_ex(yyyyddmm);
		convert_to_string_ex(datedelimiters);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateIsNegativeIgnoringCase(
						LVAL(row),
						SVAL(col),
						LVAL(ddmm)!=0,
						LVAL(yyyyddmm)!=0,
						SVAL(datedelimiters));
			}
			RETURN_LONG(r);
		}
	} else if (ZEND_NUM_ARGS()==3 &&
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) != FAILURE) {
		convert_to_long_ex(row);
		sqlrcursor *cursor=NULL;
		ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
		if (cursor) {
			if (TYPE(col)==IS_STRING) {
				convert_to_string_ex(col);
				r=cursor->getFieldAsDateIsNegativeIgnoringCase(
						LVAL(row),
						SVAL(col));
			}
			RETURN_LONG(r);
		}
	} else {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getFieldLength($sqlrcurref, $row, $col)
 *
 *  Returns the length of the specified field. "col" may be
 *  specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getfieldlength) {
	ZVAL sqlrcur;
	ZVAL row;
	ZVAL col;
	uint32_t r=0;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&row,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getFieldLength(LVAL(row),
							LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getFieldLength(LVAL(row),
							SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getRow($sqlrcurref, $row)
 *
 *  Returns an array of the values of the fields in the specified row. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getrow) {
	ZVAL sqlrcur;
	ZVAL row;
	const char * const *r;
	uint32_t *l;
	uint32_t i;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&row) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (!cursor) {
		RETURN_FALSE;
	}
	r=cursor->getRow(LVAL(row));
	l=cursor->getRowLengths(LVAL(row));
	if (!r) {
		RETURN_FALSE;
	}
	#ifdef ARRAY_INIT_CANT_FAIL
	array_init(return_value);
	#else
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}
	#endif
	for (i=0; i<cursor->colCount(); i++) {
		if (!r[i]) {
			ADD_NEXT_INDEX_NULL(return_value);
		} else {
			ADD_NEXT_INDEX_STRINGL(return_value,
						const_cast<char *>(r[i]),
						l[i],
						1);
		}
	}
}

/**
 *  call-seq:
 *  sqlrcur_getRowAssoc($sqlrcurref, $row)
 *
 *  Returns an associative array of the
 *  values of the fields in the specified row. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getrowassoc) {
	ZVAL sqlrcur;
	ZVAL row;
	const char * const *r;
	uint32_t *l;
	const char * const *rC;
	uint32_t i;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&row) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(row);

	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (!cursor) {
		RETURN_FALSE;
	}

	rC=cursor->getColumnNames();
	if (!rC) {
		RETURN_FALSE;
	}

	r=cursor->getRow(LVAL(row));
	l=cursor->getRowLengths(LVAL(row));
	if (!r) {
		RETURN_FALSE;
	}
	#ifdef ARRAY_INIT_CANT_FAIL
	array_init(return_value);
	#else
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}
	#endif
	for (i=0; i<cursor->colCount(); i++) {
		if (!r[i]) {
			ADD_ASSOC_NULL(return_value,const_cast<char *>(rC[i]));
		} else {
			ADD_ASSOC_STRINGL(return_value,
					const_cast<char *>(rC[i]),
					const_cast<char *>(r[i]),
					l[i],
					1);
		}
	}
}

/**
 *  call-seq:
 *  sqlrcur_getRowLengths($sqlrcurref, $row)
 *
 *  Returns an array of the lengths of the fields in the specified row. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getrowlengths) {
	ZVAL sqlrcur;
	ZVAL row;
	uint32_t *r;
	uint32_t i;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&row) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(row);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (!cursor) {
		RETURN_FALSE;
	}
	r=cursor->getRowLengths(LVAL(row));
	if (!r) {
		RETURN_FALSE;
	}
	#ifdef ARRAY_INIT_CANT_FAIL
	array_init(return_value);
	#else
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}
	#endif
	for (i=0; i<cursor->colCount(); i++) {
		add_next_index_long(return_value,r[i]);
	}
}

/**
 *  call-seq:
 *  sqlrcur_getRowLengthsAssoc($sqlrcurref, $row)
 *
 *  Returns an associative array of the
 *  lengths of the fields in the specified row. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getrowlengthsassoc) {
	ZVAL sqlrcur;
	ZVAL row;
	uint32_t *r;
	const char * const *rC;
	uint32_t i;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&row) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(row);

	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (!cursor) {
		RETURN_FALSE;
	}

	rC=cursor->getColumnNames();
	if (!rC) {
		RETURN_FALSE;
	}

	r=cursor->getRowLengths(LVAL(row));
	if (!r) {
		RETURN_FALSE;
	}
	#ifdef ARRAY_INIT_CANT_FAIL
	array_init(return_value);
	#else
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}
	#endif
	for (i=0; i<cursor->colCount(); i++) {
		add_assoc_long(return_value,const_cast<char *>(rC[i]),r[i]);
	}
}

/**
 *  call-seq:
 *  sqlrcur_getColumnNames($sqlrcurref)
 *
 *  Returns an array of the column names of the current result set. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnnames) {
	ZVAL sqlrcur;
	const char * const *r;
	uint32_t i;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (!cursor) {
		RETURN_FALSE;
	}
	r=cursor->getColumnNames();
	if (!r) {
		RETURN_FALSE;
	}
	#ifdef ARRAY_INIT_CANT_FAIL
	array_init(return_value);
	#else
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}
	#endif
	for (i=0; i<cursor->colCount(); i++) {
		ADD_NEXT_INDEX_STRING(return_value,const_cast<char *>(r[i]),1);
	}
}

/**
 *  call-seq:
 *  sqlrcur_getColumnName($sqlrcurref, $col)
 *
 *  Returns the name of the specified column. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnname) {
	ZVAL sqlrcur;
	ZVAL col;
	const char *r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(col);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getColumnName(LVAL(col));
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcur_getColumnType($sqlrcurref, $col)
 *
 *  Returns the type of the specified column.  "col" may be specified as the
 *  column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumntype) {
	ZVAL sqlrcur;
	ZVAL col;
	const char *r=NULL;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnType(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnType(SVAL(col));
		}
		if (r) {
			RET_STRING(const_cast<char *>(r),1);
		}
	}
	RETURN_FALSE;
}

/**
 *  call-seq:
 *  sqlrcur_getColumnLength($sqlrcurref, $col)
 *
 *  Returns the number of bytes required on the server to store the data for
 *  the specified column.  "col" may be specified as the column name or
 *  number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnlength) {
	ZVAL sqlrcur;
	ZVAL col;
	uint32_t r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnLength(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnLength(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnPrecision($sqlrcurref, $col)
 *
 *  Returns the precision of the specified column.  Precision is the total
 *  number of digits in a number.  eg: 123.45 has a precision of 5.  For
 *  non-numeric types, it's the number of characters in the string.  "col"
 *  may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnprecision) {
	ZVAL sqlrcur;
	ZVAL col;
	uint32_t r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnPrecision(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnPrecision(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnScale($sqlrcurref, $col)
 *
 *  Returns the scale of the specified column.  Scale is the total number of
 *  digits to the right of the decimal point in a number.  eg: 123.45 has a
 *  scale of 2.  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnscale) {
	ZVAL sqlrcur;
	ZVAL col;
	uint32_t r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnScale(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnScale(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsNullable($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column can contain nulls and 0 otherwise.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnisnullable) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsNullable(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsNullable(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsPrimaryKey($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column is a primary key and 0 otherwise.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnisprimarykey) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsPrimaryKey(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsPrimaryKey(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsUnique($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column is unique and 0 otherwise.  "col"
 *  may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnisunique) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsUnique(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsUnique(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsPartOfKey($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column is part of a composite key and 0
 *  otherwise.  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnispartofkey) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsPartOfKey(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsPartOfKey(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsUnsigned($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column is an unsigned number and 0 otherwise.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnisunsigned) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsUnsigned(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsUnsigned(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsZeroFilled($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column was created with the zero-fill flag and
 *  0 otherwise.  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumniszerofilled) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsZeroFilled(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsZeroFilled(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsBinary($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column contains binary data and 0 otherwise.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnisbinary) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsBinary(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsBinary(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getColumnIsAutoIncrement($sqlrcurref, $col)
 *
 *  Returns 1 if the specified column auto-increments and 0 otherwise.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getcolumnisautoincrement) {
	ZVAL sqlrcur;
	ZVAL col;
	bool r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getColumnIsAutoIncrement(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getColumnIsAutoIncrement(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_getLongest($sqlrcurref, $col)
 *
 *  Returns the length of the longest field in the specified column.
 *  "col" may be specified as the column name or number. */
DLEXPORT ZEND_FUNCTION(sqlrcur_getlongest) {
	ZVAL sqlrcur;
	ZVAL col;
	uint32_t r=0;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&col) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		if (TYPE(col)==IS_LONG) {
			convert_to_long_ex(col);
			r=cursor->getLongest(LVAL(col));
		} else if (TYPE(col)==IS_STRING) {
			convert_to_string_ex(col);
			r=cursor->getLongest(SVAL(col));
		}
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_suspendResultSet($sqlrcurref)
 *
 *  Tells the server to leave this result set open when the connection calls
 *  suspendSession() so that another connection can connect to it using
 *  resumeResultSet() after it calls resumeSession(). */
DLEXPORT ZEND_FUNCTION(sqlrcur_suspendresultset) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->suspendResultSet();
	}
}

/**
 *  call-seq:
 *  sqlrcur_getResultSetId($sqlrcurref)
 *
 *  Returns the internal ID of this result set.  This parameter may be passed
 *  to another cursor for use in the resumeResultSet() method.  Note: The
 *  value this method returns is only valid after a call to
 *  suspendResultSet(). */
DLEXPORT ZEND_FUNCTION(sqlrcur_getresultsetid) {
	ZVAL sqlrcur;
	uint16_t r;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->getResultSetId();
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_resumeResultSet($sqlrcurref, $id)
 *
 *  Resumes a result set previously left open using suspendSession().
 *  Returns 1 on success and 0 on failure. */
DLEXPORT ZEND_FUNCTION(sqlrcur_resumeresultset) {
	ZVAL sqlrcur;
	ZVAL id;
	bool r;
	if (ZEND_NUM_ARGS() != 2 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zz")
				&sqlrcur,
				&id) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(id);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->resumeResultSet(LVAL(id));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_resumeCachedResultSet($sqlrcurref, $id, $filename)
 *
 *  Resumes a result set previously left open using suspendSession() and
 *  continues caching the result set to "filename".  Returns 1 on success and 0
 *  on failure. */
DLEXPORT ZEND_FUNCTION(sqlrcur_resumecachedresultset) {
	ZVAL sqlrcur;
	ZVAL id;
	ZVAL filename;
	bool r;
	if (ZEND_NUM_ARGS() != 3 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("zzz")
				&sqlrcur,
				&id,
				&filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(id);
	convert_to_string_ex(filename);
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		r=cursor->resumeCachedResultSet(LVAL(id),
						SVAL(filename));
		RETURN_LONG(r);
	}
	RETURN_LONG(0);
}

/**
 *  call-seq:
 *  sqlrcur_closeResultSet($sqlrcurref)
 *
 *  Closes the current result set, if one is open.  Data
 *  that has been fetched already is still available but
 *  no more data may be fetched.  Server side resources
 *  for the result set are freed as well. */
DLEXPORT ZEND_FUNCTION(sqlrcur_closeresultset) {
	ZVAL sqlrcur;
	if (ZEND_NUM_ARGS() != 1 || 
		GET_PARAMETERS(
				ZEND_NUM_ARGS() TSRMLS_CC,
				PARAMS("z")
				&sqlrcur) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	sqlrcursor *cursor=NULL;
	ZEND_FETCH_RESOURCE(cursor,
				sqlrcursor *,
				sqlrcur,
				-1,
				"sqlrelay cursor",
				sqlrelay_cursor);
	if (cursor) {
		cursor->closeResultSet();
	}
}

// FIXME: flesh these out
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_alloc,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_free,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_setconnecttimeout,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getconnecttimeoutseconds,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getconnecttimeoutmicroseconds,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_setresponsetimeout,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getresponsetimeoutseconds,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getresponsetimeoutmicroseconds,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_setbindvariabledelimiters,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getbindvariabledelimiterquestionmarksupported,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getbindvariabledelimitercolonsupported,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getbindvariabledelimiteratsignsupported,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getbindvariabledelimiterdollarsignsupported,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_enablekerberos,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_enabletls,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_disableencryption,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_endsession,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_suspendsession,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getconnectionport,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getconnectionsocket,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_resumesession,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_errormessage,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_errornumber,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_debugon,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_debugoff,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getdebug,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_setdebugfile,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_setclientinfo,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getclientinfo,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_alloc,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_free,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_setresultsetbuffersize,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getresultsetbuffersize,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_dontgetcolumninfo,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumninfo,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_mixedcasecolumnnames,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_uppercasecolumnnames,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_lowercasecolumnnames,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_cachetofile,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_setcachettl,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcachefilename,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_cacheoff,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getdatabaselist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcataloglist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getschemalist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_gettabletypelist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_gettablelist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_gettypeinfolist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnlist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getprimarykeyslist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getkeyandindexlist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getprocedurelist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getprocedureparameterlist,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_sendquery,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_sendquerywithlength,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_sendfilequery,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_preparequery,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_preparequerywithlength,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_preparefilequery,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_substitution,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_clearbinds,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_countbindvariables,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_inputbind,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_inputbinddate,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_inputbindblob,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_inputbindclob,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_defineoutputbindstring,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_defineoutputbindinteger,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_defineoutputbinddouble,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_defineoutputbinddate,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_defineoutputbindblob,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_defineoutputbindclob,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_defineoutputbindcursor,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_substitutions,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_inputbinds,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_validatebinds,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_validbind,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_executequery,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_fetchfrombindcursor,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbindstring,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbindblob,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbindclob,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbindinteger,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddouble,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbindlength,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbindcursor,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddateyear,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddatemonth,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddateday,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddatehour,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddateminute,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddatesecond,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddatemicrosecond,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddatetz,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getoutputbinddateisnegative,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_opencachedresultset,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_colcount,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_rowcount,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_totalrows,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_affectedrows,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_firstrowindex,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_endofresultset,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_nextresultset,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_errormessage,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_errornumber,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getnullsasemptystrings,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getnullsasnulls,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfield,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldignoringcase,0,0,3)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasinteger,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdouble,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasboolean,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdateyear,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatemonth,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdateday,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatehour,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdateminute,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatesecond,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatemicrosecond,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdateisnegative,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasintegerignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdoubleignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasbooleanignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdateyearignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatemonthignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatedayignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatehourignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdateminuteignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatesecondignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdatemicrosecondignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldasdateisnegativeignoringcase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getfieldlength,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getrow,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getrowassoc,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getrowlengths,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getrowlengthsassoc,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnnames,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnname,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumntype,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnlength,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnprecision,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnscale,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnisnullable,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnisprimarykey,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnisunique,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnispartofkey,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnisunsigned,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumniszerofilled,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnisbinary,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getcolumnisautoincrement,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getlongest,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_getresultsetid,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_suspendresultset,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_resumeresultset,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_resumecachedresultset,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcur_closeresultset,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_ping,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_identify,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_selectdatabase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getcurrentdatabase,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getdatabaseisschema,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_selectcatalog,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getcurrentcatalog,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_selectschema,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getcurrentschema,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getcurrentuser,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getlastinsertid,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_autocommiton,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_autocommitoff,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getautocommit,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_begin,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_commit,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_rollback,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getInTransaction,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getDefaultTransactionModel,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_setTransactionModel,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getTransactionModel,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getDefaultIsolationLevel,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_setIsolationLevel,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getIsolationLevel,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_getDatabaseFeature,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_bindformat,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_nextvalformat,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_dbversion,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_dbhostname,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_dbipaddress,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_serverversion,0,0,0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlrcon_clientversion,0,0,0)
ZEND_END_ARG_INFO()


zend_function_entry sql_relay_functions[] = {
	ZEND_FE(sqlrcon_alloc,
		ARGINFO(arginfo_sqlrcon_alloc))
	ZEND_FE(sqlrcon_free,
		ARGINFO(arginfo_sqlrcon_free))
	ZEND_FE(sqlrcon_setconnecttimeout,
		ARGINFO(arginfo_sqlrcon_setconnecttimeout))
	ZEND_FE(sqlrcon_getconnecttimeoutseconds,
		ARGINFO(arginfo_sqlrcon_getconnecttimeoutseconds))
	ZEND_FE(sqlrcon_getconnecttimeoutmicroseconds,
		ARGINFO(arginfo_sqlrcon_getconnecttimeoutmicroseconds))
	ZEND_FE(sqlrcon_setresponsetimeout,
		ARGINFO(arginfo_sqlrcon_setresponsetimeout))
	ZEND_FE(sqlrcon_getresponsetimeoutseconds,
		ARGINFO(arginfo_sqlrcon_getresponsetimeoutseconds))
	ZEND_FE(sqlrcon_getresponsetimeoutmicroseconds,
		ARGINFO(arginfo_sqlrcon_getresponsetimeoutmicroseconds))
	ZEND_FE(sqlrcon_setbindvariabledelimiters,
		ARGINFO(arginfo_sqlrcon_setbindvariabledelimiters))
	ZEND_FE(sqlrcon_getbindvariabledelimiterquestionmarksupported,
	ARGINFO(arginfo_sqlrcon_getbindvariabledelimiterquestionmarksupported))
	ZEND_FE(sqlrcon_getbindvariabledelimitercolonsupported,
	ARGINFO(arginfo_sqlrcon_getbindvariabledelimitercolonsupported))
	ZEND_FE(sqlrcon_getbindvariabledelimiteratsignsupported,
	ARGINFO(arginfo_sqlrcon_getbindvariabledelimiteratsignsupported))
	ZEND_FE(sqlrcon_getbindvariabledelimiterdollarsignsupported,
	ARGINFO(arginfo_sqlrcon_getbindvariabledelimiterdollarsignsupported))
	ZEND_FE(sqlrcon_enablekerberos,
		ARGINFO(arginfo_sqlrcon_enablekerberos))
	ZEND_FE(sqlrcon_enabletls,
		ARGINFO(arginfo_sqlrcon_enabletls))
	ZEND_FE(sqlrcon_disableencryption,
		ARGINFO(arginfo_sqlrcon_disableencryption))
	ZEND_FE(sqlrcon_endsession,
		ARGINFO(arginfo_sqlrcon_endsession))
	ZEND_FE(sqlrcon_suspendsession,
		ARGINFO(arginfo_sqlrcon_suspendsession))
	ZEND_FE(sqlrcon_getconnectionport,
		ARGINFO(arginfo_sqlrcon_getconnectionport))
	ZEND_FE(sqlrcon_getconnectionsocket,
		ARGINFO(arginfo_sqlrcon_getconnectionsocket))
	ZEND_FE(sqlrcon_resumesession,
		ARGINFO(arginfo_sqlrcon_resumesession))
	ZEND_FE(sqlrcon_errormessage,
		ARGINFO(arginfo_sqlrcon_errormessage))
	ZEND_FE(sqlrcon_errornumber,
		ARGINFO(arginfo_sqlrcon_errornumber))
	ZEND_FE(sqlrcon_debugon,
		ARGINFO(arginfo_sqlrcon_debugon))
	ZEND_FE(sqlrcon_debugoff,
		ARGINFO(arginfo_sqlrcon_debugoff))
	ZEND_FE(sqlrcon_getdebug,
		ARGINFO(arginfo_sqlrcon_getdebug))
	ZEND_FE(sqlrcon_setdebugfile,
		ARGINFO(arginfo_sqlrcon_setdebugfile))
	ZEND_FE(sqlrcon_setclientinfo,
		ARGINFO(arginfo_sqlrcon_setclientinfo))
	ZEND_FE(sqlrcon_getclientinfo,
		ARGINFO(arginfo_sqlrcon_getclientinfo))
	ZEND_FE(sqlrcur_alloc,
		ARGINFO(arginfo_sqlrcur_alloc))
	ZEND_FE(sqlrcur_free,
		ARGINFO(arginfo_sqlrcur_free))
	ZEND_FE(sqlrcur_setresultsetbuffersize,
		ARGINFO(arginfo_sqlrcur_setresultsetbuffersize))
	ZEND_FE(sqlrcur_getresultsetbuffersize,
		ARGINFO(arginfo_sqlrcur_getresultsetbuffersize))
	ZEND_FE(sqlrcur_dontgetcolumninfo,
		ARGINFO(arginfo_sqlrcur_dontgetcolumninfo))
	ZEND_FE(sqlrcur_getcolumninfo,
		ARGINFO(arginfo_sqlrcur_getcolumninfo))
	ZEND_FE(sqlrcur_mixedcasecolumnnames,
		ARGINFO(arginfo_sqlrcur_mixedcasecolumnnames))
	ZEND_FE(sqlrcur_uppercasecolumnnames,
		ARGINFO(arginfo_sqlrcur_uppercasecolumnnames))
	ZEND_FE(sqlrcur_lowercasecolumnnames,
		ARGINFO(arginfo_sqlrcur_lowercasecolumnnames))
	ZEND_FE(sqlrcur_cachetofile,
		ARGINFO(arginfo_sqlrcur_cachetofile))
	ZEND_FE(sqlrcur_setcachettl,
		ARGINFO(arginfo_sqlrcur_setcachettl))
	ZEND_FE(sqlrcur_getcachefilename,
		ARGINFO(arginfo_sqlrcur_getcachefilename))
	ZEND_FE(sqlrcur_cacheoff,
		ARGINFO(arginfo_sqlrcur_cacheoff))
	ZEND_FE(sqlrcur_getdatabaselist,
		ARGINFO(arginfo_sqlrcur_getdatabaselist))
	ZEND_FE(sqlrcur_getcataloglist,
		ARGINFO(arginfo_sqlrcur_getcataloglist))
	ZEND_FE(sqlrcur_getschemalist,
		ARGINFO(arginfo_sqlrcur_getschemalist))
	ZEND_FE(sqlrcur_gettabletypelist,
		ARGINFO(arginfo_sqlrcur_gettabletypelist))
	ZEND_FE(sqlrcur_gettablelist,
		ARGINFO(arginfo_sqlrcur_gettablelist))
	ZEND_FE(sqlrcur_gettypeinfolist,
		ARGINFO(arginfo_sqlrcur_gettypeinfolist))
	ZEND_FE(sqlrcur_getcolumnlist,
		ARGINFO(arginfo_sqlrcur_getcolumnlist))
	ZEND_FE(sqlrcur_getprimarykeyslist,
		ARGINFO(arginfo_sqlrcur_getprimarykeyslist))
	ZEND_FE(sqlrcur_getkeyandindexlist,
		ARGINFO(arginfo_sqlrcur_getkeyandindexlist))
	ZEND_FE(sqlrcur_getprocedurelist,
		ARGINFO(arginfo_sqlrcur_getprocedurelist))
	ZEND_FE(sqlrcur_getprocedureparameterlist,
		ARGINFO(arginfo_sqlrcur_getprocedureparameterlist))
	ZEND_FE(sqlrcur_sendquery,
		ARGINFO(arginfo_sqlrcur_sendquery))
	ZEND_FE(sqlrcur_sendquerywithlength,
		ARGINFO(arginfo_sqlrcur_sendquerywithlength))
	ZEND_FE(sqlrcur_sendfilequery,
		ARGINFO(arginfo_sqlrcur_sendfilequery))
	ZEND_FE(sqlrcur_preparequery,
		ARGINFO(arginfo_sqlrcur_preparequery))
	ZEND_FE(sqlrcur_preparequerywithlength,
		ARGINFO(arginfo_sqlrcur_preparequerywithlength))
	ZEND_FE(sqlrcur_preparefilequery,
		ARGINFO(arginfo_sqlrcur_preparefilequery))
	ZEND_FE(sqlrcur_substitution,
		ARGINFO(arginfo_sqlrcur_substitution))
	ZEND_FE(sqlrcur_clearbinds,
		ARGINFO(arginfo_sqlrcur_clearbinds))
	ZEND_FE(sqlrcur_countbindvariables,
		ARGINFO(arginfo_sqlrcur_countbindvariables))
	ZEND_FE(sqlrcur_inputbind,
		ARGINFO(arginfo_sqlrcur_inputbind))
	ZEND_FE(sqlrcur_inputbinddate,
		ARGINFO(arginfo_sqlrcur_inputbinddate))
	ZEND_FE(sqlrcur_inputbindblob,
		ARGINFO(arginfo_sqlrcur_inputbindblob))
	ZEND_FE(sqlrcur_inputbindclob,
		ARGINFO(arginfo_sqlrcur_inputbindclob))
	ZEND_FE(sqlrcur_defineoutputbindstring,
		ARGINFO(arginfo_sqlrcur_defineoutputbindstring))
	ZEND_FE(sqlrcur_defineoutputbindinteger,
		ARGINFO(arginfo_sqlrcur_defineoutputbindinteger))
	ZEND_FE(sqlrcur_defineoutputbinddouble,
		ARGINFO(arginfo_sqlrcur_defineoutputbinddouble))
	ZEND_FE(sqlrcur_defineoutputbinddate,
		ARGINFO(arginfo_sqlrcur_defineoutputbinddate))
	ZEND_FE(sqlrcur_defineoutputbindblob,
		ARGINFO(arginfo_sqlrcur_defineoutputbindblob))
	ZEND_FE(sqlrcur_defineoutputbindclob,
		ARGINFO(arginfo_sqlrcur_defineoutputbindclob))
	ZEND_FE(sqlrcur_defineoutputbindcursor,
		ARGINFO(arginfo_sqlrcur_defineoutputbindcursor))
	ZEND_FE(sqlrcur_substitutions,
		ARGINFO(arginfo_sqlrcur_substitutions))
	ZEND_FE(sqlrcur_inputbinds,
		ARGINFO(arginfo_sqlrcur_inputbinds))
	ZEND_FE(sqlrcur_validatebinds,
		ARGINFO(arginfo_sqlrcur_validatebinds))
	ZEND_FE(sqlrcur_validbind,
		ARGINFO(arginfo_sqlrcur_validbind))
	ZEND_FE(sqlrcur_executequery,
		ARGINFO(arginfo_sqlrcur_executequery))
	ZEND_FE(sqlrcur_fetchfrombindcursor,
		ARGINFO(arginfo_sqlrcur_fetchfrombindcursor))
	ZEND_FE(sqlrcur_getoutputbindstring,
		ARGINFO(arginfo_sqlrcur_getoutputbindstring))
	ZEND_FE(sqlrcur_getoutputbindblob,
		ARGINFO(arginfo_sqlrcur_getoutputbindblob))
	ZEND_FE(sqlrcur_getoutputbindclob,
		ARGINFO(arginfo_sqlrcur_getoutputbindclob))
	ZEND_FE(sqlrcur_getoutputbindinteger,
		ARGINFO(arginfo_sqlrcur_getoutputbindinteger))
	ZEND_FE(sqlrcur_getoutputbinddouble,
		ARGINFO(arginfo_sqlrcur_getoutputbinddouble))
	ZEND_FE(sqlrcur_getoutputbindlength,
		ARGINFO(arginfo_sqlrcur_getoutputbindlength))
	ZEND_FE(sqlrcur_getoutputbindcursor,
		ARGINFO(arginfo_sqlrcur_getoutputbindcursor))
	ZEND_FE(sqlrcur_getoutputbinddateyear,
		ARGINFO(arginfo_sqlrcur_getoutputbinddateyear))
	ZEND_FE(sqlrcur_getoutputbinddatemonth,
		ARGINFO(arginfo_sqlrcur_getoutputbinddatemonth))
	ZEND_FE(sqlrcur_getoutputbinddateday,
		ARGINFO(arginfo_sqlrcur_getoutputbinddateday))
	ZEND_FE(sqlrcur_getoutputbinddatehour,
		ARGINFO(arginfo_sqlrcur_getoutputbinddatehour))
	ZEND_FE(sqlrcur_getoutputbinddateminute,
		ARGINFO(arginfo_sqlrcur_getoutputbinddateminute))
	ZEND_FE(sqlrcur_getoutputbinddatesecond,
		ARGINFO(arginfo_sqlrcur_getoutputbinddatesecond))
	ZEND_FE(sqlrcur_getoutputbinddatemicrosecond,
		ARGINFO(arginfo_sqlrcur_getoutputbinddatemicrosecond))
	ZEND_FE(sqlrcur_getoutputbinddatetz,
		ARGINFO(arginfo_sqlrcur_getoutputbinddatetz))
	ZEND_FE(sqlrcur_getoutputbinddateisnegative,
		ARGINFO(arginfo_sqlrcur_getoutputbinddateisnegative))
	ZEND_FE(sqlrcur_opencachedresultset,
		ARGINFO(arginfo_sqlrcur_opencachedresultset))
	ZEND_FE(sqlrcur_colcount,
		ARGINFO(arginfo_sqlrcur_colcount))
	ZEND_FE(sqlrcur_rowcount,
		ARGINFO(arginfo_sqlrcur_rowcount))
	ZEND_FE(sqlrcur_totalrows,
		ARGINFO(arginfo_sqlrcur_totalrows))
	ZEND_FE(sqlrcur_affectedrows,
		ARGINFO(arginfo_sqlrcur_affectedrows))
	ZEND_FE(sqlrcur_firstrowindex,
		ARGINFO(arginfo_sqlrcur_firstrowindex))
	ZEND_FE(sqlrcur_endofresultset,
		ARGINFO(arginfo_sqlrcur_endofresultset))
	ZEND_FE(sqlrcur_nextresultset,
		ARGINFO(arginfo_sqlrcur_nextresultset))
	ZEND_FE(sqlrcur_errormessage,
		ARGINFO(arginfo_sqlrcur_errormessage))
	ZEND_FE(sqlrcur_errornumber,
		ARGINFO(arginfo_sqlrcur_errornumber))
	ZEND_FE(sqlrcur_getnullsasemptystrings,
		ARGINFO(arginfo_sqlrcur_getnullsasemptystrings))
	ZEND_FE(sqlrcur_getnullsasnulls,
		ARGINFO(arginfo_sqlrcur_getnullsasnulls))
	ZEND_FE(sqlrcur_getfield,
		ARGINFO(arginfo_sqlrcur_getfield))
	ZEND_FE(sqlrcur_getfieldignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldignoringcase))
	ZEND_FE(sqlrcur_getfieldasinteger,
		ARGINFO(arginfo_sqlrcur_getfieldasinteger))
	ZEND_FE(sqlrcur_getfieldasdouble,
		ARGINFO(arginfo_sqlrcur_getfieldasdouble))
	ZEND_FE(sqlrcur_getfieldasboolean,
		ARGINFO(arginfo_sqlrcur_getfieldasboolean))
	ZEND_FE(sqlrcur_getfieldasdateyear,
		ARGINFO(arginfo_sqlrcur_getfieldasdateyear))
	ZEND_FE(sqlrcur_getfieldasdatemonth,
		ARGINFO(arginfo_sqlrcur_getfieldasdatemonth))
	ZEND_FE(sqlrcur_getfieldasdateday,
		ARGINFO(arginfo_sqlrcur_getfieldasdateday))
	ZEND_FE(sqlrcur_getfieldasdatehour,
		ARGINFO(arginfo_sqlrcur_getfieldasdatehour))
	ZEND_FE(sqlrcur_getfieldasdateminute,
		ARGINFO(arginfo_sqlrcur_getfieldasdateminute))
	ZEND_FE(sqlrcur_getfieldasdatesecond,
		ARGINFO(arginfo_sqlrcur_getfieldasdatesecond))
	ZEND_FE(sqlrcur_getfieldasdatemicrosecond,
		ARGINFO(arginfo_sqlrcur_getfieldasdatemicrosecond))
	ZEND_FE(sqlrcur_getfieldasdateisnegative,
		ARGINFO(arginfo_sqlrcur_getfieldasdateisnegative))
	ZEND_FE(sqlrcur_getfieldasintegerignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasintegerignoringcase))
	ZEND_FE(sqlrcur_getfieldasdoubleignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdoubleignoringcase))
	ZEND_FE(sqlrcur_getfieldasbooleanignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasbooleanignoringcase))
	ZEND_FE(sqlrcur_getfieldasdateyearignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdateyearignoringcase))
	ZEND_FE(sqlrcur_getfieldasdatemonthignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdatemonthignoringcase))
	ZEND_FE(sqlrcur_getfieldasdatedayignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdatedayignoringcase))
	ZEND_FE(sqlrcur_getfieldasdatehourignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdatehourignoringcase))
	ZEND_FE(sqlrcur_getfieldasdateminuteignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdateminuteignoringcase))
	ZEND_FE(sqlrcur_getfieldasdatesecondignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdatesecondignoringcase))
	ZEND_FE(sqlrcur_getfieldasdatemicrosecondignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdatemicrosecondignoringcase))
	ZEND_FE(sqlrcur_getfieldasdateisnegativeignoringcase,
		ARGINFO(arginfo_sqlrcur_getfieldasdateisnegativeignoringcase))
	ZEND_FE(sqlrcur_getfieldlength,
		ARGINFO(arginfo_sqlrcur_getfieldlength))
	ZEND_FE(sqlrcur_getrow,
		ARGINFO(arginfo_sqlrcur_getrow))
	ZEND_FE(sqlrcur_getrowassoc,
		ARGINFO(arginfo_sqlrcur_getrowassoc))
	ZEND_FE(sqlrcur_getrowlengths,
		ARGINFO(arginfo_sqlrcur_getrowlengths))
	ZEND_FE(sqlrcur_getrowlengthsassoc,
		ARGINFO(arginfo_sqlrcur_getrowlengthsassoc))
	ZEND_FE(sqlrcur_getcolumnnames,
		ARGINFO(arginfo_sqlrcur_getcolumnnames))
	ZEND_FE(sqlrcur_getcolumnname,
		ARGINFO(arginfo_sqlrcur_getcolumnname))
	ZEND_FE(sqlrcur_getcolumntype,
		ARGINFO(arginfo_sqlrcur_getcolumntype))
	ZEND_FE(sqlrcur_getcolumnlength,
		ARGINFO(arginfo_sqlrcur_getcolumnlength))
	ZEND_FE(sqlrcur_getcolumnprecision,
		ARGINFO(arginfo_sqlrcur_getcolumnprecision))
	ZEND_FE(sqlrcur_getcolumnscale,
		ARGINFO(arginfo_sqlrcur_getcolumnscale))
	ZEND_FE(sqlrcur_getcolumnisnullable,
		ARGINFO(arginfo_sqlrcur_getcolumnisnullable))
	ZEND_FE(sqlrcur_getcolumnisprimarykey,
		ARGINFO(arginfo_sqlrcur_getcolumnisprimarykey))
	ZEND_FE(sqlrcur_getcolumnisunique,
		ARGINFO(arginfo_sqlrcur_getcolumnisunique))
	ZEND_FE(sqlrcur_getcolumnispartofkey,
		ARGINFO(arginfo_sqlrcur_getcolumnispartofkey))
	ZEND_FE(sqlrcur_getcolumnisunsigned,
		ARGINFO(arginfo_sqlrcur_getcolumnisunsigned))
	ZEND_FE(sqlrcur_getcolumniszerofilled,
		ARGINFO(arginfo_sqlrcur_getcolumniszerofilled))
	ZEND_FE(sqlrcur_getcolumnisbinary,
		ARGINFO(arginfo_sqlrcur_getcolumnisbinary))
	ZEND_FE(sqlrcur_getcolumnisautoincrement,
		ARGINFO(arginfo_sqlrcur_getcolumnisautoincrement))
	ZEND_FE(sqlrcur_getlongest,
		ARGINFO(arginfo_sqlrcur_getlongest))
	ZEND_FE(sqlrcur_getresultsetid,
		ARGINFO(arginfo_sqlrcur_getresultsetid))
	ZEND_FE(sqlrcur_suspendresultset,
		ARGINFO(arginfo_sqlrcur_suspendresultset))
	ZEND_FE(sqlrcur_resumeresultset,
		ARGINFO(arginfo_sqlrcur_resumeresultset))
	ZEND_FE(sqlrcur_resumecachedresultset,
		ARGINFO(arginfo_sqlrcur_resumecachedresultset))
	ZEND_FE(sqlrcur_closeresultset,
		ARGINFO(arginfo_sqlrcur_closeresultset))
	ZEND_FE(sqlrcon_ping,
		ARGINFO(arginfo_sqlrcon_ping))
	ZEND_FE(sqlrcon_identify,
		ARGINFO(arginfo_sqlrcon_identify))
	ZEND_FE(sqlrcon_selectdatabase,
		ARGINFO(arginfo_sqlrcon_selectdatabase))
	ZEND_FE(sqlrcon_getcurrentdatabase,
		ARGINFO(arginfo_sqlrcon_getcurrentdatabase))
	ZEND_FE(sqlrcon_selectcatalog,
		ARGINFO(arginfo_sqlrcon_selectcatalog))
	ZEND_FE(sqlrcon_getcurrentcatalog,
		ARGINFO(arginfo_sqlrcon_getcurrentcatalog))
	ZEND_FE(sqlrcon_selectschema,
		ARGINFO(arginfo_sqlrcon_selectschema))
	ZEND_FE(sqlrcon_getcurrentschema,
		ARGINFO(arginfo_sqlrcon_getcurrentschema))
	ZEND_FE(sqlrcon_getdatabaseisschema,
		ARGINFO(arginfo_sqlrcon_getdatabaseisschema))
	ZEND_FE(sqlrcon_getcurrentuser,
		ARGINFO(arginfo_sqlrcon_getcurrentuser))
	ZEND_FE(sqlrcon_getlastinsertid,
		ARGINFO(arginfo_sqlrcon_getlastinsertid))
	ZEND_FE(sqlrcon_autocommiton,
		ARGINFO(arginfo_sqlrcon_autocommiton))
	ZEND_FE(sqlrcon_autocommitoff,
		ARGINFO(arginfo_sqlrcon_autocommitoff))
	ZEND_FE(sqlrcon_getautocommit,
		ARGINFO(arginfo_sqlrcon_getautocommit))
	ZEND_FE(sqlrcon_begin,
		ARGINFO(arginfo_sqlrcon_begin))
	ZEND_FE(sqlrcon_commit,
		ARGINFO(arginfo_sqlrcon_commit))
	ZEND_FE(sqlrcon_rollback,
		ARGINFO(arginfo_sqlrcon_rollback))
	ZEND_FE(sqlrcon_getInTransaction,
		ARGINFO(arginfo_sqlrcon_getInTransaction))
	ZEND_FE(sqlrcon_getDefaultTransactionModel,
		ARGINFO(arginfo_sqlrcon_getDefaultTransactionModel))
	ZEND_FE(sqlrcon_setTransactionModel,
		ARGINFO(arginfo_sqlrcon_setTransactionModel))
	ZEND_FE(sqlrcon_getTransactionModel,
		ARGINFO(arginfo_sqlrcon_getTransactionModel))
	ZEND_FE(sqlrcon_getDefaultIsolationLevel,
		ARGINFO(arginfo_sqlrcon_getDefaultIsolationLevel))
	ZEND_FE(sqlrcon_setIsolationLevel,
		ARGINFO(arginfo_sqlrcon_setIsolationLevel))
	ZEND_FE(sqlrcon_getIsolationLevel,
		ARGINFO(arginfo_sqlrcon_getIsolationLevel))
	ZEND_FE(sqlrcon_getDatabaseFeature,
		ARGINFO(arginfo_sqlrcon_getDatabaseFeature))
	ZEND_FE(sqlrcon_bindformat,
		ARGINFO(arginfo_sqlrcon_bindformat))
	ZEND_FE(sqlrcon_nextvalformat,
		ARGINFO(arginfo_sqlrcon_nextvalformat))
	ZEND_FE(sqlrcon_dbversion,
		ARGINFO(arginfo_sqlrcon_dbversion))
	ZEND_FE(sqlrcon_dbhostname,
		ARGINFO(arginfo_sqlrcon_dbhostname))
	ZEND_FE(sqlrcon_dbipaddress,
		ARGINFO(arginfo_sqlrcon_dbipaddress))
	ZEND_FE(sqlrcon_serverversion,
		ARGINFO(arginfo_sqlrcon_serverversion))
	ZEND_FE(sqlrcon_clientversion,
		ARGINFO(arginfo_sqlrcon_clientversion))
	{NULL,NULL,NULL}
};

zend_module_entry sql_relay_module_entry = {
	#if ZEND_MODULE_API_NO >= 20010901
		STANDARD_MODULE_HEADER,
	#endif	
	"sql_relay",
	sql_relay_functions,
	// extension-wide startup function
#ifdef ZEND_MODULE_STARTUP_N
	ZEND_MODULE_STARTUP_N(sqlrelay),
#else
	NULL,
#endif
	// extension-wide shutdown function
	NULL,
	// per-request startup function
	NULL,
	// per-request shutdown function
	NULL,
	NULL,
	#if ZEND_MODULE_API_NO >= 20010901
		SQLR_VERSION,
	#endif	
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(sql_relay)

}

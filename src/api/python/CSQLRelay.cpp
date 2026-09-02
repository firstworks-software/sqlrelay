/* Copyright (c) 2000 Roman Milner
   See the file COPYING for more information */

#ifdef __CYGWIN__
        #include <windows.h>
#endif

// for Python 3.10+ this must be defined BEFORE including Python.h
#include "../../../config.h"
#ifdef SQLRELAY_NEED_PY_SSIZE_T_CLEAN
        #define PY_SSIZE_T_CLEAN 1
#endif

#include <Python.h>

#include <sqlrelay/sqlrclient.h>
#include <rudiments/character.h>

#if PY_MAJOR_VERSION > 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION >= 3)
        #define SUPPORTS_UNSIGNED 1
#endif

#if PY_MAJOR_VERSION >= 3
        #define PyString_Check PyUnicode_Check
#if PY_MAJOR_VERSION > 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 3)
        #define PyString_AsString(a) PyUnicode_AsUTF8AndSize(a,NULL)
#else
        #define PyString_AsString(a) PyBytes_AS_STRING(PyUnicode_AsEncodedString(a,"utf-8","strict"))
#endif
        #define PyInt_Check PyLong_Check
        #define PyInt_AsLong PyLong_AsLong
#else
        #define PyBytes_Check PyString_Check
        #define PyBytes_AsString PyString_AsString
#endif

#ifndef PY_SSIZE_T_MIN
	typedef int Py_ssize_t;
	#define PY_SSIZE_T_MAX INT_MAX
	#define PY_SSIZE_T_MIN INT_MIN
#endif


extern "C" {

#define NEED_IS_NUMBER_TYPE_CHAR 1
#define NEED_IS_FLOAT_TYPE_CHAR 1
#define NEED_IS_BIT_TYPE_CHAR 1
#define NEED_BIT_STRING_TO_LONG 1
#define NEED_IS_BOOL_TYPE_CHAR 1
#define NEED_IS_BINARY_TYPE_CHAR 1
#define NEED_IS_DATETIME_TYPE_CHAR 1
#include <datatypes.h>

bool usenumeric=false;
PyObject *decimalmodule=NULL;
PyObject *decimal=NULL;

static PyObject *getNumericFieldsAsStrings(PyObject *self, PyObject *args) {
  usenumeric=false;
  return Py_BuildValue("h", 0);
}

static PyObject *getNumericFieldsAsNumbers(PyObject *self, PyObject *args) {
  usenumeric=true;
  return Py_BuildValue("h", 0);
}

static PyObject *buildConstPyString(const char *str,
#ifdef PY_SSIZE_T_CLEAN
ssize_t len,
#else
uint32_t len,
#endif
bool binary) {
  return Py_BuildValue(
#if PY_MAJOR_VERSION >= 3
            (binary)?"y#":
#endif
            "s#",
            str, len);
}

static PyObject *buildConstPyStringT(const char *str,
#ifdef PY_SSIZE_T_CLEAN
ssize_t len,
#else
uint32_t len,
#endif
const char *type) {
  return buildConstPyString(str,len,isBinaryTypeChar(type) && !isDateTimeTypeChar(type));
}

static PyObject *sqlrcon_alloc(PyObject *self, PyObject *args) {
  sqlrconnection *sqlrcon;
  char *host;
  char *user;
  char *password;
  char *socket;
  uint16_t port;
  int32_t retrytime;
  int32_t tries;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "sHzzzii",
#else
        "shzzzii",
#endif
        &host, &port, &socket, &user, &password, &retrytime, &tries))
    return NULL;
  sqlrcon = new sqlrconnection(host, port, socket, user, password,
                                                retrytime, tries, true);
  return Py_BuildValue("l", (long)sqlrcon);
}

static PyObject *sqlrcon_free(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  delete ((sqlrconnection *)sqlrcon);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *setConnectTimeout(PyObject *self, PyObject *args) {
  long sqlrcon;
  int32_t timeoutsec;
  int32_t timeoutusec;
  if (!PyArg_ParseTuple(args, "lii", &sqlrcon, &timeoutsec, &timeoutusec))
    return NULL;
  ((sqlrconnection *)sqlrcon)->setConnectTimeout(timeoutsec,timeoutusec);
  return Py_BuildValue("h", 0);
}

static PyObject *getConnectTimeoutSeconds(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("i", ((sqlrconnection *)sqlrcon)->getConnectTimeoutSeconds());
}

static PyObject *getConnectTimeoutMicroseconds(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("i", ((sqlrconnection *)sqlrcon)->getConnectTimeoutMicroseconds());
}

static PyObject *setResponseTimeout(PyObject *self, PyObject *args) {
  long sqlrcon;
  int32_t timeoutsec;
  int32_t timeoutusec;
  if (!PyArg_ParseTuple(args, "lii", &sqlrcon, &timeoutsec, &timeoutusec))
    return NULL;
  ((sqlrconnection *)sqlrcon)->setResponseTimeout(timeoutsec,timeoutusec);
  return Py_BuildValue("h", 0);
}

static PyObject *getResponseTimeoutSeconds(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("i", ((sqlrconnection *)sqlrcon)->getResponseTimeoutSeconds());
}

static PyObject *getResponseTimeoutMicroseconds(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("i", ((sqlrconnection *)sqlrcon)->getResponseTimeoutMicroseconds());
}

static PyObject *setBindVariableDelimiters(PyObject *self, PyObject *args) {
  long sqlrcon;
  char *delimiters;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &delimiters))
    return NULL;
  ((sqlrconnection *)sqlrcon)->setBindVariableDelimiters(delimiters);
  return Py_BuildValue("h", 0);
}

static PyObject *getBindVariableDelimiterQuestionMarkSupported(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  rc=((sqlrconnection *)sqlrcon)->getBindVariableDelimiterQuestionMarkSupported();
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getBindVariableDelimiterColonSupported(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  rc=((sqlrconnection *)sqlrcon)->getBindVariableDelimiterColonSupported();
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getBindVariableDelimiterAtSignSupported(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  rc=((sqlrconnection *)sqlrcon)->getBindVariableDelimiterAtSignSupported();
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getBindVariableDelimiterDollarSignSupported(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  rc=((sqlrconnection *)sqlrcon)->getBindVariableDelimiterDollarSignSupported();
  return Py_BuildValue("h", (short)rc);
}

static PyObject *enableKerberos(PyObject *self, PyObject *args) {
  long sqlrcon;
  char *service;
  char *mech;
  char *flags;
  if (!PyArg_ParseTuple(args, "lzzz", &sqlrcon, &service, &mech, &flags))
    return NULL;
  ((sqlrconnection *)sqlrcon)->enableKerberos(service,mech,flags);
  return Py_BuildValue("h", 0);
}

static PyObject *enableTls(PyObject *self, PyObject *args) {
  long sqlrcon;
  char *version;
  char *cert;
  char *password;
  char *ciphers;
  char *validate;
  char *ca;
  uint16_t depth;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lzzzzzzH",
#else
        "lzzzzzzh",
#endif
        &sqlrcon, &version, &cert, &password, &ciphers, &validate, &ca, &depth))
    return NULL;
  ((sqlrconnection *)sqlrcon)->enableTls(version,cert,password,ciphers,validate,ca,depth);
  return Py_BuildValue("h", 0);
}

static PyObject *disableEncryption(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrconnection *)sqlrcon)->disableEncryption();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *endSession(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrconnection *)sqlrcon)->endSession();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *suspendSession(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->suspendSession();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getConnectionPort(PyObject *self, PyObject *args) {
  long sqlrcon;
  short rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  rc=((sqlrconnection *)sqlrcon)->getConnectionPort();
  return Py_BuildValue("h", rc);
}

static PyObject *getConnectionSocket(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  rc=((sqlrconnection *)sqlrcon)->getConnectionSocket();
  return Py_BuildValue("s", rc);
}

static PyObject *resumeSession(PyObject *self, PyObject *args) {
  long sqlrcon;
  uint16_t port;
  char *socket;
  bool rc;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lHs",
#else
        "lhs",
#endif
        &sqlrcon, &port, &socket))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->resumeSession(port,socket);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *ping(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->ping();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *identify(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->identify();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *dbVersion(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->dbVersion();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *dbHostName(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->dbHostName();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *dbIpAddress(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->dbIpAddress();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *serverVersion(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->serverVersion();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *clientVersion(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->clientVersion();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *bindFormat(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->bindFormat();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *nextvalFormat(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->nextvalFormat();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *selectDatabase(PyObject *self, PyObject *args) {
  char *db;
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &db))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->selectDatabase(db);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getCurrentDatabase(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getCurrentDatabase();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *getDatabaseIsSchema(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getDatabaseIsSchema();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *selectCatalog(PyObject *self, PyObject *args) {
  char *catalog;
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &catalog))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->selectCatalog(catalog);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getCurrentCatalog(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getCurrentCatalog();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *selectSchema(PyObject *self, PyObject *args) {
  char *schema;
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &schema))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->selectSchema(schema);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getCurrentSchema(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getCurrentSchema();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *getCurrentUser(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->getCurrentUser());
}

static PyObject *getLastInsertId(PyObject *self, PyObject *args) {
  long sqlrcon;
  uint64_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getLastInsertId();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("l", rc);
}

static PyObject *autoCommitOn(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->autoCommitOn();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *autoCommitOff(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->autoCommitOff();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getAutoCommit(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getAutoCommit();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *begin(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->begin();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *commit(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->commit();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *rollback(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->rollback();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getInTransaction(PyObject *self, PyObject *args) {
  long sqlrcon;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getInTransaction();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getDefaultTransactionModel(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->getDefaultTransactionModel());
}

static PyObject *setTransactionModel(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *txmodel;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &txmodel))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->setTransactionModel(txmodel);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getTransactionModel(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->getTransactionModel());
}

static PyObject *getDefaultIsolationLevel(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->getDefaultIsolationLevel());
}

static PyObject *setIsolationLevel(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *isolationlevel;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &isolationlevel))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->setIsolationLevel(isolationlevel);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getIsolationLevel(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->getIsolationLevel());
}

static PyObject *getDatabaseFeature(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *feature;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &feature))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->getDatabaseFeature(feature));
}

static PyObject *connectionErrorMessage(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->errorMessage());
}

static PyObject *connectionErrorNumber(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  // FIXME: lame, python doesn't support building values from int64_t's
  return Py_BuildValue("l", (long)((sqlrconnection *)sqlrcon)->errorNumber());
}

static PyObject *connectionErrorSqlState(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("s", ((sqlrconnection *)sqlrcon)->errorSqlState());
}

static PyObject *debugOn(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  ((sqlrconnection *)sqlrcon)->debugOn();
  return Py_BuildValue("h", 0);
}

static PyObject *debugOff(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  ((sqlrconnection *)sqlrcon)->debugOff();
  return Py_BuildValue("h", 0);
}

static PyObject *getDebug(PyObject *self, PyObject *args) {
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  return Py_BuildValue("h", (short)((sqlrconnection *)sqlrcon)->getDebug());
}

static PyObject *setDebugFile(PyObject *self, PyObject *args) {
  char *filename;
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &filename))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrconnection *)sqlrcon)->setDebugFile(filename);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *setClientInfo(PyObject *self, PyObject *args) {
  char *clientinfo;
  long sqlrcon;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcon, &clientinfo))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrconnection *)sqlrcon)->setClientInfo(clientinfo);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *getClientInfo(PyObject *self, PyObject *args) {
  long sqlrcon;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrconnection *)sqlrcon)->getClientInfo();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", rc);
}

static PyObject *isYes(PyObject *self, PyObject *args) {
  char *str;
  bool rc;
  if (!PyArg_ParseTuple(args, "s", &str))
    return NULL;
  rc=charstring::isYes(str);
  return Py_BuildValue("h", (short)rc);
}

static PyObject *isNo(PyObject *self, PyObject *args) {
  char *str;
  bool rc;
  if (!PyArg_ParseTuple(args, "s", &str))
    return NULL;
  rc=charstring::isNo(str);
  return Py_BuildValue("h", (short)rc);
}

static PyObject *sqlrcur_alloc(PyObject *self, PyObject *args) {
  long sqlrcon;
  sqlrcursor *sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcon))
    return NULL;
  sqlrcur = new sqlrcursor((sqlrconnection *)sqlrcon,true);
  return Py_BuildValue("l", (long)sqlrcur);
}

static PyObject *sqlrcur_free(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  delete ((sqlrcursor *)sqlrcur);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *setResultSetBufferSize(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t rows;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lK",
#else
        "lL",
#endif
        &sqlrcur, &rows))
    return NULL;
  ((sqlrcursor *)sqlrcur)->setResultSetBufferSize(rows);
  return Py_BuildValue("h", 0);
}

static PyObject *getResultSetBufferSize(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getResultSetBufferSize();
  // FIXME: lame, python doesn't support building values from uint64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *dontGetColumnInfo(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->dontGetColumnInfo();
  return Py_BuildValue("h", 0);
}

static PyObject *getColumnInfo(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->getColumnInfo();
  return Py_BuildValue("h", 0);
}

static PyObject *mixedCaseColumnNames(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->mixedCaseColumnNames();
  return Py_BuildValue("h", 0);
}

static PyObject *upperCaseColumnNames(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->upperCaseColumnNames();
  return Py_BuildValue("h", 0);
}

static PyObject *lowerCaseColumnNames(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->lowerCaseColumnNames();
  return Py_BuildValue("h", 0);
}

static PyObject *cacheToFile(PyObject *self, PyObject *args) {
  long sqlrcur;
  char *filename;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &filename))
    return NULL;
  ((sqlrcursor *)sqlrcur)->cacheToFile(filename);
  return Py_BuildValue("h", 0);
}

static PyObject *setCacheTtl(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t ttl;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lI",
#else
        "li",
#endif
        &sqlrcur, &ttl))
    return NULL;
  ((sqlrcursor *)sqlrcur)->setCacheTtl(ttl);
  return Py_BuildValue("h", 0);
}

static PyObject *getCacheFileName(PyObject *self, PyObject *args) {
  long sqlrcur;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getCacheFileName();
  return Py_BuildValue("s", rc);
}

static PyObject *cacheOff(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->cacheOff();
  return Py_BuildValue("h", 0);
}

static PyObject *getDatabaseList(PyObject *self, PyObject *args) {
  char *databases;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &databases))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getDatabaseList(databases);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getCatalogList(PyObject *self, PyObject *args) {
  char *catalogs;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lz", &sqlrcur, &catalogs))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getCatalogList(catalogs);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getSchemaList(PyObject *self, PyObject *args) {
  char *schemas;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lz", &sqlrcur, &schemas))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getSchemaList(schemas);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getTableTypeList(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getTableTypeList();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getTableList(PyObject *self, PyObject *args) {
  char *tables;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lz", &sqlrcur, &tables))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getTableList(tables);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getTypeInfoList(PyObject *self, PyObject *args) {
  char *type;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lz", &sqlrcur, &type))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getTypeInfoList(type);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnList(PyObject *self, PyObject *args) {
  char *table;
  char *columns;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lsz", &sqlrcur, &table, &columns))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getColumnList(table, columns);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getPrimaryKeysList(PyObject *self, PyObject *args) {
  char *table;
  char *columns;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lsz", &sqlrcur, &table, &columns))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getPrimaryKeysList(table, columns);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getKeyAndIndexList(PyObject *self, PyObject *args) {
  char *table;
  char *qualifier;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lsz", &sqlrcur, &table, &qualifier))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getKeyAndIndexList(table, qualifier);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getProcedureList(PyObject *self, PyObject *args) {
  char *procedures;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lz", &sqlrcur, &procedures))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getProcedureList(procedures);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getProcedureParameterList(PyObject *self, PyObject *args) {
  char *procedure;
  char *parameters;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lsz", &sqlrcur, &procedure, &parameters))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getProcedureParameterList(procedure, parameters);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *setCursorName(PyObject *self, PyObject *args) {
  long sqlrcur;
  char *cursorname;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &cursorname))
    return NULL;
  ((sqlrcursor *)sqlrcur)->setCursorName(cursorname);
  return Py_BuildValue("h", 0);
}

static PyObject *getCursorName(PyObject *self, PyObject *args) {
  long sqlrcur;
  const char *rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getCursorName();
  return Py_BuildValue("s", rc);
}

static PyObject *sendQuery(PyObject *self, PyObject *args) {
  char *sqlString;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &sqlString))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->sendQuery(sqlString);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *sendQueryWithLength(PyObject *self, PyObject *args) {
  PyObject *sqlObj;
  PyObject *encoded=NULL;
  long sqlrcur;
  uint32_t length;
  const char *sqlString;
  Py_ssize_t sqlLen;
  bool rc;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lOI",
#else
        "lOi",
#endif
        &sqlrcur, &sqlObj, &length))
    return NULL;
#if PY_MAJOR_VERSION >= 3
  if (PyBytes_Check(sqlObj)) {
    if (PyBytes_AsStringAndSize(sqlObj, (char **)&sqlString, &sqlLen) < 0)
      return NULL;
  } else if (PyUnicode_Check(sqlObj)) {
    encoded=PyUnicode_AsEncodedString(sqlObj,"utf-8","strict");
    if (!encoded) return NULL;
    if (PyBytes_AsStringAndSize(encoded, (char **)&sqlString, &sqlLen) < 0) {
      Py_DECREF(encoded);
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_TypeError,
        "sendQueryWithLength: query must be str or bytes");
    return NULL;
  }
#else
  // Python 2: str is bytes-like and may contain embedded nulls
  if (PyString_Check(sqlObj)) {
    if (PyString_AsStringAndSize(sqlObj, (char **)&sqlString, &sqlLen) < 0)
      return NULL;
  } else if (PyUnicode_Check(sqlObj)) {
    encoded=PyUnicode_AsEncodedString(sqlObj,"utf-8","strict");
    if (!encoded) return NULL;
    if (PyString_AsStringAndSize(encoded, (char **)&sqlString, &sqlLen) < 0) {
      Py_DECREF(encoded);
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_TypeError,
        "sendQueryWithLength: query must be str or unicode");
    return NULL;
  }
#endif
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->sendQuery(sqlString,length);
  Py_END_ALLOW_THREADS
  Py_XDECREF(encoded);
  return Py_BuildValue("h", (short)rc);
}

static PyObject *sendFileQuery(PyObject *self, PyObject *args) {
  char *path;
  char *file;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lss", &sqlrcur, &path, &file))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->sendFileQuery(path, file);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *prepareQuery(PyObject *self, PyObject *args) {
  long sqlrcur;
  char *sqlString;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &sqlString))
    return NULL;
  ((sqlrcursor *)sqlrcur)->prepareQuery(sqlString);
  return Py_BuildValue("h", 0);
}

static PyObject *prepareQueryWithLength(PyObject *self, PyObject *args) {
  char *sqlString;
  long sqlrcur;
  uint32_t length;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lsI",
#else
        "lsi",
#endif
        &sqlrcur, &sqlString, &length))
    return NULL;
  ((sqlrcursor *)sqlrcur)->prepareQuery(sqlString,length);
  return Py_BuildValue("h", 0);
}

static PyObject *prepareFileQuery(PyObject *self, PyObject *args) {
  char *path;
  char *file;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "lss", &sqlrcur, &path, &file))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->prepareFileQuery(path, file);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *substitution(PyObject *self, PyObject *args) {
  long sqlrcur;
  char *variable;
  PyObject *value;
  uint32_t precision;
  uint32_t scale;
  uint16_t success;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lsOII",
#else
        "lsOii",
#endif
        &sqlrcur, &variable, &value, &precision, &scale))
    return NULL;
  success=1;
  if (value==Py_None) {
    ((sqlrcursor *)sqlrcur)->substitution(variable, (char *)NULL);
  } else if (PyString_Check(value)) {
    ((sqlrcursor *)sqlrcur)->substitution(variable, PyString_AsString(value));
  } else if (PyInt_Check(value)) {
    ((sqlrcursor *)sqlrcur)->substitution(variable, (int64_t)PyInt_AsLong(value));
  } else if (PyFloat_Check(value)) {
    ((sqlrcursor *)sqlrcur)->substitution(variable, (double)PyFloat_AsDouble(value), precision, scale);
  } else {
    success=0;
  }
  return Py_BuildValue("h", success);
}

static PyObject *substitutions(PyObject *self, PyObject *args) {
  PyObject *variables;
  PyObject *values;
  PyObject *precisions;
  PyObject *scales;
  long sqlrcur;
  const char *variable;
  uint16_t success;
  PyObject *value;
  if (!PyArg_ParseTuple(args, "lOOOO", &sqlrcur, &variables, &values, &precisions, &scales))
    return NULL;
  success=1;
  if (PyList_Check(variables) && PyList_Check(values)) {
    for (int i=0; i<PyList_Size(variables); i++) {
      variable=PyString_AsString(PyList_GetItem(variables,i));
      value=PyList_GetItem(values,i);
      if (value==Py_None) {
        ((sqlrcursor *)sqlrcur)->substitution(variable, (char *)NULL);
      } else if (PyString_Check(value)) {
        ((sqlrcursor *)sqlrcur)->substitution(variable, PyString_AsString(value));
      } else if (PyInt_Check(value)) {
        ((sqlrcursor *)sqlrcur)->substitution(variable, (int64_t)PyInt_AsLong(value));
      } else if (PyFloat_Check(value)) {
        ((sqlrcursor *)sqlrcur)->substitution(variable, (double)PyFloat_AsDouble(value), (uint32_t)PyInt_AsLong(PyList_GetItem(precisions,i)), (uint32_t)PyInt_AsLong(PyList_GetItem(scales,i)));
      } else {
        success=0;
      }
    }
  }
  return Py_BuildValue("h", success);
}

static PyObject *clearBinds(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->clearBinds();
  return Py_BuildValue("h", 0);
}

static PyObject *countBindVariables(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint16_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->countBindVariables();
  // FIXME: lame, python doesn't support building values from uint16_t's
  return Py_BuildValue("h", (short)rc);
}

static PyObject *inputBind(PyObject *self, PyObject *args) {
  char *variable;
  PyObject *value;
  PyObject *precision;
  uint32_t scale;
  long sqlrcur;
  uint16_t success;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lsOOI",
#else
        "lsOOi",
#endif
        &sqlrcur, &variable, &value, &precision, &scale))
    return NULL;
  success=1;
  if (value==Py_None) {
    ((sqlrcursor *)sqlrcur)->inputBind(variable, (char *)NULL);
  } else if (PyString_Check(value)) {
    // if the 3rd parameter is a string then the
    // 4th parameter might be a length parameter
    if (PyInt_Check(precision) && PyInt_AsLong(precision)>0) {
      ((sqlrcursor *)sqlrcur)->inputBind(variable, PyString_AsString(value), (uint32_t)PyInt_AsLong(precision));
    } else {
      ((sqlrcursor *)sqlrcur)->inputBind(variable, PyString_AsString(value));
    }
  } else if (value == Py_True) {
    ((sqlrcursor *)sqlrcur)->inputBind(variable, "1");
  } else if (value == Py_False) {
    ((sqlrcursor *)sqlrcur)->inputBind(variable, "0");
  } else if (PyInt_Check(value)) {
    ((sqlrcursor *)sqlrcur)->inputBind(variable, (int64_t)PyInt_AsLong(value));
  } else if (PyFloat_Check(value)) {
    ((sqlrcursor *)sqlrcur)->inputBind(variable, (double)PyFloat_AsDouble(value), (uint32_t)PyInt_AsLong(precision), (uint32_t)scale);
  } else {
    ((sqlrcursor *)sqlrcur)->inputBind(variable, PyString_AsString(PyObject_Str(value)));
  }
  return Py_BuildValue("h", success);
}

static PyObject *inputBindDate(PyObject *self, PyObject *args) {
  char *variable;
  int16_t year;
  int16_t month;
  int16_t day;
  int16_t hour;
  int16_t minute;
  int16_t second;
  int32_t microsecond;
  char *tz;
  int isnegative;
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "lshhhhhhizi",
        &sqlrcur, &variable, &year, &month, &day,
        &hour, &minute, &second, &microsecond, &tz,
        &isnegative))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrcursor *)sqlrcur)->inputBind(variable,year,month,day,
                                      hour,minute,second,
                                      microsecond,tz,
                                      (bool)isnegative);
  Py_END_ALLOW_THREADS
  Py_RETURN_NONE;
}

static PyObject *inputBindBlob(PyObject *self, PyObject *args) {
  char *variable;
  PyObject *value;
  uint32_t size;
  long sqlrcur;
  uint16_t success;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lsOI",
#else
        "lsOi",
#endif
        &sqlrcur, &variable, &value, &size))
    return NULL;
  success=1;
  if (value==Py_None) {
    ((sqlrcursor *)sqlrcur)->inputBindBlob(variable, NULL, size);
  } else if (PyString_Check(value)) {
    ((sqlrcursor *)sqlrcur)->inputBindBlob(variable, PyString_AsString(value), size);
  } else if (PyBytes_Check(value)) {
    ((sqlrcursor *)sqlrcur)->inputBindBlob(variable, PyBytes_AsString(value), size);
  } else {
    success=0;
  }
  return Py_BuildValue("h", success);
}

static PyObject *inputBindClob(PyObject *self, PyObject *args) {
  char *variable;
  PyObject *value;
  uint32_t size;
  long sqlrcur;
  uint16_t success;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lsOI",
#else
        "lsOi",
#endif
        &sqlrcur, &variable, &value, &size))
    return NULL;
  success=1;
  if (value==Py_None) {
    ((sqlrcursor *)sqlrcur)->inputBindClob(variable, NULL, size);
  } else if (PyString_Check(value)) {
    ((sqlrcursor *)sqlrcur)->inputBindClob(variable, PyString_AsString(value), size);
  } else {
    success=0;
  }
  return Py_BuildValue("h", success);
}

static PyObject *inputBinds(PyObject *self, PyObject *args) {
  PyObject *variables;
  PyObject *values;
  PyObject *precisions;
  PyObject *scales;
  long sqlrcur;
  const char *variable;
  uint16_t success;
  PyObject *value;
  if (!PyArg_ParseTuple(args, "lOOOO", &sqlrcur, &variables, &values, &precisions, &scales))
    return NULL;
  success=1;
  if (PyList_Check(variables) && PyList_Check(values)) {
    for (int i=0; i<PyList_Size(variables); i++) {
      variable=PyString_AsString(PyList_GetItem(variables,i));
      value=PyList_GetItem(values,i);
      if (value==Py_None) {
        ((sqlrcursor *)sqlrcur)->inputBind(variable, (char *)NULL);
      } else if (PyString_Check(value)) {
        ((sqlrcursor *)sqlrcur)->inputBind(variable, PyString_AsString(value));
      } else if (value == Py_True) {
        ((sqlrcursor *)sqlrcur)->inputBind(variable, "1");
      } else if (value == Py_False) {
        ((sqlrcursor *)sqlrcur)->inputBind(variable, "0");
      } else if (PyInt_Check(value)) {
        ((sqlrcursor *)sqlrcur)->inputBind(variable, (int64_t)PyInt_AsLong(value));
      } else if (PyFloat_Check(value)) {
        ((sqlrcursor *)sqlrcur)->inputBind(variable, (double)PyFloat_AsDouble(value), (unsigned short)PyInt_AsLong(PyList_GetItem(precisions,i)), (unsigned short)PyInt_AsLong(PyList_GetItem(scales,i)));
      } else {
        ((sqlrcursor *)sqlrcur)->inputBind(variable, PyString_AsString(PyObject_Str(value)));
      }
    }
  }
  return Py_BuildValue("h", success);
}

static PyObject *defineOutputBindString(PyObject *self, PyObject *args) {
  char *variable;
  uint32_t length;
  long sqlrcur;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lsI",
#else
        "lsi",
#endif
        &sqlrcur, &variable, &length))
    return NULL;
  ((sqlrcursor *)sqlrcur)->defineOutputBindString(variable, length);
  return Py_BuildValue("h", 0);
}

static PyObject *defineOutputBindInteger(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  if (!PyArg_ParseTuple(args,"ls",&sqlrcur, &variable))
    return NULL;
  ((sqlrcursor *)sqlrcur)->defineOutputBindInteger(variable);
  return Py_BuildValue("h", 0);
}

static PyObject *defineOutputBindDouble(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  if (!PyArg_ParseTuple(args,"ls",&sqlrcur, &variable))
    return NULL;
  ((sqlrcursor *)sqlrcur)->defineOutputBindDouble(variable);
  return Py_BuildValue("h", 0);
}

static PyObject *defineOutputBindDate(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  ((sqlrcursor *)sqlrcur)->defineOutputBindDate(variable);
  return Py_BuildValue("h", 0);
}

static PyObject *defineOutputBindBlob(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  ((sqlrcursor *)sqlrcur)->defineOutputBindBlob(variable);
  return Py_BuildValue("h", 0);
}

static PyObject *defineOutputBindClob(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  ((sqlrcursor *)sqlrcur)->defineOutputBindClob(variable);
  return Py_BuildValue("h", 0);
}

static PyObject *defineOutputBindCursor(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  ((sqlrcursor *)sqlrcur)->defineOutputBindCursor(variable);
  return Py_BuildValue("h", 0);
}

static PyObject *validateBinds(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->validateBinds();
  return Py_BuildValue("h", 0);
}

static PyObject *validBind(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  char *variable;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->validBind(variable);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *executeQuery(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->executeQuery();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *fetchFromBindCursor(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->fetchFromBindCursor();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getOutputBindString(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  const char *rc;
#ifdef PY_SSIZE_T_CLEAN
  ssize_t rl;
#else
  uint32_t rl;
#endif
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindString(variable);
  rl=((sqlrcursor *)sqlrcur)->getOutputBindLength(variable);
  return Py_BuildValue("s#", rc, rl);
}

static PyObject *getOutputBindBlob(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  const char *rc;
#ifdef PY_SSIZE_T_CLEAN
  ssize_t rl;
#else
  uint32_t rl;
#endif
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindBlob(variable);
  rl=((sqlrcursor *)sqlrcur)->getOutputBindLength(variable);
  return buildConstPyString(rc, rl, true);
}

static PyObject *getOutputBindClob(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  const char *rc;
#ifdef PY_SSIZE_T_CLEAN
  ssize_t rl;
#else
  uint32_t rl;
#endif
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindClob(variable);
  rl=((sqlrcursor *)sqlrcur)->getOutputBindLength(variable);
  return Py_BuildValue("s#", rc, rl);
}

static PyObject *getOutputBindInteger(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int64_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindInteger(variable);
  // FIXME: lame, python doesn't support building values from int64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getOutputBindDouble(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  double rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDouble(variable);
  return Py_BuildValue("d", rc);
}

static PyObject *getOutputBindLength(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  uint32_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindLength(variable);
  // FIXME: lame, python doesn't support building values from uint32_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getOutputBindDateYear(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int16_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateYear(variable);
  return Py_BuildValue("h", rc);
}

static PyObject *getOutputBindDateMonth(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int16_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateMonth(variable);
  return Py_BuildValue("h", rc);
}

static PyObject *getOutputBindDateDay(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int16_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateDay(variable);
  return Py_BuildValue("h", rc);
}

static PyObject *getOutputBindDateHour(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int16_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateHour(variable);
  return Py_BuildValue("h", rc);
}

static PyObject *getOutputBindDateMinute(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int16_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateMinute(variable);
  return Py_BuildValue("h", rc);
}

static PyObject *getOutputBindDateSecond(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int16_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateSecond(variable);
  return Py_BuildValue("h", rc);
}

static PyObject *getOutputBindDateMicrosecond(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  int32_t rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateMicrosecond(variable);
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getOutputBindDateTz(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  const char *rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateTz(variable);
  return Py_BuildValue("s", rc);
}

static PyObject *getOutputBindDateIsNegative(PyObject *self, PyObject *args) {
  char *variable;
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getOutputBindDateIsNegative(variable);
  return PyBool_FromLong(rc);
}

static PyObject *openCachedResultSet(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  char *filename;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &filename))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->openCachedResultSet(filename);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *colCount(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->colCount();
  // FIXME: lame, python doesn't support building values from uint32_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *rowCount(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->rowCount();
  // FIXME: lame, python doesn't support building values from uint64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *totalRows(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->totalRows();
  // FIXME: lame, python doesn't support building values from uint64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *affectedRows(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->affectedRows();
  // FIXME: lame, python doesn't support building values from uint64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *firstRowIndex(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->firstRowIndex();
  // FIXME: lame, python doesn't support building values from uint64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *endOfResultSet(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->endOfResultSet();
  return Py_BuildValue("h", (short)rc);
}

static PyObject *nextResultSet(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->nextResultSet();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *cursorErrorMessage(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  return Py_BuildValue("s", ((sqlrcursor *)sqlrcur)->errorMessage());
}

static PyObject *cursorErrorNumber(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  // FIXME: lame, python doesn't support building values from int64_t's
  return Py_BuildValue("l", (long)((sqlrcursor *)sqlrcur)->errorNumber());
}

static PyObject *cursorErrorSqlState(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  return Py_BuildValue("s", ((sqlrcursor *)sqlrcur)->errorSqlState());
}

static PyObject *getNullsAsEmptyStrings(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->getNullsAsEmptyStrings();
  return Py_BuildValue("h", 0);
}

static PyObject *getNullsAsNone(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  ((sqlrcursor *)sqlrcur)->getNullsAsNulls();
  return Py_BuildValue("h", 0);
}

static PyObject *getField(PyObject *self, PyObject *args) {
  long sqlrcur;
  const char *rc="";
#ifdef PY_SSIZE_T_CLEAN
  ssize_t rl=0;
#else
  uint32_t rl=0;
#endif
  uint64_t row;
  PyObject *col;
  const char* type;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  if (PyString_Check(col)) {
    const char *colname=PyString_AsString(col);
    Py_BEGIN_ALLOW_THREADS
    rc=((sqlrcursor *)sqlrcur)->getField(row, colname);
    rl=((sqlrcursor *)sqlrcur)->getFieldLength(row, colname);
    type = ((sqlrcursor *)sqlrcur)->getColumnType(colname);
    Py_END_ALLOW_THREADS
  } else if (PyInt_Check(col)) {
    uint32_t colind=PyInt_AsLong(col);
    Py_BEGIN_ALLOW_THREADS
    rc=((sqlrcursor *)sqlrcur)->getField(row, colind);
    rl=((sqlrcursor *)sqlrcur)->getFieldLength(row, colind);
    type = ((sqlrcursor *)sqlrcur)->getColumnType(colind);
    Py_END_ALLOW_THREADS
  }
  if (!rc) {
    Py_INCREF(Py_None);
    return Py_None;
  } else if (usenumeric && isFloatTypeChar(type)) {
    if (decimal) {
      PyObject *tuple=PyTuple_New(1);
      PyTuple_SetItem(tuple, 0, Py_BuildValue("s#", rc, rl));
      return PyObject_CallObject(decimal, tuple);
    } else {
      return Py_BuildValue("f",(double)charstring::convertToFloatC(rc));
    }
  } else if (usenumeric && isNumberTypeChar(type)) {
    return Py_BuildValue("L",charstring::convertToInteger(rc));
  } else if (isBitTypeChar(type)) {
    return Py_BuildValue("l",bitStringToLong(rc));
  } else if (isBoolTypeChar(type)) {
    if (rc && character::lower(rc[0]) == 't') {
      Py_INCREF(Py_True);
      return Py_True;
    } else if (rc && character::lower(rc[0]) == 'f') {
      Py_INCREF(Py_False);
      return Py_False;
    } else {
      Py_INCREF(Py_None);
      return Py_None;
    }
  }
  return buildConstPyStringT(rc, rl, type);
}

static PyObject *getFieldIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  const char *rc=NULL;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldIgnoringCase(row, PyString_AsString(col));
  }
  Py_END_ALLOW_THREADS
  if (!rc) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return Py_BuildValue("s", rc);
}

static PyObject *getFieldAsInteger(PyObject *self, PyObject *args) {
  long sqlrcur;
  int64_t rc=0;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsInteger(row, PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsInteger(row, PyInt_AsLong(col));
  }
  Py_END_ALLOW_THREADS
  // FIXME: lame, python doesn't support building values from int64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getFieldAsIntegerIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int64_t rc=0;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsIntegerIgnoringCase(row, PyString_AsString(col));
  }
  Py_END_ALLOW_THREADS
  // FIXME: lame, python doesn't support building values from int64_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getFieldAsDouble(PyObject *self, PyObject *args) {
  long sqlrcur;
  double rc=0.0;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsDouble(row, PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsDouble(row, PyInt_AsLong(col));
  }
  Py_END_ALLOW_THREADS
  return Py_BuildValue("d", rc);
}

static PyObject *getFieldAsDoubleIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  double rc=0.0;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsDoubleIgnoringCase(row, PyString_AsString(col));
  }
  Py_END_ALLOW_THREADS
  return Py_BuildValue("d", rc);
}

static PyObject *getFieldAsBoolean(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsBoolean(row, PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsBoolean(row, PyInt_AsLong(col));
  }
  Py_END_ALLOW_THREADS
  return PyBool_FromLong(rc);
}

static PyObject *getFieldAsBooleanIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getFieldAsBooleanIgnoringCase(row, PyString_AsString(col));
  }
  Py_END_ALLOW_THREADS
  return PyBool_FromLong(rc);
}

static PyObject *getFieldAsDateYear(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateYear(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateYear(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateYear(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateYear(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateYearIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateYearIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateYearIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateMonth(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMonth(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMonth(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMonth(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMonth(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateMonthIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMonthIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMonthIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateDay(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateDay(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateDay(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateDay(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateDay(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateDayIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateDayIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateDayIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateHour(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateHour(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateHour(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateHour(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateHour(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateHourIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateHourIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateHourIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateMinute(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMinute(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMinute(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMinute(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMinute(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateMinuteIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMinuteIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMinuteIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateSecond(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateSecond(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateSecond(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateSecond(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateSecond(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateSecondIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int16_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateSecondIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateSecondIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("h", rc);
}

static PyObject *getFieldAsDateMicrosecond(PyObject *self, PyObject *args) {
  long sqlrcur;
  int32_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMicrosecond(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMicrosecond(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMicrosecond(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMicrosecond(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getFieldAsDateMicrosecondIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  int32_t rc=0;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMicrosecondIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateMicrosecondIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getFieldAsDateIsNegative(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateIsNegative(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateIsNegative(row,
                        PyInt_AsLong(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateIsNegative(row,
                        PyString_AsString(col));
    } else if (PyInt_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateIsNegative(row,
                        PyInt_AsLong(col));
    }
    Py_END_ALLOW_THREADS
  }
  return PyBool_FromLong(rc);
}

static PyObject *getFieldAsDateIsNegativeIgnoringCase(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  uint64_t row;
  PyObject *col;
  int ddmm=0;
  int yyyyddmm=0;
  const char *datedelimiters=NULL;
  if (PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKOiis",
#else
        "lLOiis",
#endif
        &sqlrcur, &row, &col, &ddmm, &yyyyddmm, &datedelimiters)) {
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateIsNegativeIgnoringCase(row,
                        PyString_AsString(col),
                        ddmm!=0,yyyyddmm!=0,datedelimiters);
    }
    Py_END_ALLOW_THREADS
  } else {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
          "lKO",
#else
          "lLO",
#endif
          &sqlrcur, &row, &col))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    if (PyString_Check(col)) {
      rc=((sqlrcursor *)sqlrcur)->getFieldAsDateIsNegativeIgnoringCase(row,
                        PyString_AsString(col));
    }
    Py_END_ALLOW_THREADS
  }
  return PyBool_FromLong(rc);
}

static PyObject *getFieldLength(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t rc=0;
  uint64_t row;
  PyObject *col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKO",
#else
        "lLO",
#endif
        &sqlrcur, &row, &col))
    return NULL;
  if (PyString_Check(col)) {
    const char *colname=PyString_AsString(col);
    Py_BEGIN_ALLOW_THREADS
    rc=((sqlrcursor *)sqlrcur)->getFieldLength(row, colname);
    Py_END_ALLOW_THREADS
  } else if (PyInt_Check(col)) {
    uint32_t colind=PyInt_AsLong(col);
    Py_BEGIN_ALLOW_THREADS
    rc=((sqlrcursor *)sqlrcur)->getFieldLength(row, colind);
    Py_END_ALLOW_THREADS
  }
  // FIXME: lame, python doesn't support building values from uint32_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *
_get_row(sqlrcursor *sqlrcur, uint64_t row)
{
  uint32_t num_cols;
  uint32_t counter;
  const char * const *row_data;
#ifdef PY_SSIZE_T_CLEAN
  ssize_t rl;
#else
  uint32_t rl;
#endif
  uint32_t *row_lengths;
  const char *type;
  PyObject *my_list;
  num_cols=sqlrcur->colCount();
  my_list =  PyList_New(num_cols);
  Py_BEGIN_ALLOW_THREADS
  row_data=sqlrcur->getRow(row);
  row_lengths=sqlrcur->getRowLengths(row);
  Py_END_ALLOW_THREADS
  if (!row_data) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  for (counter = 0; counter < num_cols; ++counter) {
    rl=row_lengths[counter];
    type=sqlrcur->getColumnType(counter);
    if (!row_data[counter]) {
        Py_INCREF(Py_None);
        PyList_SetItem(my_list, counter, Py_None);
    } else if (usenumeric && isFloatTypeChar(type)) {
      PyObject *obj;
      if (decimal) {
        PyObject *tuple=PyTuple_New(1);
        PyTuple_SetItem(tuple, 0, Py_BuildValue("s#", row_data[counter], rl));
        obj=PyObject_CallObject(decimal, tuple);
      } else {
        obj=Py_BuildValue("f", (double)charstring::convertToFloatC(row_data[counter]));
      }
      PyList_SetItem(my_list, counter, obj);
    } else if (usenumeric && isNumberTypeChar(type)) {
      PyList_SetItem(my_list, counter, Py_BuildValue("L", charstring::convertToInteger(row_data[counter])));
    } else if (isBitTypeChar(type)) {
      PyList_SetItem(my_list, counter, Py_BuildValue("l", bitStringToLong(row_data[counter])));
    } else if (isBoolTypeChar(type)) {
      if (row_data[counter] && character::lower(row_data[counter][0]) == 't') {
        Py_INCREF(Py_True);
        PyList_SetItem(my_list, counter, Py_True);
      } else if (row_data[counter] && character::lower(row_data[counter][0]) == 'f') {
        Py_INCREF(Py_False);
        PyList_SetItem(my_list, counter, Py_False);
      } else {
        Py_INCREF(Py_None);
        PyList_SetItem(my_list, counter, Py_None);
      }
    } else {
      PyList_SetItem(my_list, counter, buildConstPyStringT(row_data[counter], row_lengths[counter], type));
    }
  }
  return my_list;
}

static PyObject *getRow(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t row;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lK",
#else
        "lL",
#endif
        &sqlrcur, &row))
    return NULL;
  return _get_row((sqlrcursor *)sqlrcur, row);
}

static PyObject *getRowDictionary(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t row;
  PyObject *my_dictionary;
  uint32_t counter;
  const char *field;
  const char *name;
  const char *type;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lK",
#else
        "lL",
#endif
        &sqlrcur, &row))
    return NULL;
  my_dictionary=PyDict_New();
  for (counter=0; counter<((sqlrcursor *)sqlrcur)->colCount(); counter++) {
    Py_BEGIN_ALLOW_THREADS
    field=((sqlrcursor *)sqlrcur)->getField(row, counter);
    Py_END_ALLOW_THREADS
    name=((sqlrcursor *)sqlrcur)->getColumnName(counter);
    type=((sqlrcursor *)sqlrcur)->getColumnType(counter);
    if (!field) {
        Py_INCREF(Py_None);
        PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_None);
    } else if (usenumeric && isFloatTypeChar(type)) {
        if (decimal) {
          PyObject *tuple=PyTuple_New(1);
          PyTuple_SetItem(tuple, 0, Py_BuildValue("s", field));
          PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), PyObject_CallObject(decimal, tuple));
        } else {
          PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_BuildValue("f",(double)charstring::convertToFloatC(field)));
        }
    } else if (usenumeric && isNumberTypeChar(type)) {
      PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_BuildValue("L", charstring::convertToInteger(field)));
    } else if (isBitTypeChar(type)) {
      PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_BuildValue("l", bitStringToLong(field)));
    } else if (isBoolTypeChar(type)) {
      if (field && character::lower(field[0]) == 't') {
        Py_INCREF(Py_True);
        PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_True);
      } else if (field && character::lower(field[0]) == 'f') {
        Py_INCREF(Py_False);
        PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_False);
      } else {
        Py_INCREF(Py_None);
        PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_None);
      }
    } else {
      if (field) {
        PyDict_SetItem(my_dictionary, Py_BuildValue("s", name),
                buildConstPyStringT(field,
                        #ifdef PY_SSIZE_T_CLEAN
                        (ssize_t)
                        #endif
                        (((sqlrcursor *)sqlrcur)->getFieldLength(row, counter)),
                        type
                )
        );
      } else {
        Py_INCREF(Py_None);
        PyDict_SetItem(my_dictionary, Py_BuildValue("s", name), Py_None);
      }
    }
  }
  return my_dictionary;
}

static PyObject *getRowRange(PyObject *self, PyObject *args) {
  uint64_t beg_row;
  uint64_t end_row;
  long sqlrcur;
  uint64_t counter;
  PyObject *my_list;
  my_list = PyList_New(0);
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKK",
#else
        "lLL",
#endif
        &sqlrcur, &beg_row, &end_row))
    return NULL;
  uint64_t max_rows=((sqlrcursor *)sqlrcur)->rowCount();
  if (end_row>=max_rows) {
          end_row=max_rows-1;
  }
  for (counter = beg_row; counter <= end_row; ++counter) {
    PyList_Append(my_list, _get_row((sqlrcursor *)sqlrcur, counter));
  }
  return my_list;
}

static PyObject *
_get_row_lengths(sqlrcursor *sqlrcur, uint64_t row)
{
  uint32_t num_cols;
  uint32_t counter;
  uint32_t *row_data;
  PyObject *my_list;
  num_cols=sqlrcur->colCount();
  my_list =  PyList_New(num_cols);
  Py_BEGIN_ALLOW_THREADS
  row_data=sqlrcur->getRowLengths(row);
  Py_END_ALLOW_THREADS
  if (!row_data) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  for (counter = 0; counter < num_cols; ++counter) {
    // FIXME: lame, python doesn't support building values from uint32_t's
    PyList_SetItem(my_list, counter, Py_BuildValue("l", (long)row_data[counter]));
  }
  return my_list;
}

static PyObject *getRowLengths(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t row;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lK",
#else
        "lL",
#endif
        &sqlrcur, &row))
    return NULL;
  return _get_row_lengths((sqlrcursor *)sqlrcur, row);
}

static PyObject *getRowLengthsDictionary(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint64_t row;
  PyObject *my_dictionary;
  const char *name;
  long fieldlength;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lK",
#else
        "lL",
#endif
        &sqlrcur, &row))
    return NULL;
  my_dictionary=PyDict_New();
  for (uint32_t counter=0; counter<((sqlrcursor *)sqlrcur)->colCount(); counter++) {
    // Don't be tempted to embed this call inside of PyDict_SetItem.  Since
    // it might have to fetch data from the server, it needs to be embedded
    // in Py_BEGIN/END_ALLOW_THREADS.  On some platforms, embedding the entire
    // PyDict_SetItem() call works, but on some, it doesn't.  I'm not sure
    // why, exactly, but that's the case.  It's safe to call getColumnName()
    // outside of Py_BEGIN/END_ALLOW_THREADS because it will never have to
    // talk to the server.  It could be embedded in PyDict_SetItem, but I'll
    // do it this way for consistency with getRowDictionary().
    Py_BEGIN_ALLOW_THREADS
    fieldlength=(long)((sqlrcursor *)sqlrcur)->getFieldLength(row,counter);
    Py_END_ALLOW_THREADS
    name=((sqlrcursor *)sqlrcur)->getColumnName(counter);
    PyDict_SetItem(my_dictionary,Py_BuildValue("s",name),Py_BuildValue("l",fieldlength));
  }
  return my_dictionary;
}

static PyObject *getRowLengthsRange(PyObject *self, PyObject *args) {
  uint64_t beg_row;
  uint64_t end_row;
  long sqlrcur;
  uint64_t counter;
  PyObject *my_list;
  my_list = PyList_New(0);
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lKK",
#else
        "lLL",
#endif
        &sqlrcur, &beg_row, &end_row))
    return NULL;
  uint64_t max_rows=((sqlrcursor *)sqlrcur)->rowCount();
  if (end_row>=max_rows) {
          end_row=max_rows-1;
  }
  for (counter = beg_row; counter <= end_row; ++counter) {
    PyList_Append(my_list, _get_row_lengths((sqlrcursor *)sqlrcur, counter));
  }
  return my_list;
}

static PyObject *getColumnName(PyObject *self, PyObject *args) {
  long sqlrcur;
  const char *rc;
  uint32_t col;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lI",
#else
        "li",
#endif
        &sqlrcur, &col))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getColumnName(col);
  return Py_BuildValue("s", rc);
}

static PyObject *getColumnNames(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t counter;
  uint32_t num_cols;
  const char * const *rc;
  PyObject *my_list;
  my_list = PyList_New(0);
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  num_cols=((sqlrcursor *)sqlrcur)->colCount();
  rc=((sqlrcursor *)sqlrcur)->getColumnNames();
  if (rc) {
    for (counter = 0; counter < num_cols; ++counter) {
      PyList_Append(my_list, Py_BuildValue("s", rc[counter]));
    }
    return my_list;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *getColumnType(PyObject *self, PyObject *args) {
  long sqlrcur;
  const char *rc="";
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnType(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnType(PyInt_AsLong(col));
  }
  return Py_BuildValue("s", rc);
}

static PyObject *getColumnLength(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t rc=0;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnLength(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnLength(PyInt_AsLong(col));
  }
  // FIXME: lame, python doesn't support building values from uint32_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getColumnPrecision(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t rc=0;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnPrecision(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnPrecision(PyInt_AsLong(col));
  }
  // FIXME: lame, python doesn't support building values from uint32_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getColumnScale(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t rc=0;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnScale(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnScale(PyInt_AsLong(col));
  }
  // FIXME: lame, python doesn't support building values from uint32_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getColumnIsNullable(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsNullable(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsNullable(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnIsPrimaryKey(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsPrimaryKey(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsPrimaryKey(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnIsUnique(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsUnique(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsUnique(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnIsPartOfKey(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsPartOfKey(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsPartOfKey(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnIsUnsigned(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsUnsigned(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsUnsigned(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnIsZeroFilled(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsZeroFilled(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsZeroFilled(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnIsBinary(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsBinary(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsBinary(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getColumnIsAutoIncrement(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsAutoIncrement(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getColumnIsAutoIncrement(PyInt_AsLong(col));
  }
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getLongest(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint32_t rc=false;
  PyObject *col;
  if (!PyArg_ParseTuple(args, "lO", &sqlrcur, &col))
    return NULL;
  if (PyString_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getLongest(PyString_AsString(col));
  } else if (PyInt_Check(col)) {
    rc=((sqlrcursor *)sqlrcur)->getLongest(PyInt_AsLong(col));
  }
  // FIXME: lame, python doesn't support building values from uint32_t's
  return Py_BuildValue("l", (long)rc);
}

static PyObject *getResultSetId(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint16_t rc;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  rc=((sqlrcursor *)sqlrcur)->getResultSetId();
  // FIXME: lame, python doesn't support building values from uint16_t's
  return Py_BuildValue("h", (short)rc);
}

static PyObject *suspendResultSet(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrcursor *)sqlrcur)->suspendResultSet();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *resumeResultSet(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  uint16_t id;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lH",
#else
        "lh",
#endif
        &sqlrcur, &id))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->resumeResultSet(id);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *resumeCachedResultSet(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  uint16_t id;
  char *filename;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lHs",
#else
        "lhs",
#endif
        &sqlrcur, &id, &filename))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->resumeCachedResultSet(id,filename);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *closeResultSet(PyObject *self, PyObject *args) {
  long sqlrcur;
  if (!PyArg_ParseTuple(args, "l", &sqlrcur))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrcursor *)sqlrcur)->closeResultSet();
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyObject *outputBindCursorIdIsValid(PyObject *self, PyObject *args) {
  long sqlrcur;
  bool rc;
  char *variable;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->outputBindCursorIdIsValid(variable);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", (short)rc);
}

static PyObject *getOutputBindCursorId(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint16_t rc;
  char *variable;
  if (!PyArg_ParseTuple(args, "ls", &sqlrcur, &variable))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  rc=((sqlrcursor *)sqlrcur)->getOutputBindCursorId(variable);
  Py_END_ALLOW_THREADS
  // FIXME: lame, python doesn't support building values from uint16_t's
  return Py_BuildValue("h", (short)rc);
}

static PyObject *attachToBindCursor(PyObject *self, PyObject *args) {
  long sqlrcur;
  uint16_t bindcursorid;
  if (!PyArg_ParseTuple(args,
#ifdef SUPPORTS_UNSIGNED
        "lH",
#else
        "lh",
#endif
        &sqlrcur, &bindcursorid))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  ((sqlrcursor *)sqlrcur)->attachToBindCursor(bindcursorid);
  Py_END_ALLOW_THREADS
  return Py_BuildValue("h", 0);
}

static PyMethodDef SQLRMethods[] = {
  {"getNumericFieldsAsStrings", getNumericFieldsAsStrings, METH_VARARGS},
  {"getNumericFieldsAsNumbers", getNumericFieldsAsNumbers, METH_VARARGS},
  {"sqlrcon_alloc",  sqlrcon_alloc, METH_VARARGS},
  {"sqlrcon_free", sqlrcon_free, METH_VARARGS},
  {"setConnectTimeout", setConnectTimeout, METH_VARARGS},
  {"getConnectTimeoutSeconds", getConnectTimeoutSeconds, METH_VARARGS},
  {"getConnectTimeoutMicroseconds", getConnectTimeoutMicroseconds, METH_VARARGS},
  {"setResponseTimeout", setResponseTimeout, METH_VARARGS},
  {"getResponseTimeoutSeconds", getResponseTimeoutSeconds, METH_VARARGS},
  {"getResponseTimeoutMicroseconds", getResponseTimeoutMicroseconds, METH_VARARGS},
  {"setBindVariableDelimiters", setBindVariableDelimiters, METH_VARARGS},
  {"getBindVariableDelimiterQuestionMarkSupported", getBindVariableDelimiterQuestionMarkSupported, METH_VARARGS},
  {"getBindVariableDelimiterColonSupported", getBindVariableDelimiterColonSupported, METH_VARARGS},
  {"getBindVariableDelimiterAtSignSupported", getBindVariableDelimiterAtSignSupported, METH_VARARGS},
  {"getBindVariableDelimiterDollarSignSupported", getBindVariableDelimiterDollarSignSupported, METH_VARARGS},
  {"enableKerberos", enableKerberos, METH_VARARGS},
  {"enableTls", enableTls, METH_VARARGS},
  {"disableEncryption", disableEncryption, METH_VARARGS},
  {"endSession", endSession, METH_VARARGS},
  {"suspendSession", suspendSession, METH_VARARGS},
  {"getConnectionPort", getConnectionPort, METH_VARARGS},
  {"getConnectionSocket", getConnectionSocket, METH_VARARGS},
  {"resumeSession", resumeSession, METH_VARARGS},
  {"ping", ping, METH_VARARGS},
  {"identify", identify, METH_VARARGS},
  {"dbVersion", dbVersion, METH_VARARGS},
  {"dbHostName", dbHostName, METH_VARARGS},
  {"dbIpAddress", dbIpAddress, METH_VARARGS},
  {"serverVersion", serverVersion, METH_VARARGS},
  {"clientVersion", clientVersion, METH_VARARGS},
  {"bindFormat", bindFormat, METH_VARARGS},
  {"nextvalFormat", nextvalFormat, METH_VARARGS},
  {"selectDatabase", selectDatabase, METH_VARARGS},
  {"getCurrentDatabase", getCurrentDatabase, METH_VARARGS},
  {"getDatabaseIsSchema", getDatabaseIsSchema, METH_VARARGS},
  {"selectCatalog", selectCatalog, METH_VARARGS},
  {"getCurrentCatalog", getCurrentCatalog, METH_VARARGS},
  {"selectSchema", selectSchema, METH_VARARGS},
  {"getCurrentSchema", getCurrentSchema, METH_VARARGS},
  {"getCurrentUser", getCurrentUser, METH_VARARGS},
  {"getLastInsertId", getLastInsertId, METH_VARARGS},
  {"autoCommitOn", autoCommitOn, METH_VARARGS},
  {"autoCommitOff", autoCommitOff, METH_VARARGS},
  {"getAutoCommit", getAutoCommit, METH_VARARGS},
  {"begin", begin, METH_VARARGS},
  {"commit", commit, METH_VARARGS},
  {"rollback", rollback, METH_VARARGS},
  {"getInTransaction", getInTransaction, METH_VARARGS},
  {"getDefaultTransactionModel", getDefaultTransactionModel, METH_VARARGS},
  {"setTransactionModel", setTransactionModel, METH_VARARGS},
  {"getTransactionModel", getTransactionModel, METH_VARARGS},
  {"getDefaultIsolationLevel", getDefaultIsolationLevel, METH_VARARGS},
  {"getDefaultIsolationLevel", getDefaultIsolationLevel, METH_VARARGS},
  {"setIsolationLevel", setIsolationLevel, METH_VARARGS},
  {"getIsolationLevel", getIsolationLevel, METH_VARARGS},
  {"getDatabaseFeature", getDatabaseFeature, METH_VARARGS},
  {"connectionErrorMessage", connectionErrorMessage, METH_VARARGS},
  {"connectionErrorNumber", connectionErrorNumber, METH_VARARGS},
  {"connectionErrorSqlState", connectionErrorSqlState, METH_VARARGS},
  {"debugOn", debugOn, METH_VARARGS},
  {"debugOff", debugOff, METH_VARARGS},
  {"getDebug", getDebug, METH_VARARGS},
  {"setDebugFile", setDebugFile, METH_VARARGS},
  {"setClientInfo", setClientInfo, METH_VARARGS},
  {"getClientInfo", getClientInfo, METH_VARARGS},
  {"isYes", isYes, METH_VARARGS},
  {"isNo", isNo, METH_VARARGS},
  {"sqlrcur_alloc",  sqlrcur_alloc, METH_VARARGS},
  {"sqlrcur_free",  sqlrcur_free, METH_VARARGS},
  {"setResultSetBufferSize", setResultSetBufferSize, METH_VARARGS},
  {"getResultSetBufferSize", getResultSetBufferSize, METH_VARARGS},
  {"dontGetColumnInfo", dontGetColumnInfo, METH_VARARGS},
  {"getColumnInfo", getColumnInfo, METH_VARARGS},
  {"mixedCaseColumnNames", mixedCaseColumnNames, METH_VARARGS},
  {"upperCaseColumnNames", upperCaseColumnNames, METH_VARARGS},
  {"lowerCaseColumnNames", lowerCaseColumnNames, METH_VARARGS},
  {"cacheToFile", cacheToFile, METH_VARARGS},
  {"setCacheTtl", setCacheTtl, METH_VARARGS},
  {"getCacheFileName", getCacheFileName, METH_VARARGS},
  {"cacheOff", cacheOff, METH_VARARGS},
  {"getDatabaseList", getDatabaseList, METH_VARARGS},
  {"getCatalogList", getCatalogList, METH_VARARGS},
  {"getSchemaList", getSchemaList, METH_VARARGS},
  {"getTableTypeList", getTableTypeList, METH_VARARGS},
  {"getTableList", getTableList, METH_VARARGS},
  {"getTypeInfoList", getTypeInfoList, METH_VARARGS},
  {"getColumnList", getColumnList, METH_VARARGS},
  {"getPrimaryKeysList", getPrimaryKeysList, METH_VARARGS},
  {"getKeyAndIndexList", getKeyAndIndexList, METH_VARARGS},
  {"getProcedureList", getProcedureList, METH_VARARGS},
  {"getProcedureParameterList", getProcedureParameterList, METH_VARARGS},
  {"setCursorName", setCursorName, METH_VARARGS},
  {"getCursorName", getCursorName, METH_VARARGS},
  {"sendQuery", sendQuery, METH_VARARGS},
  {"sendQueryWithLength", sendQueryWithLength, METH_VARARGS},
  {"sendFileQuery", sendFileQuery, METH_VARARGS},
  {"prepareQuery", prepareQuery, METH_VARARGS},
  {"prepareQueryWithLength", prepareQueryWithLength, METH_VARARGS},
  {"prepareFileQuery", prepareFileQuery, METH_VARARGS},
  {"substitution", substitution, METH_VARARGS},
  {"substitutions", substitutions, METH_VARARGS},
  {"clearBinds", clearBinds, METH_VARARGS},
  {"countBindVariables", countBindVariables, METH_VARARGS},
  {"inputBind", inputBind, METH_VARARGS},
  {"inputBindDate", inputBindDate, METH_VARARGS},
  {"inputBindBlob", inputBindBlob, METH_VARARGS},
  {"inputBindClob", inputBindClob, METH_VARARGS},
  {"inputBinds", inputBinds, METH_VARARGS},
  {"defineOutputBindString", defineOutputBindString, METH_VARARGS},
  {"defineOutputBindInteger", defineOutputBindInteger, METH_VARARGS},
  {"defineOutputBindDouble", defineOutputBindDouble, METH_VARARGS},
  {"defineOutputBindDate", defineOutputBindDate, METH_VARARGS},
  {"defineOutputBindBlob", defineOutputBindBlob, METH_VARARGS},
  {"defineOutputBindClob", defineOutputBindClob, METH_VARARGS},
  {"defineOutputBindCursor", defineOutputBindCursor, METH_VARARGS},
  {"validateBinds", validateBinds, METH_VARARGS},
  {"validBind", validBind, METH_VARARGS},
  {"executeQuery", executeQuery, METH_VARARGS},
  {"fetchFromBindCursor", fetchFromBindCursor, METH_VARARGS},
  {"getOutputBindString", getOutputBindString, METH_VARARGS},
  {"getOutputBindBlob", getOutputBindBlob, METH_VARARGS},
  {"getOutputBindClob", getOutputBindClob, METH_VARARGS},
  {"getOutputBindInteger", getOutputBindInteger, METH_VARARGS},
  {"getOutputBindDouble", getOutputBindDouble, METH_VARARGS},
  {"getOutputBindLength", getOutputBindLength, METH_VARARGS},
  {"getOutputBindDateYear", getOutputBindDateYear, METH_VARARGS},
  {"getOutputBindDateMonth", getOutputBindDateMonth, METH_VARARGS},
  {"getOutputBindDateDay", getOutputBindDateDay, METH_VARARGS},
  {"getOutputBindDateHour", getOutputBindDateHour, METH_VARARGS},
  {"getOutputBindDateMinute", getOutputBindDateMinute, METH_VARARGS},
  {"getOutputBindDateSecond", getOutputBindDateSecond, METH_VARARGS},
  {"getOutputBindDateMicrosecond", getOutputBindDateMicrosecond, METH_VARARGS},
  {"getOutputBindDateTz", getOutputBindDateTz, METH_VARARGS},
  {"getOutputBindDateIsNegative", getOutputBindDateIsNegative, METH_VARARGS},
  {"openCachedResultSet", openCachedResultSet, METH_VARARGS},
  {"colCount", colCount, METH_VARARGS},
  {"rowCount", rowCount, METH_VARARGS},
  {"totalRows", totalRows, METH_VARARGS},
  {"affectedRows", affectedRows, METH_VARARGS},
  {"firstRowIndex", firstRowIndex, METH_VARARGS},
  {"endOfResultSet", endOfResultSet, METH_VARARGS},
  {"nextResultSet", nextResultSet, METH_VARARGS},
  {"cursorErrorMessage", cursorErrorMessage, METH_VARARGS},
  {"cursorErrorNumber", cursorErrorNumber, METH_VARARGS},
  {"cursorErrorSqlState", cursorErrorSqlState, METH_VARARGS},
  {"getNullsAsEmptyStrings", getNullsAsEmptyStrings, METH_VARARGS},
  {"getNullsAsNone", getNullsAsNone, METH_VARARGS},
  {"getField", getField, METH_VARARGS},
  {"getFieldIgnoringCase", getFieldIgnoringCase, METH_VARARGS},
  {"getFieldAsInteger", getFieldAsInteger, METH_VARARGS},
  {"getFieldAsIntegerIgnoringCase", getFieldAsIntegerIgnoringCase, METH_VARARGS},
  {"getFieldAsDouble", getFieldAsDouble, METH_VARARGS},
  {"getFieldAsDoubleIgnoringCase", getFieldAsDoubleIgnoringCase, METH_VARARGS},
  {"getFieldAsBoolean", getFieldAsBoolean, METH_VARARGS},
  {"getFieldAsBooleanIgnoringCase", getFieldAsBooleanIgnoringCase, METH_VARARGS},
  {"getFieldAsDateYear", getFieldAsDateYear, METH_VARARGS},
  {"getFieldAsDateYearIgnoringCase", getFieldAsDateYearIgnoringCase, METH_VARARGS},
  {"getFieldAsDateMonth", getFieldAsDateMonth, METH_VARARGS},
  {"getFieldAsDateMonthIgnoringCase", getFieldAsDateMonthIgnoringCase, METH_VARARGS},
  {"getFieldAsDateDay", getFieldAsDateDay, METH_VARARGS},
  {"getFieldAsDateDayIgnoringCase", getFieldAsDateDayIgnoringCase, METH_VARARGS},
  {"getFieldAsDateHour", getFieldAsDateHour, METH_VARARGS},
  {"getFieldAsDateHourIgnoringCase", getFieldAsDateHourIgnoringCase, METH_VARARGS},
  {"getFieldAsDateMinute", getFieldAsDateMinute, METH_VARARGS},
  {"getFieldAsDateMinuteIgnoringCase", getFieldAsDateMinuteIgnoringCase, METH_VARARGS},
  {"getFieldAsDateSecond", getFieldAsDateSecond, METH_VARARGS},
  {"getFieldAsDateSecondIgnoringCase", getFieldAsDateSecondIgnoringCase, METH_VARARGS},
  {"getFieldAsDateMicrosecond", getFieldAsDateMicrosecond, METH_VARARGS},
  {"getFieldAsDateMicrosecondIgnoringCase", getFieldAsDateMicrosecondIgnoringCase, METH_VARARGS},
  {"getFieldAsDateIsNegative", getFieldAsDateIsNegative, METH_VARARGS},
  {"getFieldAsDateIsNegativeIgnoringCase", getFieldAsDateIsNegativeIgnoringCase, METH_VARARGS},
  {"getFieldLength", getFieldLength, METH_VARARGS},
  {"getRow", getRow, METH_VARARGS},
  {"getRowDictionary", getRowDictionary, METH_VARARGS},
  {"getRowRange", getRowRange, METH_VARARGS},
  {"getRowLengths", getRowLengths, METH_VARARGS},
  {"getRowLengthsDictionary", getRowLengthsDictionary, METH_VARARGS},
  {"getRowLengthsRange", getRowLengthsRange, METH_VARARGS},
  {"getColumnName", getColumnName, METH_VARARGS},
  {"getColumnNames", getColumnNames, METH_VARARGS},
  {"getColumnType", getColumnType, METH_VARARGS},
  {"getColumnLength", getColumnLength, METH_VARARGS},
  {"getColumnPrecision", getColumnPrecision, METH_VARARGS},
  {"getColumnScale", getColumnScale, METH_VARARGS},
  {"getColumnIsNullable", getColumnIsNullable, METH_VARARGS},
  {"getColumnIsPrimaryKey", getColumnIsPrimaryKey, METH_VARARGS},
  {"getColumnIsUnique", getColumnIsUnique, METH_VARARGS},
  {"getColumnIsPartOfKey", getColumnIsPartOfKey, METH_VARARGS},
  {"getColumnIsUnsigned", getColumnIsUnsigned, METH_VARARGS},
  {"getColumnIsZeroFilled", getColumnIsZeroFilled, METH_VARARGS},
  {"getColumnIsBinary", getColumnIsBinary, METH_VARARGS},
  {"getColumnIsAutoIncrement", getColumnIsAutoIncrement, METH_VARARGS},
  {"getLongest", getLongest, METH_VARARGS},
  {"getResultSetId", getResultSetId, METH_VARARGS},
  {"suspendResultSet", suspendResultSet, METH_VARARGS},
  {"resumeResultSet", resumeResultSet, METH_VARARGS},
  {"resumeCachedResultSet", resumeCachedResultSet, METH_VARARGS},
  {"closeResultSet", closeResultSet, METH_VARARGS},
  {"outputBindCursorIdIsValid", outputBindCursorIdIsValid, METH_VARARGS},
  {"getOutputBindCursorId", getOutputBindCursorId, METH_VARARGS},
  {"attachToBindCursor", attachToBindCursor, METH_VARARGS},
  {NULL,      NULL}        /* Sentinel */
};

#if PY_MAJOR_VERSION >= 3
static PyModuleDef sqlrmoduledef = {
  PyModuleDef_HEAD_INIT,
  "SQLRelay.CSQLRelay",
  NULL,
  -1,
  SQLRMethods,
  NULL,
  NULL,
  NULL,
  NULL
};
#endif

#ifdef _WIN32
__declspec(dllexport)
#endif
#if PY_MAJOR_VERSION >= 3
PyObject *PyInit_CSQLRelay()
#else
void initCSQLRelay()
#endif
{
#if PY_MAJOR_VERSION >= 3
  PyObject *sqlrmodule=PyModule_Create(&sqlrmoduledef);
#else
  Py_InitModule("SQLRelay.CSQLRelay", SQLRMethods);
#endif

  usenumeric=false;
  decimalmodule=PyImport_ImportModule("decimal");
  if (decimalmodule) {
    decimal=PyObject_GetAttrString(decimalmodule,"Decimal");
    if (!decimal) {
      PyErr_Clear();
    }
  } else {
    PyErr_Clear();
  }

#if PY_MAJOR_VERSION >= 3
  return sqlrmodule;
#endif
}

}

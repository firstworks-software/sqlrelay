/*
 * sqlrelayCmd.c
 * Copyright (c) 2003 Takeshi Taguchi
 * $Id: sqlrelayCmd.cpp,v 1.15 2016/03/13 19:39:54 mused Exp $
 */

#include <tcl.h>
#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>

#include <config.h>

#ifndef HAVE_TCL_GETSTRING
	#define Tcl_GetString(a) Tcl_GetStringFromObj(a,NULL)
#endif

#ifdef HAVE_TCL_CONSTCHAR
	#define CONSTCHAR const char
#else
	#define CONSTCHAR char
#endif

#ifndef HAVE_TCL_WIDEINT
	#define Tcl_WideInt long
	#define Tcl_GetWideIntFromObj(a,b,c) Tcl_GetLongFromObj(a,b,c)
#endif

#ifdef HAVE_TCL_NEWSTRINGOBJ_CONST_CHAR
	#define _Tcl_NewStringObj(a,b) Tcl_NewStringObj(a,b)
#else
	#define _Tcl_NewStringObj(a,b) Tcl_NewStringObj((char *)(a),b)
#endif

// tcl 9.0 removed the CONST compatibility macro
#ifndef CONST
	#define CONST const
#endif

// tcl 9.0 changed object/list/byte-array lengths from int to Tcl_Size
#ifndef TCL_SIZE_MAX
	typedef int Tcl_Size;
#endif

extern "C" {

/*
 * getCursorID --
 *    This procedure return tcl obj contains sqlrcur command name.
 * Results:
 *    Tcl object.
 * Side effects:
 *    count up static variable count.
 */
Tcl_Obj *getCursorID(void) {
  Tcl_Obj *id;
  static int count = 0;

  id = _Tcl_NewStringObj("sqlrcur", -1);
  Tcl_AppendStringsToObj(id, Tcl_GetString(Tcl_NewIntObj(count++)),
			(char *)NULL);
  return (id);
}

/*
 * sqlrcurDelete --
 *    This procedure is for deleting sqlrcur command.
 * Results:
 *    none
 * Side effects:
 *    call cur->free()
 */
void sqlrcurDelete(ClientData data) {
  sqlrcursor *cur = (sqlrcursor *)data;

  if (cur != (sqlrcursor *)NULL) {
    delete cur;
    cur = (sqlrcursor *)NULL;
  }
}

/*
 * sqlrcurObjCmd --
 *   This procedure is invoked to process the "sqlrcur" object command.
 * Synopsis:
 *   $cur eval query
 *   $cur setResultSetBufferSize ?rows?
 *   $cur getResultSetBufferSize
 *   $cur dontGetColumnInfo
 *   $cur getColumnInfo
 *   $cur caseColumnNames mixed|upper|low
 *   $cur cacheToFile filename
 *   $cur setCacheTtl ttl
 *   $cur getCacheFileName
 *   $cur cacheOff
 *   $cur getDatabaseList databases
 *   $cur getCatalogList catalogs
 *   $cur getSchemaList schemas
 *   $cur getTableTypeList
 *   $cur getTableList tables
 *   $cur getTypeInfoList type
 *   $cur getColumnList table columns
 *   $cur getPrimaryKeysList table columns
 *   $cur getKeyAndIndexList table qualifier
 *   $cur getProcedureList procedures
 *   $cur getProcedureParameterList procedure parameters
 *   $cur setCursorName cursorname
 *   $cur getCursorName
 *   $cur sendQuery query
 *   $cur sendQueryWithLength query length
 *   $cur sendFileQuery path filename
 *   $cur prepareQuery query
 *   $cur prepareQueryWithLength query length
 *   $cur prepareFileQuery path filename
 *   $cur substitution variable value
 *   $cur clearBinds
 *   $cur countBindVariables
 *   $cur inputBind
 *   $cur inputBindDate variable year month day hour minute second microsecond tz isnegative
 *   $cur inputBindBlob variable value size
 *   $cur inputBindClob variable value size
 *   $cur defineOutputBindString variable value
 *   $cur defineOutputBindInteger variable value
 *   $cur defineOutputBindDouble variable value
 *   $cur defineOutputBindDate variable
 *   $cur defineOutputBindBlob variable
 *   $cur defineOutputBindClob variable
 *   $cur defineOutputBindCursor variable
 *   $cur substitutions {{variable value} ...}
 *   $cur inputBinds {{variable value ?precision scale?} ...}
 *   $cur validateBinds
 *   $cur validBind
 *   $cur executeQuery
 *   $cur fetchFromBindCursor
 *   $cur getOutputBindString variable
 *   $cur getOutputBindBlob variable
 *   $cur getOutputBindClob variable
 *   $cur getOutputBindInteger variable
 *   $cur getOutputBindDouble variable
 *   $cur getOutputBindLength variable
 *   $cur getOutputBindCursor variable
 *   $cur getOutputBindDateYear variable
 *   $cur getOutputBindDateMonth variable
 *   $cur getOutputBindDateDay variable
 *   $cur getOutputBindDateHour variable
 *   $cur getOutputBindDateMinute variable
 *   $cur getOutputBindDateSecond variable
 *   $cur getOutputBindDateMicrosecond variable
 *   $cur getOutputBindDateTz variable
 *   $cur getOutputBindDateIsNegative variable
 *   $cur openCachedResultSet variable
 *   $cur colCount
 *   $cur rowCount
 *   $cur totalRows
 *   $cur affectedRows
 *   $cur firstRowIndex
 *   $cur endOfResultSet
 *   $cur nextResultSet
 *   $cur errorMessage
 *   $cur errorNumber
 *   $cur errorSqlState
 *   $cur getFieldByIndex row col
 *   $cur getFieldByName row col
 *   $cur getFieldByNameIgnoringCase row col
 *   $cur getFieldAsIntegerByIndex row col
 *   $cur getFieldAsIntegerByName row col
 *   $cur getFieldAsIntegerByNameIgnoringCase row col
 *   $cur getFieldAsDoubleByIndex row col
 *   $cur getFieldAsDoubleByName row col
 *   $cur getFieldAsDoubleByNameIgnoringCase row col
 *   $cur getFieldAsBooleanByIndex row col
 *   $cur getFieldAsBooleanByName row col
 *   $cur getFieldAsBooleanByNameIgnoringCase row col
 *   $cur getFieldAsDateYearByIndex row col
 *   $cur getFieldAsDateYearByName row col
 *   $cur getFieldAsDateYearByNameIgnoringCase row col
 *   $cur getFieldAsDateYearByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateYearByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateYearByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMonthByIndex row col
 *   $cur getFieldAsDateMonthByName row col
 *   $cur getFieldAsDateMonthByNameIgnoringCase row col
 *   $cur getFieldAsDateMonthByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMonthByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMonthByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateDayByIndex row col
 *   $cur getFieldAsDateDayByName row col
 *   $cur getFieldAsDateDayByNameIgnoringCase row col
 *   $cur getFieldAsDateDayByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateDayByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateDayByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateHourByIndex row col
 *   $cur getFieldAsDateHourByName row col
 *   $cur getFieldAsDateHourByNameIgnoringCase row col
 *   $cur getFieldAsDateHourByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateHourByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateHourByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMinuteByIndex row col
 *   $cur getFieldAsDateMinuteByName row col
 *   $cur getFieldAsDateMinuteByNameIgnoringCase row col
 *   $cur getFieldAsDateMinuteByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMinuteByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMinuteByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateSecondByIndex row col
 *   $cur getFieldAsDateSecondByName row col
 *   $cur getFieldAsDateSecondByNameIgnoringCase row col
 *   $cur getFieldAsDateSecondByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateSecondByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateSecondByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMicrosecondByIndex row col
 *   $cur getFieldAsDateMicrosecondByName row col
 *   $cur getFieldAsDateMicrosecondByNameIgnoringCase row col
 *   $cur getFieldAsDateMicrosecondByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMicrosecondByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateIsNegativeByIndex row col
 *   $cur getFieldAsDateIsNegativeByName row col
 *   $cur getFieldAsDateIsNegativeByNameIgnoringCase row col
 *   $cur getFieldAsDateIsNegativeByIndexWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateIsNegativeByNameWithDdMm row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase row col ddmm yyyyddmm datedelimiters
 *   $cur getFieldLengthByIndex row col
 *   $cur getFieldLengthByName row col
 *   $cur getRow row
 *   $cur getRowLengths row
 *   $cur getColumnNames
 *   $cur getColumnName col
 *   $cur getColumnTypeByIndex col
 *   $cur getColumnTypeByName col
 *   $cur getColumnLengthByIndex col
 *   $cur getColumnLengthByName col
 *   $cur getColumnPrecisionByIndex col
 *   $cur getColumnPrecisionByName col
 *   $cur getColumnScaleByIndex col
 *   $cur getColumnScaleByName col
 *   $cur getColumnIsNullableByIndex col
 *   $cur getColumnIsNullableByName col
 *   $cur getColumnIsPrimaryKeyByIndex col
 *   $cur getColumnIsPrimaryKeyByName col
 *   $cur getColumnIsUniqueByIndex col
 *   $cur getColumnIsUniqueByName col
 *   $cur getColumnIsPartOfKeyByIndex col
 *   $cur getColumnIsPartOfKeyByName col
 *   $cur getColumnIsUnsignedByIndex col
 *   $cur getColumnIsUnsignedByName col
 *   $cur getColumnIsZeroFilledByIndex col
 *   $cur getColumnIsZeroFilledByName col
 *   $cur getColumnIsBinaryByIndex col
 *   $cur getColumnIsBinaryByName col
 *   $cur getColumnIsAutoIncrementByIndex col
 *   $cur getColumnIsAutoIncrementByName col
 *   $cur getLongestByIndex col
 *   $cur getLongestByName col
 *   $cur getResultSetId
 *   $cur suspendResultSet
 *   $cur resumeResultSet is
 *   $cur resumeCachedResultSet id filename
 *   $cur closeResultSet
 *  Note:
 *   cur->getNullsAsEmptyStrings, and cur->getNullsAsNulls are not
 *   supported.
 */
int sqlrcurObjCmd(ClientData data, Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[]) {
  sqlrcursor *cur = (sqlrcursor *)data;
  int index;
  static CONSTCHAR *options[] = {
    "eval",
    "setResultSetBufferSize",
    "getResultSetBufferSize",
    "dontGetColumnInfo",
    "getColumnInfo",
    "caseColumnNames",
    "cacheToFile",
    "setCacheTtl",
    "getCacheFileName",
    "cacheOff",
    "getDatabaseList",
    "getCatalogList",
    "getSchemaList",
    "getTableTypeList",
    "getTableList",
    "getTypeInfoList",
    "getColumnList",
    "getPrimaryKeysList",
    "getKeyAndIndexList",
    "getProcedureList",
    "getProcedureParameterList",
    "setCursorName",
    "getCursorName",
    "sendQuery",
    "sendQueryWithLength",
    "sendFileQuery",
    "prepareQuery",
    "prepareQueryWithLength",
    "prepareFileQuery",
    "substitution",
    "clearBinds",
    "countBindVariables",
    "inputBind",
    "inputBindNull",
    "inputBindDate",
    "inputBindBlob",
    "inputBindClob",
    "defineOutputBindString",
    "defineOutputBindInteger",
    "defineOutputBindDouble",
    "defineOutputBindDate",
    "defineOutputBindBlob",
    "defineOutputBindClob",
    "defineOutputBindCursor",
    "substitutions",
    "inputBinds",
    "validateBinds",
    "validBind",
    "executeQuery",
    "fetchFromBindCursor",
    "getOutputBindString",
    "getOutputBindBlob",
    "getOutputBindClob",
    "getOutputBindInteger",
    "getOutputBindDouble",
    "getOutputBindLength",
    "getOutputBindCursor",
    "getOutputBindDateYear",
    "getOutputBindDateMonth",
    "getOutputBindDateDay",
    "getOutputBindDateHour",
    "getOutputBindDateMinute",
    "getOutputBindDateSecond",
    "getOutputBindDateMicrosecond",
    "getOutputBindDateTz",
    "getOutputBindDateIsNegative",
    "openCachedResultSet",
    "colCount",
    "rowCount",
    "totalRows",
    "affectedRows",
    "firstRowIndex",
    "endOfResultSet",
    "nextResultSet",
    "errorMessage",
    "errorNumber",
    "errorSqlState",
    "getFieldByIndex",
    "getFieldByName",
    "getFieldByNameIgnoringCase",
    "getFieldAsIntegerByIndex",
    "getFieldAsIntegerByName",
    "getFieldAsIntegerByNameIgnoringCase",
    "getFieldAsDoubleByIndex",
    "getFieldAsDoubleByName",
    "getFieldAsDoubleByNameIgnoringCase",
    "getFieldAsBooleanByIndex",
    "getFieldAsBooleanByName",
    "getFieldAsBooleanByNameIgnoringCase",
    "getFieldAsDateYearByIndex",
    "getFieldAsDateYearByName",
    "getFieldAsDateYearByNameIgnoringCase",
    "getFieldAsDateYearByIndexWithDdMm",
    "getFieldAsDateYearByNameWithDdMm",
    "getFieldAsDateYearByNameWithDdMmIgnoringCase",
    "getFieldAsDateMonthByIndex",
    "getFieldAsDateMonthByName",
    "getFieldAsDateMonthByNameIgnoringCase",
    "getFieldAsDateMonthByIndexWithDdMm",
    "getFieldAsDateMonthByNameWithDdMm",
    "getFieldAsDateMonthByNameWithDdMmIgnoringCase",
    "getFieldAsDateDayByIndex",
    "getFieldAsDateDayByName",
    "getFieldAsDateDayByNameIgnoringCase",
    "getFieldAsDateDayByIndexWithDdMm",
    "getFieldAsDateDayByNameWithDdMm",
    "getFieldAsDateDayByNameWithDdMmIgnoringCase",
    "getFieldAsDateHourByIndex",
    "getFieldAsDateHourByName",
    "getFieldAsDateHourByNameIgnoringCase",
    "getFieldAsDateHourByIndexWithDdMm",
    "getFieldAsDateHourByNameWithDdMm",
    "getFieldAsDateHourByNameWithDdMmIgnoringCase",
    "getFieldAsDateMinuteByIndex",
    "getFieldAsDateMinuteByName",
    "getFieldAsDateMinuteByNameIgnoringCase",
    "getFieldAsDateMinuteByIndexWithDdMm",
    "getFieldAsDateMinuteByNameWithDdMm",
    "getFieldAsDateMinuteByNameWithDdMmIgnoringCase",
    "getFieldAsDateSecondByIndex",
    "getFieldAsDateSecondByName",
    "getFieldAsDateSecondByNameIgnoringCase",
    "getFieldAsDateSecondByIndexWithDdMm",
    "getFieldAsDateSecondByNameWithDdMm",
    "getFieldAsDateSecondByNameWithDdMmIgnoringCase",
    "getFieldAsDateMicrosecondByIndex",
    "getFieldAsDateMicrosecondByName",
    "getFieldAsDateMicrosecondByNameIgnoringCase",
    "getFieldAsDateMicrosecondByIndexWithDdMm",
    "getFieldAsDateMicrosecondByNameWithDdMm",
    "getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase",
    "getFieldAsDateIsNegativeByIndex",
    "getFieldAsDateIsNegativeByName",
    "getFieldAsDateIsNegativeByNameIgnoringCase",
    "getFieldAsDateIsNegativeByIndexWithDdMm",
    "getFieldAsDateIsNegativeByNameWithDdMm",
    "getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase",
    "getFieldLengthByIndex",
    "getFieldLengthByName",
    "getRow",
    "getRowLengths",
    "getColumnNames",
    "getColumnName",
    "getColumnTypeByIndex",
    "getColumnTypeByName",
    "getColumnLengthByIndex",
    "getColumnLengthByName",
    "getColumnPrecisionByIndex",
    "getColumnPrecisionByName",
    "getColumnScaleByIndex",
    "getColumnScaleByName",
    "getColumnIsNullableByIndex",
    "getColumnIsNullableByName",
    "getColumnIsPrimaryKeyByIndex",
    "getColumnIsPrimaryKeyByName",
    "getColumnIsUniqueByIndex",
    "getColumnIsUniqueByName",
    "getColumnIsPartOfKeyByIndex",
    "getColumnIsPartOfKeyByName",
    "getColumnIsUnsignedByIndex",
    "getColumnIsUnsignedByName",
    "getColumnIsZeroFilledByIndex",
    "getColumnIsZeroFilledByName",
    "getColumnIsBinaryByIndex",
    "getColumnIsBinaryByName",
    "getColumnIsAutoIncrementByIndex",
    "getColumnIsAutoIncrementByName",
    "getLongestByIndex",
    "getLongestByName",
    "getResultSetId",
    "suspendResultSet",
    "resumeResultSet",
    "resumeCachedResultSet",
    "closeResultSet",
  };
  
  enum options {
    SQLRCUR_eval,
    SQLRCUR_setResultSetBufferSize,
    SQLRCUR_getResultSetBufferSize,
    SQLRCUR_dontGetColumnInfo,
    SQLRCUR_getColumnInfo,
    SQLRCUR_caseColumnNames,
    SQLRCUR_cacheToFile,
    SQLRCUR_setCacheTtl,
    SQLRCUR_getCacheFileName,
    SQLRCUR_cacheOff,
    SQLRCUR_getDatabaseList,
    SQLRCUR_getCatalogList,
    SQLRCUR_getSchemaList,
    SQLRCUR_getTableTypeList,
    SQLRCUR_getTableList,
    SQLRCUR_getTypeInfoList,
    SQLRCUR_getColumnList,
    SQLRCUR_getPrimaryKeysList,
    SQLRCUR_getKeyAndIndexList,
    SQLRCUR_getProcedureList,
    SQLRCUR_getProcedureParameterList,
    SQLRCUR_setCursorName,
    SQLRCUR_getCursorName,
    SQLRCUR_sendQuery,
    SQLRCUR_sendQueryWithLength,
    SQLRCUR_sendFileQuery,
    SQLRCUR_prepareQuery,
    SQLRCUR_prepareQueryWithLength,
    SQLRCUR_prepareFileQuery,
    SQLRCUR_substitution,
    SQLRCUR_clearBinds,
    SQLRCUR_countBindVariables,
    SQLRCUR_inputBind,
    SQLRCUR_inputBindNull,
    SQLRCUR_inputBindDate,
    SQLRCUR_inputBindBlob,
    SQLRCUR_inputBindClob,
    SQLRCUR_defineOutputBindString,
    SQLRCUR_defineOutputBindInteger,
    SQLRCUR_defineOutputBindDouble,
    SQLRCUR_defineOutputBindDate,
    SQLRCUR_defineOutputBindBlob,
    SQLRCUR_defineOutputBindClob,
    SQLRCUR_defineOutputBindCursor,
    SQLRCUR_substitutions,
    SQLRCUR_inputBinds,
    SQLRCUR_validateBinds,
    SQLRCUR_validBind,
    SQLRCUR_executeQuery,
    SQLRCUR_fetchFromBindCursor,
    SQLRCUR_getOutputBindString,
    SQLRCUR_getOutputBindBlob,
    SQLRCUR_getOutputBindClob,
    SQLRCUR_getOutputBindInteger,
    SQLRCUR_getOutputBindDouble,
    SQLRCUR_getOutputBindLength,
    SQLRCUR_getOutputBindCursor,
    SQLRCUR_getOutputBindDateYear,
    SQLRCUR_getOutputBindDateMonth,
    SQLRCUR_getOutputBindDateDay,
    SQLRCUR_getOutputBindDateHour,
    SQLRCUR_getOutputBindDateMinute,
    SQLRCUR_getOutputBindDateSecond,
    SQLRCUR_getOutputBindDateMicrosecond,
    SQLRCUR_getOutputBindDateTz,
    SQLRCUR_getOutputBindDateIsNegative,
    SQLRCUR_openCachedResultSet,
    SQLRCUR_colCount,
    SQLRCUR_rowCount,
    SQLRCUR_totalRows,
    SQLRCUR_affectedRows,
    SQLRCUR_firstRowIndex,
    SQLRCUR_endOfResultSet,
    SQLRCUR_nextResultSet,
    SQLRCUR_errorMessage,
    SQLRCUR_errorNumber,
    SQLRCUR_errorSqlState,
    SQLRCUR_getFieldByIndex,
    SQLRCUR_getFieldByName,
    SQLRCUR_getFieldByNameIgnoringCase,
    SQLRCUR_getFieldAsIntegerByIndex,
    SQLRCUR_getFieldAsIntegerByName,
    SQLRCUR_getFieldAsIntegerByNameIgnoringCase,
    SQLRCUR_getFieldAsDoubleByIndex,
    SQLRCUR_getFieldAsDoubleByName,
    SQLRCUR_getFieldAsDoubleByNameIgnoringCase,
    SQLRCUR_getFieldAsBooleanByIndex,
    SQLRCUR_getFieldAsBooleanByName,
    SQLRCUR_getFieldAsBooleanByNameIgnoringCase,
    SQLRCUR_getFieldAsDateYearByIndex,
    SQLRCUR_getFieldAsDateYearByName,
    SQLRCUR_getFieldAsDateYearByNameIgnoringCase,
    SQLRCUR_getFieldAsDateYearByIndexWithDdMm,
    SQLRCUR_getFieldAsDateYearByNameWithDdMm,
    SQLRCUR_getFieldAsDateYearByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldAsDateMonthByIndex,
    SQLRCUR_getFieldAsDateMonthByName,
    SQLRCUR_getFieldAsDateMonthByNameIgnoringCase,
    SQLRCUR_getFieldAsDateMonthByIndexWithDdMm,
    SQLRCUR_getFieldAsDateMonthByNameWithDdMm,
    SQLRCUR_getFieldAsDateMonthByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldAsDateDayByIndex,
    SQLRCUR_getFieldAsDateDayByName,
    SQLRCUR_getFieldAsDateDayByNameIgnoringCase,
    SQLRCUR_getFieldAsDateDayByIndexWithDdMm,
    SQLRCUR_getFieldAsDateDayByNameWithDdMm,
    SQLRCUR_getFieldAsDateDayByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldAsDateHourByIndex,
    SQLRCUR_getFieldAsDateHourByName,
    SQLRCUR_getFieldAsDateHourByNameIgnoringCase,
    SQLRCUR_getFieldAsDateHourByIndexWithDdMm,
    SQLRCUR_getFieldAsDateHourByNameWithDdMm,
    SQLRCUR_getFieldAsDateHourByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldAsDateMinuteByIndex,
    SQLRCUR_getFieldAsDateMinuteByName,
    SQLRCUR_getFieldAsDateMinuteByNameIgnoringCase,
    SQLRCUR_getFieldAsDateMinuteByIndexWithDdMm,
    SQLRCUR_getFieldAsDateMinuteByNameWithDdMm,
    SQLRCUR_getFieldAsDateMinuteByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldAsDateSecondByIndex,
    SQLRCUR_getFieldAsDateSecondByName,
    SQLRCUR_getFieldAsDateSecondByNameIgnoringCase,
    SQLRCUR_getFieldAsDateSecondByIndexWithDdMm,
    SQLRCUR_getFieldAsDateSecondByNameWithDdMm,
    SQLRCUR_getFieldAsDateSecondByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldAsDateMicrosecondByIndex,
    SQLRCUR_getFieldAsDateMicrosecondByName,
    SQLRCUR_getFieldAsDateMicrosecondByNameIgnoringCase,
    SQLRCUR_getFieldAsDateMicrosecondByIndexWithDdMm,
    SQLRCUR_getFieldAsDateMicrosecondByNameWithDdMm,
    SQLRCUR_getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldAsDateIsNegativeByIndex,
    SQLRCUR_getFieldAsDateIsNegativeByName,
    SQLRCUR_getFieldAsDateIsNegativeByNameIgnoringCase,
    SQLRCUR_getFieldAsDateIsNegativeByIndexWithDdMm,
    SQLRCUR_getFieldAsDateIsNegativeByNameWithDdMm,
    SQLRCUR_getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase,
    SQLRCUR_getFieldLengthByIndex,
    SQLRCUR_getFieldLengthByName,
    SQLRCUR_getRow,
    SQLRCUR_getRowLengths,
    SQLRCUR_getColumnNames,
    SQLRCUR_getColumnName,
    SQLRCUR_getColumnTypeByIndex,
    SQLRCUR_getColumnTypeByName,
    SQLRCUR_getColumnLengthByIndex,
    SQLRCUR_getColumnLengthByName,
    SQLRCUR_getColumnPrecisionByIndex,
    SQLRCUR_getColumnPrecisionByName,
    SQLRCUR_getColumnScaleByIndex,
    SQLRCUR_getColumnScaleByName,
    SQLRCUR_getColumnIsNullableByIndex,
    SQLRCUR_getColumnIsNullableByName,
    SQLRCUR_getColumnIsPrimaryKeyByIndex,
    SQLRCUR_getColumnIsPrimaryKeyByName,
    SQLRCUR_getColumnIsUniqueByIndex,
    SQLRCUR_getColumnIsUniqueByName,
    SQLRCUR_getColumnIsPartOfKeyByIndex,
    SQLRCUR_getColumnIsPartOfKeyByName,
    SQLRCUR_getColumnIsUnsignedByIndex,
    SQLRCUR_getColumnIsUnsignedByName,
    SQLRCUR_getColumnIsZeroFilledByIndex,
    SQLRCUR_getColumnIsZeroFilledByName,
    SQLRCUR_getColumnIsBinaryByIndex,
    SQLRCUR_getColumnIsBinaryByName,
    SQLRCUR_getColumnIsAutoIncrementByIndex,
    SQLRCUR_getColumnIsAutoIncrementByName,
    SQLRCUR_getLongestByIndex,
    SQLRCUR_getLongestByName,
    SQLRCUR_getResultSetId,
    SQLRCUR_suspendResultSet,
    SQLRCUR_resumeResultSet,
    SQLRCUR_resumeCachedResultSet,
    SQLRCUR_closeResultSet,
  };

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(interp, objv[1], (CONSTCHAR **)options, "option", 0,
			  (int *)&index) != TCL_OK) {
    return TCL_ERROR;
  }

  switch ((enum options)index)
    {
    case SQLRCUR_eval:
      {
	uint64_t row;
	uint32_t col;
	Tcl_Obj *rowObj, *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "query");
	  return TCL_ERROR;
	}
	if (!cur->sendQuery(Tcl_GetString(objv[2]))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewObj();
	for (row = 0; row < cur->rowCount(); row++) {
	  rowObj = Tcl_NewObj();
	  for (col = 0; col < cur->colCount(); col++) {
	    const char *field = cur->getField(row, col);
	    uint32_t length = cur->getFieldLength(row, col);
	    if (field == (char *)NULL) { field = ""; }
	    if (Tcl_ListObjAppendElement(interp, rowObj,
					 _Tcl_NewStringObj(field, length))
		!= TCL_OK) {
	      return TCL_ERROR;
	    }
	  }
	  if (Tcl_ListObjAppendElement(interp, result, rowObj) != TCL_OK) {
	    return TCL_ERROR;
	  }
	}
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_setResultSetBufferSize:
      {
	int rows = 0;
	if (objc > 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "?rows?");
	  return TCL_ERROR;
	} else if (objc == 3) {
	  if (Tcl_GetIntFromObj(interp, objv[2], &rows) != TCL_OK) {
	    return TCL_ERROR;
	  }
	  cur->setResultSetBufferSize(rows);
	} else {
	  Tcl_SetObjResult(interp,
			   Tcl_NewIntObj(cur->getResultSetBufferSize()));
	}
	break;
      }
    case SQLRCUR_getResultSetBufferSize:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp,
			 Tcl_NewIntObj(cur->getResultSetBufferSize()));
	break;
      }
    case SQLRCUR_dontGetColumnInfo:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	cur->dontGetColumnInfo();
	break;
      }
    case SQLRCUR_getColumnInfo:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	cur->getColumnInfo();
	break;
      }
    case SQLRCUR_caseColumnNames:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "mixed|upper|lower");
	  return TCL_ERROR;
	} else {
	  char *subopts = Tcl_GetString(objv[2]);
	  if (charstring::compareIgnoringCase(subopts, "mixed", 5) == 0) {
	    cur->mixedCaseColumnNames();
	  } else if (charstring::compareIgnoringCase(subopts, "upper", 5) == 0) {
	    cur->upperCaseColumnNames();
	  } else if (charstring::compareIgnoringCase(subopts, "lower", 5) == 0) {
	    cur->lowerCaseColumnNames();
	  } else {
	    Tcl_AppendResult(interp, "bad option \"", subopts, "\": must be mixed, upper, or lower", (char *)NULL);
	    return TCL_ERROR;
	  }
	}
	break;
      }
    case SQLRCUR_cacheToFile:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "filename");
	  return TCL_ERROR;
	}
	cur->cacheToFile(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_setCacheTtl:
      {
	int ttl = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "ttl");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &ttl) != TCL_OK) {
	  return TCL_ERROR;
	}
	cur->setCacheTtl(ttl);
	break;
      }
    case SQLRCUR_getCacheFileName:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	Tcl_AppendResult(interp, cur->getCacheFileName(), (char *)NULL);
	break;
      }
    case SQLRCUR_cacheOff:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	cur->cacheOff();
	break;
      }
    case SQLRCUR_getDatabaseList:
      {
	int result = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "databases");
	  return TCL_ERROR;
	}
	if (!(result = cur->getDatabaseList(Tcl_GetString(objv[2])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getCatalogList:
      {
	int result = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "catalogs");
	  return TCL_ERROR;
	}
	if (!(result = cur->getCatalogList(Tcl_GetString(objv[2])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getSchemaList:
      {
	int result = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "schemas");
	  return TCL_ERROR;
	}
	if (!(result = cur->getSchemaList(Tcl_GetString(objv[2])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getTableTypeList:
      {
	int result = 0;
	if (objc != 2) {
	  Tcl_WrongNumArgs(interp,2, objv, NULL);
	  return TCL_ERROR;
	}
	if (!(result = cur->getTableTypeList())) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getTableList:
      {
	int result = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "tables");
	  return TCL_ERROR;
	}
	if (!(result = cur->getTableList(Tcl_GetString(objv[2])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getTypeInfoList:
      {
	int result = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "type");
	  return TCL_ERROR;
	}
	if (!(result = cur->getTypeInfoList(Tcl_GetString(objv[2])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getColumnList:
      {
	int result = 0;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,2, objv, "table columns");
	  return TCL_ERROR;
	}
	if (!(result = cur->getColumnList(Tcl_GetString(objv[2]),Tcl_GetString(objv[3])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getPrimaryKeysList:
      {
	int result = 0;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,2, objv, "table columns");
	  return TCL_ERROR;
	}
	if (!(result = cur->getPrimaryKeysList(Tcl_GetString(objv[2]),Tcl_GetString(objv[3])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getKeyAndIndexList:
      {
	int result = 0;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,2, objv, "table qualifier");
	  return TCL_ERROR;
	}
	if (!(result = cur->getKeyAndIndexList(Tcl_GetString(objv[2]),Tcl_GetString(objv[3])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getProcedureList:
      {
	int result = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "procedures");
	  return TCL_ERROR;
	}
	if (!(result = cur->getProcedureList(Tcl_GetString(objv[2])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getProcedureParameterList:
      {
	int result = 0;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,2, objv, "procedure parameters");
	  return TCL_ERROR;
	}
	if (!(result = cur->getProcedureParameterList(Tcl_GetString(objv[2]),Tcl_GetString(objv[3])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_setCursorName:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "cursorname");
	  return TCL_ERROR;
	}
	cur->setCursorName(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_getCursorName:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	Tcl_AppendResult(interp, cur->getCursorName(), (char *)NULL);
	break;
      }
    case SQLRCUR_sendQuery:
      {
	int result = 0;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "query");
	  return TCL_ERROR;
	}
	if (!(result = cur->sendQuery(Tcl_GetString(objv[2])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_sendQueryWithLength:
      {
	int result = 0;
	int length = 0;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,3, objv, "query length");
	  return TCL_ERROR;
	}
	Tcl_GetIntFromObj(interp, objv[3], &length);
	Tcl_Size bytelen = 0;
	const char *bytes = (const char *)Tcl_GetByteArrayFromObj(objv[2], &bytelen);
	if (!(result = cur->sendQuery(bytes,length))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_sendFileQuery:
      {
	int result = 0;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,2, objv, "path filename");
	  return TCL_ERROR;
	}
	if (!(result = cur->sendFileQuery(Tcl_GetString(objv[2]),
				   Tcl_GetString(objv[3])))) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_prepareQuery:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "query");
	  return TCL_ERROR;
	}
	cur->prepareQuery(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_prepareQueryWithLength:
      {
	int length = 0;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,3, objv, "query length");
	  return TCL_ERROR;
	}
	Tcl_GetIntFromObj(interp, objv[3], &length);
	Tcl_Size bytelen = 0;
	const char *bytes = (const char *)Tcl_GetByteArrayFromObj(objv[2], &bytelen);
	cur->prepareQuery(bytes,length);
	break;
      }
    case SQLRCUR_prepareFileQuery:
      {
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,2, objv, "path filename");
	  return TCL_ERROR;
	}
	cur->prepareFileQuery(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]));
	break;
      }
    case SQLRCUR_substitution:
      {
	if (objc == 6) {
	  double value;
	  int precision, scale;
	  if (Tcl_GetDoubleFromObj(interp, objv[3], &value) != TCL_OK ||
	      Tcl_GetIntFromObj(interp, objv[4], &precision) != TCL_OK ||
	      Tcl_GetIntFromObj(interp, objv[5], &scale) != TCL_OK) {
	    return TCL_ERROR;
	  }
	  cur->substitution(Tcl_GetString(objv[2]), value, precision, scale);
	} else if (objc == 4) {
	  long value;
	  if (Tcl_GetLongFromObj(interp, objv[3], &value) == TCL_OK ||
	      Tcl_GetIntFromObj(interp, objv[3], (int *)&value) == TCL_OK) {
	    cur->substitution(Tcl_GetString(objv[2]), value);
	  } else {
	    cur->substitution(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]));
	  }
	} else {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable value ?precision scale?");
	  return TCL_ERROR;
	}
	break;
      }
    case SQLRCUR_clearBinds:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	cur->clearBinds();
	break;
      }
    case SQLRCUR_countBindVariables:
      {
	long count;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	count=cur->countBindVariables();
	Tcl_SetObjResult(interp, Tcl_NewLongObj(count));
	break;
      }
    case SQLRCUR_inputBind:
      {
	if (objc == 6) {
	  double value;
	  int precision, scale;
	  if (Tcl_GetDoubleFromObj(interp, objv[3], &value) != TCL_OK ||
	      Tcl_GetIntFromObj(interp, objv[4], &precision) != TCL_OK ||
	      Tcl_GetIntFromObj(interp, objv[5], &scale) != TCL_OK) {
	    return TCL_ERROR;
	  }
	  cur->inputBind(Tcl_GetString(objv[2]),
			 value,
			 (uint32_t)precision,
			 (uint32_t)scale);
	} else if (objc == 5) {
          /* string with length */
	  long length=0;
	  if (Tcl_GetLongFromObj(interp, objv[4], &length) != TCL_OK) {
	      Tcl_GetIntFromObj(interp, objv[4], (int *)&length);
          }
          /* length must be > 0 */
          if (length>0) {
	  	cur->inputBind(Tcl_GetString(objv[2]),Tcl_GetString(objv[3]),
							(uint32_t)length);
          } else {
	  	cur->inputBind(Tcl_GetString(objv[2]),Tcl_GetString(objv[3]));
          }
	} else if (objc == 4) {
	  long value;
	  if (Tcl_GetLongFromObj(interp, objv[3], &value) == TCL_OK ||
	      Tcl_GetIntFromObj(interp, objv[3], (int *)&value) == TCL_OK) {
	    /* value is long object */
	    cur->inputBind(Tcl_GetString(objv[2]), value);
	  } else {
	    /* value is not long object, so it might be string one */
	    cur->inputBind(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]));
	  } 
	} else {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable value ?precision scale?");
	  return TCL_ERROR;
	}
	break;
      }
    case SQLRCUR_inputBindNull:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	cur->inputBind(Tcl_GetString(objv[2]), (const char *)NULL);
	break;
      }
    case SQLRCUR_inputBindDate:
      {
	long year, month, day, hour, minute, second, microsecond, isnegative;
	if (objc != 12) {
	  Tcl_WrongNumArgs(interp,2, objv, "variable year month day hour minute second microsecond tz isnegative");
	  return TCL_ERROR;
	}
	if (Tcl_GetLongFromObj(interp, objv[3], &year) != TCL_OK ||
	    Tcl_GetLongFromObj(interp, objv[4], &month) != TCL_OK ||
	    Tcl_GetLongFromObj(interp, objv[5], &day) != TCL_OK ||
	    Tcl_GetLongFromObj(interp, objv[6], &hour) != TCL_OK ||
	    Tcl_GetLongFromObj(interp, objv[7], &minute) != TCL_OK ||
	    Tcl_GetLongFromObj(interp, objv[8], &second) != TCL_OK ||
	    Tcl_GetLongFromObj(interp, objv[9], &microsecond) != TCL_OK ||
	    Tcl_GetLongFromObj(interp, objv[11], &isnegative) != TCL_OK) {
	  return TCL_ERROR;
	}
	cur->inputBind(Tcl_GetString(objv[2]),
			(int16_t)year, (int16_t)month, (int16_t)day,
			(int16_t)hour, (int16_t)minute, (int16_t)second,
			(int32_t)microsecond, Tcl_GetString(objv[10]),
			(bool)isnegative);
	break;
      }
    case SQLRCUR_inputBindBlob:
      {
	long size;
	if (objc != 5) {
	  Tcl_WrongNumArgs(interp,2, objv, "variable value size");
	  return TCL_ERROR;
	}
	if (Tcl_GetLongFromObj(interp, objv[4], &size) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_Size bytelen = 0;
	const char *bytes = (const char *)Tcl_GetByteArrayFromObj(objv[3], &bytelen);
	cur->inputBindBlob(Tcl_GetString(objv[2]),
			      bytes,
			      (uint32_t)size);
	break;
      }
    case SQLRCUR_inputBindClob:
      {
	long size;
	if (objc != 5) {
	  Tcl_WrongNumArgs(interp,2, objv, "variable value size");
	  return TCL_ERROR;
	}
	if (Tcl_GetLongFromObj(interp, objv[4], &size) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_Size bytelen = 0;
	const char *bytes = (const char *)Tcl_GetByteArrayFromObj(objv[3], &bytelen);
	cur->inputBindClob(Tcl_GetString(objv[2]),
			      bytes,
			      (uint32_t)size);
	break;
      }
    case SQLRCUR_defineOutputBindString:
      {
	long length;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp,2, objv, "variable length");
	  return TCL_ERROR;
	}
	if (Tcl_GetLongFromObj(interp, objv[3], &length) != TCL_OK) {
	  return TCL_ERROR;
	}
	cur->defineOutputBindString(Tcl_GetString(objv[2]), length);
	break;
      }
    case SQLRCUR_defineOutputBindInteger:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,1, objv, "variable");
	  return TCL_ERROR;
	}
	cur->defineOutputBindInteger(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_defineOutputBindDouble:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,1, objv, "variable");
	  return TCL_ERROR;
	}
	cur->defineOutputBindDouble(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_defineOutputBindDate:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "variable");
	  return TCL_ERROR;
	}
	cur->defineOutputBindDate(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_defineOutputBindBlob:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "variable");
	  return TCL_ERROR;
	}
	cur->defineOutputBindBlob(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_defineOutputBindClob:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp,2, objv, "variable");
	  return TCL_ERROR;
	}
	cur->defineOutputBindClob(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_defineOutputBindCursor:
      {
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	cur->defineOutputBindCursor(Tcl_GetString(objv[2]));
	break;
      }
    case SQLRCUR_substitutions:
      {
	Tcl_Size num = 0, len = 0, i = 0;
	Tcl_Obj **argList, *variableObj, *valueObj, *precisionObj, *scaleObj;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "{{variable value ?precision scale?}...}");
	  return TCL_ERROR;
	}
	if (Tcl_ListObjGetElements(interp, objv[2], &num, &argList) != TCL_OK) {
	  return TCL_ERROR;
	}
	for (i = 0; i < num; i++) {
	  if (Tcl_ListObjLength(interp, argList[i], &len) != TCL_OK) {
	    return TCL_ERROR;
	  } else if (len == 4) {
	    double value;
	    int precision, scale;
	    if (Tcl_ListObjIndex(interp, argList[i], 0, 
				 &variableObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 1,
				 &valueObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 2,
				 &precisionObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 3,
				 &scaleObj) != TCL_OK ||
		Tcl_GetDoubleFromObj(interp, valueObj,
				     &value) != TCL_OK ||
		Tcl_GetIntFromObj(interp, precisionObj,
				  &precision) != TCL_OK ||
		Tcl_GetIntFromObj(interp, scaleObj,
				  &scale) != TCL_OK) {
	      return TCL_ERROR;
	    }
	    cur->substitution(Tcl_GetString(variableObj),
			      value, precision, scale);
	  } else if (len == 2) {
	    long value;
	    if (Tcl_ListObjIndex(interp, argList[i], 0,
				 &variableObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 1,
				 &valueObj) != TCL_OK) {
	      return TCL_ERROR;
	    }
	    if ( Tcl_GetLongFromObj(interp, valueObj, &value) == TCL_OK ||
		 Tcl_GetIntFromObj(interp, valueObj, (int *)&value) == TCL_OK ) {
	      cur->substitution(Tcl_GetString(variableObj), value);
	    } else {
	      cur->substitution(Tcl_GetString(variableObj),
				Tcl_GetString(valueObj));
	    }
	  } else {
	    Tcl_WrongNumArgs(interp, 2, objv, "{{variable value ?precision scale?} ...}");
	    return TCL_ERROR;
	  }
	}
	break;
      }
    case SQLRCUR_inputBinds:
      {
	Tcl_Size num = 0, len = 0, i = 0;
	Tcl_Obj **argList, *variableObj, *valueObj, *precisionObj, *scaleObj;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "{{variable value ?precision scale?}...}");
	  return TCL_ERROR;
	}
	if (Tcl_ListObjGetElements(interp, objv[2], &num, &argList) != TCL_OK) {
	  return TCL_ERROR;
	}
	for (i = 0; i < num; i++) {
	  if (Tcl_ListObjLength(interp, argList[i], &len) != TCL_OK) {
	    return TCL_ERROR;
	  } else if (len == 4) {
	    double value;
	    int precision, scale;
	    if (Tcl_ListObjIndex(interp, argList[i], 0, 
				 &variableObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 1,
				 &valueObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 2,
				 &precisionObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 3,
				 &scaleObj) != TCL_OK ||
		Tcl_GetDoubleFromObj(interp, valueObj,
				     &value) != TCL_OK ||
		Tcl_GetIntFromObj(interp, precisionObj,
				  &precision) != TCL_OK ||
		Tcl_GetIntFromObj(interp, scaleObj,
				  &scale) != TCL_OK) {
	      return TCL_ERROR;
	    }
	    cur->inputBind(Tcl_GetString(variableObj),
			   value, precision, scale);
	  } else if (len == 2) {
	    long value;
	    if (Tcl_ListObjIndex(interp, argList[i], 0,
				 &variableObj) != TCL_OK ||
		Tcl_ListObjIndex(interp, argList[i], 1,
				 &valueObj) != TCL_OK) {
	      return TCL_ERROR;
	    }
	    if ( Tcl_GetLongFromObj(interp, valueObj, &value) == TCL_OK ||
		 Tcl_GetIntFromObj(interp, valueObj, (int *)&value) == TCL_OK ) {
	      cur->inputBind(Tcl_GetString(variableObj), value);
	    } else {
	      cur->inputBind(Tcl_GetString(variableObj),
			     Tcl_GetString(valueObj));
	    }
	  } else {
	    Tcl_WrongNumArgs(interp, 2, objv, "{{variable value ?precision scale?} ...}");
	    return TCL_ERROR;
	  }
	}
	break;
      }
    case SQLRCUR_validateBinds:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	cur->validateBinds();
	break;
      }
    case SQLRCUR_validBind:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewBooleanObj(cur->validBind(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_executeQuery:
      {
	int result = 0;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	if (!(result = cur->executeQuery())) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_fetchFromBindCursor:
      {
	int result = 0;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	if (!(result = cur->fetchFromBindCursor())) {
	  Tcl_AppendResult(interp,cur->errorMessage(),(char *)NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	break;
      }
    case SQLRCUR_getOutputBindString:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = _Tcl_NewStringObj(cur->getOutputBindString(Tcl_GetString(objv[2])), cur->getOutputBindLength(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindBlob:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = _Tcl_NewStringObj(cur->getOutputBindBlob(Tcl_GetString(objv[2])), cur->getOutputBindLength(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindClob:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = _Tcl_NewStringObj(cur->getOutputBindClob(Tcl_GetString(objv[2])), cur->getOutputBindLength(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindInteger:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewLongObj(cur->getOutputBindInteger(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDouble:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewDoubleObj(cur->getOutputBindDouble(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindLength:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewLongObj(cur->getOutputBindLength(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindCursor:
      {
	Tcl_Obj *id;
	sqlrcursor *newcur = (sqlrcursor *)NULL;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	newcur = cur->getOutputBindCursor(Tcl_GetString(objv[2]),true);
	if (newcur != (sqlrcursor *)NULL) {
	  id = getCursorID();
	  Tcl_CreateObjCommand(interp,
			       Tcl_GetString(id),
			       sqlrcurObjCmd,
			       (ClientData)newcur,
			       (Tcl_CmdDeleteProc *)sqlrcurDelete);
	  Tcl_SetObjResult(interp, id);
	} else {
	  return TCL_ERROR;
	}
	break;
      }
    case SQLRCUR_getOutputBindDateYear:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->getOutputBindDateYear(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateMonth:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->getOutputBindDateMonth(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateDay:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->getOutputBindDateDay(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateHour:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->getOutputBindDateHour(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateMinute:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->getOutputBindDateMinute(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateSecond:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->getOutputBindDateSecond(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateMicrosecond:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->getOutputBindDateMicrosecond(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateTz:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = _Tcl_NewStringObj(cur->getOutputBindDateTz(Tcl_GetString(objv[2])), -1);
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_getOutputBindDateIsNegative:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewBooleanObj(cur->getOutputBindDateIsNegative(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_openCachedResultSet:
      {
	Tcl_Obj *result;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "variable");
	  return TCL_ERROR;
	}
	result = Tcl_NewBooleanObj(cur->openCachedResultSet(Tcl_GetString(objv[2])));
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_colCount:
      {
	Tcl_Obj *result;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->colCount());
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_rowCount:
      {
	Tcl_Obj *result;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->rowCount());
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_totalRows:
      {
	Tcl_Obj *result;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->totalRows());
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_affectedRows:
      {
	Tcl_Obj *result;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->affectedRows());
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_firstRowIndex:
      {
	Tcl_Obj *result;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewIntObj(cur->firstRowIndex());
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_endOfResultSet:
      {
	Tcl_Obj *result;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewBooleanObj(cur->endOfResultSet());
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_nextResultSet:
      {
	Tcl_Obj *result;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	result = Tcl_NewBooleanObj(cur->nextResultSet());
	Tcl_SetObjResult(interp, result);
	break;
      }
    case SQLRCUR_errorMessage:
      {
	const char *msg;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	if ((msg = cur->errorMessage()) == NULL) {
	  msg = "";
	}
	Tcl_SetObjResult(interp, _Tcl_NewStringObj(msg, -1));
	break;
      }
    case SQLRCUR_errorNumber:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewLongObj(cur->errorNumber()));
	break;
      }
    case SQLRCUR_errorSqlState:
      {
	const char *state;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	if ((state = cur->errorSqlState()) == NULL) {
	  state = "";
	}
	Tcl_SetObjResult(interp, _Tcl_NewStringObj(state, -1));
	break;
      }
      /*
    case SQLRCUR_getNullsAsEmptyStrings:
    case SQLRCUR_getNullsAsNulls:
      */
    case SQLRCUR_getFieldByIndex:
      {
	int row, col;
	const char *field = (const char *)NULL;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	if ((field = cur->getField(row, col)) == (const char *)NULL) {
	  field = "";
	}
	Tcl_SetObjResult(interp, Tcl_NewByteArrayObj((unsigned char *)field, cur->getFieldLength(row,col)));
	break;
      }
    case SQLRCUR_getFieldByName:
      {
	int row;
	const char *field = (const char *)NULL;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	if ((field = cur->getField(row, Tcl_GetString(objv[3]))) == (const char *)NULL) {
	  field = "";
	}
	Tcl_SetObjResult(interp, Tcl_NewByteArrayObj((unsigned char *)field, cur->getFieldLength(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldByNameIgnoringCase:
      {
	int row;
	const char *field = (const char *)NULL;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	if ((field = cur->getFieldIgnoringCase(row, Tcl_GetString(objv[3]))) == (const char *)NULL) {
	  field = "";
	}
	Tcl_SetObjResult(interp, Tcl_NewByteArrayObj((unsigned char *)field, cur->getFieldLength(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsIntegerByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewLongObj(cur->getFieldAsInteger(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsIntegerByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewLongObj(cur->getFieldAsInteger(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsIntegerByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewLongObj(cur->getFieldAsIntegerIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDoubleByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(cur->getFieldAsDouble(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDoubleByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(cur->getFieldAsDouble(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDoubleByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(cur->getFieldAsDoubleIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsBooleanByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsBoolean(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsBooleanByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsBoolean(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsBooleanByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsBooleanIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateYearByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateYear(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateYearByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateYear(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateYearByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateYearIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateYearByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateYear(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateYearByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateYear(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateYearByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateYearIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMonthByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMonth(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateMonthByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMonth(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMonthByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMonthIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMonthByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMonth(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMonthByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMonth(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMonthByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMonthIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateDayByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateDay(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateDayByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateDay(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateDayByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateDayIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateDayByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateDay(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateDayByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateDay(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateDayByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateDayIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateHourByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateHour(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateHourByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateHour(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateHourByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateHourIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateHourByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateHour(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateHourByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateHour(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateHourByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateHourIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMinuteByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMinute(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateMinuteByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMinute(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMinuteByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMinuteIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMinuteByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMinute(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMinuteByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMinute(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMinuteByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMinuteIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateSecondByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateSecond(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateSecondByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateSecond(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateSecondByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateSecondIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateSecondByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateSecond(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateSecondByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateSecond(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateSecondByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateSecondIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMicrosecondByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMicrosecond(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateMicrosecondByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMicrosecond(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMicrosecondByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMicrosecondIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMicrosecondByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMicrosecond(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMicrosecondByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMicrosecond(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getFieldAsDateMicrosecondIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateIsNegativeByIndex:
      {
	int row, col;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsDateIsNegative(row, col)));
	break;
      }
    case SQLRCUR_getFieldAsDateIsNegativeByName:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsDateIsNegative(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateIsNegativeByNameIgnoringCase:
      {
	int row;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsDateIsNegativeIgnoringCase(row, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_getFieldAsDateIsNegativeByIndexWithDdMm:
      {
	int row, col, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsDateIsNegative(row, col, ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateIsNegativeByNameWithDdMm:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsDateIsNegative(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase:
      {
	int row, ddmm, yyyyddmm;
	if (objc != 7) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col ddmm yyyyddmm datedelimiters");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[4], &ddmm) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[5], &yyyyddmm) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(cur->getFieldAsDateIsNegativeIgnoringCase(row, Tcl_GetString(objv[3]), ddmm!=0, yyyyddmm!=0, Tcl_GetString(objv[6]))));
	break;
      }
    case SQLRCUR_getFieldLengthByIndex:
      {
	int row, col;
	long length;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK ||
	    Tcl_GetIntFromObj(interp, objv[3], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	length = cur->getFieldLength(row, col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(length));
	break;
      }
    case SQLRCUR_getFieldLengthByName:
      {
	int row;
	long length;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	length = cur->getFieldLength(row, Tcl_GetString(objv[3]));
	Tcl_SetObjResult(interp, Tcl_NewLongObj(length));
	break;
      }
    case SQLRCUR_getRow:
      {
	Tcl_WideInt row;
	uint32_t col;
	const char * const *rowarray;
	uint32_t *lengtharray;
	Tcl_Obj *resultList;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row");
	  return TCL_ERROR;
	}
	if (Tcl_GetWideIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	rowarray = cur->getRow(row);
	resultList = Tcl_NewObj();
	if (!rowarray) {
	  Tcl_SetObjResult(interp, resultList);
	  break;
	}
	lengtharray = cur->getRowLengths(row);
	for (col = 0; col < cur->colCount(); col++) {
	  if (Tcl_ListObjAppendElement(interp, resultList,
				       Tcl_NewByteArrayObj((unsigned char *)((rowarray[col])?rowarray[col]:""), lengtharray[col])) != TCL_OK) {
	    return TCL_ERROR;
	  }
	}
	Tcl_SetObjResult(interp, resultList);
	break;
      }
    case SQLRCUR_getRowLengths:
      {
	Tcl_WideInt row;
	uint32_t col;
	uint32_t *lenarray;
	Tcl_Obj *resultList;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "row");
	  return TCL_ERROR;
	}
	if (Tcl_GetWideIntFromObj(interp, objv[2], &row) != TCL_OK) {
	  return TCL_ERROR;
	}
	lenarray = cur->getRowLengths(row);
	resultList = Tcl_NewObj();
	if (!lenarray) {
	  Tcl_SetObjResult(interp, resultList);
	  break;
	}
	for (col = 0; col < cur->colCount(); col++) {
	  if (Tcl_ListObjAppendElement(interp, resultList,
				       Tcl_NewLongObj(lenarray[col])) != TCL_OK) {
	    return TCL_ERROR;
	  }
	}
	Tcl_SetObjResult(interp, resultList);
	break;
      }
    case SQLRCUR_getColumnNames:
      {
	int i = 0;
	const char * const *namearray;
	Tcl_Obj *resultList;
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	namearray = cur->getColumnNames();
	resultList = Tcl_NewObj();
	while (namearray[i] != (const char *)NULL) {
	  if (Tcl_ListObjAppendElement(interp, resultList,
				       _Tcl_NewStringObj(namearray[i++], -1)) != TCL_OK) {
	    return TCL_ERROR;
	  }
	}
	Tcl_SetObjResult(interp,resultList);
	break;
      }
    case SQLRCUR_getColumnName:
      {
	int col;
	const char *name = (const char *)NULL;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	if ((name = cur->getColumnName(col)) == (const char *)NULL) {
	  name = "";
	}
	Tcl_SetObjResult(interp, _Tcl_NewStringObj(name, -1));
	break;
      }
    case SQLRCUR_getColumnTypeByIndex:
      {
	int col;
	const char *name = (const char *)NULL;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	if ((name = cur->getColumnType(col)) == (const char *)NULL) {
	  name = "";
	}
	Tcl_SetObjResult(interp, _Tcl_NewStringObj(name, -1));
	break;
      }
    case SQLRCUR_getColumnTypeByName:
      {
	const char *name = (const char *)NULL;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if ((name = cur->getColumnType(Tcl_GetString(objv[2]))) == (const char *)NULL) {
	  name = "";
	}
	Tcl_SetObjResult(interp, _Tcl_NewStringObj(name, -1));
	break;
      }
    case SQLRCUR_getColumnLengthByIndex:
      {
	int col;
	uint32_t len;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	len = cur->getColumnLength(col);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(len));
	break;
      }
    case SQLRCUR_getColumnLengthByName:
      {
	int len;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	len = cur->getColumnLength(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(len));
	break;
      }
    case SQLRCUR_getColumnPrecisionByIndex:
      {
	int col;
	uint32_t precision;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	precision = cur->getColumnPrecision(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(precision));
	break;
      }
    case SQLRCUR_getColumnPrecisionByName:
      {
	uint32_t precision;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	precision = cur->getColumnPrecision(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewLongObj(precision));
	break;
      }
    case SQLRCUR_getColumnScaleByIndex:
      {
	int col;
	uint32_t scale;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	scale = cur->getColumnScale(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(scale));
	break;
      }
    case SQLRCUR_getColumnScaleByName:
      {
	uint32_t scale;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	scale = cur->getColumnScale(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewLongObj(scale));
	break;
      }
    case SQLRCUR_getColumnIsNullableByIndex:
      {
	int col;
	bool isnullable;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	isnullable = cur->getColumnIsNullable(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(isnullable));
	break;
      }
    case SQLRCUR_getColumnIsNullableByName:
      {
	bool isnullable;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	isnullable = cur->getColumnIsNullable(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(isnullable));
	break;
      }
    case SQLRCUR_getColumnIsPrimaryKeyByIndex:
      {
	int col;
	bool isprimarykey;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	isprimarykey = cur->getColumnIsPrimaryKey(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(isprimarykey));
	break;
      }
    case SQLRCUR_getColumnIsPrimaryKeyByName:
      {
	bool isprimarykey;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	isprimarykey = cur->getColumnIsPrimaryKey(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(isprimarykey));
	break;
      }
    case SQLRCUR_getColumnIsUniqueByIndex:
      {
	int col;
	bool isunique;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	isunique = cur->getColumnIsUnique(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(isunique));
	break;
      }
    case SQLRCUR_getColumnIsUniqueByName:
      {
	bool isunique;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	isunique = cur->getColumnIsUnique(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(isunique));
	break;
      }
    case SQLRCUR_getColumnIsPartOfKeyByIndex:
      {
	int col;
	bool ispartofkey;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	ispartofkey = cur->getColumnIsPartOfKey(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(ispartofkey));
	break;
      }
    case SQLRCUR_getColumnIsPartOfKeyByName:
      {
	bool ispartofkey;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	ispartofkey = cur->getColumnIsPartOfKey(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(ispartofkey));
	break;
      }
    case SQLRCUR_getColumnIsUnsignedByIndex:
      {
	int col;
	bool isunsigned;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	isunsigned = cur->getColumnIsUnsigned(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(isunsigned));
	break;
      }
    case SQLRCUR_getColumnIsUnsignedByName:
      {
	bool isunsigned;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	isunsigned = cur->getColumnIsUnsigned(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(isunsigned));
	break;
      }
    case SQLRCUR_getColumnIsZeroFilledByIndex:
      {
	int col;
	bool iszerofilled;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	iszerofilled = cur->getColumnIsZeroFilled(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(iszerofilled));
	break;
      }
    case SQLRCUR_getColumnIsZeroFilledByName:
      {
	bool iszerofilled;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	iszerofilled = cur->getColumnIsZeroFilled(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(iszerofilled));
	break;
      }
    case SQLRCUR_getColumnIsBinaryByIndex:
      {
	int col;
	bool isbinary;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	isbinary = cur->getColumnIsBinary(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(isbinary));
	break;
      }
    case SQLRCUR_getColumnIsBinaryByName:
      {
	bool isbinary;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	isbinary = cur->getColumnIsBinary(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(isbinary));
	break;
      }
    case SQLRCUR_getColumnIsAutoIncrementByIndex:
      {
	int col;
	bool isautoinc;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	isautoinc = cur->getColumnIsAutoIncrement(col);
	Tcl_SetObjResult(interp, Tcl_NewLongObj(isautoinc));
	break;
      }
    case SQLRCUR_getColumnIsAutoIncrementByName:
      {
	bool isautoinc;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	isautoinc = cur->getColumnIsAutoIncrement(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(isautoinc));
	break;
      }
    case SQLRCUR_getLongestByIndex:
      {
	int col;
	uint32_t len;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &col) != TCL_OK) {
	  return TCL_ERROR;
	}
	len = cur->getLongest(col);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(len));
	break;
      }
    case SQLRCUR_getLongestByName:
      {
	int len;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "col");
	  return TCL_ERROR;
	}
	len = cur->getLongest(Tcl_GetString(objv[2]));
	Tcl_SetObjResult(interp, Tcl_NewIntObj(len));
	break;
      }
    case SQLRCUR_getResultSetId:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(cur->getResultSetId()));
	break;
      }
    case SQLRCUR_suspendResultSet:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	cur->suspendResultSet();
	break;
      }
    case SQLRCUR_resumeResultSet:
      {
	int id;
	if (objc != 3) {
	  Tcl_WrongNumArgs(interp, 2, objv, "id");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &id) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp,
			 Tcl_NewBooleanObj(cur->resumeResultSet(id)));
	break;
      }
    case SQLRCUR_resumeCachedResultSet:
      {
	int id;
	if (objc != 4) {
	  Tcl_WrongNumArgs(interp, 2, objv, "id filename");
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &id) != TCL_OK) {
	  return TCL_ERROR;
	}
	Tcl_SetObjResult(interp,
			 Tcl_NewBooleanObj(cur->resumeCachedResultSet(id, Tcl_GetString(objv[3]))));
	break;
      }
    case SQLRCUR_closeResultSet:
      {
	if (objc > 2) {
	  Tcl_WrongNumArgs(interp, 2, objv, NULL);
	  return TCL_ERROR;
	}
	cur->closeResultSet();
	break;
      }
    }
  return TCL_OK;
}

/*
 * sqlrcurCmd --
 *    create new sqlrcur object command. This command itselfs is a
 *    subcommand of sqlrcon object command. see below.
 */
int sqlrcurCmd(ClientData data, Tcl_Interp *interp,
	       int objc, Tcl_Obj *CONST objv[]) {
  sqlrconnection *con = (sqlrconnection *)data;
  sqlrcursor *cur = (sqlrcursor *)NULL;
  Tcl_Obj *id;

  if ((cur = new sqlrcursor(con,true)) == (sqlrcursor *)NULL) {
    Tcl_AppendResult(interp, "cannot allocate sqlrcursor", (char *)NULL);
    return TCL_ERROR;
  }

  id = getCursorID();
  Tcl_CreateObjCommand(interp,
		       Tcl_GetString(id),
		       sqlrcurObjCmd,
		       (ClientData)cur,
		       (Tcl_CmdDeleteProc *)sqlrcurDelete);
  Tcl_SetObjResult(interp, id);
  return TCL_OK;
}

/*
 * sqlrconDelete --
 */
void sqlrconDelete(ClientData data) {
  sqlrconnection *con = (sqlrconnection *)data;
  if (con != (sqlrconnection *)NULL) {
    delete con;
    con = (sqlrconnection *)NULL;
  }
}

/*
 * sqlrconObjCmd --
 *    sqlrcon object command.
 * Synopsis:
 *  $con free
 *  $con setConnectTimeout
 *  $con getConnectTimeoutSeconds
 *  $con getConnectTimeoutMicroseconds
 *  $con setResponseTimeout
 *  $con getResponseTimeoutSeconds
 *  $con getResponseTimeoutMicroseconds
 *  $con setBindVariableDelimiters
 *  $con getBindVariableDelimiterQuestionMarkSupported
 *  $con getBindVariableDelimiterColonSupported
 *  $con getBindVariableDelimiterAtSignSupported
 *  $con getBindVariableDelimiterDollarSignSupported
 *  $con enableKerberos
 *  $con enableTls
 *  $con disableEncryption
 *  $con endSession
 *  $con suspendSession
 *  $con getConnectionPort
 *  $con getConnectionSocket
 *  $con resumeSession port socket
 *  $con ping
 *  $con identify
 *  $con dbVersion
 *  $con dbHostName
 *  $con dbIpAddress
 *  $con serverVersion
 *  $con clientVersion
 *  $con bindFormat
 *  $con nextvalFormat
 *  $con selectDatabase db
 *  $con getCurrentDatabase
 *  $con selectCatalog catalog
 *  $con getCurrentCatalog
 *  $con selectSchema schema
 *  $con getCurrentSchema
 *  $con getDatabaseIsSchema
 *  $con getCurrentUser
 *  $con getLastInsertId
 *  $con autoCommit bool
 *  $con begin
 *  $con commit
 *  $con rollback
 *  $con errorMessage
 *  $con errorSqlState
 *  $con debug ?bool?
 *  $con setDebugFile debugfilename
 *  $con setClientInfo clientinfo
 *  $con getClientInfo
 *  $con sqlrcur
 *     set cur [$con sqlrcur]
 */
int sqlrconObjCmd(ClientData data, Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[]) {
  sqlrconnection *con = (sqlrconnection *)data;
  int index;
  static CONSTCHAR *options[] = {
    "free",
    "setConnectTimeout",
    "getConnectTimeoutSeconds",
    "getConnectTimeoutMicroseconds",
    "setResponseTimeout",
    "getResponseTimeoutSeconds",
    "getResponseTimeoutMicroseconds",
    "setBindVariableDelimiters",
    "getBindVariableDelimiterQuestionMarkSupported",
    "getBindVariableDelimiterColonSupported",
    "getBindVariableDelimiterAtSignSupported",
    "getBindVariableDelimiterDollarSignSupported",
    "enableKerberos",
    "enableTls",
    "disableEncryption",
    "endSession",
    "suspendSession",
    "getConnectionPort",
    "getConnectionSocket",
    "resumeSession",
    "ping",
    "identify",
    "dbVersion",
    "dbHostName",
    "dbIpAddress",
    "serverVersion",
    "clientVersion",
    "bindFormat",
    "nextvalFormat",
    "selectDatabase",
    "getCurrentDatabase",
    "selectCatalog",
    "getCurrentCatalog",
    "selectSchema",
    "getCurrentSchema",
    "getDatabaseIsSchema",
    "getCurrentUser",
    "getLastInsertId",
    "autoCommit",
    "getAutoCommit",
    "begin",
    "commit",
    "rollback",
    "getInTransaction",
    "getDefaultTransactionModel",
    "setTransactionModel",
    "getTransactionModel",
    "setIsolationLevel",
    "getIsolationLevel",
    "getDatabaseFeature",
    "errorMessage",
    "errorNumber",
    "errorSqlState",
    "debug",
    "setDebugFile",
    "setClientInfo",
    "getClientInfo",
    "sqlrcur",
  };
  enum options {
    SQLR_FREE,
    SQLR_SETCONNECTTIMEOUT,
    SQLR_GETCONNECTTIMEOUTSECONDS,
    SQLR_GETCONNECTTIMEOUTMICROSECONDS,
    SQLR_SETRESPONSETIMEOUT,
    SQLR_GETRESPONSETIMEOUTSECONDS,
    SQLR_GETRESPONSETIMEOUTMICROSECONDS,
    SQLR_SETBINDVARIABLEDELIMITERS,
    SQLR_GETBINDVARIABLEDELIMITERQUESTIONMARKSUPPORTED,
    SQLR_GETBINDVARIABLEDELIMITERCOLONSUPPORTED,
    SQLR_GETBINDVARIABLEDELIMITERATSIGNSUPPORTED,
    SQLR_GETBINDVARIABLEDELIMITERDOLLARSIGNSUPPORTED,
    SQLR_ENABLEKERBEROS,
    SQLR_ENABLETLS,
    SQLR_DISABLEENCRYPTION,
    SQLR_ENDSESSION,
    SQLR_SUSPENDSESSION,
    SQLR_GETCONNECTIONPORT,
    SQLR_GETCONNECTIONSOCKET,
    SQLR_RESUMESESSION,
    SQLR_PING,
    SQLR_IDENTIFY,
    SQLR_DBVERSION,
    SQLR_DBHOSTNAME,
    SQLR_DBIPADDRESS,
    SQLR_SERVERVERSION,
    SQLR_CLIENTVERSION,
    SQLR_BINDFORMAT,
    SQLR_NEXTVALFORMAT,
    SQLR_SELECTDATABASE,
    SQLR_GETCURRENTDATABASE,
    SQLR_SELECTCATALOG,
    SQLR_GETCURRENTCATALOG,
    SQLR_SELECTSCHEMA,
    SQLR_GETCURRENTSCHEMA,
    SQLR_GETDATABASEISSCHEMA,
    SQLR_GETCURRENTUSER,
    SQLR_GETLASTINSERTID,
    SQLR_AUTOCOMMIT,
    SQLR_GETAUTOCOMMIT,
    SQLR_BEGIN,
    SQLR_COMMIT,
    SQLR_ROLLBACK,
    SQLR_GETINTRANSACTION,
    SQLR_GETDEFAULTTRANSACTIONMODEL,
    SQLR_SETTRANSACTIONMODEL,
    SQLR_GETTRANSACTIONMODEL,
    SQLR_SETISOLATIONLEVEL,
    SQLR_GETISOLATIONLEVEL,
    SQLR_GETDATABASEFEATURE,
    SQLR_ERRORMESSAGE,
    SQLR_ERRORNUMBER,
    SQLR_ERRORSQLSTATE,
    SQLR_DEBUG,
    SQLR_SETDEBUGFILE,
    SQLR_SETCLIENTINFO,
    SQLR_GETCLIENTINFO,
    SQLR_SQLRCUR,
  };

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(interp, objv[1], (CONSTCHAR **)options, "option", 0,
			  (int *)&index) != TCL_OK) {
    return TCL_ERROR;
  }

  switch ((enum options)index) {
  case SQLR_FREE: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    sqlrconDelete(con);
    break;
  }
  case SQLR_SETCONNECTTIMEOUT: {
    int timeoutsec;
    int timeoutusec;
    if (objc > 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "timeoutsec timeoutusec");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &timeoutsec) != TCL_OK) {
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &timeoutusec) != TCL_OK) {
      return TCL_ERROR;
    }
    con->setConnectTimeout(timeoutsec,timeoutusec);
    break;
  }
  case SQLR_GETCONNECTTIMEOUTSECONDS: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getConnectTimeoutSeconds()));
    break;
  }
  case SQLR_GETCONNECTTIMEOUTMICROSECONDS: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getConnectTimeoutMicroseconds()));
    break;
  }
  case SQLR_SETRESPONSETIMEOUT: {
    int timeoutsec;
    int timeoutusec;
    if (objc > 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "timeoutsec timeoutusec");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &timeoutsec) != TCL_OK) {
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &timeoutusec) != TCL_OK) {
      return TCL_ERROR;
    }
    con->setResponseTimeout(timeoutsec,timeoutusec);
    break;
  }
  case SQLR_GETRESPONSETIMEOUTSECONDS: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getResponseTimeoutSeconds()));
    break;
  }
  case SQLR_GETRESPONSETIMEOUTMICROSECONDS: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getResponseTimeoutMicroseconds()));
    break;
  }
  case SQLR_SETBINDVARIABLEDELIMITERS: {
    const char *delimiter;

    if (objc > 3) {
      Tcl_WrongNumArgs(interp, 1, objv, "delimiter");
      return TCL_ERROR;
    }

    delimiter = Tcl_GetString(objv[2]);

    con->setBindVariableDelimiters(delimiter);
    break;
  }
  case SQLR_GETBINDVARIABLEDELIMITERQUESTIONMARKSUPPORTED: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getBindVariableDelimiterQuestionMarkSupported()));
  }
  case SQLR_GETBINDVARIABLEDELIMITERCOLONSUPPORTED: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getBindVariableDelimiterColonSupported()));
  }
  case SQLR_GETBINDVARIABLEDELIMITERATSIGNSUPPORTED: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getBindVariableDelimiterAtSignSupported()));
  }
  case SQLR_GETBINDVARIABLEDELIMITERDOLLARSIGNSUPPORTED: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(
			con->getBindVariableDelimiterDollarSignSupported()));
  }
  case SQLR_ENABLEKERBEROS: {
    const char *service;
    const char *mech;
    const char *flags;

    if (objc > 5) {
      Tcl_WrongNumArgs(interp, 3, objv, "service mech flags");
      return TCL_ERROR;
    }

    service = Tcl_GetString(objv[2]);
    mech = Tcl_GetString(objv[3]);
    flags = Tcl_GetString(objv[4]);

    con->enableKerberos(service, mech, flags);
    break;
  }
  case SQLR_ENABLETLS: {
    const char *version;
    const char *cert;
    const char *password;
    const char *ciphers;
    const char *validate;
    const char *ca;
    int depth;

    if (objc > 9) {
      Tcl_WrongNumArgs(interp, 7, objv, "version cert password ciphers validate ca depth");
      return TCL_ERROR;
    }

    version = Tcl_GetString(objv[2]);
    cert = Tcl_GetString(objv[3]);
    password = Tcl_GetString(objv[4]);
    ciphers = Tcl_GetString(objv[5]);
    validate = Tcl_GetString(objv[6]);
    ca = Tcl_GetString(objv[7]);
    if (Tcl_GetIntFromObj(interp, objv[8], &depth) != TCL_OK) {
      return TCL_ERROR;
    }

    con->enableTls(version, cert, password, ciphers, validate, ca, depth);
    break;
  }
  case SQLR_DISABLEENCRYPTION: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    con->disableEncryption();
    break;
  }
  case SQLR_ENDSESSION: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    con->endSession();
    break;
  }
  case SQLR_SUSPENDSESSION: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->suspendSession()));
    break;
  }
  case SQLR_GETCONNECTIONPORT: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, "getConnectionPort");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->getConnectionPort()));
    break;
  }
  case SQLR_GETCONNECTIONSOCKET: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->getConnectionSocket(),
				      -1));
    break;
  }
  case SQLR_RESUMESESSION: {
    int port;
    const char *socket;

    if (objc != 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "port socket");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &port) != TCL_OK) {
      return TCL_ERROR;
    }
    socket = Tcl_GetString(objv[3]);

    Tcl_SetObjResult(interp,
		     Tcl_NewBooleanObj(con->resumeSession(port, socket)));
    break;
  }
  case SQLR_PING: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     Tcl_NewBooleanObj(con->ping()));
    break;
  }
  case SQLR_IDENTIFY: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->identify(), -1));
    break;
  }
  case SQLR_DBVERSION: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->dbVersion(), -1));
    break;
  }
  case SQLR_DBHOSTNAME: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->dbHostName(), -1));
    break;
  }
  case SQLR_DBIPADDRESS: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->dbIpAddress(), -1));
    break;
  }
  case SQLR_CLIENTVERSION: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->clientVersion(), -1));
    break;
  }
  case SQLR_SERVERVERSION: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->serverVersion(), -1));
    break;
  }
  case SQLR_BINDFORMAT: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->bindFormat(), -1));
    break;
  }
  case SQLR_NEXTVALFORMAT: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->nextvalFormat(), -1));
    break;
  }
  case SQLR_SELECTDATABASE: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp,2, objv, "db");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->selectDatabase(Tcl_GetString(objv[2]))));
    break;
  }
  case SQLR_GETCURRENTDATABASE: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->getCurrentDatabase(), -1));
    break;
  }
  case SQLR_SELECTCATALOG: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp,2, objv, "catalog");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->selectCatalog(Tcl_GetString(objv[2]))));
    break;
  }
  case SQLR_GETCURRENTCATALOG: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->getCurrentCatalog(), -1));
    break;
  }
  case SQLR_SELECTSCHEMA: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp,2, objv, "schema");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->selectSchema(Tcl_GetString(objv[2]))));
    break;
  }
  case SQLR_GETCURRENTSCHEMA: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     _Tcl_NewStringObj(con->getCurrentSchema(), -1));
    break;
  }
  case SQLR_GETDATABASEISSCHEMA: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->getDatabaseIsSchema()));
    break;
  }
  case SQLR_GETCURRENTUSER: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->getCurrentUser(), -1));
    break;
  }
  case SQLR_GETLASTINSERTID: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->getLastInsertId()));
    break;
  }
  case SQLR_AUTOCOMMIT: {
    int flag = 0;
    if (objc !=3) {
      Tcl_WrongNumArgs(interp, 2, objv, "bool");
      return TCL_ERROR;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[2], &flag) != TCL_OK) {
      return TCL_ERROR;
    }
    if (flag) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(con->autoCommitOn()));
    } else {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(con->autoCommitOff()));
    }
    break;
  }
  case SQLR_GETAUTOCOMMIT: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->getAutoCommit()));
    break;
  }
  case SQLR_BEGIN: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->begin()));
    break;
  }
  case SQLR_COMMIT: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->commit()));
    break;
  }
  case SQLR_ROLLBACK:
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->rollback()));
    break;
  case SQLR_GETINTRANSACTION:
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->getInTransaction()));
    break;
  case SQLR_GETDEFAULTTRANSACTIONMODEL: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->getDefaultTransactionModel(), -1));
    break;
  }
  case SQLR_SETTRANSACTIONMODEL: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp, 2, objv, "txmodel");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->setTransactionModel(Tcl_GetString(objv[2]))));
    break;
  }
  case SQLR_GETTRANSACTIONMODEL: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->getTransactionModel(), -1));
    break;
  }
  case SQLR_SETISOLATIONLEVEL: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp, 2, objv, "isolationlevel");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(con->setIsolationLevel(Tcl_GetString(objv[2]))));
    break;
  }
  case SQLR_GETISOLATIONLEVEL: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->getIsolationLevel(), -1));
    break;
  }
  case SQLR_GETDATABASEFEATURE: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp, 2, objv, "feature");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->getDatabaseFeature(Tcl_GetString(objv[2])), -1));
    break;
  }
  case SQLR_ERRORMESSAGE: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->errorMessage(), -1));
    break;
  }
  case SQLR_ERRORNUMBER: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewLongObj(con->errorNumber()));
    break;
  }
  case SQLR_ERRORSQLSTATE: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->errorSqlState(), -1));
    break;
  }
  case SQLR_DEBUG: {
    int flag = 0;
    if (objc == 2) {
      Tcl_SetObjResult(interp,
		       Tcl_NewBooleanObj(con->getDebug()));
      break;
    } else if (objc == 3) {
      if (Tcl_GetBooleanFromObj(interp, objv[2], &flag) != TCL_OK) {
	return TCL_ERROR;
      }
      if (flag) {
	con->debugOn();
      } else {
	con->debugOff();
      }
    } else {
      Tcl_WrongNumArgs(interp, 2, objv, "debug ?bool?");
      return TCL_ERROR;
    }
  }
  case SQLR_SETDEBUGFILE: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp,2, objv, "debugfilename");
      return TCL_ERROR;
    }
    con->setDebugFile(Tcl_GetString(objv[2]));
    break;
  }
  case SQLR_SETCLIENTINFO: {
    if (objc != 3) {
      Tcl_WrongNumArgs(interp,2, objv, "clientinfo");
      return TCL_ERROR;
    }
    con->setClientInfo(Tcl_GetString(objv[2]));
    break;
  }
  case SQLR_GETCLIENTINFO: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,_Tcl_NewStringObj(con->getClientInfo(), -1));
    break;
  }
  case SQLR_SQLRCUR: {
    if (objc > 2) {
      Tcl_WrongNumArgs(interp, 2, objv, NULL);
      return TCL_ERROR;
    }
    if (sqlrcurCmd(data, interp, objc, objv) != TCL_OK) {
      return TCL_ERROR;
    }
  }
  }
  return TCL_OK;
}

/*
 * sqlrconCmd --
 *   the sqlrcon command itselfs.
 * Synopsis:
 *   set con [sqlrcon -server server -port port -user user -password password ?-retrytime time -tries count?]
 * OR
 *   set con [sqlrcon -socket socket -user user -password password ?-retrytime time -tries count?]
 */
int sqlrconCmd(ClientData dummy, Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[]) {
  static int count = 0;
  static CONSTCHAR *optionStrings[] = {
    "-server",
    "-port",
    "-socket",
    "-user",
    "-password",
    "-retrytime",
    "-tries",
    (char *)NULL
  };
  enum options {
    SQLRCON_SERVER,
    SQLRCON_PORT,
    SQLRCON_SOCKET,
    SQLRCON_USER,
    SQLRCON_PASSWORD,
    SQLRCON_RETRYTIME,
    SQLRCON_TRIES,
  };
  int i;
  CONSTCHAR *server, *socket, *user, *password;
  int port = 9000, retrytime = 0, tries = 1;
  sqlrconnection *con = (sqlrconnection *)NULL;
  Tcl_Obj *id;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
    return TCL_ERROR;
  }

  server = socket = user = password = "";
  for (i = 1; objc > i; i += 2) {
    int index;

    if (Tcl_GetIndexFromObj(interp, objv[i],
			    (CONSTCHAR **)optionStrings,
			    "option",
			    0,
			    (int *)&index) != TCL_OK) {
      return TCL_ERROR;
    }

    switch ((enum options)index) {
    case SQLRCON_SERVER: {
      server = Tcl_GetString(objv[i+1]);
      break;
    }
    case SQLRCON_PORT: {
      if (Tcl_GetIntFromObj(interp, objv[i+1], &port) != TCL_OK) {
	return TCL_ERROR;
      }
      break;
    }
    case SQLRCON_SOCKET:
      socket = Tcl_GetString(objv[i+1]);
      break;
    case SQLRCON_USER: {
      user = Tcl_GetString(objv[i+1]);
      break;
    }
    case SQLRCON_PASSWORD: {
      password = Tcl_GetString(objv[i+1]);
      break;
    }
    case SQLRCON_RETRYTIME: {
      if (Tcl_GetIntFromObj(interp, objv[i+1], &retrytime) != TCL_OK) {
	return TCL_ERROR;
      }
      break;
    }
    case SQLRCON_TRIES:
      if (Tcl_GetIntFromObj(interp, objv[i+1], &tries) != TCL_OK) {
	return TCL_ERROR;
      }
      break;
    }
  }

  if (charstring::compare("",server,1) == 0 && charstring::compare("", socket, 1) == 0) {
    Tcl_AppendResult(interp, 
		     "-server name or -socket name required", (char *)NULL);
    return TCL_ERROR;
  }

  con = new sqlrconnection(server, port, socket, user, password,
		                retrytime, tries,true);
  

  id = _Tcl_NewStringObj("sqlrcon", -1);
  Tcl_AppendStringsToObj(id, Tcl_GetString(Tcl_NewIntObj(count++)),
			(char *)NULL);

  Tcl_CreateObjCommand(interp, Tcl_GetString(id), sqlrconObjCmd,
		       (ClientData)con,
		       (Tcl_CmdDeleteProc *)sqlrconDelete);

  Tcl_SetObjResult(interp, id);
  return TCL_OK;
}

DLLEXPORT int Sqlrelay_Init(Tcl_Interp *interp) {
#ifdef USE_TCL_STUBS
  Tcl_InitStubs(interp, "8.2", 0);
#endif
  Tcl_CreateObjCommand(interp, "sqlrcon", sqlrconCmd,
		       (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  return Tcl_PkgProvide(interp, "sqlrelay", "1.0");
}

// tcl 9.0 derives the init function name from the load prefix as-is (lowercase
// "sqlrelay" -> sqlrelay_Init); tcl 8.x capitalized it (-> Sqlrelay_Init).
// provide both so "load ... sqlrelay" works on either tcl.
DLLEXPORT int sqlrelay_Init(Tcl_Interp *interp) {
  return Sqlrelay_Init(interp);
}

}

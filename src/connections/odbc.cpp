// Copyright (c) David Muse
// See the file COPYING for more information

// note that config.h must come first to avoid some macro redefinition warnings
#include <config.h>

// windows needs this and it doesn't appear to hurt on other platforms
#include <rudiments/private/winsock.h>

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <sqltypes.h>

// more gyrations to avoid macro redefinition warnings
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

// note that sqlrserver.h must be included after sqltypes.h to
// get around a problem with CHAR/xmlChar in gnome-xml
#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/ucs2character.h>
#include <rudiments/ucs2charstring.h>
#include <rudiments/utf8character.h>
#include <rudiments/utf8charstring.h>
#include <rudiments/utf16character.h>
#include <rudiments/utf16charstring.h>
#include <rudiments/error.h>
#include <rudiments/stdio.h>
#include <rudiments/process.h>
#include <rudiments/sys.h>

#include <datatypes.h>
#include <defines.h>

#ifdef HAVE_IODBC
	#include <iodbcinst.h>
#endif

#define MAX_LOB_CHUNK_SIZE	2147483647

struct odbccolumn {
	char		name[4096];
	uint16_t	namesize;
	char		dbtypename[64];
#if (ODBCVER >= 0x0300) && defined(SQLCOLATTRIBUTE_SQLLEN)
	SQLLEN		type;
	SQLLEN		concisetype;
	SQLLEN		size;
	SQLLEN		precision;
	SQLLEN		scale;
	SQLLEN		nullable;
	SQLLEN		unsignednumber;
	SQLLEN		autoincrement;
#else
	SQLINTEGER	type;
	SQLINTEGER	concisetype;
	SQLINTEGER	size;
	SQLINTEGER	precision;
	SQLINTEGER	scale;
	SQLINTEGER	nullable;
	SQLINTEGER	unsignednumber;
	SQLINTEGER	autoincrement;
#endif
	char		table[4096];
	uint16_t	tablesize;
};

struct datebind {
	int16_t			*year;
	int16_t			*month;
	int16_t			*day;
	int16_t			*hour;
	int16_t			*minute;
	int16_t			*second;
	int32_t			*microsecond;
	const char		**tz;
	SQL_TIMESTAMP_STRUCT	buffer;
};

struct charbind {
	charbind() : value(NULL), valuesize(0), ucsvalue(NULL) {}
	~charbind() { delete[] ucsvalue; }
	char		*value;
	uint32_t	valuesize;
	// scratch buffer used for unicode input-output string binds;
	// NULL unless inputOutputBind(...,char *,...) allocated one
	byte_t		*ucsvalue;
};

struct flagtoname {
	SQLUINTEGER	flag;
	const char	*name;
};

class odbcconnection;

class SQLRSERVER_DLLSPEC odbccursor : public sqlrservercursor {
	friend class odbcconnection;
	private:
		odbccursor(sqlrserverconnection *conn, uint16_t id);
		~odbccursor();
		void		allocateResultSetBuffers(int32_t columncount);
		void		deallocateResultSetBuffers();
		bool		prepareQuery(const char *query,
						uint32_t size);
		bool		allocateStatementHandle();
		void		initializeColCounts();
		void		initializeRowCounts();
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						int64_t *value);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						double *value, 
						uint32_t precision,
						uint32_t scale);
		bool		inputBind(const char *variable,
						uint16_t variablesize,
						int64_t year,
						int16_t month,
						int16_t day,
						int16_t hour,
						int16_t minute,
						int16_t second,
						int32_t microsecond,
						const char *tz,
						bool isnegative,
						int16_t *isnull);
		bool		inputBindBlob(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						int16_t *isnull);
		bool		outputBind(const char *variable, 
						uint16_t variablesize,
						char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						int64_t *value,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						double *value,
						uint32_t *precision,
						uint32_t *scale,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						int16_t *year,
						int16_t *month,
						int16_t *day,
						int16_t *hour,
						int16_t *minute,
						int16_t *second,
						int32_t *microsecond,
						const char **tz,
						bool *isnegative,
						int16_t *isnull);
		bool		inputOutputBind(const char *variable, 
						uint16_t variablesize,
						char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		bool		inputOutputBind(const char *variable, 
						uint16_t variablesize,
						int64_t *value,
						int16_t *isnull);
		bool		inputOutputBind(const char *variable,
						uint16_t variablesize,
						double *value,
						uint32_t *precision,
						uint32_t *scale,
						int16_t *isnull);
		bool		inputOutputBind(const char *variable,
						uint16_t variablesize,
						int16_t *year,
						int16_t *month,
						int16_t *day,
						int16_t *hour,
						int16_t *minute,
						int16_t *second,
						int32_t *microsecond,
						const char **tz,
						bool *isnegative,
						int16_t *isnull);
		int16_t		getNonNullBindValue();
		int16_t		getNullBindValue();
		bool		executeQuery(const char *query,
						uint32_t size);
		bool		handleColumns(bool getcolumninfo,
						bool bindcolumns);
		bool		appendNullColumns(uint8_t count);
		bool		appendNullColumn();
		bool		appendColumnListColumns();
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t i);
		uint16_t	getColumnNameSize(uint32_t i);
		uint16_t	getColumnType(uint32_t i);
		uint32_t	getColumnSize(uint32_t i);
		uint32_t	getColumnPrecision(uint32_t i);
		uint32_t	getColumnScale(uint32_t i);
		uint16_t	getColumnIsNullable(uint32_t i);
		uint16_t	getColumnIsUnsigned(uint32_t i);
		uint16_t	getColumnIsBinary(uint32_t i);
		uint16_t	getColumnIsAutoIncrement(uint32_t i);
		const char	*getColumnTable(uint32_t i);
		uint16_t	getColumnTableSize(uint32_t i);
		bool		noRowsToReturn();
		bool		cacheRowsAndDrainResultSets();
		bool		cacheCurrentRow(uint64_t *cachedbytes);
		void		fetchCachedRow();
		void		clearCachedRows();
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null);
		bool		getLobFieldLength(uint32_t col, uint64_t *size);
		bool		getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread);
		bool		nextResultSet(bool *nextresultsetavailable);
		void		closeResultSet();

		bool		columnInfoIsValidAfterPrepare();

#if (ODBCVER >= 0x0300) && defined(SQLCOLATTRIBUTE_SQLLEN)
		bool		isLob(SQLLEN type);
#else
		bool		isLob(SQLINTEGER type);
#endif

		void		setConvCharError(const char *baseerror,
						const char *detailerror);


		SQLRETURN	erg;
		SQLHSTMT	stmt;
		SQLSMALLINT	ncols;
		#ifdef SQLROWCOUNT_SQLLEN
		SQLLEN 		affectedrows;
		#else
		SQLINTEGER 	affectedrows;
		#endif

		int32_t		columncount;
		char		**field;
		#ifdef SQLBINDCOL_SQLLEN
		SQLLEN		*loblength;
		SQLLEN		*indicator;
		#else
		SQLINTEGER	*loblength;
		SQLINTEGER	*indicator;
		#endif
		odbccolumn 	*column;

		uint16_t		maxbindcount;
		SQL_DATE_STRUCT		*indatebind;
		SQL_TIME_STRUCT		*intimebind;
		SQL_TIMESTAMP_STRUCT	*intsbind;
		datebind		**outdatebind;
		charbind		**outcharbind;
		int16_t			**outisnullptr;
		datebind		**inoutdatebind;
		charbind		**inoutcharbind;
		int16_t			**inoutisnullptr;
		#ifdef SQLBINDPARAMETER_SQLLEN
		SQLLEN			*outisnull;
		SQLLEN			*inoutisnull;
		SQLLEN			*inbindlength;
		SQLLEN			sqlnulldata;
		#else
		SQLINTEGER		*outisnull;
		SQLINTEGER		*inoutisnull;
		SQLINTEGER		*inbindlength;
		SQLINTEGER		sqlnulldata;
		#endif

		// whether the driver said this parameter is aimed at a
		// binary column, cached so a bulk load asks once per
		// position per prepare rather than once per row
		bool			*nullbindisbinary;
		bool			*nullbinddescribed;
		bool	nullBindIsBinary(uint16_t pos);

		bool		bindformaterror;

		uint32_t	row;
		uint32_t	maxrow;
		uint32_t	totalrows;

		singlylinkedlist< unsigned char * >	cachedrows;
		listnode< unsigned char * >		*currentcachedrow;
		bool					cachedrowsarecomplete;

		bool		resultsetsdrained;

		stringbuffer	errormsg;

		#ifdef HAVE_SQLCONNECTW
		singlylinkedlist<byte_t *>	ucsinbindstrings;
		#endif

		bool		columninfoisvalidafterprepare;

		odbcconnection	*odbcconn;

		char	columnnamescratch[4096];
};

class SQLRSERVER_DLLSPEC odbcconnection : public sqlrserverconnection {
	friend class odbccursor;
	public:
		odbcconnection(sqlrservercontroller *cont);
		~odbcconnection();
	private:
		void		handleConnectString();
		bool		logIn(const char **error, const char **warning);
		char		*odbcDriverConnectionString(
						const char *userasc,
						const char *passwordasc);
		void		pushConnstrValue(char **pptr,
						size_t *pbuffavail,
						const char *keyword,
						const char *value);
		char		*traceFileName(const char *tracefilenameformat);
		const char	*logInError(const char *errmsg);
		sqlrservercursor	*newCursor(uint16_t id);
		void		deleteCursor(sqlrservercursor *curs);
		void		logOut();
		#if (ODBCVER>=0x0300)
		bool		setAutoCommitOn();
		bool		setAutoCommitOff();
		bool		supportsAutoCommit();
		bool		getDefaultAutoCommit();
		const char	*beginTransactionQuery();
		bool		commit();
		bool		rollback();
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
		#endif
		bool		isLiveConnection(SQLCHAR *state);
		bool		ping();
		const char	*getDbType();
		const char	*getDbVersion();
		const char * const	*getDatabaseFeatures();
		const char	*getBindFormat();
		const char	*getNextvalFormat();
		const char	*getLastInsertIdQuery();
		bool		getListsByApiCalls();
		bool		getCatalogList(sqlrservercursor *cursor,
						const char *catalog);
		bool		getSchemaList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema);
		bool		getTableTypeList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *tabletypes);
		bool		getTableList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes);
		bool		getTypeInfoList(sqlrservercursor *cursor,
						const char *db,
						const char *schema,
						const char *type);
		bool		getColumnList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column);
		bool		getPrimaryKeysList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table);
		bool		getKeyAndIndexList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table);
		bool		getProcedureList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *procedure);
		bool		getProcedureParameterList(
						sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*selectCatalogQuery();
		char		*getCurrentCatalog();
		char		*getCurrentSchema();
		char		*getCurrentUser();
		const char	*mapIsolationLevel(
						const char *isolevel,
						sqlrserverisolationlevelformat_t
								fromformat,
						sqlrserverisolationlevelformat_t
								toformat);
		bool		setIsolationLevel(const char *isolevel);
		const char	*getDbHostNameQuery();
		const char	*getDbIpAddressQuery();
		void		flagsToNames(stringbuffer *sb,
						SQLUINTEGER flags,
						flagtoname *ftn);
		void		appendToList(stringbuffer *sb,
						const char *value,
						bool condition);


		SQLRETURN	erg;
		SQLHENV		env;
		SQLHDBC		dbc;

		const char	*driver;
		const char	*driverconnect;
		const char	*dsn;
		const char	*server;
		const char	*db;
		const char	*trace;
		const char	*tracefile;
		const char	*odbcversion;
		const char	*lastinsertidquery;
		bool		mars;
		bool		getcolumntables;
		const char	*overrideschema;
		bool		unicode;
		const char	*ncharencoding;

		stringbuffer	errormessage;

		char		dbversion[512];
		char		**databasefeatures;

		const char	*begintxquery;
		bool		usecharforlobbind;
		bool		describenullbinds;
		bool		hasdescribeparam;
		SQLSMALLINT	fractionscale;
		bool		supportsfraction;
		bool		timestampfortime;
		uint32_t	maxallowedvarcharbindsize;
		uint32_t	maxvarcharbindsize;
		SQLINTEGER	*columninfonotvalidyeterror;
		bool		sqltypedatetosqlcbinary;
		bool		fetchlobsasstrings;

		#if (ODBCVER>=0x0300)
		stringbuffer	errormsg;
		#endif
};

#ifdef HAVE_SQLCONNECTW
#include <rudiments/iconvert.h>

size_t isUcs2(const char *encoding) {
	return (charstring::contains(encoding,"UCS2") ||
			charstring::contains(encoding,"UCS-2"));
}

size_t isUtf16(const char *encoding) {
	return (charstring::contains(encoding,"UTF16") ||
			charstring::contains(encoding,"UTF-16"));
}

size_t isUtf8(const char *encoding) {
	return (charstring::contains(encoding,"UTF8") ||
			charstring::contains(encoding,"UTF-8"));
}

// returns number of characters in the string, not counting any byte-order
// mark or the null terminator
size_t len(const byte_t *str, const char *encoding) {

	const byte_t	*ptr=str;
	size_t		res=0;

	if (isUcs2(encoding)) {

		// skip any byte-order mark
		if (ucs2charstring::isByteOrderMark((const ucs2_t *)str)) {
			ptr+=ucs2character::getBomSize();
		}

		res=ucs2charstring::getLength((ucs2_t *)ptr);

	} else if (isUtf16(encoding)) {

		// skip any byte-order mark
		bool bigendian=false;
		if (utf16charstring::isByteOrderMark(
						(const utf16_t *)str)) {
			bigendian=utf16charstring::isBigEndian(
						(const utf16_t *)str);
			ptr+=utf16character::getBomSize();
		}

		res=utf16charstring::getLength((utf16_t *)ptr,bigendian);

	} else if (isUtf8(encoding)) {

		// skip any byte-order mark
		if (utf8charstring::isByteOrderMark((const utf8_t *)str)) {
			ptr+=utf8character::getBomSize();
		}

		res=utf8charstring::getLength((utf8_t *)ptr);
		
	} else {
		res=charstring::getLength((const char *)str);
	}
	return res;
}

// returns number of bytes in the string, including any byte-order mark
// and the null terminator
size_t stringSize(const byte_t *str, const char *encoding) {

	const byte_t	*ptr=str;
	size_t		res=0;

	if (isUcs2(encoding)) {

		// skip any byte-order mark
		if (ucs2charstring::isByteOrderMark((const ucs2_t *)str)) {
			res+=ucs2character::getBomSize();
			ptr+=ucs2character::getBomSize();
		}

		res+=ucs2charstring::getSize((ucs2_t *)ptr);

	} else if (isUtf16(encoding)) {

		// skip any byte-order mark
		bool bigendian=false;
		if (utf16charstring::isByteOrderMark(
						(const utf16_t *)str)) {
			bigendian=utf16charstring::isBigEndian(
						(const utf16_t *)str);
			res+=utf16character::getBomSize();
			ptr+=utf16character::getBomSize();
		}

		res+=utf16charstring::getSize((utf16_t *)ptr,bigendian);

	} else if (isUtf8(encoding)) {

		// skip any byte-order mark
		if (utf8charstring::isByteOrderMark((const utf8_t *)str)) {
			res+=utf8character::getBomSize();
			ptr+=utf8character::getBomSize();
		}

		res+=utf8charstring::getSize((utf8_t *)ptr);
		
	} else {
		res=charstring::getSize((const char *)str);
	}
	return res;
}

size_t nullSize(const char *encoding) {
	if (isUcs2(encoding)) {
		return ucs2character::getNullSize();
	} else if (isUtf16(encoding)) {
		return 2;
	} else if (isUtf8(encoding)) {
		return 1;
	} else {
		return character::getNullSize();
	}
}

byte_t *convertCharset(const byte_t *inbuf,
				size_t insize,
				const char *inenc,
				const char *outenc,
				char **error) {

	// initialize error
	if (error) {
		*error=NULL;
	}

	// get size of null terminator
	size_t	nullsize=nullSize(outenc);

	// calculate size of output buffer (in bytes)
	// (3 is max size of byte order mark)
	size_t	multiplier=4;
	if (isUcs2(inenc) && isUcs2(outenc)) {
		multiplier=1;
	}
	size_t	outsize=len(inbuf,inenc)*multiplier+3+nullsize;

	// allocate the output buffer
	byte_t	*outbuf=new byte_t[outsize];

	// open converter
	// FIXME: reuse this rather than re-creating it over and over
	iconvert	ic;
	ic.setFromEncoding(inenc);
	ic.setFromBuffer(inbuf);
	ic.setFromBufferSize(insize);
	ic.setToEncoding(outenc);
	ic.setToBuffer(outbuf);
	ic.setToBufferSize(outsize);

	// convert
	if (!ic.convert()) {
		if (error) {
			char	*err=error::getErrorString();
			charstring::printf(error,
				"iconvert::convert(): %s "
				"(in=%s/%ld/%ld out=%s/%ld/%ld)",
				err,
				inenc,insize,ic.getFromBufferPosition()-inbuf,
				outenc,outsize,ic.getToBufferPosition()-outbuf);
			delete[] err;
		}
		// null-terminate the output
		bytestring::zero(outbuf,nullsize);
		return outbuf;
	}
	byte_t	*outbufend=(byte_t *)ic.getToBufferPosition();

	// SQL Server doesn't like UTF-16 values to have a byte-order mark
	// (and it wants them to be big-endian)
	// FIXME: make this configurable somehow...
	if (isUtf16(outenc) &&
		((outbuf[0]==0xFF && outbuf[1]==0xFE) ||
		(outbuf[0]==0xFE && outbuf[1]==0xFF))) {
		bytestring::copyWithOverlap(outbuf,outbuf+2,outbufend-outbuf-2);
		outbufend-=2;
	}

	// null-terminate the output
	bytestring::zero(outbufend,nullsize);

	return outbuf;
}

byte_t *convertCharset(const byte_t *inbuf,
				const char *inenc,
				const char *outenc,
				char **error) {
	return convertCharset(inbuf,stringSize(inbuf,inenc),inenc,outenc,error);
}
#endif

odbcconnection::odbcconnection(sqlrservercontroller *cont) :
					sqlrserverconnection(cont) {
	driver=NULL;
	driverconnect=NULL;
	dsn=NULL;
	server=NULL;
	db=NULL;
	trace=NULL;
	tracefile=NULL;
	odbcversion=NULL;
	lastinsertidquery=NULL;
	mars=false;
	getcolumntables=false;
	overrideschema=NULL;
	unicode=true;
	ncharencoding=NULL;
	columninfonotvalidyeterror=NULL;
	databasefeatures=NULL;
}

odbcconnection::~odbcconnection() {
	delete[] columninfonotvalidyeterror;
	if (databasefeatures) {
		for (int i=0; i<FEATURE_COUNT; i++) {
			delete[] databasefeatures[i];
		}
		delete[] databasefeatures;
	}
}

void odbcconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	driver=cont->getConnectStringValue("driver");
	driverconnect=cont->getConnectStringValue("driverconnect");
	dsn=cont->getConnectStringValue("dsn");
	server=cont->getConnectStringValue("server");
	db=cont->getConnectStringValue("db");

	trace=cont->getConnectStringValue("trace");
	tracefile=cont->getConnectStringValue("tracefile");

	odbcversion=cont->getConnectStringValue("odbcversion");

	lastinsertidquery=cont->getConnectStringValue("lastinsertidquery");

	mars=charstring::isYes(cont->getConnectStringValue("mars"));
	getcolumntables=charstring::isYes(
			cont->getConnectStringValue("getcolumntables"));
	const char	*os=cont->getConnectStringValue("overrideschema");
	if (!charstring::isNullOrEmpty(os)) {
		overrideschema=os;
	}

	unicode=!charstring::isNo(cont->getConnectStringValue("unicode"));
	ncharencoding=cont->getConnectStringValue("ncharencoding");
	if (charstring::isNullOrEmpty(ncharencoding) ||
		(charstring::compare(ncharencoding,"UCS2",4) &&
		charstring::compare(ncharencoding,"UCS-2",5) &&
		charstring::compare(ncharencoding,"UTF16",5) &&
		charstring::compare(ncharencoding,"UTF-16",6))) {
		ncharencoding="UCS-2//TRANSLIT";
	}

	// unixodbc doesn't support array fetches
	cont->setFetchAtOnce(1);
}

bool odbcconnection::logIn(const char **error, const char **warning) {

	// allocate environment handle
#if (ODBCVER >= 0x0300)
	erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		*error="Failed to allocate environment handle";
		SQLFreeHandle(SQL_HANDLE_ENV,env);
		return false;
	}

	if (!charstring::compare(odbcversion,"2")) {
		erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
					(void *)SQL_OV_ODBC2,0);
	#ifdef SQL_OV_ODBC3_80
	} else if (!charstring::compare(odbcversion,"3.8")) {
		erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
					(void *)SQL_OV_ODBC3_80,0);
	#endif
	} else {
		erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
					(void *)SQL_OV_ODBC3,0);
	}
#else
	erg=SQLAllocEnv(&env);
#endif
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		*error="Failed to allocate environment handle";
		return false;
	}

	// allocate connection handle
	#if (ODBCVER >= 0x0300)
	erg=SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc);
	#else
	erg=SQLAllocConnect(env,&dbc);
	#endif
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		*error="Failed to allocate connection handle";
		#if (ODBCVER >= 0x0300)
		SQLFreeHandle(SQL_HANDLE_ENV,env);
		#else
		SQLFreeEnv(env);
		#endif
		return false;
	}

#if (ODBCVER >= 0x0300)
	// trace paramters may have been set in the DSN,
	// but we can also override them here...
	if (!charstring::isNullOrEmpty(tracefile)) {
		// FIXME: does this need to persist?
		char	*tracefilename=traceFileName(tracefile);
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_TRACEFILE,
				(SQLPOINTER *)tracefilename,
				SQL_NTS);
		delete[] tracefilename;
	}
	if (charstring::isYes(trace)) {
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_TRACE,
				(SQLPOINTER *)SQL_OPT_TRACE_ON,
				0);
	} else if (charstring::isNo(trace)) {
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_TRACE,
				(SQLPOINTER *)SQL_OPT_TRACE_OFF,
				0);
	}

	// set the initial db
	if (!charstring::isNullOrEmpty(db)) {
		erg = SQLSetConnectAttr(dbc,SQL_ATTR_CURRENT_CATALOG,
					(SQLPOINTER *)db,SQL_NTS);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			*error="Failed to set database";
			SQLFreeHandle(SQL_HANDLE_DBC,dbc);
			SQLFreeHandle(SQL_HANDLE_ENV,env);
			return false;
		}
	}

	// set the connect timeout
	uint64_t	connecttimeout=cont->getConnectTimeout();
	if (connecttimeout) {
		erg=SQLSetConnectAttr(dbc,SQL_LOGIN_TIMEOUT,
					(SQLPOINTER *)connecttimeout,0);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			*error="Failed to set connect timeout";
			SQLFreeHandle(SQL_HANDLE_DBC,dbc);
			SQLFreeHandle(SQL_HANDLE_ENV,env);
			return false;
		}
	}
#endif

	// enable SQL Server MARS, if configured to do so
	if (mars) {
		SQLSetConnectAttr(dbc,1224,(SQLPOINTER *)1,SQL_IS_UINTEGER);
	}

	// connect to the database
	const char	*userasc=cont->getLoginUser();
	const char	*passwordasc=cont->getLoginPassword();

	if (!charstring::isNullOrEmpty(driver)) {

		char	*sqlconnectdriverstring=
			odbcDriverConnectionString(userasc,passwordasc);

		// These values are useful to look at from the debugger,
		// it is not a good security practice to directly log
		// the string, because it might contain a plaintext password.
		SQLCHAR		outconnectionstring[2048];
		SQLSMALLINT	outconnectionstringlen;
		erg=SQLDriverConnect(dbc,
				(SQLHWND)NULL,
				(SQLCHAR *)sqlconnectdriverstring,
				(SQLSMALLINT)charstring::getLength(
						sqlconnectdriverstring),
				outconnectionstring,
				(SQLSMALLINT)sizeof(outconnectionstring),
				&outconnectionstringlen,
				(SQLSMALLINT)SQL_DRIVER_NOPROMPT);

		delete[] sqlconnectdriverstring;

	} else {

		const char	*dsnasc=dsn;

		#ifdef HAVE_SQLCONNECTW
		if (unicode) {
			byte_t	*dsnucs=(dsnasc)?
					convertCharset((const byte_t *)
							dsnasc,
							"UTF-8",
							"UCS-2//TRANSLIT",
							NULL):NULL;
			byte_t	*userucs=(userasc)?
					convertCharset((const byte_t *)
							userasc,
							"UTF-8",
							"UCS-2//TRANSLIT",
							NULL):NULL;
			byte_t	*passworducs=(passwordasc)?
					convertCharset((const byte_t *)
							passwordasc,
							"UTF-8",
							"UCS-2//TRANSLIT",
							NULL):NULL;
			erg=SQLConnectW(dbc,(SQLWCHAR *)dsnucs,SQL_NTS,
					(SQLWCHAR *)userucs,SQL_NTS,
					(SQLWCHAR *)passworducs,SQL_NTS);
			delete[] dsnucs;
			delete[] userucs;
			delete[] passworducs;
		} else {
		#endif
			erg=SQLConnect(dbc,(SQLCHAR *)dsnasc,SQL_NTS,
					(SQLCHAR *)userasc,SQL_NTS,
					(SQLCHAR *)passwordasc,SQL_NTS);
		#ifdef HAVE_SQLCONNECTW
		}
		#endif
	}
	
	if (erg==SQL_SUCCESS_WITH_INFO) {
		*warning=logInError(NULL);
	} else if (erg!=SQL_SUCCESS) {
		*error=logInError("SQLConnect failed");
		#if (ODBCVER >= 0x0300)
		SQLFreeHandle(SQL_HANDLE_DBC,dbc);
		SQLFreeHandle(SQL_HANDLE_ENV,env);
		#else
		SQLFreeConnect(dbc);
		SQLFreeEnv(env);
		#endif
		return false;
	}

	// get the type of database
	char		dbmsnamebuffer[1024];
	dbmsnamebuffer[0]='\0';
	SQLSMALLINT	dbmsnamelen=0;
	if (SQLGetInfo(dbc,
			SQL_DBMS_NAME,
			dbmsnamebuffer,
			sizeof(dbmsnamebuffer),
			&dbmsnamelen)==SQL_SUCCESS) {
		dbmsnamebuffer[dbmsnamelen]='\0';
	}

	// set some default params
	begintxquery=sqlrserverconnection::beginTransactionQuery();
	usecharforlobbind=true;
	describenullbinds=false;

	// Whether the driver implements SQLDescribeParam at all.  SQL Relay's
	// own odbc driver doesn't, and answers this honestly, so an
	// odbc-module-to-sqlrelay chain has to fall back rather than fail.
	SQLUSMALLINT	describeparamsupported=0;
	hasdescribeparam=(SQLGetFunctions(dbc,SQL_API_SQLDESCRIBEPARAM,
					&describeparamsupported)==SQL_SUCCESS &&
					describeparamsupported);
	// When binding dates using SQLBindParameter, the "decimal
	// digits" parameter refers to the number of digits in the
	// "fraction" part of the date.  Since that is in nanoseconds
	// (billionths of a second (0-999999999)) in ODBC, the
	// "decimal digits" parameter must be 9 to accomodate the
	// full range.
	fractionscale=9;
	supportsfraction=true;
	timestampfortime=true;
	maxallowedvarcharbindsize=0;
	maxvarcharbindsize=0;
	columninfonotvalidyeterror=NULL;
	sqltypedatetosqlcbinary=true;
	fetchlobsasstrings=false;

	// override some default params based on the db-type
	if (!charstring::compare(dbmsnamebuffer,"Teradata")) {
		begintxquery="BT";
		usecharforlobbind=false;
		// See below...  Teradata only supports 6 digits though.
		fractionscale=6;
		// Well... Teradata theoretically supports 6 digits of
		// fractional seconds, but any attempt to actually bind
		// fractional seconds results in "[Teradata][Support] (40520)
		// Datetime field overflow resulting from invalid datetime."
		supportsfraction=false;
		// Teradata doesn't like it if you bind a SQL_TIMESTAMP_STRUCT
		// to a TIME datatype.
		timestampfortime=false;
	} else if (!charstring::compare(dbmsnamebuffer,
						"Microsoft SQL Server",20)) {
		// SQL Server defines a varchar/nvarchar as 4000 characters
		// long, but you can actually store up to 2Gb in them.
		// However, if you send a valuesize > 4000 characters during
		// a bind, then something in the chain doesn't like it.  To
		// work around this, you have to send 0.  Go figure...
		maxallowedvarcharbindsize=4000;
		maxvarcharbindsize=0;

		// With MS SQL Server, there are various cases where column
		// metadata can't be fetched until post-execute.  For example:
		//
		// sqlrsh commands like:
		//	inputbind 1 = 'hello';
		//	select ?;
		// fail with:
		//	11521:
		//	[Microsoft][ODBC Driver 17 for SQL Server][SQL Server]
		//	The metadata could not be determined because statement
		//	'select @P1' uses an undeclared parameter in a context
		//	that affects its metadata.
		// Sored procedures that optionally execute selects which
		// return different numbers of columns fail with:
		// 	11512:
		// 	[Microsoft][ODBC Driver 17 for SQL Server][SQL Server]
		// 	The metadata could not be determined because the
		// 	statement '...some select query...' is not compatible
		// 	with the statement '...some other select query...' in
		// 	procedure '...some procedure...'.
		// So, in cases like this we can catch the error and defer
		// getting/sending column info until later.
		// Basically, it's error codes 11509-11530 but there could be
		// others too...
		columninfonotvalidyeterror=new SQLINTEGER[23];
		columninfonotvalidyeterror[0]=11509;
		columninfonotvalidyeterror[1]=11510;
		columninfonotvalidyeterror[2]=11511;
		columninfonotvalidyeterror[3]=11512;
		columninfonotvalidyeterror[4]=11513;
		columninfonotvalidyeterror[5]=11514;
		columninfonotvalidyeterror[6]=11515;
		columninfonotvalidyeterror[7]=11516;
		columninfonotvalidyeterror[8]=11517;
		columninfonotvalidyeterror[9]=11518;
		columninfonotvalidyeterror[10]=11519;
		columninfonotvalidyeterror[11]=11520;
		columninfonotvalidyeterror[12]=11521;
		columninfonotvalidyeterror[13]=11522;
		columninfonotvalidyeterror[14]=11523;
		columninfonotvalidyeterror[15]=11524;
		columninfonotvalidyeterror[16]=11525;
		columninfonotvalidyeterror[17]=11526;
		columninfonotvalidyeterror[18]=11527;
		columninfonotvalidyeterror[19]=11528;
		columninfonotvalidyeterror[20]=11529;
		columninfonotvalidyeterror[21]=11530;
		columninfonotvalidyeterror[22]=0;

		// SQL Server doesn't like for you to convert SQL_TYPE_DATE
		// to SQL_C_BINARY
		sqltypedatetosqlcbinary=false;

		// SQL Server has trouble mixing SQLBindCol and SQLGetData.
		// If you SQLBindCol a column (eg. column 4) then you can't use
		// SQLGetData to fetch an earlier column (eg. column 3).
		// A workaround is to use SQLBindCol in all cases and fetch
		// LOBs as strings.
		fetchlobsasstrings=true;

		// SQL Server likes "BEGIN TRANSACTION" to begin transactions.
		begintxquery="BEGIN TRANSACTION";

		// SQL Server won't implicitly convert a character parameter
		// to binary, varbinary or image, so the character bind can't
		// reach those columns at all.  It also charset-converts the
		// value, which raw bytes don't survive.  Bind blobs as
		// binary instead.  (The trade is that a blob bind can no
		// longer reach a text or ntext column.)
		usecharforlobbind=false;

		// The same conversion rule applies to a null bind, which
		// can't be a blob bind because the client api flattens a
		// null lob into a plain null.  Ask the driver what the
		// parameter is aimed at, for nulls only.  (See the null arm
		// of odbccursor::inputBind().)
		describenullbinds=true;
	}

	return true;
}

char *odbcconnection::traceFileName(const char *tracefilenameformat) {

	// This would be a good candidate for promotion to rudiments,
	// These format operators are enough to provide a unique log file
	// name, per-process:
	// %p means PID
	// %t means a timestamp.
	// %h means the hostname.
	// If any of these appears more than once then the output filename
	// may be truncated.

	pid_t	pid=process::getProcessId();

	datetime dt;
	dt.initFromSystemDateTime();
	time_t	now=dt.getEpoch();

	char	*hostname=sys::getHostName();

	size_t	tracefilenamebuffersize=charstring::getLength(
							tracefilenameformat);
	tracefilenamebuffersize+=charstring::getIntegerLength((int64_t)pid);
	tracefilenamebuffersize+=charstring::getIntegerLength((int64_t)now);
	tracefilenamebuffersize+=charstring::getLength(hostname);
	tracefilenamebuffersize+=1;

	char		*tracefilename=new char[tracefilenamebuffersize];
	char		*outptr=tracefilename;
	size_t		outptrsize=tracefilenamebuffersize-1;
	const char	*ptr=tracefilenameformat;
	*outptr=0;
	while (*ptr && (outptrsize>0)) {
		if (*ptr=='%') {
			char	*insertstring=NULL;
			int64_t	insertnumber=0;
			ptr++;
			if (*ptr=='p') {
				insertnumber=pid;
			} else if (*ptr=='t') {
				insertnumber=now;
			} else if (*ptr=='h') {
				insertstring=hostname;
			}
			if (insertstring!=NULL) {
				charstring::printf(
					outptr,outptrsize,"%s",insertstring);
			} else {
				charstring::printf(
					outptr,outptrsize,"%ld",insertnumber);
			}
			ptr++;
			size_t	outptrinc=charstring::getLength(outptr);
			outptrsize-=outptrinc;
			outptr+=outptrinc;
		} else {
			*outptr++=*ptr++;
			outptrsize--;
			*outptr=0;
		}
	}
	delete[] hostname;
	return tracefilename;
}

char *odbcconnection::odbcDriverConnectionString(const char *userasc,
						const char *passwordasc) {

	// FIXME: use a stringbuffer
	size_t	buffsize=1024;
	size_t	buffavail=buffsize;
	char	*buff=new char[buffsize];
	char	*ptr=buff;

	// At least with unixODBC, we find that if the DSN is not the first
	// field, there will be an SQLDriverConnect error of:
	//
	//	state 08001
	//	errnum 0
	//	message [unixODBC][Microsoft][ODBC Driver 11 for SQL Server]Neither DSN nor SERVER keyword supplied
	//
	// If DSN is specified then the DRIVER seems to be ignored. This makes
	// sense actually.

	if (!charstring::isNullOrEmpty(dsn)) {
		pushConnstrValue(&ptr,&buffavail,"DSN",dsn);
	}
	if (!charstring::isNullOrEmpty(driver)) {
		pushConnstrValue(&ptr,&buffavail,"DRIVER",driver);
	}
	if (!charstring::isNullOrEmpty(driverconnect)) {
		// we push this extra info right after the DSN or DRIVER
		// so that we can clearly see it in the unixODBC trace which
		// tends to truncate at about 130 characters.
		byte_t	*rawdriverconnect=
				charstring::base64Decode(driverconnect);
		pushConnstrValue(&ptr,&buffavail,NULL,
				(const char *)rawdriverconnect);
		delete[] rawdriverconnect;
	}
	if (!(charstring::isNullOrEmpty(server) ||
			charstring::contains(buff,";SERVER="))) {
		pushConnstrValue(&ptr,&buffavail,"SERVER",server);
	}
	if (!(charstring::isNullOrEmpty(userasc) ||
			charstring::contains(buff,";UID="))) {
		pushConnstrValue(&ptr,&buffavail,"UID",userasc);
	}
	if (!(charstring::isNullOrEmpty(passwordasc) ||
			charstring::contains(buff,";PWD="))) {
		pushConnstrValue(&ptr,&buffavail,"PWD",passwordasc);
	}
	if (!charstring::contains(buff, ";WSID=")) {
		pushConnstrValue(&ptr,&buffavail,"WSID",sys::getHostName());
	}
	if (!charstring::contains(buff, ";APP=")) {
		// FIXME: use one of the SQLR macros here...
		pushConnstrValue(&ptr,&buffavail,
					"APP","SQLRelay-" SQLR_VERSION);
	}
	return buff;
}

void odbcconnection::pushConnstrValue(char **pptr, size_t *pbuffavail,
				const char *keyword, const char *value) {

	const char	*openbracket="";
	const char	*closebracket="";
	char		*ptr=*pptr;
	size_t		buffavail=*pbuffavail;
	if (charstring::contains(value,';')) {
		openbracket="{";
		closebracket="}";
	}
	if (keyword == NULL) {
		// here we are just going to push a raw value.
		// With an extra semicolon just in case.
		charstring::printf(ptr,buffavail,"%s;",value);
	} else {
		charstring::printf(ptr,buffavail,"%s=%s%s%s;",
				keyword,openbracket,value,closebracket);
	}
	size_t	ptrinc=charstring::getLength(ptr);
	ptr+=ptrinc;
	buffavail-=ptrinc;
	*pptr=ptr;
	*pbuffavail=buffavail;
}

const char *odbcconnection::logInError(const char *errmsg) {

	errormessage.clear();
	if (errmsg) {
		errormessage.append(errmsg)->append(": ");
	}

	// get the error message
	SQLCHAR		state[SQL_SQLSTATE_SIZE+1];
	SQLINTEGER	nativeerrnum;
	SQLCHAR		errorbuffer[1024];
	SQLSMALLINT	errsize;

	bytestring::zero(state,sizeof(state));
	bytestring::zero(errorbuffer,sizeof(errorbuffer));

	SQLGetDiagRec(SQL_HANDLE_DBC,dbc,1,state,&nativeerrnum,
				errorbuffer,sizeof(errorbuffer),&errsize);

	errormessage.append(errorbuffer,errsize);
	return errormessage.getString();
}

sqlrservercursor *odbcconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new odbccursor((sqlrserverconnection *)this,id);
}

void odbcconnection::deleteCursor(sqlrservercursor *curs) {
	delete (odbccursor *)curs;
}

void odbcconnection::logOut() {
	SQLDisconnect(dbc);
	#if (ODBCVER >= 0x0300)
	SQLFreeHandle(SQL_HANDLE_DBC,dbc);
	SQLFreeHandle(SQL_HANDLE_ENV,env);
	#else
	SQLFreeConnect(dbc);
	SQLFreeEnv(env);
	#endif
	dbc=NULL;
	env=NULL;
}

bool odbcconnection::ping() {
	return true;
}

const char *odbcconnection::getDbType() {
	return "odbc";
}

const char *odbcconnection::getDbVersion() {
	SQLSMALLINT	dbversionlen;
	SQLGetInfo(dbc,SQL_DBMS_VER,
			(SQLPOINTER)dbversion,
			(SQLSMALLINT)sizeof(dbversion),
			&dbversionlen);
	return dbversion;
}

void odbcconnection::flagsToNames(stringbuffer *sb,
					SQLUINTEGER flags,
					flagtoname *ftn) {
	bool	first=true;
	for (int i=0; ftn[i].name; i++) {
		if (flags&ftn[i].flag) {
			if (!first) {
				sb->append(',');
			}
			sb->append(ftn[i].name);
			first=false;
		}
	}
}

void odbcconnection::appendToList(stringbuffer *sb,
					const char *value,
					bool condition) {
	if (condition) {
		if (sb->getSize()>0) {
			sb->append(',');
		}
		sb->append(value);
	}
}

const char * const *odbcconnection::getDatabaseFeatures() {

	if (databasefeatures) {
		return databasefeatures;
	}

	databasefeatures=new char *[FEATURE_COUNT];

	char		strbuf[1024];
	SQLUINTEGER	uintbuf=0;
	SQLUSMALLINT	usmallintbuf=0;
	SQLSMALLINT	size=0;
	stringbuffer	sb;

	// SQL_AGGREGATE_FUNCTIONS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_AGGREGATE_FUNCTIONS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	af[]={
		{SQL_AF_ALL,"ALL"},
		{SQL_AF_AVG,"AVG"},
		{SQL_AF_COUNT,"COUNT"},
		{SQL_AF_DISTINCT,"DISTINCT"},
		{SQL_AF_MAX,"MAX"},
		{SQL_AF_MIN,"MIN"},
		{SQL_AF_SUM,"SUM"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,af);
	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=sb.detachString();

	// SQL_ACCESSIBLE_PROCEDURES -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_ACCESSIBLE_PROCEDURES,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_ACCESSIBLE_TABLES -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_ACCESSIBLE_TABLES,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_ALTER_DOMAIN -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_ALTER_DOMAIN,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ad[]={
		{SQL_AD_ADD_DOMAIN_CONSTRAINT,
				"ADD_DOMAIN_CONSTRAINT"},
		{SQL_AD_ADD_DOMAIN_DEFAULT,
				"ADD_DOMAIN_DEFAULT"},
		{SQL_AD_CONSTRAINT_NAME_DEFINITION,
				"CONSTRAINT_NAME_DEFINITION"},
		{SQL_AD_DROP_DOMAIN_CONSTRAINT,
				"DROP_DOMAIN_CONSTRAINT"},
		{SQL_AD_DROP_DOMAIN_DEFAULT,
				"DROP_DOMAIN_DEFAULT"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ad);
	databasefeatures[FEATURE_ALTER_DOMAIN_CLAUSES]=sb.detachString();

	// SQL_ALTER_TABLE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_ALTER_TABLE,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	at[]={
		{SQL_AT_ADD_COLUMN,"ADD_COLUMN"},
		{SQL_AT_DROP_COLUMN,"DROP_COLUMN"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,at);
	databasefeatures[FEATURE_ALTER_TABLE_OPERATIONS]=sb.detachString();

	// SQL_SQL_CONFORMANCE -> enum
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL_CONFORMANCE,&uintbuf,sizeof(uintbuf),&size);
	appendToList(&sb,"ENTRY_LEVEL",uintbuf>=SQL_SC_SQL92_ENTRY);
	appendToList(&sb,"FULL",uintbuf==SQL_SC_SQL92_FULL);
	appendToList(&sb,"INTERMEDIATE",uintbuf>=SQL_SC_SQL92_INTERMEDIATE);
	databasefeatures[FEATURE_ANSI92_SQL_LEVELS]=sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
						charstring::duplicate("");

	// SQL_BATCH_SUPPORT -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_BATCH_SUPPORT,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	bs[]={
		{SQL_BS_SELECT_EXPLICIT,"SELECT_EXPLICIT"},
		{SQL_BS_ROW_COUNT_EXPLICIT,"ROW_COUNT_EXPLICIT"},
		{SQL_BS_SELECT_PROC,"SELECT_PROC"},
		{SQL_BS_ROW_COUNT_PROC,"ROW_COUNT_PROC"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,bs);
	databasefeatures[FEATURE_BATCH_OPERATIONS]=sb.detachString();

	// SQL_BATCH_ROW_COUNT -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_BATCH_ROW_COUNT,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	brc[]={
		{SQL_BRC_PROCEDURES,"PROCEDURES"},
		{SQL_BRC_EXPLICIT,"EXPLICIT"},
		{SQL_BRC_ROLLED_UP,"ROLLED_UP"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,brc);
	databasefeatures[FEATURE_BATCH_ROW_COUNTS]=sb.detachString();

	// SQL_QUALIFIER_NAME_SEPARATOR -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_QUALIFIER_NAME_SEPARATOR,
					strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_CATALOG_SEPARATOR]=
				charstring::duplicate(strbuf);

	// SQL_QUALIFIER_TERM -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_QUALIFIER_TERM,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_CATALOG_TERM]=charstring::duplicate(strbuf);

	// SQL_QUALIFIER_USAGE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_QUALIFIER_USAGE,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	cu[]={
		{SQL_CU_DML_STATEMENTS,"DATA_MANIPULATION"},
		{SQL_CU_INDEX_DEFINITION,"INDEX_DEFINITIONS"},
		{SQL_CU_PRIVILEGE_DEFINITION,"PRIVILEGE_DEFINITIONS"},
		{SQL_CU_PROCEDURE_INVOCATION,"PROCEDURE_CALLS"},
		{SQL_CU_TABLE_DEFINITION,"TABLE_DEFINITIONS"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,cu);
	databasefeatures[FEATURE_CATALOG_USAGE]=sb.detachString();

	// SQL_COLLATION_SEQ -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_COLLATION_SEQ,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_COLLATION_SEQ]=charstring::duplicate(strbuf);

	// SQL_CREATE_ASSERTION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_ASSERTION,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ca[]={
		{SQL_CA_CREATE_ASSERTION,
			"CREATE_ASSERTION"},
		{SQL_CA_CONSTRAINT_INITIALLY_DEFERRED,
			"CONSTRAINT_INITIALLY_DEFERRED"},
		{SQL_CA_CONSTRAINT_INITIALLY_IMMEDIATE,
			"CONSTRAINT_INITIALLY_IMMEDIATE"},
		{SQL_CA_CONSTRAINT_DEFERRABLE,
			"CONSTRAINT_DEFERRABLE"},
		{SQL_CA_CONSTRAINT_NON_DEFERRABLE,
			"CONSTRAINT_NON_DEFERRABLE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ca);
	databasefeatures[FEATURE_CREATE_ASSERTION_CLAUSES]=sb.detachString();

	// SQL_CREATE_CHARACTER_SET -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_CHARACTER_SET,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ccs[]={
		{SQL_CCS_CREATE_CHARACTER_SET,
			"CREATE_CHARACTER_SET"},
		{SQL_CCS_COLLATE_CLAUSE,"COLLATE_CLAUSE"},
		{SQL_CCS_LIMITED_COLLATION,
			"LIMITED_COLLATION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ccs);
	databasefeatures[FEATURE_CREATE_CHARACTER_SET_CLAUSES]=
							sb.detachString();

	// SQL_CREATE_COLLATION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_COLLATION,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ccol[]={
		{SQL_CCOL_CREATE_COLLATION,"CREATE_COLLATION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ccol);
	databasefeatures[FEATURE_CREATE_COLLATION_CLAUSES]=sb.detachString();

	// SQL_CREATE_DOMAIN -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_DOMAIN,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	cdo[]={
		{SQL_CDO_CREATE_DOMAIN,
			"CREATE_DOMAIN"},
		{SQL_CDO_CONSTRAINT_NAME_DEFINITION,
			"CONSTRAINT_NAME_DEFINITION"},
		{SQL_CDO_DEFAULT,
			"DEFAULT"},
		{SQL_CDO_CONSTRAINT,
			"CONSTRAINT"},
		{SQL_CDO_COLLATION,
			"COLLATION"},
		{SQL_CDO_CONSTRAINT_INITIALLY_DEFERRED,
			"CONSTRAINT_INITIALLY_DEFERRED"},
		{SQL_CDO_CONSTRAINT_INITIALLY_IMMEDIATE,
			"CONSTRAINT_INITIALLY_IMMEDIATE"},
		{SQL_CDO_CONSTRAINT_DEFERRABLE,
			"CONSTRAINT_DEFERRABLE"},
		{SQL_CDO_CONSTRAINT_NON_DEFERRABLE,
			"CONSTRAINT_NON_DEFERRABLE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,cdo);
	databasefeatures[FEATURE_CREATE_DOMAIN_CLAUSES]=sb.detachString();

	// SQL_CREATE_SCHEMA -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_SCHEMA,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	cs[]={
		{SQL_CS_CREATE_SCHEMA,"CREATE_SCHEMA"},
		{SQL_CS_AUTHORIZATION,"AUTHORIZATION"},
		{SQL_CS_DEFAULT_CHARACTER_SET,"DEFAULT_CHARACTER_SET"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,cs);
	databasefeatures[FEATURE_CREATE_SCHEMA_CLAUSES]=sb.detachString();

	// SQL_CREATE_TABLE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_TABLE,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ct[]={
		{SQL_CT_CREATE_TABLE,
				"CREATE_TABLE"},
		{SQL_CT_TABLE_CONSTRAINT,
				"TABLE_CONSTRAINT"},
		{SQL_CT_CONSTRAINT_NAME_DEFINITION,
				"CONSTRAINT_NAME_DEFINITION"},
		{SQL_CT_COMMIT_DELETE,
				"COMMIT_DELETE"},
		{SQL_CT_COMMIT_PRESERVE,
				"COMMIT_PRESERVE"},
		{SQL_CT_GLOBAL_TEMPORARY,
				"GLOBAL_TEMPORARY"},
		{SQL_CT_LOCAL_TEMPORARY,
				"LOCAL_TEMPORARY"},
		{SQL_CT_COLUMN_CONSTRAINT,
				"COLUMN_CONSTRAINT"},
		{SQL_CT_COLUMN_DEFAULT,
				"COLUMN_DEFAULT"},
		{SQL_CT_COLUMN_COLLATION,
				"COLUMN_COLLATION"},
		{SQL_CT_CONSTRAINT_INITIALLY_DEFERRED,
				"CONSTRAINT_INITIALLY_DEFERRED"},
		{SQL_CT_CONSTRAINT_INITIALLY_IMMEDIATE,
				"CONSTRAINT_INITIALLY_IMMEDIATE"},
		{SQL_CT_CONSTRAINT_DEFERRABLE,
				"CONSTRAINT_DEFERRABLE"},
		{SQL_CT_CONSTRAINT_NON_DEFERRABLE,
				"CONSTRAINT_NON_DEFERRABLE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ct);
	databasefeatures[FEATURE_CREATE_TABLE_CLAUSES]=sb.detachString();

	// SQL_CREATE_TRANSLATION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_TRANSLATION,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ctr[]={
		{SQL_CTR_CREATE_TRANSLATION,"CREATE_TRANSLATION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ctr);
	databasefeatures[FEATURE_CREATE_TRANSLATION_CLAUSES]=sb.detachString();

	// SQL_CREATE_VIEW -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CREATE_VIEW,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	cv[]={
		{SQL_CV_CREATE_VIEW,"CREATE_VIEW"},
		{SQL_CV_CHECK_OPTION,"CHECK_OPTION"},
		{SQL_CV_CASCADED,"CASCADED"},
		{SQL_CV_LOCAL,"LOCAL"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,cv);
	databasefeatures[FEATURE_CREATE_VIEW_CLAUSES]=sb.detachString();

	// SQL_TXN_CAPABLE -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_TXN_CAPABLE,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_DATA_DEFINITION_TRANSACTION_BEHAVIOR]=
		charstring::duplicate(
			(usmallintbuf==SQL_TC_DDL_COMMIT)?"CAUSES_COMMIT":
			(usmallintbuf==SQL_TC_DDL_IGNORE)?
				"IGNORED_IN_TRANSACTIONS":"");

	// SQL_DDL_INDEX -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DDL_INDEX,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	di[]={
		{SQL_DI_CREATE_INDEX,"CREATE_INDEX"},
		{SQL_DI_DROP_INDEX,"DROP_INDEX"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,di);
	databasefeatures[FEATURE_DDL_INDEX_OPERATIONS]=sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_DEFAULT_RESULT_SET_HOLDABILITY]=
					charstring::duplicate("");
	databasefeatures[FEATURE_DELETES_ARE_DETECTED]=
					charstring::duplicate("");

	// SQL_MAX_ROW_SIZE_INCLUDES_LONG -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_MAX_ROW_SIZE_INCLUDES_LONG,
					strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_DOES_MAX_ROW_SIZE_INCLUDE_BLOBS]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_DROP_ASSERTION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_ASSERTION,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	da[]={
		{SQL_DA_DROP_ASSERTION,"DROP_ASSERTION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,da);
	databasefeatures[FEATURE_DROP_ASSERTION_CLAUSES]=sb.detachString();

	// SQL_DROP_CHARACTER_SET -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_CHARACTER_SET,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	dcs[]={
		{SQL_DCS_DROP_CHARACTER_SET,"DROP_CHARACTER_SET"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,dcs);
	databasefeatures[FEATURE_DROP_CHARACTER_SET_CLAUSES]=sb.detachString();

	// SQL_DROP_COLLATION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_COLLATION,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	dc[]={
		{SQL_DC_DROP_COLLATION,"DROP_COLLATION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,dc);
	databasefeatures[FEATURE_DROP_COLLATION_CLAUSES]=sb.detachString();

	// SQL_DROP_DOMAIN -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_DOMAIN,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	dd[]={
		{SQL_DD_DROP_DOMAIN,"DROP_DOMAIN"},
		{SQL_DD_CASCADE,"CASCADE"},
		{SQL_DD_RESTRICT,"RESTRICT"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,dd);
	databasefeatures[FEATURE_DROP_DOMAIN_CLAUSES]=sb.detachString();

	// SQL_DROP_SCHEMA -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_SCHEMA,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ds[]={
		{SQL_DS_DROP_SCHEMA,"DROP_SCHEMA"},
		{SQL_DS_CASCADE,"CASCADE"},
		{SQL_DS_RESTRICT,"RESTRICT"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ds);
	databasefeatures[FEATURE_DROP_SCHEMA_CLAUSES]=sb.detachString();

	// SQL_DROP_TABLE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_TABLE,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	dt[]={
		{SQL_DT_DROP_TABLE,"DROP_TABLE"},
		{SQL_DT_CASCADE,"CASCADE"},
		{SQL_DT_RESTRICT,"RESTRICT"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,dt);
	databasefeatures[FEATURE_DROP_TABLE_CLAUSES]=sb.detachString();

	// SQL_DROP_TRANSLATION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_TRANSLATION,
				&uintbuf,sizeof(uintbuf),&size);
	flagtoname	dtr[]={
		{SQL_DTR_DROP_TRANSLATION,"DROP_TRANSLATION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,dtr);
	databasefeatures[FEATURE_DROP_TRANSLATION_CLAUSES]=sb.detachString();

	// SQL_DROP_VIEW -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DROP_VIEW,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	dv[]={
		{SQL_DV_DROP_VIEW,"DROP_VIEW"},
		{SQL_DV_CASCADE,"CASCADE"},
		{SQL_DV_RESTRICT,"RESTRICT"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,dv);
	databasefeatures[FEATURE_DROP_VIEW_CLAUSES]=sb.detachString();

	// SQL_SPECIAL_CHARACTERS -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_SPECIAL_CHARACTERS,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_EXTRA_NAME_CHARACTERS]=
					charstring::duplicate(strbuf);

	// SQL_SQL92_FOREIGN_KEY_DELETE_RULE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_DELETE_RULE,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	fkdr[]={
		{SQL_SFKD_CASCADE,"CASCADE"},
		{SQL_SFKD_NO_ACTION,"NO_ACTION"},
		{SQL_SFKD_SET_DEFAULT,"SET_DEFAULT"},
		{SQL_SFKD_SET_NULL,"SET_NULL"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,fkdr);
	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=sb.detachString();

	// SQL_SQL92_FOREIGN_KEY_UPDATE_RULE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_UPDATE_RULE,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	fkur[]={
		{SQL_SFKU_CASCADE,"CASCADE"},
		{SQL_SFKU_NO_ACTION,"NO_ACTION"},
		{SQL_SFKU_SET_DEFAULT,"SET_DEFAULT"},
		{SQL_SFKU_SET_NULL,"SET_NULL"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,fkur);
	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=sb.detachString();

	// SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2 -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	foca2[]={
		{SQL_CA2_READ_ONLY_CONCURRENCY,"READ_ONLY_CONCURRENCY"},
		{SQL_CA2_LOCK_CONCURRENCY,"LOCK_CONCURRENCY"},
		{SQL_CA2_OPT_ROWVER_CONCURRENCY,"OPT_ROWVER_CONCURRENCY"},
		{SQL_CA2_OPT_VALUES_CONCURRENCY,"OPT_VALUES_CONCURRENCY"},
		{SQL_CA2_SENSITIVITY_ADDITIONS,"SENSITIVITY_ADDITIONS"},
		{SQL_CA2_SENSITIVITY_DELETIONS,"SENSITIVITY_DELETIONS"},
		{SQL_CA2_SENSITIVITY_UPDATES,"SENSITIVITY_UPDATES"},
		{SQL_CA2_MAX_ROWS_SELECT,"MAX_ROWS_SELECT"},
		{SQL_CA2_MAX_ROWS_INSERT,"MAX_ROWS_INSERT"},
		{SQL_CA2_MAX_ROWS_DELETE,"MAX_ROWS_DELETE"},
		{SQL_CA2_MAX_ROWS_UPDATE,"MAX_ROWS_UPDATE"},
		{SQL_CA2_MAX_ROWS_CATALOG,"MAX_ROWS_CATALOG"},
		{SQL_CA2_CRC_EXACT,"CRC_EXACT"},
		{SQL_CA2_CRC_APPROXIMATE,"CRC_APPROXIMATE"},
		{SQL_CA2_SIMULATE_NON_UNIQUE,"SIMULATE_NON_UNIQUE"},
		{SQL_CA2_SIMULATE_TRY_UNIQUE,"SIMULATE_TRY_UNIQUE"},
		{SQL_CA2_SIMULATE_UNIQUE,"SIMULATE_UNIQUE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,foca2);
	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
							sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
					charstring::duplicate("");

	// SQL_SQL92_GRANT -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_GRANT,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	sg[]={
		{SQL_SG_DELETE_TABLE,"DELETE_TABLE"},
		{SQL_SG_INSERT_COLUMN,"INSERT_COLUMN"},
		{SQL_SG_INSERT_TABLE,"INSERT_TABLE"},
		{SQL_SG_REFERENCES_TABLE,"REFERENCES_TABLE"},
		{SQL_SG_REFERENCES_COLUMN,"REFERENCES_COLUMN"},
		{SQL_SG_SELECT_TABLE,"SELECT_TABLE"},
		{SQL_SG_UPDATE_COLUMN,"UPDATE_COLUMN"},
		{SQL_SG_UPDATE_TABLE,"UPDATE_TABLE"},
		{SQL_SG_USAGE_ON_DOMAIN,"USAGE_ON_DOMAIN"},
		{SQL_SG_USAGE_ON_CHARACTER_SET,"USAGE_ON_CHARACTER_SET"},
		{SQL_SG_USAGE_ON_COLLATION,"USAGE_ON_COLLATION"},
		{SQL_SG_USAGE_ON_TRANSLATION,"USAGE_ON_TRANSLATION"},
		{SQL_SG_WITH_GRANT_OPTION,"WITH_GRANT_OPTION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,sg);
	databasefeatures[FEATURE_GRANT_CLAUSES]=sb.detachString();

	// SQL_GROUP_BY -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_GROUP_BY,&usmallintbuf,sizeof(usmallintbuf),&size);
	appendToList(&sb,"BASIC",
			usmallintbuf!=SQL_GB_NOT_SUPPORTED);
	appendToList(&sb,"BEYOND_SELECT",
			usmallintbuf==SQL_GB_GROUP_BY_CONTAINS_SELECT ||
			usmallintbuf==SQL_GB_NO_RELATION);
	appendToList(&sb,"UNRELATED",
			usmallintbuf==SQL_GB_NO_RELATION);
	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=sb.detachString();

	// SQL_IDENTIFIER_CASE -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_IDENTIFIER_CASE,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		charstring::duplicate(
			(usmallintbuf==SQL_IC_LOWER)?"LOWER":
			(usmallintbuf==SQL_IC_MIXED)?"MIXED":
			(usmallintbuf==SQL_IC_UPPER)?"UPPER":
			(usmallintbuf==SQL_IC_SENSITIVE)?"SENSITIVE":"");

	// SQL_IDENTIFIER_QUOTE_CHAR -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_IDENTIFIER_QUOTE_CHAR,
					strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
					charstring::duplicate(strbuf);

	// SQL_INDEX_KEYWORDS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_INDEX_KEYWORDS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ik[]={
		{SQL_IK_ASC,"ASC"},
		{SQL_IK_DESC,"DESC"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ik);
	databasefeatures[FEATURE_INDEX_KEYWORDS]=sb.detachString();

	// SQL_INFO_SCHEMA_VIEWS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_INFO_SCHEMA_VIEWS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	isv[]={
		{SQL_ISV_ASSERTIONS,"ASSERTIONS"},
		{SQL_ISV_CHARACTER_SETS,"CHARACTER_SETS"},
		{SQL_ISV_CHECK_CONSTRAINTS,"CHECK_CONSTRAINTS"},
		{SQL_ISV_COLLATIONS,"COLLATIONS"},
		{SQL_ISV_COLUMN_DOMAIN_USAGE,"COLUMN_DOMAIN_USAGE"},
		{SQL_ISV_COLUMN_PRIVILEGES,"COLUMN_PRIVILEGES"},
		{SQL_ISV_COLUMNS,"COLUMNS"},
		{SQL_ISV_CONSTRAINT_COLUMN_USAGE,"CONSTRAINT_COLUMN_USAGE"},
		{SQL_ISV_CONSTRAINT_TABLE_USAGE,"CONSTRAINT_TABLE_USAGE"},
		{SQL_ISV_DOMAIN_CONSTRAINTS,"DOMAIN_CONSTRAINTS"},
		{SQL_ISV_DOMAINS,"DOMAINS"},
		{SQL_ISV_KEY_COLUMN_USAGE,"KEY_COLUMN_USAGE"},
		{SQL_ISV_REFERENTIAL_CONSTRAINTS,"REFERENTIAL_CONSTRAINTS"},
		{SQL_ISV_SCHEMATA,"SCHEMATA"},
		{SQL_ISV_SQL_LANGUAGES,"SQL_LANGUAGES"},
		{SQL_ISV_TABLE_CONSTRAINTS,"TABLE_CONSTRAINTS"},
		{SQL_ISV_TABLE_PRIVILEGES,"TABLE_PRIVILEGES"},
		{SQL_ISV_TABLES,"TABLES"},
		{SQL_ISV_TRANSLATIONS,"TRANSLATIONS"},
		{SQL_ISV_USAGE_PRIVILEGES,"USAGE_PRIVILEGES"},
		{SQL_ISV_VIEW_COLUMN_USAGE,"VIEW_COLUMN_USAGE"},
		{SQL_ISV_VIEW_TABLE_USAGE,"VIEW_TABLE_USAGE"},
		{SQL_ISV_VIEWS,"VIEWS"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,isv);
	databasefeatures[FEATURE_INFO_SCHEMA_VIEWS]=sb.detachString();

	// SQL_INSERT_STATEMENT -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_INSERT_STATEMENT,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	is[]={
		{SQL_IS_INSERT_LITERALS,"INSERT_LITERALS"},
		{SQL_IS_INSERT_SEARCHED,"INSERT_SEARCHED"},
		{SQL_IS_SELECT_INTO,"SELECT_INTO"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,is);
	databasefeatures[FEATURE_INSERT_OPERATIONS]=sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_INSERTS_ARE_DETECTED]=
		charstring::duplicate("");

	// SQL_QUALIFIER_LOCATION -> usmallint, check for SQL_CL_START
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_QUALIFIER_LOCATION,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		charstring::duplicate(
			(usmallintbuf==SQL_CL_START)?"true":"false");

	// SQL_TXN_ISOLATION_OPTION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_TXN_ISOLATION_OPTION,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	tilf[]={
		{SQL_TXN_READ_UNCOMMITTED,"READ_UNCOMMITTED"},
		{SQL_TXN_READ_COMMITTED,"READ_COMMITTED"},
		{SQL_TXN_REPEATABLE_READ,"REPEATABLE_READ"},
		{SQL_TXN_SERIALIZABLE,"SERIALIZABLE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,tilf);
	databasefeatures[FEATURE_ISOLATION_LEVELS]=sb.detachString();

	// SQL_FILE_USAGE -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_FILE_USAGE,&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		charstring::duplicate(
			(usmallintbuf==SQL_FILE_TABLE)?
				"LOCAL_FILE_PER_TABLE":
			(usmallintbuf==SQL_FILE_CATALOG)?
				"LOCAL_FILES":"");

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
					charstring::duplicate("");

	// SQL_LOCK_TYPES -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_LOCK_TYPES,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	lt[]={
		{SQL_LCK_NO_CHANGE,"NO_CHANGE"},
		{SQL_LCK_EXCLUSIVE,"EXCLUSIVE"},
		{SQL_LCK_UNLOCK,"UNLOCK"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,lt);
	databasefeatures[FEATURE_LOCK_TYPES]=sb.detachString();

	// SQL_MAX_BINARY_LITERAL_LEN -> uintval
	uintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_BINARY_LITERAL_LEN,
					&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		charstring::parseNumber((uint32_t)uintbuf);

	// SQL_MAX_CATALOG_NAME_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_CATALOG_NAME_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_CHAR_LITERAL_LEN -> uintval
	uintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_CHAR_LITERAL_LEN,
					&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		charstring::parseNumber((uint32_t)uintbuf);

	// SQL_MAX_COLUMN_NAME_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_COLUMN_NAME_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_COLUMNS_IN_GROUP_BY -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_GROUP_BY,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_COLUMNS_IN_INDEX -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_INDEX,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_COLUMNS_IN_ORDER_BY -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_ORDER_BY,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_COLUMNS_IN_SELECT -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_SELECT,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		charstring::parseNumber(
			cont->capDatabaseFeatureLimit(
				usmallintbuf,cont->getMaxColumnCount()));

	// SQL_MAX_COLUMNS_IN_TABLE -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_TABLE,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		charstring::parseNumber(usmallintbuf);

	// SQL_ACTIVE_CONNECTIONS -> usmallint
	databasefeatures[FEATURE_MAX_CONNECTIONS]=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	// SQL_MAX_CURSOR_NAME_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_CURSOR_NAME_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_IDENTIFIER_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_IDENTIFIER_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_INDEX_SIZE -> uintval
	uintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_INDEX_SIZE,&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		charstring::parseNumber((uint32_t)uintbuf);

	// SQL_MAX_PROCEDURE_NAME_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_PROCEDURE_NAME_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_ROW_SIZE -> uintval
	uintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_ROW_SIZE,&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		charstring::parseNumber((uint32_t)uintbuf);

	// SQL_MAX_OWNER_NAME_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_OWNER_NAME_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_STATEMENT_LEN -> uintval
	uintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_STATEMENT_LEN,&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		charstring::parseNumber(
			cont->capDatabaseFeatureLimit(
				uintbuf,cont->getConfig()->getMaxQuerySize()));

	// SQL_ACTIVE_STATEMENTS -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_ACTIVE_STATEMENTS,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_STATEMENTS]=
		charstring::parseNumber(
			cont->capDatabaseFeatureLimit(
				usmallintbuf,cont->getConfig()->getMaxCursors()));

	// SQL_MAX_TABLE_NAME_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_TABLE_NAME_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_TABLES_IN_SELECT -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_TABLES_IN_SELECT,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		charstring::parseNumber(usmallintbuf);

	// SQL_MAX_USER_NAME_LEN -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_MAX_USER_NAME_LEN,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		charstring::parseNumber(usmallintbuf);

	// SQL_NEED_LONG_DATA_LEN -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_NEED_LONG_DATA_LEN,
				strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_CONCAT_NULL_BEHAVIOR -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_CONCAT_NULL_BEHAVIOR,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		charstring::duplicate(
			(usmallintbuf==SQL_CB_NULL)?"true":"false");

	// SQL_NULL_COLLATION -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_NULL_COLLATION,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		charstring::duplicate(
			(usmallintbuf==SQL_NC_END)?"AT_END":
			(usmallintbuf==SQL_NC_START)?"AT_START":
			(usmallintbuf==SQL_NC_HIGH)?"HIGH":
			(usmallintbuf==SQL_NC_LOW)?"LOW":"");

	// SQL_NUMERIC_FUNCTIONS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_NUMERIC_FUNCTIONS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	nf[]={
		{SQL_FN_NUM_ABS,"ABS"},
		{SQL_FN_NUM_ACOS,"ACOS"},
		{SQL_FN_NUM_ASIN,"ASIN"},
		{SQL_FN_NUM_ATAN,"ATAN"},
		{SQL_FN_NUM_ATAN2,"ATAN2"},
		{SQL_FN_NUM_CEILING,"CEILING"},
		{SQL_FN_NUM_COS,"COS"},
		{SQL_FN_NUM_COT,"COT"},
		{SQL_FN_NUM_DEGREES,"DEGREES"},
		{SQL_FN_NUM_EXP,"EXP"},
		{SQL_FN_NUM_FLOOR,"FLOOR"},
		{SQL_FN_NUM_LOG,"LOG"},
		{SQL_FN_NUM_LOG10,"LOG10"},
		{SQL_FN_NUM_MOD,"MOD"},
		{SQL_FN_NUM_PI,"PI"},
		{SQL_FN_NUM_POWER,"POWER"},
		{SQL_FN_NUM_RADIANS,"RADIANS"},
		{SQL_FN_NUM_RAND,"RAND"},
		{SQL_FN_NUM_ROUND,"ROUND"},
		{SQL_FN_NUM_SIGN,"SIGN"},
		{SQL_FN_NUM_SIN,"SIN"},
		{SQL_FN_NUM_SQRT,"SQRT"},
		{SQL_FN_NUM_TAN,"TAN"},
		{SQL_FN_NUM_TRUNCATE,"TRUNCATE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,nf);
	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=sb.detachString();

	// SQL_CURSOR_COMMIT_BEHAVIOR, SQL_CURSOR_ROLLBACK_BEHAVIOR -> enums
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_CURSOR_COMMIT_BEHAVIOR,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	bool	openAcrossCommit=(usmallintbuf==SQL_CB_PRESERVE);
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_CURSOR_ROLLBACK_BEHAVIOR,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	bool	openAcrossRollback=(usmallintbuf==SQL_CB_PRESERVE);
	databasefeatures[FEATURE_OPEN_CURSORS_ACROSS]=
		charstring::duplicate(
			(openAcrossCommit && openAcrossRollback)?
				"COMMIT,ROLLBACK":
			openAcrossCommit?"COMMIT":
			openAcrossRollback?"ROLLBACK":"");
	databasefeatures[FEATURE_OPEN_STATEMENTS_ACROSS]=
		charstring::duplicate(
			(openAcrossCommit && openAcrossRollback)?
				"COMMIT,ROLLBACK":
			openAcrossCommit?"COMMIT":
			openAcrossRollback?"ROLLBACK":"");

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_OTHERS_DELETES_ARE_VISIBLE]=
					charstring::duplicate("");
	databasefeatures[FEATURE_OTHERS_INSERTS_ARE_VISIBLE]=
					charstring::duplicate("");
	databasefeatures[FEATURE_OTHERS_UPDATES_ARE_VISIBLE]=
					charstring::duplicate("");

	// SQL_OJ_CAPABILITIES -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_OJ_CAPABILITIES,&uintbuf,sizeof(uintbuf),&size);
	appendToList(&sb,"BASIC",uintbuf&(SQL_OJ_LEFT|SQL_OJ_RIGHT));
	appendToList(&sb,"FULL",uintbuf&SQL_OJ_FULL);
	appendToList(&sb,"LIMITED",uintbuf&SQL_OJ_LEFT);
	databasefeatures[FEATURE_OUTER_JOINS]=sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_OWN_DELETES_ARE_VISIBLE]=
					charstring::duplicate("");
	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
					charstring::duplicate("");
	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
					charstring::duplicate("");

	// SQL_SQL92_PREDICATES -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_PREDICATES,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	sp[]={
		{SQL_SP_BETWEEN,"BETWEEN"},
		{SQL_SP_COMPARISON,"COMPARISON"},
		{SQL_SP_EXISTS,"EXISTS"},
		{SQL_SP_IN,"IN"},
		{SQL_SP_ISNOTNULL,"ISNOTNULL"},
		{SQL_SP_ISNULL,"ISNULL"},
		{SQL_SP_LIKE,"LIKE"},
		{SQL_SP_MATCH_FULL,"MATCH_FULL"},
		{SQL_SP_MATCH_PARTIAL,"MATCH_PARTIAL"},
		{SQL_SP_MATCH_UNIQUE_FULL,"MATCH_UNIQUE_FULL"},
		{SQL_SP_MATCH_UNIQUE_PARTIAL,"MATCH_UNIQUE_PARTIAL"},
		{SQL_SP_OVERLAPS,"OVERLAPS"},
		{SQL_SP_QUANTIFIED_COMPARISON,"QUANTIFIED_COMPARISON"},
		{SQL_SP_UNIQUE,"UNIQUE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,sp);
	databasefeatures[FEATURE_PREDICATES]=sb.detachString();

	// SQL_PROCEDURE_TERM -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_PROCEDURE_TERM,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_PROCEDURE_TERM]=charstring::duplicate(strbuf);

	// SQL_QUOTED_IDENTIFIER_CASE -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_QUOTED_IDENTIFIER_CASE,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		charstring::duplicate(
			(usmallintbuf==SQL_IC_LOWER)?"LOWER":
			(usmallintbuf==SQL_IC_MIXED)?"MIXED":
			(usmallintbuf==SQL_IC_UPPER)?"UPPER":
			(usmallintbuf==SQL_IC_SENSITIVE)?"SENSITIVE":"");

	// SQL_SQL92_RELATIONAL_JOIN_OPERATORS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_RELATIONAL_JOIN_OPERATORS,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	rjo[]={
		{SQL_SRJO_CORRESPONDING_CLAUSE,"CORRESPONDING_CLAUSE"},
		{SQL_SRJO_CROSS_JOIN,"CROSS_JOIN"},
		{SQL_SRJO_EXCEPT_JOIN,"EXCEPT_JOIN"},
		{SQL_SRJO_FULL_OUTER_JOIN,"FULL_OUTER_JOIN"},
		{SQL_SRJO_INNER_JOIN,"INNER_JOIN"},
		{SQL_SRJO_INTERSECT_JOIN,"INTERSECT_JOIN"},
		{SQL_SRJO_LEFT_OUTER_JOIN,"LEFT_OUTER_JOIN"},
		{SQL_SRJO_NATURAL_JOIN,"NATURAL_JOIN"},
		{SQL_SRJO_RIGHT_OUTER_JOIN,"RIGHT_OUTER_JOIN"},
		{SQL_SRJO_UNION_JOIN,"UNION_JOIN"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,rjo);
	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
					charstring::duplicate("");
	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
					charstring::duplicate("");
	databasefeatures[FEATURE_RESULT_SET_TYPES]=
					charstring::duplicate("");

	// SQL_SQL92_REVOKE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_REVOKE,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	sr[]={
		{SQL_SR_CASCADE,"CASCADE"},
		{SQL_SR_DELETE_TABLE,"DELETE_TABLE"},
		{SQL_SR_GRANT_OPTION_FOR,"GRANT_OPTION_FOR"},
		{SQL_SR_INSERT_COLUMN,"INSERT_COLUMN"},
		{SQL_SR_INSERT_TABLE,"INSERT_TABLE"},
		{SQL_SR_REFERENCES_COLUMN,"REFERENCES_COLUMN"},
		{SQL_SR_REFERENCES_TABLE,"REFERENCES_TABLE"},
		{SQL_SR_RESTRICT,"RESTRICT"},
		{SQL_SR_SELECT_TABLE,"SELECT_TABLE"},
		{SQL_SR_UPDATE_COLUMN,"UPDATE_COLUMN"},
		{SQL_SR_UPDATE_TABLE,"UPDATE_TABLE"},
		{SQL_SR_USAGE_ON_DOMAIN,"USAGE_ON_DOMAIN"},
		{SQL_SR_USAGE_ON_CHARACTER_SET,"USAGE_ON_CHARACTER_SET"},
		{SQL_SR_USAGE_ON_COLLATION,"USAGE_ON_COLLATION"},
		{SQL_SR_USAGE_ON_TRANSLATION,"USAGE_ON_TRANSLATION"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,sr);
	databasefeatures[FEATURE_REVOKE_CLAUSES]=sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_ROW_ID_LIFETIME]=charstring::duplicate("");

	// SQL_SQL92_ROW_VALUE_CONSTRUCTOR -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_ROW_VALUE_CONSTRUCTOR,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	rvc[]={
		{SQL_SRVC_VALUE_EXPRESSION,"VALUE_EXPRESSION"},
		{SQL_SRVC_NULL,"NULL"},
		{SQL_SRVC_DEFAULT,"DEFAULT"},
		{SQL_SRVC_ROW_SUBQUERY,"ROW_SUBQUERY"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,rvc);
	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
							sb.detachString();

	// SQL_OWNER_TERM -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_OWNER_TERM,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SCHEMA_TERM]=charstring::duplicate(strbuf);

	// SQL_OWNER_USAGE -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_OWNER_USAGE,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	su[]={
		{SQL_SU_DML_STATEMENTS,"DATA_MANIPULATION"},
		{SQL_SU_INDEX_DEFINITION,"INDEX_DEFINITIONS"},
		{SQL_SU_PRIVILEGE_DEFINITION,"PRIVILEGE_DEFINITIONS"},
		{SQL_SU_PROCEDURE_INVOCATION,"PROCEDURE_CALLS"},
		{SQL_SU_TABLE_DEFINITION,"TABLE_DEFINITIONS"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,su);
	databasefeatures[FEATURE_SCHEMA_USAGE]=sb.detachString();

	// SQL_SCROLL_CONCURRENCY -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SCROLL_CONCURRENCY,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	scco[]={
		{SQL_SCCO_READ_ONLY,"READ_ONLY"},
		{SQL_SCCO_LOCK,"LOCK"},
		{SQL_SCCO_OPT_ROWVER,"OPT_ROWVER"},
		{SQL_SCCO_OPT_VALUES,"OPT_VALUES"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,scco);
	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=sb.detachString();

	// SQL_SEARCH_PATTERN_ESCAPE -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_SEARCH_PATTERN_ESCAPE,
					strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
					charstring::duplicate(strbuf);

	// SQL_ODBC_SQL_CONFORMANCE -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_ODBC_SQL_CONFORMANCE,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	appendToList(&sb,"CORE",usmallintbuf>=SQL_OSC_CORE);
	appendToList(&sb,"EXTENDED",usmallintbuf>=SQL_OSC_EXTENDED);
	appendToList(&sb,"MINIMUM",true);
	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=sb.detachString();

	// SQL_KEYWORDS -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_KEYWORDS,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SQL_KEYWORDS]=charstring::duplicate(strbuf);

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_SQL_STATE_TYPE]=charstring::duplicate("");

	// SQL_STATIC_CURSOR_ATTRIBUTES2 -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_STATIC_CURSOR_ATTRIBUTES2,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	sca2[]={
		{SQL_CA2_READ_ONLY_CONCURRENCY,"READ_ONLY_CONCURRENCY"},
		{SQL_CA2_LOCK_CONCURRENCY,"LOCK_CONCURRENCY"},
		{SQL_CA2_OPT_ROWVER_CONCURRENCY,"OPT_ROWVER_CONCURRENCY"},
		{SQL_CA2_OPT_VALUES_CONCURRENCY,"OPT_VALUES_CONCURRENCY"},
		{SQL_CA2_SENSITIVITY_ADDITIONS,"SENSITIVITY_ADDITIONS"},
		{SQL_CA2_SENSITIVITY_DELETIONS,"SENSITIVITY_DELETIONS"},
		{SQL_CA2_SENSITIVITY_UPDATES,"SENSITIVITY_UPDATES"},
		{SQL_CA2_MAX_ROWS_SELECT,"MAX_ROWS_SELECT"},
		{SQL_CA2_MAX_ROWS_INSERT,"MAX_ROWS_INSERT"},
		{SQL_CA2_MAX_ROWS_DELETE,"MAX_ROWS_DELETE"},
		{SQL_CA2_MAX_ROWS_UPDATE,"MAX_ROWS_UPDATE"},
		{SQL_CA2_MAX_ROWS_CATALOG,"MAX_ROWS_CATALOG"},
		{SQL_CA2_CRC_EXACT,"CRC_EXACT"},
		{SQL_CA2_CRC_APPROXIMATE,"CRC_APPROXIMATE"},
		{SQL_CA2_SIMULATE_NON_UNIQUE,"SIMULATE_NON_UNIQUE"},
		{SQL_CA2_SIMULATE_TRY_UNIQUE,"SIMULATE_TRY_UNIQUE"},
		{SQL_CA2_SIMULATE_UNIQUE,"SIMULATE_UNIQUE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,sca2);
	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=sb.detachString();

	// SQL_PROCEDURES -> Y/N
	// no obvious ODBC equivalent for call syntax
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_PROCEDURES,
				strbuf,sizeof(strbuf),&size);
	appendToList(&sb,"PROCEDURES",strbuf[0]=='Y');
	databasefeatures[FEATURE_STORED_PROGRAMS]=sb.detachString();

	// SQL_STRING_FUNCTIONS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_STRING_FUNCTIONS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	sf[]={
		{SQL_FN_STR_CONCAT,"CONCAT"},
		{SQL_FN_STR_INSERT,"INSERT"},
		{SQL_FN_STR_LEFT,"LEFT"},
		{SQL_FN_STR_LTRIM,"LTRIM"},
		{SQL_FN_STR_LENGTH,"LENGTH"},
		{SQL_FN_STR_LOCATE,"LOCATE"},
		{SQL_FN_STR_LCASE,"LCASE"},
		{SQL_FN_STR_REPEAT,"REPEAT"},
		{SQL_FN_STR_REPLACE,"REPLACE"},
		{SQL_FN_STR_RIGHT,"RIGHT"},
		{SQL_FN_STR_RTRIM,"RTRIM"},
		{SQL_FN_STR_SUBSTRING,"SUBSTRING"},
		{SQL_FN_STR_UCASE,"UCASE"},
		{SQL_FN_STR_ASCII,"ASCII"},
		{SQL_FN_STR_CHAR,"CHAR"},
		{SQL_FN_STR_DIFFERENCE,"DIFFERENCE"},
		{SQL_FN_STR_LOCATE_2,"LOCATE_2"},
		{SQL_FN_STR_SOUNDEX,"SOUNDEX"},
		{SQL_FN_STR_SPACE,"SPACE"},
		#if (ODBCVER >= 0x0300)
		{SQL_FN_STR_BIT_LENGTH,"BIT_LENGTH"},
		{SQL_FN_STR_CHAR_LENGTH,"CHAR_LENGTH"},
		{SQL_FN_STR_CHARACTER_LENGTH,"CHARACTER_LENGTH"},
		{SQL_FN_STR_OCTET_LENGTH,"OCTET_LENGTH"},
		{SQL_FN_STR_POSITION,"POSITION"},
		#endif
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,sf);
	databasefeatures[FEATURE_STRING_FUNCTIONS]=sb.detachString();

	// SQL_SUBQUERIES -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SUBQUERIES,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	squ[]={
		{SQL_SQ_COMPARISON,"COMPARISONS"},
		{SQL_SQ_EXISTS,"EXISTS"},
		{SQL_SQ_IN,"INS"},
		{SQL_SQ_QUANTIFIED,"QUANTIFIEDS"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,squ);
	databasefeatures[FEATURE_SUBQUERY_USAGE]=sb.detachString();

	// SQL_BATCH_SUPPORT -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_BATCH_SUPPORT,&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		charstring::duplicate((uintbuf!=0)?"true":"false");

	// SQL_COLUMN_ALIAS -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_COLUMN_ALIAS,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_CONVERT_FUNCTIONS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_CONVERT_FUNCTIONS,&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		charstring::duplicate(
			(uintbuf&SQL_FN_CVT_CONVERT)?"true":"false");

	// SQL_SUBQUERIES -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SUBQUERIES,&uintbuf,sizeof(uintbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		charstring::duplicate(
			(uintbuf&SQL_SQ_CORRELATED_SUBQUERIES)?"true":"false");

	// SQL_DESCRIBE_PARAMETER -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_DESCRIBE_PARAMETER,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_DESCRIBE_PARAMETER]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_EXPRESSIONS_IN_ORDERBY -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_EXPRESSIONS_IN_ORDERBY,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
					charstring::duplicate("");

	// SQL_INTEGRITY -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_INTEGRITY,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_LIKE_ESCAPE_CLAUSE -> Y/N
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_LIKE_ESCAPE_CLAUSE,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"false");

	// SQL_MULT_RESULT_SETS -> true/false
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_MULT_RESULT_SETS,
				strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"");

	// SQL_MULTIPLE_ACTIVE_TXN -> true/false
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_MULTIPLE_ACTIVE_TXN,
				strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_TRANSACTIONS]=
		charstring::duplicate((strbuf[0]=='Y')?"true":"");

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_SUPPORTS_NAMED_PARAMETERS]=
					charstring::duplicate("");

	// SQL_NON_NULLABLE_COLUMNS -> usmallint
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_NON_NULLABLE_COLUMNS,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_NON_NULLABLE_COLUMNS]=
		charstring::duplicate(
			(usmallintbuf==SQL_NNC_NON_NULL)?"true":"false");

	// SQL_ORDER_BY_COLUMNS_IN_SELECT -> Y/N (inverted)
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_ORDER_BY_COLUMNS_IN_SELECT,
					strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_ORDER_BY_UNRELATED]=
		charstring::duplicate((strbuf[0]=='N')?"true":"false");

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_SUPPORTS_SAVEPOINTS]=
					charstring::duplicate("");

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_SUPPORTS_SELECT_FOR_UPDATE]=
					charstring::duplicate("");

	// SQL_TXN_CAPABLE -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_TXN_CAPABLE,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		charstring::duplicate(
			(usmallintbuf!=SQL_TC_NONE)?"true":"false");

	// SQL_SYSTEM_FUNCTIONS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SYSTEM_FUNCTIONS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	syf[]={
		{SQL_FN_SYS_USERNAME,"USER"},
		{SQL_FN_SYS_DBNAME,"DBNAME"},
		{SQL_FN_SYS_IFNULL,"IFNULL"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,syf);
	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=sb.detachString();

	// SQL_CORRELATION_NAME -> enum
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_CORRELATION_NAME,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		charstring::duplicate(
			(usmallintbuf==SQL_CN_DIFFERENT)?
				"BASIC,DIFFERENT":
			(usmallintbuf!=SQL_CN_NONE)?
				"BASIC":"");

	// SQL_TABLE_TERM -> string
	strbuf[0]='\0';
	SQLGetInfo(dbc,SQL_TABLE_TERM,strbuf,sizeof(strbuf),&size);
	databasefeatures[FEATURE_TABLE_TERM]=charstring::duplicate(strbuf);

	// SQL_TIMEDATE_ADD_INTERVALS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_TIMEDATE_ADD_INTERVALS,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	tdai[]={
		{SQL_FN_TSI_FRAC_SECOND,"FRAC_SECOND"},
		{SQL_FN_TSI_SECOND,"SECOND"},
		{SQL_FN_TSI_MINUTE,"MINUTE"},
		{SQL_FN_TSI_HOUR,"HOUR"},
		{SQL_FN_TSI_DAY,"DAY"},
		{SQL_FN_TSI_WEEK,"WEEK"},
		{SQL_FN_TSI_MONTH,"MONTH"},
		{SQL_FN_TSI_QUARTER,"QUARTER"},
		{SQL_FN_TSI_YEAR,"YEAR"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,tdai);
	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=sb.detachString();

	// SQL_TIMEDATE_DIFF_INTERVALS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_TIMEDATE_DIFF_INTERVALS,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	tddi[]={
		{SQL_FN_TSI_FRAC_SECOND,"FRAC_SECOND"},
		{SQL_FN_TSI_SECOND,"SECOND"},
		{SQL_FN_TSI_MINUTE,"MINUTE"},
		{SQL_FN_TSI_HOUR,"HOUR"},
		{SQL_FN_TSI_DAY,"DAY"},
		{SQL_FN_TSI_WEEK,"WEEK"},
		{SQL_FN_TSI_MONTH,"MONTH"},
		{SQL_FN_TSI_QUARTER,"QUARTER"},
		{SQL_FN_TSI_YEAR,"YEAR"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,tddi);
	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=sb.detachString();

	// SQL_TIMEDATE_FUNCTIONS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_TIMEDATE_FUNCTIONS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	tf[]={
		{SQL_FN_TD_NOW,"NOW"},
		{SQL_FN_TD_CURDATE,"CURDATE"},
		{SQL_FN_TD_DAYOFMONTH,"DAYOFMONTH"},
		{SQL_FN_TD_DAYOFWEEK,"DAYOFWEEK"},
		{SQL_FN_TD_DAYOFYEAR,"DAYOFYEAR"},
		{SQL_FN_TD_MONTH,"MONTH"},
		{SQL_FN_TD_QUARTER,"QUARTER"},
		{SQL_FN_TD_WEEK,"WEEK"},
		{SQL_FN_TD_YEAR,"YEAR"},
		{SQL_FN_TD_CURTIME,"CURTIME"},
		{SQL_FN_TD_HOUR,"HOUR"},
		{SQL_FN_TD_MINUTE,"MINUTE"},
		{SQL_FN_TD_SECOND,"SECOND"},
		{SQL_FN_TD_TIMESTAMPADD,"TIMESTAMPADD"},
		{SQL_FN_TD_TIMESTAMPDIFF,"TIMESTAMPDIFF"},
		{SQL_FN_TD_DAYNAME,"DAYNAME"},
		{SQL_FN_TD_MONTHNAME,"MONTHNAME"},
		#if (ODBCVER >= 0x0300)
		{SQL_FN_TD_CURRENT_DATE,"CURRENT_DATE"},
		{SQL_FN_TD_CURRENT_TIME,"CURRENT_TIME"},
		{SQL_FN_TD_CURRENT_TIMESTAMP,"CURRENT_TIMESTAMP"},
		{SQL_FN_TD_EXTRACT,"EXTRACT"},
		#endif
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,tf);
	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=sb.detachString();

	// SQL_DATETIME_LITERALS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_DATETIME_LITERALS,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	dtl[]={
		{SQL_DL_SQL92_DATE,"DATE"},
		{SQL_DL_SQL92_TIME,"TIME"},
		{SQL_DL_SQL92_TIMESTAMP,"TIMESTAMP"},
		{SQL_DL_SQL92_INTERVAL_YEAR,"INTERVAL_YEAR"},
		{SQL_DL_SQL92_INTERVAL_MONTH,"INTERVAL_MONTH"},
		{SQL_DL_SQL92_INTERVAL_DAY,"INTERVAL_DAY"},
		{SQL_DL_SQL92_INTERVAL_HOUR,"INTERVAL_HOUR"},
		{SQL_DL_SQL92_INTERVAL_MINUTE,"INTERVAL_MINUTE"},
		{SQL_DL_SQL92_INTERVAL_SECOND,"INTERVAL_SECOND"},
		{SQL_DL_SQL92_INTERVAL_YEAR_TO_MONTH,
			"INTERVAL_YEAR_TO_MONTH"},
		{SQL_DL_SQL92_INTERVAL_DAY_TO_HOUR,
			"INTERVAL_DAY_TO_HOUR"},
		{SQL_DL_SQL92_INTERVAL_DAY_TO_MINUTE,
			"INTERVAL_DAY_TO_MINUTE"},
		{SQL_DL_SQL92_INTERVAL_DAY_TO_SECOND,
			"INTERVAL_DAY_TO_SECOND"},
		{SQL_DL_SQL92_INTERVAL_HOUR_TO_MINUTE,
			"INTERVAL_HOUR_TO_MINUTE"},
		{SQL_DL_SQL92_INTERVAL_HOUR_TO_SECOND,
			"INTERVAL_HOUR_TO_SECOND"},
		{SQL_DL_SQL92_INTERVAL_MINUTE_TO_SECOND,
			"INTERVAL_MINUTE_TO_SECOND"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,dtl);
	databasefeatures[FEATURE_TIME_DATE_LITERALS]=sb.detachString();

	// SQL_TXN_CAPABLE -> enum (reuse from above)
	usmallintbuf=0;
	SQLGetInfo(dbc,SQL_TXN_CAPABLE,
				&usmallintbuf,sizeof(usmallintbuf),&size);
	databasefeatures[FEATURE_TRANSACTION_DDL_DML]=
		charstring::duplicate(
			(usmallintbuf==SQL_TC_ALL)?"DDL_AND_DML":
			(usmallintbuf==SQL_TC_DML)?"DML_ONLY":"");

	// SQL_UNION -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_UNION,&uintbuf,sizeof(uintbuf),&size);
	flagtoname	un[]={
		{SQL_U_UNION,"UNION"},
		{SQL_U_UNION_ALL,"UNION_ALL"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,un);
	databasefeatures[FEATURE_UNION_CLAUSES]=sb.detachString();

	// no obvious ODBC equivalent
	databasefeatures[FEATURE_UPDATES_ARE_DETECTED]=
					charstring::duplicate("");

	// SQL_SQL92_VALUE_EXPRESSIONS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_SQL92_VALUE_EXPRESSIONS,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	sve[]={
		{SQL_SVE_CASE,"CASE"},
		{SQL_SVE_CAST,"CAST"},
		{SQL_SVE_COALESCE,"COALESCE"},
		{SQL_SVE_NULLIF,"NULLIF"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,sve);
	databasefeatures[FEATURE_VALUE_EXPRESSIONS]=sb.detachString();

	// SQL_POSITIONED_STATEMENTS -> bitmask
	uintbuf=0;
	SQLGetInfo(dbc,SQL_POSITIONED_STATEMENTS,
					&uintbuf,sizeof(uintbuf),&size);
	flagtoname	ps[]={
		{SQL_PS_POSITIONED_DELETE,"DELETE"},
		{SQL_PS_POSITIONED_UPDATE,"UPDATE"},
		{0,NULL}
	};
	flagsToNames(&sb,uintbuf,ps);
	databasefeatures[FEATURE_WHERE_CURRENT_OF_OPERATIONS]=sb.detachString();


	return databasefeatures;
}

const char *odbcconnection::getBindFormat() {
	// FIXME: not true for all db's
	return "?";
}

const char *odbcconnection::getNextvalFormat() {
	// FIXME: not true for all db's
	return "";
}

const char *odbcconnection::getLastInsertIdQuery() {
	return lastinsertidquery;
}

bool odbcconnection::getListsByApiCalls() {
	return true;
}

bool odbcconnection::getCatalogList(sqlrservercursor *cursor,
						const char *catalog) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// get the catalogs
	erg=SQLTables(odbccur->stmt,
			(SQLCHAR *)SQL_ALL_CATALOGS,SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getSchemaList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// get the schemas
	erg=SQLTables(odbccur->stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)SQL_ALL_SCHEMAS,SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getTableTypeList(sqlrservercursor *cursor,
					const char *catalog,
					const char *schema,
					const char *tabletypes) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// get the table types
	const char	*tt=SQL_ALL_TABLE_TYPES;
	if (!charstring::isNullOrEmpty(tabletypes)) {
		tt=tabletypes;
	}
	erg=SQLTables(odbccur->stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)tt,SQL_NTS);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getTableList(sqlrservercursor *cursor,
					const char *catalog,
					const char *schema,
					const char *table,
					uint16_t objecttypes) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// use defaults for NULL parameters
	if (!schema) {
		schema="";
	}
	// FIXME: should this be SQL_ALL_TABLES?
	if (!table) {
		table="%";
	}

	stringbuffer	tabletype;
	if (objecttypes&DB_OBJECT_TABLE) {
		tabletype.append("TABLE");
	}
	if (objecttypes&DB_OBJECT_VIEW) {
		if (tabletype.getSize()) {
			tabletype.append(',');
		}
		tabletype.append("VIEW");
	}
	if (objecttypes&DB_OBJECT_ALIAS) {
		if (tabletype.getSize()) {
			tabletype.append(',');
		}
		tabletype.append("ALIAS");
	}
	if (objecttypes&DB_OBJECT_SYNONYM) {
		if (tabletype.getSize()) {
			tabletype.append(',');
		}
		tabletype.append("SYNONYM");
	}

	// get the table list
	erg=SQLTables(odbccur->stmt,
			(SQLCHAR *)catalog,SQL_NTS,
			(SQLCHAR *)schema,SQL_NTS,
			(SQLCHAR *)table,SQL_NTS,
			(SQLCHAR *)tabletype.getString(),SQL_NTS);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getTypeInfoList(sqlrservercursor *cursor,
					const char *db,
					const char *schema,
					const char *type) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// map the string type to a number
	// FIXME: this will be slooowwww... improve it
	SQLSMALLINT	typenumber=-200;
	if (!charstring::compareIgnoringCase(type,"CHAR")) {
		typenumber=SQL_CHAR;
	} else if (!charstring::compareIgnoringCase(type,"VARCHAR")) {
		typenumber=SQL_VARCHAR;
	} else if (!charstring::compareIgnoringCase(type,"LONGVARCHAR")) {
		typenumber=SQL_LONGVARCHAR;
	} else if (!charstring::compareIgnoringCase(type,"WCHAR")) {
		typenumber=SQL_WCHAR;
	} else if (!charstring::compareIgnoringCase(type,"WVARCHAR")) {
		typenumber=SQL_WVARCHAR;
	} else if (!charstring::compareIgnoringCase(type,"WLONGVARCHAR")) {
		typenumber=SQL_WLONGVARCHAR;
	} else if (!charstring::compareIgnoringCase(type,"DECIMAL")) {
		typenumber=SQL_DECIMAL;
	} else if (!charstring::compareIgnoringCase(type,"NUMERIC")) {
		typenumber=SQL_NUMERIC;
	} else if (!charstring::compareIgnoringCase(type,"SMALLINT")) {
		typenumber=SQL_SMALLINT;
	} else if (!charstring::compareIgnoringCase(type,"INTEGER")) {
		typenumber=SQL_INTEGER;
	} else if (!charstring::compareIgnoringCase(type,"REAL")) {
		typenumber=SQL_REAL;
	} else if (!charstring::compareIgnoringCase(type,"FLOAT")) {
		typenumber=SQL_FLOAT;
	} else if (!charstring::compareIgnoringCase(type,"DOUBLE")) {
		typenumber=SQL_DOUBLE;
	} else if (!charstring::compareIgnoringCase(type,"DATE")) {
		typenumber=SQL_DATE;
	} else if (!charstring::compareIgnoringCase(type,"TIME")) {
		typenumber=SQL_TIME;
	} else if (!charstring::compareIgnoringCase(type,"TIMESTAMP")) {
		typenumber=SQL_TIMESTAMP;
	} else if (!charstring::compareIgnoringCase(type,"BIT")) {
		typenumber=SQL_BIT;
	} else if (!charstring::compareIgnoringCase(type,"TINYINT")) {
		typenumber=SQL_TINYINT;
	} else if (!charstring::compareIgnoringCase(type,"BIGINT")) {
		typenumber=SQL_BIGINT;
	} else if (!charstring::compareIgnoringCase(type,"BINARY")) {
		typenumber=SQL_BINARY;
	} else if (!charstring::compareIgnoringCase(type,"VARBINARY")) {
		typenumber=SQL_VARBINARY;
	} else if (!charstring::compareIgnoringCase(type,"LONGVARBINARY")) {
		typenumber=SQL_LONGVARBINARY;
	} else if (!charstring::compareIgnoringCase(type,"TYPE_DATE")) {
		typenumber=SQL_TYPE_DATE;
	} else if (!charstring::compareIgnoringCase(type,"TYPE_TIME")) {
		typenumber=SQL_TYPE_TIME;
	} else if (!charstring::compareIgnoringCase(type,"TYPE_TIMESTAMP")) {
		typenumber=SQL_TYPE_TIMESTAMP;
	#ifdef SQL_TYPE_UTCDATETIME
	} else if (!charstring::compareIgnoringCase(type,"TYPE_UTCDATETIME")) {
		typenumber=SQL_TYPE_UTCDATETIME;
	#endif
	#ifdef SQL_TYPE_UTCTIME
	} else if (!charstring::compareIgnoringCase(type,"TYPE_UCTTIME")) {
		typenumber=SQL_TYPE_UTCTIME;
	#endif
	} else if (!charstring::compareIgnoringCase(type,"INTERVAL_MONTH")) {
		typenumber=SQL_INTERVAL_MONTH;
	} else if (!charstring::compareIgnoringCase(type,"INTERVAL_YEAR")) {
		typenumber=SQL_INTERVAL_YEAR;
	} else if (!charstring::compareIgnoringCase(
					type,"INTERVAL_YEAR_TO_MONTH")) {
		typenumber=SQL_INTERVAL_YEAR_TO_MONTH;
	} else if (!charstring::compareIgnoringCase(type,"INTERVAL_DAY")) {
		typenumber=SQL_INTERVAL_DAY;
	} else if (!charstring::compareIgnoringCase(type,"INTERVAL_HOUR")) {
		typenumber=SQL_INTERVAL_HOUR;
	} else if (!charstring::compareIgnoringCase(type,"INTERVAL_MINUTE")) {
		typenumber=SQL_INTERVAL_MINUTE;
	} else if (!charstring::compareIgnoringCase(type,"INTERVAL_SECOND")) {
		typenumber=SQL_INTERVAL_SECOND;
	} else if (!charstring::compareIgnoringCase(
					type,"INTERVAL_DAY_TO_HOUR")) {
		typenumber=SQL_INTERVAL_DAY_TO_HOUR;
	} else if (!charstring::compareIgnoringCase(
					type,"INTERVAL_DAY_TO_MINUTE")) {
		typenumber=SQL_INTERVAL_DAY_TO_MINUTE;
	} else if (!charstring::compareIgnoringCase(
					type,"INTERVAL_DAY_TO_SECOND")) {
		typenumber=SQL_INTERVAL_DAY_TO_SECOND;
	} else if (!charstring::compareIgnoringCase(
					type,"INTERVAL_HOUR_TO_MINUTE")) {
		typenumber=SQL_INTERVAL_HOUR_TO_MINUTE;
	} else if (!charstring::compareIgnoringCase(
					type,"INTERVAL_HOUR_TO_SECOND")) {
		typenumber=SQL_INTERVAL_HOUR_TO_SECOND;
	} else if (!charstring::compareIgnoringCase(
					type,"INTERVAL_MINUTE_TO_SECOND")) {
		typenumber=SQL_INTERVAL_MINUTE_TO_SECOND;
	} else if (!charstring::compareIgnoringCase(type,"GUID")) {
		typenumber=SQL_GUID;
	} else if (!charstring::compareIgnoringCase(type,"*")) {
		typenumber=SQL_ALL_TYPES;
	}

	// remap date/time types to the appropriate odbc2/3 type,
	// just in case the client isn't well-behaved
	if (!charstring::compare(odbcversion,"2")) {
		switch (typenumber) {
			case SQL_TYPE_DATE:
				typenumber=SQL_DATE;
				break;
			case SQL_TYPE_TIME:
				typenumber=SQL_TIME;
				break;
			case SQL_TYPE_TIMESTAMP:
				typenumber=SQL_TIMESTAMP;
				break;
		}
	} else {
		switch (typenumber) {
			case SQL_DATE:
				typenumber=SQL_TYPE_DATE;
				break;
			case SQL_TIME:
				typenumber=SQL_TYPE_TIME;
				break;
			case SQL_TIMESTAMP:
				typenumber=SQL_TYPE_TIMESTAMP;
				break;
		}
	}

	// get the type list
	erg=SQLGetTypeInfo(odbccur->stmt,typenumber);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getColumnList(sqlrservercursor *cursor,
					const char *catalog,
					const char *schema,
					const char *table,
					const char *column) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// use defaults for NULL parameters
	if (!schema) {
		schema="";
	}
	if (!table) {
		table="";
	}

	// use % if column was empty
	column=(!charstring::isNullOrEmpty(column))?column:"%";

	// get the column list
	erg=SQLColumns(odbccur->stmt,
			(SQLCHAR *)catalog,SQL_NTS,
			(SQLCHAR *)schema,SQL_NTS,
			(SQLCHAR *)table,SQL_NTS,
			(SQLCHAR *)column,SQL_NTS);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendColumnListColumns() &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getPrimaryKeysList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// use defaults for NULL parameters
	if (!schema) {
		schema="";
	}
	if (!table) {
		table="";
	}

	// get the primary key list
	erg=SQLPrimaryKeys(odbccur->stmt,
			(SQLCHAR *)catalog,SQL_NTS,
			(SQLCHAR *)schema,SQL_NTS,
			(SQLCHAR *)table,SQL_NTS);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getKeyAndIndexList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// use defaults for NULL parameters
	if (!schema) {
		schema="";
	}
	if (!table) {
		table="";
	}

	// get the key and index list
	erg=SQLStatistics(odbccur->stmt,
			(SQLCHAR *)catalog,SQL_NTS,
			(SQLCHAR *)schema,SQL_NTS,
			(SQLCHAR *)table,SQL_NTS,
			SQL_INDEX_UNIQUE,
			SQL_QUICK);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getProcedureList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *procedure) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// use defaults for NULL parameters
	if (!schema) {
		schema="";
	}
	const char	*procname="%";
	if (!charstring::isNullOrEmpty(procedure)) {
		procname=procedure;
	}

	// get the procedure list
	erg=SQLProcedures(odbccur->stmt,
			(SQLCHAR *)catalog,SQL_NTS,
			(SQLCHAR *)schema,SQL_NTS,
			(SQLCHAR *)procname,SQL_NTS);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

bool odbcconnection::getProcedureParameterList(
					sqlrservercursor *cursor,
					const char *catalog,
					const char *schema,
					const char *procedure) {

	odbccursor	*odbccur=(odbccursor *)cursor;

	// allocate the statement handle
	if (!odbccur->allocateStatementHandle()) {
		return false;
	}

	if (getcolumntables) {
		SQLSetStmtAttr(odbccur->stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// initialize column and row counts
	odbccur->initializeColCounts();
	odbccur->initializeRowCounts();

	// Unlike SQLColumns/SQLTables, SQLProcedureColumns reads "" as
	// meaning outside of any catalog/schema, so pass NULL instead.

	// get the column list
	erg=SQLProcedureColumns(odbccur->stmt,
			(SQLCHAR *)catalog,
			charstring::getLength(catalog),
			(SQLCHAR *)schema,
			charstring::getLength(schema),
			(SQLCHAR *)procedure,
			charstring::getLength(procedure),
			(SQLCHAR *)NULL,0);
	bool	retval=(erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);

	// parse the column information
	return (retval)?
		(odbccur->handleColumns(true,true) &&
		odbccur->appendNullColumn()):false;
}

const char *odbcconnection::selectCatalogQuery() {
	// FIXME: this won't work with every database
	return "use %s";
}

char *odbcconnection::getCurrentCatalog() {
	char	*currentdb=new char[256];
	SQLSMALLINT	currentdblen;
	SQLGetInfo(dbc,SQL_DATABASE_NAME,
			(SQLPOINTER)currentdb,
			(SQLSMALLINT)256,
			&currentdblen);
	return currentdb;
}

char *odbcconnection::getCurrentSchema() {
	char	*currentschema=new char[256];
	SQLSMALLINT	currentschemalen;
	SQLGetInfo(dbc,SQL_USER_NAME,
			(SQLPOINTER)currentschema,
			(SQLSMALLINT)256,
			&currentschemalen);
	return currentschema;
}

char *odbcconnection::getCurrentUser() {
	char	*currentuser=new char[256];
	SQLSMALLINT	currentuserlen;
	SQLGetInfo(dbc,SQL_USER_NAME,
			(SQLPOINTER)currentuser,
			(SQLSMALLINT)256,
			&currentuserlen);
	return currentuser;
}

#if (ODBCVER >= 0x0300)
bool odbcconnection::setAutoCommitOn() {
	// FIXME: I'm not sure this is necessary for non-sqlserver/sap/sybase
	cont->closeAllResultSets();
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_ON,
				sizeof(SQLINTEGER));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbcconnection::setAutoCommitOff() {
	// FIXME: I'm not sure this is necessary for non-sqlserver/sap/sybase
	cont->closeAllResultSets();
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_OFF,
				sizeof(SQLINTEGER));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbcconnection::supportsAutoCommit() {
	return true;
}

bool odbcconnection::getDefaultAutoCommit() {
	return true;
}

const char *odbcconnection::beginTransactionQuery() {
	return begintxquery;
}

bool odbcconnection::commit() {
	// FIXME: I'm not sure this is necessary for non-sqlserver/sap/sybase
	cont->closeAllResultSets();
	return (SQLEndTran(SQL_HANDLE_ENV,env,SQL_COMMIT)==SQL_SUCCESS);
}

bool odbcconnection::rollback() {
	// FIXME: I'm not sure this is necessary for non-sqlserver/sap/sybase
	cont->closeAllResultSets();
	return (SQLEndTran(SQL_HANDLE_ENV,env,SQL_ROLLBACK)==SQL_SUCCESS);
}

void odbcconnection::getError(char *errorbuffer,
				uint32_t errorbuffersize,
				uint32_t *errorsize,
				int64_t *errorcode,
				bool *liveconnection) {
	SQLCHAR		state[SQL_SQLSTATE_SIZE+1];
	SQLINTEGER	nativeerrnum;
	SQLSMALLINT	errsize;

	bytestring::zero(state,sizeof(state));

	SQLGetDiagRec(SQL_HANDLE_DBC,dbc,1,state,&nativeerrnum,
				(SQLCHAR *)errorbuffer,errorbuffersize,
				&errsize);

	// set return values
	*errorsize=errsize;
	*errorcode=nativeerrnum;
	*liveconnection=isLiveConnection(state);
}
#endif

bool odbcconnection::isLiveConnection(SQLCHAR *state) {
	// TODO: Gain access to the dbc, and in ODBC 3.5 see if
	// SQL_ATTR_CONNECTION_DEAD is SQL_CD_TRUE.
	return bytestring::compare("08S01",state,5) &&
		bytestring::compare("08003",state,5);
}

const char *odbcconnection::mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat) {

	if (fromformat==toformat) {
		return isolevel;
	}

	// translate "isolevel" to one of ODBC's 4 canonical levels...
	SQLUINTEGER	level=0;
	if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compare(isolevel,
					"TRANSACTION_READ_UNCOMMITTED")) {
			level=SQL_TXN_READ_UNCOMMITTED;
		} else if (!charstring::compare(isolevel,
					"TRANSACTION_READ_COMMITTED")) {
			level=SQL_TXN_READ_COMMITTED;
		} else if (!charstring::compare(isolevel,
					"TRANSACTION_REPEATABLE_READ")) {
			level=SQL_TXN_REPEATABLE_READ;
		} else if (!charstring::compare(isolevel,
					"TRANSACTION_SERIALIZABLE") ||
				!charstring::compare(isolevel,
					"TRANSACTION_SNAPSHOT")) {
			// ODBC has no snapshot level, so fold
			// snapshot into serializable
			level=SQL_TXN_SERIALIZABLE;
		}
	} else {
		if (!charstring::compare(isolevel,
					"SQL_TXN_READ_UNCOMMITTED") ||
			!charstring::compareIgnoringCase(isolevel,
					"READ UNCOMMITTED") ||
			!charstring::compareIgnoringCase(isolevel,
					"READ-UNCOMMITTED") ||
			!charstring::compareIgnoringCase(isolevel,
					"dirty read") ||
			!charstring::compareIgnoringCase(isolevel,"UR") ||
			!charstring::compare(isolevel,"0")) {
			level=SQL_TXN_READ_UNCOMMITTED;
		} else if (!charstring::compare(isolevel,
					"SQL_TXN_READ_COMMITTED") ||
			!charstring::compareIgnoringCase(isolevel,
					"READ COMMITTED") ||
			!charstring::compareIgnoringCase(isolevel,
					"READ-COMMITTED") ||
			!charstring::compareIgnoringCase(isolevel,
				"read committed no record version") ||
			!charstring::compareIgnoringCase(isolevel,
					"read consistency") ||
			!charstring::compareIgnoringCase(isolevel,
					"committed read") ||
			!charstring::compareIgnoringCase(isolevel,"CS") ||
			!charstring::compare(isolevel,"1")) {
			level=SQL_TXN_READ_COMMITTED;
		} else if (!charstring::compare(isolevel,
					"SQL_TXN_REPEATABLE_READ") ||
			!charstring::compareIgnoringCase(isolevel,
					"REPEATABLE READ") ||
			!charstring::compareIgnoringCase(isolevel,
					"REPEATABLE-READ") ||
			!charstring::compareIgnoringCase(isolevel,
					"cursor stability") ||
			!charstring::compareIgnoringCase(isolevel,
					"snapshot") ||
			!charstring::compareIgnoringCase(isolevel,"RS") ||
			!charstring::compare(isolevel,"2")) {
			level=SQL_TXN_REPEATABLE_READ;
		} else if (!charstring::compare(isolevel,
					"SQL_TXN_SERIALIZABLE") ||
			!charstring::compareIgnoringCase(isolevel,
					"SERIALIZABLE") ||
			!charstring::compareIgnoringCase(isolevel,
					"snapshot table stability") ||
			!charstring::compareIgnoringCase(isolevel,"RR") ||
			!charstring::compare(isolevel,"3")) {
			level=SQL_TXN_SERIALIZABLE;
		}
	}

	// bail if we couldn't translate
	if (!level) {
		return isolevel;
	}

	// now translate "level" back to a string
	if (toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		switch (level) {
			case SQL_TXN_READ_UNCOMMITTED:
				return "TRANSACTION_READ_UNCOMMITTED";
			case SQL_TXN_READ_COMMITTED:
				return "TRANSACTION_READ_COMMITTED";
			case SQL_TXN_REPEATABLE_READ:
				return "TRANSACTION_REPEATABLE_READ";
			case SQL_TXN_SERIALIZABLE:
				return "TRANSACTION_SERIALIZABLE";
		}
	} else {
		switch (level) {
			case SQL_TXN_READ_UNCOMMITTED:
				return "SQL_TXN_READ_UNCOMMITTED";
			case SQL_TXN_READ_COMMITTED:
				return "SQL_TXN_READ_COMMITTED";
			case SQL_TXN_REPEATABLE_READ:
				return "SQL_TXN_REPEATABLE_READ";
			case SQL_TXN_SERIALIZABLE:
				return "SQL_TXN_SERIALIZABLE";
		}
	}

	// bail if we couldn't translate "level"
	return isolevel;
}

bool odbcconnection::setIsolationLevel(const char *isolevel) {

	if (charstring::isNullOrEmpty(isolevel)) {
		return false;
	}

	// normalize to the canonical SQL_TXN_* form
	const char	*odbciso=mapIsolationLevel(isolevel,
					SQLRSERVERISOLATIONLEVELFORMAT_NATIVE,
					SQLRSERVERISOLATIONLEVELFORMAT_ODBC);

	// decode it into the ODBC macro
	SQLUINTEGER	level;
	if (!charstring::compare(odbciso,"SQL_TXN_READ_UNCOMMITTED")) {
		level=SQL_TXN_READ_UNCOMMITTED;
	} else if (!charstring::compare(odbciso,"SQL_TXN_READ_COMMITTED")) {
		level=SQL_TXN_READ_COMMITTED;
	} else if (!charstring::compare(odbciso,"SQL_TXN_REPEATABLE_READ")) {
		level=SQL_TXN_REPEATABLE_READ;
	} else if (!charstring::compare(odbciso,"SQL_TXN_SERIALIZABLE")) {
		level=SQL_TXN_SERIALIZABLE;
	} else {
		// unrecognized
		return false;
	}

	#if (ODBCVER >= 0x0300)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
				(SQLPOINTER)(uintptr_t)level,0);
	#else
	erg=SQLSetConnectOption(dbc,SQL_TXN_ISOLATION,level);
	#endif
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

const char *odbcconnection::getDbHostNameQuery() {
	// FIXME: only works with MS SQL Server
	return "SELECT cast(@@SERVERNAME as varchar(64))";
}

const char *odbcconnection::getDbIpAddressQuery() {
	// FIXME: only works with MS SQL Server
	return "SELECT CAST(SERVERPROPERTY('ComputerNamePhysicalNetBIOS') as varchar(64))";
}

odbccursor::odbccursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {
	odbcconn=(odbcconnection *)conn;
	stmt=NULL;
	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	indatebind=new SQL_DATE_STRUCT[maxbindcount];
	intimebind=new SQL_TIME_STRUCT[maxbindcount];
	intsbind=new SQL_TIMESTAMP_STRUCT[maxbindcount];
	outdatebind=new datebind *[maxbindcount];
	outcharbind=new charbind *[maxbindcount];
	outisnullptr=new int16_t *[maxbindcount];
	inoutdatebind=new datebind *[maxbindcount];
	inoutcharbind=new charbind *[maxbindcount];
	inoutisnullptr=new int16_t *[maxbindcount];
	#ifdef SQLBINDPARAMETER_SQLLEN
	outisnull=new SQLLEN[maxbindcount];
	inoutisnull=new SQLLEN[maxbindcount];
	inbindlength=new SQLLEN[maxbindcount];
	#else
	outisnull=new SQLINTEGER[maxbindcount];
	inoutisnull=new SQLINTEGER[maxbindcount];
	inbindlength=new SQLINTEGER[maxbindcount];
	#endif
	nullbindisbinary=new bool[maxbindcount];
	nullbinddescribed=new bool[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		outdatebind[i]=NULL;
		outcharbind[i]=NULL;
		outisnullptr[i]=NULL;
		outisnull[i]=0;
		inoutdatebind[i]=NULL;
		inoutcharbind[i]=NULL;
		inoutisnullptr[i]=NULL;
		inoutisnull[i]=0;
		inbindlength[i]=0;
		nullbindisbinary[i]=false;
		nullbinddescribed[i]=false;
	}
	sqlnulldata=SQL_NULL_DATA;
	bindformaterror=false;
	cachedrows.setManageArrayValues(true);
	currentcachedrow=NULL;
	cachedrowsarecomplete=false;
	resultsetsdrained=false;
	#ifdef HAVE_SQLCONNECTW
	ucsinbindstrings.setManageArrayValues(true);
	#endif
	allocateResultSetBuffers(conn->cont->getMaxColumnCount());
	initializeColCounts();
	initializeRowCounts();
}

odbccursor::~odbccursor() {
	clearCachedRows();
	delete[] indatebind;
	delete[] intimebind;
	delete[] intsbind;
	delete[] outdatebind;
	delete[] outcharbind;
	delete[] outisnullptr;
	delete[] outisnull;
	delete[] inoutdatebind;
	delete[] inoutcharbind;
	delete[] inoutisnullptr;
	delete[] inoutisnull;
	delete[] inbindlength;
	delete[] nullbindisbinary;
	delete[] nullbinddescribed;
	#ifdef HAVE_SQLCONNECTW
	ucsinbindstrings.clear();
	#endif
	deallocateResultSetBuffers();
}

void odbccursor::allocateResultSetBuffers(int32_t columncount) {

	if (!columncount) {
		this->columncount=0;
		field=NULL;
		loblength=NULL;
		indicator=NULL;
		column=NULL;
	} else {
		this->columncount=columncount;
		field=new char *[columncount];
		#ifdef SQLBINDCOL_SQLLEN
		loblength=new SQLLEN[columncount];
		indicator=new SQLLEN[columncount];
		#else
		loblength=new SQLINTEGER[columncount];
		indicator=new SQLINTEGER[columncount];
		#endif
		uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();
		column=new odbccolumn[columncount];
		for (int32_t i=0; i<columncount; i++) {
			field[i]=new char[maxfieldsize];
		}
	}
}

void odbccursor::deallocateResultSetBuffers() {
	if (columncount) {
		for (int32_t i=0; i<columncount; i++) {
			delete[] field[i];
		}
		delete[] column;
		delete[] field;
		delete[] loblength;
		delete[] indicator;
		columncount=0;
	}
}

bool odbccursor::prepareQuery(const char *query, uint32_t size) {

	bindformaterror=false;

	// initialize column count
	initializeColCounts();

	// allocate the statement handle
	if (!allocateStatementHandle()) {
		return false;
	}

	if (odbcconn->getcolumntables && !getExecuteDirect()) {

		// MS SQL Server only returns column table names when using a
		// server cursor or when the query contains a FOR BROWSE clause.
		//
		// Some apps need the table name.
		//
		// Setting the cursor type to static appears to be the least
		// invasive way of influencing the server to use a server cursor
		// and thus return column names.
		//
		// (see more below)
		SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
				(SQLPOINTER)SQL_CURSOR_STATIC,
				SQL_IS_INTEGER);
	}

	// a new statement means new parameters, so what the driver said
	// about the old ones doesn't apply
	for (uint16_t i=0; i<maxbindcount; i++) {
		nullbindisbinary[i]=false;
		nullbinddescribed[i]=false;
	}

	// prepare the query...

	#ifdef HAVE_SQLCONNECTW
	if (odbcconn->unicode) {

		ucsinbindstrings.clear();

		if (getExecuteDirect()) {
			return true;
		}

		char	*err=NULL;
		byte_t	*queryucs=convertCharset((const byte_t *)query,
							size,
							"UTF-8",
							"UCS-2//TRANSLIT",
							&err);
		if (err) {
			delete[] queryucs;
			setConvCharError("prepare query",err);
			return false;
		}
		erg=SQLPrepareW(stmt,(SQLWCHAR *)queryucs,SQL_NTS);
		delete[] queryucs;
	} else {
	#endif
		if (getExecuteDirect()) {
			return true;
		}
		erg=SQLPrepare(stmt,(SQLCHAR *)query,size);
	#ifdef HAVE_SQLCONNECTW
	}
	#endif

	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		return false;
	}

	if (!handleColumns(true,false)) {
		return false;
	}

	if (odbcconn->getcolumntables) {

		// (continued from above)
		//
		// If we want column table names then we had to do something
		// like use a static cursor above.  However, if we actually
		// execute with a static cursor, then performance is really
		// bad if there are a decent number of rows.
		//
		// To work around, we'll grab the column info, reallocate the
		// statment handle, letting its cursor type default to a
		// forward-only cursor, and re-prepare it.
		//
		// This is generally faster than fetching from a static cursor.
		// It won't be for complex queries that return small result
		// sets, but we'll hope that isn't the case.
		//
		// Arguably this should be controlled by a directive on a
		// query-by-query basis like execute-direct is.
		if (!allocateStatementHandle()) {
			return false;
		}

		#ifdef HAVE_SQLCONNECTW
		if (odbcconn->unicode) {

			ucsinbindstrings.clear();

			byte_t *queryucs=convertCharset((const byte_t *)query,
							size,
							"UTF-8",
							"UCS-2//TRANSLIT",
							NULL);
			erg=SQLPrepareW(stmt,(SQLWCHAR *)queryucs,SQL_NTS);
			delete[] queryucs;
		} else {
		#endif
			erg=SQLPrepare(stmt,(SQLCHAR *)query,size);
		#ifdef HAVE_SQLCONNECTW
		}
		#endif
	}

	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::allocateStatementHandle() {

	if (stmt) {
		#if (ODBCVER >= 0x0300)
		SQLFreeHandle(SQL_HANDLE_STMT,stmt);
		#else
		SQLFreeStmt(stmt,SQL_DROP);
		#endif
		stmt=NULL;
	}

	#if (ODBCVER >= 0x0300)
	erg=SQLAllocHandle(SQL_HANDLE_STMT,odbcconn->dbc,&stmt);
	#else
	erg=SQLAllocStmt(odbcconn->dbc,&stmt);
	#endif
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

// Whether the parameter at pos is aimed at a binary column.  Only a null
// bind needs to ask - every other bind knows its own type - and only on a
// back end that won't implicitly convert a character parameter to a binary
// one.  Answering false is always safe: it keeps the character bind that has
// been the only behaviour up to now.
bool odbccursor::nullBindIsBinary(uint16_t pos) {

	if (!odbcconn->describenullbinds || !odbcconn->hasdescribeparam) {
		return false;
	}

	// SQLDescribeParam needs a prepared statement, and execute-direct
	// never prepares one
	if (getExecuteDirect()) {
		return false;
	}

	if (nullbinddescribed[pos-1]) {
		return nullbindisbinary[pos-1];
	}

	// A failed describe is the ordinary case for a parameter whose type
	// the back end can't deduce - "select ?" is the obvious one - so it
	// isn't an error, just an answer of "no".  The diagnostic it leaves
	// on stmt is cleared by the SQLBindParameter that follows.
	SQLSMALLINT	datatype=0;
	#ifdef SQLBINDPARAMETER_SQLLEN
	SQLULEN		parametersize=0;
	#else
	SQLUINTEGER	parametersize=0;
	#endif
	SQLSMALLINT	decimaldigits=0;
	SQLSMALLINT	nullable=0;
	SQLRETURN	erg=SQLDescribeParam(stmt,pos,&datatype,&parametersize,
						&decimaldigits,&nullable);

	nullbinddescribed[pos-1]=true;
	nullbindisbinary[pos-1]=((erg==SQL_SUCCESS ||
					erg==SQL_SUCCESS_WITH_INFO) &&
				(datatype==SQL_BINARY ||
					datatype==SQL_VARBINARY ||
					datatype==SQL_LONGVARBINARY));

	return nullbindisbinary[pos-1];
}

bool odbccursor::inputBind(const char *variable,
				uint16_t variablesize,
				const char *value,
				uint32_t valuesize,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	if (*isnull==SQL_NULL_DATA) {

		// A null bind is generic - one SQLBindParameter call serves
		// every bind type and every column type, because the client
		// api flattens a null lob bind into a plain null bind before
		// it reaches the wire.  So the only way to tell whether this
		// one is aimed at a binary column is to ask the driver.
		SQLSMALLINT	paramtype=SQL_VARCHAR;
		if (nullBindIsBinary(pos)) {

			// SQL_VARBINARY rather than whatever the driver
			// actually said - a ColumnSize of 0 is allowed with
			// SQL_VARBINARY but not SQL_LONGVARBINARY (see #975),
			// and this is the same call that
			// odbccursor::inputBindBlob()'s null arm already
			// makes, which is known to reach binary, varbinary
			// and image alike.
			paramtype=SQL_VARBINARY;
		}

		// We used to do something like this to bind a NULL:
		//
		//	erg=SQLBindParameter(stmt,
		//			pos,
		//			SQL_PARAM_INPUT,
		//			SQL_C_BINARY,
		//			SQL_CHAR,
		//			1,		// in characters
		//			0,
		//			val,
		//			buffersize,	// in bytes
		//			&sqlnulldata);
		//
		// (code to set val and buffersize used to be above this if)
		// (see #975 and the next "see #975" comment below
		// for why there was a 1 for the 6th (ColumnSize) parameter)
		//
		// However, with ODBC Driver 17 for SQL Server (and possibly
		// other versions, and other drivers) the above fails.
		//
		// It's not exactly clear what fails.  It's experimentally
		// verifiable that, in this case, val="" and buffersize=0.
		// Somehow though, val gets interpreted as "*".
		//
		// It's also seemingly random what works.  Eg.
		//
		// This works:
		//
		//	select case when :foo is null then 'foo-null'
		//	else 'foo-not-null' end as foo;
		//
		// It returns 'foo-null'
		//
		// However, this fails:
		// 
		//	select isnull(:foo, 99) as foo;
		//
		// It returns '*'
		//
		// Similarly, this also fails:
		// 
		//	select cast(isnull(:foo, 99) as int) as foo;
		//
		// It throws "Conversion failed when converting the varchar"
		// "value '*' to data type int." as that '*' can't be converted
		// to an int.
		// 	
		// It's strange.
		//
		// Setting the ColumnSize to 0 seems like an intuitive fix, but
		// if ParameterType is SQL_CHAR, then we can't do that (see the
		// "see #975" comment below for more detail on that).
		//
		// However, if we use a ParameterType of SQL_VARCHAR, then
		// we can set ParameterValuePtr to NULL, ColumnSize to 0, and
		// BufferLength to 0, and then it works.  Apparently a
		// ColumnSize of 0 is allowed with SQL_VARCHARs and
		// SQL_VARBINARYs, but not SQL_LONGVARCHARs or
		// SQL_LONGVARBINARYs as it turns out.
		//
		// see #6232 for more detail
		//
		//
		// NOTE: the 4th (ValueType) parameter must be
		// SQL_C_BINARY (as opposed to SQL_C_WCHAR or SQL_C_CHAR)
		// for this to work with blobs
		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_BINARY,
				paramtype,
				0,		// in characters
				0,
				NULL,
				0,		// in bytes
				&sqlnulldata);

	} else {

		SQLPOINTER	val=NULL;
		SQLSMALLINT	valtype=SQL_C_CHAR;
		SQLSMALLINT	paramtype=SQL_CHAR;
		SQLLEN		buffersize=valuesize;
		#ifdef HAVE_SQLCONNECTW
		if (odbcconn->unicode) {

			const char	*encoding=odbcconn->ncharencoding;
			char	*err=NULL;
			byte_t	*valueucs=convertCharset(
						(const byte_t *)value,
						valuesize,
						"UTF-8",encoding,
						&err);
			if (err) {
				delete[] valueucs;
				setConvCharError("input bind",err);
				return false;
			}
			valuesize=len(valueucs,encoding);
			buffersize=stringSize(valueucs,encoding);
			ucsinbindstrings.append(valueucs);
			val=(SQLPOINTER)valueucs;
			valtype=SQL_C_WCHAR;
			paramtype=SQL_WVARCHAR;
		} else {
		#endif
			val=(SQLPOINTER)value;
		#ifdef HAVE_SQLCONNECTW
		}
		#endif

		if (!valuesize) {
			// see #975
			// When binding an empty string, it's intuitive to set
			// ColumnSize to 0, as ParameterValuePtr is "" and, as
			// such, contains 0 characters.  This works with most
			// drivers.
			//
			// However, with SQL Server Native Client ODBC Driver
			// version 11.0 (and possibly later versions)...
			//
			// In ODBC-2 mode, it allows a ColumnSize of 0, but in
			// non-ODBC-2 mode, it throws: "Invalid precision value"
			//
			// Setting ColumnSize to 1 appears to work around this,
			// and works in all ODBC-modes.  BufferLength appears
			// to override ColumnSize, so it's ok that the string
			// is actually 0 characters long, even though we declare
			// it to be 1.
			//
			// This workaound doesn't appear to break any other
			// ODBC drivers either, so we'll go with it for now.
			//
			// NOTE: per the "see #6232" comment above...
			// ColumnSize of 0 may only be invalid for a
			// ParameterType of SQL_CHAR.  Using SQL_VARCHAR
			// instead might be another solution to this problem.
			valuesize=1;
		} else if (odbcconn->maxallowedvarcharbindsize &&
			valuesize>odbcconn->maxallowedvarcharbindsize) {
			valuesize=odbcconn->maxvarcharbindsize;
		}

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				valtype,
				paramtype,
				valuesize,	// in characters
				0,
				val,
				buffersize,	// in bytes
				NULL);
	}
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_SBIGINT,
				SQL_BIGINT,
				0,
				0,
				value,
				sizeof(int64_t),
				NULL);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t precision,
				uint32_t scale) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_DOUBLE,
				SQL_DECIMAL,
				precision,
				scale,
				value,
				sizeof(double),
				NULL);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputBind(const char *variable,
				uint16_t variablesize,
				int64_t year,
				int16_t month,
				int16_t day,
				int16_t hour,
				int16_t minute,
				int16_t second,
				int32_t microsecond,
				const char *tz,
				bool isnegative,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	bool	validdate=(year>=0 && month>=0 && day>=0);
	bool	validtime=(hour>=0 && minute>=0 && second>=0 && microsecond>=0);

	if (validdate && !validtime) {

		SQL_DATE_STRUCT	*ts=&(indatebind[pos-1]);
		ts->year=year;
		ts->month=month;
		ts->day=day;

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_DATE,
				SQL_DATE,
				0,
				0,
				ts,
				0,
				NULL);

	} else if (!validdate && validtime && !odbcconn->timestampfortime) {

		SQL_TIME_STRUCT	*ts=&(intimebind[pos-1]);
		ts->hour=hour;
		ts->minute=minute;
		ts->second=second;

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_TIME,
				SQL_TIME,
				0,
				odbcconn->fractionscale,
				ts,
				0,
				NULL);

	} else {

		SQL_TIMESTAMP_STRUCT	*ts=&(intsbind[pos-1]);
		ts->year=year;
		ts->month=month;
		ts->day=day;
		ts->hour=hour;
		ts->minute=minute;
		ts->second=second;
		if (odbcconn->supportsfraction) {
			if (odbcconn->fractionscale==9) {
				ts->fraction=microsecond*1000;
			} else if (odbcconn->fractionscale==6) {
				ts->fraction=microsecond;
			}
		} else {
			ts->fraction=0;
		}

		// FIXME: this works with the SQL Server Native Client ODBC
		// drivers, but not the old "standard" SQL Server driver
		// (seconds and fractional seconds are truncated).  The web is
		// riddled with people trying to get it to work with the old
		// driver.  None of their solutions worked for me.  There is
		// probably some magic that will work.  I'll have to find it
		// some day.
		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_TIMESTAMP,
				SQL_TIMESTAMP,
				0,
				odbcconn->fractionscale,
				ts,
				0,
				NULL);
	}
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputBindBlob(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						int16_t *isnull) {

	// fall back to the character bind for databases whose drivers
	// can't take a binary bind
	if (odbcconn->usecharforlobbind) {
		return sqlrservercursor::inputBindBlob(
						variable,
						variablesize,
						value,
						valuesize,
						isnull);
	}

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	if (*isnull==SQL_NULL_DATA) {

		// see the "see #6232" comment in inputBind() for why a
		// ColumnSize of 0 works here
		inbindlength[pos-1]=SQL_NULL_DATA;

		erg=SQLBindParameter(stmt,
					pos,
					SQL_PARAM_INPUT,
					SQL_C_BINARY,
					SQL_VARBINARY,
					0,		// in bytes
					0,
					NULL,
					0,		// in bytes
					&(inbindlength[pos-1]));

		return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
	}

	// StrLen_or_IndPtr has to be passed for a binary bind.  If it's
	// NULL then the driver looks for a null terminator to find the end
	// of the value, and a binary value can contain nulls of its own.
	// (Passing NULL here used to truncate any value containing a 0
	// byte at the first one.)
	inbindlength[pos-1]=valuesize;

	// SQL Server rejects a ColumnSize over 8000 for a SQL_VARBINARY,
	// and Teradata rejects a SQL_LONGVARBINARY for a byte column
	// that's part of an index, so use the narrower type where it fits.
	SQLSMALLINT	paramtype=(valuesize>8000)?
					SQL_LONGVARBINARY:SQL_VARBINARY;

	// a ColumnSize of 0 is invalid here - see "see #975" in inputBind()
	uint32_t	columnsize=(valuesize)?valuesize:1;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_BINARY,
				paramtype,
				columnsize,	// in bytes
				0,
				(SQLPOINTER)value,
				valuesize,	// in bytes
				&(inbindlength[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::outputBind(const char *variable, 
				uint16_t variablesize,
				char *value, 
				uint32_t valuesize, 
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	SQLLEN		buffersize=valuesize;

	if (odbcconn->maxallowedvarcharbindsize &&
		valuesize>odbcconn->maxallowedvarcharbindsize) {
		valuesize=odbcconn->maxvarcharbindsize;
		// FIXME: should buffersize also be reduced here
	}

	charbind	*cb=new charbind;
	cb->value=value;
	// This has to be buffersize.  The reduction above turns valuesize into
	// a column size in characters, but the post-execute conversion needs
	// the size of the buffer in bytes.
	cb->valuesize=(uint32_t)buffersize;

	outdatebind[pos-1]=NULL;
	outcharbind[pos-1]=cb;
	outisnullptr[pos-1]=isnull;

	#ifdef HAVE_SQLCONNECTW
	if (odbcconn->unicode) {

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_WCHAR,
				SQL_WVARCHAR,
				valuesize,		// in characters
				0,
				(SQLPOINTER)value,
				buffersize,		// in bytes
				&(outisnull[pos-1]));

	} else {
	#endif

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_CHAR,
				SQL_VARCHAR,
				valuesize,		// in characters
				0,
				(SQLPOINTER)value,
				buffersize,		// in bytes
				&(outisnull[pos-1]));

	#ifdef HAVE_SQLCONNECTW
	}
	#endif

	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::outputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	outdatebind[pos-1]=NULL;
	outcharbind[pos-1]=NULL;
	outisnullptr[pos-1]=isnull;

	*value=0;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_SBIGINT,
				SQL_BIGINT,
				0,
				0,
				value,
				sizeof(int64_t),
				&(outisnull[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::outputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t *precision,
				uint32_t *scale,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	outdatebind[pos-1]=NULL;
	outcharbind[pos-1]=NULL;
	outisnullptr[pos-1]=isnull;

	*value=0.0;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_DOUBLE,
				SQL_DOUBLE,
				0,
				0,
				value,
				sizeof(double),
				&(outisnull[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::outputBind(const char *variable,
				uint16_t variablesize,
				int16_t *year,
				int16_t *month,
				int16_t *day,
				int16_t *hour,
				int16_t *minute,
				int16_t *second,
				int32_t *microsecond,
				const char **tz,
				bool *isnegative,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	datebind	*db=new datebind;
	db->year=year;
	db->month=month;
	db->day=day;
	db->hour=hour;
	db->minute=minute;
	db->second=second;
	db->microsecond=microsecond;
	db->tz=tz;

	*isnegative=false;

	outdatebind[pos-1]=db;
	outcharbind[pos-1]=NULL;
	outisnullptr[pos-1]=isnull;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_TIMESTAMP,
				SQL_TIMESTAMP,
				// FIXME: shouldn't these be 29,9
				// like an input/output bind?
				0,
				0,
				&(db->buffer),
				0,
				&(outisnull[pos-1])
				);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputOutputBind(const char *variable, 
				uint16_t variablesize,
				char *value, 
				uint32_t valuesize, 
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	SQLSMALLINT	valtype=SQL_C_CHAR;
	SQLSMALLINT	paramtype=SQL_CHAR;
	SQLLEN		buffersize=valuesize;
	// Bytes of valid data already sitting in the bind buffer, in
	// whichever encoding actually gets bound below.  Used to set the
	// StrLen_or_IndPtr so the driver sees the whole input value.
	size_t		sizetocopy=charstring::getLength(value);
	// Scratch buffer for the unicode case, sized to hold the
	// converted value (and to give the driver as much room to write
	// output back as the caller's buffer implies).  Left NULL, and
	// "value" bound directly, for the non-unicode case.
	byte_t		*ucsvalue=NULL;
	SQLLEN		ucsvaluesize=0;
	#ifdef HAVE_SQLCONNECTW
	if (odbcconn->unicode) {

		const char	*encoding=odbcconn->ncharencoding;
		char	*err=NULL;
		byte_t	*valueucs=convertCharset(
				(const byte_t *)value,
				stringSize((const byte_t *)value,"UTF-8"),
				"UTF-8",encoding,
				&err);
		if (err) {
			delete[] valueucs;
			setConvCharError("input-output bind",err);
			return false;
		}
		// stringSize() already counts the null terminator
		sizetocopy=stringSize(valueucs,encoding);

		// Convert into a scratch buffer instead of truncating the
		// converted value back into "value".  "value" was sized
		// for the utf-8 form; ucs-2 needs up to twice that, so
		// reusing it silently cut the value down to the utf-8
		// size.  Size the scratch buffer to the larger of the
		// converted value and double the caller's buffer, so it
		// has room for both the converted input and any output
		// the driver writes back.
		ucsvaluesize=(SQLLEN)((size_t)valuesize*2);
		if (ucsvaluesize<(SQLLEN)sizetocopy) {
			ucsvaluesize=(SQLLEN)sizetocopy;
		}
		ucsvalue=new byte_t[ucsvaluesize];
		bytestring::copy(ucsvalue,valueucs,sizetocopy);

		delete[] valueucs;
		valtype=SQL_C_WCHAR;
		paramtype=SQL_WVARCHAR;
	}
	#endif

	charbind	*cb=new charbind;
	cb->value=value;
	cb->valuesize=valuesize;
	cb->ucsvalue=ucsvalue;

	inoutdatebind[pos-1]=NULL;
	inoutcharbind[pos-1]=cb;
	inoutisnullptr[pos-1]=isnull;

	inoutisnull[pos-1]=(*isnull==SQL_NULL_DATA)?
				sqlnulldata:(SQLLEN)sizetocopy;

	// FIXME: original code...
	/*erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT_OUTPUT,
				SQL_C_CHAR,
				SQL_VARCHAR,
				valuesize,
				0,
				(SQLPOINTER)value,
				valuesize,
				&(inoutisnull[pos-1]));*/

	if (*isnull==SQL_NULL_DATA) {
		// the 4th parameter (ValueType) must by
		// SQL_C_BINARY (as opposed to SQL_C_WCHAR or SQL_C_CHAR)
		// for this to work with blobs
		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT_OUTPUT,
				SQL_C_BINARY,
				SQL_CHAR,
				1,
				0,
				(SQLPOINTER)value,
				buffersize,
				&(inoutisnull[pos-1]));
	} else {

		if (!valuesize) {
			// In ODBC-2 mode, SQL Server Native Client 11.0
			// (at least) allows a valuesize of 0, when the value
			// is "".  In non-ODBC-2 mode, it throws:
			// "Invalid precision value" Using a valuesize of 1
			// works with all ODBC-modes.  Hopefully it works with
			// all drivers.
			valuesize=1;
		} else if (odbcconn->maxallowedvarcharbindsize &&
			valuesize>odbcconn->maxallowedvarcharbindsize) {
			valuesize=odbcconn->maxvarcharbindsize;
			// FIXME: should buffersize also be reduced here
		}

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT_OUTPUT,
				valtype,
				paramtype,
				valuesize,	// in characters
				0,
				(ucsvalue)?
					(SQLPOINTER)ucsvalue:
					(SQLPOINTER)value,
				(ucsvalue)?ucsvaluesize:buffersize,
				&(inoutisnull[pos-1]));
	}
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputOutputBind(const char *variable, 
				uint16_t variablesize,
				int64_t *value,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	inoutdatebind[pos-1]=NULL;
	inoutcharbind[pos-1]=NULL;
	inoutisnullptr[pos-1]=isnull;

	inoutisnull[pos-1]=(*isnull==SQL_NULL_DATA)?
				sqlnulldata:sizeof(int64_t);

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT_OUTPUT,
				SQL_C_SBIGINT,
				SQL_BIGINT,
				0,
				0,
				value,
				sizeof(int64_t),
				&(inoutisnull[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputOutputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t *precision,
				uint32_t *scale,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	inoutdatebind[pos-1]=NULL;
	inoutcharbind[pos-1]=NULL;
	inoutisnullptr[pos-1]=isnull;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT_OUTPUT,
				SQL_C_DOUBLE,
				SQL_DOUBLE,
				*precision,
				*scale,
				value,
				sizeof(double),
				&(outisnull[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool odbccursor::inputOutputBind(const char *variable,
				uint16_t variablesize,
				int16_t *year,
				int16_t *month,
				int16_t *day,
				int16_t *hour,
				int16_t *minute,
				int16_t *second,
				int32_t *microsecond,
				const char **tz,
				bool *isnegative,
				int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	datebind	*db=new datebind;
	db->year=year;
	db->month=month;
	db->day=day;
	db->hour=hour;
	db->minute=minute;
	db->second=second;
	db->microsecond=microsecond;
	db->tz=tz;

	db->buffer.year=*year;
	db->buffer.month=*month;
	db->buffer.day=*day;
	db->buffer.hour=*hour;
	db->buffer.minute=*minute;
	db->buffer.second=*second;
	db->buffer.fraction=(*microsecond)*1000;

	*isnegative=false;

	inoutdatebind[pos-1]=db;
	inoutcharbind[pos-1]=NULL;
	inoutisnullptr[pos-1]=isnull;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT_OUTPUT,
				SQL_C_TIMESTAMP,
				SQL_TIMESTAMP,
				29,
				9,
				&(db->buffer),
				0,
				&(outisnull[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

int16_t odbccursor::getNonNullBindValue() {
	return 0;
}

int16_t odbccursor::getNullBindValue() {
	return SQL_NULL_DATA;
}

bool odbccursor::executeQuery(const char *query, uint32_t size) {

	// initialize counts
	initializeRowCounts();

	// query timeout is an odbc-driver level read timeout but with
	// special cleanup handling in better drivers to tell the server
	// to stop executing the query after the client read timeout...
	// * init from query timeout specified in the connect string
	// * override with query timeout set via a directive
	// * if it's still > 0 then actually set the timeout
	uint64_t	statementquerytimeout=conn->cont->getQueryTimeout();
	if (getQueryTimeout()>0) {
		statementquerytimeout=getQueryTimeout();
	}
	if (statementquerytimeout>0) {
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_QUERY_TIMEOUT,
					(SQLPOINTER)statementquerytimeout,
					SQL_IS_UINTEGER);
		// FIXME: do we care if this fails?
	}

	// execute the query
	if (getExecuteDirect()) {
		#ifdef HAVE_SQLCONNECTW
		if (odbcconn->unicode) {
			char	*err=NULL;
			byte_t	*queryucs=convertCharset((const byte_t *)query,
							size,
							"UTF-8",
							"UCS-2//TRANSLIT",
							&err);
			if (err) {
				delete[] queryucs;
				setConvCharError("execute query",err);
				return false;
			}
			erg=SQLExecDirectW(stmt,(SQLWCHAR *)queryucs,SQL_NTS);
			delete[] queryucs;
		} else {
		#endif
			erg=SQLExecDirect(stmt,(SQLCHAR *)query,size);
		#ifdef HAVE_SQLCONNECTW
		}
		#endif
	} else {
		erg=SQLExecute(stmt);
	}

	#ifdef HAVE_SQLCONNECTW
	// free buffers used to convert string-binds to unicode
	ucsinbindstrings.clear();
	#endif

	if (erg!=SQL_SUCCESS &&
			erg!=SQL_SUCCESS_WITH_INFO
			#if defined(SQL_NO_DATA)
			&& erg!=SQL_NO_DATA
			#elif defined(SQL_NO_DATA_FOUND)
			&& erg!=SQL_NO_DATA_FOUND
			#endif
		) {
		return false;
	}

	checkForTempTable(query,size);

	// if we're not exec-direct'ing, and if column info is valid after
	// prepare, then we must have already done the first half of this in
	// prepareQuery()
	if (!handleColumns(getExecuteDirect() ||
			!columninfoisvalidafterprepare,true)) {
		return false;
	}

	// get the row count
	erg=SQLRowCount(stmt,&affectedrows);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		return false;
	}

	// Data isn't written to the output bind buffers (at least with the
	// Microsoft ODBC Driver) until SQLMoreResults() returns SQL_NO_DATA.
	// If the query returned rows, then they have to be buffered before
	// the result sets can be drained, or draining would throw them away.
	if (!cacheRowsAndDrainResultSets()) {
		return false;
	}

	// convert date output binds and copy out isnulls
	//for (uint16_t i=0; i<getOutputBindCount(); i++) {
	// FIXME: inefficient
	for (uint16_t i=0; i<maxbindcount; i++) {
		if (outdatebind[i]) {
			datebind	*db=outdatebind[i];
			*(db->year)=db->buffer.year;
			*(db->month)=db->buffer.month;
			*(db->day)=db->buffer.day;
			*(db->hour)=db->buffer.hour;
			*(db->minute)=db->buffer.minute;
			*(db->second)=db->buffer.second;
			*(db->microsecond)=db->buffer.fraction/1000;
			*(db->tz)=NULL;
		}
		#ifdef HAVE_SQLCONNECTW
		if (odbcconn->unicode && outcharbind[i]) {
			// convert wchar output binds to user coding
			char		*value=outcharbind[i]->value;
			uint32_t	valuesize=outcharbind[i]->valuesize;
			char		*err=NULL;
			byte_t		*u=convertCharset(
						(const byte_t *)value,
						odbcconn->ncharencoding,
						"UTF-8",&err);
			if (err) {
				delete[] u;
				setConvCharError("output bind",err);
				return false;
			}
			// clamp to the bind buffer
			size_t	nullsize=nullSize("UTF-8");
			size_t	s=stringSize(u,"UTF-8");
			if (s>valuesize) {
				s=valuesize;
			}

			// stringSize() counts the null terminator
			s=(s>nullsize)?s-nullsize:0;

			// copy in and null-terminate
			bytestring::copy(value,u,s);
			if (s+nullsize<=valuesize) {
				bytestring::zero(value+s,nullsize);
			}
			delete[] u;
		}
		#endif
		if (outisnullptr[i]) {
			*(outisnullptr[i])=outisnull[i];
			// FIXME: get these to work
			/*if (outisnull[i]==SQL_NO_TOTAL) {
				// This is most likely caused by the fact that
				// we should be using SQL_C_WCHAR and SQL_WCHAR
				// instead of forcing ODBC to do the conversion
				// for us.   In a work-around we just kludge
				// away the space with padding.
				// Work-around in SQL: return only varchar, not 
				// varchar.
				char	*valuep=sb->value;
				for (int k=(sb->BufferLength-2);
					k>=0 && valuep[k]==' ';k--) {
					valuep[k]=0;
				}
			} else if (outisnull[i]>=0 &&
					outisnull[i]<sb->BufferLength) {
				// forcibly null-terminate the buffer
				sb->value[outisnull[i]]=0;
			}*/
		}
	}
	//for (uint16_t i=0; i<getInputOutputBindCount(); i++) {
	// FIXME: inefficient
	for (uint16_t i=0; i<maxbindcount; i++) {
		if (inoutdatebind[i]) {
			datebind	*db=inoutdatebind[i];
			*(db->year)=db->buffer.year;
			*(db->month)=db->buffer.month;
			*(db->day)=db->buffer.day;
			*(db->hour)=db->buffer.hour;
			*(db->minute)=db->buffer.minute;
			*(db->second)=db->buffer.second;
			*(db->microsecond)=db->buffer.fraction/1000;
			*(db->tz)=NULL;
		}
		#ifdef HAVE_SQLCONNECTW
		if (odbcconn->unicode && inoutcharbind[i]) {
			// convert wchar output binds to user coding
			char		*value=inoutcharbind[i]->value;
			uint32_t	valuesize=inoutcharbind[i]->valuesize;
			// the driver wrote its output into the scratch
			// buffer (if one was allocated for the bind),
			// not into the undersized caller buffer
			byte_t		*ucsvalue=inoutcharbind[i]->ucsvalue;
			char		*err=NULL;
			byte_t		*u=convertCharset(
						(ucsvalue)?
							ucsvalue:
							(const byte_t *)value,
						odbcconn->ncharencoding,
						"UTF-8",&err);
			if (err) {
				delete[] u;
				setConvCharError("input-output bind",err);
				return false;
			}
			// clamp to the bind buffer
			size_t	nullsize=nullSize("UTF-8");
			size_t	s=stringSize(u,"UTF-8");
			if (s>valuesize) {
				s=valuesize;
			}

			// stringSize() counts the null terminator
			s=(s>nullsize)?s-nullsize:0;

			// copy in and null-terminate
			bytestring::copy(value,u,s);
			if (s+nullsize<=valuesize) {
				bytestring::zero(value+s,nullsize);
			}
			delete[] u;
		}
		#endif
		if (inoutisnullptr[i]) {
			*(inoutisnullptr[i])=inoutisnull[i];
		}
	}

	return true;
}

void odbccursor::initializeColCounts() {
	ncols=0;
	columninfoisvalidafterprepare=true;
}

void odbccursor::initializeRowCounts() {
	row=0;
	maxrow=0;
	totalrows=0;
	affectedrows=-1;
	resultsetsdrained=false;
	clearCachedRows();
}

bool odbccursor::handleColumns(bool getcolumninfo, bool bindcolumns) {

	// get the column count
	erg=SQLNumResultCols(stmt,&ncols);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {

		// column info may not be valid until post-execute for
		// particular queries
		// (eg. "select ?" with a bind value in MS SQL Server, or a
		// stored procedure that optionally executes selects with
		// different numbers of columns)
		if (odbcconn->columninfonotvalidyeterror) {
			SQLCHAR		state[SQL_SQLSTATE_SIZE+1];
			SQLINTEGER	nativeerrnum=0;
			SQLSMALLINT	errsize=0;
			bytestring::zero(state,sizeof(state));
			SQLGetDiagRec(SQL_HANDLE_STMT,stmt,1,
							state,&nativeerrnum,
							NULL,0,&errsize);
			for (SQLINTEGER *ptr=
					odbcconn->columninfonotvalidyeterror;
					*ptr; ptr++) {
				if (nativeerrnum==*ptr) {
					columninfoisvalidafterprepare=false;
					erg=SQL_SUCCESS;
					return true;
				}
			}
		}
		return false;
	}

	// limit column count if necessary
	uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
	if (maxcolumncount && (uint32_t)ncols>maxcolumncount) {
		ncols=maxcolumncount;
	}

	if (getcolumninfo) {

		// allocate buffers if necessary
		if (!maxcolumncount) {
			// free any buffers left over from a previous result
			// set of this same query (see nextResultSet())
			deallocateResultSetBuffers();
			allocateResultSetBuffers(ncols);
		}

		// run through the columns
		for (SQLSMALLINT i=0; i<ncols; i++) {

			if (conn->cont->getSendColumnInfo()) {
#if (ODBCVER >= 0x0300)
				// column name
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_LABEL,
						column[i].name,4096,
						(SQLSMALLINT *)
						&(column[i].namesize),
						NULL);
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}
				column[i].namesize=
					charstring::getLength(column[i].name);

				// column size
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_LENGTH,
						NULL,0,NULL,
						&(column[i].size));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}
	
				// column type
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_TYPE,
						NULL,0,NULL,
						&(column[i].type));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// SQL_DESC_TYPE is the "verbose" type -
				// SQL_DATETIME for a date, time or timestamp
				// column alike, on drivers that follow the
				// spec.  SQL_DESC_CONCISE_TYPE discriminates
				// those - SQL_TYPE_DATE, SQL_TYPE_TIME or
				// SQL_TYPE_TIMESTAMP - on every driver seen
				// so far, but not MS SQL Server's datetime vs.
				// datetime2 (see the type name check below),
				// so keep both.
				column[i].concisetype=column[i].type;
				if (column[i].type==SQL_DATETIME) {
					erg=SQLColAttribute(stmt,i+1,
							SQL_DESC_CONCISE_TYPE,
							NULL,0,NULL,
							&(column[i].concisetype));
					if (erg!=SQL_SUCCESS &&
						erg!=SQL_SUCCESS_WITH_INFO) {
						return false;
					}
				}

				// column type name
				// MS SQL Server reports datetime and datetime2
				// identically - SQL_DESC_TYPE SQL_DATETIME,
				// SQL_DESC_CONCISE_TYPE SQL_TYPE_TIMESTAMP,
				// and a datetime2(3) even has the same length,
				// precision and scale as a datetime.  The type
				// name is the only way to tell them apart.
				SQLSMALLINT	dbtypenamesize;
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_TYPE_NAME,
						column[i].dbtypename,
						sizeof(column[i].dbtypename),
						&dbtypenamesize,NULL);
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// column precision
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_PRECISION,
						NULL,0,NULL,
						&(column[i].precision));
				// Some drivers (Redshift) like to return -1
				// for the precision of some (TEXT/NTEXT)
				// columns.  This wreaks havoc on the client
				// side, as the value is interpreted as 2^32-1.
				// Override the -1 with the size.
				if (column[i].precision==-1) {
					column[i].precision=column[i].size;
				}
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// column scale
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_SCALE,
						NULL,0,NULL,
						&(column[i].scale));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// column nullable
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_NULLABLE,
						NULL,0,NULL,
						&(column[i].nullable));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// primary key

				// unique

				// part of key

				// unsigned number
				erg=SQLColAttribute(stmt,i+1,SQL_DESC_UNSIGNED,
						NULL,0,NULL,
						&(column[i].unsignednumber));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// zero fill

				// binary

				// autoincrement
				erg=SQLColAttribute(stmt,i+1,
						SQL_DESC_AUTO_UNIQUE_VALUE,
						NULL,0,NULL,
						&(column[i].autoincrement));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// table name
				erg=SQLColAttribute(stmt,i+1,
						SQL_DESC_BASE_TABLE_NAME,
						column[i].table,4096,
						(SQLSMALLINT *)
						&(column[i].tablesize),
						NULL);
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}
				// Some databases (Hive) like to return
				// columns as table.column.
				// If the column name was table.column then
				// split it and override the table name.
				char	*dot=charstring::findFirst(
							column[i].name,'.');
				if (dot) {
					char	*col=dot+1;
					*dot='\0';
					charstring::copy(column[i].table,
								column[i].name);
					charstring::copy(columnnamescratch,col);
					charstring::copy(column[i].name,
							columnnamescratch);
					column[i].namesize=
						charstring::getLength(
							column[i].name);
				}
				column[i].tablesize=
					charstring::getLength(column[i].table);

#else
				// column name
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_LABEL,
						column[i].name,4096,
						(SQLSMALLINT *)
						&(column[i].namesize),
						NULL);
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}
				// FIXME: above we reset namesize
				// to stringSize(name)...

				// column size
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_LENGTH,
						NULL,0,NULL,
						&(column[i].size));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// column type
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_TYPE,
						NULL,0,NULL,
						&(column[i].type));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}
				// ODBC 2.x has no verbose/concise type
				// split, SQL_COLUMN_TYPE already
				// discriminates date, time and timestamp
				column[i].concisetype=column[i].type;

				// column type name
				// (see the comment on the odbc 3 version
				// of this call above)
				SQLSMALLINT	dbtypenamesize;
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_TYPE_NAME,
						column[i].dbtypename,
						sizeof(column[i].dbtypename),
						&dbtypenamesize,NULL);
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// column precision
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_PRECISION,
						NULL,0,NULL,
						&(column[i].precision));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// column scale
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_SCALE,
						NULL,0,NULL,
						&(column[i].scale));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// column nullable
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_NULLABLE,
						NULL,0,NULL,
						&(column[i].nullable));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// primary key

				// unique

				// part of key

				// unsigned number
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_UNSIGNED,
						NULL,0,NULL,
						&(column[i].unsignednumber));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}

				// zero fill

				// binary

				// autoincrement
				#ifdef SQL_DESC_AUTO_UNIQUE_VALUE
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_AUTO_INCREMENT,
						NULL,0,NULL,
						&(column[i].autoincrement));
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}
				#else
				column[i].autoincrement=0;
				#endif

				// table name
				erg=SQLColAttributes(stmt,i+1,
						SQL_COLUMN_TABLE_NAME,
						column[i].table,4096,
						(SQLSMALLINT *)
						&(column[i].tablesize),
						NULL);
				if (erg!=SQL_SUCCESS &&
					erg!=SQL_SUCCESS_WITH_INFO) {
					return false;
				}
				// Some databases (Hive) like to return
				// columns as table.column.
				// If the column name was table.column then
				// split it and override the table name.
				char	*dot=charstring::findFirst(
							column[i].name,'.');
				if (dot) {
					char	*col=dot+1;
					*dot='\0';
					charstring::copy(column[i].table,
								column[i].name);
					charstring::copy(columnnamescratch,col);
					charstring::copy(column[i].name,
							columnnamescratch);
					column[i].namesize=
						charstring::getLength(
							column[i].name);
				}
				column[i].tablesize=
					charstring::getLength(column[i].table);
#endif
			}
		}
	}

	if (bindcolumns) {

		// allocate buffers if necessary
		/*if (!maxcolumncount) {
			allocateResultSetBuffers(ncols);
		}*/

		uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();

		// run through the columns
		for (SQLSMALLINT i=0; i<ncols; i++) {

			// bind the column to a buffer
			#ifdef HAVE_SQLCONNECTW
			if (odbcconn->unicode) {
				if (column[i].type==SQL_WVARCHAR ||
					column[i].type==SQL_WCHAR) {
					erg=SQLBindCol(stmt,i+1,SQL_C_WCHAR,
							field[i],maxfieldsize,
							&(indicator[i]));
				} else if (column[i].type==SQL_TYPE_TIMESTAMP ||
					(odbcconn->sqltypedatetosqlcbinary &&
					column[i].type==SQL_TYPE_DATE)) {
					erg=SQLBindCol(stmt,i+1,SQL_C_BINARY,
							field[i],maxfieldsize,
							&(indicator[i]));
				} else if (!isLob(column[i].type)) {
					erg=SQLBindCol(stmt,i+1,SQL_C_CHAR,
							field[i],maxfieldsize,
							&(indicator[i]));
				}
			} else {
			#endif
				if (!isLob(column[i].type)) {
					erg=SQLBindCol(stmt,i+1,SQL_C_CHAR,
							field[i],maxfieldsize,
							&(indicator[i]));
				}
			#ifdef HAVE_SQLCONNECTW
			}
			#endif
		
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}
		}
	}

	return true;
}

bool odbccursor::appendNullColumns(uint8_t count) {

	// the get*List() result sets need trailing NULL columns for
	// the sqlrservercontroller to map unprovided columns to...

	// grow the column buffers, if necessary...
	if (ncols+count>columncount) {

		// bump the column count
		int32_t		newcount=ncols+count;

		// allocate a new set of column buffers
		char		**newfield=new char *[newcount];
		#ifdef SQLBINDCOL_SQLLEN
		SQLLEN		*newloblength=new SQLLEN[newcount];
		SQLLEN		*newindicator=new SQLLEN[newcount];
		#else
		SQLINTEGER	*newloblength=new SQLINTEGER[newcount];
		SQLINTEGER	*newindicator=new SQLINTEGER[newcount];
		#endif
		odbccolumn	*newcolumn=new odbccolumn[newcount];

		// keep the existing columns
		for (int32_t i=0; i<columncount; i++) {
			newfield[i]=field[i];
			newloblength[i]=loblength[i];
			newindicator[i]=indicator[i];
			newcolumn[i]=column[i];
		}

		// the appended columns are null and need no data buffers
		for (int32_t i=columncount; i<newcount; i++) {
			newfield[i]=NULL;
		}

		delete[] field;
		delete[] loblength;
		delete[] indicator;
		delete[] column;

		field=newfield;
		loblength=newloblength;
		indicator=newindicator;
		column=newcolumn;
		columncount=newcount;
	}

	// append null columns
	for (uint8_t i=0; i<count; i++) {
		column[ncols].name[0]='\0';
		column[ncols].namesize=0;
		column[ncols].dbtypename[0]='\0';
		column[ncols].type=SQL_CHAR;
		column[ncols].concisetype=SQL_CHAR;
		column[ncols].size=0;
		column[ncols].precision=0;
		column[ncols].scale=0;
		column[ncols].nullable=SQL_NULLABLE;
		column[ncols].unsignednumber=0;
		column[ncols].autoincrement=0;
		column[ncols].table[0]='\0';
		column[ncols].tablesize=0;
		indicator[ncols]=SQL_NULL_DATA;
		ncols++;
	}

	return true;
}

bool odbccursor::appendNullColumn() {
	return appendNullColumns(1);
}

bool odbccursor::appendColumnListColumns() {

	// the odbc api can't supply the numeric_precision, column_key,
	// and is_autoincrement columns of the SQLColumns()+ format
	return appendNullColumns(3);
}

void odbccursor::getError(char *errorbuffer,
				uint32_t errorbuffersize,
				uint32_t *errorsize,
				int64_t *errorcode,
				bool *liveconnection) {
	if (bindformaterror) {
		// handle bind format errors
		*errorsize=charstring::getLength(
				SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING);
		charstring::safeCopy(errorbuffer,
				errorbuffersize,
				SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING,
				*errorsize);
		*errorcode=SQLR_ERROR_INVALIDBINDVARIABLEFORMAT;
		*liveconnection=true;
		return;
	}

	SQLCHAR		state[SQL_SQLSTATE_SIZE+1];
	SQLINTEGER	nativeerrnum;
	SQLSMALLINT	errsize;

	bytestring::zero(state,sizeof(state));

	SQLGetDiagRec(SQL_HANDLE_STMT,stmt,1,state,&nativeerrnum,
				(SQLCHAR *)errorbuffer,errorbuffersize,
				&errsize);

	// set return values
	*errorsize=errsize;
	*errorcode=nativeerrnum;
	*liveconnection=odbcconn->isLiveConnection(state);
}

uint64_t odbccursor::getAffectedRows() {
	return affectedrows;
}

uint32_t odbccursor::colCount() {
	return ncols;
}

const char *odbccursor::getColumnName(uint32_t i) {
	return column[i].name;
}

uint16_t odbccursor::getColumnNameSize(uint32_t i) {
	return column[i].namesize;
}

uint16_t odbccursor::getColumnType(uint32_t i) {

	switch (column[i].type) {

		// generic types...
		case SQL_CHAR:
			return CHAR_DATATYPE;
		case SQL_VARCHAR:
			return VARCHAR_DATATYPE;
		case SQL_LONGVARCHAR:
			return LONGVARCHAR_DATATYPE;
		case SQL_WCHAR:
			return NCHAR_DATATYPE;
		case SQL_WVARCHAR:
			return NVARCHAR_DATATYPE;
		case SQL_WLONGVARCHAR:
			return NTEXT_DATATYPE;
		case SQL_DECIMAL:
			// MS SQL Server reports money, smallmoney and
			// decimal the same way here, so go by the type name.
			// They aren't decimals - they're fixed scale-4 types
			// with their own wire format, and a client that
			// describes the column can tell.
			if (!charstring::compareIgnoringCase(
					column[i].dbtypename,"money")) {
				return MONEY_DATATYPE;
			}
			if (!charstring::compareIgnoringCase(
					column[i].dbtypename,"smallmoney")) {
				return SMALLMONEY_DATATYPE;
			}
			return DECIMAL_DATATYPE;
		case SQL_NUMERIC:
			return NUMERIC_DATATYPE;
		case SQL_SMALLINT:
			return SMALLINT_DATATYPE;
		case SQL_INTEGER:
			return INTEGER_DATATYPE;
		case SQL_REAL:
			return REAL_DATATYPE;
		case SQL_FLOAT:
			return FLOAT_DATATYPE;
		case SQL_DOUBLE:
			return DOUBLE_DATATYPE;
		case SQL_DATE:
		//case SQL_DATETIME:
		//	(odbc 3 dup of SQL_DATE)
			// MS SQL Server reports datetime and datetime2 the
			// same way here, so go by the type name.  datetime2
			// carries more fractional second digits than datetime,
			// which matters to clients that render it themselves.
			if (!charstring::compareIgnoringCase(
					column[i].dbtypename,"datetime2")) {
				return TIMESTAMP_DATATYPE;
			}
			// and smalldatetime the same way - it's half the
			// width of a datetime on the wire
			if (!charstring::compareIgnoringCase(
					column[i].dbtypename,"smalldatetime")) {
				return SMALLDATETIME_DATATYPE;
			}
			// and MS SQL Server's own datetime
			if (!charstring::compareIgnoringCase(
					column[i].dbtypename,"datetime")) {
				return DATETIME_DATATYPE;
			}
			// Most other drivers put SQL_DATETIME here for a
			// date, time and timestamp column alike (this is
			// the "verbose" type).  SQL_DESC_CONCISE_TYPE
			// discriminates them - SQL_TYPE_DATE, SQL_TYPE_TIME
			// or SQL_TYPE_TIMESTAMP - so use it here instead.
			switch (column[i].concisetype) {
				case SQL_TYPE_DATE:
					return DATE_DATATYPE;
				case SQL_TYPE_TIME:
					return TIME_DATATYPE;
				case SQL_TYPE_TIMESTAMP:
					return TIMESTAMP_DATATYPE;
				default:
					break;
			}
			return DATETIME_DATATYPE;
		case SQL_TIME:
		//case SQL_INTERVAL:
		//	(odbc 3 dup of SQL_TIME)
			return TIME_DATATYPE;
		case SQL_TIMESTAMP:
			return TIMESTAMP_DATATYPE;
		case SQL_BIT:
			return BIT_DATATYPE;
		case SQL_TINYINT:
			return TINYINT_DATATYPE;
		case SQL_BIGINT:
			return BIGINT_DATATYPE;
		case SQL_BINARY:
			return BINARY_DATATYPE;
		case SQL_VARBINARY:
			return VARBINARY_DATATYPE;
		case SQL_LONGVARBINARY:
			return LONGVARBINARY_DATATYPE;
		case SQL_TYPE_DATE:
			return DATE_DATATYPE;
		case SQL_TYPE_TIME:
			return TIME_DATATYPE;
		case SQL_TYPE_TIMESTAMP:
			return TIMESTAMP_DATATYPE;
		// FIXME:
		// #ifdef SQL_TYPE_UTCDATETIME
		//case SQL_TYPE_UTCDATETIME:
		// FIXME:
		// #ifdef SQL_TYPE_UTCTIME
		//case SQL_TYPE_UTCTIME:
		// FIXME:
		// interval types...
		case SQL_GUID:
			return UNIQUEIDENTIFIER_DATATYPE;

		// MS SQL Server types
		case -150:
			// FIXME:
			// this is "sql_variant"
			// is there a better type to map it to?
			return VARCHAR_DATATYPE;
		case -152:
			return XML_DATATYPE;
		case -154:
			return TIME_DATATYPE;
		case -155:
			return DATETIMEOFFSET_DATATYPE;

		default:
			return UNKNOWN_DATATYPE;
	}
}

uint32_t odbccursor::getColumnSize(uint32_t i) {
	return column[i].size;
}

uint32_t odbccursor::getColumnPrecision(uint32_t i) {
	return column[i].precision;
}

uint32_t odbccursor::getColumnScale(uint32_t i) {
	return column[i].scale;
}

uint16_t odbccursor::getColumnIsNullable(uint32_t i) {
	return column[i].nullable;
}

uint16_t odbccursor::getColumnIsUnsigned(uint32_t i) {
	return column[i].unsignednumber;
}

uint16_t odbccursor::getColumnIsBinary(uint32_t i) {
	uint16_t	type=getColumnType(i);
	return (type==BINARY_DATATYPE ||
		type==LONGVARBINARY_DATATYPE ||
		type==VARBINARY_DATATYPE);
}

uint16_t odbccursor::getColumnIsAutoIncrement(uint32_t i) {
	return column[i].autoincrement;
}

const char *odbccursor::getColumnTable(uint32_t i) {
	return column[i].table;
}

uint16_t odbccursor::getColumnTableSize(uint32_t i) {
	return column[i].tablesize;
}

bool odbccursor::noRowsToReturn() {
	// if there are no columns, then there can't be any rows either
	return (!ncols);
}

// Cap on how much of a result set will be buffered to get at the output binds
// behind it.  A result set larger than this keeps all of its rows - the ones
// that were buffered are just served from the buffer first - but the output
// binds are left unwritten, because the result sets can't be drained without
// losing the rest of the rows.
#define MAXOUTPUTBINDROWCACHESIZE (16*1024*1024)

bool odbccursor::cacheRowsAndDrainResultSets() {

	// At least with the Microsoft ODBC Driver, a stored procedure's output
	// parameters and return status don't reach the bind buffers until
	// SQLMoreResults() has returned SQL_NO_DATA.  They arrive at the end of
	// the wire protocol's token stream, after the rows, so no driver can
	// hand them over any earlier.
	//
	// The protocol modules send the output bind values as part of the
	// result set header, ahead of the rows, so the values have to be in
	// hand by the time this method returns.  That means the rows have to be
	// buffered here, and the result sets drained, before the caller copies
	// the output binds out.

	// nothing to drain if the query has no output binds
	if (!getOutputBindCount() && !getInputOutputBindCount()) {
		return true;
	}

	// If the query didn't return any rows then there's nothing to buffer,
	// and the driver has already written the bind buffers.  This is the
	// case for a plain rpc, and it worked before any of this existed.
	if (!ncols) {
		return true;
	}

	// Lob fields are read from the live statement with SQLGetData, so they
	// can't be buffered.  Leave the result set alone rather than lose them,
	// and let the output binds go unwritten.
	for (SQLSMALLINT i=0; i<ncols; i++) {
		if (isLob(column[i].type)) {
			return true;
		}
	}

	// buffer the rows
	uint64_t	cachedbytes=0;
	for (;;) {

		bool	error=false;
		if (!fetchRow(&error)) {
			if (error) {
				return false;
			}
			break;
		}

		// If the result set is too big to buffer then stop here.  The
		// rows that were buffered are still served from the buffer,
		// and the rest still come from the statement, so no row is
		// lost.  The result sets just don't get drained, so the output
		// binds keep whatever they were initialized to.
		if (!cacheCurrentRow(&cachedbytes)) {
			currentcachedrow=cachedrows.getFirst();
			cachedrowsarecomplete=false;
			return true;
		}
	}

	// drain the rest of the result sets, which is what finally makes the
	// driver write the output bind buffers
	for (;;) {
		erg=SQLMoreResults(stmt);
		#if defined(SQL_NO_DATA)
		if (erg==SQL_NO_DATA) {
			break;
		}
		#elif defined(SQL_NO_DATA_FOUND)
		if (erg==SQL_NO_DATA_FOUND) {
			break;
		}
		#endif
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			return false;
		}
	}

	currentcachedrow=cachedrows.getFirst();
	cachedrowsarecomplete=true;
	resultsetsdrained=true;

	return true;
}

bool odbccursor::cacheCurrentRow(uint64_t *cachedbytes) {

	// A buffered row is the indicator and data of each column, in column
	// order, packed one after another.  A null or empty field contributes
	// its indicator and no data.
	uint64_t	rowsize=0;
	for (SQLSMALLINT i=0; i<ncols; i++) {
		rowsize=rowsize+sizeof(indicator[i]);
		if (indicator[i]>0) {
			rowsize=rowsize+indicator[i];
		}
	}

	// bail if buffering this row would run past the cap
	if (*cachedbytes+rowsize>MAXOUTPUTBINDROWCACHESIZE) {
		return false;
	}

	unsigned char	*cachedrow=new unsigned char[rowsize];
	unsigned char	*ptr=cachedrow;
	for (SQLSMALLINT i=0; i<ncols; i++) {
		bytestring::copy(ptr,&(indicator[i]),sizeof(indicator[i]));
		ptr=ptr+sizeof(indicator[i]);
		if (indicator[i]>0) {
			bytestring::copy(ptr,field[i],indicator[i]);
			ptr=ptr+indicator[i];
		}
	}
	cachedrows.append(cachedrow);

	*cachedbytes=*cachedbytes+rowsize;

	return true;
}

void odbccursor::fetchCachedRow() {

	unsigned char	*ptr=currentcachedrow->getValue();
	for (SQLSMALLINT i=0; i<ncols; i++) {
		bytestring::copy(&(indicator[i]),ptr,sizeof(indicator[i]));
		ptr=ptr+sizeof(indicator[i]);
		if (indicator[i]>0) {
			bytestring::copy(field[i],ptr,indicator[i]);
			ptr=ptr+indicator[i];
		}
	}

	currentcachedrow=currentcachedrow->getNext();
}

void odbccursor::clearCachedRows() {
	cachedrows.clear();
	currentcachedrow=NULL;
	cachedrowsarecomplete=false;
}

bool odbccursor::fetchRow(bool *error) {

	*error=false;

	// If the rows were buffered up front, to get at the output binds behind
	// them, then serve them from the buffer.  When the buffer runs out,
	// either the result set is done, or the buffer hit its cap and the rest
	// of the rows still have to come from the statement.
	if (currentcachedrow) {
		fetchCachedRow();
		return true;
	}
	if (cachedrowsarecomplete) {
		return false;
	}

	erg=SQLFetch(stmt);
	if (erg==SQL_ERROR) {
		*error=true;
		return false;
	}
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		return false;
	}
	
	#ifdef HAVE_SQLCONNECTW
	if (odbcconn->unicode) {
		// convert wvarchar/wchar fields to user coding
		uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();
		for (SQLSMALLINT i=0; i<ncols; i++) {
			if (column[i].type==SQL_WVARCHAR ||
					column[i].type==SQL_WCHAR) {
				if (indicator[i]!=SQL_NULL_DATA && field[i]) {
					char	*err=NULL;
					byte_t	*u=convertCharset(
						(const byte_t *)field[i],
						odbcconn->ncharencoding,
						"UTF-8",&err);
					if (err) {
						delete[] u;
						setConvCharError("fetch",err);
						return false;
					}
					// clamp to the field buffer
					size_t	nullsize=nullSize("UTF-8");
					size_t	s=stringSize(u,"UTF-8");
					if (s>maxfieldsize) {
						s=maxfieldsize;
					}

					// stringSize() counts the null
					// terminator, indicator[] must not
					s=(s>nullsize)?s-nullsize:0;

					// copy in and null-terminate
					bytestring::copy(field[i],u,s);
					if (s+nullsize<=maxfieldsize) {
						bytestring::zero(field[i]+s,
								nullsize);
					}
					indicator[i]=s;
					delete[] u;
				}
			}
		}
	}
	#endif

	return true;
}

void odbccursor::getField(uint32_t col,
				const char **fld, uint64_t *fldsize,
				bool *lob, bool *null) {

	// handle NULLs
	if (indicator[col]==SQL_NULL_DATA) {
		*null=true;
		return;
	}

	// handle lobs
	if (isLob(column[col].type)) {
		*lob=true;
		return;
	}

	// handle normal datatypes
	*fld=field[col];
	*fldsize=indicator[col];
}

bool odbccursor::getLobFieldLength(uint32_t col, uint64_t *length) {

	// get the length of the lob

	// a valid buffer must be provided, but it's ok to fetch 0 bytes into it
	SQLCHAR	buffer[1];
	erg=SQLGetData(stmt,col+1,SQL_C_BINARY,buffer,0,&(loblength[col]));
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		return false;
	}

	// FIXME: SQL Server XML types reliably return SQL_NO_TOTAL, so for now
	// we aren't handling them as LOBs.  Is there some other way we can
	// determine the length?
	if (loblength[col]==SQL_NO_TOTAL) {
		return false;
	}

	// copy out the length
	*length=loblength[col];

	return true;
}

bool odbccursor::getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {

	// bail if we're attempting to start reading past the end
	if (offset>(uint64_t)loblength[col]) {
		return false;
	}

	// prevent attempts to read past the end
	if (offset+charstoread>(uint64_t)loblength[col]) {
		charstoread=charstoread-((offset+charstoread)-loblength[col]);
	}

	// read a lob segment, at most MAX_LOB_CHUNK_SIZE bytes at a time
	uint64_t	totalbytesread=0;
	SQLLEN		bytestoread=0;
	uint64_t	remainingbytestoread=charstoread;
	for (;;) {

		// figure out how many bytes to read this time
		if (remainingbytestoread<MAX_LOB_CHUNK_SIZE) {
			bytestoread=remainingbytestoread;
		} else {
			bytestoread=MAX_LOB_CHUNK_SIZE;
			remainingbytestoread=remainingbytestoread-
						MAX_LOB_CHUNK_SIZE;
		}

		// read the bytes
		SQLLEN	ind=0;
		erg=SQLGetData(stmt,col+1,
				SQL_C_BINARY,
				buffer+totalbytesread,
				bytestoread,&ind);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			return false;
		}

		// determine how many bytes were read
		uint64_t	bytesread=
			(ind>=bytestoread || ind==SQL_NO_TOTAL)?bytestoread:ind;

		// update total bytes read
		totalbytesread=totalbytesread+bytesread;

		// bail if we're done reading
		if ((SQLUINTEGER)bytesread<bytestoread ||
				totalbytesread==charstoread) {
			break;
		}
	}

	// return number of bytes/chars read
	*charsread=totalbytesread;

	return true;
}

bool odbccursor::nextResultSet(bool *nextresultsetavailable) {

	*nextresultsetavailable=false;

	if (!stmt) {
		return true;
	}

	// once cacheRowsAndDrainResultSets() has walked off of the end of the
	// result sets, there's nothing left to advance to
	if (resultsetsdrained) {
		return true;
	}

	// advance to the next result set
	erg=SQLMoreResults(stmt);
	#if defined(SQL_NO_DATA)
	if (erg==SQL_NO_DATA) {
		resultsetsdrained=true;
		ncols=0;
		return true;
	}
	#elif defined(SQL_NO_DATA_FOUND)
	if (erg==SQL_NO_DATA_FOUND) {
		resultsetsdrained=true;
		ncols=0;
		return true;
	}
	#endif
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		return false;
	}

	// Column bindings persist by ordinal across SQLMoreResults(), and a
	// stale binding for an ordinal that the new result set doesn't have is
	// left alone rather than cleared.  So the new result set has to be
	// described and bound from scratch, exactly like the first one.
	initializeRowCounts();
	if (!handleColumns(true,true)) {
		// handleColumns() may have already freed and reallocated the
		// column buffers for the new result set before failing, so
		// ncols has to be zeroed here too - otherwise the caller's
		// cached column-header snapshot (which it only refreshes on
		// success) is left describing a result set whose backing
		// buffers are gone.
		ncols=0;
		return false;
	}

	// get the row count
	erg=SQLRowCount(stmt,&affectedrows);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		ncols=0;
		return false;
	}

	*nextresultsetavailable=true;

	return true;
}

void odbccursor::closeResultSet() {

	clearCachedRows();
	resultsetsdrained=false;

	if (stmt) {
		SQLCloseCursor(stmt);
		// The msdn.microsoft.com documentation says that this call
		// is equivalent to SQLFreeStmt with SQL_CLOSE.  If so, then
		// we should be able to set stmt to NULL here.  But, if we do,
		// then we get SQLExecute.c][170]Error: SQL_INVALID_HANDLE.
		// So apparently the microsoft documentation is wrong.
	}

	for (uint16_t i=0; i<getOutputBindCount(); i++) {
		delete outdatebind[i];
	}

	for (uint16_t i=0; i<getOutputBindCount(); i++) {
		delete outcharbind[i];
	}

	for (uint16_t i=0; i<getInputOutputBindCount(); i++) {
		delete inoutdatebind[i];
	}

	for (uint16_t i=0; i<getInputOutputBindCount(); i++) {
		delete inoutcharbind[i];
	}

	// FIXME: inefficient, but there appears to be a case where
	// closeResultSet isn't called, and stale ptrs get left lingering
	// around...
	for (uint16_t i=0; i<maxbindcount; i++) {
		outdatebind[i]=NULL;
		outcharbind[i]=NULL;
		outisnullptr[i]=NULL;
		outisnull[i]=0;
		inoutdatebind[i]=NULL;
		inoutcharbind[i]=NULL;
		inoutisnullptr[i]=NULL;
		inoutisnull[i]=0;
		inbindlength[i]=0;
		nullbindisbinary[i]=false;
		nullbinddescribed[i]=false;
	}

	if (!conn->cont->getMaxColumnCount()) {
		deallocateResultSetBuffers();
	}

	// NOTE: this is a bit of a kludge.
	//
	// ncols is reset at the beginning of prepareQuery, and other methods,
	// but, since we rely on it to decide whether there are rows to return,
	// it really needs to be reset here.
	//
	// If sqlrservercontroller intercepts the query (eg. if it's a begin,
	// commit, rollback, etc.) then prepareQuery() will never be called,
	// and this won't be reset.  If it was > 0 from the previous query,
	// then a begin (for example) will think that it has rows to return,
	// and the subsequent SQLFetch will fail with a
	// "function sequence error".  We can avoid that by setting ncols=0
	// here, which will cause noRowsToReturn() to return false by default,
	// and avoid the fetch.
	//
	// Arguably, other things should be reset here too
	// (columninfoisvalidafterprepare, various row counts, etc.) but this
	// is the critical one for now, so we'll sort that out later.
	ncols=0;
}

bool odbccursor::columnInfoIsValidAfterPrepare() {
	return columninfoisvalidafterprepare;
}

#if (ODBCVER >= 0x0300) && defined(SQLCOLATTRIBUTE_SQLLEN)
bool odbccursor::isLob(SQLLEN type) {
#else
bool odbccursor::isLob(SQLINTEGER type) {
#endif

	if (odbcconn->fetchlobsasstrings) {
		return false;
	}

	// FIXME: -152 (SQL Server XML) types are kind-of also LOBs, but
	// attempts to get their sizes reliably result in SQL_NO_TOTAL.
	// We don't (currently) have a way of determining their sizes,
	// so, for now, we'll handle them as non-LOBs.
	return (type==SQL_LONGVARCHAR ||
		type==SQL_LONGVARBINARY ||
		type==SQL_WLONGVARCHAR);
}

void odbccursor::setConvCharError(const char *baseerror,
						const char *detailerror) {

	stringbuffer	err;
	err.append(baseerror)->append(": ")->append(detailerror);
	conn->cont->setError(this,err.getString(),
				SQLR_ERROR_CHARACTER_CONVERSION_FAILED,true);
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_odbcconnection(
						sqlrservercontroller *cont) {
		return new odbcconnection(cont);
	}
}

// Copyright (c) David Muse
// See the file COPYING for more information

#include <config.h>

#include <sqlrelay/sqlrserver.h>
#include <rudiments/file.h>
#include <rudiments/socketclient.h>
#include <rudiments/bytestring.h>
#include <rudiments/userentry.h>
#include <rudiments/groupentry.h>
#include <rudiments/process.h>
#include <rudiments/permissions.h>
#include <rudiments/snooze.h>
#include <rudiments/error.h>
#include <rudiments/character.h>
#include <rudiments/charstring.h>
#include <rudiments/randomnumber.h>
#include <rudiments/sys.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>
#include <rudiments/semaphoreset.h>
#include <rudiments/sharedmemory.h>
#include <rudiments/unixsocketserver.h>
#include <rudiments/unixsocketclient.h>
#include <rudiments/inetsocketserver.h>
#include <rudiments/listener.h>
#include <rudiments/md5.h>
#include <rudiments/sensitivevalue.h>

#include <defines.h>
#include <defaults.h>
#define NEED_DATATYPESTRING 1
#define NEED_IS_BIT_TYPE_CHAR 1
#define NEED_IS_BIT_TYPE_INT 1
#define NEED_IS_BOOL_TYPE_CHAR 1
#define NEED_IS_BOOL_TYPE_INT 1
#define NEED_IS_FLOAT_TYPE_CHAR 1
#define NEED_IS_FLOAT_TYPE_INT 1
#define NEED_IS_NUMBER_TYPE_CHAR 1
#define NEED_IS_NUMBER_TYPE_INT 1
#define NEED_IS_BLOB_TYPE_CHAR 1
#define NEED_IS_BLOB_TYPE_INT 1
#define NEED_IS_CLOB_TYPE_CHAR 1
#define NEED_IS_CLOB_TYPE_INT 1
#define NEED_IS_UNSIGNED_TYPE_CHAR 1
#define NEED_IS_UNSIGNED_TYPE_INT 1
#define NEED_IS_BINARY_TYPE_CHAR 1
#define NEED_IS_BINARY_TYPE_INT 1
#define NEED_IS_DATETIME_TYPE_CHAR 1
#define NEED_IS_DATETIME_TYPE_INT 1
#include <datatypes.h>
#define NEED_BEFORE_BIND_VARIABLE 1
#define NEED_IS_BIND_DELIMITER 1
#define NEED_AFTER_BIND_VARIABLE 1
#define NEED_COUNT_BIND_VARIABLES 1
#include <bindvariables.h>

#define ACK	6

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrserverconnectiondeclarations.cpp"
		#include "sqlrparserdeclarations.cpp"
	}
#endif

class sqlrdatabaseobject {
	public:
		const char	*database;
		const char	*schema;
		const char	*object;
		const char	*dependency;
};


class sqlrquery_lastinsertidlistcursor : public sqlrquerycursor {
	public:
		sqlrquery_lastinsertidlistcursor(
					sqlrserverconnection *sqlrcon,
					sqlrquery *q,
					domnode *parameters,
					uint16_t id);
		~sqlrquery_lastinsertidlistcursor();

		void	setLastInsertId(uint64_t lid);

		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
		uint16_t	getColumnType(uint32_t col);
		uint32_t	getColumnSize(uint32_t col);
		uint32_t	getColumnPrecision(uint32_t col);
		uint32_t	getColumnScale(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		bool		noRowsToReturn();
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob, bool *null);
	private:
		bool		firstrow;
		char		*lid;
};

sqlrquery_lastinsertidlistcursor::sqlrquery_lastinsertidlistcursor(
					sqlrserverconnection *sqlrcon,
					sqlrquery *q,
					domnode *parameters,
					uint16_t id) :
				sqlrquerycursor(sqlrcon,q,parameters,id) {
	firstrow=true;
	lid=NULL;
}

sqlrquery_lastinsertidlistcursor::~sqlrquery_lastinsertidlistcursor() {
	delete[] lid;
}

void sqlrquery_lastinsertidlistcursor::setLastInsertId(uint64_t lid) {
	firstrow=true;
	delete[] this->lid;
	this->lid=charstring::parseNumber(lid);
}

uint32_t sqlrquery_lastinsertidlistcursor::colCount() {
	return 1;
}

const char *sqlrquery_lastinsertidlistcursor::getColumnName(uint32_t col) {
	return (col<1)?"LAST_INSERT_ID()":NULL;
}

uint16_t sqlrquery_lastinsertidlistcursor::getColumnType(uint32_t col) {
	return (col<1)?NUMBER_DATATYPE:0;
}

uint32_t sqlrquery_lastinsertidlistcursor::getColumnSize(uint32_t col) {
	return (col<1)?10:0;
}

uint32_t sqlrquery_lastinsertidlistcursor::getColumnPrecision(uint32_t col) {
	return (col<1)?10:0;
}

uint32_t sqlrquery_lastinsertidlistcursor::getColumnScale(uint32_t col) {
	return 0;
}

uint16_t sqlrquery_lastinsertidlistcursor::getColumnIsNullable(uint32_t col) {
	return 0;
}

bool sqlrquery_lastinsertidlistcursor::noRowsToReturn() {
	return false;
}

bool sqlrquery_lastinsertidlistcursor::fetchRow(bool *error) {
	*error=false;
	if (firstrow) {
		firstrow=false;
		return true;
	}
	return false;
}

void sqlrquery_lastinsertidlistcursor::getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null) {
	*field=NULL;
	*fieldsize=0;
	*lob=false;
	*null=false;

	if (!col) {
		*field=lid;
		*fieldsize=charstring::getLength(lid);
	} else {
		*null=true;
	}
}

class sqlrservercontrollerprivate {
	friend class sqlrservercontroller;

	listener		_lsnr;
	sqlrserverconnection	*_conn;

	sqlrconfigs		*_sqlrcfgs;
	sqlrconfig		*_cfg;
	sqlrpaths		*_pth;

	bool			_debug;
	stringbuffer		_logbuffer;

	sqlrshm			*_shm;
	sqlrconnstatistics	*_connstats;

	sqlrcmdline	*_cmdl;

	semaphoreset	*_semset;
	sharedmemory	*_shmem;

	sqlrprotocols				*_sqlrpr;
	sqlrparser				*_sqlrp;
	sqlrdirectives				*_sqlrd;
	sqlrquerytranslations			*_sqlrt;
	sqlrfilters				*_sqlrf;
	sqlrbindvariabletranslations		*_sqlrbvt;
	sqlrresultsetheadertranslations		*_sqlrrsht;
	sqlrresultsettranslations		*_sqlrrst;
	sqlrresultsetrowtranslations		*_sqlrrsrt;
	sqlrresultsetrowblocktranslations	*_sqlrrsrbt;
	sqlrerrortranslations			*_sqlret;
	sqlrtriggers				*_sqlrtr;
	sqlrloggers				*_sqlrlg;
	sqlrnotifications			*_sqlrn;
	sqlrschedules				*_sqlrs;
	sqlrqueries				*_sqlrq;
	sqlrpwdencs				*_sqlrpe;
	sqlrauths				*_sqlra;
	sqlrmoduledatas				*_sqlrmd;

	stringbuffer	_infobuffer;

	filedescriptor	*_clientsock;

	uint16_t	_protocolindex;
	sqlrprotocol	*_currentprotocol;

	const char	*_user;
	const char	*_password;

	bool		_catalogchanged;
	char		*_originalcatalog;

	bool		_schemachanged;
	char		*_originalschema;

	connectstringcontainer	*_constr;

	char		*_updown;

	uint16_t	_inetport;
	stringbuffer	_unixsocket;

	sqlrtxmodel_t	_initialtxmodel;
	sqlrtxmodel_t	_txmodel;
	bool		_intransaction;

	bool		_needscommitorrollback;

	bool		_autocommit;
	bool		_initialautocommit;
	bool		_endtxwithautocommiton;

	bool		_fakeinputbinds;
	bool		_translatebinds;
	bool		_questionmarksupported;
	bool		_colonsupported;
	bool		_atsignsupported;
	bool		_dollarsignsupported;

	const char	*_isolationlevel;

	bool		_sendcolumninfo;

	uint32_t	_accepttimeout;

	bool		_suspendedsession;

	inetsocketserver	**_serversockin;
	uint64_t		_serversockincount;
	unixsocketserver	*_serversockun;

	memorypool	_sessionpool;
	memorypool	_txpool;

	bool		_debugsql;
	bool		_debugbulkload;
	bool		_debugsqlrparser;
	bool		_debugsqlrdirectives;
	bool		_debugsqlrquerytranslations;
	bool		_debugsqlrfilters;
	bool		_debugbindtranslation;
	bool		_debugsqlrbindvariabletranslation;
	bool		_debugsqlrresultsettranslation;
	bool		_debugsqlrresultsetrowtranslation;
	bool		_debugsqlrresultsetrowblocktranslation;
	bool		_debugsqlrresultsetheadertranslation;
	bool		_debugsqlrerrortranslation;
	bool		_debugsqlrmoduledata;

	dynamiclib	_conndl;
	dynamiclib	_sqlrpdl;

	domnode	*_sqlrpnode;

	uint16_t	_cursorcount;
	uint16_t	_mincursorcount;
	uint16_t	_maxcursorcount;
	sqlrservercursor	**_cur;

	char		*_decrypteddbpassword;

	unixsocketclient	_handoffsockun;
	bool			_proxymode;
	uint32_t		_proxypid;

	bool		_connected;
	bool		_inclientsession;
	bool		_loggedin;
	uint32_t	_reloginseed;
	time_t		_relogintime;

	bool		_scalerspawned;
	const char	*_connectionid;
	int32_t		_ttl;

	char		*_pidfile;

	int32_t		_idleclienttimeout;

	bool		_decrementonclose;
	bool		_silent;

	uint32_t	_maxquerysize;
	uint16_t	_maxbindcount;
	uint32_t	_maxerrorsize;

	uint32_t	_fetchatonce;
	uint32_t	_maxcolumncount;
	uint32_t	_maxfieldsize;

	uint64_t	_connecttimeout;
	uint64_t	_querytimeout;
	bool		_executedirect;

	const char	*_dbtype;

	int64_t		_loggedinsec;
	int64_t		_loggedinusec;

	const char	*_dbhostname;
	const char	*_dbipaddress;

	char		*_reformattedvalue;
	uint32_t	_reformattedvaluesize;

	singlylinkedlist< char * >	_globaltemptables;
	bool				_allglobaltemptables;
	singlylinkedlist< char * >	_temptablesfordrop;
	singlylinkedlist< char * >	_temptablesfortrunc;

	dictionary< uint32_t, uint32_t >	*_columnmap;
	dictionary< uint32_t, const char * >	*_columnnamemap;

	dictionary< uint32_t, uint32_t >	_mysqlcatalogscolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqlschemascolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqltabletypescolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqltablescolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqltypeinfocolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqlcolumnscolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqlprimarykeyscolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqlkeyandindexcolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqlprocedurescolumnmap;
	dictionary< uint32_t, uint32_t >	_mysqlprocedureparametercolumnmap;
	dictionary< uint32_t, const char * >	_mysqlcatalogscolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqlschemascolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqltabletypescolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqltablescolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqltypeinfocolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqlcolumnscolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqlprimarykeyscolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqlkeyandindexcolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqlprocedurescolumnnamemap;
	dictionary< uint32_t, const char * >	_mysqlprocedureparametercolumnnamemap;

	dictionary< uint32_t, uint32_t >	_odbccatalogscolumnmap;
	dictionary< uint32_t, uint32_t >	_odbcschemascolumnmap;
	dictionary< uint32_t, uint32_t >	_odbctabletypescolumnmap;
	dictionary< uint32_t, uint32_t >	_odbctablescolumnmap;
	dictionary< uint32_t, uint32_t >	_odbctypeinfocolumnmap;
	dictionary< uint32_t, uint32_t >	_odbccolumnscolumnmap;
	dictionary< uint32_t, uint32_t >	_odbcprimarykeyscolumnmap;
	dictionary< uint32_t, uint32_t >	_odbckeyandindexcolumnmap;
	dictionary< uint32_t, uint32_t >	_odbcprocedurescolumnmap;
	dictionary< uint32_t, uint32_t >	_odbcprocedureparametercolumnmap;
	dictionary< uint32_t, const char * >	_odbccatalogscolumnnamemap;
	dictionary< uint32_t, const char * >	_odbcschemascolumnnamemap;
	dictionary< uint32_t, const char * >	_odbctabletypescolumnnamemap;
	dictionary< uint32_t, const char * >	_odbctablescolumnnamemap;
	dictionary< uint32_t, const char * >	_odbctypeinfocolumnnamemap;
	dictionary< uint32_t, const char * >	_odbccolumnscolumnnamemap;
	dictionary< uint32_t, const char * >	_odbcprimarykeyscolumnnamemap;
	dictionary< uint32_t, const char * >	_odbckeyandindexcolumnnamemap;
	dictionary< uint32_t, const char * >	_odbcprocedurescolumnnamemap;
	dictionary< uint32_t, const char * >	_odbcprocedureparametercolumnnamemap;

	dictionary< uint32_t, uint32_t >	_jdbccatalogscolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbcschemascolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbctabletypescolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbctablescolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbctypeinfocolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbccolumnscolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbcprimarykeyscolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbckeyandindexcolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbcprocedurescolumnmap;
	dictionary< uint32_t, uint32_t >	_jdbcprocedureparametercolumnmap;
	dictionary< uint32_t, const char * >	_jdbccatalogscolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbcschemascolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbctabletypescolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbctablescolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbctypeinfocolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbccolumnscolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbcprimarykeyscolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbckeyandindexcolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbcprocedurescolumnnamemap;
	dictionary< uint32_t, const char * >	_jdbcprocedureparametercolumnnamemap;

	const char	**_columnnames;
	uint16_t	*_columnnamesizes;
	uint16_t	*_columntypes;
	const char	**_columntypenames;
	uint16_t	*_columntypenamesizes;
	uint32_t	*_columnsizes;
	uint32_t	*_columnprecisions;
	uint32_t	*_columnscales;
	uint16_t	*_columnisnullables;
	uint16_t	*_columnisprimarykeys;
	uint16_t	*_columnisuniques;
	uint16_t	*_columnispartofkeys;
	uint16_t	*_columnisunsigneds;
	uint16_t	*_columniszerofilleds;
	uint16_t	*_columnisbinarys;
	uint16_t	*_columnisautoincrements;
	const char	**_columntables;
	uint16_t	*_columntablesizes;

	const char	**_fieldnames;
	const char	**_fields;
	uint64_t	*_fieldsizes;
	bool		*_lobs;
	bool		*_nulls;

	char			*_bulkserveridfilename;
	sharedmemory		*_bulkservershmem;
	byte_t	 		*_bulkservershm;
	byte_t	 		*_bulkservershmquery;
	byte_t	 		*_bulkservershmdataformat;
	sharedmemory		*_bulkclientshmem;
	byte_t			*_bulkclientshm;
	sqlrservercursor	*_bulkcursor;
	const char		*_bulkerrorfieldtable;
	const char		*_bulkerrorrowtable;
	uint64_t		_bulkmaxerrorcount;
	uint64_t		_bulkdroperrortables;
	uint64_t		_bulkquerysize;
	const char		*_bulkquery;
	const byte_t		*_bulkdataformat;
	singlylinkedlist<const byte_t *>	_bulkdata;
	singlylinkedlist<uint64_t>		_bulkdatasize;

	char	*_catalog;
	char	*_schema;
	char	*_object;

	dictionary<char *,linkedlist<char *> *>	_colcache;
	dictionary<char *,const char *>		_autoinccolcache;
	dictionary<char *,const char *>		_primarykeycolcache;

	dictionary< sqlrdatabaseobject *, char * >	_tablenamemap;
	dictionary< sqlrdatabaseobject *, char * >	_indexnamemap;

	dictionary< sqlrservercursor *, bool >		_overrideaffectedrows;
	dictionary< sqlrservercursor *, uint64_t >	_affectedrows;
};

sqlrservercontroller::sqlrservercontroller() : sqlrserverbase() {

	pvt=new sqlrservercontrollerprivate;

	pvt->_conn=NULL;

	pvt->_sqlrcfgs=NULL;
	pvt->_cfg=NULL;
	pvt->_pth=NULL;

	pvt->_debug=false;

	pvt->_shm=NULL;
	pvt->_connstats=NULL;

	pvt->_cmdl=NULL;
	pvt->_semset=NULL;
	pvt->_shmem=NULL;

	pvt->_updown=NULL;

	pvt->_clientsock=NULL;

	pvt->_protocolindex=0;
	pvt->_currentprotocol=NULL;

	pvt->_user=NULL;
	pvt->_password=NULL;

	pvt->_catalogchanged=false;
	pvt->_originalcatalog=NULL;

	pvt->_schemachanged=false;
	pvt->_originalschema=NULL;

	pvt->_serversockun=NULL;
	pvt->_serversockin=NULL;
	pvt->_serversockincount=0;

	pvt->_inetport=0;

	pvt->_needscommitorrollback=false;

	pvt->_autocommit=false;
	pvt->_initialautocommit=false;
	pvt->_endtxwithautocommiton=false;

	pvt->_initialtxmodel=SQLRTXMODEL_UNKNOWN;
	pvt->_txmodel=SQLRTXMODEL_UNKNOWN;
	pvt->_intransaction=false;

	pvt->_fakeinputbinds=false;
	pvt->_translatebinds=false;
	pvt->_questionmarksupported=true;
	pvt->_colonsupported=true;
	pvt->_atsignsupported=true;
	pvt->_dollarsignsupported=true;

	pvt->_isolationlevel=NULL;

	pvt->_sendcolumninfo=true;

	pvt->_maxquerysize=0;
	pvt->_maxbindcount=0;
	pvt->_maxerrorsize=0;
	pvt->_idleclienttimeout=-1;

	pvt->_fetchatonce=1;
	pvt->_maxcolumncount=0;
	pvt->_maxfieldsize=0;

	pvt->_connecttimeout=0;
	pvt->_querytimeout=0;
	pvt->_executedirect=false;

	pvt->_dbtype=NULL;

	pvt->_connected=false;
	pvt->_inclientsession=false;
	pvt->_loggedin=false;
	pvt->_reloginseed=0;
	pvt->_relogintime=0;

	pvt->_sqlrpr=NULL;
	pvt->_sqlrp=NULL;
	pvt->_sqlrd=NULL;
	pvt->_sqlrt=NULL;
	pvt->_sqlrf=NULL;
	pvt->_sqlrbvt=NULL;
	pvt->_sqlrrsht=NULL;
	pvt->_sqlrrst=NULL;
	pvt->_sqlrrsrt=NULL;
	pvt->_sqlrrsrbt=NULL;
	pvt->_sqlret=NULL;
	pvt->_sqlrtr=NULL;
	pvt->_sqlrlg=NULL;
	pvt->_sqlrn=NULL;
	pvt->_sqlrs=NULL;
	pvt->_sqlrq=NULL;
	pvt->_sqlrpe=NULL;
	pvt->_sqlra=NULL;
	pvt->_sqlrmd=NULL;

	pvt->_decrypteddbpassword=NULL;

	pvt->_debugsql=false;
	pvt->_debugbulkload=false;
	pvt->_debugsqlrparser=false;
	pvt->_debugsqlrdirectives=false;
	pvt->_debugsqlrquerytranslations=false;
	pvt->_debugsqlrfilters=false;
	pvt->_debugbindtranslation=false;
	pvt->_debugsqlrbindvariabletranslation=false;
	pvt->_debugsqlrresultsettranslation=false;
	pvt->_debugsqlrresultsetrowtranslation=false;
	pvt->_debugsqlrresultsetrowblocktranslation=false;
	pvt->_debugsqlrresultsetheadertranslation=false;
	pvt->_debugsqlrmoduledata=false;

	pvt->_cur=NULL;

	pvt->_pidfile=NULL;

	pvt->_decrementonclose=false;
	pvt->_silent=false;

	pvt->_loggedinsec=0;
	pvt->_loggedinusec=0;

	pvt->_dbhostname=NULL;
	pvt->_dbipaddress=NULL;

	pvt->_reformattedvalue=NULL;
	pvt->_reformattedvaluesize=0;

	pvt->_globaltemptables.setManageArrayValues(true);
	pvt->_allglobaltemptables=false;

	pvt->_proxymode=false;
	pvt->_proxypid=0;

	pvt->_columnmap=NULL;
	pvt->_columnnamemap=NULL;

	pvt->_bulkserveridfilename=NULL;
	pvt->_bulkservershmem=NULL;
	pvt->_bulkservershm=NULL;
	pvt->_bulkservershmquery=NULL;
	pvt->_bulkservershmdataformat=NULL;
	pvt->_bulkclientshmem=NULL;
	pvt->_bulkclientshm=NULL;
	pvt->_bulkcursor=NULL;
	pvt->_bulkerrorfieldtable=NULL;
	pvt->_bulkerrorrowtable=NULL;
	pvt->_bulkmaxerrorcount=0;
	pvt->_bulkquerysize=0;
	pvt->_bulkquery=NULL;
	pvt->_bulkdataformat=NULL;

	pvt->_catalog=NULL;
	pvt->_schema=NULL;
	pvt->_object=NULL;

	pvt->_colcache.setManageArrayKeys(true);
	pvt->_colcache.setManageValues(true);
}

sqlrservercontroller::~sqlrservercontroller() {

	shutDown();

	if (pvt->_connstats) {
		bytestring::zero(pvt->_connstats,sizeof(sqlrconnstatistics));
	}

	delete pvt->_cmdl;
	delete pvt->_sqlrcfgs;

	delete[] pvt->_updown;

	delete[] pvt->_originalcatalog;
	delete[] pvt->_originalschema;

	delete pvt->_pth;

	delete pvt->_shmem;

	delete pvt->_semset;

	if (pvt->_unixsocket.getSize()) {
		file::remove(pvt->_unixsocket.getString());
	}

	delete pvt->_sqlrpr;
	delete pvt->_sqlrp;
	delete pvt->_sqlrd;
	delete pvt->_sqlrt;
	delete pvt->_sqlrf;
	delete pvt->_sqlrbvt;
	delete pvt->_sqlrrsht;
	delete pvt->_sqlrrst;
	delete pvt->_sqlrrsrt;
	delete pvt->_sqlrrsrbt;
	delete pvt->_sqlret;
	delete pvt->_sqlrtr;
	delete pvt->_sqlrlg;
	delete pvt->_sqlrn;
	delete pvt->_sqlrs;
	delete pvt->_sqlrq;
	delete pvt->_sqlrpe;
	delete pvt->_sqlra;
	delete pvt->_sqlrmd;

	delete[] pvt->_decrypteddbpassword;

	if (pvt->_pidfile) {
		file::remove(pvt->_pidfile);
		delete[] pvt->_pidfile;
	}

	delete[] pvt->_reformattedvalue;

	delete pvt->_conn;

	if (pvt->_bulkserveridfilename) {
		file::remove(pvt->_bulkserveridfilename);
		delete[] pvt->_bulkserveridfilename;
	}
	delete pvt->_bulkservershmem;
	delete pvt->_bulkclientshmem;
	delete pvt->_bulkcursor;

	delete[] pvt->_catalog;
	delete[] pvt->_schema;
	delete[] pvt->_object;

	delete pvt;
}

bool sqlrservercontroller::init(int argc, const char **argv) {

	// process command line
	pvt->_cmdl=new sqlrcmdline(argc,argv);

	// initialize paths
	pvt->_pth=new sqlrpaths(pvt->_cmdl);

	// create the pid file as early as possible, but especially before
	// login,  in case login takes a while and someone runs sqlr-stop while
	// we're logging in
	createPidFile();

	// default id warning
	if (!charstring::compare(pvt->_cmdl->getId(),DEFAULT_ID)) {
		stderror.printf("Warning: using default id.\n");
	}

	// get whether this connection was spawned by the scaler
	pvt->_scalerspawned=pvt->_cmdl->isFound("-scaler");

	// get the connection id from the command line
	pvt->_connectionid=pvt->_cmdl->getValue("-connectionid");
	if (charstring::isNullOrEmpty(pvt->_connectionid)) {
		pvt->_connectionid=DEFAULT_CONNECTIONID;
		stderror.printf("Warning: using default connectionid.\n");
	}

	// get the time to live from the command line
	const char	*ttlstr=pvt->_cmdl->getValue("-ttl");
	pvt->_ttl=(!charstring::isNullOrEmpty(ttlstr))?
				charstring::convertToInteger(ttlstr):-1;

	// should we run quietly?
	pvt->_silent=pvt->_cmdl->isFound("-silent");

	// load the configuration
	pvt->_sqlrcfgs=new sqlrconfigs(pvt->_pth);
	pvt->_cfg=pvt->_sqlrcfgs->load(pvt->_pth->getConfigUrl(),
						pvt->_cmdl->getId());
	if (!pvt->_cfg) {
		return false;
	}

	setUserAndGroup();

	// update various configurable parameters
	pvt->_maxquerysize=pvt->_cfg->getMaxQuerySize();
	pvt->_maxbindcount=pvt->_cfg->getMaxBindCount();
	pvt->_maxerrorsize=pvt->_cfg->getMaxErrorSize();
	pvt->_idleclienttimeout=pvt->_cfg->getIdleClientTimeout();
	pvt->_debug=pvt->_cfg->getDebugConnections();
	pvt->_debugsql=pvt->_cfg->getDebugSql();
	pvt->_debugbulkload=pvt->_cfg->getDebugBulkLoad();

	// initialize logger modules
	domnode	*loggers=pvt->_cfg->getLoggers();
	if (!loggers->isNullNode()) {
		pvt->_sqlrlg=new sqlrloggers(pvt->_pth,loggers);
		pvt->_sqlrlg->load();
		pvt->_sqlrlg->init(NULL,this);
	}

	// initialize password encryption modules
	domnode	*pwdencs=pvt->_cfg->getPasswordEncryptions();
	if (!pwdencs->isNullNode()) {
		pvt->_sqlrpe=new sqlrpwdencs(
				pvt->_pth,
				pvt->_cfg->getDebugPasswordEncryptions(),
				pwdencs);
		pvt->_sqlrpe->load();
	}	

	// initialize auth modules
	domnode	*auths=pvt->_cfg->getAuths();
	if (!auths->isNullNode()) {
		pvt->_sqlra=new sqlrauths(this,auths);
		pvt->_sqlra->load();
	}

	// initialize database connection module
	pvt->_conn=initConnection(pvt->_cfg->getDbase());
	if (!pvt->_conn) {
		return false;
	}

	// initialize transaction model -- the configured value is the
	// "initial" model that we'll restore at end-of-session; the
	// active _txmodel starts out as a copy of it
	const char	*txmodel=pvt->_cfg->getTransactionModel();
	if (!charstring::compare(txmodel,"native")) {
		pvt->_initialtxmodel=pvt->_conn->getNativeTransactionModel();
	} else {
		pvt->_initialtxmodel=stringToTransactionModel(txmodel);
	}
	pvt->_txmodel=pvt->_initialtxmodel;

	buildColumnMaps();

	// initialize notification modules
	domnode	*notifications=pvt->_cfg->getNotifications();
	if (!notifications->isNullNode()) {
		pvt->_sqlrn=new sqlrnotifications(pvt->_pth,notifications);
		pvt->_sqlrn->load();
	}

	// initialize schedule modules
	domnode	*schedules=pvt->_cfg->getSchedules();
	if (!schedules->isNullNode()) {
		pvt->_sqlrs=new sqlrschedules(this,schedules);
		pvt->_sqlrs->load();
	}

	// wait for listener startup
	if (!waitForListenerStartup()) {
		return false;
	}

	// handle the connect string
	pvt->_constr=pvt->_cfg->getConnectString(pvt->_connectionid);
	if (!pvt->_constr) {
		stderror.printf("Error: invalid connectionid \"%s\".\n",
							pvt->_connectionid);
		return false;
	}
	pvt->_conn->handleConnectString();

	initDatabaseAvailableFileName();

	// set unix socket filename (for suspended/resumed sessions)
	pvt->_unixsocket.append(pvt->_pth->getSocketsDir())->
				append((uint32_t)process::getProcessId())->
				append(".sock");

	if (!createSharedMemoryAndSemaphores(pvt->_cmdl->getId())) {
		return false;
	}

	// if there's no way to interrupt a semaphore wait,
	// then force the ttl to zero
	if (pvt->_ttl>0 &&
			!pvt->_semset->supportsTimedSemaphoreOperations() &&
			!sys::getSignalsInterruptSystemCalls()) {
		pvt->_ttl=0;
	}

	// detach
	if (pvt->_conn->mustDetachBeforeLogIn() &&
			!pvt->_cmdl->isFound("-nodetach")) {

		process::detach();

		// (re)create the pid file with the pid of the detached process
		createPidFile();
	}

	// log in
	bool	reloginatstart=pvt->_cfg->getReLoginAtStart();
	if (!reloginatstart) {

		if (!attemptLogIn(!pvt->_silent)) {
			return false;
		}

	} else {

		while (!attemptLogIn(false)) {
			snooze::macrosnooze(5);
			if (process::getShutDownFlag()) {
				return false;
			}
		}
	}

	// detach
	if (!pvt->_conn->mustDetachBeforeLogIn() &&
			!pvt->_cmdl->isFound("-nodetach")) {

		process::detach();

		// (re)create the pid file with the pid of the detached process
		createPidFile();
	}

	// init connection stats
	initConnStats();

	// initialize module data modules
	pvt->_debugsqlrmoduledata=pvt->_cfg->getDebugModuleDatas();
	domnode	*moduledatas=pvt->_cfg->getModuleDatas();
	if (!moduledatas->isNullNode()) {
		pvt->_sqlrmd=new sqlrmoduledatas(this,moduledatas);
		pvt->_sqlrmd->load();
	}

	// initialize query directive modules
	pvt->_debugsqlrdirectives=pvt->_cfg->getDebugDirectives();
	domnode	*directives=pvt->_cfg->getDirectives();
	if (!directives->isNullNode()) {
		pvt->_sqlrd=new sqlrdirectives(this,directives);
		pvt->_sqlrd->load();
	}

	// initialize query translation modules
	pvt->_debugsqlrquerytranslations=pvt->_cfg->getDebugQueryTranslations();
	domnode	*querytranslations=pvt->_cfg->getQueryTranslations();
	if (!querytranslations->isNullNode()) {
		pvt->_sqlrp=newParser();
		pvt->_sqlrt=new sqlrquerytranslations(this,querytranslations);
		pvt->_sqlrt->load();
	}

	// initialize query filter modules
	pvt->_debugsqlrfilters=pvt->_cfg->getDebugFilters();
	domnode	*filters=pvt->_cfg->getFilters();
	if (!filters->isNullNode()) {
		if (!pvt->_sqlrp) {
			pvt->_sqlrp=newParser();
		}
		pvt->_sqlrf=new sqlrfilters(this,filters);
		pvt->_sqlrf->load();
	}

	// initialize bind variable translation modules
	pvt->_debugsqlrbindvariabletranslation=
				pvt->_cfg->getDebugBindVariableTranslations();
	domnode	*bindvariabletranslations=
				pvt->_cfg->getBindVariableTranslations();
	if (!bindvariabletranslations->isNullNode()) {
		pvt->_sqlrbvt=new sqlrbindvariabletranslations(
					this,bindvariabletranslations);
		pvt->_sqlrbvt->load();
	}

	// initialize result set header translation modules
	pvt->_debugsqlrresultsetheadertranslation=
			pvt->_cfg->getDebugResultSetHeaderTranslations();
	domnode	*resultsetheadertranslations=
			pvt->_cfg->getResultSetHeaderTranslations();
	if (!resultsetheadertranslations->isNullNode()) {
		pvt->_sqlrrsht=new sqlrresultsetheadertranslations(
					this,resultsetheadertranslations);
		pvt->_sqlrrsht->load();
	}

	// initialize result set translation modules
	pvt->_debugsqlrresultsettranslation=
				pvt->_cfg->getDebugResultSetTranslations();
	domnode	*resultsettranslations=
				pvt->_cfg->getResultSetTranslations();
	if (!resultsettranslations->isNullNode()) {
		pvt->_sqlrrst=new sqlrresultsettranslations(
					this,resultsettranslations);
		pvt->_sqlrrst->load();
	}

	// initialize result set row translation modules
	pvt->_debugsqlrresultsetrowtranslation=
				pvt->_cfg->getDebugResultSetRowTranslations();
	domnode	*resultsetrowtranslations=
				pvt->_cfg->getResultSetRowTranslations();
	if (!resultsetrowtranslations->isNullNode()) {
		pvt->_sqlrrsrt=new sqlrresultsetrowtranslations(
					this,resultsetrowtranslations);
		pvt->_sqlrrsrt->load();
	}

	// initialize result set row block translation modules
	pvt->_debugsqlrresultsetrowblocktranslation=
			pvt->_cfg->getDebugResultSetRowBlockTranslations();
	domnode	*resultsetrowblocktranslations=
				pvt->_cfg->getResultSetRowBlockTranslations();
	if (!resultsetrowblocktranslations->isNullNode()) {
		pvt->_sqlrrsrbt=new sqlrresultsetrowblocktranslations(
					this,resultsetrowblocktranslations);
		pvt->_sqlrrsrbt->load();
	}

	// initialize error translation modules
	pvt->_debugsqlrerrortranslation=
			pvt->_cfg->getDebugErrorTranslations();
	domnode	*errortranslations=
			pvt->_cfg->getErrorTranslations();
	if (!errortranslations->isNullNode()) {
		pvt->_sqlret=new sqlrerrortranslations(this,errortranslations);
		pvt->_sqlret->load();
	}

	// initialize trigger modules
	domnode	*triggers=pvt->_cfg->getTriggers();
	if (!triggers->isNullNode()) {
		// for triggers, we'll need an sqlrparser as well
		if (!pvt->_sqlrp) {
			pvt->_sqlrp=newParser();
		}
		pvt->_sqlrtr=new sqlrtriggers(this,triggers);
		pvt->_sqlrtr->load();
	}

	// get fake input bind variable behavior
	// (this may have already been set true by the connect string)
	pvt->_fakeinputbinds=(pvt->_fakeinputbinds ||
				pvt->_cfg->getFakeInputBindVariables());

	// get translate bind variable behavior
	pvt->_translatebinds=pvt->_cfg->getTranslateBindVariables();
	pvt->_questionmarksupported=
		pvt->_cfg->getBindVariableDelimiterQuestionMarkSupported();
	pvt->_colonsupported=
		pvt->_cfg->getBindVariableDelimiterColonSupported();
	pvt->_atsignsupported=
		pvt->_cfg->getBindVariableDelimiterAtSignSupported();
	pvt->_dollarsignsupported=
		pvt->_cfg->getBindVariableDelimiterDollarSignSupported();
	pvt->_debugbindtranslation=pvt->_cfg->getDebugBindTranslations();

	// initialize cursors
	pvt->_mincursorcount=pvt->_cfg->getCursors();
	pvt->_maxcursorcount=pvt->_cfg->getMaxCursors();
	if (!initCursors(pvt->_mincursorcount)) {
		closeCursors(false);
		logOut();
		return false;
	}

	// increment connection counter
	if (pvt->_cfg->getDynamicScaling()) {
		incrementConnectionCount();
	}

	// set the isolation level
	pvt->_isolationlevel=pvt->_cfg->getIsolationLevel();
	pvt->_conn->setIsolationLevel(pvt->_isolationlevel);

	// get the catalog we're using so
	// we can switch back to it at end of session
	pvt->_originalcatalog=pvt->_conn->getCurrentCatalog();

	// get the schema we're using so
	// we can switch back to it at end of session
	pvt->_originalschema=pvt->_conn->getCurrentSchema();

	markDatabaseAvailable();

	// get the custom query handlers
	domnode	*queries=pvt->_cfg->getQueries();
	if (!queries->isNullNode()) {
		pvt->_sqlrq=new sqlrqueries(this,queries);
		pvt->_sqlrq->load();
	}

	// init client protocols
	pvt->_sqlrpr=new sqlrprotocols(this,pvt->_cfg->getListeners());
	pvt->_sqlrpr->load();

	sqlrserverbase::init();

	return true;
}

void sqlrservercontroller::createPidFile() {

	// if a pid file already exists, then remove it
	if (pvt->_pidfile) {
		file::remove(pvt->_pidfile);
		delete[] pvt->_pidfile;
	}

	// create a pid file
	pid_t	pid=process::getProcessId();
	charstring::printf(&pvt->_pidfile,
				"%ssqlr-connection-%s.%ld.pid",
				pvt->_pth->getPidDir(),
				pvt->_cmdl->getId(),
				(long)pid);
	process::createPidFile(pvt->_pidfile,permissions::getOwnerReadWrite());

	// There is, unfortunately an unavoidable race condition here.  This
	// is called right after detach(), but there's no way to detach, delete
	// the original pid file, and create another one in an atomic operation.
	// And, there's no telling when sqlr-stop will be run.  It could happen
	// somewhere in the middle of all of this and either errnoeusly attempt
	// to kill the parent process or just not see a pid file at all.  Not
	// sure what to do about that.
}

void sqlrservercontroller::setUserAndGroup() {

	// get the user that we're currently running as
	char	*currentuser=
		userentry::getName(process::getEffectiveUserId());

	// get the group that we're currently running as
	char	*currentgroup=
		groupentry::getName(process::getEffectiveGroupId());

	stringbuffer	errorstr;

	// switch groups, but only if we're not currently running as the
	// group that we should switch to
	if (charstring::compare(currentgroup,pvt->_cfg->getRunAsGroup()) &&
			!process::setGroup(pvt->_cfg->getRunAsGroup())) {
		errorstr.append("Warning: could not change group to ")->
			append(pvt->_cfg->getRunAsGroup())->append('\n');
	}

	// switch users, but only if we're not currently running as the
	// user that we should switch to
	if (charstring::compare(currentuser,pvt->_cfg->getRunAsUser()) &&
			!process::setUser(pvt->_cfg->getRunAsUser())) {
		errorstr.append("Warning: could not change user to ")->
			append(pvt->_cfg->getRunAsUser())->append('\n');
	}

	// write the error, if there was one
	stderror.write(errorstr.getString(),errorstr.getSize());

	// clean up
	delete[] currentuser;
	delete[] currentgroup;
}

sqlrserverconnection *sqlrservercontroller::initConnection(const char *dbase) {

#ifdef SQLRELAY_ENABLE_SHARED
	// load the connection module
	stringbuffer	modulename;
	modulename.append(pvt->_pth->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("connection_");
	modulename.append(dbase)->append(".")->append(SQLRELAY_MODULESUFFIX);
	if (!pvt->_conndl.open(modulename.getString(),true,true)) {
		char	*error=pvt->_conndl.getError();
		stderror.printf("failed to load connection module: %s\n%s\n",
				modulename.getString(),(error)?error:"");
		delete[] error;
		return NULL;
	}

	// load the connection itself
	stringbuffer	functionname;
	functionname.append("new_")->append(dbase)->append("connection");
	sqlrserverconnection	*(*newConn)(sqlrservercontroller *)=
			(sqlrserverconnection *(*)(sqlrservercontroller *))
			pvt->_conndl.getSymbol(functionname.getString());
	if (!newConn) {
		char	*error=pvt->_conndl.getError();
		stderror.printf("failed to load connection: %s\n%s\n",
				dbase,(error)?error:"");
		delete[] error;
		return NULL;
	}

	sqlrserverconnection	*conn=(*newConn)(this);

#else
	sqlrserverconnection	*conn;
	#include "sqlrserverconnectionassignments.cpp"
	{
		conn=NULL;
	}
#endif
	return conn;
}

bool sqlrservercontroller::waitForListenerStartup() {

	// check for listener's pid file
	// (Look a few times.  It might not be there right away.  The listener
	// writes it out after forking and it's possible that the connection
	// might start up after the sqlr-listener has forked, but before it
	// writes out the pid file)
	char	*listenerpidfile=NULL;
	charstring::printf(&listenerpidfile,
				"%ssqlr-listener-%s.pid",
				pvt->_pth->getPidDir(),
				pvt->_cmdl->getId());

	// On most platforms, 1 second is plenty of time to wait for the
	// listener to come up, but on windows, it can take a while longer.
	// On 64-bit windows, when running 32-bit apps, listening on an inet
	// socket can take an especially long time.
	uint16_t	listenertimeout=10;
	char		*osname=sys::getOperatingSystemName();
	if (!charstring::compareIgnoringCase(osname,"Windows")) {
		listenertimeout=100;
		if ((!charstring::compareIgnoringCase(
				sys::getOperatingSystemArchitecture(),
				"x86_64") ||
			!charstring::compareIgnoringCase(
				sys::getOperatingSystemArchitecture(),
				"amd64")) &&
			sizeof(void *)==4) {
			listenertimeout=200;
		}
	}
	delete[] osname;

	bool	retval=true;
	bool	found=false;
	for (uint16_t i=0; !found && i<listenertimeout; i++) {
		if (process::getShutDownFlag()) {
			retval=false;
			break;
		}
		if (i) {
			snooze::microsnooze(0,100000);
		}
		found=(process::checkForPidFile(listenerpidfile)!=-1);
	}
	if (!found) {
		stderror.printf("\n%s-connection error:\n"
				"	The pid file %s"
				" was not found.\n"
				"	This usually means "
					"that the %s-listener \n"
				"is not running.\n"
				"	The %s-listener must be running "
				"for the %s-connection to start.\n\n",
				SQLR,listenerpidfile,SQLR,SQLR,SQLR);
		retval=false;
	}

	delete[] listenerpidfile;

	return retval;
}

void sqlrservercontroller::initDatabaseAvailableFileName() {

	// initialize the database up/down filename
	charstring::printf(&pvt->_updown,
				"%s%s-%s.up",
				pvt->_pth->getIpcDir(),
				pvt->_cmdl->getId(),
				pvt->_connectionid);
}

bool sqlrservercontroller::attemptLogIn(bool printerrors) {

	debugStart("log in");

	// log in
	if (!logIn(printerrors)) {
		return false;
	}

	// get stats
	datetime	dt;
	dt.initFromSystemDateTime();
	pvt->_loggedinsec=dt.getSecond();
	pvt->_loggedinusec=dt.getMicrosecond();

	debugEnd();
	return true;
}

bool sqlrservercontroller::logIn(bool printerrors) {

	// don't do anything if we're already logged in
	if (pvt->_loggedin) {
		return true;
	}

	// attempt to log in
	const char	*err=NULL;
	const char	*warning=NULL;
	if (!pvt->_conn->logIn(&err,&warning)) {
		if (printerrors) {
			stringbuffer	loginerror;
			loginerror.append("Couldn't log into database.\n");
			if (err) {
				loginerror.append(err)->append('\n');
			}
			stderror.write(loginerror.getString(),
						loginerror.getSize());
		}
		if (pvt->_sqlrlg) {
			raiseInternalErrorEvent(NULL,
				"database login failed: %s",
				(err)?err:"");
		}
		return false;
	}
	if (warning) {
		if (printerrors) {
			stderror.printf("Warning logging into database.\n%s\n",
					(warning)?warning:"");
		}
		if (pvt->_sqlrlg) {
			raiseInternalWarningEvent(NULL,
				"database login warning: %s",
				(warning)?warning:"");
		}
	}

	// initialize transaction and autocommit state
	pvt->_intransaction=
		(pvt->_conn->getNativeTransactionModel()!=SQLRTXMODEL_NONE &&
		pvt->_conn->getNativeTransactionModel()!=SQLRTXMODEL_IMPLICIT);
	pvt->_autocommit=!pvt->_intransaction;

// I'm not convinced we need this.
#if 0
	// bootstrap the autocommit state to match the flag
	// (use only pvt->_conn calls here since we're bootstrapping)
	// (ignore errors, in case the db throws an error if you commit
	// outside of a tx, begin inside one, or set autocommit on/off when
	// it already is)
	if (pvt->_conn->getNativeTransactionModel()!=SQLRTXMODEL_NONE) {
		if (pvt->_conn->supportsAutoCommit()) {
			// this branch handles all implicit-tx dbs
			// (which must support autocommit) and explicit-tx
			// dbs which also support autocommit
			if (pvt->_autocommit) {
				pvt->_conn->setAutoCommitOn();
			} else {
				pvt->_conn->setAutoCommitOff();
			}
		} else {
			// if we're here then the db must be an explicit-tx
			// db that doesn't support autocommit
			if (pvt->_autocommit) {
				pvt->_conn->commit();
			} else {
				pvt->_conn->begin();
			}
		}
	}
#endif

	// set the autocommit state that we want to be in
	setAutoCommit(pvt->_initialautocommit);

	// success... update stats
	incrementOpenDatabaseConnections();

	// update db host name and ip address
	// (Only do this if logging is enabled.  The loggers use them, and if
	// someone forgot to put the database host name in DNS then it can
	// cause the connection to delay until a DNS timeout occurs.)
	if (pvt->_sqlrlg) {
		// this will cause the db host name and ip address
		// to be fetched from the db and stored locally
		getDbHostName();
	}

	pvt->_loggedin=true;

	// If the db is behind a load balancer, we need to re-login
	// periodically to redistribute connections over newly added nodes.
	// Determine when to re-login next.
	if (pvt->_constr->getBehindLoadBalancer()) {
		datetime	dt;
		if (dt.initFromSystemDateTime()) {
			if (!pvt->_reloginseed) {
				// Ideally we'd use randomnumber:getSeed for
				// this, but on some platforms that's generated
				// from the epoch and could end up being the
				// same for all sqlr-connections.  The process
				// id is guaranteed unique.
				pvt->_reloginseed=process::getProcessId();
			}
			pvt->_reloginseed=randomnumber::generate(
							pvt->_reloginseed);
			int32_t	seconds=randomnumber::scale(
							pvt->_reloginseed,
							600,900);
			pvt->_relogintime=dt.getEpoch()+seconds;
		}
	}

	raiseDbLogInEvent();

	return true;
}

void sqlrservercontroller::logOut() {

	// don't do anything if we're already logged out
	if (!pvt->_loggedin) {
		return;
	}

	debugStart("log out");

	// log out
	pvt->_conn->logOut();

	// update stats
	decrementOpenDatabaseConnections();

	raiseDbLogOutEvent();

	pvt->_loggedin=false;

	debugWrite("success");
	debugEnd();
}

bool sqlrservercontroller::setAutoCommit(bool ac) {
	debugStart("set autocommit");
	if (ac) {
		debugWrite("on");
		if (!setAutoCommitOn()) {
			debugWrite("failed");
			stderror.printf("Couldn't set autocommit on.\n");
			debugEnd();
			return false;
		}
	} else {
		debugWrite("off");
		if (!setAutoCommitOff()) {
			debugWrite("failed");
			stderror.printf("Couldn't set autocommit off.\n");
			debugEnd();
			return false;
		}
	}
	debugWrite("success");
	debugEnd();
	return true;
}

bool sqlrservercontroller::initCursors(uint16_t count) {

	debugStart("initializing cursors");
	debugWrite("maxcursorcount: %hd",pvt->_maxcursorcount);
	debugWrite("cursorcount: %hd",pvt->_cursorcount);

	pvt->_cursorcount=count;
	if (!pvt->_cur) {
		pvt->_cur=new sqlrservercursor *[pvt->_maxcursorcount];
		bytestring::zero(pvt->_cur,
				pvt->_maxcursorcount*
				sizeof(sqlrservercursor *));
	}

	for (uint16_t i=0; i<pvt->_cursorcount; i++) {

		if (!pvt->_cur[i]) {
			pvt->_cur[i]=newCursor(i);
		}
		if (!open(pvt->_cur[i])) {
			raiseInternalErrorEvent(
				NULL,"cursor init failed: %hd",i);
			debugEnd();
			return false;
		}
	}

	debugWrite("success");
	debugEnd();
	return true;
}

sqlrservercursor *sqlrservercontroller::newCursor(uint16_t id) {
	sqlrservercursor	*cursor=pvt->_conn->newCursor(id);
	if (cursor) {
		incrementOpenDatabaseCursors();
	}
	return cursor;
}

sqlrservercursor *sqlrservercontroller::newCursor() {
	// return a cursor with an ID that can't already exist
	return newCursor(pvt->_cursorcount+1);
}

void sqlrservercontroller::incrementConnectionCount() {

	debugStart("incrementing connection count");

	if (pvt->_scalerspawned) {

		debugWrite("scaler will do the job");
		signalScalerToRead();

	} else {

		acquireConnectionCountMutex();

		// increment the counter
		pvt->_shm->totalconnections++;
		pvt->_decrementonclose=true;

		releaseConnectionCountMutex();
	}

	debugEnd();
}

void sqlrservercontroller::decrementConnectionCount() {

	debugStart("decrementing connection count");

	if (pvt->_scalerspawned) {

		debugWrite("scaler will do the job");

	} else {

		acquireConnectionCountMutex();

		if (pvt->_shm->totalconnections) {
			pvt->_shm->totalconnections--;
		}
		pvt->_decrementonclose=false;

		releaseConnectionCountMutex();
	}

	debugEnd();
}

void sqlrservercontroller::markDatabaseAvailable() {

	debugStart("marking database available");

	debugWrite("creating %s",pvt->_updown);

	// the database is up if the file is there, 
	// opening and closing it will create it
	file	fd;
	fd.create(pvt->_updown,permissions::getOwnerReadWrite());

	debugEnd();
}

void sqlrservercontroller::markDatabaseUnavailable() {

	debugStart("marking database unavailable");

	// if the database is behind a load balancer, don't mark it unavailable
	if (pvt->_constr->getBehindLoadBalancer()) {
		return;
	}

	debugWrite("unlinking %s",pvt->_updown);

	// the database is down if the file isn't there
	file::remove(pvt->_updown);

	debugEnd();
}

bool sqlrservercontroller::openSockets() {

	debugStart("listening on sockets");

	// open the unix socket
	if (pvt->_cfg->getListenOnUnix() && !pvt->_serversockun) {

		pvt->_serversockun=new unixsocketserver();
		if (pvt->_serversockun->listen(
				pvt->_unixsocket.getString(),0000,128)) {
			debugWrite("listening on unix socket: %s",
						pvt->_unixsocket.getString());
			pvt->_lsnr.addReadFileDescriptor(pvt->_serversockun);
		} else {
			debugWrite(
					"failed to listen on socket: %s",
					pvt->_unixsocket.getString());
			raiseInternalErrorEvent(NULL,
					"failed to listen on socket: %s",
					pvt->_unixsocket.getString());

			char	*currentuser=
					userentry::getName(
						process::getEffectiveUserId());
			char	*currentgroup=
					groupentry::getName(
						process::getEffectiveGroupId());
			stderror.printf("Could not listen on unix socket: %s\n"
					"Make sure that the directory is "
					"writable by %s:%s.\n\n",
					pvt->_unixsocket.getString(),
					currentuser,currentgroup);
			delete[] currentuser;
			delete[] currentgroup;
			delete pvt->_serversockun;
			pvt->_serversockun=NULL;
			return false;
		}
	}

	bool	retval=true;

	// open the next available inet socket
	if (pvt->_cfg->getListenOnInet() && !pvt->_serversockin) {

		const char	*addresses=pvt->_cfg->getDefaultAddresses();

		char		**addr=NULL;
		uint64_t	addrcount=0;
		charstring::split(addresses,",",true,&addr,&addrcount);

		pvt->_serversockincount=addrcount;
		pvt->_serversockin=new inetsocketserver *[addrcount];

		uint64_t	index=0;
		for (index=0; index<pvt->_serversockincount; index++) {
			pvt->_serversockin[index]=NULL;
			if (!retval) {
				continue;
			}
			pvt->_serversockin[index]=new inetsocketserver();
			if (pvt->_serversockin[index]->
				listen(addr[index],pvt->_inetport,128)) {

				if (!pvt->_inetport) {
					pvt->_inetport=
					pvt->_serversockin[index]->getPort();
				}

				debugWrite(
					"listening on inet socket: %hd",
					pvt->_inetport);

				pvt->_lsnr.addReadFileDescriptor(
						pvt->_serversockin[index]);

			} else {
				raiseInternalErrorEvent(NULL,
					"failed to listen on port: %hd",
					pvt->_inetport);

				stderror.printf(
					"Could not listen on inet socket: ",
					"%hd\n\n",pvt->_inetport);

				retval=false;
			}
		}

		if (!retval) {
			// clean up
			for (index=0; index<pvt->_serversockincount; index++) {
				delete pvt->_serversockin[index];
			}
			delete[] pvt->_serversockin;
			pvt->_serversockincount=0;
		}

		// clean up addresses
		for (index=0; index<addrcount; index++) {
			delete[] addr[index];
		}
		delete[] addr;
	}

	debugEnd();

	return retval;
}

bool sqlrservercontroller::listen() {

	uint16_t	sessioncount=0;

	int32_t		softttl=pvt->_cfg->getSoftTtl();
	datetime	startdt;
	startdt.initFromSystemDateTime();

	// loop to handle reconnects, timed out listeners, and other oddities
	for (;;) {

		if (process::getShutDownFlag()) {
			return true;
		}

		waitForAvailableDatabase();
		initSession();
		if (!getAListener(pvt->_connectionid)) {
			return false;
		}

		// loop to handle suspended sessions
		bool	reconnect=false;
		for (;;) {

			if (process::getShutDownFlag()) {
				return false;
			}

			int32_t	success=waitForClient();

			if (success==1) {

				pvt->_suspendedsession=false;

				// have a session with the client
				clientSession();

				// break out of the suspended session loop
				// unless the client suspended the session
				if (!pvt->_suspendedsession) {
					break;
				}

			} else if (success==2) {

				// This is a special case, basically it means
				// that the listener wants the connection to
				// reconnect to the database.  Break out of
				// the suspended session loop and loop back
				// so that can be handled naturally.
				reconnect=true;
				break;

			} else if (success==-1) {

				// If something weird happened, then break out
				// of the suspendedsession loop, loop back, and
				// wait for another client.
				break;

			} else if (success==0) {

				// if waitForClient() fails to wait for someone
				// to pick up the suspended session then roll
				// back and break out of the suspended session
				// loop
				if (isTransactional()) {
					rollback();
				}
				pvt->_suspendedsession=false;
				break;
			}
		}

		if (!reconnect && pvt->_cfg->getDynamicScaling()) {

			decrementConnectedClientCount();

			// for dynamically spawned connections, bail on
			// various conditions...
			if (pvt->_scalerspawned) {

				// if the ttl is 0...
				if (!pvt->_ttl) {
					return true;
				}

				// if we've already handled some number of
				// client sessions...
				if (pvt->_ttl>0 &&
					pvt->_cfg->getMaxSessionCount()) {
					sessioncount++;
					if (sessioncount==
					pvt->_cfg->getMaxSessionCount()) {
						return true;
					}
				}

				// if we've been alive for too long...
				if (softttl>0) {
					datetime	currentdt;
					currentdt.initFromSystemDateTime();
					if (currentdt.getEpoch()-
						startdt.getEpoch()>=softttl) {
						return true;
					}
				}
			}
		}
	}
}

void sqlrservercontroller::waitForAvailableDatabase() {

	debugStart("waiting for available database");

	setState(WAIT_FOR_AVAIL_DB);

	if (!file::exists(pvt->_updown)) {
		debugWrite("database is not available");
		reLogIn(false);
		markDatabaseAvailable();
	}

	debugWrite("success");
	debugEnd();
}

void sqlrservercontroller::reLogIn(bool deadconnection) {

	markDatabaseUnavailable();

	// FIXME: only run these if a dead connection prompted
	// a relogin, not if we couldn't login at startup

	// run the session end queries
	sessionEndQueries();

	// get the current catalog so we can restore it
	char	*currentcatalog=pvt->_conn->getCurrentCatalog();

	// get the current schema so we can restore it
	char	*currentschema=pvt->_conn->getCurrentSchema();

	// get the isolation level so we can restore it
	const char	*isolevel=pvt->_conn->getIsolationLevel();

	debugStart("relogging in");

	// attempt to log in over and over, once every 5 seconds
	int32_t	oldcursorcount=pvt->_cursorcount;
	closeCursors(false);
	logOut();
	for (;;) {

		if (process::getShutDownFlag()) {
			delete[] currentcatalog;
			delete[] currentschema;
			return;
		}
			
		debugWrite("trying...");

		incrementReLogInCount();

		if (logIn(false)) {
			if (!initCursors(oldcursorcount)) {
				closeCursors(false);
				logOut();
			} else {
				break;
			}
		}
		snooze::macrosnooze(5);
	}

	debugEnd();

	// run the session-start queries
	// FIXME: only run these if a dead connection prompted
	// a relogin, not if we couldn't login at startup
	sessionStartQueries();

	// restore the catalog
	pvt->_conn->selectCatalog(currentcatalog);
	delete[] currentcatalog;

	// restore the schema
	pvt->_conn->selectSchema(currentschema);
	delete[] currentschema;

	// restore the isolation level
	pvt->_conn->setIsolationLevel(isolevel);

	markDatabaseAvailable();
}

void sqlrservercontroller::initSession() {

	debugStart("initializing session");

	pvt->_needscommitorrollback=false;
	pvt->_suspendedsession=false;
	for (int32_t i=0; i<pvt->_cursorcount; i++) {
		release(pvt->_cur[i]);
	}
	pvt->_accepttimeout=5;

	debugWrite("success");
	debugEnd();
}

bool sqlrservercontroller::getAListener(const char *connectionid) {

	debugStart("getting a listener");

	// connect to listener if we haven't already
	// and pass it this process's pid
	if (!pvt->_connected) {
		if (!registerForHandoff()) {
			return false;
		}
	}

	// save the original ttl
	int32_t	originalttl=pvt->_ttl;

	// get the time before announcing
	time_t	before=0;
	if (originalttl>0) {
		datetime	dt;
		dt.initFromSystemDateTime();
		before=dt.getEpoch();
	}

	// This will fall through if the ttl was reached while waiting.
	// In that case, since we failed to acquire the announce mutex,
	// we don't need to release it.  We also don't need to reset the
	// ttl because we're going to exit.
	if (!acquireShmAccess()) {
		debugWrite("aborting getting a listener");
		debugEnd();
		return false;
	}

	// reset semaphores
	// FIXME: document why...
	if (!resetSemaphores()) {
		releaseShmAccess();
		debugEnd();
		return false;
	}

	setState(ANNOUNCE_AVAILABILITY);

	// write the connectionid and pid into the segment
	charstring::copy(pvt->_shm->connectionid,
				connectionid,MAXCONNECTIONIDLEN);
	pvt->_shm->connectioninfo.connectionpid=process::getProcessId();

	signalListenerToRead();

	// get the time after announcing and update the ttl
	if (originalttl>0) {
		datetime	dt;
		dt.initFromSystemDateTime();
		pvt->_ttl=pvt->_ttl-(dt.getEpoch()-before);
	}

	// this will fall through if the ttl was reached while waiting
	bool	success=false;
	if (originalttl<=0 || pvt->_ttl) {
		success=waitForListenerToFinishReading();
	}

	// release the announce mutex earlier
	releaseShmAccess();

	if (success) {

		// reset ttl
		pvt->_ttl=originalttl;

	} else {
		// a timeout must have occurred...

		// Close the handoff socket.  At this point, the listener
		// will have read the connection data and will attempt to
		// hand off the client to this connection.  The socket must
		// be closed when it tries so the handoff will fail and the
		// listener can loop back and try again with a different
		// connection.
		pvt->_handoffsockun.close();

		debugWrite(
			"ttl reached, aborting getting a listener");
	}

	debugEnd();
	return success;
}

bool sqlrservercontroller::registerForHandoff() {

	debugStart("registering for handoff");

	// construct the name of the socket to connect to
	char	*handoffsockname=NULL;
	charstring::printf(&handoffsockname,
				"%s%s-handoff.sock",
				pvt->_pth->getSocketsDir(),
				pvt->_cmdl->getId());

	debugWrite("handoffsockname: %s",handoffsockname);

	// Try to connect over and over forever on 1 second intervals.
	// If the connect succeeds but the write fails, loop back and
	// try again.
	pvt->_connected=false;
	for (;;) {

		if (process::getShutDownFlag()) {
			debugWrite("shutdown");
			break;
		}

		pvt->_handoffsockun.setFileName(handoffsockname);
		pvt->_handoffsockun.setRetryWait(1);
		if (pvt->_handoffsockun.connect()==RESULT_SUCCESS) {
			if (pvt->_handoffsockun.write(
				(uint32_t)process::getProcessId())==
							sizeof(uint32_t)) {
				pvt->_connected=true;
				break;
			}
			pvt->_handoffsockun.flushWriteBuffer(-1,-1);
			deRegisterForHandoff();
		}

		debugWrite("sleeping 1s...");
		snooze::macrosnooze(1);
	}

	debugWrite("success");
	debugEnd();

	delete[] handoffsockname;

	return pvt->_connected;
}

void sqlrservercontroller::deRegisterForHandoff() {
	
	debugStart("de-registering for handoff");

	// construct the name of the socket to connect to
	char	*removehandoffsockname=NULL;
	charstring::printf(&removehandoffsockname,
				"%s%s-removehandoff.sock",
				pvt->_pth->getSocketsDir(),
				pvt->_cmdl->getId());

	debugWrite("removehandoffsockname: %s",removehandoffsockname);

	// attach to the socket and write the process id
	unixsocketclient	removehandoffsockun;
	removehandoffsockun.setFileName(removehandoffsockname);
	removehandoffsockun.connect();
	removehandoffsockun.write((uint32_t)process::getProcessId());
	removehandoffsockun.flushWriteBuffer(-1,-1);

	debugEnd();

	delete[] removehandoffsockname;
}

int32_t sqlrservercontroller::waitForClient() {

	debugStart("waiting for client");

	setState(WAIT_CLIENT);

	// reset proxy mode flag
	pvt->_proxymode=false;

	if (!pvt->_suspendedsession) {

		// If we're not in the middle of a suspended session,
		// talk to the listener...


		// the client file descriptor
		int32_t	descriptor;

		// What is this loop all aboout?
		// If the listener is proxying clients, it can't tell whether
		// the client succeeded in transmitting an END_SESSION or
		// whether it even tried, so it sends one when the client
		// disconnects no matter what.  If the client did send one then
		// we'll receive a second END_SESSION here.  Depending on the
		// byte order of the host, we'll receive either a 1536 or 6.
		// If we got an END_SESION then just loop back and read again,
		// the command will follow.
		uint16_t	command;
		do {
			// get the command
			if (pvt->_handoffsockun.read(&command,1,0)!=
							sizeof(uint16_t)) {
				raiseInternalErrorEvent(NULL,
						"read handoff command failed");
				debugEnd();
				// If this fails, then the listener most likely
				// died because sqlr-stop was run.  Arguably
				// this condition should initiate a shut down
				// of this process as well, but for now we'll
				// just wait to be shut down manually.
				// Unfortunatley, that means looping over and
				// over, with that read failing every time.
				// We'll sleep so as not to slam the machine
				// while we loop.
				snooze::microsnooze(0,100000);
				return -1;
			}
		} while (command==1536 || command==6);

		if (command==HANDOFF_RECONNECT) {

			// if we're supposed to reconnect, then just do that...
			debugEnd();
			return 2;

		} else if (command==HANDOFF_PASS) {

			if (!getProtocol()) {
				debugEnd();
				return -1;
			}

			// Receive the client file descriptor and use it.
			if (!pvt->_handoffsockun.receiveSocket(
						&descriptor,1,0)) {
				raiseInternalErrorEvent(NULL,
						"failed to receive "
						"client file descriptor");
				debugEnd();
				// If this fails, then the listener most likely
				// died because sqlr-stop was run.  Arguably
				// this condition should initiate a shut down
				// of this process as well, but for now we'll
				// just wait to be shut down manually.
				// Unfortunatley, that means looping over and
				// over, with that read above failing every
				// time, thus the  sleep so as not to slam the
				// machine while we loop.
				return -1;
			}

			// ack the handoff
			pvt->_handoffsockun.write((byte_t)ACK);

		} else if (command==HANDOFF_PROXY) {

			if (!getProtocol()) {
				debugEnd();
				return -1;
			}

			debugWrite("listener is proxying the client");

			// get the listener's pid
			if (pvt->_handoffsockun.read(
					&pvt->_proxypid,1,0)!=
					sizeof(uint32_t)) {
				raiseInternalErrorEvent(NULL,
						"failed to read process "
						"id during proxy handoff");
				debugEnd();
				return -1;
			}

			debugWrite("listener pid: %d",pvt->_proxypid);

			// acknowledge
			pvt->_handoffsockun.write((byte_t)ACK);
			pvt->_handoffsockun.flushWriteBuffer(-1,-1);

			descriptor=pvt->_handoffsockun.getFileDescriptor();

			pvt->_proxymode=true;

		} else {

			raiseInternalErrorEvent(NULL,
					"received invalid handoff mode");
			debugEnd();
			return -1;
		}

		// set up the client socket
		pvt->_clientsock=new socketclient;
		pvt->_clientsock->setFileDescriptor(descriptor);

		// On most systems, the file descriptor is in whatever mode
		// it was in the other process, but on FreeBSD < 5.0 and
		// possibly other systems, it ends up in non-blocking mode
		// in this process, independent of its mode in the other
		// process.  So, we force it to blocking mode here.
		pvt->_clientsock->setNonBlockingMode(false);

	} else {

		// If we're in the middle of a suspended session, wait for
		// a client to reconnect...
		uint32_t	totalseconds=0;
		int32_t		sec=(pvt->_accepttimeout)?1:0;
		for (;;) {

			// If accepttimeout is anthing but 0, then wait 1
			// second for the client to connect.  If accepttimeout
			// is 0 then fall through immediately unless the client
			// is already connected.
			int32_t	result=pvt->_lsnr.listen(sec,0);

			// bail on error
			if (result==RESULT_ERROR) {
				raiseInternalErrorEvent(NULL,
					"wait for client connect failed");
				debugEnd();
				return 0;
			}

			// if we didn't get a timeout, then break out of this
			// loop and handle the client connection
			if (result!=RESULT_TIMEOUT) {
				break;
			}

			// bail if the shutdown flag got set
			if (process::getShutDownFlag()) {
				debugWrite("shutdown");
				debugEnd();
				return 0;
			}

			// increment the number of seconds we've been waiting
			// (if accepttimeout is 0 then technically we've waited
			// 0 seconds, but close enough)
			totalseconds++;

			// bail if we've been waiting more seconds than the
			// accepttimeout or if we made it here and the accept
			if (totalseconds>=pvt->_accepttimeout) {
				raiseInternalErrorEvent(NULL,
					"wait for client connect failed");
				debugEnd();
				return 0;
			}
		}

		// get the first socket that had data available...
		filedescriptor	*fd=pvt->_lsnr.getReadReadyList()->
						getFirst()->getValue();

		inetsocketserver	*iss=NULL;
		for (uint64_t index=0; index<pvt->_serversockincount; index++) {
			if (fd==pvt->_serversockin[index]) {
				iss=pvt->_serversockin[index];
			}
		}
		if (iss) {
			pvt->_clientsock=iss->accept();
		} else if (fd==pvt->_serversockun) {
			pvt->_clientsock=pvt->_serversockun->accept();
		}

		if (fd) {
			debugWrite("client reconnect succeeded");
		} else {
			raiseInternalErrorEvent(NULL,"client reconnect failed");
		}
		debugWrite("done waiting for client");

		if (!fd) {
			debugEnd();
			return 0;
		}
	}

	debugEnd();
	return 1;
}

bool sqlrservercontroller::getProtocol() {

	debugStart("getting the protocol index");

	// get protocol index
	if (pvt->_handoffsockun.read(&pvt->_protocolindex,1,0)!=
							sizeof(uint16_t)) {
		debugWrite("failed");
		debugEnd();
		return false;
	}

	debugWrite("success");
	debugEnd();
	return true;
}

void sqlrservercontroller::clientSession() {

	debugStart("client session");

	pvt->_inclientsession=true;

	// update various stats
	setState(SESSION_START);
	setClientAddr();
	setClientSessionStartTime();
	incrementOpenClientConnections();

	raiseClientConnectedEvent();

	// have client session using the appropriate protocol
	pvt->_currentprotocol=pvt->_sqlrpr->getProtocol(pvt->_protocolindex);
	clientsessionexitstatus_t	exitstatus=
					CLIENTSESSIONEXITSTATUS_ERROR;
	if (pvt->_currentprotocol) {
		exitstatus=pvt->_currentprotocol->clientSession(
							pvt->_clientsock);
	} else {
		closeClientConnection(0);
	}

	closeSuspendedSessionSockets();

	const char	*info;
	switch (exitstatus) {
		case CLIENTSESSIONEXITSTATUS_CLOSED_CONNECTION:
			info="client closed connection";
			break;
		case CLIENTSESSIONEXITSTATUS_ENDED_SESSION:
			info="client ended the session";
			break;
		case CLIENTSESSIONEXITSTATUS_SUSPENDED_SESSION:
			info="client suspended the session";
			break;
		case CLIENTSESSIONEXITSTATUS_ERROR:
		default:
			// Don't use the word "error" here.
			//
			// Conditions that result in
			// CLIENTSESSIONEXITSTATUS_ERROR
			// might not warrant investigation by operations staff.
			//
			// For example:
			// * Programs can crash or exit at just the right
			// 	moment.
			// * Load balancers often check to be sure a service is
			// 	still running by just connecting and
			// 	disconnecting.
			// * Ad-hock sqlrsh users might supply invalid
			//	credentials.
			//
			// We don't want "error" making its way into the logs
			// or log analyzers will generate a bunch of
			// false-positives.
			//
			// If a "real" error occurs, it will be reported
			// elsewhere.
			info="server closed connection";
			break;
	}
	raiseClientDisconnectedEvent(info);

	decrementOpenClientConnections();

	pvt->_inclientsession=false;

	debugEnd();
}

sqlrservercursor *sqlrservercontroller::getCursor(uint16_t id) {

	// get the specified cursor
	for (uint16_t i=0; i<pvt->_cursorcount; i++) {
		if (pvt->_cur[i]->getId()==id) {
			incrementTimesCursorReused(); 
			return pvt->_cur[i];
		}
	}

	raiseClientProtocolErrorEvent(NULL,1,
				"get cursor failed: "
				"client requested an invalid cursor: %hd",id);

	return NULL;
}

sqlrservercursor *sqlrservercontroller::getCursor() {

	debugStart("get cursor");

	// find an available cursor
	for (uint16_t i=0; i<pvt->_cursorcount; i++) {
		if (pvt->_cur[i]->getState()==SQLRCURSORSTATE_AVAILABLE) {
			debugWrite("available cursor: %hd",i);
			pvt->_cur[i]->setState(SQLRCURSORSTATE_BUSY);
			incrementTimesNewCursorUsed();
			debugEnd();
			return pvt->_cur[i];
		}
	}

	// apparently there weren't any available cursors...

	// if we can't create any new cursors then return an error
	if (pvt->_cursorcount==pvt->_maxcursorcount) {
		debugWrite("all cursors are busy");
		for (uint16_t i=0; i<pvt->_cursorcount; i++) {
			uint32_t	querysize=pvt->_cur[i]->getQuerySize();
			if (querysize>40) {
				querysize=40;
			}
			debugWrite("cursor %hd: %.*s",i,querysize,
						pvt->_cur[i]->getQueryBuffer());
		}
		debugEnd();
		return NULL;
	}

	// create new cursors
	uint16_t	expandto=pvt->_cursorcount+
					pvt->_cfg->getCursorsGrowBy();
	if (expandto>=pvt->_maxcursorcount) {
		expandto=pvt->_maxcursorcount;
	}
	uint16_t	firstnewcursor=pvt->_cursorcount;
	do {
		pvt->_cur[pvt->_cursorcount]=newCursor(pvt->_cursorcount);
		if (!open(pvt->_cur[pvt->_cursorcount])) {
			raiseInternalErrorEvent(
					NULL,"cursor init failure: %hd",
					pvt->_cursorcount);
			return NULL;
		}
		pvt->_cursorcount++;
	} while (pvt->_cursorcount<expandto);
	
	// return the first new cursor that we created
	pvt->_cur[firstnewcursor]->setState(SQLRCURSORSTATE_BUSY);
	incrementTimesNewCursorUsed();
	debugEnd();
	return pvt->_cur[firstnewcursor];
}

bool sqlrservercontroller::auth(sqlrcredentials *cred) {

	debugStart("auth");

	// authenticate
	const char	*autheduser=NULL;
	if (pvt->_sqlra) {
		autheduser=pvt->_sqlra->auth(cred);
	}
	if (autheduser) {

		setAuthenticatedUser(autheduser,charstring::getLength(autheduser));

		// consult connection schedules
		if (pvt->_sqlrs &&
			!pvt->_sqlrs->allowed(pvt->_conn,getAuthenticatedUser())) {
			debugWrite("connection schedule violation");
			raiseScheduleViolationEvent(getAuthenticatedUser());
			debugEnd();
			return false;
		}

		debugWrite("success");
		debugEnd();
		return true;
	}

	debugWrite("failed");
	raiseClientConnectionRefusedEvent("auth failed");
	debugEnd();
	return false;
}

bool sqlrservercontroller::changeUser(const char *newuser,
					const char *newpassword) {
	debugStart("change user");
	closeCursors(false);
	logOut();
	setLoginUser(newuser);
	setLoginPassword(newpassword);
	bool	retval=(logIn(false) && initCursors(pvt->_cursorcount));
	debugEnd();
	return retval;
}

bool sqlrservercontroller::changeProxiedUser(const char *newuser,
						const char *newpassword) {
	debugStart("change proxied user");
	bool	retval=pvt->_conn->changeProxiedUser(newuser,newpassword);
	debugEnd();
	return retval;
}

sqlrpwdenc *sqlrservercontroller::getPasswordEncryptionById(const char *id) {
	return (pvt->_sqlrpe)?pvt->_sqlrpe->getPasswordEncryptionById(id):NULL;
}

void sqlrservercontroller::beginSession() {
	sessionStartQueries();
}

void sqlrservercontroller::suspendSession(const char **unixsocket,
						uint16_t *inetport) {

	// mark the session suspended
	pvt->_suspendedsession=true;

	// we can't wait forever for the client to resume, set a timeout
	pvt->_accepttimeout=pvt->_cfg->getSessionTimeout();

	// abort and release all cursors that aren't suspended...
	debugStart("aborting busy cursors");
	for (int32_t i=0; i<pvt->_cursorcount; i++) {
		if (pvt->_cur[i]->getState()==SQLRCURSORSTATE_BUSY) {
			abort(pvt->_cur[i]);
			release(pvt->_cur[i]);
		}
	}
	debugEnd();

	// open sockets to resume on
	debugStart("opening sockets to resume on");
	*unixsocket=NULL;
	*inetport=0;
	if (openSockets()) {
		if (pvt->_serversockun) {
			*unixsocket=pvt->_unixsocket.getString();
		}
		*inetport=pvt->_inetport;
	}
	debugEnd();
}

bool sqlrservercontroller::setAutoCommitOn() {

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d: autocommit on\n",
					process::getProcessId());
	}

	// if the database isn't transactional then autocommit is always on
	if (!isTransactional()) {
		pvt->_autocommit=true;
		return true;
	}

	// bail if autocommit is already on or if we're not in a transaction
	if (pvt->_autocommit || !pvt->_intransaction) {
		return true;
	}

	// if the transaction model is "explicit-error" then throw an
	// error if we're currently in a transaction
	if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_ERROR) {
		setError(SQLR_ERROR_AUTOCOMMIT_ON_IN_TX_BLOCK_STRING,
				SQLR_ERROR_AUTOCOMMIT_ON_IN_TX_BLOCK,
				true);
		return false;
	}

	// if the transaction model is "explicit-deferred" then set a flag to
	// tell the upcoming endTransaction() to leave autocommit on
	// (and fall through to setAutocommitOn() or commit() below)
	if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_DEFERRED) {
		pvt->_endtxwithautocommiton=true;
	}

	// if the database supports autocommit then call its autocommit method
	if (pvt->_conn->supportsAutoCommit()) {
		if (!pvt->_conn->setAutoCommitOn()) {
			return false;
		}
		pvt->_autocommit=true;
		return endTransaction(true);
	}

	// if the database doesn't support autocommit...

	// if we're faking implicit transactions then execute a direct commit
	if (fakingImplicitTransactions()) {
		if (!pvt->_conn->commit()) {
			return false;
		}
		pvt->_autocommit=true;
		return endTransaction(true);
	}

	// if the database supports explicit transactions (or we're faking them)
	// then implement this with a commit
	if (supportsExplicitTransactions() || fakingExplicitTransactions()) {
		return commit();
	}

	// the db:
	// * is transactional
	// * doesn't support it's own method of enabling autocommit
	// * doesn't support explicit transactions
	// * we're not faking explicit transactions
	//
	// in this case, there's no way to enable autocommit
	// FIXME: we need to manually execute a commit after every query
	pvt->_autocommit=false;
	return false;
}

bool sqlrservercontroller::setAutoCommitOff() {

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d: autocommit off\n",
					process::getProcessId());
	}

	// if the database isn't transactional then autocommit is always on
	if (!isTransactional()) {
		pvt->_autocommit=true;
		return false;
	}

	// if autocommit is already off or if we're in a transaction...
	if (!pvt->_autocommit || pvt->_intransaction) {

		// if the transaction model is "explicit-error" then throw an
		// error if we're currently in a transaction
		if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_ERROR) {
			setError(SQLR_ERROR_AUTOCOMMIT_OFF_IN_TX_BLOCK_STRING,
					SQLR_ERROR_AUTOCOMMIT_OFF_IN_TX_BLOCK,
					true);
			return false;
		}

		// if the transaction model is "explicit-deferred" then call
		// autocommit off if the db supports it, and unset any flag
		// that was set to tell the upcoming endTransaction() to
		// leave autocommit on
		if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_DEFERRED) {
			if (!fakingExplicitDeferredTransactions() &&
					pvt->_conn->supportsAutoCommit()) {
				if (!pvt->_conn->setAutoCommitOff()) {
					return false;
				}
			}
			pvt->_endtxwithautocommiton=false;
		}

		// otherwise, just bail
		return true;
	}

	// if the database supports autocommit then call its autocommit method
	if (pvt->_conn->supportsAutoCommit()) {
		if (!pvt->_conn->setAutoCommitOff()) {
			return false;
		}
		if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_DEFERRED) {
			pvt->_endtxwithautocommiton=false;
		}
		return beginTransaction();
	}

	// if the database doesn't support autocommit...

	// if we're faking implicit transactions then execute a direct begin
	if (fakingImplicitTransactions()) {
		return pvt->_conn->begin() && beginTransaction();
	}

	// if the database supports explicit transactions (or we're faking them)
	// then implement this with a begin
	if (supportsExplicitTransactions() || fakingExplicitTransactions()) {
		return begin();
	}

	// the db:
	// * is transactional
	// * doesn't support it's own method of enabling autocommit
	// * doesn't support explicit transactions
	// * we're not faking explicit transactions
	//
	// in this case, autocommit is always off
	// FIXME: unless we're manually executing a commit after every query
	pvt->_autocommit=false;
	return true;
}

bool sqlrservercontroller::begin() {

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d: begin\n",process::getProcessId());
	}

	// bail if the database isn't transactional
	if (!isTransactional()) {
		return true;
	}

	// bail if autocommit is off
	if (!pvt->_autocommit) {
		return true;
	}

	// bail if we're currently in a transaction
	if (pvt->_intransaction) {
		setError(SQLR_ERROR_BEGIN_IN_TX_BLOCK_STRING,
				SQLR_ERROR_BEGIN_IN_TX_BLOCK,true);
		return false;
	}

	// bail if the database doesn't support explicit transactions,
	// and we're not faking them
	if (!supportsExplicitTransactions() && !fakingExplicitTransactions()) {
		return true;
	}

	// if we are faking explicit transactions then implement this with
	// autocommit off
	if (fakingExplicitTransactions()) {

		// ...but, only if the db supports autocommit, otherwise we'll
		// end up in an infinite begin()-setAutoCommitOff()-begin()-...
		// recursion
		if (pvt->_conn->supportsAutoCommit()) {
			if (!setAutoCommitOff()) {
				return false;
			}
			// user explicitly began the tx; autocommit should
			// return to on at commit/rollback (overrides the
			// _endtxwithautocommiton=false that setAutoCommitOff sets for
			// the "user wants persistent autocommit-off" case)
			if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_DEFERRED) {
				pvt->_endtxwithautocommiton=true;
			}
			return true;
		}

		// If we're faking explicit transactions then the db doesn't
		// support a begin statement.  If the db ALSO doesn't support
		// autocommit, then there's no way to begin a transaction.
		return false;
	}

	// faking-implicit mode bails at the top of this function
	// (autocommit is always conceptually off, so the "autocommit is off"
	// guard catches it -- begin is meaningless when we're always in a tx)

	// native begin -- this is an explicit user-driven tx; autocommit
	// should return to on at commit/rollback
	if (!pvt->_conn->begin() || !beginTransaction()) {
		return false;
	}
	if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_DEFERRED) {
		pvt->_endtxwithautocommiton=true;
	}
	return true;
}

bool sqlrservercontroller::commit() {

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d: commit\n",process::getProcessId());
	}

	// bail if the database isn't transactional
	if (!isTransactional()) {
		return true;
	}

	// bail if autocommit is on
	if (pvt->_autocommit) {
		return true;
	}

	// bail if we're not currently in a transaction
	if (!pvt->_intransaction) {
		setError(SQLR_ERROR_COMMIT_NOT_IN_TX_BLOCK_STRING,
				SQLR_ERROR_COMMIT_NOT_IN_TX_BLOCK,true);
		return false;
	}

	// commit
	return pvt->_conn->commit() && endTransaction(true);
}

bool sqlrservercontroller::rollback() {

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d: rollback\n",process::getProcessId());
	}

	// bail if the database isn't transactional
	if (!isTransactional()) {
		return true;
	}

	// bail if autocommit is on
	if (pvt->_autocommit) {
		return true;
	}

	// bail if we're not currently in a transaction
	if (!pvt->_intransaction) {
		setError(SQLR_ERROR_ROLLBACK_NOT_IN_TX_BLOCK_STRING,
				SQLR_ERROR_ROLLBACK_NOT_IN_TX_BLOCK,true);
		return false;
	}

	// rollback
	return pvt->_conn->rollback() && endTransaction(false);
}

bool sqlrservercontroller::isTransactional() {
	return pvt->_txmodel!=SQLRTXMODEL_NONE;
}

bool sqlrservercontroller::supportsImplicitTransactions() {
	return pvt->_conn->getNativeTransactionModel()==
					SQLRTXMODEL_IMPLICIT;
}

bool sqlrservercontroller::supportsExplicitTransactions() {
	return pvt->_conn->getNativeTransactionModel()!=
					SQLRTXMODEL_NONE &&
		pvt->_conn->getNativeTransactionModel()!=
					SQLRTXMODEL_IMPLICIT;
}

bool sqlrservercontroller::fakingImplicitTransactions() {
	return pvt->_conn->getNativeTransactionModel()!=
					SQLRTXMODEL_IMPLICIT &&
				pvt->_txmodel==
					SQLRTXMODEL_IMPLICIT;
}

bool sqlrservercontroller::fakingExplicitTransactions() {
	return pvt->_conn->getNativeTransactionModel()==
					SQLRTXMODEL_IMPLICIT &&
				pvt->_txmodel!=
					SQLRTXMODEL_IMPLICIT;
}

bool sqlrservercontroller::fakingExplicitDeferredTransactions() {
	return pvt->_conn->getNativeTransactionModel()!=
					SQLRTXMODEL_EXPLICIT_DEFERRED &&
				pvt->_txmodel==
					SQLRTXMODEL_EXPLICIT_DEFERRED;
}

bool sqlrservercontroller::beginTransaction() {

	// FIXME: arguably modules should have beginTransaction() methods

	// set in-tx flag
	pvt->_intransaction=true;

	// set autocommit flag
	pvt->_autocommit=false;

	// raise events
	raiseBeginTransactionEvent();

	return true;
}

bool sqlrservercontroller::endTransaction(bool commit) {

	// reset protocol modules
	if (pvt->_sqlrpr) {
		pvt->_sqlrpr->endTransaction(commit);
	}

	// reset translation modules
	if (pvt->_sqlrt) {
		pvt->_sqlrt->endTransaction(commit);
	}

	// reset filter modules
	if (pvt->_sqlrf) {
		pvt->_sqlrf->endTransaction(commit);
	}

	// reset bind variable translation modules
	if (pvt->_sqlrbvt) {
		pvt->_sqlrbvt->endTransaction(commit);
	}

	// reset result set header translation modules
	if (pvt->_sqlrrsht) {
		pvt->_sqlrrsht->endTransaction(commit);
	}

	// reset result set translation modules
	if (pvt->_sqlrrst) {
		pvt->_sqlrrst->endTransaction(commit);
	}

	// reset result set row translation modules
	if (pvt->_sqlrrsrt) {
		pvt->_sqlrrsrt->endTransaction(commit);
	}

	// reset result set row block translation modules
	if (pvt->_sqlrrsrbt) {
		pvt->_sqlrrsrbt->endTransaction(commit);
	}

	// reset error translation modules
	if (pvt->_sqlret) {
		pvt->_sqlret->endTransaction(commit);
	}

	// reset trigger modules
	if (pvt->_sqlrtr) {
		pvt->_sqlrtr->endTransaction(commit);
	}

	// reset logger modules
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->endTransaction(commit);
	}

	// reset notification modules
	if (pvt->_sqlrn) {
		pvt->_sqlrn->endTransaction(commit);
	}

	// reset query modules
	if (pvt->_sqlrq) {
		pvt->_sqlrq->endTransaction(commit);
	}

	// reset module datas
	if (pvt->_sqlrmd) {
		pvt->_sqlrmd->endTransaction(commit);
	}

	// clear column caches
	clearColumnCaches();

	// clear per-session pool
	pvt->_txpool.clear();

	if (pvt->_txmodel==SQLRTXMODEL_EXPLICIT_DEFERRED) {
		if (fakingExplicitTransactions() &&
				pvt->_conn->supportsAutoCommit()) {
			if (pvt->_endtxwithautocommiton) {
				pvt->_conn->setAutoCommitOn();
			} else {
				pvt->_conn->setAutoCommitOff();
			}
		}
		pvt->_autocommit=pvt->_endtxwithautocommiton;
		pvt->_endtxwithautocommiton=false;
	} else if (supportsExplicitTransactions() ||
				fakingExplicitTransactions()) {
		if (fakingExplicitTransactions() &&
				pvt->_conn->supportsAutoCommit()) {
			pvt->_conn->setAutoCommitOn();
		}
		pvt->_autocommit=true;
	} else if (fakingImplicitTransactions()) {
		if (!pvt->_autocommit && !pvt->_conn->supportsAutoCommit()) {
			pvt->_conn->begin();
		}
	}

	// set in-tx flag
	pvt->_intransaction=!pvt->_autocommit;

	// reset needs commit/rollback flag
	pvt->_needscommitorrollback=false;

	// raise events
	if (commit) {
		raiseCommitEvent();
	} else {
		raiseRollbackEvent();
	}

	return true;
}

void sqlrservercontroller::clearColumnCaches() {
	for (listnode<char *> *colcachenode=
			pvt->_colcache.getKeys()->getFirst();
			colcachenode; colcachenode=colcachenode->getNext()) {
		pvt->_colcache.getValue(colcachenode->getValue())->clear();
	}
	pvt->_colcache.clear();
	pvt->_autoinccolcache.clear();
	pvt->_primarykeycolcache.clear();
}

bool sqlrservercontroller::selectDatabase(const char *db) {
	if (pvt->_cfg->getIgnoreSelectDatabase()) {
		return true;
	}
	return (getDatabaseIsSchema())?selectSchema(db):selectCatalog(db);
}

char *sqlrservercontroller::getCurrentDatabase() {
	return (getDatabaseIsSchema())?getCurrentSchema():getCurrentCatalog();
}

bool sqlrservercontroller::getDatabaseIsSchema() {
	return pvt->_conn->getDatabaseIsSchema();
}

bool sqlrservercontroller::selectCatalog(const char *catalog) {
	if (pvt->_conn->selectCatalog(catalog)) {
		// set a flag indicating that the catalog has been changed
		// so it can be reset at the end of the session
		pvt->_catalogchanged=true;
		return true;
	}
	return false;
}

char *sqlrservercontroller::getCurrentCatalog() {
	return pvt->_conn->getCurrentCatalog();
}

bool sqlrservercontroller::selectSchema(const char *schema) {
	if (pvt->_conn->selectSchema(schema)) {
		// set a flag indicating that the schema has been changed
		// so it can be reset at the end of the session
		pvt->_schemachanged=true;
		return true;
	}
	return false;
}

char *sqlrservercontroller::getCurrentSchema() {
	return pvt->_conn->getCurrentSchema();
}

char *sqlrservercontroller::getCurrentUser() {
	return pvt->_conn->getCurrentUser();
}

bool sqlrservercontroller::getLastInsertId(uint64_t *id) {
	return pvt->_conn->getLastInsertId(id);
}

const char *sqlrservercontroller::getNoopQuery() {
	return pvt->_conn->getNoopQuery();
}

const char *sqlrservercontroller::getDefaultIsolationLevel(
				sqlrserverisolationlevelformat_t format) {
	return pvt->_conn->mapIsolationLevel(
				pvt->_conn->getDefaultIsolationLevel(),
				SQLRSERVERISOLATIONLEVELFORMAT_NATIVE,
				format);
}

bool sqlrservercontroller::setIsolationLevel(const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat) {
	const char	*mappedisolevel=
			pvt->_conn->mapIsolationLevel(
					isolevel,fromformat,
					SQLRSERVERISOLATIONLEVELFORMAT_NATIVE);
	return pvt->_conn->setIsolationLevel(mappedisolevel);
}

const char *sqlrservercontroller::getIsolationLevel(
				sqlrserverisolationlevelformat_t toformat) {
	const char	*isolevel=pvt->_conn->getIsolationLevel();
	return pvt->_conn->mapIsolationLevel(
				isolevel,SQLRSERVERISOLATIONLEVELFORMAT_NATIVE,
				toformat);
}

sqlrtxmodel_t sqlrservercontroller::getTransactionModel() {
	return pvt->_txmodel;
}

sqlrtxmodel_t sqlrservercontroller::getNativeTransactionModel() {
	return pvt->_conn->getNativeTransactionModel();
}

sqlrtxmodel_t sqlrservercontroller::stringToTransactionModel(
						const char *txmodel) {

	// "native" is intentionally not mapped here; resolving it requires
	// asking the active connection what its native model is, which a
	// static helper can't do.  Callers handle that case before calling
	// this function.
	if (!charstring::compare(txmodel,"none")) {
		return SQLRTXMODEL_NONE;
	}
	if (!charstring::compare(txmodel,"implicit")) {
		return SQLRTXMODEL_IMPLICIT;
	}
	if (!charstring::compare(txmodel,"explicit")) {
		return SQLRTXMODEL_EXPLICIT;
	}
	if (!charstring::compare(txmodel,"explicit-deferred")) {
		return SQLRTXMODEL_EXPLICIT_DEFERRED;
	}
	if (!charstring::compare(txmodel,"explicit-error")) {
		return SQLRTXMODEL_EXPLICIT_ERROR;
	}
	return SQLRTXMODEL_UNKNOWN;
}

const char *sqlrservercontroller::transactionModelToString(
						sqlrtxmodel_t txmodel) {
	switch (txmodel) {
		case SQLRTXMODEL_NONE:
			return "none";
		case SQLRTXMODEL_IMPLICIT:
			return "implicit";
		case SQLRTXMODEL_EXPLICIT:
			return "explicit";
		case SQLRTXMODEL_EXPLICIT_DEFERRED:
			return "explicit-deferred";
		case SQLRTXMODEL_EXPLICIT_ERROR:
			return "explicit-error";
		case SQLRTXMODEL_UNKNOWN:
		default:
			return "unknown";
	}
}

bool sqlrservercontroller::setTransactionModel(sqlrtxmodel_t txmodel) {

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d: set transaction model %d\n",
					process::getProcessId(),(int)txmodel);
	}

	// bail if the tx model isn't actually changing
	if (txmodel==pvt->_txmodel) {
		return true;
	}

	// capture the autocommit state
	bool	savedautocommit=pvt->_autocommit;

	// make sure that we're outside of a transaction, with autocommit on
	// (skip if we're already in NONE mode -- NONE forces autocommit-on
	// and not-in-tx, so the commit/setAutoCommitOn would be no-ops)
	if (pvt->_txmodel!=SQLRTXMODEL_NONE) {
		sqlrtxmodel_t	savedtxmodel=pvt->_txmodel;
		pvt->_txmodel=pvt->_conn->getNativeTransactionModel();
		if (!commit() || !setAutoCommitOn()) {
			pvt->_txmodel=savedtxmodel;
			return false;
		}
	}

	// switch the tx model
	pvt->_txmodel=txmodel;

	// restore the original autocommit state...

	// only possible if the new model supports autocommit-off
	if (txmodel!=SQLRTXMODEL_NONE) {
		// only necessary if autocommit was originally off
		if (!savedautocommit) {
			return setAutoCommitOff();
		}
	}
	return true;
}

bool sqlrservercontroller::ping() {
	return pvt->_conn->ping();
}

bool sqlrservercontroller::getListsByApiCalls() {
	return pvt->_conn->getListsByApiCalls();
}

bool sqlrservercontroller::fakePrepareAndExecuteForApiCall(
					sqlrservercursor *cursor) {
	cursor->setResultSetHeaderHasBeenHandled(false);
	cursor->getBindMappingsPool()->clear();
	cursor->setQuerySize(0);
	cursor->getQueryBuffer()[0]='\0';
	if (pvt->_sqlrt && !translateQuery(cursor)) {
		return false;
	}
	cursor->clearTotalRowsFetched();
	return true;
}

bool sqlrservercontroller::getDatabaseList(sqlrservercursor *cursor,
							const char *db) {
	if (getDatabaseIsSchema()) {
		char	*catalog=getCurrentCatalog();
		bool	retval=getSchemaList(cursor,catalog,db);
		delete[] catalog;
		return retval;
	}
	return getCatalogList(cursor,db);
}

bool sqlrservercontroller::getCatalogList(sqlrservercursor *cursor,
						const char *catalog) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getCatalogList(cursor,catalog) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getSchemaList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getSchemaList(cursor,catalog,schema) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getTableTypeList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *tabletypes) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getTableTypeList(cursor,catalog,
						schema,tabletypes) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getTableList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getTableList(cursor,catalog,schema,
						table,objecttypes) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getTypeInfoList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *type) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getTypeInfoList(cursor,catalog,schema,type) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getColumnList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getColumnList(cursor,catalog,schema,
						table,column) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getPrimaryKeysList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getPrimaryKeysList(cursor,catalog,schema,table) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getKeyAndIndexList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *table) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getKeyAndIndexList(cursor,catalog,schema,table) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getProcedureList(sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *procedure) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getProcedureList(cursor,catalog,
						schema,procedure) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getProcedureParameterList(
						sqlrservercursor *cursor,
						const char *catalog,
						const char *schema,
						const char *procedure) {
	return fakePrepareAndExecuteForApiCall(cursor) &&
		pvt->_conn->getProcedureParameterList(
					cursor,catalog,schema,procedure) &&
		handleResultSetHeader(cursor);
}

bool sqlrservercontroller::getLastInsertIdList(sqlrservercursor *cursor) {

	// get the last insert id
	uint64_t	lid;
	if (!getLastInsertId(&lid)) {
		return false;
	}

	// create a fake cursor that returns the last insert id
	// as the first column of the first row
	sqlrquery_lastinsertidlistcursor	*lidcur=new
		sqlrquery_lastinsertidlistcursor(pvt->_conn,NULL,NULL,0);
	lidcur->setLastInsertId(lid);
	cursor->setCustomQueryCursor(lidcur);

	// pretend that we actually executed a query
	if (!fakePrepareAndExecuteForApiCall(lidcur)) {
		return false;
	}

	// set up column flags, buffers, and mappings
	return handleResultSetHeader(lidcur);
}

const char *sqlrservercontroller::getGlobalTempTableListQuery() {
	return pvt->_conn->getGlobalTempTableListQuery();
}

void sqlrservercontroller::saveError() {

	// don't overwrite any message that's already been saved
	if (pvt->_conn->getErrorNumber() || pvt->_conn->getErrorSize()) {
		return;
	}

	// fetch the error into the connection error buffers and flags
	uint32_t	errorsize;
	int64_t		errorcode;
	bool		liveconnection;
	pvt->_conn->getError(pvt->_conn->getErrorBuffer(),
				pvt->_cfg->getMaxErrorSize(),
				&errorsize,
				&errorcode,
				&liveconnection);
	pvt->_conn->setErrorSize(errorsize);
	pvt->_conn->setErrorNumber(errorcode);
	pvt->_conn->setLiveConnection(liveconnection);

	if (pvt->_debugsql) {
		stdoutput.printf("%d:ERROR:\n%d:",
				process::getProcessId(),errorcode);
		stdoutput.write(pvt->_conn->getErrorBuffer(),errorsize);
		stdoutput.write('\n');
	}
}

void sqlrservercontroller::saveErrorFromCursor(sqlrservercursor *cursor) {

	// don't overwrite any message that's already been saved
	if (pvt->_conn->getErrorNumber() || pvt->_conn->getErrorSize()) {
		return;
	}

	// fetch the error into the connection error buffers and flags
	uint32_t	errorsize;
	int64_t		errorcode;
	bool		liveconnection;
	getError(cursor,pvt->_conn->getErrorBuffer(),
				pvt->_cfg->getMaxErrorSize(),
				&errorsize,
				&errorcode,
				&liveconnection);
	pvt->_conn->setErrorSize(errorsize);
	pvt->_conn->setErrorNumber(errorcode);
	pvt->_conn->setLiveConnection(liveconnection);

	if (pvt->_debugsql) {
		stdoutput.printf("%d:ERROR:\n%d:",
				process::getProcessId(),errorcode);
		stdoutput.write(pvt->_conn->getErrorBuffer(),errorsize);
		stdoutput.write('\n');
	}
}


void sqlrservercontroller::getError(const char **errorbuffer,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection) {
	saveError();
	*errorbuffer=pvt->_conn->getErrorBuffer();
	*errorsize=pvt->_conn->getErrorSize();
	*errorcode=pvt->_conn->getErrorNumber();
	*liveconnection=pvt->_conn->getLiveConnection();

	if (pvt->_sqlret) {
		int64_t		tec=*errorcode;
		stringbuffer	translatederror;
		if (pvt->_sqlret->run(pvt->_conn,NULL,
						*errorcode,
						*errorbuffer,
						*errorsize,
						&tec,
						&translatederror)) {
			*errorcode=tec;
			charstring::safeCopy(pvt->_conn->getErrorBuffer(),
					pvt->_conn->getErrorBufferSize(),
					translatederror.getString(),
					translatederror.getSize());
			*errorsize=translatederror.getSize();
		}
		// FIXME: report error if this fails?
	}
}

void sqlrservercontroller::getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection) {

	// fetch the error
	const char	*errorstring=NULL;
	getError(&errorstring,errorsize,errorcode,liveconnection);

	// copy the error out
	charstring::safeCopy(errorbuffer,errorbuffersize,
				errorstring,*errorsize);
	if (*errorsize>errorbuffersize) {
		*errorsize=errorbuffersize;
	}
}

void sqlrservercontroller::clearError() {
	setError(NULL,0,true);
}

void sqlrservercontroller::setError(const char *err,
						uint32_t errsize,
						int64_t errn,
						bool liveconn) {

	char		*errorbuffer=pvt->_conn->getErrorBuffer();
	if (errsize>pvt->_maxerrorsize) {
		errsize=pvt->_maxerrorsize;
	}
	charstring::safeCopy(errorbuffer,pvt->_maxerrorsize,err,errsize);
	if (errsize<pvt->_maxerrorsize) {
		errorbuffer[errsize]='\0';
	}
	pvt->_conn->setErrorSize(errsize);
	pvt->_conn->setErrorNumber(errn);
	pvt->_conn->setLiveConnection(liveconn);
}

void sqlrservercontroller::setError(const char *err,
					int64_t errn,
					bool liveconn) {
	setError(err,charstring::getLength(err),errn,liveconn);
}

char *sqlrservercontroller::getErrorBuffer() {
	return pvt->_conn->getErrorBuffer();
}

uint32_t sqlrservercontroller::getErrorBufferSize() {
	return pvt->_conn->getErrorBufferSize();
}

uint32_t sqlrservercontroller::getErrorSize() {
	return pvt->_conn->getErrorSize();
}

void sqlrservercontroller::setErrorSize(uint32_t errorsize) {
	pvt->_conn->setErrorSize(errorsize);
}

uint32_t sqlrservercontroller::getErrorNumber() {
	return pvt->_conn->getErrorNumber();
}

void sqlrservercontroller::setErrorNumber(uint32_t errnum) {
	pvt->_conn->setErrorNumber(errnum);
}

bool sqlrservercontroller::getLiveConnection() {
	return pvt->_conn->getLiveConnection();
}

void sqlrservercontroller::setLiveConnection(bool liveconnection) {
	pvt->_conn->setLiveConnection(liveconnection);
}

bool sqlrservercontroller::checkInterceptQuery(sqlrservercursor *cursor) {

	// for now, we only intercept transaction queries
	switch (cursor->getQueryType()) {
		case SQLRQUERYTYPE_BEGIN:
		case SQLRQUERYTYPE_COMMIT:
		case SQLRQUERYTYPE_ROLLBACK:
		case SQLRQUERYTYPE_AUTOCOMMIT_ON:
		case SQLRQUERYTYPE_AUTOCOMMIT_OFF:
		case SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_ON:
		case SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_OFF:
			return true;
		default:
			return false;
	}
}

bool sqlrservercontroller::interceptQuery(sqlrservercursor *cursor) {

	cursor->setQueryWasIntercepted(false);

	// Get the query type.  It will have been set by checkInterceptQuery().
	sqlrquerytype_t	querytype=cursor->getQueryType();

	// Intercept begins and handle them.  If we're faking begins, commit
	// and rollback queries also need to be intercepted as well, otherwise
	// the query will be sent directly to the db and endFakeBeginTransaction
	// won't get called.
	bool	retval=false;
	switch (querytype) {
		case SQLRQUERYTYPE_BEGIN:
			cursor->setQueryWasIntercepted(true);
			cursor->setInputBindCount(0);
			cursor->setOutputBindCount(0);
			cursor->setInputOutputBindCount(0);
			pvt->_sendcolumninfo=false;
			retval=begin();
			if (!retval) {
				copyConnectionErrorToCursor(cursor);
			}
			break;
		case SQLRQUERYTYPE_COMMIT:
			cursor->setQueryWasIntercepted(true);
			cursor->setInputBindCount(0);
			cursor->setOutputBindCount(0);
			cursor->setInputOutputBindCount(0);
			pvt->_sendcolumninfo=false;
			retval=commit();
			if (!retval) {
				copyConnectionErrorToCursor(cursor);
			}
			break;
		case SQLRQUERYTYPE_ROLLBACK:
			cursor->setQueryWasIntercepted(true);
			cursor->setInputBindCount(0);
			cursor->setOutputBindCount(0);
			cursor->setInputOutputBindCount(0);
			pvt->_sendcolumninfo=false;
			retval=rollback();
			if (!retval) {
				copyConnectionErrorToCursor(cursor);
			}
			break;
		case SQLRQUERYTYPE_AUTOCOMMIT_ON:
			cursor->setQueryWasIntercepted(true);
			cursor->setInputBindCount(0);
			cursor->setOutputBindCount(0);
			cursor->setInputOutputBindCount(0);
			pvt->_sendcolumninfo=false;
			// FIXME: should we also handle
			// SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_ON
			// somehow
			retval=setAutoCommitOn();
			if (!retval) {
				copyConnectionErrorToCursor(cursor);
			}
			break;
		case SQLRQUERYTYPE_AUTOCOMMIT_OFF:
			cursor->setQueryWasIntercepted(true);
			cursor->setInputBindCount(0);
			cursor->setOutputBindCount(0);
			cursor->setInputOutputBindCount(0);
			pvt->_sendcolumninfo=false;
			// FIXME: should we also handle
			// SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_OFF
			// somehow
			retval=setAutoCommitOff();
			if (!retval) {
				copyConnectionErrorToCursor(cursor);
			}
			break;
		default:
			break;
	}
	return retval;
}

bool sqlrservercontroller::skipComment(const char **ptr,
						const char *endptr) {
	while (*ptr<endptr && !charstring::compare(*ptr,"--",2)) {
		while (**ptr && **ptr!='\n') {
			(*ptr)++;
		}
	}
	return *ptr!=endptr;
}

bool sqlrservercontroller::skipWhitespace(const char **ptr,
						const char *endptr) {
	while (character::isWhitespace(**ptr) && *ptr<endptr) {
		(*ptr)++;
	}
	return *ptr!=endptr;
}

const char *sqlrservercontroller::skipComments(const char *query) {
	if (!query) {
		return NULL;
	}
	const char	*ptr=query;
	while (*ptr) {
		if (!charstring::compare(ptr,"--",2)) {
			while (*ptr && *ptr!='\n') {
				ptr++;
			}
			if (*ptr) {
				ptr++;
			}
		} else {
			return ptr;
		}
	}
	return ptr;
}

const char *sqlrservercontroller::skipWhitespace(const char *query) {
	if (!query) {
		return NULL;
	}
	const char	*ptr=query;
	while (*ptr && character::isWhitespace(*ptr)) {
		ptr++;
	}
	return ptr;
}

const char *sqlrservercontroller::skipWhitespaceAndComments(const char *query) {
	if (!query) {
		return NULL;
	}
	const char	*ptr=query;
	while (*ptr) {
		if (character::isWhitespace(*ptr)) {
			ptr++;
		} else if (!charstring::compare(ptr,"--",2)) {
			while (*ptr && *ptr!='\n') {
				ptr++;
			}
			if (*ptr) {
				ptr++;
			}
		} else {
			return ptr;
		}
	}
	return ptr;
}

static const char *asciitohex[]={
	"00","01","02","03","04","05","06","07",
	"08","09","0A","0B","0C","0D","0E","0F",
	"10","11","12","13","14","15","16","17",
	"18","19","1A","1B","1C","1D","1E","1F",
	"20","21","22","23","24","25","26","27",
	"28","29","2A","2B","2C","2D","2E","2F",
	"30","31","32","33","34","35","36","37",
	"38","39","3A","3B","3C","3D","3E","3F",
	"40","41","42","43","44","45","46","47",
	"48","49","4A","4B","4C","4D","4E","4F",
	"50","51","52","53","54","55","56","57",
	"58","59","5A","5B","5C","5D","5E","5F",
	"60","61","62","63","64","65","66","67",
	"68","69","6A","6B","6C","6D","6E","6F",
	"70","71","72","73","74","75","76","77",
	"78","79","7A","7B","7C","7D","7E","7F",
	"80","81","82","83","84","85","86","87",
	"88","89","8A","8B","8C","8D","8E","8F",
	"90","91","92","93","94","95","96","97",
	"98","99","9A","9B","9C","9D","9E","9F",
	"A0","A1","A2","A3","A4","A5","A6","A7",
	"A8","A9","AA","AB","AC","AD","AE","AF",
	"B0","B1","B2","B3","B4","B5","B6","B7",
	"B8","B9","BA","BB","BC","BD","BE","BF",
	"C0","C1","C2","C3","C4","C5","C6","C7",
	"C8","C9","CA","CB","CC","CD","CE","CF",
	"D0","D1","D2","D3","D4","D5","D6","D7",
	"D8","D9","DA","DB","DC","DD","DE","DF",
	"E0","E1","E2","E3","E4","E5","E6","E7",
	"E8","E9","EA","EB","EC","ED","EE","EF",
	"F0","F1","F2","F3","F4","F5","F6","F7",
	"F8","F9","FA","FB","FC","FD","FE","FF"
};

const char *sqlrservercontroller::asciiToHex(byte_t ch) {
	return asciitohex[ch];
}

static const char *asciitooctal[]={
	"000","001","002","003","004","005","006","007",
	"010","011","012","013","014","015","016","017",
	"020","021","022","023","024","025","026","027",
	"030","031","032","033","034","035","036","037",
	"040","041","042","043","044","045","046","047",
	"050","051","052","053","054","055","056","057",
	"060","061","062","063","064","065","066","067",
	"070","071","072","073","074","075","076","077",
	"100","101","102","103","104","105","106","107",
	"110","111","112","113","114","115","116","117",
	"120","121","122","123","124","125","126","127",
	"130","131","132","133","134","135","136","137",
	"140","141","142","143","144","145","146","147",
	"150","151","152","153","154","155","156","157",
	"160","161","162","163","164","165","166","167",
	"170","171","172","173","174","175","176","177",
	"200","201","202","203","204","205","206","207",
	"210","211","212","213","214","215","216","217",
	"220","221","222","223","224","225","226","227",
	"230","231","232","233","234","235","236","237",
	"240","241","242","243","244","245","246","247",
	"250","251","252","253","254","255","256","257",
	"260","261","262","263","264","265","266","267",
	"270","271","272","273","274","275","276","277",
	"300","301","302","303","304","305","306","307",
	"310","311","312","313","314","315","316","317",
	"320","321","322","323","324","325","326","327",
	"330","331","332","333","334","335","336","337",
	"340","341","342","343","344","345","346","347",
	"350","351","352","353","354","355","356","357",
	"360","361","362","363","364","365","366","367",
	"370","371","372","373","374","375","376","377"
};

const char *sqlrservercontroller::asciiToOctal(byte_t ch) {
	return asciitooctal[ch];
}

bool sqlrservercontroller::hasBindVariables(const char *query,
						uint32_t querysize) {
	return ::countBindVariables(query,querysize,
				pvt->_questionmarksupported,
				pvt->_colonsupported,
				pvt->_atsignsupported,
				pvt->_dollarsignsupported);
}

uint16_t sqlrservercontroller::countBindVariables(const char *query,
							uint32_t querysize) {
	return ::countBindVariables(query,querysize,
				pvt->_questionmarksupported,
				pvt->_colonsupported,
				pvt->_atsignsupported,
				pvt->_dollarsignsupported);
}

void sqlrservercontroller::splitObjectName(const char *currentcatalog,
						const char *currentschema,
						const char *objecttype,
						const char *combinedobject,
						const char **catalog,
						const char **schema,
						const char **object) {

	// init return values
	delete[] pvt->_catalog;
	delete[] pvt->_schema;
	delete[] pvt->_object;
	pvt->_catalog=NULL;
	pvt->_schema=NULL;
	pvt->_object=NULL;

	// split the combined object
	char		**parts=NULL;
	uint64_t	partcount=0;
	charstring::split(combinedobject,".",true,&parts,&partcount);

	if (!charstring::compareIgnoringCase(objecttype,"catalog")) {

		// if the combined object is supposed to be a catalog, then
		// it can only be in the following format:
		// * catalog

		// delete all but the first part
		while (partcount>1) {
			delete[] parts[partcount-1];
			partcount--;
		}

		if (partcount==1) {
			pvt->_catalog=parts[0];
		}

	} else if (!charstring::compareIgnoringCase(objecttype,"schema")) {

		// if the combined object is supposed to be a schema, then
		// it might be in one of the following formats:
		// * catalog.schema
		// * schema

		// delete all but the first 2 parts
		while (partcount>2) {
			delete[] parts[partcount-1];
			partcount--;
		}

		switch (partcount) {
			case 2:
				pvt->_catalog=parts[0];
				pvt->_schema=parts[1];
				break;
			case 1:
				pvt->_schema=parts[0];
				break;
		}

	} else {

		// if the combined object is supposed to be an object, then
		// it might be in one of the following formats:
		// * catalog.schema.object
		// * (currentcatalog.)schema.object
		// 	or
		// 	catalog.(defaultschema.)object
		// * object

		// delete all but the first 3 parts
		while (partcount>3) {
			delete[] parts[partcount-1];
			partcount--;
		}

		switch (partcount) {
			case 3:
				pvt->_catalog=parts[0];
				pvt->_schema=parts[1];
				pvt->_object=parts[2];
				break;
			case 2:
				// If there are 2 parts then it could mean:
				// * catalog.(defaultschema.)object
				//   or
				// * (currentcatalog.)schema.object...
				// If the first part is not the same as the
				// current catalog, then we'll guess
				// (currentcatalog.)schema.object, but we don't
				// really know for sure. The app may really
				// mean to target another catalog.
				if (!charstring::compare(
						parts[0],currentcatalog)) {
					pvt->_catalog=parts[0];
					pvt->_schema=
					charstring::duplicate(currentschema);
				} else {
					pvt->_catalog=
					charstring::duplicate(currentcatalog);
					pvt->_schema=parts[0];
				}
				pvt->_object=parts[1];
				break;
			case 1:
				pvt->_catalog=
					charstring::duplicate(currentcatalog);
				pvt->_schema=
					charstring::duplicate(currentschema);
				pvt->_object=parts[0];
				break;
		}
	}

	// clean up
	delete[] parts;

	// pass values out
	*catalog=pvt->_catalog;
	*schema=pvt->_schema;
	*object=pvt->_object;
}

bool sqlrservercontroller::parseInsert(const char *query,
					uint32_t querysize,
					sqlrquerytype_t *querytype,
					char **table,
					linkedlist<char *> **columns,
					linkedlist<char *> **allcolumns,
					const char **autoinccolumn,
					bool *columnsincludeautoinccolumn,
					const char **primarykeycolumn,
					bool *columnsincludeprimarykeycolumn,
					linkedlist<char *> **values,
					const char **rawvalues) {

	// init return values
	if (querytype) {
		*querytype=SQLRQUERYTYPE_ETC;
	}
	if (table) {
		*table=NULL;
	}
	if (columns) {
		*columns=NULL;
	}
	if (allcolumns) {
		*allcolumns=NULL;
	}
	if (autoinccolumn) {
		*autoinccolumn=NULL;
	}
	if (columnsincludeautoinccolumn) {
		*columnsincludeautoinccolumn=false;
	}
	if (values) {
		*values=NULL;
	}
	if (rawvalues) {
		*rawvalues=NULL;
	}

	// init query type
	sqlrquerytype_t	localquerytype=SQLRQUERYTYPE_ETC;

	// get the start and end of the query
	const char	*start=skipWhitespaceAndComments(query);
	const char	*end=query+querysize;

	// FIXME: assumes normalized query...

	if (querysize>12 && !charstring::compare(start,"insert into ",12)) {

		// if it was an insert...

		// initialize the query type to plain insert, if it turns out
		// to be a multi-insert or insert-select then we'll determine
		// that later and update this accordingly
		localquerytype=SQLRQUERYTYPE_INSERT;

		// skip past "insert into "
		start+=12;

		// find first space after table name
		// FIXME: the table name could be quoted and contain a space
		const char	*ptr=charstring::findFirst(start,' ');
		if (ptr>=end) {
			return false;
		}

		// get table name and strip any quoting
		char	*localtable=charstring::duplicate(start,ptr-start);
		charstring::stripSet(localtable,"\"'`[]");

		// skip ahead to whatever is past the table name,
		// which should be one of:
		// * the "(" before the list of columns
		// * the word "values"
		// * the word "select"
		ptr++;
		if (ptr>=end) {
			return false;
		}

		// get the full list of columns that are in the table,
		// as well as any auto_increment and primarky key columns
		linkedlist<char *>	*localallcolumns;
		const char		*localautoinccolumn;
		const char		*localprimarykeycolumn;
		getColumnsInTable(localtable,&localallcolumns,
						&localautoinccolumn,
						&localprimarykeycolumn);

		// create the list of columns to return
		linkedlist<char *>	*localcolumns=new linkedlist<char *>();
		localcolumns->setManageArrayValues(true);

		// if we found the "(" before a list of columns,
		// then parse the list of columns directly
		if (*ptr=='(') {
			ptr++;
			const char	*colsend=
					charstring::findFirst(ptr,')');
			getColumnsFromInsertQuery(ptr,colsend,localcolumns);
			ptr=colsend;

			// NOTE: it might be tempting not to worry about
			// skipping whitepace and the closing paren, and more
			// whitespace, and just findFirst("values (") or
			// findFirst("values(") below, but that's unreliable,
			// as some upserts include values(`...`) functions and
			// we can end up skipping all the way to one of those.

			// we should be after the last column name, skip any
			// whitespace after that
			skipWhitespace(&ptr,end);

			// we should be on the closing paren, skip that
			ptr++;

			// we should be after the closing paren, skip any
			// whitespace after that
			skipWhitespace(&ptr,end);
		}
		if (ptr>=end) {
			return false;
		}
		
		// look for the "values (" keyword
		// (see note above for why we don't just do a findFirst() here)
		// FIXME: the below is kind-of a kludge...
		// sometimes queries are written:
		//	insert into blah values(...);
		// with no space after "values", and the normalize translation
		// doesn't fix this (though it ought to)
		const char	*localrawvalues=NULL;
		if (end>ptr+7) {
			if (!charstring::compare(ptr,"values(",7)) {
				localrawvalues=ptr+7;
			}
		}
		if (!localrawvalues && end>ptr+8) {
			if (!charstring::compare(ptr,"values (",8)) {
				localrawvalues=ptr+8;
			}
		}

		if (localrawvalues) {

			// if we found the "values (" keyword then this is
			// either a plain insert or multi-insert...

			// split the (first set of) values here
			linkedlist<char *>	*localvalues=
						new linkedlist<char *>();
			localvalues->setManageArrayValues(true);
			bool			multiinsert;
			getFirstValuesFromInsertQuery(localrawvalues,end,
						localvalues,&multiinsert);

			// if there were multiple sets of values
			// then this query is a multi-insert
			if (multiinsert) {
				localquerytype=SQLRQUERYTYPE_MULTIINSERT;
			}

			// if the query didn't contain a list of columns,
			// then derive the columns from the number of values
			// and the columns from allcolumns...
			if (!localcolumns->getCount()) {
				deriveColumnsFromInsertQuery(
							localvalues,
							localallcolumns,
							localcolumns);
			}

			// determine if the list of columns contains the
			// autoincrement or primary key columns
			bool	localcolsincludeautoinccol=false;
			bool	localcolsincludeprimarykeycol=false;
			for (listnode<char *> *node=localcolumns->getFirst();
						node; node=node->getNext()) {
				const char	*col=node->getValue();
				if (!compareQuoted(col,localautoinccolumn)) {
					localcolsincludeautoinccol=true;
				}
				if (!compareQuoted(col,localprimarykeycolumn)) {
					localcolsincludeprimarykeycol=true;
				}
			}

			// copy values out
			if (values) {
				*values=localvalues;
			} else {
				delete localvalues;
			}
			if (rawvalues) {
				*rawvalues=localrawvalues;
			}
			if (columnsincludeautoinccolumn) {
				*columnsincludeautoinccolumn=
					localcolsincludeautoinccol;
			}
			if (columnsincludeprimarykeycolumn) {
				*columnsincludeprimarykeycolumn=
					localcolsincludeprimarykeycol;
			}

		} else {

			// if we didn't find the "values (" keyword then
			// this must be some kind of insert ... select
			localquerytype=SQLRQUERYTYPE_INSERTSELECT;
		}

		// copy values out
		if (table) {
			*table=localtable;
		} else {
			delete[] localtable;
		}
		if (columns) {
			*columns=localcolumns;
		} else {
			delete localcolumns;
		}
		if (allcolumns) {
			*allcolumns=localallcolumns;
		}
		if (autoinccolumn) {
			*autoinccolumn=localautoinccolumn;
		}
		if (primarykeycolumn) {
			*primarykeycolumn=localprimarykeycolumn;
		}

	} else if (querysize>7 && !charstring::compare(start,"select ",7)) {

		// if it was a select...

		localquerytype=SQLRQUERYTYPE_SELECT;

		// FIXME: detect select-into
	}

	// copy values out
	if (querytype) {
		*querytype=localquerytype;
	}

	return true;
}

bool sqlrservercontroller::reformatDateTime(const char *value,
						uint64_t valuesize,
						const char **newvalue,
						uint64_t *newvaluesize,
						bool ddmm,
						bool yyyyddmm,
						const char *datedelimiters,
						const char *datetimeformat,
						const char *dateformat,
						const char *timeformat) {

	// parse the value
	int16_t	year=-1;
	int16_t	month=-1;
	int16_t	day=-1;
	int16_t	hour=-1;
	int16_t	minute=-1;
	int16_t	second=-1;
	int32_t	microsecond=-1;
	bool	isnegative=false;
	if (!datetime::parse(value,ddmm,yyyyddmm,
				datedelimiters,
				&year,&month,&day,
				&hour,&minute,&second,
				&microsecond,&isnegative)) {
		return false;
	}

	// decide which format to use based on what parts
	// were detected in the date/time
	const char	*format=datetimeformat;
	if (hour==-1) {
		format=dateformat;
	} else if (day==-1) {
		format=timeformat;
	}

	// convert to the specified format
	delete[] pvt->_reformattedvalue;
	pvt->_reformattedvalue=datetime::formatAs(format,
						year,month,day,
						hour,minute,second,
						microsecond,isnegative);
	pvt->_reformattedvaluesize=
			charstring::getLength(pvt->_reformattedvalue);

	if (pvt->_debugsqlrresultsettranslation) {
		stdoutput.printf("\nconverted datetime "
			"\"%s\" to \"%s\"\nusing ddmm=%d and yyyyddmm=%d\n",
			value,pvt->_reformattedvalue,ddmm,yyyyddmm);
	}

	// set return values
	*newvalue=pvt->_reformattedvalue;
	*newvaluesize=pvt->_reformattedvaluesize;

	return true;
}

int32_t sqlrservercontroller::compareQuoted(const char *str1,
						const char *str2) {

	// handle NULLs
	if (!str1 && !str2) {
		return 0;
	}
	if (!str2) {
		return 1;
	}
	if (!str1) {
		return -1;
	}

	// compare str1 and str2, ignoring any quoting by single-quotes,
	// double-quotes, back-quotes, or square brackets
	const char	*str1ptr=str1;
	const char	*str2ptr=str2;
	char	*str1copy=NULL;
	char	*str2copy=NULL;
	if (character::isInSet(*str1,"`'\"[")) {
		str1copy=charstring::duplicate(str1);
		str1ptr=str1copy+1;
		str1copy[charstring::getLength(str1copy)-1]='\0';
	}
	if (character::isInSet(*str2,"`'\"[")) {
		str2copy=charstring::duplicate(str2);
		str2ptr=str2copy+1;
		str2copy[charstring::getLength(str2copy)-1]='\0';
	}
	uint32_t	result=charstring::compare(str1ptr,str2ptr);
	delete[] str1copy;
	delete[] str2copy;
	return result;
}

void sqlrservercontroller::getColumnsInTable(const char *table, 
					linkedlist<char *> **columns,
					const char **autoinccolumn,
					const char **primarykeycolumn) {

	// attempt to get the columns from various caches
	*columns=pvt->_colcache.getValue((char *)table);
	*autoinccolumn=pvt->_autoinccolcache.getValue((char *)table);
	*primarykeycolumn=pvt->_primarykeycolcache.getValue((char *)table);
	if (*columns) {
		return;
	}

	// failing that, look them up in the database (and cache them)...

	// create a new linkedlist to store the columns for this table
	*columns=new linkedlist<char *>();
	(*columns)->setManageArrayValues(true);

	// get all of the columns in the table
	sqlrservercursor        *gclcur=newCursor();
	if (open(gclcur)) {

		bool	retval=getColumnList(gclcur,NULL,NULL,table,NULL);
		if (retval) {

			setColumnListFormat(SQLRSERVERLISTFORMAT_MYSQL);

			bool    error;
			while (fetchRow(gclcur,&error)) {

				const char	*column;
				const char	*columnkey;
				const char	*extra;
				uint64_t	fieldsize;
				bool		lob;
				bool		null;
				getField(gclcur,0,&column,
						&fieldsize,&lob,&null);
				getField(gclcur,6,&columnkey,
						&fieldsize,&lob,&null);
				getField(gclcur,8,&extra,
						&fieldsize,&lob,&null);

				char	*dup=charstring::duplicate(column);
				(*columns)->append(dup);
				if (charstring::contains(columnkey,
							"PRI")) {
					*primarykeycolumn=dup;
				}
				if (charstring::contains(extra,
							"auto_increment")) {
					*autoinccolumn=dup;
				}

				// FIXME: kludgy
				nextRow(gclcur);
			}

			setColumnListFormat(SQLRSERVERLISTFORMAT_NULL);
		}
	}
	closeResultSet(gclcur);
	close(gclcur);
	deleteCursor(gclcur);

	// cache table -> columns/autoinccolumns mappings
	char	*tablecopy=charstring::duplicate(table);
	pvt->_colcache.setValue(tablecopy,*columns);
	pvt->_autoinccolcache.setValue(tablecopy,*autoinccolumn);
	pvt->_primarykeycolcache.setValue(tablecopy,*primarykeycolumn);
}

void sqlrservercontroller::getColumnsFromInsertQuery(
					const char *start,
					const char *queryend,
					linkedlist<char *> *columns) {
	// split the provided set of comma-separated columns
	char		**cols=NULL;
	uint64_t	colcount=0;
	charstring::split(start,queryend-start,",",true,&cols,&colcount);
	columns->listcollection<char *>::append(cols,colcount);
	delete[] cols;
}

void sqlrservercontroller::getFirstValuesFromInsertQuery(
						const char *start,
						const char *queryend,
						linkedlist<char *> *values,
						bool *multiinsert) {
	const char	*c=start;
	uint32_t	parens=0;
	const char	*startofvalue=c;
	for (;;) {

		// handle quotes
		if (*c=='\'') {
			c=charstring::findEndOfQuotedString(
						c,queryend-c,'\'',true,true);
		}

		// handle parens
		if (*c=='(') {

			parens++;

		} else if (*c==')') {

			if (parens) {
				parens--;
			} else {
				// copy out the last value
				values->append(charstring::duplicate(
							startofvalue,
							c-startofvalue));

				// if there's a comma after the closing
				// paren, then this is a multi-insert
				c++;
				*multiinsert=(*c==',');
				return;
			}

		} else

		// handle commas between values
		if (*c==',') {

			// copy out the value if we found a comma
			values->append(charstring::duplicate(
							startofvalue,
							c-startofvalue));
			startofvalue=c+1;
		}

		// keep going
		c++;
	}
}

void sqlrservercontroller::deriveColumnsFromInsertQuery(
					linkedlist<char *> *values,
					linkedlist<char *> *allcolumns,
					linkedlist<char *> *columns) {
	// step through the values, populating columns
	// with the corresponding column from allcolumns
	listnode<char *>	*vnode=values->getFirst();
	listnode<char *>	*acnode=allcolumns->getFirst();
	while (vnode && acnode) {
		columns->append(charstring::duplicate(acnode->getValue()));
		vnode=vnode->getNext();
		acnode=acnode->getNext();
	}
}

bool sqlrservercontroller::isBitType(const char *type) {
	return ::isBitTypeChar(type);
}

bool sqlrservercontroller::isBitType(int16_t type) {
	return ::isBitTypeInt(type);
}

bool sqlrservercontroller::isBoolType(const char *type) {
	return ::isBoolTypeChar(type);
}

bool sqlrservercontroller::isBoolType(int16_t type) {
	return ::isBoolTypeInt(type);
}

bool sqlrservercontroller::isFloatType(const char *type) {
	return ::isFloatTypeChar(type);
}

bool sqlrservercontroller::isFloatType(int16_t type) {
	return ::isFloatTypeInt(type);
}

bool sqlrservercontroller::isNumberType(const char *type) {
	return ::isNumberTypeChar(type);
}

bool sqlrservercontroller::isNumberType(int16_t type) {
	return ::isNumberTypeInt(type);
}

bool sqlrservercontroller::isBlobType(const char *type) {
	return ::isBlobTypeChar(type);
}

bool sqlrservercontroller::isBlobType(int16_t type) {
	return ::isBlobTypeInt(type);
}

bool sqlrservercontroller::isClobType(const char *type) {
	return ::isClobTypeChar(type);
}

bool sqlrservercontroller::isClobType(int16_t type) {
	return ::isClobTypeInt(type);
}

bool sqlrservercontroller::isUnsignedType(const char *type) {
	return ::isUnsignedTypeChar(type);
}

bool sqlrservercontroller::isUnsignedType(int16_t type) {
	return ::isUnsignedTypeInt(type);
}

bool sqlrservercontroller::isBinaryType(const char *type) {
	return ::isBinaryTypeChar(type);
}

bool sqlrservercontroller::isBinaryType(int16_t type) {
	return ::isBinaryTypeInt(type);
}

bool sqlrservercontroller::isDateTimeType(const char *type) {
	return ::isDateTimeTypeChar(type);
}

bool sqlrservercontroller::isDateTimeType(int16_t type) {
	return ::isDateTimeTypeInt(type);
}

const char * const *sqlrservercontroller::dataTypeStrings() {
	return datatypestring;
}

// FIXME: push loglevel/eventtype up and consolidate somehow
static const char *loglevels[]={"DEBUG","INFO","WARNING","ERROR"};

const char *sqlrservercontroller::getLogLevel(sqlrloglevel_t level) {
	return loglevels[(uint8_t)level];
}

sqlrloglevel_t sqlrservercontroller::getLogLevel(const char *level) {
	uint16_t	retval=SQLRLOGGER_LOGLEVEL_DEBUG;
	for (const char * const *ll=loglevels; *ll; ll++) {
		if (!charstring::compareIgnoringCase(level,*ll)) {
			break;
		}
		retval++;
	}
	return (sqlrloglevel_t)retval;
}

bool sqlrservercontroller::applyDirectives(sqlrservercursor *cursor) {

	if (pvt->_debugsqlrdirectives) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("applying directives...\n");
	}

	// apply translation rules
	const char	*query=cursor->getQueryBuffer();
	if (!pvt->_sqlrd->run(pvt->_conn,cursor,query)) {
		if (pvt->_debugsqlrdirectives) {
			stdoutput.printf("a directive failed\n");
		}
		// FIXME: raise directive failed event...
		// FIXME: return an error somehow
		return false;
	}

	return true;
}

bool sqlrservercontroller::translateQuery(sqlrservercursor *cursor) {

	const char	*query=cursor->getQueryBuffer();
	uint32_t	querysize=cursor->getQuerySize();

	if (pvt->_debugsqlrquerytranslations) {
		stdoutput.write("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.write("translating query...\n\n");
		stdoutput.write("original:\n\"");
		stdoutput.safePrint(query,querysize);
		stdoutput.write("\"\n");
	}

	// clear the query tree
	cursor->clearQueryTree();

	// apply translation rules
	stringbuffer	*translatedquery=cursor->getTranslatedQueryBuffer();
	translatedquery->clear();
	if (!pvt->_sqlrt->run(pvt->_conn,cursor,pvt->_sqlrp,
					query,querysize,translatedquery)) {
		raiseTranslationFailureEvent(cursor,query);
		if (pvt->_sqlrt->getUseOriginalOnError()) {
			if (pvt->_debugsqlrquerytranslations) {
				stdoutput.write("translation failed, "
						"using original:\n\"");
				stdoutput.safePrint(query,querysize);
				stdoutput.write("\"\n");
			}
			return true;
		}
		setError(cursor,pvt->_sqlrt->getError(),
				SQLR_ERROR_QUERYTRANSLATION,true);
		return false;
	}

	// update the query tree
	if (pvt->_sqlrp) {
		cursor->setQueryTree(pvt->_sqlrp->detachQueryTree());
	}

	if (pvt->_debugsqlrquerytranslations) {
		stdoutput.write("translated:\n\"");
		stdoutput.safePrint(translatedquery->getString(),
					translatedquery->getSize());
		stdoutput.write("\"\n");
	}

	// bail if the translated query is too large
	if (translatedquery->getSize()>pvt->_maxquerysize) {
		if (pvt->_debugsqlrquerytranslations) {
			stdoutput.write("translated query too large\n");
		}
		return false;
	}

	// replace with a noop if the query is empty
	if (charstring::isNullOrEmpty(translatedquery->getString())) {
		translatedquery->append(pvt->_conn->getNoopQuery());
	}

	// write the translated query to the cursor's query buffer
	// so it'll be there if we decide to re-execute it later
	bytestring::copy(cursor->getQueryBuffer(),
			translatedquery->getString(),
			translatedquery->getSize());
	cursor->setQuerySize(translatedquery->getSize());
	cursor->getQueryBuffer()[cursor->getQuerySize()]='\0';
	return true;
}

void sqlrservercontroller::translateBindVariables(sqlrservercursor *cursor) {

	// clear bind mappings
	cursor->getBindMappings()->clear();

	// get query buffer
	char	*querybuffer=cursor->getQueryBuffer();

	// debug
	if (pvt->_debugbindtranslation) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("translating bind variables...\n");
		stdoutput.printf("original:\n%s\n",querybuffer);
	}
	if (getLoggingEnabled()) {
		debugStart("translating bind variables");
		debugWrite("original:");
		debugWrite(querybuffer);
	}

	// convert queries from whatever bind variable format they currently
	// use to the format required by the database...

	bool			translated=false;
	queryparsestate_t	parsestate=IN_QUERY;
	stringbuffer		newquery;
	stringbuffer		currentbind;

	// use 1-based index for bind variables
	uint16_t	bindindex=1;
	
	// run through the querybuffer...
	const char	*ptr=querybuffer;
	const char	*endptr=querybuffer+cursor->getQuerySize();
	char		prev='\0';
	do {

		// if we're in the query...
		if (parsestate==IN_QUERY) {

			// if we find a quote, we're in quotes
			if (*ptr=='\'') {
				parsestate=IN_QUOTES;
			}

			// if we find whitespace or a couple of other things
			// then the next thing could be a bind variable
			if (beforeBindVariable(ptr)) {
				parsestate=BEFORE_BIND;
			}

			// append the character
			newquery.append(*ptr);

			// move on
			if (*ptr=='\\' && prev=='\\') {
				prev='\0';
			} else {
				prev=*ptr;
			}
			ptr++;
			continue;
		}

		// copy anything in quotes verbatim
		if (parsestate==IN_QUOTES) {

			// if we find a quote, but not an escaped quote,
			// then we're back in the query
			// (or we're in between one of these: '...''...'
			// which is functionally the same)
			if (*ptr=='\'' && prev!='\\') {
				parsestate=IN_QUERY;
			}

			// append the character
			newquery.append(*ptr);

			// move on
			if (*ptr=='\\' && prev=='\\') {
				prev='\0';
			} else {
				prev=*ptr;
			}
			ptr++;
			continue;
		}

		if (parsestate==BEFORE_BIND) {

			// if we find a bind variable...
			if (isBindDelimiter(ptr,
					pvt->_questionmarksupported,
					pvt->_colonsupported,
					pvt->_atsignsupported,
					pvt->_dollarsignsupported)) {
				parsestate=IN_BIND;
				currentbind.clear();
				continue;
			}

			// if we didn't find a bind variable then we're just
			// back in the query
			parsestate=IN_QUERY;
			continue;
		}

		// if we're in a bind variable...
		if (parsestate==IN_BIND) {

			// If we find whitespace or a few other things
			// then we're done with the bind variable.  Process it.
			// Otherwise get the variable itself in another buffer.
			bool	endofbind=afterBindVariable(ptr);
			if (endofbind || ptr==endptr-1) {

				// special case...
				// last character in the query
				if (!endofbind && ptr==endptr-1) {
					currentbind.append(*ptr);
					if (*ptr=='\\' && prev=='\\') {
						prev='\0';
					} else {
						prev=*ptr;
					}
					ptr++;
				}

				// if the current bind variable format doesn't
				// match the db bind format...
				if (!matchesNativeBindFormat(
						currentbind.getString())) {

					// translate...
					translated=true;
					translateBindVariableInStringAndMap(
								cursor,
								&currentbind,
								bindindex,
								&newquery);
				} else {
					newquery.append(
						currentbind.getString());
				}
				bindindex++;

				parsestate=IN_QUERY;

			} else {

				// move on
				currentbind.append(*ptr);
				if (*ptr=='\\' && prev=='\\') {
					prev='\0';
				} else {
					prev=*ptr;
				}
				ptr++;
			}
			continue;
		}

	} while (ptr<endptr);


	// if no translation was performed
	if (!translated) {
		if (pvt->_debugbindtranslation) {
			stdoutput.printf(
				"\n  no bind translation performed\n\n");
		}
		debugWrite("no bind translation performed");
		debugEnd();
		return;
	}


	// if we made it here then some conversion
	// was done - update the querybuffer...
	const char	*newq=newquery.getString();
	uint32_t	newqsize=newquery.getSize();
	if (newqsize>pvt->_maxquerysize) {
		newqsize=pvt->_maxquerysize;
	}
	bytestring::copy(querybuffer,newq,newqsize);
	querybuffer[newqsize]='\0';
	cursor->setQuerySize(newqsize);


	// debug
	if (pvt->_debugbindtranslation) {
		stdoutput.printf("\ntranslated:\n%s\n\n",querybuffer);
	}
	if (getLoggingEnabled()) {
		debugWrite("translated:");
		debugWrite(querybuffer);
	}
	debugEnd();
}

bool sqlrservercontroller::matchesNativeBindFormat(const char *bind) {

	const char	*bindformat=pvt->_conn->getBindFormat();
	size_t		bindformatsize=charstring::getLength(bindformat);

	// the bind variable name matches the format if...
	// * the first character of the bind variable name matches the 
	//   first character of the bind format
	//
	//	and...
	//
	// * the format is just a single character
	// 	or..
	// * the second character of the format is a 1 and the second character
	//   of the bind variable name is a digit
	// 	or..
	// * the second character of the format is a * and the second character
	//   of the bind varaible name is alphanumeric
	return (bind[0]==bindformat[0]  &&
		(bindformatsize==1 ||
		(bindformat[1]=='1' && character::isDigit(bind[1])) ||
		(bindformat[1]=='*' && character::isAlphanumeric(bind[1]))));
}

void sqlrservercontroller::translateBindVariableInStringAndMap(
						sqlrservercursor *cursor,
						stringbuffer *currentbind,
						uint16_t bindindex,
						stringbuffer *newquery) {

	// get the bind format
	const char	*bindformat=pvt->_conn->getBindFormat();

	// replace the bind variable delimiter with whatever we would expect to
	// find for this database
	currentbind->setPositionRelativeToBeginning(0);
	currentbind->write(bindformat[0]);

	// append the first character of the bind format to the new query
	newquery->append(bindformat[0]);

	if (bindformat[1]=='\0') {

		// This section handles single-character bind variable
		// placeholder such as ?'s. (mysql, db2 and firebird format)

		// replace bind variable itself with number
		mapBindVariable(cursor,currentbind->getString(),
					currentbind->getSize(),
					bindindex);

	} else if (bindformat[1]=='1' &&
			!charstring::isNumber(currentbind->getString()+1)) {

		// This section handles 2-character placeholders where the
		// second position is a number, such as $1 (postgresql-format).

		// replace bind variable in string with number
		newquery->append(bindindex);

		// replace bind variable itself with number
		mapBindVariable(cursor,currentbind->getString(),
					currentbind->getSize(),
					bindindex);

	} else {

		// This section handles everything else, such as :*, @*.
		// (oracle, sybase, and ms sql server formats)

		// If the current bind variable was a single character
		// placeholder (such as a ?) then replace it with a delimited
		// number.  Otherwise use it as-is...

		if (currentbind->getStringLength()==1) {

			// replace bind variable in string with number
			newquery->append(bindindex);

			// replace bind variable itself with number
			mapBindVariable(cursor,currentbind->getString(),
						currentbind->getSize(),
						bindindex);
		} else {
			newquery->append(currentbind->getString()+1,
					currentbind->getSize()-1);
		}
	}
}

void sqlrservercontroller::mapBindVariable(sqlrservercursor *cursor,
						const char *bindvariable,
						uint64_t bindvariablesize,
						uint16_t bindindex) {

	// if the current bind variable is a ? then just
	// set it NULL for special handling later
	if (!charstring::compare(bindvariable,"?")) {
		bindvariable=NULL;
	}

	// create the new bind var name and get its size
	char		*tempnumber=charstring::parseNumber(bindindex);
	uint16_t	tempnumbersize=charstring::getLength(tempnumber);

	char	*oldvariable=(char *)cursor->getBindMappingsPool()->
						allocate(bindvariablesize+1);
	char	*newvariable=(char *)cursor->getBindMappingsPool()->
						allocate(tempnumbersize+2);

	charstring::copy(oldvariable,bindvariable,bindvariablesize);
	oldvariable[bindvariablesize]='\0';

	newvariable[0]=getBindFormat()[0];
	charstring::copy(newvariable+1,tempnumber);
	newvariable[tempnumbersize+1]='\0';

	// map existing name to new name
	cursor->getBindMappings()->setValue(oldvariable,newvariable);
				
	// clean up
	delete[] tempnumber;
}

void sqlrservercontroller::translateBindVariablesFromMappings(
						sqlrservercursor *cursor) {

	// index variable
	uint16_t	i=0;

	// debug and logging
	if (pvt->_debugbindtranslation) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("remapping bind variables:\n");
		stdoutput.printf("  input binds:\n");
		for (i=0; i<cursor->getInputBindCount(); i++) {
			stdoutput.printf("    %s\n",
				cursor->getInputBinds()[i].variable);
		}
		stdoutput.printf("  output binds:\n");
		for (i=0; i<cursor->getOutputBindCount(); i++) {
			stdoutput.printf("    %s\n",
				cursor->getOutputBinds()[i].variable);
		}
		stdoutput.printf("  input/output binds:\n");
		for (i=0; i<cursor->getInputOutputBindCount(); i++) {
			stdoutput.printf("    %s\n",
				cursor->getInputOutputBinds()[i].variable);
		}
		stdoutput.printf("\n");
	}
	debugStart("remapping bind variables");
	if (getLoggingEnabled()) {
		debugStart("input binds");
		for (i=0; i<cursor->getInputBindCount(); i++) {
			debugWrite(
				cursor->getInputBinds()[i].variable);
		}
		debugEnd();
		debugStart("output binds");
		for (i=0; i<cursor->getOutputBindCount(); i++) {
			debugWrite(
				cursor->getOutputBinds()[i].variable);
		}
		debugEnd();
		debugStart("input/output binds");
		for (i=0; i<cursor->getInputOutputBindCount(); i++) {
			debugWrite(
				cursor->getInputOutputBinds()[i].variable);
		}
		debugEnd();
	}

	// run three passes - input binds, output binds, input/output binds
	bool	remapped=false;
	for (i=0; i<3; i++) {

		dictionary<char *, char *>	*mappings=
						cursor->getBindMappings();
		uint16_t		count=0;
		sqlrserverbindvar	*vars=NULL;
		if (i==0) {
			count=cursor->getInputBindCount();
			vars=cursor->getInputBinds();
		} else if (i==1) {
			count=cursor->getOutputBindCount();
			vars=cursor->getOutputBinds();
		} else if (i==2) {
			count=cursor->getInputOutputBindCount();
			vars=cursor->getInputOutputBinds();
		}

		for (uint16_t j=0; j<count; j++) {

			// get the bind var
			sqlrserverbindvar	*b=&(vars[j]);

			// remap it
			char	*newvariable;
			if (mappings->getValue(b->variable,&newvariable)) {
				b->variable=newvariable;
				b->variablesize=
					charstring::getLength(b->variable);
				remapped=true;
			}
		}
	}

	// if no remapping was performed
	if (!remapped) {
		if (pvt->_debugbindtranslation) {
			stdoutput.printf("  no variables remapped\n\n");
		}
		debugWrite("no variables remapped");
		debugEnd();
		return;
	}

	// debug and logging
	if (pvt->_debugbindtranslation) {
		stdoutput.printf("  remapped input binds:\n");
		for (i=0; i<cursor->getInputBindCount(); i++) {
			stdoutput.printf("    %s\n",
				cursor->getInputBinds()[i].variable);
		}
		stdoutput.printf("  remapped output binds:\n");
		for (i=0; i<cursor->getOutputBindCount(); i++) {
			stdoutput.printf("    %s\n",
				cursor->getOutputBinds()[i].variable);
		}
		stdoutput.printf("  remapped input/output binds:\n");
		for (i=0; i<cursor->getOutputBindCount(); i++) {
			stdoutput.printf("    %s\n",
				cursor->getInputOutputBinds()[i].variable);
		}
		stdoutput.printf("\n");
	}
	if (getLoggingEnabled()) {
		debugStart("remapped input binds");
		for (i=0; i<cursor->getInputBindCount(); i++) {
			debugWrite(
				cursor->getInputBinds()[i].variable);
		}
		debugEnd();
		debugStart("remapped output binds");
		for (i=0; i<cursor->getOutputBindCount(); i++) {
			debugWrite(
				cursor->getOutputBinds()[i].variable);
		}
		debugEnd();
		debugStart("remapped input/output binds");
		for (i=0; i<cursor->getInputOutputBindCount(); i++) {
			debugWrite(
				cursor->getInputOutputBinds()[i].variable);
		}
		debugEnd();
	}
	debugEnd();
}

void sqlrservercontroller::translateBeginTransaction(sqlrservercursor *cursor) {

	// get query buffer
	char	*querybuffer=cursor->getQueryBuffer();

	// debug
	debugStart("translating begin tx query...");
	debugWrite("original:");
	debugWrite(querybuffer);

	// translate query
	const char	*beginquery=pvt->_conn->beginTransactionQuery();
	uint32_t	querysize=charstring::getLength(beginquery);
	charstring::copy(querybuffer,beginquery,querysize);
	querybuffer[querysize]='\0';
	cursor->setQuerySize(querysize);

	// debug
	debugWrite("converted:");
	debugWrite(querybuffer);
	debugEnd();
}

bool sqlrservercontroller::filterQuery(sqlrservercursor *cursor, bool before) {

	const char	*query=cursor->getQueryBuffer();

	if (pvt->_debugsqlrfilters) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("filtering:\n\"%s\"\n\n",query);
	}

	// apply filters
	const char	*err=NULL;
	int64_t		errn=0;
	bool		success=(before)?
				pvt->_sqlrf->runBeforeFilters(pvt->_conn,
								cursor,
								pvt->_sqlrp,
								query,
								&err,&errn):
				pvt->_sqlrf->runAfterFilters(pvt->_conn,
								cursor,
								pvt->_sqlrp,
								query,
								&err,&errn);
	if (!success) {
		setError(cursor,err,errn,true);
		raiseFilterViolationEvent(cursor);
		if (pvt->_debugsqlrfilters) {
			stdoutput.printf("query filtered out\n");
		} 
		return false;
	}

	if (pvt->_debugsqlrfilters) {
		stdoutput.printf("query accepted\n");
	}
	return true;
}

sqlrservercursor *sqlrservercontroller::useCustomQueryCursor(	
						sqlrservercursor *cursor) {

	// do we need to use a custom query cursor for this query?

	// not if custom queries aren't enabled...
	if (!pvt->_sqlrq) {
		return cursor;
	}

	// see if the query matches one of the custom queries
	// FIXME: the 0 below isn't safe, none of the custom queries do
	// anything with the id, but they might in the future so it needs to be
	// unique
	sqlrquerycursor	*customcursor=pvt->_sqlrq->match(
						pvt->_conn,
						cursor->getQueryBuffer(),
						cursor->getQuerySize(),0);
				
	// if not...
	if (!customcursor) {
		return cursor;
	}

	// if so...

	// open the custom cursor
	customcursor->open();

	// copy the query that we just got into
	// the custom query cursor's buffers
	bytestring::copy(
		customcursor->getQueryBuffer(),
		cursor->getQueryBuffer(),
		cursor->getQuerySize());
	customcursor->getQueryBuffer()[cursor->getQuerySize()]='\0';
	customcursor->setQuerySize(cursor->getQuerySize());

	// set the custom cursor' state
	customcursor->setState(SQLRCURSORSTATE_BUSY);

	// attach the custom cursor to the cursor
	cursor->setCustomQueryCursor(customcursor);

	// return the custom cursor
	return customcursor;
}

bool sqlrservercontroller::isCustomQuery(sqlrservercursor *cursor) {
	return cursor->isCustomQuery();
}

bool sqlrservercontroller::handleBinds(sqlrservercursor *cursor) {

	// translate binds
	if (pvt->_sqlrbvt && cursor->getInputBindCount()) {
		if (pvt->_debugsqlrbindvariabletranslation) {
			stdoutput.printf("\n===================="
				 	"===================="
				 	"===================="
				 	"===================\n\n");
			stdoutput.printf("translating bind variables:\n");
		}
		if (!pvt->_sqlrbvt->run(pvt->_conn,cursor)) {
			setError(cursor,pvt->_sqlrbvt->getError(),
				SQLR_ERROR_BINDVARIABLETRANSLATION,true);
			return false;
		}
	}

	sqlrserverbindvar	*bind=NULL;
	
	// iterate through the arrays, binding values to variables
	for (int16_t in=0; in<cursor->getInputBindCount(); in++) {

		bind=&cursor->getInputBinds()[in];

		// bind the value to the variable
		if (bind->type==SQLRSERVERBINDVARTYPE_STRING ||
				bind->type==SQLRSERVERBINDVARTYPE_NULL) {
			if (!cursor->inputBind(
					bind->variable,
					bind->variablesize,
					bind->value.stringval,
					bind->valuesize,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_INTEGER) {
			if (!cursor->inputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.integerval)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_DOUBLE) {
			if (!cursor->inputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.doubleval.value,
					bind->value.doubleval.precision,
					bind->value.doubleval.scale)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_DATE) {
			if (!cursor->inputBind(
					bind->variable,
					bind->variablesize,
					bind->value.dateval.year,
					bind->value.dateval.month,
					bind->value.dateval.day,
					bind->value.dateval.hour,
					bind->value.dateval.minute,
					bind->value.dateval.second,
					bind->value.dateval.microsecond,
					bind->value.dateval.tz,
					bind->value.dateval.isnegative,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_BLOB) {
			if (!cursor->inputBindBlob(
					bind->variable,
					bind->variablesize,
					bind->value.stringval,
					bind->valuesize,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_CLOB) {
			if (!cursor->inputBindClob(
					bind->variable,
					bind->variablesize,
					bind->value.stringval,
					bind->valuesize,
					&bind->isnull)) {
				return false;
			}
		}
	}

	for (int16_t out=0; out<cursor->getOutputBindCount(); out++) {

		bind=&cursor->getOutputBinds()[out];

		// bind the value to the variable
		if (bind->type==SQLRSERVERBINDVARTYPE_STRING) {
			if (!cursor->outputBind(
					bind->variable,
					bind->variablesize,
					bind->value.stringval,
					bind->valuesize+1,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_INTEGER) {
			if (!cursor->outputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.integerval,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_DOUBLE) {
			if (!cursor->outputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.doubleval.value,
					&bind->value.doubleval.precision,
					&bind->value.doubleval.scale,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_DATE) {
			if (!cursor->outputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.dateval.year,
					&bind->value.dateval.month,
					&bind->value.dateval.day,
					&bind->value.dateval.hour,
					&bind->value.dateval.minute,
					&bind->value.dateval.second,
					&bind->value.dateval.microsecond,
					(const char **)&bind->value.dateval.tz,
					&bind->value.dateval.isnegative,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_BLOB) {
			if (!cursor->outputBindBlob(
					bind->variable,
					bind->variablesize,out,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_CLOB) {
			if (!cursor->outputBindClob(
					bind->variable,
					bind->variablesize,out,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_CURSOR) {

			bool	found=false;

			// find the cursor that we acquired earlier...
			for (uint16_t j=0; j<pvt->_cursorcount; j++) {

				if (pvt->_cur[j]->getId()==
						bind->value.cursorid) {
					found=true;

					// bind the cursor
					if (!cursor->outputBindCursor(
							bind->variable,
							bind->variablesize,
							pvt->_cur[j])) {
						return false;
					}
					break;
				}
			}

			// this shouldn't happen, but if it does, return false
			if (!found) {
				return false;
			}
		}
	}

	for (int16_t inout=0;
		inout<cursor->getInputOutputBindCount(); inout++) {

		bind=&cursor->getInputOutputBinds()[inout];

		// bind the value to the variable
		if (bind->type==SQLRSERVERBINDVARTYPE_STRING ||
				bind->type==SQLRSERVERBINDVARTYPE_NULL) {
			if (!cursor->inputOutputBind(
					bind->variable,
					bind->variablesize,
					bind->value.stringval,
					bind->valuesize+1,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_INTEGER) {
			if (!cursor->inputOutputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.integerval,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_DOUBLE) {
			if (!cursor->inputOutputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.doubleval.value,
					&bind->value.doubleval.precision,
					&bind->value.doubleval.scale,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_DATE) {
			if (!cursor->inputOutputBind(
					bind->variable,
					bind->variablesize,
					&bind->value.dateval.year,
					&bind->value.dateval.month,
					&bind->value.dateval.day,
					&bind->value.dateval.hour,
					&bind->value.dateval.minute,
					&bind->value.dateval.second,
					&bind->value.dateval.microsecond,
					(const char **)&bind->value.dateval.tz,
					&bind->value.dateval.isnegative,
					&bind->isnull)) {
				return false;
			}
		} /*else if (bind->type==SQLRSERVERBINDVARTYPE_BLOB) {
			if (!cursor->inputOutputBindBlob(
					bind->variable,
					bind->variablesize,inout,
					&bind->isnull)) {
				return false;
			}
		} else if (bind->type==SQLRSERVERBINDVARTYPE_CLOB) {
			if (!cursor->inputOutputBindClob(
					bind->variable,
					bind->variablesize,inout,
					&bind->isnull)) {
				return false;
			}
		}*/
	}

	return true;
}

bool sqlrservercontroller::prepareQuery(sqlrservercursor *cursor,
						const char *query,
						uint32_t querysize) {
	return prepareQuery(cursor,query,querysize,false,false,false);
}

bool sqlrservercontroller::prepareQuery(sqlrservercursor *cursor,
						const char *query,
						uint32_t querysize,
						bool enabledirectives,
						bool enabletranslations,
						bool enablefilters) {

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d:%d:prepare:\n",
					process::getProcessId(),
					cursor->getId());
		stdoutput.write(query,querysize);
		stdoutput.write('\n');
	}

	// The standard paradigm is:
	//
	// * prepare(query)
	// * bind(variable,value)
	// * execute()
	//
	// Under various circumstances, we may need to fake binds though.  This
	// requires the query to be rewritten to include the bind values before
	// being prepared.  In that case, we must defer preparing the query
	// until the execution phase so that we'll be sure to have all of the
	// bind values on hand ("lazy prepares").
	//
	// The sqlrclient protocol is fine with that, but other protocols like
	// mysql and tds want to be able to return column info after prepare
	// but before execution.
	//
	// So, we can't just generally do lazy-prepares.  Instead, we'll follow
	// the standard paradigm when we can, and lazy-prepare when we must.
	//
	// That way, non-sqlrclient protocols will work as expected, unless
	// fakeinputbinds="yes" is explicitly set.
	//
	// There are some queries that must be fake-bind'ed, and thus
	// lazy-prepared (eg. Oracle's "create as select", and some mysql
	// queries...) but they don't return a result set.  So, they'll end up
	// working fine, even with protocols that want to return column info
	// after prepare, because it turns out that there's no column info to
	// return anyway.

	// clean up the previous result set
	closeResultSet(cursor);

	// re-init error data
	clearError(cursor);

	// reset flags
	cursor->setColumnInfoIsValid(false);
	cursor->setQueryHasBeenPreProcessed(false);
	cursor->setQueryHasBeenPrepared(false);
	cursor->setQueryHasBeenExecuted(false);
	cursor->setQueryNeedsIntercept(false);
	cursor->setQueryWasIntercepted(false);
	cursor->setBindsWereFaked(false);
	cursor->setFakeInputBindsForThisQuery(pvt->_fakeinputbinds);
	cursor->setQueryStatus(SQLRQUERYSTATUS_ERROR);
	cursor->setQueryType(SQLRQUERYTYPE_ETC);
	cursor->setResultSetHeaderHasBeenHandled(false);

	// reset column mapping
	pvt->_columnmap=NULL;
	pvt->_columnnamemap=NULL;

	// sanity check
	if (querysize>pvt->_maxquerysize) {
		querysize=pvt->_maxquerysize;
		cursor->setQuerySize(pvt->_maxquerysize);
		// FIXME: Should we throw an error here?  If not, we'll end up
		// trying to execute a partial query, which could fail, or
		// could just truncate part of the where clause, yielding a
		// valid, but incorrect result.
	}

	// copy query to cursor's query buffer if necessary
	if (query!=cursor->getQueryBuffer()) {
		bytestring::copy(cursor->getQueryBuffer(),query,querysize);
		cursor->getQueryBuffer()[querysize]='\0';
		cursor->setQuerySize(querysize);
	}

	// bail if we are just generally configured to fake input binds
	if (cursor->getFakeInputBindsForThisQuery()) {
		return true;
	}

	// determine the query type
	cursor->setQueryType(cursor->determineQueryType(
					cursor->getQueryBuffer(),
					cursor->getQuerySize()));

	// do this here instead of inside translateBindVariables
	// because translateQuery might use it
	cursor->getBindMappingsPool()->clear();

	// before-filter query
	if (enablefilters && pvt->_sqlrf) {
		if (!filterQuery(cursor,true)) {

			// raise query-related events
			raiseQueryReceivedEvent(cursor);
			raiseQueryPreparedEvent(cursor);
			raiseQueryExecutedEvent(cursor);

			cursor->setQueryStatus(
				SQLRQUERYSTATUS_FILTER_VIOLATION);
			return false;
		}
	}

	// apply directives
	if (enabledirectives && pvt->_sqlrd) {
		applyDirectives(cursor);
	}

	// translate query
	if (enabletranslations && pvt->_sqlrt) {

		if (translateQuery(cursor)) {

			// re-determine the query type
			cursor->setQueryType(
				cursor->determineQueryType(
					cursor->getQueryBuffer(),
					cursor->getQuerySize()));

		} else {

			// raise query-related events
			raiseQueryReceivedEvent(cursor);
			raiseQueryPreparedEvent(cursor);
			raiseQueryExecutedEvent(cursor);

			// error is already set by translateQuery()
			return false;
		}
	}

	// translate bind variables
	if (pvt->_translatebinds) {
		translateBindVariables(cursor);
	}

	// translate "begin" queries
	// FIXME: can we just let interceptQuery below handle this?
	if ((supportsExplicitTransactions() || fakingExplicitTransactions()) &&
				cursor->getQueryType()==SQLRQUERYTYPE_BEGIN) {
		translateBeginTransaction(cursor);
	}

	// after-filter query
	if (enablefilters && pvt->_sqlrf) {
		if (!filterQuery(cursor,false)) {

			// raise query-related events
			raiseQueryReceivedEvent(cursor);
			raiseQueryPreparedEvent(cursor);
			raiseQueryExecutedEvent(cursor);

			cursor->setQueryStatus(
				SQLRQUERYSTATUS_FILTER_VIOLATION);
			return false;
		}
	}

	// (re)get the query now that it's been translated
	query=cursor->getQueryBuffer();
	querysize=cursor->getQuerySize();
	if (enabletranslations && pvt->_sqlrt &&
			pvt->_debugsql && !pvt->_debugsqlrquerytranslations) {
		stdoutput.printf("\n%d:%d:translated:\n%.*s\n",
					process::getProcessId(),
					cursor->getId(),
					querysize,query);
	}

	// fake input binds if this specific query doesn't support them
	if (!cursor->supportsNativeBinds(query,querysize)) {
		cursor->setFakeInputBindsForThisQuery(true);
	}

	// set flag indicating that the query has been preprocessed
	cursor->setQueryHasBeenPreProcessed(true);

	// Check to see if the query needs to be intercepted.  Don't
	// actually intercept it yet, but bail if it needs to be.
	cursor->setQueryNeedsIntercept(checkInterceptQuery(cursor));
	if (cursor->getQueryNeedsIntercept()) {
		return true;
	}

	// Bail if this query should fake input binds.  This an happen if:
	// * the instance is generally configured to fake input binds
	// * one of the translations has set the
	// 	fakeinputbindsforthisquery flag true
	// * the specific query doesn't support native binds
	// In any of these cases, the cursor's fakeinputbindsforthisquery
	// flag will have been set true.
	if (cursor->getFakeInputBindsForThisQuery()) {
		return true;
	}

	debugStart("preparing query");

	// log query-received
	raiseQueryReceivedEvent(cursor);

	// set the query start time (in case the prepare fails)
	datetime	dt;
	dt.initFromSystemDateTime();
	cursor->setQueryStart(dt.getSecond(),dt.getMicrosecond());

	// prepare the query
	bool	success=cursor->prepareQuery(query,querysize);

	// log query-prepared
	raiseQueryPreparedEvent(cursor);

	if (!success) {

		// set the query end time
		dt.initFromSystemDateTime();
		cursor->setQueryEnd(dt.getSecond(),dt.getMicrosecond());

		// update query and error counts
		incrementQueryCount(cursor->getQueryType());
		incrementTotalErrors();

		// save the error
		saveError(cursor);
		debugWrite("failed: \"%.*s\"",
					cursor->getErrorSize(),
					cursor->getErrorBuffer());

		// log query-executed (attempt)
		raiseQueryExecutedEvent(cursor);

		debugEnd();
		return false;
	}

	// set flag indicating that the query has been prepared
	cursor->setQueryHasBeenPrepared(true);

	debugWrite("success");
	debugEnd();

	// handle column info now if it's valid at this point
	return (columnInfoIsValidAfterPrepare(cursor))?
				handleResultSetHeader(cursor):true;
}

void sqlrservercontroller::setQuerySuppressed(sqlrservercursor *cursor,
							bool querysuppressed) {
	cursor->setQuerySuppressed(querysuppressed);
}

bool sqlrservercontroller::getQuerySuppressed(sqlrservercursor *cursor) {
	return cursor->getQuerySuppressed();
}

bool sqlrservercontroller::executeQuery(sqlrservercursor *cursor) {
	return executeQuery(cursor,false,false,false,false);
}

bool sqlrservercontroller::executeQuery(sqlrservercursor *cursor,
						bool enabledirectives,
						bool enabletranslations,
						bool enablefilters,
						bool enabletriggers) {

	// set state
	setState((isCustomQuery(cursor))?PROCESS_CUSTOM:PROCESS_SQL);

	// reset affected rows
	pvt->_overrideaffectedrows.setValue(cursor,false);
	pvt->_affectedrows.setValue(cursor,0);

	// if we're re-executing
	if (cursor->getQueryHasBeenExecuted()) {

		// clean up the previous result set
		closeResultSet(cursor);

		// re-init error data
		clearError(cursor);

		// if we're faking binds then the original
		// query must be re-prepared
		if (cursor->getFakeInputBindsForThisQuery()) {
			cursor->setQueryHasBeenPrepared(false);
			cursor->setColumnInfoIsValid(false);
		}
	}

	// if the query hasn't been preprocessed then execute various
	// filters, translations, and checks
	if (!cursor->getQueryHasBeenPreProcessed()) {

		// determine the query type
		cursor->setQueryType(
			cursor->determineQueryType(
					cursor->getQueryBuffer(),
					cursor->getQuerySize()));

		// do this here instead of inside translateBindVariables
		// because translateQuery might use it
		cursor->getBindMappingsPool()->clear();

		// before-filter query
		if (enablefilters && pvt->_sqlrf) {
			if (!filterQuery(cursor,true)) {

				// log query-executed
				// FIXME: if query hasn't been prepared at this
				// point then we need to raise received and
				// prepared events too
				raiseQueryExecutedEvent(cursor);

				cursor->setQueryStatus(
					SQLRQUERYSTATUS_FILTER_VIOLATION);
				return false;
			}
		}

		// apply directives
		if (enabledirectives && pvt->_sqlrd) {
			applyDirectives(cursor);
		}

		// translate query
		if (enabletranslations && pvt->_sqlrt) {

			if (translateQuery(cursor)) {

				// re-determine the query type
				cursor->setQueryType(
					cursor->determineQueryType(
						cursor->getQueryBuffer(),
						cursor->getQuerySize()));

			} else {

				// log query-executed
				// FIXME: if query hasn't been prepared at this
				// point then we need to raise received and
				// prepared events too
				raiseQueryExecutedEvent(cursor);

				// error is already set by translateQuery()
				return false;
			}
		}

		// translate bind variables
		if (pvt->_translatebinds) {
			translateBindVariables(cursor);
		}

		// translate "begin" queries
		// FIXME: can we just let interceptQuery below handle this?
		if ((supportsExplicitTransactions() || 
					fakingExplicitTransactions()) &&
				cursor->getQueryType()==SQLRQUERYTYPE_BEGIN) {
			translateBeginTransaction(cursor);
		}

		// after-filter query
		if (enablefilters && pvt->_sqlrf) {
			if (!filterQuery(cursor,false)) {

				// log query-executed
				// FIXME: if query hasn't been prepared at this
				// point then we need to raise received and
				// prepared events too
				raiseQueryExecutedEvent(cursor);

				cursor->setQueryStatus(
					SQLRQUERYSTATUS_FILTER_VIOLATION);
				return false;
			}
		}

		// fake input binds if this specific query doesn't support them
		if (!cursor->supportsNativeBinds(
					cursor->getQueryBuffer(),
					cursor->getQuerySize())) {
			cursor->setFakeInputBindsForThisQuery(true);
		}

		// check to see if the query needs to be intercepted,
		// but don't actually intercept it yet
		cursor->setQueryNeedsIntercept(checkInterceptQuery(cursor));

		// set flag indicating that the query has been preprocessed
		cursor->setQueryHasBeenPreProcessed(true);
	}

	// Do we need to fake input binds?
	// We do if:
	// * the instance is generally configured to fake input binds
	// * one of the translations has set the
	// 	fakeinputbindsforthisquery flag true
	// * the specific query doesn't support native binds
	// In any of these cases, the cursor's fakeinputbindsforthisquery flag
	// will have been set true.
	if (cursor->getFakeInputBindsForThisQuery()) {

		debugStart("faking binds");

		if (cursor->fakeInputBinds()) {
			if (pvt->_debugbindtranslation) {
				stdoutput.printf(
				"after faking input binds:\n%s\n\n",
				cursor->
				getQueryWithFakeInputBindsBuffer()->
							getString());
			}
			cursor->setBindsWereFaked(true);
		}

		debugEnd();
	}

	// (re)set the query start time
	// (which may have been set earlier during prepare,
	// in case the prepare failed)
	datetime	dt;
	dt.initFromSystemDateTime();
	cursor->setQueryStart(dt.getSecond(),dt.getMicrosecond());

	// init result
	bool	success=true;

	// if the query needs to be intercepted, then intercept it
	if (cursor->getQueryNeedsIntercept()) {

		success=interceptQuery(cursor);
		if (cursor->getQueryWasIntercepted()) {

			// set the query end time
			dt.initFromSystemDateTime();
			cursor->setQueryEnd(dt.getSecond(),dt.getMicrosecond());

			if (success) {
				cursor->setQueryStatus(SQLRQUERYSTATUS_SUCCESS);
			}

			// log query-executed
			// FIXME: if query hasn't been prepared at this
			// point then we need to raise received and
			// prepared events too
			raiseQueryExecutedEvent(cursor);

			return success;
		}
	}

	// get the query
	const char	*query=cursor->getQueryBuffer();
	uint32_t	querysize=cursor->getQuerySize();
	if (cursor->getBindsWereFaked()) {
		query=cursor->getQueryWithFakeInputBindsBuffer()->
							getString();
		querysize=cursor->getQueryWithFakeInputBindsBuffer()->
							getSize();
	}

	// if the query still hasn't been prepared (probably
	// because we're faking binds), then prepare it now
	if (!cursor->getQueryHasBeenPrepared()) {

		debugStart("preparing query");

		// log query-received
		raiseQueryReceivedEvent(cursor);

		// set the query start time (in case the prepare fails)
		dt.initFromSystemDateTime();
		cursor->setQueryStart(dt.getSecond(),dt.getMicrosecond());

		// prepare the query
		success=cursor->prepareQuery(query,querysize);

		// log query-prepared
		raiseQueryPreparedEvent(cursor);

		// log result

		if (!success) {

			// set the query end time
			dt.initFromSystemDateTime();
			cursor->setQueryEnd(dt.getSecond(),dt.getMicrosecond());

			// update query and error counts
			incrementQueryCount(cursor->getQueryType());
			incrementTotalErrors();

			// save the error
			saveError(cursor);
			debugWrite("failed: \"%.*s\"",
						cursor->getErrorSize(),
						cursor->getErrorBuffer());

			// log query-executed (attempt)
			raiseQueryExecutedEvent(cursor);

			debugEnd();
			return false;
		}

		// set flag indicating that the query has been prepared
		cursor->setQueryHasBeenPrepared(true);

		debugWrite("success");
		debugEnd();
	}

	debugStart("executing query");

	// translate bind variables (from mappings)
	translateBindVariablesFromMappings(cursor);

	// handle binds (unless they were faked during the prepare)
	if (!cursor->getBindsWereFaked()) {

		// set the query start time (in case handleBinds fails)
		dt.initFromSystemDateTime();
		cursor->setQueryStart(dt.getSecond(),dt.getMicrosecond());

		debugStart("handling binds");

		if (!handleBinds(cursor)) {

			// set the query end time
			dt.initFromSystemDateTime();
			cursor->setQueryEnd(dt.getSecond(),dt.getMicrosecond());

			// update query and error counts
			incrementQueryCount(cursor->getQueryType());
			incrementTotalErrors();

			// get the error
			saveError(cursor);
			debugWrite("failed: \"%.*s\"",
						cursor->getErrorSize(),
						cursor->getErrorBuffer());

			// log query-executed (attempt)
			raiseQueryExecutedEvent(cursor);

			debugEnd();
			return false;
		}

		debugWrite("success");
		debugEnd();
	}

	// handle before-triggers
	setQuerySuppressed(cursor,false);
	if (enabletriggers && pvt->_sqlrtr) {
		success=pvt->_sqlrtr->runBeforeTriggers(pvt->_conn,cursor);
	}
	// FIXME: handle if runBeforeTriggers set success=false;
	

	// (re)set the query start time
	dt.initFromSystemDateTime();
	cursor->setQueryStart(dt.getSecond(),dt.getMicrosecond());

	if (pvt->_debugsql) {
		stdoutput.printf("\n===================="
				 "===================="
				 "===================="
				 "===================\n\n");
		stdoutput.printf("%d:%d:execute:\n",
					process::getProcessId(),
					cursor->getId());
		stdoutput.write(query,querysize);
		stdoutput.write('\n');
	}

	// execute the query
	if (success) {
		if (!getQuerySuppressed(cursor)) {
			success=cursor->executeQuery(query,querysize);
		} else {
			// FIXME: do we need to do anything to fake
			// like a query was actually executed
		}
	}

	// set flag indicating that the query has been executed
	// NOTE: We want to do this whether the query succeeds or fails so that
	// if its reexecuted, closeResultSet() and clearError() will be called
	// before the reexecution.
	cursor->setQueryHasBeenExecuted(true);

	// set the query end time
	dt.initFromSystemDateTime();
	cursor->setQueryEnd(dt.getSecond(),dt.getMicrosecond());

	// on failure, save the error
	// get it here rather than below because with some db's
	// after-triggers can mask the error
	if (!success) {
		saveError(cursor);
		debugWrite("failed: \"%.*s\"",
					cursor->getErrorSize(),
					cursor->getErrorBuffer());
	}

	// reset total rows fetched
	cursor->clearTotalRowsFetched();

	// update query and error counts
	incrementQueryCount(cursor->getQueryType());
	if (!success) {
		incrementTotalErrors();
	}

	// If the query was successful, and the column cache isn't empty,
	// and the query was a drop, then clear the column caches.  Do this
	// before running after-triggers as they may use the column caches.
	if (success && pvt->_colcache.getCount() &&
			cursor->getQueryType()==SQLRQUERYTYPE_DROP) {
		clearColumnCaches();
	}

	// handle after-triggers...
	// Run these whether the query suceeded or failed.  There are triggers
	// like replay and upsert that only run if specific failures occurred.
	if (enabletriggers && pvt->_sqlrtr) {

		// If the triggers succeeded, and there's no lingering error,
		// then we want "success" to be true.
		//
		// There might be a lingering error if the original query set
		// an error, and it wasn't cleared (on purpose) by any of the
		// triggers.
		//
		// If a trigger fails, it should return false, and optionally
		// set an error.  The case where a trigger sets an error but
		// still returns true should also be caught here, though I
		// can't think of a case where a well-behaved trigger should do 
		// his.
		success=pvt->_sqlrtr->runAfterTriggers(pvt->_conn,cursor) &&
						!getErrorNumber(cursor) &&
						!getErrorSize(cursor);
	}

	// was the query a commit or rollback?
	if (success) {
		commitOrRollback(cursor);
	}

	// special case intercepts...
	// rather than actually intercepting these, we allow the db to run
	// them and update state here.  This must run after commitOrRollback()
	// so that endTransaction()'s _needscommitorrollback=false isn't
	// immediately re-flipped to true by commitOrRollback() treating the
	// SET as not-select DML.
	if (success) {
		if (cursor->getQueryType()==
				SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_ON) {
			// FIXME: this is not correct for dbs which
			// defer autocommit (eg. mysql)
			pvt->_autocommit=true;
			if (pvt->_intransaction) {
				// the set autocommit on caused a commit,
				// make sure to run the end-transaction stuff
				endTransaction(true);
			}
		} else if (cursor->getQueryType()==
				SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_OFF) {
			// FIXME: this is not correct for dbs which
			// defer autocommit (eg. mysql)
			pvt->_autocommit=false;
			if (!pvt->_intransaction) {
				// the set autocommit off caused a begin,
				// make sure to run the begin-transaction stuff
				beginTransaction();
			}
		}
	}

	debugWrite((success)?"success":"failed");
	debugEnd();

	if (success) {

		// get affected rows
		if (!pvt->_overrideaffectedrows.getValue(cursor)) {
			pvt->_affectedrows.setValue(
				cursor,cursor->getAffectedRows());
		}

		cursor->setQueryStatus(SQLRQUERYSTATUS_SUCCESS);
	}

	// log query-executed
	raiseQueryExecutedEvent(cursor);

	return (success)?handleResultSetHeader(cursor):false;
}

void sqlrservercontroller::setNeedsCommitOrRollback(bool needed) {
	pvt->_needscommitorrollback=needed;
}

bool sqlrservercontroller::getNeedsCommitOrRollback() {
	return pvt->_needscommitorrollback;
}

void sqlrservercontroller::commitOrRollback(sqlrservercursor *cursor) {

	debugStart("commit or rollback check");

	// if the query was a commit or rollback, set a flag indicating so
	if (isTransactional()) {
		if (cursor->queryIsCommitOrRollback()) {
			pvt->_needscommitorrollback=false;
		} else if (cursor->queryIsNotSelect()) {
			pvt->_needscommitorrollback=true;
		}
	}

	debugWrite("commit/rollback %s",
		(pvt->_needscommitorrollback)?"needed":"not needed");

	debugEnd();
}

bool sqlrservercontroller::getInTransaction() {
	return pvt->_intransaction;
}

bool sqlrservercontroller::columnInfoIsValidAfterPrepare(
					sqlrservercursor *cursor) {
	return !cursor->getExecuteDirect() &&
		cursor->columnInfoIsValidAfterPrepare();
}

void sqlrservercontroller::setSendColumnInfo(bool sendcolumninfo) {
	pvt->_sendcolumninfo=sendcolumninfo;
}

bool sqlrservercontroller::getSendColumnInfo() {
	return pvt->_sendcolumninfo;
}

bool sqlrservercontroller::skipRows(sqlrservercursor *cursor,
						uint64_t rows, bool *error) {

	debugStart("skipping rows");
	debugWrite("rows: %lld",rows);

	for (uint64_t i=0; i<rows; i++) {

		debugWrite("skip...");

		bool	error;
		if (!skipRow(cursor,&error)) {
			if (error) {
				debugWrite("error");
			} else {
				debugWrite("end of result set");
			}
			debugEnd();
			return false;
		}

		cursor->incrementTotalRowsFetched();
	}

	debugEnd();
	return true;
}

void sqlrservercontroller::setDatabaseListFormat(
				sqlrserverlistformat_t listformat) {
	if (getDatabaseIsSchema()) {
		setSchemaListFormat(listformat);
	}
	setCatalogListFormat(listformat);
}

void sqlrservercontroller::setCatalogListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqlcatalogscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqlcatalogscolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbccatalogscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbccatalogscolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbccatalogscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbccatalogscolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setSchemaListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	// currently only iplemented for oracle
	// FIXME: ...and probably not correctly
	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqlschemascolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqlschemascolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbcschemascolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbcschemascolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbcschemascolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbcschemascolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setTableTypeListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	// currently only implemented for oracle
	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqltabletypescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqltabletypescolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbctabletypescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbctabletypescolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbctabletypescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbctabletypescolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setTableListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqltablescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqltablescolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbctablescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbctablescolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbctablescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbctablescolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setTypeInfoListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	// currently only implemented for oracle
	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqltypeinfocolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqltypeinfocolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbctypeinfocolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbctypeinfocolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbctypeinfocolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbctypeinfocolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setColumnListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqlcolumnscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqlcolumnscolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbccolumnscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbccolumnscolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbccolumnscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbccolumnscolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setPrimaryKeyListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqlprimarykeyscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqlprimarykeyscolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbcprimarykeyscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbcprimarykeyscolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbcprimarykeyscolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbcprimarykeyscolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setKeyAndIndexListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqlkeyandindexcolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqlkeyandindexcolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbckeyandindexcolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbckeyandindexcolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbckeyandindexcolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbckeyandindexcolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setProcedureListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	// currently only implemented for oracle
	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqlprocedurescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqlprocedurescolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbcprocedurescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbcprocedurescolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbcprocedurescolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbcprocedurescolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::setProcedureParameterListFormat(
					sqlrserverlistformat_t listformat) {

	// FIXME: for now, don't remap columns if api calls are used to get
	// lists, as the the columns won't have an extra NULL at the end that
	// unsupported columns can be mapped to.
	//
	// this "happens to work" for odbc passthrough:
	// ODBC -> sqlrelay client -> sqlrelay server -> ODBC -> some db
	// but wouldn't if either ODBC were replaced with something else
	if (pvt->_conn->getListsByApiCalls()) {
		pvt->_columnmap=NULL;
		pvt->_columnnamemap=NULL;
		return;
	}

	switch (listformat) {
		case SQLRSERVERLISTFORMAT_MYSQL:
			pvt->_columnmap=
				&(pvt->_mysqlprocedureparametercolumnmap);
			pvt->_columnnamemap=
				&(pvt->_mysqlprocedureparametercolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			pvt->_columnmap=
				&(pvt->_odbcprocedureparametercolumnmap);
			pvt->_columnnamemap=
				&(pvt->_odbcprocedureparametercolumnnamemap);
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			pvt->_columnmap=
				&(pvt->_jdbcprocedureparametercolumnmap);
			pvt->_columnnamemap=
				&(pvt->_jdbcprocedureparametercolumnnamemap);
			break;
		default:
			pvt->_columnmap=NULL;
			pvt->_columnnamemap=NULL;
			break;
	}
}

void sqlrservercontroller::buildColumnMaps() {
	buildToMySQLColumnMaps();
	buildToODBCColumnMaps();
	buildToJDBCColumnMaps();
}

void sqlrservercontroller::buildToMySQLColumnMaps() {

	// mysql getCatalogList
	// map from ODBC SQLTables() format to mysql "show databases" format
	// (all backends return ODBC SQLTables() format)
	//
	// Database <- TABLE_CAT
	pvt->_mysqlcatalogscolumnmap.setValue(0,0);
	pvt->_mysqlcatalogscolumnnamemap.setValue(0,"Database");

	// mysql getSchemaList
	// map from ODBC SQLTables() format to mysql "show schemas" format
	// (all backends return ODBC SQLTables() format)
	//
	// Database <- TABLE_SCHEM
	pvt->_mysqlschemascolumnmap.setValue(0,1);
	pvt->_mysqlschemascolumnnamemap.setValue(0,"Database");

	// mysql getTableTypeList
	// map from ODBC SQLTables() format to mysql
	// "select distinct table_type from information_schema.tables" format
	// (mysql doesn't have a built-in get table types query)
	// (all backends return ODBC SQLTables() format)
	//
	// table_type <- TABLE_TYPE_NAME
	pvt->_mysqltabletypescolumnmap.setValue(0,3);
	pvt->_mysqltabletypescolumnnamemap.setValue(0,"table_type");

	// mysql getTableList
	// map from ODBC SQLTables() format to mysql "show tables" format
	// (all backends return ODBC SQLTables() format)
	//
	// Tables_in_xxx <- TABLE_NAME
	pvt->_mysqltablescolumnmap.setValue(0,2);
	pvt->_mysqltablescolumnnamemap.setValue(0,"Tables_in_xxx");

	// mysql getTypeInfoList
	// mysql doesn't have anything like this, and all backends return
	// ODBC SQLGetTypeInfo() format, so just map 1 to 1 and lowercase the
	// column names for consistency with other mysql tables
	//
	// TYPE_NAME <- TYPE_NAME
	pvt->_mysqltypeinfocolumnmap.setValue(0,0);
	// DATA_TYPE <- DATA_TYPE
	pvt->_mysqltypeinfocolumnmap.setValue(1,1);
	// COLUMN_SIZE <- COLUMN_SIZE
	pvt->_mysqltypeinfocolumnmap.setValue(2,2);
	// LITERAL_PREFIX <- LITERAL_PREFIX
	pvt->_mysqltypeinfocolumnmap.setValue(3,3);
	// LITERAL_SUFFIX <- LITERAL_SUFFIX
	pvt->_mysqltypeinfocolumnmap.setValue(4,4);
	// CREATE_PARAMS <- CREATE_PARAMS
	pvt->_mysqltypeinfocolumnmap.setValue(5,5);
	// NULLABLE <- NULLABLE
	pvt->_mysqltypeinfocolumnmap.setValue(6,6);
	// CASE_SENSITIVE <- CASE_SENSITIVE
	pvt->_mysqltypeinfocolumnmap.setValue(7,7);
	// SEARCHABLE <- SEARCHABLE
	pvt->_mysqltypeinfocolumnmap.setValue(8,8);
	// UNSIGNED_ATTRIBUTE <- UNSIGNED_ATTRIBUTE
	pvt->_mysqltypeinfocolumnmap.setValue(9,9);
	// FIXED_PREC_SCALE <- FIXED_PREC_SCALE
	pvt->_mysqltypeinfocolumnmap.setValue(10,10);
	// AUTO_UNIQUE_VALUE <- AUTO_UNIQUE_VALUE
	pvt->_mysqltypeinfocolumnmap.setValue(11,11);
	// LOCAL_TYPE_NAME <- LOCAL_TYPE_NAME
	pvt->_mysqltypeinfocolumnmap.setValue(12,12);
	// MINIMUM_SCALE <- MINIMUM_SCALE
	pvt->_mysqltypeinfocolumnmap.setValue(13,13);
	// MAXIMUM_SCALE <- MAXIMUM_SCALE
	pvt->_mysqltypeinfocolumnmap.setValue(14,14);
	// SQL_DATA_TYPE <- SQL_DATA_TYPE
	pvt->_mysqltypeinfocolumnmap.setValue(15,15);
	// SQL_DATETIME_SUB <- SQL_DATETIME_SUB
	pvt->_mysqltypeinfocolumnmap.setValue(16,16);
	// NUM_PREC_RADIX <- NUM_PREC_RADIX
	pvt->_mysqltypeinfocolumnmap.setValue(17,17);
	// INTERVAL_PRECISION <- INTERVAL_PRECISION
	pvt->_mysqltypeinfocolumnmap.setValue(18,18);
	pvt->_mysqltypeinfocolumnnamemap.setValue(0,"type_name");
	pvt->_mysqltypeinfocolumnnamemap.setValue(1,"data_type");
	pvt->_mysqltypeinfocolumnnamemap.setValue(2,"precision");
	pvt->_mysqltypeinfocolumnnamemap.setValue(3,"literal_prefix");
	pvt->_mysqltypeinfocolumnnamemap.setValue(4,"literal_suffix");
	pvt->_mysqltypeinfocolumnnamemap.setValue(5,"create_params");
	pvt->_mysqltypeinfocolumnnamemap.setValue(6,"nullable");
	pvt->_mysqltypeinfocolumnnamemap.setValue(7,"case_sensitive");
	pvt->_mysqltypeinfocolumnnamemap.setValue(8,"searchable");
	pvt->_mysqltypeinfocolumnnamemap.setValue(9,"unsigned_attribute");
	pvt->_mysqltypeinfocolumnnamemap.setValue(10,"fixed_prec_scale");
	pvt->_mysqltypeinfocolumnnamemap.setValue(11,"auto_increment");
	pvt->_mysqltypeinfocolumnnamemap.setValue(12,"local_type_name");
	pvt->_mysqltypeinfocolumnnamemap.setValue(13,"minumum_scale");
	pvt->_mysqltypeinfocolumnnamemap.setValue(14,"maxiumm_scale");
	pvt->_mysqltypeinfocolumnnamemap.setValue(15,"sql_data_type");
	pvt->_mysqltypeinfocolumnnamemap.setValue(16,"sql_datetime_sub");
	pvt->_mysqltypeinfocolumnnamemap.setValue(17,"num_prec_radix");
	pvt->_mysqltypeinfocolumnnamemap.setValue(18,"interval_precision");

	// mysql getColumnList
	// map from ODBC SQLColumns()+ format to ??? format
	// FIXME: what format is this, definitely not mysql format
	// (all backends return ODBC SQLColumns()+ format)
	//
	// column_name <- column_name
	pvt->_mysqlcolumnscolumnmap.setValue(0,3);
	// data_type <- type_name
	pvt->_mysqlcolumnscolumnmap.setValue(1,5);
	// character_maximum_length <- column_size
	pvt->_mysqlcolumnscolumnmap.setValue(2,6);
	// numeric_precision <- numeric_precision
	pvt->_mysqlcolumnscolumnmap.setValue(3,18);
	// numeric_scale <- decimal_digits
	pvt->_mysqlcolumnscolumnmap.setValue(4,8);
	// is_nullable <- is_nullable
	pvt->_mysqlcolumnscolumnmap.setValue(5,17);
	// column_key <- column_key
	pvt->_mysqlcolumnscolumnmap.setValue(6,19);
	// column_default <- column_default
	pvt->_mysqlcolumnscolumnmap.setValue(7,12);
	// extra <- remarks
	pvt->_mysqlcolumnscolumnmap.setValue(8,11);
	pvt->_mysqlcolumnscolumnnamemap.setValue(0,"column_name");
	pvt->_mysqlcolumnscolumnnamemap.setValue(1,"data_type");
	pvt->_mysqlcolumnscolumnnamemap.setValue(2,"character_maximum_length");
	pvt->_mysqlcolumnscolumnnamemap.setValue(3,"numeric_precision");
	pvt->_mysqlcolumnscolumnnamemap.setValue(4,"numeric_scale");
	pvt->_mysqlcolumnscolumnnamemap.setValue(5,"is_nullable");
	pvt->_mysqlcolumnscolumnnamemap.setValue(6,"column_key");
	pvt->_mysqlcolumnscolumnnamemap.setValue(7,"column_default");
	pvt->_mysqlcolumnscolumnnamemap.setValue(8,"extra");

	// mysql getPrimaryKeysList
	// map from ODBC SQLPrimaryKeys() format to mysql "show index" format
	// (all backends return ODBC SQLPrimaryKeys() format)
	//
	// Table <- TABLE_NAME
	pvt->_mysqlprimarykeyscolumnmap.setValue(0,2);
	// Non_unique <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(1,6);
	// Key_name <- PK_NAME
	pvt->_mysqlprimarykeyscolumnmap.setValue(2,5);
	// Seq_in_index <- KEY_SEQ
	pvt->_mysqlprimarykeyscolumnmap.setValue(3,4);
	// Column_name <- COLUMN_NAME
	pvt->_mysqlprimarykeyscolumnmap.setValue(4,3);
	// Collation <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(5,6);
	// Cardinality <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(6,6);
	// Sub_part <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(7,6);
	// Packed <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(8,6);
	// Null <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(9,6);
	// Index_type <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(10,6);
	// Comment <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(11,6);
	// Index_comment <- null
	pvt->_mysqlprimarykeyscolumnmap.setValue(12,6);
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(0,"table");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(1,"non_unique");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(2,"key_name");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(3,"seq_in_index");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(4,"column_name");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(5,"collation");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(6,"cardinality");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(7,"sub_part");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(8,"packed");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(9,"null");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(10,"index_type");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(11,"comment");
	pvt->_mysqlprimarykeyscolumnnamemap.setValue(12,"index_comment");

	// mysql getKeyAndIndexList
	// map from ODBC SQLStatistics() format to mysql "show index" format
	// (all backends return ODBC SQLStatistics() format)
	//
	// table <- TABLE_NAME
	pvt->_mysqlkeyandindexcolumnmap.setValue(0,2);
	// non_unique <- NON_UNIQUE
	pvt->_mysqlkeyandindexcolumnmap.setValue(1,3);
	// key_name <- INDEX_NAME
	pvt->_mysqlkeyandindexcolumnmap.setValue(2,5);
	// seq_in_index <- ORDINAL_POSITION
	pvt->_mysqlkeyandindexcolumnmap.setValue(3,7);
	// column_name <- COLUMN_NAME
	pvt->_mysqlkeyandindexcolumnmap.setValue(4,8);
	// collation <- ASC_OR_DESC
	pvt->_mysqlkeyandindexcolumnmap.setValue(5,9);
	// cardinality <- CARDINALITY
	pvt->_mysqlkeyandindexcolumnmap.setValue(6,10);
	// sub_part <- null
	pvt->_mysqlkeyandindexcolumnmap.setValue(7,13);
	// packed <- null
	pvt->_mysqlkeyandindexcolumnmap.setValue(8,13);
	// null <- null
	pvt->_mysqlkeyandindexcolumnmap.setValue(9,13);
	// index_type <- TYPE
	pvt->_mysqlkeyandindexcolumnmap.setValue(10,6);
	// comment <- null
	pvt->_mysqlkeyandindexcolumnmap.setValue(11,13);
	// index_comment <- null
	pvt->_mysqlkeyandindexcolumnmap.setValue(12,13);
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(0,"table");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(1,"non_unique");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(2,"key_name");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(3,"seq_in_index");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(4,"column_name");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(5,"collation");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(6,"cardinality");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(7,"sub_part");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(8,"packed");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(9,"null");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(10,"index_type");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(11,"comment");
	pvt->_mysqlkeyandindexcolumnnamemap.setValue(12,"index_comment");

	// mysql getProcedureList
	// map from ODBC SQLProcedures() format to mysql
	// "select ... from information_schema.routines" format
	// (mysql doesn't have a built-in get table types query)
	// (all backends return ODBC SQLProcedures() format)
	//
	// routine_catalog <- PROCEDURE_CAT
	pvt->_mysqlprocedurescolumnmap.setValue(0,0);
	// routine_schema <- PROCEDURE_SCHEM
	pvt->_mysqlprocedurescolumnmap.setValue(1,1);
	// routine_name <- PROCEDURE_NAME
	pvt->_mysqlprocedurescolumnmap.setValue(2,2);
	// data_type <- PROCEDURE_TYPE
	pvt->_mysqlprocedurescolumnmap.setValue(3,7);
	pvt->_mysqlprocedurescolumnnamemap.setValue(0,"routine_catalog");
	pvt->_mysqlprocedurescolumnnamemap.setValue(1,"routine_schema");
	pvt->_mysqlprocedurescolumnnamemap.setValue(2,"routine_name");
	pvt->_mysqlprocedurescolumnnamemap.setValue(3,"data_type");

	// mysql getProcedureParameterList
	// map from ODBC SQLProcedureColumns() format to mysql
	// "select ... from information_schema.parameters" format
	// (all backends return ODBC SQLProcedureColumns() format)
	//
	// parameter_name <- COLUMN_NAME
	pvt->_mysqlprocedureparametercolumnmap.setValue(0,3);
	// parameter_mode <- COLUMN_TYPE
	pvt->_mysqlprocedureparametercolumnmap.setValue(1,4);
	// data_type <- TYPE_NAME
	pvt->_mysqlprocedureparametercolumnmap.setValue(2,6);
	// character_maximum_length <- COLUMN_SIZE
	pvt->_mysqlprocedureparametercolumnmap.setValue(3,7);
	// ordinal_position <- ORDINAL_POSITION
	pvt->_mysqlprocedureparametercolumnmap.setValue(4,17);
	pvt->_mysqlprocedureparametercolumnnamemap.setValue(
					0,"parameter_name");
	pvt->_mysqlprocedureparametercolumnnamemap.setValue(
					1,"parameter_mode");
	pvt->_mysqlprocedureparametercolumnnamemap.setValue(
					2,"data_type");
	pvt->_mysqlprocedureparametercolumnnamemap.setValue(
					3,"character_maximum_length");
	pvt->_mysqlprocedureparametercolumnnamemap.setValue(
					4,"ordinal_position");
}

void sqlrservercontroller::buildToODBCColumnMaps() {

	// ODBC getCatalogList
	// all backends return ODBC SQLTables() format, so just map 1 to 1
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_odbccatalogscolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_odbccatalogscolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_odbccatalogscolumnmap.setValue(2,2);
	// TABLE_TYPE <- TABLE_TYPE
	pvt->_odbccatalogscolumnmap.setValue(3,3);
	// REMARKS <- REMARKS
	pvt->_odbccatalogscolumnmap.setValue(4,4);
	pvt->_odbccatalogscolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_odbccatalogscolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_odbccatalogscolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_odbccatalogscolumnnamemap.setValue(3,"TABLE_TYPE");
	pvt->_odbccatalogscolumnnamemap.setValue(4,"REMARKS");

	// ODBC getSchemaList
	// all backends return ODBC SQLTables() format, so just map 1 to 1
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_odbcschemascolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_odbcschemascolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_odbcschemascolumnmap.setValue(2,2);
	// TABLE_TYPE <- TABLE_TYPE
	pvt->_odbcschemascolumnmap.setValue(3,3);
	// REMARKS <- REMARKS
	pvt->_odbcschemascolumnmap.setValue(4,4);
	pvt->_odbcschemascolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_odbcschemascolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_odbcschemascolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_odbcschemascolumnnamemap.setValue(3,"TABLE_TYPE");
	pvt->_odbcschemascolumnnamemap.setValue(4,"REMARKS");

	// ODBC getTableTypeList
	// all backends return ODBC SQLTables() format, so just map 1 to 1
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_odbctabletypescolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_odbctabletypescolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_odbctabletypescolumnmap.setValue(2,2);
	// TABLE_TYPE <- TABLE_TYPE
	pvt->_odbctabletypescolumnmap.setValue(3,3);
	// REMARKS <- REMARKS
	pvt->_odbctabletypescolumnmap.setValue(4,4);
	pvt->_odbctabletypescolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_odbctabletypescolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_odbctabletypescolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_odbctabletypescolumnnamemap.setValue(3,"TABLE_TYPE");
	pvt->_odbctabletypescolumnnamemap.setValue(4,"REMARKS");

	// ODBC getTableList
	// all backends return ODBC SQLTables() format, so just map 1 to 1
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_odbctablescolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_odbctablescolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_odbctablescolumnmap.setValue(2,2);
	// TABLE_TYPE <- TABLE_TYPE
	pvt->_odbctablescolumnmap.setValue(3,3);
	// REMARKS <- REMARKS
	pvt->_odbctablescolumnmap.setValue(4,4);
	pvt->_odbctablescolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_odbctablescolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_odbctablescolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_odbctablescolumnnamemap.setValue(3,"TABLE_TYPE");
	pvt->_odbctablescolumnnamemap.setValue(4,"REMARKS");

	// ODBC getTypeInfoList
	// all backends return ODBC SQLGetTypeInfo() format, so just map 1 to 1
	//
	// TYPE_NAME <- TYPE_NAME
	pvt->_odbctypeinfocolumnmap.setValue(0,0);
	// DATA_TYPE <- DATA_TYPE
	pvt->_odbctypeinfocolumnmap.setValue(1,1);
	// COLUMN_SIZE <- COLUMN_SIZE
	pvt->_odbctypeinfocolumnmap.setValue(2,2);
	// LITERAL_PREFIX <- LITERAL_PREFIX
	pvt->_odbctypeinfocolumnmap.setValue(3,3);
	// LITERAL_SUFFIX <- LITERAL_SUFFIX
	pvt->_odbctypeinfocolumnmap.setValue(4,4);
	// CREATE_PARAMS <- CREATE_PARAMS
	pvt->_odbctypeinfocolumnmap.setValue(5,5);
	// NULLABLE <- NULLABLE
	pvt->_odbctypeinfocolumnmap.setValue(6,6);
	// CASE_SENSITIVE <- CASE_SENSITIVE
	pvt->_odbctypeinfocolumnmap.setValue(7,7);
	// SEARCHABLE <- SEARCHABLE
	pvt->_odbctypeinfocolumnmap.setValue(8,8);
	// UNSIGNED_ATTRIBUTE <- UNSIGNED_ATTRIBUTE
	pvt->_odbctypeinfocolumnmap.setValue(9,9);
	// FIXED_PREC_SCALE <- FIXED_PREC_SCALE
	pvt->_odbctypeinfocolumnmap.setValue(10,10);
	// AUTO_UNIQUE_VALUE <- AUTO_UNIQUE_VALUE
	pvt->_odbctypeinfocolumnmap.setValue(11,11);
	// LOCAL_TYPE_NAME <- LOCAL_TYPE_NAME
	pvt->_odbctypeinfocolumnmap.setValue(12,12);
	// MINIMUM_SCALE <- MINIMUM_SCALE
	pvt->_odbctypeinfocolumnmap.setValue(13,13);
	// MAXIMUM_SCALE <- MAXIMUM_SCALE
	pvt->_odbctypeinfocolumnmap.setValue(14,14);
	// SQL_DATA_TYPE <- SQL_DATA_TYPE
	pvt->_odbctypeinfocolumnmap.setValue(15,15);
	// SQL_DATETIME_SUB <- SQL_DATETIME_SUB
	pvt->_odbctypeinfocolumnmap.setValue(16,16);
	// NUM_PREC_RADIX <- NUM_PREC_RADIX
	pvt->_odbctypeinfocolumnmap.setValue(17,17);
	// INTERVAL_PRECISION <- INTERVAL_PRECISION
	pvt->_odbctypeinfocolumnmap.setValue(18,18);
	pvt->_odbctypeinfocolumnnamemap.setValue(0,"TYPE_NAME");
	pvt->_odbctypeinfocolumnnamemap.setValue(1,"DATA_TYPE");
	pvt->_odbctypeinfocolumnnamemap.setValue(2,"PRECISION");
	pvt->_odbctypeinfocolumnnamemap.setValue(3,"LITERAL_PREFIX");
	pvt->_odbctypeinfocolumnnamemap.setValue(4,"LITERAL_SUFFIX");
	pvt->_odbctypeinfocolumnnamemap.setValue(5,"CREATE_PARAMS");
	pvt->_odbctypeinfocolumnnamemap.setValue(6,"NULLABLE");
	pvt->_odbctypeinfocolumnnamemap.setValue(7,"CASE_SENSITIVE");
	pvt->_odbctypeinfocolumnnamemap.setValue(8,"SEARCHABLE");
	pvt->_odbctypeinfocolumnnamemap.setValue(9,"UNSIGNED_ATTRIBUTE");
	pvt->_odbctypeinfocolumnnamemap.setValue(10,"FIXED_PREC_SCALE");
	pvt->_odbctypeinfocolumnnamemap.setValue(11,"AUTO_INCREMENT");
	pvt->_odbctypeinfocolumnnamemap.setValue(12,"LOCAL_TYPE_NAME");
	pvt->_odbctypeinfocolumnnamemap.setValue(13,"MINIMUM_SCALE");
	pvt->_odbctypeinfocolumnnamemap.setValue(14,"MAXIMUM_SCALE");
	pvt->_odbctypeinfocolumnnamemap.setValue(15,"SQL_DATA_TYPE");
	pvt->_odbctypeinfocolumnnamemap.setValue(16,"SQL_DATETIME_SUB");
	pvt->_odbctypeinfocolumnnamemap.setValue(17,"NUM_PREC_RADIX");
	pvt->_odbctypeinfocolumnnamemap.setValue(18,"INTERVAL_PRECISION");

	// ODBC getColumnList
	// map from ODBC SQLColumns()+ format to ODBC SQLColumns() format
	// (all backends return ODBC SQLColumns()+ format)
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_odbccolumnscolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_odbccolumnscolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_odbccolumnscolumnmap.setValue(2,2);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_odbccolumnscolumnmap.setValue(3,3);
	// DATA_TYPE <- DATA_TYPE (numeric)
	pvt->_odbccolumnscolumnmap.setValue(4,4);
	// TYPE_NAME <- TYPE_NAME
	pvt->_odbccolumnscolumnmap.setValue(5,5);
	// COLUMN_SIZE <- COLUMN_SIZE
	pvt->_odbccolumnscolumnmap.setValue(6,6);
	// BUFFER_LENGTH <- BUFFER_LENGTH
	pvt->_odbccolumnscolumnmap.setValue(7,7);
	// DECIMAL_DIGITS <- DECIMAL_DIGITS (smallint - scale)
	pvt->_odbccolumnscolumnmap.setValue(8,8);
	// NUM_PREC_RADIX <- NUM_PREC_RADIX (smallint - precision)
	pvt->_odbccolumnscolumnmap.setValue(9,9);
	// NULLABLE <- NULLABLE
	pvt->_odbccolumnscolumnmap.setValue(10,10);
	// REMARKS <- REMARKS
	pvt->_odbccolumnscolumnmap.setValue(11,11);
	// COLUMN_DEF <- COLUMN_DEF
	pvt->_odbccolumnscolumnmap.setValue(12,12);
	// SQL_DATA_TYPE <- SQL_DATA_TYPE
	pvt->_odbccolumnscolumnmap.setValue(13,13);
	// SQL_DATETIME_SUB <- SQL_DATETIME_SUB
	pvt->_odbccolumnscolumnmap.setValue(14,14);
	// CHAR_OCTET_LENGTH <- CHAR_OCTET_LENGTH
	pvt->_odbccolumnscolumnmap.setValue(15,15);
	// ORDINAL_POSITION <- ORDINAL_POSITION
	pvt->_odbccolumnscolumnmap.setValue(16,16);
	// IS_NULLABLE <- IS_NULLABLE
	pvt->_odbccolumnscolumnmap.setValue(17,17);
	pvt->_odbccolumnscolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_odbccolumnscolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_odbccolumnscolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_odbccolumnscolumnnamemap.setValue(3,"COLUMN_NAME");
	pvt->_odbccolumnscolumnnamemap.setValue(4,"DATA_TYPE");
	pvt->_odbccolumnscolumnnamemap.setValue(5,"TYPE_NAME");
	pvt->_odbccolumnscolumnnamemap.setValue(6,"COLUMN_SIZE");
	pvt->_odbccolumnscolumnnamemap.setValue(7,"BUFFER_LENGTH");
	pvt->_odbccolumnscolumnnamemap.setValue(8,"DECIMAL_DIGITS");
	pvt->_odbccolumnscolumnnamemap.setValue(9,"NUM_PREC_RADIX");
	pvt->_odbccolumnscolumnnamemap.setValue(10,"NULLABLE");
	pvt->_odbccolumnscolumnnamemap.setValue(11,"REMARKS");
	pvt->_odbccolumnscolumnnamemap.setValue(12,"COLUMN_DEF");
	pvt->_odbccolumnscolumnnamemap.setValue(13,"SQL_DATA_TYPE");
	pvt->_odbccolumnscolumnnamemap.setValue(14,"SQL_DATETIME_SUB");
	pvt->_odbccolumnscolumnnamemap.setValue(15,"CHAR_OCTET_LENGTH");
	pvt->_odbccolumnscolumnnamemap.setValue(16,"ORDINAL_POSITION");
	pvt->_odbccolumnscolumnnamemap.setValue(17,"IS_NULLABLE");

	// ODBC getPrimaryKeysList
	// all backends return ODBC SQLPrimaryKeys() format, so just map 1 to 1
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_odbcprimarykeyscolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_odbcprimarykeyscolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_odbcprimarykeyscolumnmap.setValue(2,2);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_odbcprimarykeyscolumnmap.setValue(3,3);
	// KEY_SEQ <- KEY_SEQ
	pvt->_odbcprimarykeyscolumnmap.setValue(4,4);
	// PK_NAME <- PK_NAME
	pvt->_odbcprimarykeyscolumnmap.setValue(5,5);
	pvt->_odbcprimarykeyscolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_odbcprimarykeyscolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_odbcprimarykeyscolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_odbcprimarykeyscolumnnamemap.setValue(3,"COLUMN_NAME");
	pvt->_odbcprimarykeyscolumnnamemap.setValue(4,"KEY_SEQ");
	pvt->_odbcprimarykeyscolumnnamemap.setValue(5,"PK_NAME");

	// ODBC getKeyAndIndexList
	// all backends return ODBC SQLStatistics() format, so just map 1 to 1
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_odbckeyandindexcolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_odbckeyandindexcolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_odbckeyandindexcolumnmap.setValue(2,2);
	// NON_UNIQUE <- NON_UNIQUE
	pvt->_odbckeyandindexcolumnmap.setValue(3,3);
	// INDEX_QUALIFIER <- INDEX_QUALIFIER
	pvt->_odbckeyandindexcolumnmap.setValue(4,4);
	// INDEX_NAME <- INDEX_NAME
	pvt->_odbckeyandindexcolumnmap.setValue(5,5);
	// TYPE <- TYPE
	pvt->_odbckeyandindexcolumnmap.setValue(6,6);
	// ORDINAL_POSITION <- ORDINAL_POSITION
	pvt->_odbckeyandindexcolumnmap.setValue(7,7);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_odbckeyandindexcolumnmap.setValue(8,8);
	// ASC_OR_DESC <- ASC_OR_DESC
	pvt->_odbckeyandindexcolumnmap.setValue(9,9);
	// CARDINALITY <- CARDINALITY
	pvt->_odbckeyandindexcolumnmap.setValue(10,10);
	// PAGES <- PAGES
	pvt->_odbckeyandindexcolumnmap.setValue(11,11);
	// FILTER_CONDITION <- FILTER_CONDITION
	pvt->_odbckeyandindexcolumnmap.setValue(12,12);
	pvt->_odbckeyandindexcolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_odbckeyandindexcolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_odbckeyandindexcolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_odbckeyandindexcolumnnamemap.setValue(3,"NON_UNIQUE");
	pvt->_odbckeyandindexcolumnnamemap.setValue(4,"INDEX_QUALIFIER");
	pvt->_odbckeyandindexcolumnnamemap.setValue(5,"INDEX_NAME");
	pvt->_odbckeyandindexcolumnnamemap.setValue(6,"TYPE");
	pvt->_odbckeyandindexcolumnnamemap.setValue(7,"ORDINAL_POSITION");
	pvt->_odbckeyandindexcolumnnamemap.setValue(8,"COLUMN_NAME");
	pvt->_odbckeyandindexcolumnnamemap.setValue(9,"ASC_OR_DESC");
	pvt->_odbckeyandindexcolumnnamemap.setValue(10,"CARDINALITY");
	pvt->_odbckeyandindexcolumnnamemap.setValue(11,"PAGES");
	pvt->_odbckeyandindexcolumnnamemap.setValue(12,"FILTER_CONDITION");

	// ODBC getProcedureList
	// all backends return ODBC SQLProcedurs() format, so just map 1 to 1
	//
	// PROCEDURE_CAT <- PROCEDURE_CAT
	pvt->_odbcprocedurescolumnmap.setValue(0,0);
	// PROCEDURE_SCHEM <- PROCEDURE_SCHEM
	pvt->_odbcprocedurescolumnmap.setValue(1,1);
	// PROCEDURE_NAME <- PROCEDURE_NAME
	pvt->_odbcprocedurescolumnmap.setValue(2,2);
	// NUM_INPUT_PARAMS <- NUM_INPUT_PARAMS
	pvt->_odbcprocedurescolumnmap.setValue(3,3);
	// NUM_OUTPUT_PARAMS <- NUM_OUTPUT_PARAMS
	pvt->_odbcprocedurescolumnmap.setValue(4,4);
	// NUM_RESULT_SETS <- NUM_RESULT_SETS
	pvt->_odbcprocedurescolumnmap.setValue(5,5);
	// REMARKS <- REMARKS
	pvt->_odbcprocedurescolumnmap.setValue(6,6);
	// PROCEDURE_TYPE <- PROCEDURE_TYPE
	pvt->_odbcprocedurescolumnmap.setValue(7,7);
	pvt->_odbcprocedurescolumnnamemap.setValue(0,"PROCEDURE_CAT");
	pvt->_odbcprocedurescolumnnamemap.setValue(1,"PROCEDURE_SCHEM");
	pvt->_odbcprocedurescolumnnamemap.setValue(2,"PROCEDURE_NAME");
	pvt->_odbcprocedurescolumnnamemap.setValue(3,"NUM_INPUT_PARAMS");
	pvt->_odbcprocedurescolumnnamemap.setValue(4,"NUM_OUTPUT_PARAMS");
	pvt->_odbcprocedurescolumnnamemap.setValue(5,"NUM_RESULT_SETS");
	pvt->_odbcprocedurescolumnnamemap.setValue(6,"REMARKS");
	pvt->_odbcprocedurescolumnnamemap.setValue(7,"PROCEDURE_TYPE");

	// ODBC getProcedureParameterList
	// all backends return ODBC SQLProcedureColumns() format,
	// so just map 1 to 1
	//
	// PROCEDURE_CAT <- PROCEDURE_CAT
	pvt->_odbcprocedureparametercolumnmap.setValue(0,0);
	// PROCEDURE_SCHEM <- PROCEDURE_SCHEM
	pvt->_odbcprocedureparametercolumnmap.setValue(1,1);
	// PROCEDURE_NAME <- PROCEDURE_NAME
	pvt->_odbcprocedureparametercolumnmap.setValue(2,2);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_odbcprocedureparametercolumnmap.setValue(3,3);
	// COLUMN_TYPE <- COLUMN_TYPE
	pvt->_odbcprocedureparametercolumnmap.setValue(4,4);
	// DATA_TYPE <- DATA_TYPE
	pvt->_odbcprocedureparametercolumnmap.setValue(5,5);
	// TYPE_NAME <- TYPE_NAME
	pvt->_odbcprocedureparametercolumnmap.setValue(6,6);
	// COLUMN_SIZE <- COLUMN_SIZE
	pvt->_odbcprocedureparametercolumnmap.setValue(7,7);
	// BUFFER_LENGTH <- BUFFER_LENGTH
	pvt->_odbcprocedureparametercolumnmap.setValue(8,8);
	// DECIMAL_DIGITS <- DECIMAL_DIGITS
	pvt->_odbcprocedureparametercolumnmap.setValue(9,9);
	// NUM_PREC_RADIX <- NUM_PREC_RADIX
	pvt->_odbcprocedureparametercolumnmap.setValue(10,10);
	// NULLABLE <- NULLABLE
	pvt->_odbcprocedureparametercolumnmap.setValue(11,11);
	// REMARKS <- REMARKS
	pvt->_odbcprocedureparametercolumnmap.setValue(12,12);
	// COLUMN_DEF <- COLUMN_DEF
	pvt->_odbcprocedureparametercolumnmap.setValue(13,13);
	// SQL_DATA_TYPE <- SQL_DATA_TYPE
	pvt->_odbcprocedureparametercolumnmap.setValue(14,14);
	// SQL_DATETIME_SUB <- SQL_DATETIME_SUB
	pvt->_odbcprocedureparametercolumnmap.setValue(15,15);
	// CHAR_OCTET_LENGTH <- CHAR_OCTET_LENGTH
	pvt->_odbcprocedureparametercolumnmap.setValue(16,16);
	// ORDINAL_POSITION <- ORDINAL_POSITION
	pvt->_odbcprocedureparametercolumnmap.setValue(17,17);
	// IS_NULLABLE <- IS_NULLABLE
	pvt->_odbcprocedureparametercolumnmap.setValue(18,18);
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					0,"PROCEDURE_CAT");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					1,"PROCEDURE_SCHEM");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					2,"PROCEDURE_NAME");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					3,"COLUMN_NAME");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					4,"COLUMN_TYPE");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					5,"DATA_TYPE");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					6,"TYPE_NAME");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					7,"COLUMN_SIZE");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					8,"BUFFER_LENGTH");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					9,"DECIMAL_DIGITS");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					10,"NUM_PREC_RADIX");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					11,"NULLABLE");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					12,"REMARKS");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					13,"COLUMN_DEF");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					14,"SQL_DATA_TYPE");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					15,"SQL_DATETIME_SUB");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					16,"CHAR_OCTET_LENGTH");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					17,"ORDINAL_POSITION");
	pvt->_odbcprocedureparametercolumnnamemap.setValue(
					18,"IS_NULLABLE");
}

void sqlrservercontroller::buildToJDBCColumnMaps() {

	// JDBC getCatalogList
	// map from ODBC SQLTables() format to JDBC getCatalogs() format
	// (all backends return ODBC SQLTables() format)
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_jdbccatalogscolumnmap.setValue(0,0);
	pvt->_jdbccatalogscolumnnamemap.setValue(0,"TABLE_CAT");

	// JDBC getSchemaList
	// map from ODBC SQLTables() format to JDBC getSchemas() format
	// (all backends return ODBC SQLTables() format)
	//
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_jdbcschemascolumnmap.setValue(0,1);
	// TABLE_CATALOG <- TABLE_CAT
	pvt->_jdbcschemascolumnmap.setValue(1,0);
	pvt->_jdbcschemascolumnnamemap.setValue(0,"TABLE_SCHEM");
	pvt->_jdbcschemascolumnnamemap.setValue(1,"TABLE_CATALOG");

	// JDBC getTableTypeList
	// map from ODBC SQLTables() format to JDBC getTableTypes() format
	// (all backends return ODBC SQLTables() format)
	//
	// TABLE_TYPE <- TABLE_TYPE
	pvt->_jdbctabletypescolumnmap.setValue(0,3);
	pvt->_jdbctabletypescolumnnamemap.setValue(0,"TABLE_TYPE");

	// JDBC getTableList
	// map from ODBC SQLTables() format to JDBC getTables() format
	// (all backends return ODBC SQLTables() format)
	//
	// TABLE_CAT
	pvt->_jdbctablescolumnmap.setValue(0,0);
	// TABLE_SCHEM
	pvt->_jdbctablescolumnmap.setValue(1,1);
	// TABLE_NAME
	pvt->_jdbctablescolumnmap.setValue(2,2);
	// TABLE_TYPE
	pvt->_jdbctablescolumnmap.setValue(3,3);
	// REMARKS
	pvt->_jdbctablescolumnmap.setValue(4,4);
	// TYPE_CAT <- NULL
	pvt->_jdbctablescolumnmap.setValue(5,5);
	// TYPE_SCHEM <- NULL
	pvt->_jdbctablescolumnmap.setValue(6,5);
	// TYPE_NAME <- NULL
	pvt->_jdbctablescolumnmap.setValue(7,5);
	// SELF_REFERENCING_COL_NAME <- NULL
	pvt->_jdbctablescolumnmap.setValue(8,5);
	// REF_GENERATION <- NULL
	pvt->_jdbctablescolumnmap.setValue(9,5);
	pvt->_jdbctablescolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_jdbctablescolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_jdbctablescolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_jdbctablescolumnnamemap.setValue(3,"TABLE_TYPE");
	pvt->_jdbctablescolumnnamemap.setValue(4,"REMARKS");
	pvt->_jdbctablescolumnnamemap.setValue(5,"TYPE_CAT");
	pvt->_jdbctablescolumnnamemap.setValue(6,"TYPE_SCHEM");
	pvt->_jdbctablescolumnnamemap.setValue(7,"TYPE_NAME");
	pvt->_jdbctablescolumnnamemap.setValue(8,"SELF_REFERENCING_COL_NAME");
	pvt->_jdbctablescolumnnamemap.setValue(9,"REF_GENERATION");

	// JDBC getTypeInfoList
	// map from ODBC SQLGetTypeInfo() format to JDBC getTypeInf() format
	// (all backends return ODBC SQLGetTypeInfo() format)
	//
	// TYPE_NAME <- TYPE_NAME
	pvt->_jdbctypeinfocolumnmap.setValue(0,0);
	// DATA_TYPE <- DATA_TYPE
	pvt->_jdbctypeinfocolumnmap.setValue(1,1);
	// PRECISION <- COLUMN_SIZE
	pvt->_jdbctypeinfocolumnmap.setValue(2,2);
	// LITERAL_PREFIX <- LITERAL_PREFIX
	pvt->_jdbctypeinfocolumnmap.setValue(3,3);
	// LITERAL_SUFFIX <- LITERAL_SUFFIX
	pvt->_jdbctypeinfocolumnmap.setValue(4,4);
	// CREATE_PARAMS <- CREATE_PARAMS
	pvt->_jdbctypeinfocolumnmap.setValue(5,5);
	// NULLABLE <- NULLABLE
	pvt->_jdbctypeinfocolumnmap.setValue(6,6);
	// CASE_SENSITIVE <- CASE_SENSITIVE
	pvt->_jdbctypeinfocolumnmap.setValue(7,7);
	// SEARCHABLE <- SEARCHABLE
	pvt->_jdbctypeinfocolumnmap.setValue(8,8);
	// UNSIGNED_ATTRIBUTE <- UNSIGNED_ATTRIBUTE
	pvt->_jdbctypeinfocolumnmap.setValue(9,9);
	// FIXED_PREC_SCALE <- FIXED_PREC_SCALE
	pvt->_jdbctypeinfocolumnmap.setValue(10,10);
	// AUTO_INCREMENT <- AUTO_UNIQUE_VALUE
	pvt->_jdbctypeinfocolumnmap.setValue(11,11);
	// LOCAL_TYPE_NAME <- LOCAL_TYPE_NAME
	pvt->_jdbctypeinfocolumnmap.setValue(12,12);
	// MINIMUM_SCALE <- MINIMUM_SCALE
	pvt->_jdbctypeinfocolumnmap.setValue(13,13);
	// MAXIMUM_SCALE <- MAXIMUM_SCALE
	pvt->_jdbctypeinfocolumnmap.setValue(14,14);
	// SQL_DATA_TYPE <- SQL_DATA_TYPE
	pvt->_jdbctypeinfocolumnmap.setValue(15,15);
	// SQL_DATETIME_SUB <- SQL_DATETIME_SUB
	pvt->_jdbctypeinfocolumnmap.setValue(16,16);
	// NUM_PREC_RADIX <- NUM_PREC_RADIX
	pvt->_jdbctypeinfocolumnmap.setValue(17,17);
	pvt->_jdbctypeinfocolumnnamemap.setValue(0,"TYPE_NAME");
	pvt->_jdbctypeinfocolumnnamemap.setValue(1,"DATA_TYPE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(2,"PRECISION");
	pvt->_jdbctypeinfocolumnnamemap.setValue(3,"LITERAL_PREFIX");
	pvt->_jdbctypeinfocolumnnamemap.setValue(4,"LITERAL_SUFFIX");
	pvt->_jdbctypeinfocolumnnamemap.setValue(5,"CREATE_PARAMS");
	pvt->_jdbctypeinfocolumnnamemap.setValue(6,"NULLABLE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(7,"CASE_SENSITIVE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(8,"SEARCHABLE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(9,"UNSIGNED_ATTRIBUTE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(10,"FIXED_PREC_SCALE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(11,"AUTO_INCREMENT");
	pvt->_jdbctypeinfocolumnnamemap.setValue(12,"LOCAL_TYPE_NAME");
	pvt->_jdbctypeinfocolumnnamemap.setValue(13,"MINIMUM_SCALE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(14,"MAXIMUM_SCALE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(15,"SQL_DATA_TYPE");
	pvt->_jdbctypeinfocolumnnamemap.setValue(16,"SQL_DATETIME_SUB");
	pvt->_jdbctypeinfocolumnnamemap.setValue(17,"NUM_PREC_RADIX");

	// JDBC getColumnList
	// map from ODBC SQLColumns()+ format to JDBC getColumns() format
	// (all backends return ODBC SQLColumns()+ format)
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_jdbccolumnscolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_jdbccolumnscolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_jdbccolumnscolumnmap.setValue(2,2);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_jdbccolumnscolumnmap.setValue(3,3);
	// DATA_TYPE <- DATA_TYPE (numeric)
	pvt->_jdbccolumnscolumnmap.setValue(4,4);
	// TYPE_NAME <- TYPE_NAME
	pvt->_jdbccolumnscolumnmap.setValue(5,5);
	// COLUMN_SIZE <- COLUMN_SIZE
	pvt->_jdbccolumnscolumnmap.setValue(6,6);
	// BUFFER_LENGTH <- BUFFER_LENGTH
	pvt->_jdbccolumnscolumnmap.setValue(7,7);
	// DECIMAL_DIGITS <- DECIMAL_DIGITS (smallint - scale)
	pvt->_jdbccolumnscolumnmap.setValue(8,8);
	// NUM_PREC_RADIX <- NUM_PREC_RADIX (smallint - precision)-
	pvt->_jdbccolumnscolumnmap.setValue(9,9);
	// NULLABLE <- NULLABLE
	pvt->_jdbccolumnscolumnmap.setValue(10,10);
	// REMARKS <- REMARKS
	pvt->_jdbccolumnscolumnmap.setValue(11,11);
	// COLUMN_DEF <- COLUMN_DEF
	pvt->_jdbccolumnscolumnmap.setValue(12,12);
	// SQL_DATA_TYPE <- SQL_DATA_TYPE
	pvt->_jdbccolumnscolumnmap.setValue(13,13);
	// SQL_DATETIME_SUB <- SQL_DATETIME_SUB
	pvt->_jdbccolumnscolumnmap.setValue(14,14);
	// CHAR_OCTET_LENGTH <- CHAR_OCTET_LENGTH
	pvt->_jdbccolumnscolumnmap.setValue(15,15);
	// ORDINAL_POSITION <- ORDINAL_POSITION
	pvt->_jdbccolumnscolumnmap.setValue(16,16);
	// IS_NULLABLE <- IS_NULLABLE
	pvt->_jdbccolumnscolumnmap.setValue(17,17);
	// SCOPE_CATALOG <- NULL
	pvt->_jdbccolumnscolumnmap.setValue(18,20);
	// SCOPE_SCHEMA <- NULL
	pvt->_jdbccolumnscolumnmap.setValue(19,20);
	// SCOPE_TABLE <- NULL
	pvt->_jdbccolumnscolumnmap.setValue(20,20);
	// SOURCE_DATA_TYPE <- NULL
	pvt->_jdbccolumnscolumnmap.setValue(21,20);
	// IS_AUTOINCREMENT <- NULL
	// (FIXME: we do have this info, just not as a boolean field,
	// currently, it's part of the remarks)
	pvt->_jdbccolumnscolumnmap.setValue(22,20);
	// IS_GENERATEDCOLUMN <- NULL
	pvt->_jdbccolumnscolumnmap.setValue(23,20);
	pvt->_jdbccolumnscolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_jdbccolumnscolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_jdbccolumnscolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_jdbccolumnscolumnnamemap.setValue(3,"COLUMN_NAME");
	pvt->_jdbccolumnscolumnnamemap.setValue(4,"DATA_TYPE");
	pvt->_jdbccolumnscolumnnamemap.setValue(5,"TYPE_NAME");
	pvt->_jdbccolumnscolumnnamemap.setValue(6,"COLUMN_SIZE");
	pvt->_jdbccolumnscolumnnamemap.setValue(7,"BUFFER_LENGTH");
	pvt->_jdbccolumnscolumnnamemap.setValue(8,"DECIMAL_DIGITS");
	pvt->_jdbccolumnscolumnnamemap.setValue(9,"NUM_PREC_RADIX");
	pvt->_jdbccolumnscolumnnamemap.setValue(10,"NULLABLE");
	pvt->_jdbccolumnscolumnnamemap.setValue(11,"REMARKS");
	pvt->_jdbccolumnscolumnnamemap.setValue(12,"COLUMN_DEF");
	pvt->_jdbccolumnscolumnnamemap.setValue(13,"SQL_DATA_TYPE");
	pvt->_jdbccolumnscolumnnamemap.setValue(14,"SQL_DATETIME_SUB");
	pvt->_jdbccolumnscolumnnamemap.setValue(15,"CHAR_OCTET_LENGTH");
	pvt->_jdbccolumnscolumnnamemap.setValue(16,"ORDINAL_POSITION");
	pvt->_jdbccolumnscolumnnamemap.setValue(17,"IS_NULLABLE");
	pvt->_jdbccolumnscolumnnamemap.setValue(18,"SCOPE_CATALOG");
	pvt->_jdbccolumnscolumnnamemap.setValue(19,"SCOPE_SCHEMA");
	pvt->_jdbccolumnscolumnnamemap.setValue(20,"SCOPE_TABLE");
	pvt->_jdbccolumnscolumnnamemap.setValue(21,"SOURCE_DATA_TYPE");
	pvt->_jdbccolumnscolumnnamemap.setValue(22,"IS_AUTOINCREMENT");
	pvt->_jdbccolumnscolumnnamemap.setValue(23,"IS_GENERATEDCOLUMN");

	// JDBC getPrimaryKeysList
	// map from ODBC SQLPrimaryKeys() format to JDBC getPrimaryKeys() format
	// (all backends return ODBC SQLPrimaryKeys() format)
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_jdbcprimarykeyscolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_jdbcprimarykeyscolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_jdbcprimarykeyscolumnmap.setValue(2,2);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_jdbcprimarykeyscolumnmap.setValue(3,3);
	// KEY_SEQ <- KEY_SEQ
	pvt->_jdbcprimarykeyscolumnmap.setValue(4,4);
	// PK_NAME <- PK_NAME
	pvt->_jdbcprimarykeyscolumnmap.setValue(5,5);
	pvt->_jdbcprimarykeyscolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_jdbcprimarykeyscolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_jdbcprimarykeyscolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_jdbcprimarykeyscolumnnamemap.setValue(3,"COLUMN_NAME");
	pvt->_jdbcprimarykeyscolumnnamemap.setValue(4,"KEY_SEQ");
	pvt->_jdbcprimarykeyscolumnnamemap.setValue(5,"PK_NAME");

	// JDBC getKeyAndIndexList
	// map from ODBC SQLStatistics() format to JDBC getIndexInfo() format
	// (all backends return ODBC SQLStatistics() format)
	//
	// TABLE_CAT <- TABLE_CAT
	pvt->_jdbckeyandindexcolumnmap.setValue(0,0);
	// TABLE_SCHEM <- TABLE_SCHEM
	pvt->_jdbckeyandindexcolumnmap.setValue(1,1);
	// TABLE_NAME <- TABLE_NAME
	pvt->_jdbckeyandindexcolumnmap.setValue(2,2);
	// NON_UNIQUE <- NON_UNIQUE
	pvt->_jdbckeyandindexcolumnmap.setValue(3,3);
	// INDEX_QUALIFIER <- INDEX_QUALIFIER
	pvt->_jdbckeyandindexcolumnmap.setValue(4,4);
	// INDEX_NAME <- INDEX_NAME
	pvt->_jdbckeyandindexcolumnmap.setValue(5,5);
	// TYPE <- TYPE
	pvt->_jdbckeyandindexcolumnmap.setValue(6,6);
	// ORDINAL_POSITION <- ORDINAL_POSITION
	pvt->_jdbckeyandindexcolumnmap.setValue(7,7);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_jdbckeyandindexcolumnmap.setValue(8,8);
	// ASC_OR_DESC <- ASC_OR_DESC
	pvt->_jdbckeyandindexcolumnmap.setValue(9,9);
	// CARDINALITY <- CARDINALITY
	pvt->_jdbckeyandindexcolumnmap.setValue(10,10);
	// PAGES <- PAGES
	pvt->_jdbckeyandindexcolumnmap.setValue(11,11);
	// FILTER_CONDITION <- FILTER_CONDITION
	pvt->_jdbckeyandindexcolumnmap.setValue(12,12);
	pvt->_jdbckeyandindexcolumnnamemap.setValue(0,"TABLE_CAT");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(1,"TABLE_SCHEM");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(2,"TABLE_NAME");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(3,"NON_UNIQUE");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(4,"INDEX_QUALIFIER");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(5,"INDEX_NAME");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(6,"TYPE");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(7,"ORDINAL_POSITION");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(8,"COLUMN_NAME");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(9,"ASC_OR_DESC");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(10,"CARDINALITY");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(11,"PAGES");
	pvt->_jdbckeyandindexcolumnnamemap.setValue(12,"FILTER_CONDITION");

	// JDBC getProcedureList
	// map from ODBC SQLProcedures() format to JDBC getProcedures() format
	// (all backends return ODBC SQLProcedures() format)
	//
	// PROCEDURE_CAT <- PROCEDURE_CAT
	pvt->_jdbcprocedurescolumnmap.setValue(0,0);
	// PROCEDURE_SCHEM <- PROCEDURE_SCHEM
	pvt->_jdbcprocedurescolumnmap.setValue(1,1);
	// PROCEDURE_NAME <- PROCEDURE_NAME
	pvt->_jdbcprocedurescolumnmap.setValue(2,2);
	// NUM_INPUT_PARAMS <- NUM_INPUT_PARAMS
	pvt->_jdbcprocedurescolumnmap.setValue(3,3);
	// NUM_OUTPUT_PARAMS <- NUM_OUTPUT_PARAMS
	pvt->_jdbcprocedurescolumnmap.setValue(4,4);
	// NUM_RESULT_SETS <- NUM_RESULT_SETS
	pvt->_jdbcprocedurescolumnmap.setValue(5,5);
	// REMARKS <- REMARKS
	pvt->_jdbcprocedurescolumnmap.setValue(6,6);
	// PROCEDURE_TYPE <- PROCEDURE_TYPE
	pvt->_jdbcprocedurescolumnmap.setValue(7,7);
	pvt->_jdbcprocedurescolumnnamemap.setValue(0,"PROCEDURE_CAT");
	pvt->_jdbcprocedurescolumnnamemap.setValue(1,"PROCEDURE_SCHEM");
	pvt->_jdbcprocedurescolumnnamemap.setValue(2,"PROCEDURE_NAME");
	pvt->_jdbcprocedurescolumnnamemap.setValue(3,"NUM_INPUT_PARAMS");
	pvt->_jdbcprocedurescolumnnamemap.setValue(4,"NUM_OUTPUT_PARAMS");
	pvt->_jdbcprocedurescolumnnamemap.setValue(5,"NUM_RESULT_SETS");
	pvt->_jdbcprocedurescolumnnamemap.setValue(6,"REMARKS");
	pvt->_jdbcprocedurescolumnnamemap.setValue(7,"PROCEDURE_TYPE");

	// JDBC getProcedureParameterList
	// map from ODBC SQLProcedureColumns() format to
	// JDBC getProcedureColumns() format
	// (all backends return ODBC SQLProcedureColumns() format)
	//
	// PROCEDURE_CAT <- PROCEDURE_CAT
	pvt->_jdbcprocedureparametercolumnmap.setValue(0,0);
	// PROCEDURE_SCHEM <- PROCEDURE_SCHEM
	pvt->_jdbcprocedureparametercolumnmap.setValue(1,1);
	// PROCEDURE_NAME <- PROCEDURE_NAME
	pvt->_jdbcprocedureparametercolumnmap.setValue(2,2);
	// COLUMN_NAME <- COLUMN_NAME
	pvt->_jdbcprocedureparametercolumnmap.setValue(3,3);
	// COLUMN_TYPE <- COLUMN_TYPE
	pvt->_jdbcprocedureparametercolumnmap.setValue(4,4);
	// DATA_TYPE <- DATA_TYPE
	pvt->_jdbcprocedureparametercolumnmap.setValue(5,5);
	// TYPE_NAME <- TYPE_NAME
	pvt->_jdbcprocedureparametercolumnmap.setValue(6,6);
	// PRECISION <- COLUMN_SIZE
	pvt->_jdbcprocedureparametercolumnmap.setValue(7,7);
	// LENGTH <- BUFFER_LENGTH
	pvt->_jdbcprocedureparametercolumnmap.setValue(8,8);
	// SCALE <- DECIMAL_DIGITS
	pvt->_jdbcprocedureparametercolumnmap.setValue(9,9);
	// RADIX <- NUM_PREC_RADIX
	pvt->_jdbcprocedureparametercolumnmap.setValue(10,10);
	// NULLABLE <- NULLABLE
	pvt->_jdbcprocedureparametercolumnmap.setValue(11,11);
	// REMARKS <- REMARKS
	pvt->_jdbcprocedureparametercolumnmap.setValue(12,12);
	// COLUMN_DEF <- COLUMN_DEF
	pvt->_jdbcprocedureparametercolumnmap.setValue(13,13);
	// SQL_DATA_TYPE <- SQL_DATA_TYPE
	pvt->_jdbcprocedureparametercolumnmap.setValue(14,14);
	// SQL_DATETIME_SUB <- SQL_DATETIME_SUB
	pvt->_jdbcprocedureparametercolumnmap.setValue(15,15);
	// CHAR_OCTET_LENGTH <- CHAR_OCTET_LENGTH
	pvt->_jdbcprocedureparametercolumnmap.setValue(16,16);
	// ORDINAL_POSITION <- ORDINAL_POSITION
	pvt->_jdbcprocedureparametercolumnmap.setValue(17,17);
	// IS_NULLABLE <- IS_NULLABLE
	pvt->_jdbcprocedureparametercolumnmap.setValue(18,18);
	// SPECIFIC_NAME <- null
	pvt->_jdbcprocedureparametercolumnmap.setValue(19,19);
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					0,"PROCEDURE_CAT");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					1,"PROCEDURE_SCHEM");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					2,"PROCEDURE_NAME");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					3,"COLUMN_NAME");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					4,"COLUMN_TYPE");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					5,"DATA_TYPE");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					6,"TYPE_NAME");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					7,"PRECISION");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					8,"LENGTH");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					9,"SCALE");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					10,"RADIX");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					11,"NULLABLE");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					12,"REMARKS");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					13,"COLUMN_DEF");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					14,"SQL_DATA_TYPE");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					15,"SQL_DATETIME_SUB");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					16,"CHAR_OCTET_LENGTH");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					17,"ORDINAL_POSITION");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					18,"IS_NULLABLE");
	pvt->_jdbcprocedureparametercolumnnamemap.setValue(
					19,"SPECIFIC_NAME");
}

void sqlrservercontroller::setColumnMap() {
	// FIXME: set the column map, depending on what kind of query was run,
	// and what column format we're set to use for that query type
}

uint32_t sqlrservercontroller::mapColumn(uint32_t col) {
	return (pvt->_columnmap)?pvt->_columnmap->getValue(col):col;
}

uint32_t sqlrservercontroller::mapColumnCount(uint32_t colcount) {
	return (pvt->_columnmap)?pvt->_columnmap->getCount():colcount;
}

bool sqlrservercontroller::reformatField(sqlrservercursor *cursor,
						const char *name,
						uint32_t index,
						const char **field,
						uint64_t *fieldsize) {
	// run translations
	if (pvt->_sqlrrst) {

		if (pvt->_debugsqlrresultsettranslation) {
			stdoutput.printf("\n===================="
				 	"===================="
				 	"===================="
				 	"===================\n\n");
			stdoutput.printf("translating result set "
					"field %d (%s)...\n",index,name);
			stdoutput.printf("original:\n%s\n",*field);
		}

		if (!pvt->_sqlrrst->run(pvt->_conn,cursor,
					name,index,field,fieldsize)) {
			setError(cursor,pvt->_sqlrrst->getError(),
				SQLR_ERROR_RESULTSETTRANSLATION,true);
			return false;
		}

		if (pvt->_debugsqlrresultsettranslation) {
			stdoutput.printf("translated:\n%.*s\n\n",
						*fieldsize,*field);
		}
	}
	return true;
}

bool sqlrservercontroller::reformatRow(sqlrservercursor *cursor,
						uint32_t colcount,
						const char * const *names,
						const char ***fields,
						uint64_t **fieldsizes) {

	// run translations
	if (pvt->_sqlrrsrt) {
		if (pvt->_debugsqlrresultsetrowtranslation) {
			stdoutput.printf("\n===================="
				 	"===================="
				 	"===================="
				 	"===================\n\n");
			stdoutput.printf("translating result set row\n");
			for (uint32_t i=0; i<colcount; i++) {
				stdoutput.printf("field %d (%s)...\n",
								i,names[i]);
				stdoutput.printf("original:\n%s\n",
								(*fields)[i]);
			}
		}

		if (!pvt->_sqlrrsrt->run(pvt->_conn,cursor,colcount,
						names,fields,fieldsizes)) {
			setError(cursor,pvt->_sqlrrsrt->getError(),
				SQLR_ERROR_RESULTSETROWTRANSLATION,true);
			return false;
		}

		if (pvt->_debugsqlrresultsetrowtranslation) {
			for (uint32_t i=0; i<colcount; i++) {
				stdoutput.printf("translated:\n%.*s\n\n",
					(*fieldsizes)[i],(*fields)[i]);
			}
		}
	}
	return true;
}

void sqlrservercontroller::closeAllResultSets() {
	debugStart("closing result sets for all cursors");
	for (int32_t i=0; i<pvt->_cursorcount; i++) {
		if (pvt->_cur[i]) {
			pvt->_cur[i]->closeResultSet();
		}
	}
	debugEnd();
}

void sqlrservercontroller::endSession() {

	debugStart("ending session");

	setState(SESSION_END);

	debugStart("aborting all cursors");
	for (int32_t i=0; i<pvt->_cursorcount; i++) {
		if (pvt->_cur[i]) {
			abort(pvt->_cur[i]);
			release(pvt->_cur[i]);
		}
	}
	debugWrite("success");
	debugEnd();

	// must set suspendedsession to false here so resumed sessions won't 
	// automatically re-suspend
	pvt->_suspendedsession=false;

	// Run end-of-session rollback or commit before dropping tables and
	// running session-end-queries.  Some queries, including drop table,
	// cause an implicit commit.  If we need to rollback, then make sure
	// that's done first.
	if (
		// NOTE:
		// When using a database that supports sql blocks, it's
		// possible to construct a block that confuses the
		// begin-interceptor into not intercepting the begin.
		//
		// Eg:
		//
		// select 1
		// begin transaction;
		//
		// (sent as a single query)
		//
		// If an app does this on purpose (intending to manually run a
		// commit later) and then runs some DML and then ends up
		// ending the session before running the commit/rollback, then
		// a commit/rollback needs to be run here.
		//
		// Call the commit/rollback if we're faking explicit
		// transactions.
		//
		// Worst case, if we weren't in a fake explicit transaction
		// (and were in autocommit mode) and the db cares, then it will
		// throw an error, which will be ignored.
		fakingExplicitTransactions() ||

		// call the commit/rollback if we're faking
		// implicit transactions
		fakingImplicitTransactions() ||

		(isTransactional() &&
			pvt->_needscommitorrollback)) {

		// otherwise, commit or rollback as necessary
		if (pvt->_cfg->getEndOfSessionCommit()) {
			debugStart("committing");
			commit();
			debugEnd();
		} else {
			debugStart("rolling back");
			rollback();
			debugEnd();
		}
	}

	// truncate/drop temp tables
	// (Do this before running the end-session queries becuase
	// with oracle, it may be necessary to log out and log back in to
	// drop a temp table.  With each log-out the session end queries
	// are run and with each log-in the session start queries are run.)
	truncateTempTables(pvt->_cur[0]);
	dropTempTables(pvt->_cur[0]);

	// run session-end queries
	sessionEndQueries();

	// reset catalog
	if (pvt->_catalogchanged) {
		// FIXME: we're ignoring the result and error,
		// should we do something if there's an error?
		pvt->_conn->selectCatalog(pvt->_originalcatalog);
		pvt->_catalogchanged=false;
	}

	// reset schema
	if (pvt->_schemachanged) {
		// FIXME: we're ignoring the result and error,
		// should we do something if there's an error?
		pvt->_conn->selectSchema(pvt->_originalschema);
		pvt->_schemachanged=false;
	}

	// reset transaction model (commits any in-flight tx under the
	// current model, then drives state to the initial model's baseline)
	setTransactionModel(pvt->_initialtxmodel);

	// reset initial autocommit behavior
	setAutoCommit(pvt->_initialautocommit);

	// set isolation level
	pvt->_conn->setIsolationLevel(pvt->_isolationlevel);

	// NOTE: For debugging, it's nice to know what the most recent
	// clientinfo was, so lets not reset this.  Hopefully not resetting it
	// doesn't break something.
	// reset the client info
	//setClientInfo("",0);

	// reset protocol modules
	if (pvt->_sqlrpr) {
		pvt->_sqlrpr->endSession();
	}

	// reset parser modules
	if (pvt->_sqlrp) {
		pvt->_sqlrp->endSession();
	}

	// reset translation modules
	pvt->_tablenamemap.clear();
	pvt->_indexnamemap.clear();
	if (pvt->_sqlrt) {
		pvt->_sqlrt->endSession();
	}
	for (int32_t i=0; i<pvt->_cursorcount; i++) {
		if (pvt->_cur[i]) {
			pvt->_cur[i]->getTranslatedQueryBuffer()->clear();
		}
	}

	// reset filter modules
	if (pvt->_sqlrf) {
		pvt->_sqlrf->endSession();
	}

	// reset bind variable translation modules
	if (pvt->_sqlrbvt) {
		pvt->_sqlrbvt->endSession();
	}

	// reset result set header translation modules
	if (pvt->_sqlrrsht) {
		pvt->_sqlrrsht->endSession();
	}

	// reset result set translation modules
	if (pvt->_sqlrrst) {
		pvt->_sqlrrst->endSession();
	}

	// reset result set row translation modules
	if (pvt->_sqlrrsrt) {
		pvt->_sqlrrsrt->endSession();
	}

	// reset result set row block translation modules
	if (pvt->_sqlrrsrbt) {
		pvt->_sqlrrsrbt->endSession();
	}

	// reset error translation modules
	if (pvt->_sqlret) {
		pvt->_sqlret->endSession();
	}

	// reset trigger modules
	if (pvt->_sqlrtr) {
		pvt->_sqlrtr->endSession();
	}

	// reset logger modules
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->endSession();
	}

	// reset notification modules
	if (pvt->_sqlrn) {
		pvt->_sqlrn->endSession();
	}

	// reset schedule modules
	if (pvt->_sqlrs) {
		pvt->_sqlrs->endSession();
	}

	// reset query modules
	if (pvt->_sqlrq) {
		pvt->_sqlrq->endSession();
	}

	// reset auth modules
	if (pvt->_sqlra) {
		pvt->_sqlra->endSession();
	}

	// reset module datas
	if (pvt->_sqlrmd) {
		pvt->_sqlrmd->endSession();
	}

	// clear per-session pool
	pvt->_sessionpool.clear();

	// shrink the cursor array, if necessary
	// FIXME: it would probably be more efficient to scale
	// these down gradually rather than all at once
	while (pvt->_cursorcount>pvt->_mincursorcount) {
		pvt->_cursorcount--;
		close(pvt->_cur[pvt->_cursorcount]);
		deleteCursor(pvt->_cur[pvt->_cursorcount]);
		pvt->_cur[pvt->_cursorcount]=NULL;
	}

	// end the session
	pvt->_conn->endSession();

	// if the db is behind a load balancer, re-login
	// periodically to redistribute connections
	if (pvt->_constr->getBehindLoadBalancer()) {
		debugStart("relogging in to redistribute connections");
		datetime	dt;
		if (dt.initFromSystemDateTime()) {
			if (dt.getEpoch()>=pvt->_relogintime) {
				reLogIn(false);
			}
		}
		debugEnd();
	}

	debugEnd();
}

void sqlrservercontroller::dropTempTables(sqlrservercursor *cursor) {

	// run through the temp table list, dropping tables
	for (listnode< char * >
			*sln=pvt->_temptablesfordrop.getFirst();
			sln; sln=sln->getNext()) {

		// some databases (oracle) require us to truncate the
		// table before it can be dropped
		if (pvt->_conn->tempTableTruncateBeforeDrop()) {
			truncateTempTable(cursor,sln->getValue());
		}

		dropTempTable(cursor,sln->getValue());
		delete[] sln->getValue();
	}
	pvt->_temptablesfordrop.clear();
}

void sqlrservercontroller::dropTempTable(sqlrservercursor *cursor,
						const char *tablename) {

	stringbuffer	dropquery;
	dropquery.append("drop table ");
	dropquery.append(pvt->_conn->tempTablePrefix());
	dropquery.append(tablename);

	// kind of a kluge...
	// The cursor might already have a querytree associated with it and
	// if it does then executeQuery below might cause some triggers to
	// be run on that tree rather than on the tree for the drop query
	// we intend to run.
	cursor->clearQueryTree();

	if (prepareQuery(cursor,dropquery.getString(),dropquery.getSize())) {
		executeQuery(cursor);
	}
	cursor->closeResultSet();
}

void sqlrservercontroller::truncateTempTables(sqlrservercursor *cursor) {

	// run through the temp table list, truncating tables
	for (listnode< char * >
			*sln=pvt->_temptablesfortrunc.getFirst();
			sln; sln=sln->getNext()) {
		truncateTempTable(cursor,sln->getValue());
		delete[] sln->getValue();
	}
	pvt->_temptablesfortrunc.clear();

	// truncate global temp tables...

	// all tables...
	if (pvt->_allglobaltemptables) {

		const char	*tablename=NULL;
		uint64_t	fieldsize;
		bool		lob;
		bool		null;
		const char	*query=getGlobalTempTableListQuery();

		sqlrservercursor	*gttcur=newCursor();
		if (open(gttcur) &&
			prepareQuery(gttcur,query,
				charstring::getLength(query)) &&
			executeQuery(gttcur)) {

			bool	error;
			while (fetchRow(gttcur,&error)) {
				getField(gttcur,0,
					&tablename,&fieldsize,&lob,&null);
				truncateTempTable(cursor,tablename);

				// FIXME: kludgy
				nextRow(gttcur);
			}
		}
		closeResultSet(gttcur);
		close(gttcur);
		deleteCursor(gttcur);

		return;
	}

	// specific tables...
	for (listnode< char * > *sln=pvt->_globaltemptables.getFirst();
						sln; sln=sln->getNext()) {
		truncateTempTable(cursor,sln->getValue());
	}
}

void sqlrservercontroller::truncateTempTable(sqlrservercursor *cursor,
						const char *tablename) {
	stringbuffer	truncatequery;
	truncatequery.append(cursor->truncateTableQuery());
	truncatequery.append(" ")->append(tablename);
	if (prepareQuery(cursor,truncatequery.getString(),
					truncatequery.getSize())) {
		executeQuery(cursor);
	}
	cursor->closeResultSet();
}

void sqlrservercontroller::closeClientConnection(uint32_t bytes) {

	// Sometimes the server sends the result set and closes the socket
	// while part of it is buffered but not yet transmitted.  This causes
	// the client to receive a partial result set or error.  Telling the
	// socket to linger doesn't always fix it.  Doing a read here should 
	// guarantee that the client will close its end of the connection 
	// before the server closes its end; the server will wait for data 
	// from the client (which it will never receive) and when the client 
	// closes its end (which it will only do after receiving the entire
	// result set) the read will fall through.  This should guarantee 
	// that the client will get the the entire result set without
	// requiring the client to send data back indicating so.
	//
	// Also, if auth fails, the client could send an entire query
	// and bind vars before it reads the error and closes the socket.
	// We have to absorb all of that data.  We shouldn't just loop forever
	// though, that would provide a point of entry for a DOS attack.  We'll
	// read the maximum number of bytes that could be sent.
	debugStart("waiting for client to close the connection");
	uint16_t	dummy;
	uint32_t	counter=0;
	pvt->_clientsock->setNonBlockingMode(true);
	while (pvt->_clientsock->read(&dummy,pvt->_idleclienttimeout,0)>0 &&
								counter<bytes) {
		counter++;
	}
	pvt->_clientsock->setNonBlockingMode(false);
	debugWrite("success");
	debugEnd();

	// close the client socket
	debugStart("closing the client socket");
	pvt->_clientsock->close();
	delete pvt->_clientsock;
	debugWrite("success");
	debugEnd();

	// in proxy mode, the client socket is pointed at the handoff
	// socket which now needs to be reestablished
	if (pvt->_proxymode) {
		registerForHandoff();
	}
}

void sqlrservercontroller::closeSuspendedSessionSockets() {

	if (pvt->_suspendedsession) {
		return;
	}

	// If we're no longer in a suspended session but had to open a set of
	// sockets to handle a suspended session, close those sockets here.
	if (pvt->_serversockun || pvt->_serversockin) {
		debugStart("closing sockets from "
				"a previously suspended session");
	}
	if (pvt->_serversockun) {
		pvt->_lsnr.removeFileDescriptor(pvt->_serversockun);
		delete pvt->_serversockun;
		pvt->_serversockun=NULL;
	}
	if (pvt->_serversockin) {
		for (uint64_t index=0;
				index<pvt->_serversockincount;
				index++) {
			pvt->_lsnr.removeFileDescriptor(
					pvt->_serversockin[index]);
			delete pvt->_serversockin[index];
			pvt->_serversockin[index]=NULL;
		}
		delete[] pvt->_serversockin;
		pvt->_serversockin=NULL;
		pvt->_serversockincount=0;
	}
	if (pvt->_serversockun || pvt->_serversockin) {
		debugEnd();
	}
}

void sqlrservercontroller::shutDown() {

	debugStart("closing connection");

	if (pvt->_inclientsession) {
		endSession();
		decrementOpenClientConnections();
		pvt->_inclientsession=false;
	}

	// decrement the connection counter or signal the scaler to
	if (pvt->_decrementonclose && 
			pvt->_cfg->getDynamicScaling() &&
			pvt->_semset &&
			pvt->_shmem) {
		decrementConnectionCount();
	}

	// deregister and close the handoff socket if necessary
	if (pvt->_connected) {
		deRegisterForHandoff();
	}

	// close the cursors
	closeCursors(true);

	// try to log out
	logOut();

	// clear the pool
	pvt->_lsnr.removeAllFileDescriptors();

	// close, clean up all sockets
	delete pvt->_serversockun;

	for (uint64_t index=0; index<pvt->_serversockincount; index++) {
		delete pvt->_serversockin[index];
	}
	delete[] pvt->_serversockin;

	// The scaler might need to decrement the connection count after
	// waiting for the child to exit.  On unix-like platforms, we can
	// handle that with SIGCHLD/waitpid().  On other platforms we can
	// do it with a semaphore.
	if (!pvt->_decrementonclose &&
			pvt->_cfg &&
			pvt->_cfg->getDynamicScaling() &&
			pvt->_semset &&
			pvt->_shmem &&
			!process::supportsGetChildStateChange()) {
		pvt->_semset->signal(11);
	}

	debugEnd();
}

void sqlrservercontroller::closeCursors(bool destroy) {

	debugStart("closing cursors");

	if (pvt->_cur) {
		while (pvt->_cursorcount) {
			pvt->_cursorcount--;

			if (pvt->_cur[pvt->_cursorcount]) {
				pvt->_cur[pvt->_cursorcount]->closeResultSet();
				close(pvt->_cur[pvt->_cursorcount]);
				if (destroy) {
					deleteCursor(
						pvt->_cur[pvt->_cursorcount]);
					pvt->_cur[pvt->_cursorcount]=NULL;
				}
			}
		}
		if (destroy) {
			delete[] pvt->_cur;
			pvt->_cur=NULL;
		}
	}

	debugWrite("success");
	debugEnd();
}

void sqlrservercontroller::release(sqlrservercursor *cursor) {
	cursor->setState(SQLRCURSORSTATE_AVAILABLE);
}

void sqlrservercontroller::deleteCursor(sqlrservercursor *cursor) {
	pvt->_overrideaffectedrows.remove(cursor);
	pvt->_affectedrows.remove(cursor);
	pvt->_conn->deleteCursor(cursor);
	decrementOpenDatabaseCursors();
}

bool sqlrservercontroller::createSharedMemoryAndSemaphores(const char *id) {

	debugStart("attaching to shared memory and semaphores");

	char	*idfilename=NULL;
	charstring::printf(&idfilename,"%s%s.ipc",pvt->_pth->getIpcDir(),id);

       	debugWrite("id filename: %s",idfilename);

	// connect to the shared memory
	debugStart("attaching to shared memory");
	pvt->_shmem=new sharedmemory();
	if (!pvt->_shmem->attach(file::generateKey(idfilename,1),
						sizeof(sqlrshm))) {
		char	*err=error::getErrorString();
		stderror.printf("Couldn't attach to shared memory segment: "
				"%s\n",err);
		delete[] err;
		delete pvt->_shmem;
		pvt->_shmem=NULL;
		delete[] idfilename;
		debugWrite("failed");
		debugEnd();
		return false;
	}
	pvt->_shm=(sqlrshm *)pvt->_shmem->getPointer();
	if (!pvt->_shm) {
		stderror.printf("Failed to get pointer to shm\n");
		delete pvt->_shmem;
		pvt->_shmem=NULL;
		delete[] idfilename;
		debugWrite("failed");
		debugEnd();
		return false;
	}
	debugWrite("success");
	debugEnd();


	// connect to the semaphore set
	debugStart("attaching to semaphores");
	pvt->_semset=new semaphoreset();
	if (!pvt->_semset->attach(file::generateKey(idfilename,1),13)) {
		char	*err=error::getErrorString();
		stderror.printf("Couldn't attach to semaphore set: "
				"%s\n",err);
		delete[] err;
		delete pvt->_semset;
		delete pvt->_shmem;
		pvt->_semset=NULL;
		pvt->_shmem=NULL;
		delete[] idfilename;
		debugWrite("failed");
		debugEnd();
		return false;
	}
	debugWrite("success");
	debugEnd();

	debugEnd();

	delete[] idfilename;

	return true;
}

void sqlrservercontroller::decrementConnectedClientCount() {

	debugStart("decrementing session count");

	if (!pvt->_semset->waitWithUndo(5)) {
		// FIXME: bail somehow
	}

	// increment the connections-in-use count
	if (pvt->_shm->connectedclients) {
		pvt->_shm->connectedclients--;
	}

	// update the peak connections-in-use count
	if (pvt->_shm->connectedclients>pvt->_shm->peak_connectedclients) {
		pvt->_shm->peak_connectedclients=pvt->_shm->connectedclients;
	}

	// update the peak connections-in-use over the previous minute count
	datetime	dt;
	dt.initFromSystemDateTime();
	if (pvt->_shm->connectedclients>
			pvt->_shm->peak_connectedclients_1min ||
		dt.getEpoch()/60>
			pvt->_shm->peak_connectedclients_1min_time/60) {

		pvt->_shm->peak_connectedclients_1min=
				pvt->_shm->connectedclients;
		pvt->_shm->peak_connectedclients_1min_time=
				dt.getEpoch();
	}

	if (!pvt->_semset->signalWithUndo(5)) {
		// FIXME: bail somehow
	}

	debugEnd();
}

bool sqlrservercontroller::acquireShmAccess() {

	debugStart("acquiring exclusive shm access");

	setState(WAIT_SEMAPHORE);

	bool	timedout=false;
	if (!semWait(pvt->_semset,0,NULL,true,pvt->_ttl,&timedout)) {
		if (timedout) {
			debugWrite("timeout");
		} else {
			debugWrite("failed");
		}
		debugEnd();
		return false;
	}

	// success...
	debugWrite("success");
	debugEnd();
	return true;
}

bool sqlrservercontroller::resetSemaphores() {

	debugStart("resetting semaphores");

	if (!pvt->_semset->setValue(2,0)) {
		debugWrite("failed to reset semaphore 2");
		debugEnd();
		return false;
	}
	if (!pvt->_semset->setValue(3,0)) {
		debugWrite("failed to reset semaphore 3");
		debugEnd();
		return false;
	}

	debugWrite("success");
	debugEnd();
	return true;
}

void sqlrservercontroller::signalListenerToRead() {
	debugStart("signaling listener to read");
	pvt->_semset->signal(2);
	debugWrite("success");
	debugEnd();
}

bool sqlrservercontroller::waitForListenerToFinishReading() {

	debugStart("waiting for listener");

	bool	timedout=false;
	if (!semWait(pvt->_semset,3,NULL,false,pvt->_ttl,&timedout)) {
		if (timedout) {
			debugWrite("timeout");
		} else {
			debugWrite("failed");
		}
		debugEnd();
		return false;
	}

	// success...
	debugWrite("success");
	debugEnd();
	return true;
}

void sqlrservercontroller::releaseShmAccess() {
	debugStart("releasing exclusive shm access");
	pvt->_semset->signalWithUndo(0);
	debugWrite("success");
	debugEnd();
}

void sqlrservercontroller::acquireConnectionCountMutex() {
	debugStart("acquiring connection count mutex");
	pvt->_semset->waitWithUndo(4);
	debugWrite("success");
	debugEnd();
}

void sqlrservercontroller::releaseConnectionCountMutex() {
	debugStart("releasing connection count mutex");
	pvt->_semset->signalWithUndo(4);
	debugWrite("success");
	debugEnd();
}

void sqlrservercontroller::signalScalerToRead() {
	debugStart("signaling scaler to read");
	pvt->_semset->signal(8);
	debugWrite("success");
	debugEnd();
}

void sqlrservercontroller::initConnStats() {

	pvt->_semset->waitWithUndo(9);

	// Find an available location in the connstats array.
	// It shouldn't be possible for sqlr-start or sqlr-scaler to start
	// more than MAXCONNECTIONS, so unless someone started one manually,
	// it should always be possible to find an open one.
	for (uint32_t i=0; i<MAXCONNECTIONS; i++) {
		pvt->_connstats=&(pvt->_shm->connstats[i]);
		if (!pvt->_connstats->processid) {

			pvt->_semset->signalWithUndo(9);

			// initialize the connection stats
			clearConnStats();
			setState(INIT);
			pvt->_connstats->index=i;
			pvt->_connstats->processid=process::getProcessId();
			pvt->_connstats->loggedinsec=pvt->_loggedinsec;
			pvt->_connstats->loggedinusec=pvt->_loggedinusec;
			return;
		}
	}

	pvt->_semset->signalWithUndo(9);

	// in case someone started a connection manually and
	// exceeded MAXCONNECTIONS, set this NULL here
	pvt->_connstats=NULL;
}

void sqlrservercontroller::clearConnStats() {
	if (!pvt->_connstats) {
		return;
	}
	bytestring::zero(pvt->_connstats,sizeof(struct sqlrconnstatistics));
}

sqlrparser *sqlrservercontroller::newParser() {

	pvt->_sqlrpnode=pvt->_cfg->getParser();
	const char	*module=pvt->_sqlrpnode->getAttributeValue("module");
	if (charstring::isNullOrEmpty(module)) {
		module="default";
	}

	pvt->_debugsqlrparser=pvt->_cfg->getDebugParser();

	if (pvt->_debugsqlrparser) {
		stdoutput.printf("loading parser module: %s\n",module);
	}

#ifdef SQLRELAY_ENABLE_SHARED
	// load the parser module
	stringbuffer	modulename;
	modulename.append(pvt->_pth->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("parser_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	if (!pvt->_sqlrpdl.open(modulename.getString(),true,true)) {
		char	*error=pvt->_sqlrpdl.getError();
		stderror.printf("failed to load parser module: %s\n%s\n",
						module,(error)?error:"");
		delete[] error;
		return NULL;
	}

	// load the parser itself
	stringbuffer	functionname;
	functionname.append("new_sqlrparser_")->append(module);
	sqlrparser	*(*newParser)(sqlrservercontroller *, domnode *)=
			(sqlrparser *(*)(sqlrservercontroller *, domnode *))
			pvt->_sqlrpdl.getSymbol(functionname.getString());
	if (!newParser) {
		char	*error=pvt->_sqlrpdl.getError();
		stderror.printf("failed to load parser: %s\n%s\n",
				module,(error)?error:"");
		delete[] error;
		return NULL;
	}

	sqlrparser	*parser=(*newParser)(this,pvt->_sqlrpnode);

#else
	sqlrparser	*parser;
	stringbuffer	parsername;
	parsername.append(module);
	#include "sqlrparserassignments.cpp"
	{
		parser=NULL;
	}
#endif

	if (pvt->_debugsqlrparser) {
		stdoutput.printf("success\n");
	}

	return parser;
}

bool sqlrservercontroller::bulkLoadBegin(const char *id,
						const char *errorfieldtable,
						const char *errorrowtable,
						uint64_t maxerrorcount,
						bool droperrortables) {

	// FIXME: validate "errorfieldtable" for safety
	// FIXME: validate "errorrowtable" for safety

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load begin:\n"
				"		id: \"%s\"\n",
						process::getProcessId(),id);
		stdoutput.printf("		error table 1: \"%s\"\n"
				"		error table 2: \"%s\"\n"
				"		max error count: %lld\n"
				"		drop error tables: %s\n",
				errorfieldtable,
				errorrowtable,
				maxerrorcount,
				(droperrortables)?"yes":"no");
	}

	// FIXME: bail if error tables already exist

	// get an md5 sum of the id
	// use this rather than using the table directly because:
	// * the id could be sensitive information
	// * the id might not conform to valid file naming conventions
	md5	m;
	m.append((const byte_t *)id,charstring::getLength(id));
	char	*md5str=charstring::hexEncode(m.getHash(),m.getHashSize());
	id=md5str;

	// create a key file and key
	delete[] pvt->_bulkserveridfilename;
	charstring::printf(&pvt->_bulkserveridfilename,"%sbulk-%s.ipc",
						pvt->_pth->getIpcDir(),id);
	delete[] md5str;
	if (!file::createFile(pvt->_bulkserveridfilename,
				permissions::getOwnerReadWrite())) {
		setError(SQLR_ERROR_BULKLOADBEGIN_IPC_FILE_STRING,
				SQLR_ERROR_BULKLOADBEGIN_IPC_FILE,true);
		bulkLoadEnd();
		return false;
	}
	key_t	key=file::generateKey(pvt->_bulkserveridfilename,1);
	if (key==-1) {
		setError(SQLR_ERROR_BULKLOADBEGIN_IPC_KEY_STRING,
				SQLR_ERROR_BULKLOADBEGIN_IPC_KEY,true);
		bulkLoadEnd();
		return false;
	}

	// calculate shared memory segment size
	uint64_t	shmsize=charstring::getLength(errorfieldtable)+1+
				charstring::getLength(errorrowtable)+1+
				sizeof(uint64_t)+
				sizeof(bool)+
				pvt->_maxquerysize+1+
				sizeof(uint16_t)+
				pvt->_maxbindcount*
					(sizeof(sqlrserverbindvartype_t)+
					sizeof(uint32_t));

	// create shared memory
	pvt->_bulkservershmem=new sharedmemory;
	if (!pvt->_bulkservershmem->create(key,shmsize,
				permissions::parsePermString("rw-r-----"))) {
		setError(SQLR_ERROR_BULKLOADBEGIN_SHM_STRING,
				SQLR_ERROR_BULKLOADBEGIN_SHM,true);
		bulkLoadEnd();
		return false;
	}
	pvt->_bulkservershm=(byte_t *)pvt->_bulkservershmem->getPointer();
	bytestring::zero(pvt->_bulkservershm,pvt->_maxquerysize+1);

	// put error tables, maxerrorcount,
	// and drop error tables flag in shared memory
	byte_t	*ptr=pvt->_bulkservershm;

	uint64_t	size=charstring::getLength(errorfieldtable);
	pvt->_bulkerrorfieldtable=(const char *)ptr;
	bytestring::copy(ptr,errorfieldtable,size);
	ptr+=size;
	*ptr='\0';
	ptr++;

	size=charstring::getLength(errorrowtable);
	pvt->_bulkerrorrowtable=(const char *)ptr;
	bytestring::copy(ptr,errorrowtable,size);
	ptr+=size;
	*ptr='\0';
	ptr++;

	bytestring::copy(ptr,&maxerrorcount,sizeof(uint64_t));
	ptr+=sizeof(uint64_t);

	bytestring::copy(ptr,&droperrortables,sizeof(bool));
	ptr+=sizeof(bool);

	// get positions for query and data format
	pvt->_bulkservershmquery=ptr;
	pvt->_bulkservershmdataformat=ptr+sizeof(uint64_t)+pvt->_maxquerysize+1;

	return true;
}

bool sqlrservercontroller::bulkLoadCheckpoint(const char *id) {

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load checkpoint: \"%s\"\n",
						process::getProcessId(),id);
	}

	// FIXME: not sure what to do here...
	// maybe run a checkpoint query on each joined connection?

	// FIXME: do something...
	return true;
}

bool sqlrservercontroller::bulkLoadPrepareQuery(const char *query,
						uint64_t querysize,
						uint16_t inbindcount,
						sqlrserverbindvar *inbinds) {

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load prepare query:\n%.*s\n",
				process::getProcessId(),querysize,query);
	}

	// validate that the query is an insert
	const char	*ptr=skipWhitespaceAndComments(query);
	if (charstring::compareIgnoringCase(ptr,"insert",6) ||
				!character::isWhitespace(*(ptr+6))) {
		setError(SQLR_ERROR_BULKLOADPREPARE_INVALID_QUERY_STRING,
				SQLR_ERROR_BULKLOADPREPARE_INVALID_QUERY,true);
		return false;
	}

	// create error tables
	if (!bulkLoadCreateErrorTables(query,querysize,
					pvt->_bulkerrorfieldtable,
					pvt->_bulkerrorrowtable)) {
		return false;
	}

	// put the query length and query in shared memory
	byte_t	*qptr=pvt->_bulkservershmquery;
	*((uint64_t *)qptr)=querysize;
	qptr+=sizeof(uint64_t);
	bytestring::copy(qptr,query,querysize);
	qptr[querysize]='\0';

	// put the data format in shared memory...
	ptr=(const char *)pvt->_bulkservershmdataformat;

	// copy in the number of data format elements...
	*((uint16_t *)ptr)=inbindcount;
	ptr+=sizeof(uint16_t);

	// for each data format element
	for (uint16_t i=0; i<inbindcount; i++) {

		sqlrserverbindvar	*inbind=&(inbinds[i]);

		// copy in the data format element type
		*((sqlrserverbindvartype_t *)ptr)=inbind->type;
		ptr+=sizeof(sqlrserverbindvartype_t);

		// copy in the data format element size
		*((uint32_t *)ptr)=inbind->valuesize;
		ptr+=sizeof(uint32_t);

		if (pvt->_debugbulkload) {
			stdoutput.printf("	%.*s - %d(%d)\n",
						inbind->variablesize,
						inbind->variable,
						inbind->type,
						inbind->valuesize);
		}
	}

	return true;
}

bool sqlrservercontroller::bulkLoadCreateErrorTables(
					const char *query,
					uint64_t querysize,
					const char *errorfieldtable,
					const char *errorrowtable) {

	bool	retval=false;
	sqlrservercursor	*cursor=newCursor();
	if (open(cursor)) {
		retval=bulkLoadCreateErrorTable1(
				cursor,query,querysize,errorfieldtable) &&
			bulkLoadCreateErrorTable2(
				cursor,query,querysize,errorrowtable);
	}
	close(cursor);
	deleteCursor(cursor);

	return retval;
}

bool sqlrservercontroller::bulkLoadCreateErrorTable1(
					sqlrservercursor *cursor,
					const char *query,
					uint64_t querysize,
					const char *errorfieldtable) {

	// FIXME: this is right for teradata, but not right in general...
	// teradata doesn't allow DDL inside of a
	// tx unless it's the last thing in the tx
	bool	wasintx=getInTransaction();
	if (wasintx) {
		prepareQuery(cursor,"ET",2);
		executeQuery(cursor);
		closeResultSet(cursor);

		prepareQuery(cursor,"BT",2);
		executeQuery(cursor);
		closeResultSet(cursor);
	}

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load create error table: %s\n",
							process::getProcessId(),
							errorfieldtable);
	}

	// FIXME: this needs to be overridable per-connection

	const char	*errorfieldtablequery=
			"create table %s ("
			"	ErrorCode integer,"
			"	ErrorFieldName varchar(120),"
			"	DataParcel varbyte(64000)"
			")";

	bool	retval=true;
	stringbuffer	str;
	str.printf(errorfieldtablequery,errorfieldtable);
	if (!prepareQuery(cursor,str.getString(),str.getSize()) ||
						!executeQuery(cursor)) {
		saveErrorFromCursor(cursor);
		retval=false;
	}
	closeResultSet(cursor);

	// FIXME: this is right for teradata, but not right in general...
	// teradata doesn't allow DDL inside of a
	// tx unless it's the last thing in the tx
	if (wasintx) {
		prepareQuery(cursor,"ET",2);
		executeQuery(cursor);
		closeResultSet(cursor);

		prepareQuery(cursor,"BT",2);
		executeQuery(cursor);
		closeResultSet(cursor);
		pvt->_intransaction=true;
		raiseBeginTransactionEvent();
	}

	return retval;
}

bool sqlrservercontroller::bulkLoadCreateErrorTable2(
					sqlrservercursor *cursor,
					const char *query,
					uint64_t querysize,
					const char *errorrowtable) {

	// FIXME: this is right for teradata, but not right in general...
	// teradata doesn't allow DDL inside of a
	// tx unless it's the last thing in the tx
	bool	wasintx=getInTransaction();
	if (wasintx) {
		prepareQuery(cursor,"ET",2);
		executeQuery(cursor);
		closeResultSet(cursor);

		prepareQuery(cursor,"BT",2);
		executeQuery(cursor);
		closeResultSet(cursor);
	}

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load create error table: %s\n",
					process::getProcessId(),errorrowtable);
	}

	// FIXME: this needs to be overridable per-connection
	// and the default implementation needs to construct a create table
	// from scratch, not relying on create table ... as select ... as this
	// isn't supported by all db's


	// get the table name from the query...
	char	*table=NULL;
	bulkLoadParseInsert(query,querysize,&table,NULL,NULL);

	const char	*errorrowtablequery=
			"create table %s as (select * from %s) with no data";

	bool	retval=true;
	stringbuffer	str;
	str.printf(errorrowtablequery,errorrowtable,table);
	if (!prepareQuery(cursor,str.getString(),str.getSize()) ||
						!executeQuery(cursor)) {
		saveErrorFromCursor(cursor);
		retval=false;
	}
	closeResultSet(cursor);
	delete[] table;

	// FIXME: this is right for teradata, but not right in general...
	// teradata doesn't allow DDL inside of a
	// tx unless it's the last thing in the tx
	if (wasintx) {
		prepareQuery(cursor,"ET",2);
		executeQuery(cursor);
		closeResultSet(cursor);

		prepareQuery(cursor,"BT",2);
		executeQuery(cursor);
		closeResultSet(cursor);
		pvt->_intransaction=true;
		raiseBeginTransactionEvent();
	}

	return retval;
}

bool sqlrservercontroller::bulkLoadJoin(const char *id) {

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load join:\n"
				"		id: \"%s\"\n",
					process::getProcessId(),id);
	}

	// get an md5 sum of the id (see bulkLoadBegin for why)
	md5	m;
	m.append((const byte_t *)id,charstring::getLength(id));
	char	*md5str=charstring::hexEncode(m.getHash(),m.getHashSize());
	id=md5str;

	// create the key
	char	*idfilename;
	charstring::printf(&idfilename,"%sbulk-%s.ipc",
				pvt->_pth->getIpcDir(),id);
	delete[] md5str;
	key_t	key=file::generateKey(idfilename,1);
	delete[] idfilename;
	if (key==-1) {
		setError(SQLR_ERROR_BULKLOADJOIN_IPC_KEY_STRING,
				SQLR_ERROR_BULKLOADJOIN_IPC_KEY,true);
		return false;
	}

	// (re)init shared memory
	if (pvt->_bulkclientshmem) {
		delete pvt->_bulkclientshmem;
	}

	// attach to shared memory
	pvt->_bulkclientshmem=new sharedmemory;
	if (!pvt->_bulkclientshmem->attach(key,pvt->_maxquerysize+1)) {
		setError(SQLR_ERROR_BULKLOADJOIN_SHM_STRING,
				SQLR_ERROR_BULKLOADJOIN_SHM,true);
		return false;
	}
	pvt->_bulkclientshm=(byte_t *)pvt->_bulkclientshmem->getPointer();

	// get error tables
	const byte_t	*ptr=pvt->_bulkclientshm;

	pvt->_bulkerrorfieldtable=(const char *)ptr;
	ptr+=charstring::getLength((const char *)ptr);
	ptr++;

	pvt->_bulkerrorrowtable=(const char *)ptr;
	ptr+=charstring::getLength((const char *)ptr);
	ptr++;

	// get max error count
	pvt->_bulkmaxerrorcount=*((uint64_t *)ptr);
	ptr+=sizeof(uint64_t);

	// get drop error tables flag
	pvt->_bulkdroperrortables=*((bool *)ptr);
	ptr+=sizeof(bool);

	// get query size and query
	pvt->_bulkquerysize=*((uint64_t *)ptr);
	ptr+=sizeof(uint64_t);
	pvt->_bulkquery=(const char *)ptr;
	ptr+=pvt->_maxquerysize+1;

	// get data format definitions
	pvt->_bulkdataformat=(const byte_t *)ptr;

	// clear bulk data lists
	pvt->_bulkdata.clear();
	pvt->_bulkdatasize.clear();

	if (pvt->_debugbulkload) {
		stdoutput.printf("		error table 1: \"%s\"\n"
				"		error table 2: \"%s\"\n"
				"		max error count: %lld\n"
				"		drop error tables : %s\n"
				"		query:\n%.*s\n",
				pvt->_bulkerrorfieldtable,
				pvt->_bulkerrorrowtable,
				pvt->_bulkmaxerrorcount,
				(pvt->_bulkdroperrortables)?"yes":"no",
				pvt->_bulkquerysize,
				pvt->_bulkquery);
	}

	return true;
}

bool sqlrservercontroller::bulkLoadInputBind(const byte_t *data,
							uint64_t datasize) {

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load input bind:\n",
					process::getProcessId());
		stdoutput.safePrint(data,datasize);
		stdoutput.write('\n');
	}

	pvt->_bulkdata.append(data);
	pvt->_bulkdatasize.append(datasize);

	return true;
}

bool sqlrservercontroller::bulkLoadExecuteQuery() {

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load execute query - %d rows\n",
						process::getProcessId(),
						pvt->_bulkdata.getCount());
	}

	// init the bulk cursor
	pvt->_bulkcursor=newCursor();

	// open the cursor
	bool	success=true;
	if (!open(pvt->_bulkcursor)) {
		setError(SQLR_ERROR_BULKLOADEXECUTE_OPEN_CURSOR_STRING,
				SQLR_ERROR_BULKLOADEXECUTE_OPEN_CURSOR,true);
		success=false;
	}

	// prepare the query
	if (success && !prepareQuery(pvt->_bulkcursor,
					pvt->_bulkquery,
					pvt->_bulkquerysize,
					true,true,true)) {
		saveErrorFromCursor(pvt->_bulkcursor);
		success=false;
	}

	if (success) {

		bulkLoadInitBinds();

		// run through the bulk data, binding and executing each row
		uint64_t		errorcount=0;
		listnode<const byte_t *>
				*datanode=pvt->_bulkdata.getFirst();
		listnode<uint64_t>
				*datasizenode=pvt->_bulkdatasize.getFirst();
		while (datanode) {

			bulkLoadBindRow(datanode->getValue(),
					datasizenode->getValue());

			if (!executeQuery(pvt->_bulkcursor)) {
				bulkLoadError();
				errorcount++;
			}


			// bail if too many errors occurred
			if (errorcount>pvt->_bulkmaxerrorcount) {
				setError(
			SQLR_ERROR_BULKLOADEXECUTE_TOO_MANY_ERRORS_STRING,
			SQLR_ERROR_BULKLOADEXECUTE_TOO_MANY_ERRORS,true);
				success=false;
				break;
			}

			datanode=datanode->getNext();
			datasizenode=datasizenode->getNext();
		}
	}

	// close the bulk cursor and clean up
	closeResultSet(pvt->_bulkcursor);
	close(pvt->_bulkcursor);
	deleteCursor(pvt->_bulkcursor);
	pvt->_bulkcursor=NULL;
	pvt->_bulkdata.clear();
	pvt->_bulkdatasize.clear();

	return success;
}

void sqlrservercontroller::bulkLoadInitBinds() {

	// get the table, column names, and binds from the query...
	char			*table=NULL;
	linkedlist<char *>	cols;
	cols.setManageArrayValues(true);
	linkedlist<char *>	binds;
	binds.setManageArrayValues(true);
	bulkLoadParseInsert(pvt->_bulkquery,
				pvt->_bulkquerysize,
				&table,&cols,&binds);

	// map columns to binds (if we actually have columns)
	dictionary<char *, char *>	bindtocol;
	bool				havecols=false;
	if (cols.getCount()) {
		havecols=true;
		listnode<char *> *bind=binds.getFirst();
		listnode<char *> *col=cols.getFirst();
		while (bind && col) {
			bindtocol.setValue(bind->getValue(),col->getValue());
			bind=bind->getNext();
			col=col->getNext();
		}
	}

	// Get column info for the table and set bind type accordingly...
	// Ideally we'd call getColumnList() rather than running a "select *",
	// but some ODBC drivers (eg. teradata) don't support SQLColumns(),
	// so getColumnList() fails.  Everyone supports "select *", and as
	// long as we don't fetch anything, it's fast enough.
	sqlrservercursor	*cur=newCursor();
	if (open(cur)) {

		stringbuffer	query;
		query.append("select * from ")->append(table);

		if (prepareQuery(cur,query.getString(),
					query.getSize()) &&
					executeQuery(cur)) {

			memorypool		*bindpool=
						getBindPool(pvt->_bulkcursor);
			sqlrserverbindvar	*inbinds=
						getInputBinds(pvt->_bulkcursor);

			// run through the binds...
			uint16_t	inbindcount=0;
			listnode<char *> *bind=binds.getFirst();
			while (bind) {

				// set up the input bind name
				sqlrserverbindvar	*inbind=
							&(inbinds[inbindcount]);
				char		*var=bind->getValue();
				uint16_t	varsize=
						charstring::getLength(var);
				inbind->variable=
					(char *)bindpool->allocate(varsize+1);
				charstring::copy(inbind->variable,var);
				inbind->variablesize=varsize;

				// figure out which column the bind maps to
				uint16_t	colindex=inbindcount;
				if (havecols) {

					const char	*col=
						bindtocol.getValue(
							bind->getValue());

					for (uint16_t i=0;
							i<colCount(cur);
							i++) {

						if (!charstring::compare(col,
							getColumnName(cur,i))) {
							colindex=i;
						}
					}

					// FIXME: what if we don't find it?
				}

				// get the type of the column
				uint16_t	type=getColumnType(
								cur,colindex);

				// set the bind type from the column type
				// (the order of these tests is import, eg.
				// floats are numbers and dates are binary)
				if (isFloatType(type)) {
					inbind->type=
						SQLRSERVERBINDVARTYPE_DOUBLE;
				} else if (isNumberType(type)) {
					inbind->type=
						SQLRSERVERBINDVARTYPE_INTEGER;
				} else if (isDateTimeType(type)) {
					inbind->type=
						SQLRSERVERBINDVARTYPE_DATE;
				} else if (isBinaryType(type)) {
					inbind->type=
						SQLRSERVERBINDVARTYPE_BLOB;
				} else {
					inbind->type=
						SQLRSERVERBINDVARTYPE_STRING;
				}

				// bump to the next bind
				bind=bind->getNext();
				inbindcount++;
			}

			// set the input bind count
			setInputBindCount(pvt->_bulkcursor,inbindcount);
		}

		closeResultSet(cur);
	} else {
		// FIXME: error...
	}
	close(cur);
	deleteCursor(cur);

	// clean up
	delete[] table;
}

void sqlrservercontroller::bulkLoadParseInsert(const char *query,
						uint64_t querysize,
						char **table,
						linkedlist<char *> *cols,
						linkedlist<char *> *binds) {

	// get query end
	const char	*queryend=query+querysize;

	// skip whitespace and comments
	const char	*ptr=skipWhitespaceAndComments(query);
	if (!*ptr) {
		return;
	}

	// skip "insert"
	ptr+=6;
	if (ptr>=queryend) {
		return;
	}

	// skip whitespace
	ptr=skipWhitespaceAndComments(ptr);
	if (!*ptr) {
		return;
	}

	// skip "into"
	ptr+=4;
	if (ptr>=queryend) {
		return;
	}

	// skip whitespace
	ptr=skipWhitespaceAndComments(ptr);
	if (!*ptr) {
		return;
	}

	// get table
	const char	*start=ptr;
	while (*ptr && !character::isWhitespace(*ptr)) {
		ptr++;
	}
	if (!*ptr) {
		return;
	}

	// return the table
	if (table) {
		// FIXME: make "table" safe to substitute into a query
		*table=charstring::duplicate(start,ptr-start);
	}

	// FIXME: Some db's (teradata) don't require a values keyword.
	// If it is missing then the parenthesized list following the
	// table is the column values, rather than the list of columns.

	// skip to column list
	ptr=skipWhitespaceAndComments(ptr);
	if (!*ptr) {
		return;
	}

	// parse column list
	if (*ptr=='(') {

		// skip (
		ptr++;
		if (!*ptr) {
			return;
		}

		// skip to )
		start=ptr;
		while (*ptr && *ptr!=')') {
			ptr++;
		}
		if (!*ptr) {
			return;
		}

		// parse out columns
		if (cols) {
			char		**parts;
			uint64_t	partcount;
			charstring::split(start,ptr-start,
						",",false,&parts,&partcount);
			for (uint64_t i=0; i<partcount; i++) {
				charstring::bothTrim(parts[i]);
				cols->append(parts[i]);
			}
		}
	}

	// skip )
	ptr++;
	if (!*ptr) {
		return;
	}

	// skip to "values"
	ptr=skipWhitespaceAndComments(ptr);
	if (!*ptr) {
		return;
	}

	// skip "values"
	ptr+=6;
	if (ptr>=queryend) {
		return;
	}

	// skip to actual values
	ptr=skipWhitespaceAndComments(ptr);
	if (!*ptr) {
		return;
	}

	// parse values list
	// FIXME: currently this assumes that all values are binds
	if (*ptr=='(') {

		// skip (
		ptr++;
		if (!*ptr) {
			return;
		}

		// skip to )
		start=ptr;
		while (*ptr && *ptr!=')') {
			ptr++;
		}
		if (!*ptr) {
			return;
		}

		// parse out binds
		if (binds) {
			char		**parts;
			uint64_t	partcount;
			charstring::split(start,ptr-start,
						",",false,&parts,&partcount);
			for (uint64_t i=0; i<partcount; i++) {
				charstring::bothTrim(parts[i]);
				// override whatever bind prefix was in the
				// query with the correct one (in case a
				// non-native bind format was used)
				parts[i][0]=getBindFormat()[0];
				binds->append(parts[i]);
			}
		}
	}
}

void sqlrservercontroller::bulkLoadBindRow(const byte_t *data,
							uint64_t datasize) {

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bind row {\n",process::getProcessId());
	}

	// get the number of data format elements...
	const byte_t	*ptr=pvt->_bulkdataformat;
	uint16_t	formatcount=*((uint16_t *)ptr);
	ptr+=sizeof(uint16_t);

	// get the input bind count and input binds
	uint16_t		inbindcount=getInputBindCount(pvt->_bulkcursor);
	sqlrserverbindvar	*inbinds=getInputBinds(pvt->_bulkcursor);
	memorypool		*bindpool=getBindPool(pvt->_bulkcursor);

	uint16_t formatindex=0;
	uint16_t inbindindex=0;
	for (;;) {

		// get the format element type
		sqlrserverbindvartype_t	type=*((sqlrserverbindvartype_t *)ptr);
		ptr+=sizeof(sqlrserverbindvartype_t);

		// get the format element size
		uint32_t	size=*((uint32_t *)ptr);
		ptr+=sizeof(uint32_t);
		
		// skip delimiters and newlines
		if (type==SQLRSERVERBINDVARTYPE_DELIMITER ||
				type==SQLRSERVERBINDVARTYPE_NEWLINE) {
			if (pvt->_debugbulkload) {
				if (type==SQLRSERVERBINDVARTYPE_DELIMITER) {
					stdoutput.printf(
						"	delimiter: %.*s\n",
						size,data);
				}
				if (type==SQLRSERVERBINDVARTYPE_NEWLINE) {
					stdoutput.printf("	newline\n");
				}
			}
			formatindex++;
			data+=size;
			continue;
		}

		// for now we only support static-length strings
		if (type!=SQLRSERVERBINDVARTYPE_STRING) {
			// FIXME: set error...
			break;
		}

		// get the bind value
		sqlrserverbindvar	*inbind=&(inbinds[inbindindex]);
		const byte_t		*val=data;
		inbind->valuesize=size;
		data+=size;

		if (pvt->_debugbulkload) {
			stdoutput.printf("	%.*s %d(%d): ",
						inbind->variablesize,
						inbind->variable,
						inbind->type,
						size);
		}

		char	*temp=NULL;
		switch (inbind->type) {

			case SQLRSERVERBINDVARTYPE_STRING:
			case SQLRSERVERBINDVARTYPE_BLOB:
			case SQLRSERVERBINDVARTYPE_CLOB:
				inbind->value.stringval=(char *)val;
				if (pvt->_debugbulkload) {
					stdoutput.printf("(%d) ",
						inbind->valuesize);
					stdoutput.printf("%.*s\n",
						inbind->valuesize,
						inbind->value.stringval);
				}
				break;

			case SQLRSERVERBINDVARTYPE_INTEGER:
				inbind->value.integerval=
					charstring::convertToInteger((char *)val);
				inbind->isnull=getNonNullBindValue();
				if (pvt->_debugbulkload) {
					stdoutput.printf("%d\n",
						inbind->value.integerval);
				}
				break;

			case SQLRSERVERBINDVARTYPE_DOUBLE:
				{
				temp=charstring::duplicate(
					(char *)val,inbind->valuesize);
				inbind->value.doubleval.value=
					charstring::convertToFloat(temp);
				inbind->value.doubleval.precision=
					inbind->valuesize-
					((inbind->value.
						doubleval.value<0)?1:0);
				const char	*dot=
					charstring::findFirst(temp,'.');
				if (dot) {
					inbind->value.doubleval.
							precision--;
					dot++;
					inbind->value.doubleval.scale=
						temp+inbind->valuesize-dot;
				} else {
					inbind->value.
						doubleval.scale=0;
				}
				delete[] temp;
				if (pvt->_debugbulkload) {
					stdoutput.printf("%*.*f\n",
						inbind->value.
							doubleval.precision,
						inbind->value.
							doubleval.scale,
						inbind->value.
							doubleval.value);
				}
				}
				break;

			case SQLRSERVERBINDVARTYPE_DATE:
				{
				temp=charstring::duplicate(
					(char *)val,inbind->valuesize);
				charstring::bothTrim(temp);

				// copy out the timezone
				// (and null terminate the date before it)
				char	*firstspace=
					charstring::findFirst(temp," ");
				char	*lastspace=
					charstring::findLast(temp," ");
				if (lastspace && lastspace!=firstspace) {
					const char	*tz=lastspace+1;
					inbind->value.dateval.tz=
						(char *)bindpool->allocate(
						temp+inbind->valuesize-tz+1);
					charstring::copy(inbind->value.
								dateval.tz,tz);
					*lastspace='\0';
				} else {
					inbind->value.dateval.tz=NULL;
				}

				// FIXME: this assumes ISO-ish date/time format
				// but who knows what we'll actually get, the
				// ddmm, yyyyddmm, and delimiter parameters need
				// to be configurable...
				if (!datetime::parse(
						temp,
						false,
						false,
						"-",
						&inbind->value.dateval.year,
						&inbind->value.dateval.month,
						&inbind->value.dateval.day,
						&inbind->value.dateval.hour,
						&inbind->value.dateval.minute,
						&inbind->value.dateval.second,
						&inbind->value.dateval.
								microsecond,
						&inbind->value.dateval.
								isnegative)) {
					// FIXME: what if this fails?
				}
				delete[] temp;

				if (pvt->_debugbulkload) {
					stdoutput.printf(
						"%04hd-%02hd-%02hd "
						"%02hd:%02hd:%02hd.%06d %s\n",
						inbind->value.dateval.year,
						inbind->value.dateval.month,
						inbind->value.dateval.day,
						inbind->value.dateval.hour,
						inbind->value.dateval.minute,
						inbind->value.dateval.second,
						inbind->value.dateval.
								microsecond,
						((inbind->value.dateval.tz)?
							inbind->value.
								dateval.tz:""));
				}
				}

			default:
				break;
		}

		if (type==SQLRSERVERBINDVARTYPE_NULL) {
			inbind->isnull=getNullBindValue();
		} else {
			inbind->isnull=getNonNullBindValue();
		}

		// move on
		inbindindex++;
		if (inbindindex==inbindcount) {
			break;
		}
		formatindex++;
		if (formatindex==formatcount) {
			break;
		}
	}

	if (pvt->_debugbulkload) {
		stdoutput.printf("}\n");
	}
}

void sqlrservercontroller::bulkLoadError() {

	// get the error
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	getError(pvt->_bulkcursor,
			pvt->_bulkcursor->getErrorBuffer(),
			pvt->_maxerrorsize,
			&errorsize,&errnum,&liveconnection);

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load error\n%d\n%.*s\n",
					process::getProcessId(),
					errnum,errorsize,
					pvt->_bulkcursor->getErrorBuffer());
	}

	// store the error
	if (!bulkLoadStoreError(errnum,
				pvt->_bulkcursor->getErrorBuffer(),
				errorsize,
				pvt->_bulkerrorfieldtable,
				pvt->_bulkerrorrowtable)) {
		// FIXME: error...
	}
}

bool sqlrservercontroller::bulkLoadStoreError(int64_t errorcode,
						const char *error,
						uint32_t errorsize,
						const char *bulkerrorfieldtable,
						const char *bulkerrorrowtable) {

	// FIXME: this needs to be overridable per-connection

	// FIXME: reuse a cursor...
	sqlrservercursor	*cur=newCursor();
	if (open(cur)) {

		// insert into errorfieldtable...

		// FIXME: use binds...
		const char	*errorquery=
				"insert into %s values (%lld,'%s','%s')";
		stringbuffer	query;
		query.printf(errorquery,
				bulkerrorfieldtable,
				errorcode,
				// FIXME: get the column somehow
				"bad_column",
				// FIXME: get the data somehow
				"bad_data");

		if (!prepareQuery(cur,query.getString(),
					query.getSize()) ||
					!executeQuery(cur)) {
			// FIXME: error...
		}
		closeResultSet(cur);

		// FIXME: insert into errorrowtable...
	}
	close(cur);
	deleteCursor(cur);

	return true;
}

bool sqlrservercontroller::bulkLoadEnd() {

	if (pvt->_debugbulkload) {
		stdoutput.printf("%d: bulk load end\n",
					process::getProcessId());
	}

	if (pvt->_bulkdroperrortables) {
		if (!bulkLoadDropErrorTables(pvt->_bulkerrorfieldtable,
						pvt->_bulkerrorrowtable)) {
			// FIXME: error...
		}
	}

	// delete shared memory
	delete pvt->_bulkservershmem;
	pvt->_bulkservershmem=NULL;
	pvt->_bulkservershm=NULL;

	// remove the id file
	if (pvt->_bulkserveridfilename) {
		file::remove(pvt->_bulkserveridfilename);
		delete[] pvt->_bulkserveridfilename;
	}
	pvt->_bulkserveridfilename=NULL;

	return true;
}

bool sqlrservercontroller::bulkLoadDropErrorTables(
					const char *errorfieldtable,
					const char *errorrowtable) {
	// FIXME: implement this...
	// this needs to be overridable per-connection
	return true;
}

void sqlrservercontroller::setState(enum sqlrconnectionstate_t state) {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->state=state;
	datetime	dt;
	dt.initFromSystemDateTime();
	pvt->_connstats->statestartsec=dt.getSecond();
	pvt->_connstats->statestartusec=dt.getMicrosecond();
}

enum sqlrconnectionstate_t sqlrservercontroller::getState() {
	if (!pvt->_connstats) {
		return NOT_AVAILABLE;
	}
	return pvt->_connstats->state;
}

void sqlrservercontroller::setClientSessionStartTime() {
	if (!pvt->_connstats) {
		return;
	}
	datetime	dt;
	dt.initFromSystemDateTime();
	pvt->_connstats->clientsessionsec=dt.getSecond();
	pvt->_connstats->clientsessionusec=dt.getMicrosecond();
}

void sqlrservercontroller::setAuthenticatedUser(const char *user,
							uint32_t usersize) {
	if (!pvt->_connstats) {
		return;
	}
	uint32_t	size=usersize;
	if (size>USERSIZE-1) {
		size=USERSIZE-1;
	}
	charstring::copy(pvt->_connstats->user,user,size);
	pvt->_connstats->user[size]='\0';
}

void sqlrservercontroller::setCurrentQuery(const char *query,
						uint32_t querysize) {
	if (!pvt->_connstats) {
		return;
	}
	uint32_t	size=querysize;
	if (size>STATSQLTEXTLEN-1) {
		size=STATSQLTEXTLEN-1;
	}
	charstring::copy(pvt->_connstats->sqltext,query,size);
	pvt->_connstats->sqltext[size]='\0';
}

void sqlrservercontroller::setClientInfo(const char *info,
						uint32_t infosize) {
	if (!pvt->_connstats) {
		return;
	}
	uint64_t	size=infosize;
	if (size>STATCLIENTINFOLEN-1) {
		size=STATCLIENTINFOLEN-1;
	}
	charstring::copy(pvt->_connstats->clientinfo,info,size);
	pvt->_connstats->clientinfo[size]='\0';
}

void sqlrservercontroller::setClientAddr() {
	if (!pvt->_connstats) {
		return;
	}
	if (pvt->_clientsock) {
		char	*clientaddrbuf=pvt->_clientsock->getPeerAddress();
		if (clientaddrbuf) {
			charstring::copy(
				pvt->_connstats->clientaddr,clientaddrbuf);
			delete[] clientaddrbuf;
		} else {
			charstring::copy(
				pvt->_connstats->clientaddr,"UNIX");
		}
	} else {
		charstring::copy(pvt->_connstats->clientaddr,"internal");
	}
}

void sqlrservercontroller::setInstanceDisabled(bool disabled) {
	pvt->_shm->disabled=disabled;
}

bool sqlrservercontroller::getInstanceDisabled() {
	return pvt->_shm->disabled;
}

uint32_t sqlrservercontroller::getOpenClientConnections() {
	return pvt->_shm->open_cli_connections;
}

uint32_t sqlrservercontroller::getOpenedClientConnections() {
	return pvt->_shm->opened_cli_connections;
}

void sqlrservercontroller::incrementOpenClientConnections() {
	pvt->_semset->waitWithUndo(9);
	pvt->_shm->open_cli_connections++;
	pvt->_shm->opened_cli_connections++;
	pvt->_semset->signalWithUndo(9);
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nconnect++;
}

void sqlrservercontroller::decrementOpenClientConnections() {
	pvt->_semset->waitWithUndo(9);
	if (pvt->_shm->open_cli_connections) {
		pvt->_shm->open_cli_connections--;
	}
	pvt->_semset->signalWithUndo(9);
}

uint32_t sqlrservercontroller::getOpenDatabaseConnections() {
	return pvt->_shm->open_db_connections;
}

uint32_t sqlrservercontroller::getOpenedDatabaseConnections() {
	return pvt->_shm->opened_db_connections;
}

void sqlrservercontroller::incrementOpenDatabaseConnections() {
	pvt->_semset->waitWithUndo(9);
	pvt->_shm->open_db_connections++;
	pvt->_shm->opened_db_connections++;
	pvt->_semset->signalWithUndo(9);
}

void sqlrservercontroller::decrementOpenDatabaseConnections() {
	pvt->_semset->waitWithUndo(9);
	if (pvt->_shm->open_db_connections) {
		pvt->_shm->open_db_connections--;
	}
	pvt->_semset->signalWithUndo(9);
}

uint32_t sqlrservercontroller::getOpenDatabaseCursors() {
	return pvt->_shm->open_db_cursors;
}

uint32_t sqlrservercontroller::getOpenedDatabaseCursors() {
	return pvt->_shm->opened_db_cursors;
}

void sqlrservercontroller::incrementOpenDatabaseCursors() {
	pvt->_semset->waitWithUndo(9);
	pvt->_shm->open_db_cursors++;
	pvt->_shm->opened_db_cursors++;
	pvt->_semset->signalWithUndo(9);
}

void sqlrservercontroller::decrementOpenDatabaseCursors() {
	pvt->_semset->waitWithUndo(9);
	if (pvt->_shm->open_db_cursors) {
		pvt->_shm->open_db_cursors--;
	}
	pvt->_semset->signalWithUndo(9);
}

uint32_t sqlrservercontroller::getTimesNewCursorUsed() {
	return pvt->_shm->times_new_cursor_used;
}

void sqlrservercontroller::incrementTimesNewCursorUsed() {
	pvt->_semset->waitWithUndo(9);
	pvt->_shm->times_new_cursor_used++;
	pvt->_semset->signalWithUndo(9);
}

uint32_t sqlrservercontroller::getTimesCursorReused() {
	return pvt->_shm->times_cursor_reused;
}

void sqlrservercontroller::incrementTimesCursorReused() {
	pvt->_semset->waitWithUndo(9);
	pvt->_shm->times_cursor_reused++;
	pvt->_semset->signalWithUndo(9);
}

uint32_t sqlrservercontroller::getQueryCount(sqlrquerytype_t querytype) {

	datetime	dt;
	dt.initFromSystemDateTime();
	time_t	now=dt.getEpoch();
	int	index=now%STATQPSKEEP;

	switch (querytype) {
		case SQLRQUERYTYPE_SELECT:
			return pvt->_shm->qps_select[index];
		case SQLRQUERYTYPE_INSERT:
		case SQLRQUERYTYPE_INSERTSELECT:
		case SQLRQUERYTYPE_SELECTINTO:
		case SQLRQUERYTYPE_MULTIINSERT:
			return pvt->_shm->qps_insert[index];
		case SQLRQUERYTYPE_UPDATE:
			return pvt->_shm->qps_update[index];
		case SQLRQUERYTYPE_DELETE:
			return pvt->_shm->qps_delete[index];
		case SQLRQUERYTYPE_CREATE:
			return pvt->_shm->qps_create[index];
		case SQLRQUERYTYPE_DROP:
			return pvt->_shm->qps_drop[index];
		case SQLRQUERYTYPE_ALTER:
			return pvt->_shm->qps_alter[index];
		case SQLRQUERYTYPE_CUSTOM:
			return pvt->_shm->qps_custom[index];
		default:
			// catches BEGIN, COMMIT, ROLLBACK, AUTOCOMMIT ON/OFF,
			// SET INCLUDING AUTOCOMMIT ON/OFF, and ETC
			return pvt->_shm->qps_etc[index];
	}
}

uint32_t sqlrservercontroller::getTotalQueryCount() {
	return pvt->_shm->total_queries;
}

void sqlrservercontroller::incrementQueryCount(sqlrquerytype_t querytype) {

	pvt->_semset->waitWithUndo(9);

	// update total queries
	pvt->_shm->total_queries++;

	// update queries-per-second stats...

	// re-init stats if necessary
	datetime	dt;
	dt.initFromSystemDateTime();
	time_t	now=dt.getEpoch();
	int	index=now%STATQPSKEEP;
	if (pvt->_shm->timestamp[index]!=now) {
		pvt->_shm->timestamp[index]=now;
		pvt->_shm->qps_select[index]=0;
		pvt->_shm->qps_update[index]=0;
		pvt->_shm->qps_insert[index]=0;
		pvt->_shm->qps_delete[index]=0;
		pvt->_shm->qps_create[index]=0;
		pvt->_shm->qps_drop[index]=0;
		pvt->_shm->qps_alter[index]=0;
		pvt->_shm->qps_custom[index]=0;
		pvt->_shm->qps_etc[index]=0;
	}

	// increment per-query-type stats
	switch (querytype) {
		case SQLRQUERYTYPE_SELECT:
			pvt->_shm->qps_select[index]++;
			break;
		case SQLRQUERYTYPE_INSERT:
		case SQLRQUERYTYPE_INSERTSELECT:
		case SQLRQUERYTYPE_SELECTINTO:
		case SQLRQUERYTYPE_MULTIINSERT:
			pvt->_shm->qps_insert[index]++;
			break;
		case SQLRQUERYTYPE_UPDATE:
			pvt->_shm->qps_update[index]++;
			break;
		case SQLRQUERYTYPE_DELETE:
			pvt->_shm->qps_delete[index]++;
			break;
		case SQLRQUERYTYPE_CREATE:
			pvt->_shm->qps_create[index]++;
			break;
		case SQLRQUERYTYPE_DROP:
			pvt->_shm->qps_drop[index]++;
			break;
		case SQLRQUERYTYPE_ALTER:
			pvt->_shm->qps_alter[index]++;
			break;
		case SQLRQUERYTYPE_CUSTOM:
			pvt->_shm->qps_custom[index]++;
			break;
		default:
			// catches BEGIN, COMMIT, ROLLBACK, AUTOCOMMIT ON/OFF,
			// SET INCLUDING AUTOCOMMIT ON/OFF, and ETC
			pvt->_shm->qps_etc[index]++;
			break;
	}

	pvt->_semset->signalWithUndo(9);

	if (!pvt->_connstats) {
		return;
	}
	if (querytype==SQLRQUERYTYPE_CUSTOM) {
		pvt->_connstats->ncustomsql++;
	} else {
		pvt->_connstats->nsql++;
	}
}

uint32_t sqlrservercontroller::getTotalErrors() {
	return pvt->_shm->total_errors;
}

void sqlrservercontroller::incrementTotalErrors() {
	pvt->_semset->waitWithUndo(9);
	pvt->_shm->total_errors++;
	pvt->_semset->signalWithUndo(9);
}

uint32_t sqlrservercontroller::getAuthCount() {
	return pvt->_connstats->nauth;
}

void sqlrservercontroller::incrementAuthCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nauth++;
}

uint32_t sqlrservercontroller::getSuspendSessionCount() {
	return pvt->_connstats->nsuspend_session;
}

void sqlrservercontroller::incrementSuspendSessionCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nsuspend_session++;
}

uint32_t sqlrservercontroller::getEndSessionCount() {
	return pvt->_connstats->nend_session;
}

void sqlrservercontroller::incrementEndSessionCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nend_session++;
}

uint32_t sqlrservercontroller::getPingCount() {
	return pvt->_connstats->nping;
}

void sqlrservercontroller::incrementPingCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nping++;
}

uint32_t sqlrservercontroller::getIdentifyCount() {
	return pvt->_connstats->nidentify;
}

void sqlrservercontroller::incrementIdentifyCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nidentify++;
}

uint32_t sqlrservercontroller::getSetAutoCommitCount() {
	return pvt->_connstats->nsetautocommit;
}

void sqlrservercontroller::incrementSetAutoCommitCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nsetautocommit++;
}

uint32_t sqlrservercontroller::getBeginCount() {
	return pvt->_connstats->nbegin;
}

void sqlrservercontroller::incrementBeginCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nbegin++;
}

uint32_t sqlrservercontroller::getCommitCount() {
	return pvt->_connstats->ncommit;
}

void sqlrservercontroller::incrementCommitCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ncommit++;
}

uint32_t sqlrservercontroller::getRollbackCount() {
	return pvt->_connstats->nrollback;
}

void sqlrservercontroller::incrementRollbackCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nrollback++;
}

uint32_t sqlrservercontroller::getDbVersionCount() {
	return pvt->_connstats->ndbversion;
}

void sqlrservercontroller::incrementDbVersionCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ndbversion++;
}

uint32_t sqlrservercontroller::getGetBindFormatCount() {
	return pvt->_connstats->nbindformat;
}

void sqlrservercontroller::incrementGetBindFormatCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nbindformat++;
}

uint32_t sqlrservercontroller::getGetServerVersionCount() {
	return pvt->_connstats->nserverversion;
}

void sqlrservercontroller::incrementGetServerVersionCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nserverversion++;
}

uint32_t sqlrservercontroller::getSelectDatabaseCount() {
	return pvt->_connstats->nselectdatabase;
}

void sqlrservercontroller::incrementSelectDatabaseCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nselectdatabase++;
}

uint32_t sqlrservercontroller::getGetCurrentDatabaseCount() {
	return pvt->_connstats->ngetcurrentdatabase;
}

void sqlrservercontroller::incrementGetCurrentDatabaseCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ngetcurrentdatabase++;
}

uint32_t sqlrservercontroller::getGetLastInsertIdCount() {
	return pvt->_connstats->ngetlastinsertid;
}

void sqlrservercontroller::incrementGetLastInsertIdCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ngetlastinsertid++;
}

uint32_t sqlrservercontroller::getDbHostNameCount() {
	return pvt->_connstats->ndbhostname;
}

void sqlrservercontroller::incrementDbHostNameCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ndbhostname++;
}

uint32_t sqlrservercontroller::getDbIpAddressCount() {
	return pvt->_connstats->ndbipaddress;
}

void sqlrservercontroller::incrementDbIpAddressCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ndbipaddress++;
}

uint32_t sqlrservercontroller::getNewQueryCount() {
	return pvt->_connstats->nnewquery;
}

void sqlrservercontroller::incrementNewQueryCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nnewquery++;
}

uint32_t sqlrservercontroller::getReexecuteQueryCount() {
	return pvt->_connstats->nreexecutequery;
}

void sqlrservercontroller::incrementReexecuteQueryCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nreexecutequery++;
}

uint32_t sqlrservercontroller::getFetchFromBindCursorCount() {
	return pvt->_connstats->nfetchfrombindcursor;
}

void sqlrservercontroller::incrementFetchFromBindCursorCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nfetchfrombindcursor++;
}

uint32_t sqlrservercontroller::getFetchResultSetCount() {
	return pvt->_connstats->nfetchresultset;
}

void sqlrservercontroller::incrementFetchResultSetCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nfetchresultset++;
}

uint32_t sqlrservercontroller::getAbortResultSetCount() {
	return pvt->_connstats->nabortresultset;
}

void sqlrservercontroller::incrementAbortResultSetCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nabortresultset++;
}

uint32_t sqlrservercontroller::getSuspendResultSetCount() {
	return pvt->_connstats->nsuspendresultset;
}

void sqlrservercontroller::incrementSuspendResultSetCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nsuspendresultset++;
}

uint32_t sqlrservercontroller::getResumeResultSetCount() {
	return pvt->_connstats->nresumeresultset;
}

void sqlrservercontroller::incrementResumeResultSetCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nresumeresultset++;
}

uint32_t sqlrservercontroller::getGetDbListCount() {
	return pvt->_connstats->ngetdblist;
}

void sqlrservercontroller::incrementGetDbListCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ngetdblist++;
}

uint32_t sqlrservercontroller::getGetTableListCount() {
	return pvt->_connstats->ngettablelist;
}

void sqlrservercontroller::incrementGetTableListCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ngettablelist++;
}

uint32_t sqlrservercontroller::getGetColumnListCount() {
	return pvt->_connstats->ngetcolumnlist;
}

void sqlrservercontroller::incrementGetColumnListCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ngetcolumnlist++;
}

uint32_t sqlrservercontroller::getGetQueryTreeCount() {
	return pvt->_connstats->ngetquerytree;
}

void sqlrservercontroller::incrementGetQueryTreeCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->ngetquerytree++;
}

uint32_t sqlrservercontroller::getReLogInCount() {
	return pvt->_connstats->nrelogin;
}

void sqlrservercontroller::incrementReLogInCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nrelogin++;
}

uint32_t sqlrservercontroller::getNextResultSetCount() {
	return pvt->_connstats->nnextresultset;
}

void sqlrservercontroller::incrementNextResultSetCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nnextresultset++;
}

uint32_t sqlrservercontroller::getNextResultSetAvailableCount() {
	return pvt->_connstats->nnextresultsetavailable;
}

void sqlrservercontroller::incrementNextResultSetAvailableCount() {
	if (!pvt->_connstats) {
		return;
	}
	pvt->_connstats->nnextresultsetavailable++;
}

const char *sqlrservercontroller::getAuthenticatedUser() {
	return (pvt->_connstats)?pvt->_connstats->user:NULL;
}

const char *sqlrservercontroller::getCurrentQuery() {
	return (pvt->_connstats)?pvt->_connstats->sqltext:NULL;
}

const char *sqlrservercontroller::getClientInfo() {
	return (pvt->_connstats)?pvt->_connstats->clientinfo:NULL;
}

const char *sqlrservercontroller::getClientAddr() {
	return (pvt->_connstats)?pvt->_connstats->clientaddr:NULL;
}

uint32_t sqlrservercontroller::getStatisticsIndex() {
	if (!pvt->_connstats) {
		return 0;
	}
	return pvt->_connstats->index;
}

void sqlrservercontroller::sessionStartQueries() {
	// run a configurable set of queries at the start of each session
	for (listnode< char * > *node=
			pvt->_cfg->getSessionStartQueries()->getFirst();
			node; node=node->getNext()) {
		sessionQuery(node->getValue());
	}
}

void sqlrservercontroller::sessionEndQueries() {
	// run a configurable set of queries at the end of each session
	for (listnode< char * > *node=
			pvt->_cfg->getSessionEndQueries()->getFirst();
			node; node=node->getNext()) {
		sessionQuery(node->getValue());
	}
}

void sqlrservercontroller::sessionQuery(const char *query) {

	// create the select database query
	size_t	querysize=charstring::getLength(query);

	sqlrservercursor	*cur=newCursor();
	if (open(cur) &&
		prepareQuery(cur,query,querysize) && executeQuery(cur)) {
		closeResultSet(cur);
	}
	close(cur);
	deleteCursor(cur);
}

const char *sqlrservercontroller::getConnectStringValue(const char *variable) {

	// If we're using password encryption and the password is requested,
	// and the password encryption module supports two-way encryption,
	// then return the decrypted version of the password, otherwise just
	// return the value as-is.
	const char	*peid=pvt->_constr->getPasswordEncryption();
	if (pvt->_sqlrpe && charstring::getLength(peid) &&
			!charstring::compare(variable,"password")) {

		// handle password files
		sensitivevalue	sv;
		sv.setPath(pvt->_cfg->getPasswordPath());
		sv.parse(pvt->_constr->getConnectStringValue(variable));

		sqlrpwdenc	*pe=
			pvt->_sqlrpe->getPasswordEncryptionById(peid);
		if (!pe->oneWay()) {
			delete[] pvt->_decrypteddbpassword;
			pvt->_decrypteddbpassword=
					pe->decrypt(sv.getTextValue());
			return pvt->_decrypteddbpassword;
		}
	}
	return pvt->_constr->getConnectStringValue(variable);
}

void sqlrservercontroller::setLoginUser(const char *user) {
	pvt->_user=user;
}

void sqlrservercontroller::setLoginPassword(const char *password) {
	pvt->_password=password;
}

const char *sqlrservercontroller::getLoginUser() {
	return pvt->_user;
}

const char *sqlrservercontroller::getLoginPassword() {
	return pvt->_password;
}

void sqlrservercontroller::setConnectTimeout(uint64_t connecttimeout) {
	pvt->_connecttimeout=connecttimeout;
}

uint64_t sqlrservercontroller::getConnectTimeout() {
	return pvt->_connecttimeout;
}

void sqlrservercontroller::setQueryTimeout(uint64_t querytimeout) {
	pvt->_querytimeout=querytimeout;
}

uint64_t sqlrservercontroller::getQueryTimeout() {
	return pvt->_querytimeout;
}

void sqlrservercontroller::setExecuteDirect(bool executedirect) {
	pvt->_executedirect=executedirect;
}

bool sqlrservercontroller::getExecuteDirect() {
	return pvt->_executedirect;
}

void sqlrservercontroller::setDbType(const char *dbtype) {
	pvt->_dbtype=dbtype;
}

void sqlrservercontroller::setInitialAutoCommit(bool iac) {
	if (!iac && !isTransactional()) {
		stderror.printf("Warning: autocommit=no is set, but the "
				"database doesn't support transactions, "
				"falling back to autocommit=yes.\n");
		pvt->_initialautocommit=true;
		return;
	}
	pvt->_initialautocommit=iac;
}

bool sqlrservercontroller::getInitialAutoCommit() {
	return pvt->_initialautocommit;
}

bool sqlrservercontroller::getAutoCommit() {
	return pvt->_autocommit;
}

const char *sqlrservercontroller::getBindFormat() {
	return pvt->_conn->getBindFormat();
}

int16_t sqlrservercontroller::getNonNullBindValue() {
	return pvt->_conn->getNonNullBindValue();
}

int16_t sqlrservercontroller::getNullBindValue() {
	return pvt->_conn->getNullBindValue();
}

bool sqlrservercontroller::getBindValueIsNull(int16_t isnull) {
	return pvt->_conn->getBindValueIsNull(isnull);
}

const char *sqlrservercontroller::getNextvalFormat() {
	return pvt->_conn->getNextvalFormat();
}

void sqlrservercontroller::setFakeInputBinds(bool fake) {
	pvt->_fakeinputbinds=fake;
}

bool sqlrservercontroller::getFakeInputBinds() {
	return pvt->_fakeinputbinds;
}

void sqlrservercontroller::setFetchAtOnce(uint32_t fetchatonce) {
	pvt->_fetchatonce=fetchatonce;
}

void sqlrservercontroller::setMaxColumnCount(uint32_t maxcolumncount) {
	pvt->_maxcolumncount=maxcolumncount;
}

void sqlrservercontroller::setMaxFieldSize(uint32_t maxfieldsize) {
	pvt->_maxfieldsize=maxfieldsize;
}

uint32_t sqlrservercontroller::getFetchAtOnce() {
	return pvt->_fetchatonce;
}

uint32_t sqlrservercontroller::getMaxColumnCount() {
	return pvt->_maxcolumncount;
}

uint32_t sqlrservercontroller::getMaxFieldSize() {
	return pvt->_maxfieldsize;
}

void sqlrservercontroller::addTempTableForDrop(const char *table) {
	pvt->_temptablesfordrop.append(charstring::duplicate(table));
}

void sqlrservercontroller::addGlobalTempTables(const char *gtts) {

	// all tables
	if (!charstring::compare(gtts,"%")) {
		pvt->_allglobaltemptables=true;
		return;
	}

	// specific tables
	char		**gttlist=NULL;
	uint64_t	gttlistcount=0;
	charstring::split(gtts,",",true,&gttlist,&gttlistcount);
	for (uint64_t i=0; i<gttlistcount; i++) {
		pvt->_globaltemptables.append(gttlist[i]);
	}
	delete[] gttlist;
}

void sqlrservercontroller::addTempTableForTrunc(const char *table) {
	pvt->_temptablesfortrunc.append(charstring::duplicate(table));
}

bool sqlrservercontroller::getLoggingEnabled() {
	return (pvt->_sqlrlg!=NULL);
}

bool sqlrservercontroller::getNotificationsEnabled() {
	return (pvt->_sqlrn!=NULL);
}

bool sqlrservercontroller::getDebug() {
	return pvt->_debug;
}

void sqlrservercontroller::debugStart(const char *title, ...) {
	if (!pvt->_debug) {
		return;
	}
	pvt->_logbuffer.clear();
	va_list	argp;
	va_start(argp,title);
	pvt->_logbuffer.printf(title,&argp);
	va_end(argp);
	raiseDebugStartEvent(pvt->_logbuffer.getString());
}

void sqlrservercontroller::debugWrite(const char *string, ...) {
	if (!pvt->_debug) {
		return;
	}
	pvt->_logbuffer.clear();
	va_list	argp;
	va_start(argp,string);
	pvt->_logbuffer.printf(string,&argp);
	va_end(argp);
	raiseDebugWriteEvent(pvt->_logbuffer.getString());
}

void sqlrservercontroller::debugEnd() {
	if (!pvt->_debug) {
		return;
	}
	raiseDebugEndEvent();
}

void sqlrservercontroller::raiseDebugStartEvent(const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->start(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_DEBUG,
					SQLREVENT_DEBUG_MESSAGE,
					pvt->_infobuffer.getString());
	}
	// FIXME: sqlrn?
}

void sqlrservercontroller::raiseDebugWriteEvent(const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_DEBUG,
					SQLREVENT_DEBUG_MESSAGE,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_DEBUG_MESSAGE,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseDebugEndEvent() {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->end(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_DEBUG,
					SQLREVENT_DEBUG_MESSAGE);
	}
	// FIXME: sqlrn?
}

void sqlrservercontroller::raiseClientConnectedEvent() {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_CLIENT_CONNECTED,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_CLIENT_CONNECTED,
					NULL);
	}
}

void sqlrservercontroller::raiseClientConnectionRefusedEvent(
						const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_WARNING,
					SQLREVENT_CLIENT_CONNECTION_REFUSED,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_CLIENT_CONNECTION_REFUSED,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseClientDisconnectedEvent(
						const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_CLIENT_DISCONNECTED,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_CLIENT_DISCONNECTED,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseClientProtocolErrorEvent(
						sqlrservercursor *cursor,
						ssize_t result,
						const char *info, ...) {
	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (result==0) {
		pvt->_infobuffer.append(": client closed connection");
	} else if (result==RESULT_ERROR) {
		pvt->_infobuffer.append(": error");
	} else if (result==RESULT_TIMEOUT) {
		pvt->_infobuffer.append(": timeout");
	} else if (result==RESULT_ABORT) {
		pvt->_infobuffer.append(": abort");
	}

	if (error::getErrorNumber()) {
		char	*error=error::getErrorString();
		pvt->_infobuffer.append(": ")->append((error)?error:"");
		delete[] error;
	}

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_ERROR,
					SQLREVENT_CLIENT_PROTOCOL_ERROR,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_CLIENT_PROTOCOL_ERROR,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseDbLogInEvent() {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_DB_LOGIN,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_DB_LOGIN,
					NULL);
	}
}

void sqlrservercontroller::raiseDbLogOutEvent() {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_DB_LOGOUT,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_DB_LOGOUT,
					NULL);
	}
}

void sqlrservercontroller::raiseDbErrorEvent(sqlrservercursor *cursor,
							const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_ERROR,
					SQLREVENT_DB_ERROR,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_DB_ERROR,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseDbWarningEvent(sqlrservercursor *cursor,
							const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_WARNING,
					SQLREVENT_DB_WARNING,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_DB_WARNING,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseQueryReceivedEvent(sqlrservercursor *cursor) {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_QUERY_RECEIVED,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_QUERY_RECEIVED,
					NULL);
	}
}

void sqlrservercontroller::raiseQueryPreparedEvent(sqlrservercursor *cursor) {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_QUERY_PREPARED,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_QUERY_PREPARED,
					NULL);
	}
}

void sqlrservercontroller::raiseQueryExecutedEvent(sqlrservercursor *cursor) {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_QUERY_EXECUTED,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_QUERY_EXECUTED,
					NULL);
	}
}

void sqlrservercontroller::raiseFilterViolationEvent(sqlrservercursor *cursor) {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_FILTER_VIOLATION,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_FILTER_VIOLATION,
					NULL);
	}
}

void sqlrservercontroller::raiseInternalErrorEvent(sqlrservercursor *cursor,
							const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}
	if (error::getErrorNumber()) {
		char	*error=error::getErrorString();
		pvt->_infobuffer.append(": ")->append((error)?error:"");
		delete[] error;
	}

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_ERROR,
					SQLREVENT_INTERNAL_ERROR,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_INTERNAL_ERROR,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseInternalWarningEvent(sqlrservercursor *cursor,
							const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	pvt->_infobuffer.append(info);
	if (error::getErrorNumber()) {
		char	*error=error::getErrorString();
		pvt->_infobuffer.append(": ")->append((error)?error:"");
		delete[] error;
	}
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_WARNING,
					SQLREVENT_INTERNAL_WARNING,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_INTERNAL_WARNING,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseScheduleViolationEvent(const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_WARNING,
					SQLREVENT_SCHEDULE_VIOLATION,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_SCHEDULE_VIOLATION,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseIntegrityViolationEvent(const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_ERROR,
					SQLREVENT_INTEGRITY_VIOLATION,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_INTEGRITY_VIOLATION,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseTranslationFailureEvent(
						sqlrservercursor *cursor,
						const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_ERROR,
					SQLREVENT_TRANSLATION_FAILURE,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_TRANSLATION_FAILURE,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseParseFailureEvent(
						sqlrservercursor *cursor,
						const char *info, ...) {

	if (!pvt->_sqlrlg && !pvt->_sqlrn) {
		return;
	}

	pvt->_infobuffer.clear();
	va_list	argp;
	va_start(argp,info);
	pvt->_infobuffer.printf(info,&argp);
	va_end(argp);

	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_ERROR,
					SQLREVENT_PARSE_FAILURE,
					pvt->_infobuffer.getString());
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_PARSE_FAILURE,
					pvt->_infobuffer.getString());
	}
}

void sqlrservercontroller::raiseCursorOpenEvent(sqlrservercursor *cursor) {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_CURSOR_OPEN,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_CURSOR_OPEN,
					NULL);
	}
}

void sqlrservercontroller::raiseCursorCloseEvent(sqlrservercursor *cursor) {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,cursor,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_CURSOR_CLOSE,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,cursor,
					SQLREVENT_CURSOR_CLOSE,
					NULL);
	}
}

void sqlrservercontroller::raiseBeginTransactionEvent() {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_BEGIN_TRANSACTION,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_BEGIN_TRANSACTION,
					NULL);
	}
}

void sqlrservercontroller::raiseCommitEvent() {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_COMMIT,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_COMMIT,
					NULL);
	}
}

void sqlrservercontroller::raiseRollbackEvent() {
	if (pvt->_sqlrlg) {
		pvt->_sqlrlg->write(NULL,pvt->_conn,NULL,
					SQLRLOGGER_LOGLEVEL_INFO,
					SQLREVENT_ROLLBACK,
					NULL);
	}
	if (pvt->_sqlrn) {
		pvt->_sqlrn->run(NULL,pvt->_conn,NULL,
					SQLREVENT_ROLLBACK,
					NULL);
	}
}

const char *sqlrservercontroller::getDbHostName() {
	if (!pvt->_dbhostname || !pvt->_conn->cacheDbHostInfo()) {
		pvt->_dbhostname=pvt->_conn->getDbHostName();
		pvt->_dbipaddress=pvt->_conn->getDbIpAddress();
	}
	return pvt->_dbhostname;
}

const char *sqlrservercontroller::getDbIpAddress() {
	if (!pvt->_dbipaddress || !pvt->_conn->cacheDbHostInfo()) {
		pvt->_dbhostname=pvt->_conn->getDbHostName();
		pvt->_dbipaddress=pvt->_conn->getDbIpAddress();
	}
	return pvt->_dbipaddress;
}

const char *sqlrservercontroller::getDbType() {
	return (pvt->_dbtype)?pvt->_dbtype:pvt->_conn->getDbType();
}

const char *sqlrservercontroller::getDbVersion() {
	return pvt->_conn->getDbVersion();
}

const char * const *sqlrservercontroller::getDatabaseFeatures() {
	return pvt->_conn->getDatabaseFeatures();
}

sqlrdatabaseobject *sqlrservercontroller::createDatabaseObject(
						const char *database,
						const char *schema,
						const char *object,
						const char *dependency) {

	// initialize copy pointers
	char	*databasecopy=NULL;
	char	*schemacopy=NULL;
	char	*objectcopy=NULL;
	char	*dependencycopy=NULL;

	// create buffers and copy data into them
	if (database) {
		databasecopy=(char *)pvt->_sessionpool.allocate(
				charstring::getLength(database)+1);
		charstring::copy(databasecopy,database);
	}
	if (schema) {
		schemacopy=(char *)pvt->_sessionpool.allocate(
				charstring::getLength(schema)+1);
		charstring::copy(schemacopy,schema);
	}
	if (object) {
		objectcopy=(char *)pvt->_sessionpool.allocate(
				charstring::getLength(object)+1);
		charstring::copy(objectcopy,object);
	}
	if (dependency) {
		dependencycopy=(char *)pvt->_sessionpool.allocate(
				charstring::getLength(dependency)+1);
		charstring::copy(dependencycopy,dependency);
	}

	// create the databaseobject
	sqlrdatabaseobject	*dbo=(sqlrdatabaseobject *)
			pvt->_sessionpool.allocate(sizeof(sqlrdatabaseobject));


	// populate it
	// (if placement new worked as expected on all platforms, we wouldn't
	// need to do this, we could pass them into the constructor or
	// use setters or something...)
	dbo->database=databasecopy;
	dbo->schema=schemacopy;
	dbo->object=objectcopy;
	dbo->dependency=dependencycopy;

	// return it
	return dbo;
}

void sqlrservercontroller::setReplacementTableName(
					const char *database,
					const char *schema,
					const char *oldtable,
					const char *newtable) {
	setReplacementName(&pvt->_tablenamemap,
				createDatabaseObject(
					database,
					schema,
					oldtable,
					NULL),
				newtable);
}

void sqlrservercontroller::setReplacementIndexName(
					const char *database,
					const char *schema,
					const char *oldindex,
					const char *newindex,
					const char *table) {
	setReplacementName(&pvt->_indexnamemap,
				createDatabaseObject(
					database,
					schema,
					oldindex,
					table),
				newindex);
}

void sqlrservercontroller::setReplacementName(
				dictionary< sqlrdatabaseobject *, char *> *dict,
				sqlrdatabaseobject *oldobject,
				const char *newobject) {
	dict->setValue(oldobject,(char *)newobject);
}

bool sqlrservercontroller::getReplacementTableName(const char *database,
						const char *schema,
						const char *oldtable,
						const char **newtable) {
	return getReplacementName(&pvt->_tablenamemap,
					database,schema,
					oldtable,newtable);
}

bool sqlrservercontroller::getReplacementIndexName(const char *database,
						const char *schema,
						const char *oldindex,
						const char **newindex) {
	return getReplacementName(&pvt->_indexnamemap,
					database,schema,
					oldindex,newindex);
}

bool sqlrservercontroller::getReplacementName(
				dictionary< sqlrdatabaseobject *, char *> *dict,
				const char *database,
				const char *schema,
				const char *oldobject,
				const char **newobject) {

	*newobject=NULL;
	for (listnode<sqlrdatabaseobject *> *node=dict->getKeys()->getFirst();
						node; node=node->getNext()) {

		sqlrdatabaseobject	*dbo=node->getValue();
		if (!charstring::compare(dbo->database,database) &&
			!charstring::compare(dbo->schema,schema) &&
			!charstring::compare(dbo->object,oldobject)) {
			*newobject=dict->getValue(dbo);
			return true;
		}
	}
	return false;
}

bool sqlrservercontroller::removeReplacementTable(const char *database,
							const char *schema,
							const char *table) {

	// remove the table
	if (!removeReplacement(&pvt->_tablenamemap,database,schema,table)) {
		return false;
	}

	// remove any indices that depend on the table
	for (listnode<sqlrdatabaseobject *> *node=
			pvt->_indexnamemap.getKeys()->getFirst(); node;) {

		sqlrdatabaseobject	*dbo=node->getValue();

		// make sure to move on to the next node here rather than
		// after calling remove, otherwise it could cause a
		// reference-after-free condition
		node=node->getNext();

		if (!charstring::compare(dbo->database,database) &&
			!charstring::compare(dbo->schema,schema) &&
			!charstring::compare(dbo->dependency,table)) {

			pvt->_indexnamemap.remove(dbo);
		}
	}
	return true;
}

bool sqlrservercontroller::removeReplacementIndex(const char *database,
							const char *schema,
							const char *index) {
	return removeReplacement(&pvt->_indexnamemap,database,schema,index);
}

bool sqlrservercontroller::removeReplacement(
				dictionary< sqlrdatabaseobject *, char *> *dict,
				const char *database,
				const char *schema,
				const char *object) {

	for (listnode<sqlrdatabaseobject *> *node=dict->getKeys()->getFirst();
						node; node=node->getNext()) {

		sqlrdatabaseobject	*dbo=node->getValue();
		const char		*replacementobject=dict->getValue(dbo);
		if (!charstring::compare(dbo->database,database) &&
			!charstring::compare(dbo->schema,schema) &&
			!charstring::compare(replacementobject,object)) {

			dict->remove(dbo);
			return true;
		}
	}
	return false;
}

const char *sqlrservercontroller::getId() {
	return pvt->_cmdl->getId();
}

const char *sqlrservercontroller::getConnectionId() {
	return pvt->_connectionid;
}

bool sqlrservercontroller::fetchFromBindCursor(sqlrservercursor *cursor) {

	// set state
	setState(PROCESS_SQL);

	debugStart("fetching from bind cursor");

	// reset flags
	cursor->setColumnInfoIsValid(false);
	cursor->setResultSetHeaderHasBeenHandled(false);

	// clear query buffer just so some future operation doesn't
	// get confused into thinking this cursor actually ran one
	cursor->getQueryBuffer()[0]='\0';
	cursor->setQuerySize(0);

	bool	success=cursor->fetchFromBindCursor();
	
	// reset total rows fetched
	cursor->clearTotalRowsFetched();

	if (success) {
		success=handleResultSetHeader(cursor);
	} else {
		// on failure save the error
		saveError(cursor);
	}

	debugWrite((success)?"success":"failed");
	debugEnd();

	return success;
}

bool sqlrservercontroller::nextResultSet(sqlrservercursor *cursor,
						bool *nextresultsetavailable) {

	debugStart("next result set");

	bool	success=cursor->nextResultSet(nextresultsetavailable);

	// on failure get the error (unless it's already been set)
	if (!success && !cursor->getErrorNumber()) {
		uint32_t	errorsize;
		int64_t		errnum;
		bool		liveconnection;
		getError(cursor,
			cursor->getErrorBuffer(),
			pvt->_maxerrorsize,
			&errorsize,&errnum,&liveconnection);
		cursor->setErrorSize(errorsize);
		cursor->setErrorNumber(errnum);
		cursor->setLiveConnection(liveconnection);
	}

	debugWrite((success)?"success":"failed");
	debugEnd();

	return success;
}

void sqlrservercontroller::saveError(sqlrservercursor *cursor) {

	// don't overwrite any message that's already been saved
	if (cursor->getErrorNumber() || cursor->getErrorSize()) {
		return;
	}

	// fetch the error into the per-cursor error buffers and flags
	uint32_t	errorsize;
	int64_t		errorcode;
	bool		liveconnection;
	cursor->getError(cursor->getErrorBuffer(),
				pvt->_cfg->getMaxErrorSize(),
				&errorsize,
				&errorcode,
				&liveconnection);
	cursor->setErrorSize(errorsize);
	cursor->setErrorNumber(errorcode);
	cursor->setLiveConnection(liveconnection);

	if (pvt->_debugsql) {
		stdoutput.printf("\n%d:%d:ERROR:\n%d:",
					process::getProcessId(),
					cursor->getId(),
					errorcode);
		stdoutput.write(cursor->getErrorBuffer(),errorsize);
		stdoutput.write('\n');
	}
}

void sqlrservercontroller::getError(sqlrservercursor *cursor,
						const char **errorbuffer,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection) {
	saveError(cursor);
	*errorbuffer=cursor->getErrorBuffer();
	*errorsize=cursor->getErrorSize();
	*errorcode=cursor->getErrorNumber();
	*liveconnection=cursor->getLiveConnection();

	if (pvt->_sqlret) {
		int64_t		tec=*errorcode;
		stringbuffer	translatederror;
		if (pvt->_sqlret->run(pvt->_conn,cursor,
						*errorcode,
						*errorbuffer,
						*errorsize,
						&tec,
						&translatederror)) {
			*errorcode=tec;
			charstring::safeCopy(cursor->getErrorBuffer(),
					cursor->getErrorBufferSize(),
					translatederror.getString(),
					translatederror.getSize());
			*errorsize=translatederror.getSize();
		}
		// FIXME: report error if this fails?
	}
}

void sqlrservercontroller::getError(sqlrservercursor *cursor,
						char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection) {

	// fetch the error
	const char	*errorstring=NULL;
	getError(cursor,&errorstring,errorsize,errorcode,liveconnection);

	// copy the error out
	charstring::safeCopy(errorbuffer,errorbuffersize,
				errorstring,*errorsize);
	if (*errorsize>errorbuffersize) {
		*errorsize=errorbuffersize;
	}
}

bool sqlrservercontroller::knowsRowCount(sqlrservercursor *cursor) {
	return cursor->knowsRowCount();
}

uint64_t sqlrservercontroller::rowCount(sqlrservercursor *cursor) {
	return cursor->rowCount();
}

bool sqlrservercontroller::knowsAffectedRows(sqlrservercursor *cursor) {
	return cursor->knowsAffectedRows();
}

void sqlrservercontroller::setAffectedRows(sqlrservercursor *cursor,
						uint64_t affectedrows) {
	pvt->_overrideaffectedrows.setValue(cursor,true);
	pvt->_affectedrows.setValue(cursor,affectedrows);
}

uint64_t sqlrservercontroller::getAffectedRows(sqlrservercursor *cursor) {
	return pvt->_affectedrows.getValue(cursor);
}

uint32_t sqlrservercontroller::colCount(sqlrservercursor *cursor) {

	// "db"cursor::prepareQuery() resets the column count to 0 before
	// preparing the query.  If the database knows the column count
	// post-prepare, then "db"cursor::prepareQuery() also sets it
	// immediately after preparing the query.
	//
	// But...
	//
	// If we're faking binds, then sqlrservercontroller::prepareQuery()
	// doesn't actually call "db"cursor::prepareQuery().  Instead,
	// sqlrservercontroller::executeQuery() calls it after faking the
	// binds.
	//
	// The app won't know that though, and if we're using a front end
	// (like the mysql fron end) which talks to the server after prepare,
	// then the app might do something like:
	//
	// * mysql_stmt_prepare(...)
	// * mysql_stmt_field_count(...)
	// * mysql_stmt_execute(...);
	//
	// ...expecting mysql_stmt_field_count to return the valid column count.
	//
	// Since we're faking binds though, and "db"cursor::prepareQuery()
	// hasn't actually been called by the time mysql_stmt_field_count is
	// called, then the column count returned by "db"cursor::colCount()
	// will still be the column count from the previous query.
	//
	// We can't just set it to 0 in closeResultSet because we want to be
	// able to access the column metadata after calling that.
	//
	// So, we'll handle it here.
	//
	// When the client prepares the query,
	// sqlrserverconnection::prepareQuery() will be called.  When it
	// executes the query, sqlrserverconnection::executeQuery() will be
	// called.  During that period, if bind-faking is enabled, then
	// cursor->getQueryHasBeenPrepared() will return false.  In that case,
	// return 0 for the column count.
	//
	// This results in the expected behavior when faking binds - the column
	// metadata isn't available until after execute.
	//
	// Arguably, the various getColumn*() methods should return 0 or NULL
	// as well at this time...
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return mapColumnCount(cursor->colCount());
}

uint16_t sqlrservercontroller::columnTypeFormat(sqlrservercursor *cursor) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return COLUMN_TYPE_IDS;
	}
	return cursor->columnTypeFormat();
}

const char *sqlrservercontroller::getColumnName(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return NULL;
	}
	if (pvt->_columnnamemap) {
		return pvt->_columnnamemap->getValue(col);
	}
	return cursor->getColumnNameFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnNameSize(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	if (pvt->_columnnamemap) {
		// FIXME: use a static map for these
		return charstring::getLength(
				pvt->_columnnamemap->getValue(col));
	}
	return cursor->getColumnNameSizeFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnType(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return UNKNOWN_DATATYPE;
	}
	return cursor->getColumnTypeFromBuffer(mapColumn(col));
}

const char *sqlrservercontroller::getColumnTypeName(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return NULL;
	}
	return cursor->getColumnTypeNameFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnTypeNameSize(sqlrservercursor *cursor,
								uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnTypeNameSizeFromBuffer(mapColumn(col));
}

uint32_t sqlrservercontroller::getColumnSize(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnSizeFromBuffer(mapColumn(col));
}

uint32_t sqlrservercontroller::getColumnPrecision(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnPrecisionFromBuffer(mapColumn(col));
}

uint32_t sqlrservercontroller::getColumnScale(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnScaleFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsNullable(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsNullableFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsPrimaryKey(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsPrimaryKeyFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsUnique(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsUniqueFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsPartOfKey(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsPartOfKeyFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsUnsigned(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsUnsignedFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsZeroFilled(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsZeroFilledFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsBinary(sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsBinaryFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnIsAutoIncrement(
						sqlrservercursor *cursor,
							uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnIsAutoIncrementFromBuffer(mapColumn(col));
}

const char *sqlrservercontroller::getColumnTable(sqlrservercursor *cursor,
								uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnTableFromBuffer(mapColumn(col));
}

uint16_t sqlrservercontroller::getColumnTableSize(sqlrservercursor *cursor,
								uint32_t col) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return 0;
	}
	return cursor->getColumnTableSizeFromBuffer(mapColumn(col));
}

bool sqlrservercontroller::getColumnNames(const char *query,
					stringbuffer *output) {

	// sanity check on the query
	if (charstring::isNullOrEmpty(query)) {
		return false;
	}

	size_t	querysize=charstring::getLength(query);

	sqlrservercursor	*gcncur=newCursor();
	if (open(gcncur) &&
		prepareQuery(gcncur,query,querysize) && executeQuery(gcncur)) {

		// build column list...
		getColumnNames(gcncur,output);
	}
	closeResultSet(gcncur);
	close(gcncur);
	deleteCursor(gcncur);
	return true;
}

void sqlrservercontroller::getColumnNames(sqlrservercursor *cursor,
							stringbuffer *output) {
	// see comment in colCount()
	if (!cursor->getColumnInfoIsValid()) {
		return;
	}
	for (uint32_t i=0; i<colCount(cursor); i++) {
		if (i) {
			output->append(',');
		}
		output->append(getColumnName(cursor,i),
				getColumnNameSize(cursor,i));
	}
}

bool sqlrservercontroller::handleResultSetHeader(sqlrservercursor *cursor) {

	// set flag indicating that the column info is now valid
	cursor->setColumnInfoIsValid(true);

	// This could get called multiple times, depending on whether
	// column info is valid post-prepare or post-execute.  It's easier
	// to just call it and bail if its already been called, than to
	// keep track of whether it needs to be called or not and not call
	// it if it doesn't.
	if (cursor->getResultSetHeaderHasBeenHandled()) {
		return true;
	}
	cursor->setResultSetHeaderHasBeenHandled(true);

	// set column map
	setColumnMap();

	// get arrays of field pointers,
	// helpfully provided for us by the cursor
	cursor->getColumnPointers(&(pvt->_columnnames),
				&(pvt->_columnnamesizes),
				&(pvt->_columntypes),
				&(pvt->_columntypenames),
				&(pvt->_columntypenamesizes),
				&(pvt->_columnsizes),
				&(pvt->_columnprecisions),
				&(pvt->_columnscales),
				&(pvt->_columnisnullables),
				&(pvt->_columnisprimarykeys),
				&(pvt->_columnisuniques),
				&(pvt->_columnispartofkeys),
				&(pvt->_columnisunsigneds),
				&(pvt->_columniszerofilleds),
				&(pvt->_columnisbinarys),
				&(pvt->_columnisautoincrements),
				&(pvt->_columntables),
				&(pvt->_columntablesizes));

	// remap columns
	uint32_t	colcount=colCount(cursor);
	for (uint32_t col=0; col<colcount; col++) {
		pvt->_columnnames[col]=
			cursor->getColumnName(mapColumn(col));
		pvt->_columnnamesizes[col]=
			cursor->getColumnNameSize(mapColumn(col));
		pvt->_columntypes[col]=
			cursor->getColumnType(mapColumn(col));
		pvt->_columntypenames[col]=
			cursor->getColumnTypeName(mapColumn(col));
		pvt->_columntypenamesizes[col]=
			cursor->getColumnTypeNameSize(mapColumn(col));
		pvt->_columnsizes[col]=
			cursor->getColumnSize(mapColumn(col));
		pvt->_columnprecisions[col]=
			cursor->getColumnPrecision(mapColumn(col));
		pvt->_columnscales[col]=
			cursor->getColumnScale(mapColumn(col));
		pvt->_columnisnullables[col]=
			cursor->getColumnIsNullable(mapColumn(col));
		pvt->_columnisprimarykeys[col]=
			cursor->getColumnIsPrimaryKey(mapColumn(col));
		pvt->_columnisuniques[col]=
			cursor->getColumnIsUnique(mapColumn(col));
		pvt->_columnispartofkeys[col]=
			cursor->getColumnIsPartOfKey(mapColumn(col));
		pvt->_columnisunsigneds[col]=
			cursor->getColumnIsUnsigned(mapColumn(col));
		pvt->_columniszerofilleds[col]=
			cursor->getColumnIsZeroFilled(mapColumn(col));
		pvt->_columnisbinarys[col]=
			cursor->getColumnIsBinary(mapColumn(col));
		pvt->_columnisautoincrements[col]=
			cursor->getColumnIsAutoIncrement(mapColumn(col));
		pvt->_columntables[col]=
			cursor->getColumnTable(mapColumn(col));
		pvt->_columntablesizes[col]=
			cursor->getColumnTableSize(mapColumn(col));
	}

	// translate columns
	if (pvt->_sqlrrsht && colcount) {

		if (pvt->_debugsqlrresultsetheadertranslation) {
			stdoutput.printf("\n===================="
				 	"===================="
				 	"===================="
				 	"===================\n\n");
			stdoutput.printf("translating result set header...\n");
		}

		if (!pvt->_sqlrrsht->run(pvt->_conn,
					cursor,colcount,
					&pvt->_columnnames,
					&pvt->_columnnamesizes,
					&pvt->_columntypes,
					&pvt->_columntypenames,
					&pvt->_columntypenamesizes,
					&pvt->_columnsizes,
					&pvt->_columnprecisions,
					&pvt->_columnscales,
					&pvt->_columnisnullables,
					&pvt->_columnisprimarykeys,
					&pvt->_columnisuniques,
					&pvt->_columnispartofkeys,
					&pvt->_columnisunsigneds,
					&pvt->_columniszerofilleds,
					&pvt->_columnisbinarys,
					&pvt->_columnisautoincrements,
					&pvt->_columntables,
					&pvt->_columntablesizes)) {
			setError(cursor,pvt->_sqlrrsht->getError(),
				SQLR_ERROR_RESULTSETHEADERTRANSLATION,true);
			return false;
		}
	}
	return true;
}

bool sqlrservercontroller::noRowsToReturn(sqlrservercursor *cursor) {
	return cursor->noRowsToReturn();
}

bool sqlrservercontroller::skipRow(sqlrservercursor *cursor, bool *error) {
	return cursor->skipRow(error);
}

bool sqlrservercontroller::fetchRow(sqlrservercursor *cursor, bool *error) {

	// initialize error
	*error=false;

	// get arrays of field pointers,
	// helpfully provided for us by the cursor
	cursor->getFieldPointers(&(pvt->_fieldnames),
					&(pvt->_fields),
					&(pvt->_fieldsizes),
					&(pvt->_lobs),
					&(pvt->_nulls));

	// get the column count
	// NOTE: Don't just set colcount=colCount(cursor) here.  If a column
	// map is being used, then it returns the column count of the map, which
	// could be smaller than the actual column count.  We need to be sure we
	// fetch all columns, so we need to use the raw column count here.
	uint32_t	colcount=(cursor->getColumnInfoIsValid())?
						cursor->colCount():0;

	// for timings...
	datetime	dt;

	if (pvt->_sqlrrsrbt) {

		// if we have row block translations, then
		// this is a little complex...

		// if we're on the first row of a block...
		if (!(cursor->getTotalRowsFetched()%
				pvt->_sqlrrsrbt->getRowBlockCount())) {

			if (pvt->_debugsqlrresultsetrowblocktranslation) {
				stdoutput.printf("\n===================="
				 		"===================="
				 		"===================="
				 		"===================\n\n");
				stdoutput.printf("translating result "
						"set row block...\n");
			}

			// for each row in the block...
			for (uint32_t j=0;
				j<pvt->_sqlrrsrbt->getRowBlockCount(); j++) {

				// fetch the row, bail if fetch failed
				if (!cursor->fetchRow(error)) {
					break;
				}

				// handle errors
				if (*error) {
					return false;
				}

				// use the provided field pointer arrays to get
				// the pointers to the column names and actual
				// field data
				for (uint32_t i=0; i<colcount; i++) {

					pvt->_fieldnames[i]=
						getColumnName(cursor,i);
					pvt->_fields[i]=NULL;
					pvt->_fieldsizes[i]=0;
					pvt->_lobs[i]=false;
					pvt->_nulls[i]=false;
					cursor->getField(i,
						&(pvt->_fields[i]),
						&(pvt->_fieldsizes[i]),
						&(pvt->_lobs[i]),
						&(pvt->_nulls[i]));

					// A connection module might return the
					// actual field size, even if its
					// larger than the buffer that the data
					// was copied into.  Override
					// fieldsize, if necessary, just to
					// be safe.
					if (pvt->_maxfieldsize &&
						pvt->_fieldsizes[i]>
							pvt->_maxfieldsize) {
						pvt->_fieldsizes[i]=
							pvt->_maxfieldsize;
					}
				}

				// send the row to the translators
				if (!pvt->_sqlrrsrbt->setRow(
							cursor->conn,
							cursor,
							colcount,
							pvt->_fieldnames,
							pvt->_fields,
							pvt->_fieldsizes,
							pvt->_lobs,
							pvt->_nulls)) {
					*error=true;
					setError(cursor,
						pvt->_sqlrrsrbt->getError(),
					SQLR_ERROR_RESULTSETROWBLOCKTRANSLATION,
						true);
					return false;
				}
			}

			// run the translators
			if (!pvt->_sqlrrsrbt->run(cursor->conn,cursor,
						colcount,pvt->_fieldnames)) {
				*error=true;
				setError(cursor,pvt->_sqlrrsrbt->getError(),
					SQLR_ERROR_RESULTSETROWBLOCKTRANSLATION,
					true);
				return false;
			}
		}

		// get the row from the translators
		if (!pvt->_sqlrrsrbt->getRow(cursor->conn,cursor,
						colcount,
						&(pvt->_fields),
						&(pvt->_fieldsizes),
						&(pvt->_lobs),
						&(pvt->_nulls))) {
			if (pvt->_sqlrrsrbt->getError()) {
				*error=true;
				setError(cursor,pvt->_sqlrrsrbt->getError(),
					SQLR_ERROR_RESULTSETROWBLOCKTRANSLATION,
					true);
			}
			return false;
		}

	} else {

		// if we don't have any row block translations, then
		// this is a little more straightforward...

		// fetch the row, bail if fetch failed
		if (!cursor->fetchRow(error)) {
			return false;
		}

		// handle errors
		if (*error) {
			return false;
		}

		// use the provided field pointer arrays to get the
		// pointers to the column names and actual field data
		for (uint32_t i=0; i<colcount; i++) {

			pvt->_fieldnames[i]=getColumnName(cursor,i);
			pvt->_fields[i]=NULL;
			pvt->_fieldsizes[i]=0;
			pvt->_lobs[i]=false;
			pvt->_nulls[i]=false;
			cursor->getField(i,&(pvt->_fields[i]),
						&(pvt->_fieldsizes[i]),
						&(pvt->_lobs[i]),
						&(pvt->_nulls[i]));

			// A connection module might return the actual field
			// size, even if its larger than the buffer that the
			// data was copied into.  Override fieldsize, if
			// necessary, just to be safe.
			if (pvt->_maxfieldsize &&
				pvt->_fieldsizes[i]>pvt->_maxfieldsize) {
				pvt->_fieldsizes[i]=pvt->_maxfieldsize;
			}
		}
	}

	// reformat the row
	if (!reformatRow(cursor,colcount,pvt->_fieldnames,
				&(pvt->_fields),&(pvt->_fieldsizes))) {
		*error=true;
		return false;
	}

	// bump total rows fetched
	cursor->incrementTotalRowsFetched();

	return true;
}

void sqlrservercontroller::nextRow(sqlrservercursor *cursor) {
	cursor->nextRow();
}

bool sqlrservercontroller::getField(sqlrservercursor *cursor,
						uint32_t col,
						const char **field,
						uint64_t *fieldsize,
						bool *lob,
						bool *null) {

	// return the requested field (which these pointers
	// were set to during the previous call to fetchRow)
	uint32_t	actualcol=mapColumn(col);
	*field=pvt->_fields[actualcol];
	*fieldsize=pvt->_fieldsizes[actualcol];
	*lob=pvt->_lobs[actualcol];
	*null=pvt->_nulls[actualcol];

	// reformat the field
	return reformatField(cursor,pvt->_fieldnames[col],
					col,field,fieldsize);
}

bool sqlrservercontroller::getLobFieldLength(sqlrservercursor *cursor,
							uint32_t col,
							uint64_t *length) {
	return cursor->getLobFieldLength(mapColumn(col),length);
}

bool sqlrservercontroller::getLobFieldSegment(sqlrservercursor *cursor,
							uint32_t col,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread) {
	return cursor->getLobFieldSegment(mapColumn(col),buffer,buffersize,
						offset,charstoread,charsread);
}

void sqlrservercontroller::closeLobField(sqlrservercursor *cursor,
							uint32_t col) {
	cursor->closeLobField(mapColumn(col));
}

void sqlrservercontroller::suspendResultSet(sqlrservercursor *cursor) {
	cursor->setState(SQLRCURSORSTATE_SUSPENDED);
	if (cursor->getCustomQueryCursor()) {
		cursor->getCustomQueryCursor()->
			setState(SQLRCURSORSTATE_SUSPENDED);
	}
}

void sqlrservercontroller::closeResultSet(sqlrservercursor *cursor) {
	cursor->closeResultSet();
	if (pvt->_sqlrmd) {
		pvt->_sqlrmd->closeResultSet(cursor);
	}
}

uint16_t sqlrservercontroller::getId(sqlrservercursor *cursor) {
	return cursor->getId();
}

memorypool *sqlrservercontroller::getBindPool(sqlrservercursor *cursor) {
	return cursor->getBindPool();
}

memorypool *sqlrservercontroller::getBindMappingsPool(
						sqlrservercursor *cursor) {
	return cursor->getBindMappingsPool();
}

dictionary<char *, char *> *sqlrservercontroller::getBindMappings(
						sqlrservercursor *cursor) {
	return cursor->getBindMappings();
}


bool sqlrservercontroller::copyBinds(sqlrservercursor *sourcecur,
						sqlrservercursor *destcur) {
	return copyInputBinds(sourcecur,destcur) &&
		copyOutputBinds(sourcecur,destcur) &&
		copyInputOutputBinds(sourcecur,destcur);
	
}

bool sqlrservercontroller::copyInputBinds(sqlrservercursor *sourcecur,
						sqlrservercursor *destcur) {

	uint16_t		srcindex=0;
	uint16_t		srccount=getInputBindCount(sourcecur);
	sqlrserverbindvar	*srcvars=getInputBinds(sourcecur);
	uint16_t		destindex=getInputBindCount(destcur);
	uint16_t		destmax=getConfig()->getMaxBindCount();
	sqlrserverbindvar	*destvars=getInputBinds(destcur);
	memorypool		*destpool=getBindPool(destcur);

	while (srcindex<srccount && destindex<destmax) {
		copyBind(&(srcvars[srcindex]),&(destvars[destindex]),destpool);
		srcindex++;
		destindex++;
	}
	setInputBindCount(destcur,destindex);

	return srcindex==srccount;
}

bool sqlrservercontroller::copyOutputBinds(sqlrservercursor *sourcecur,
						sqlrservercursor *destcur) {

	uint16_t		srcindex=0;
	uint16_t		srccount=getOutputBindCount(sourcecur);
	sqlrserverbindvar	*srcvars=getOutputBinds(sourcecur);
	uint16_t		destindex=getOutputBindCount(destcur);
	uint16_t		destmax=getConfig()->getMaxBindCount();
	sqlrserverbindvar	*destvars=getOutputBinds(destcur);
	memorypool		*destpool=getBindPool(destcur);

	while (srcindex<srccount && destindex<destmax) {
		copyBind(&(srcvars[srcindex]),&(destvars[destindex]),destpool);
		srcindex++;
		destindex++;
	}
	setOutputBindCount(destcur,destindex);

	return srcindex==srccount;
}

bool sqlrservercontroller::copyInputOutputBinds(sqlrservercursor *sourcecur,
						sqlrservercursor *destcur) {

	uint16_t 		srcindex=0;
	uint16_t		srccount=getInputOutputBindCount(sourcecur);
	sqlrserverbindvar	*srcvars=getInputOutputBinds(sourcecur);
	uint16_t		destindex=getInputOutputBindCount(destcur);
	uint16_t		destmax=getConfig()->getMaxBindCount();
	sqlrserverbindvar	*destvars=getInputOutputBinds(destcur);
	memorypool		*destpool=getBindPool(destcur);

	while (srcindex<srccount && destindex<destmax) {
		copyBind(&(srcvars[srcindex]),&(destvars[destindex]),destpool);
		srcindex++;
		destindex++;
	}
	setInputOutputBindCount(destcur,destindex);

	return srcindex==srccount;
}

void sqlrservercontroller::copyBind(sqlrserverbindvar *source,
						sqlrserverbindvar *dest,
						memorypool *destpool) {

	// byte-copy everything
	bytestring::copy(dest,source,sizeof(sqlrserverbindvar));

	// (re)copy variable name
	dest->variablesize=source->variablesize;
	dest->variable=(char *)destpool->allocate(dest->variablesize+1);
	charstring::copy(dest->variable,source->variable);
	
	// (re)copy strings
	if (source->type==SQLRSERVERBINDVARTYPE_STRING) {
		dest->value.stringval=
			(char *)destpool->allocate(source->valuesize+1);
		charstring::copy(dest->value.stringval,
					source->value.stringval);
	} else if (source->type==SQLRSERVERBINDVARTYPE_DATE) {
		dest->value.dateval.tz=
			(char *)destpool->allocate(
				charstring::getLength(
					source->value.dateval.tz)+1);
		charstring::copy(dest->value.dateval.tz,
					source->value.dateval.tz);
	}
}

void sqlrservercontroller::setFakeInputBindsForThisQuery(
					sqlrservercursor *cursor, bool fake) {
	cursor->setFakeInputBindsForThisQuery(fake);
}

bool sqlrservercontroller::getFakeInputBindsForThisQuery(
					sqlrservercursor *cursor) {
	return cursor->getFakeInputBindsForThisQuery();
}

void sqlrservercontroller::setInputBindCount(sqlrservercursor *cursor,
						uint16_t inbindcount) {
	cursor->setInputBindCount(inbindcount);
}

uint16_t sqlrservercontroller::getInputBindCount(sqlrservercursor *cursor) {
	return cursor->getInputBindCount();
}

sqlrserverbindvar *sqlrservercontroller::getInputBinds(
						sqlrservercursor *cursor) {
	return cursor->getInputBinds();
}

void sqlrservercontroller::setOutputBindCount(sqlrservercursor *cursor,
						uint16_t outbindcount) {
	cursor->setOutputBindCount(outbindcount);
}

uint16_t sqlrservercontroller::getOutputBindCount(sqlrservercursor *cursor) {
	return cursor->getOutputBindCount();
}

sqlrserverbindvar *sqlrservercontroller::getOutputBinds(
						sqlrservercursor *cursor) {
	return cursor->getOutputBinds();
}

bool sqlrservercontroller::getLobOutputBindLength(sqlrservercursor *cursor,
							uint16_t index,
							uint64_t *length) {
	return cursor->getLobOutputBindLength(index,length);
}

bool sqlrservercontroller::getLobOutputBindSegment(sqlrservercursor *cursor,
							uint16_t index,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread) {
	return cursor->getLobOutputBindSegment(index,buffer,buffersize,
						offset,charstoread,charsread);
}

void sqlrservercontroller::closeLobOutputBind(sqlrservercursor *cursor,
							uint16_t index) {
	cursor->closeLobOutputBind(index);
}

void sqlrservercontroller::setInputOutputBindCount(sqlrservercursor *cursor,
						uint16_t outbindcount) {
	cursor->setInputOutputBindCount(outbindcount);
}

uint16_t sqlrservercontroller::getInputOutputBindCount(
						sqlrservercursor *cursor) {
	return cursor->getInputOutputBindCount();
}

sqlrserverbindvar *sqlrservercontroller::getInputOutputBinds(
						sqlrservercursor *cursor) {
	return cursor->getInputOutputBinds();
}

bool sqlrservercontroller::getLobInputOutputBindLength(
						sqlrservercursor *cursor,
						uint16_t index,
						uint64_t *length) {
	return cursor->getLobInputOutputBindLength(index,length);
}

bool sqlrservercontroller::getLobInputOutputBindSegment(
						sqlrservercursor *cursor,
						uint16_t index,
						char *buffer,
						uint64_t buffersize,
						uint64_t offset,
						uint64_t charstoread,
						uint64_t *charsread) {
	return cursor->getLobInputOutputBindSegment(index,buffer,buffersize,
						offset,charstoread,charsread);
}

void sqlrservercontroller::closeLobInputOutputBind(sqlrservercursor *cursor,
							uint16_t index) {
	cursor->closeLobInputOutputBind(index);
}

bool sqlrservercontroller::open(sqlrservercursor *cursor) {
	raiseCursorOpenEvent(cursor);
	return cursor->open();
}

bool sqlrservercontroller::close(sqlrservercursor *cursor) {
	raiseCursorCloseEvent(cursor);
	return cursor->close();
}

void sqlrservercontroller::abort(sqlrservercursor *cursor) {
	cursor->abort();
}

char *sqlrservercontroller::getQueryBuffer(sqlrservercursor *cursor) {
	return cursor->getQueryBuffer();
}

void sqlrservercontroller::setQuerySize(sqlrservercursor *cursor,
						uint32_t querysize) {
	cursor->setQuerySize(querysize);
}

uint32_t  sqlrservercontroller::getQuerySize(sqlrservercursor *cursor) {
	return cursor->getQuerySize();
}

void sqlrservercontroller::setQueryStatus(sqlrservercursor *cursor,
						sqlrquerystatus_t status) {
	cursor->setQueryStatus(status);
}

sqlrquerystatus_t sqlrservercontroller::getQueryStatus(
						sqlrservercursor *cursor) {
	return cursor->getQueryStatus();
}

void sqlrservercontroller::setQueryTree(sqlrservercursor *cursor,
							xmldom *tree) {
	cursor->setQueryTree(tree);
}

xmldom *sqlrservercontroller::getQueryTree(sqlrservercursor *cursor) {
	return cursor->getQueryTree();
}

void sqlrservercontroller::clearQueryTree(sqlrservercursor *cursor) {
	cursor->clearQueryTree();
}

stringbuffer *sqlrservercontroller::getTranslatedQueryBuffer(
					sqlrservercursor *cursor) {
	return cursor->getTranslatedQueryBuffer();
}

const char *sqlrservercontroller::getTranslatedQuery(
					sqlrservercursor *cursor) {
	return getTranslatedQueryBuffer(cursor)->getString();
}

void sqlrservercontroller::setCommandStart(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec) {
	cursor->setCommandStart(sec,usec);
}

uint64_t sqlrservercontroller::getCommandStartSec(sqlrservercursor *cursor) {
	return cursor->getCommandStartSec();
}

uint64_t sqlrservercontroller::getCommandStartUSec(sqlrservercursor *cursor) {
	return cursor->getCommandStartUSec();
}

void sqlrservercontroller::setCommandEnd(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec) {
	cursor->setCommandEnd(sec,usec);
}

uint64_t sqlrservercontroller::getCommandEndSec(sqlrservercursor *cursor) {
	return cursor->getCommandEndSec();
}

uint64_t sqlrservercontroller::getCommandEndUSec(sqlrservercursor *cursor) {
	return cursor->getCommandEndUSec();
}

void sqlrservercontroller::setQueryStart(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec) {
	cursor->setQueryStart(sec,usec);
}

uint64_t sqlrservercontroller::getQueryStartSec(sqlrservercursor *cursor) {
	return cursor->getQueryStartSec();
}

uint64_t sqlrservercontroller::getQueryStartUSec(sqlrservercursor *cursor) {
	return cursor->getQueryStartUSec();
}

void sqlrservercontroller::setQueryEnd(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec) {
	cursor->setQueryEnd(sec,usec);
}

uint64_t sqlrservercontroller::getQueryEndSec(sqlrservercursor *cursor) {
	return cursor->getQueryEndSec();
}

uint64_t sqlrservercontroller::getQueryEndUSec(sqlrservercursor *cursor) {
	return cursor->getQueryEndUSec();
}

void sqlrservercontroller::setState(sqlrservercursor *cursor,
						sqlrcursorstate_t state) {
	cursor->setState(state);
}

sqlrcursorstate_t sqlrservercontroller::getState(sqlrservercursor *cursor) {
	return cursor->getState();
}

uint64_t sqlrservercontroller::getTotalRowsFetched(sqlrservercursor *cursor) {
	return cursor->getTotalRowsFetched();
}

void sqlrservercontroller::clearError(sqlrservercursor *cursor) {
	setError(cursor,NULL,0,true);
}

void sqlrservercontroller::setError(sqlrservercursor *cursor,
						const char *err,
						uint32_t errsize,
						int64_t errn,
						bool liveconn) {

	char		*errorbuffer=cursor->getErrorBuffer();
	if (errsize>pvt->_maxerrorsize) {
		errsize=pvt->_maxerrorsize;
	}
	charstring::safeCopy(errorbuffer,pvt->_maxerrorsize,err,errsize);
	if (errsize<pvt->_maxerrorsize) {
		errorbuffer[errsize]='\0';
	}
	cursor->setErrorSize(errsize);
	cursor->setErrorNumber(errn);
	cursor->setLiveConnection(liveconn);
}

void sqlrservercontroller::setError(sqlrservercursor *cursor,
						const char *err,
						int64_t errn,
						bool liveconn) {
	setError(cursor,err,charstring::getLength(err),errn,liveconn);
}

void sqlrservercontroller::copyConnectionErrorToCursor(
					sqlrservercursor *cursor) {
	setError(cursor,pvt->_conn->getErrorBuffer(),
			pvt->_conn->getErrorSize(),
			pvt->_conn->getErrorNumber(),
			pvt->_conn->getLiveConnection());
}

char *sqlrservercontroller::getErrorBuffer(sqlrservercursor *cursor) {
	return cursor->getErrorBuffer();
}

uint32_t sqlrservercontroller::getErrorBufferSize(sqlrservercursor *cursor) {
	return cursor->getErrorBufferSize();
}

uint32_t sqlrservercontroller::getErrorSize(sqlrservercursor *cursor) {
	return cursor->getErrorSize();
}

void sqlrservercontroller::setErrorSize(sqlrservercursor *cursor,
						uint32_t errorsize) {
	cursor->setErrorSize(errorsize);
}

uint32_t sqlrservercontroller::getErrorNumber(sqlrservercursor *cursor) {
	return cursor->getErrorNumber();
}

void sqlrservercontroller::setErrorNumber(sqlrservercursor *cursor,
						uint32_t errnum) {
	cursor->setErrorNumber(errnum);
}

bool sqlrservercontroller::getLiveConnection(sqlrservercursor *cursor) {
	return cursor->getLiveConnection();
}

void sqlrservercontroller::setLiveConnection(sqlrservercursor *cursor,
						bool liveconnection) {
	cursor->setLiveConnection(liveconnection);
}

memorypool *sqlrservercontroller::getPerSessionMemoryPool() {
	return &pvt->_sessionpool;
}

memorypool *sqlrservercontroller::getPerTransactionMemoryPool() {
	return &pvt->_txpool;
}

sqlrparser *sqlrservercontroller::getParser() {
	return pvt->_sqlrp;
}

gsscontext *sqlrservercontroller::getGssContext() {
	return (pvt->_currentprotocol)?
			pvt->_currentprotocol->getGssContext():NULL;
}

tlscontext *sqlrservercontroller::getTlsContext() {
	return (pvt->_currentprotocol)?
			pvt->_currentprotocol->getTlsContext():NULL;
}

sqlrconfig *sqlrservercontroller::getConfig() {
	return pvt->_cfg;
}

sqlrpaths *sqlrservercontroller::getPaths() {
	return pvt->_pth;
}

sqlrshm *sqlrservercontroller::getShm() {
	return pvt->_shm;
}

sqlrmoduledata *sqlrservercontroller::getModuleData(const char *id) {
	return (pvt->_sqlrmd)?pvt->_sqlrmd->getModuleData(id):NULL;
}

bool sqlrservercontroller::send(byte_t *data, size_t size) {
	return pvt->_conn->send(data,size);
}

bool sqlrservercontroller::recv(byte_t **data, size_t *size) {
	return pvt->_conn->recv(data,size);
}

// Copyright (c) David Muse
// See the file COPYING for more information

#include <config.h>

#include <sqlrelay/sqlrserver.h>

#include <rudiments/stringbuffer.h>
#include <rudiments/memorypool.h>
#include <rudiments/datetime.h>
#include <rudiments/userentry.h>
#include <rudiments/process.h>
#include <rudiments/file.h>

#include <datatypes.h>
#include <defaults.h>
#include <defines.h>

enum sqlrclientquerytype_t {
	SQLRCLIENTQUERYTYPE_QUERY=0,
	SQLRCLIENTQUERYTYPE_DATABASE_LIST,
	SQLRCLIENTQUERYTYPE_CATALOG_LIST,
	SQLRCLIENTQUERYTYPE_SCHEMA_LIST,
	SQLRCLIENTQUERYTYPE_TABLE_LIST,
	SQLRCLIENTQUERYTYPE_TABLE_LIST_2,
	SQLRCLIENTQUERYTYPE_TABLE_TYPE_LIST,
	SQLRCLIENTQUERYTYPE_COLUMN_LIST,
	SQLRCLIENTQUERYTYPE_PRIMARY_KEYS_LIST,
	SQLRCLIENTQUERYTYPE_KEY_AND_INDEX_LIST,
	SQLRCLIENTQUERYTYPE_PROCEDURE_PARAMETER_LIST,
	SQLRCLIENTQUERYTYPE_TYPE_INFO_LIST,
	SQLRCLIENTQUERYTYPE_PROCEDURE_LIST,
	SQLRCLIENTQUERYTYPE_LAST_INSERT_ID_LIST
};

class SQLRSERVER_DLLSPEC sqlrprotocol_sqlrclient : public sqlrprotocol {
	public:
		sqlrprotocol_sqlrclient(sqlrservercontroller *cont,
							domnode *parameters);
		virtual	~sqlrprotocol_sqlrclient();

		clientsessionexitstatus_t	clientSession(
							filedescriptor *cs);

	private:
		bool	acceptSecurityContext();
		bool	getCommand(uint16_t *command);
		sqlrservercursor	*getCursor(uint16_t command);
		void	noAvailableCursors(uint16_t command);
		void	sendNotAuthenticatedError();
		void	sendUnsupportedProtocolError();
		bool	authCommand();
		bool	getUserFromClient();
		bool	getPasswordFromClient();
		void	suspendSessionCommand();
		void	pingCommand();
		void	identifyCommand();
		void	setAutoCommitCommand();
		void	getAutoCommitCommand();
		void	beginCommand();
		void	commitCommand();
		void	rollbackCommand();
		void	getInTransactionCommand();
		void	dbVersionCommand();
		void	bindFormatCommand();
		void	getNextvalFormatCommand();
		void	serverVersionCommand();
		void	selectDatabaseCommand();
		void	getCurrentDatabaseCommand();
		void	getDatabaseIsSchemaCommand();
		void	selectCatalogCommand();
		void	getCurrentCatalogCommand();
		void	selectSchemaCommand();
		void	getCurrentSchemaCommand();
		void	getCurrentUserCommand();
		void	getLastInsertIdCommand();
		void	dbHostNameCommand();
		void	dbIpAddressCommand();
		void	setIsolationLevelCommand();
		void	getIsolationLevelCommand();
		void	getDefaultIsolationLevelCommand();
		void	setTransactionModelCommand();
		void	getTransactionModelCommand();
		void	getDefaultTransactionModelCommand();
		void	getDatabaseFeaturesCommand();
		bool	newQueryCommand(sqlrservercursor *cursor);
		bool	reExecuteQueryCommand(sqlrservercursor *cursor);
		bool	fetchFromBindCursorCommand(sqlrservercursor *cursor);
		bool	processQueryOrBindCursor(sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype,
					sqlrserverlistformat_t listformat,
					bool reexecute,
					bool bindcursor);
		bool	getClientInfo(sqlrservercursor *cursor);
		bool	getQuery(sqlrservercursor *cursor);
		bool	getInputBinds(sqlrservercursor *cursor);
		bool	getOutputBinds(sqlrservercursor *cursor);
		bool	getInputOutputBinds(sqlrservercursor *cursor);
		bool	getBindVarCount(sqlrservercursor *cursor,
						uint16_t *count);
		bool	getBindVarName(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						memorypool *bindpool);
		bool	getBindVarType(sqlrserverbindvar *bv);
		bool	getBindSize(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						uint32_t *maxsize);
		void	getNullBind(sqlrserverbindvar *bv,
						memorypool *bindpool);
		bool	getStringBind(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						memorypool *bindpool);
		bool	getIntegerBind(sqlrserverbindvar *bv);
		bool	getDoubleBind(sqlrserverbindvar *bv);
		bool	getDateBind(sqlrserverbindvar *bv,
						memorypool *bindpool);
		bool	getLobBind(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						memorypool *bindpool);
		bool	getSendColumnInfo();
		bool	getSkipAndFetch(bool initial, sqlrservercursor *cursor);
		void	returnResultSetHeader(sqlrservercursor *cursor,
							bool sendbindvalues);
		void	returnColumnInfo(sqlrservercursor *cursor,
							uint16_t format);
		uint16_t	protocolAppropriateColumnType(uint16_t coltype);
		void	sendRowCounts(bool knowsactual, uint64_t actual,
					bool knowsaffected, uint64_t affected);
		void	returnOutputBindValues(sqlrservercursor *cursor,
							bool sendbindvalues);
		void	returnOutputBindBlob(sqlrservercursor *cursor,
							uint16_t index);
		void	returnOutputBindClob(sqlrservercursor *cursor,
							uint16_t index);
		void	sendLobOutputBind(sqlrservercursor *cursor,
							uint16_t index);
		void	returnInputOutputBindValues(sqlrservercursor *cursor,
							bool sendbindvalues);
		void	sendColumnDefinition(const char *name,
						uint16_t namesize,
						uint16_t type, 
						uint32_t size,
						uint32_t precision,
						uint32_t scale,
						uint16_t nullable,
						uint16_t primarykey,
						uint16_t unique,
						uint16_t partofkey,
						uint16_t unsignednumber,
						uint16_t zerofill,
						uint16_t binary,
						uint16_t autoincrement,
						const char *table,
						uint16_t tablesize);
		void	sendColumnDefinitionString(const char *name,
						uint16_t namesize,
						const char *type, 
						uint16_t typesize,
						uint32_t size,
						uint32_t precision,
						uint32_t scale,
						uint16_t nullable,
						uint16_t primarykey,
						uint16_t unique,
						uint16_t partofkey,
						uint16_t unsignednumber,
						uint16_t zerofill,
						uint16_t binary,
						uint16_t autoincrement,
						const char *table,
						uint16_t tablesize);
		bool	returnResultSetData(sqlrservercursor *cursor,
						bool getskipandfetch,
						bool overridelazyfetch);
		void	returnFetchError(sqlrservercursor *cursor);
		void	returnRow(sqlrservercursor *cursor);
		void	sendField(const char *data, uint32_t size);
		void	sendNullField();
		void	sendLobField(sqlrservercursor *cursor, uint32_t col);
		void	startSendingLong(uint64_t longsize);
		void	sendLongSegment(const char *data, uint32_t size);
		void	endSendingLong();
		void	returnError(bool forcedisconnect);
		void	returnError(sqlrservercursor *cursor,
						bool forcedisconnect);
		bool	fetchResultSetCommand(sqlrservercursor *cursor);
		void	abortResultSetCommand(sqlrservercursor *cursor);
		void	suspendResultSetCommand(sqlrservercursor *cursor);
		bool	resumeResultSetCommand(sqlrservercursor *cursor);
		bool	getDatabaseListCommand(sqlrservercursor *cursor);
		bool	getCatalogListCommand(sqlrservercursor *cursor);
		bool	getSchemaListCommand(sqlrservercursor *cursor);
		bool	getTableListCommand(sqlrservercursor *cursor);
		bool	getTableList2Command(sqlrservercursor *cursor);
		bool	getTableTypeListCommand(sqlrservercursor *cursor);
		bool	getColumnListCommand(sqlrservercursor *cursor);
		bool	getPrimaryKeysListCommand(sqlrservercursor *cursor);
		bool	getKeyAndIndexListCommand(sqlrservercursor *cursor);
		bool	getProcedureParameterListCommand(
						sqlrservercursor *cursor);
		bool	getTypeInfoListCommand(sqlrservercursor *cursor);
		bool	getProcedureListCommand(sqlrservercursor *cursor);
		bool	getLastInsertIdListCommand(sqlrservercursor *cursor);
		bool	getObjectListCommand(sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype);
		bool	getObjectList(sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype,
					const char *object,
					sqlrserverlistformat_t listformat,
					uint16_t objecttypes);
		bool	getComponentListCommand(sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype);
		bool	getComponentList(sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype,
					const char *object,
					const char *wild,
					sqlrserverlistformat_t listformat);
		void	escapeParameter(stringbuffer *buffer,
						const char *parameter);
		bool	getQueryTreeCommand(sqlrservercursor *cursor);
		bool	getTranslatedQueryCommand(sqlrservercursor *cursor);
		bool	nextResultSetCommand(sqlrservercursor *cursor);

		void	debugCommand(uint16_t command);
		void	debugListFormat(sqlrserverlistformat_t listformat);

		stringbuffer	debugstr;

		filedescriptor	*clientsock;

		securitycontext	*ctx;

		int32_t		idleclienttimeout;

		uint64_t	maxclientinfosize;
		uint32_t	maxquerysize;
		uint16_t	maxbindcount;
		uint16_t	maxbindnamesize;
		uint32_t	maxstringbindvaluesize;
		uint32_t	maxlobbindvaluesize;
		bool		waitfordowndb;

		char		userbuffer[USERSIZE];
		char		passwordbuffer[USERSIZE];

		char		*clientinfo;
		uint64_t	clientinfosize;

		uint64_t	skip;
		uint64_t	fetch;
		bool		lazyfetch;

		char		lobbuffer[32768];

		uint16_t	protocolversion;
		uint16_t	endresultset;

		bool		authenticated;
};

sqlrprotocol_sqlrclient::sqlrprotocol_sqlrclient(
					sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrprotocol(cont,parameters) {

	idleclienttimeout=cont->getConfig()->getIdleClientTimeout();
	maxclientinfosize=cont->getConfig()->getMaxClientInfoSize();
	maxquerysize=cont->getConfig()->getMaxQuerySize();
	maxbindcount=cont->getConfig()->getMaxBindCount();
	maxbindnamesize=cont->getConfig()->getMaxBindNameSize();
	maxstringbindvaluesize=
			cont->getConfig()->getMaxStringBindValueSize();
	maxlobbindvaluesize=cont->getConfig()->getMaxLobBindValueSize();
	lazyfetch=false;
	waitfordowndb=cont->getConfig()->getWaitForDownDatabase();
	clientinfo=new char[maxclientinfosize+1];
	clientsock=NULL;
	authenticated=false;

	if (useKrb()) {
		ctx=getGssContext();
	} else if (useTls()) {
		ctx=getTlsContext();
	} else {
		ctx=NULL;
	}

	debugStart("parameters");
	debugWrite("idleclienttimeout: %d",idleclienttimeout);
	debugWrite("maxclientinfosize: %lld",(long long)maxclientinfosize);
	debugWrite("maxquerysize: %d",maxquerysize);
	debugWrite("maxbindcount: %hd",maxbindcount);
	debugWrite("maxbindnamesize: %hd",maxbindnamesize);
	debugWrite("maxstringbindvaluesize: %d",maxstringbindvaluesize);
	debugWrite("maxlobbindvaluesize: %d",maxlobbindvaluesize);
	debugWrite("waitfordowndb: %d",waitfordowndb);
	if (useKrb()) {
		debugWrite("krb: yes");
		debugWrite("krb keytab: %s",
			getGssContext()->getCredentials()->getKeytab());
		debugWrite("krb service: %s",
			getGssContext()->getService());
		debugStart("krb mechs");
		for (uint64_t i=0;
			i<getGssContext()->getCredentials()->
					getActualMechanismCount();
			i++) {
			debugWrite("%s",getGssContext()->
						getCredentials()->
						getActualMechanism(i)->
						getString());
		}
		debugEnd();
		// FIXME: print as bits or something
		debugWrite("krb flags: %d",
			getGssContext()->getActualFlags());
	} else {
		debugWrite("krb: no");
	}
	if (useTls()) {
		debugWrite("tls: yes");
		debugWrite("tls version: %s",
			getTlsContext()->getProtocolVersion());
		debugWrite("tls cert: %s",
			getTlsContext()->getCertificateChainFile());
		debugWrite("tls key: %s",
			getTlsContext()->getPrivateKeyFile());
		debugWrite("tls password: %s","(hidden)");
		debugWrite("tls validate: %d",
			getTlsContext()->getValidatePeer());
		debugWrite("tls ca: %s",
			getTlsContext()->getCertificateAuthority());
		debugWrite("tls ciphers: %s",
			getTlsContext()->getCiphers());
		debugWrite("tls depth: %d",
			getTlsContext()->getValidationDepth());
	} else {
		debugWrite("tls: no");
	}
	debugEnd();

	protocolversion=0;
	endresultset=END_RESULT_SET;
}

sqlrprotocol_sqlrclient::~sqlrprotocol_sqlrclient() {
	delete[] clientinfo;
}

clientsessionexitstatus_t sqlrprotocol_sqlrclient::clientSession(
							filedescriptor *cs) {

	clientsock=cs;

	// this instance is reused for every client of a pooled connection,
	// so a client that gets this connection next must not inherit the
	// previous client's authentication - unless this call is that same
	// client resuming a session it suspended, on a new socket
	if (!cont->isResumedSession()) {
		authenticated=false;
	}

	// set up the socket
	clientsock->setTranslateByteOrder(true);
	clientsock->setNaglesAlgorithmEnabled(false);
	//clientsock->setSocketReadBufferSize(65536);
	//clientsock->setSocketWriteBufferSize(65536);
	clientsock->setReadBufferSize(65536);
	clientsock->setWriteBufferSize(65536);
	//clientsock->useAsyncWrite();

	clientsessionexitstatus_t	status=CLIENTSESSIONEXITSTATUS_ERROR;

	// accept security context, if necessary
	if (!acceptSecurityContext()) {
		return status;
	}

	// During each session, the client will send a series of commands.
	// The session ends when the client ends it or when certain commands
	// fail.
	bool			loop=true;
	bool			endsession=true;
	uint16_t		command;
	do {

		// handle disabled instance
		// FIXME: push up?
		if (cont->getInstanceDisabled()) {
			endsession=true;
			break;
		}

		// get a command from the client
		if (!getCommand(&command)) {
			break;
		}

		// get the command start time
		datetime	dt;
		dt.initFromSystemDateTime();

		// handle client protocol version as a command, for now
		if (command==SQLRCLIENT_PROTOCOL_VERSION) {
			if (clientsock->read(&protocolversion,
						idleclienttimeout,0)==
						sizeof(uint16_t)) {
				// reject a version the server doesn't support
				if (protocolversion<
					SQLRCLIENT_PROTOCOL_VERSION_MIN ||
					protocolversion>
					SQLRCLIENT_PROTOCOL_VERSION_MAX) {
					sendUnsupportedProtocolError();
					endsession=false;
					break;
				}
				// END_RESULT_SET was 3 in protocol version 1,
				// but changed in version 2
				endresultset=(protocolversion==1)?
							3:END_RESULT_SET;
				continue;
			}
			endsession=false;
			break;
		} else

		// handle bad commands
		if (command>MAX_COMMAND) {
			endsession=true;
			break;
		} else

		// everything else requires a successful AUTH first.  PING and
		// IDENTIFY are exempted because they're useful as a pre-login
		// health check, and END_SESSION because a client that never
		// logged in still needs to be able to hang up cleanly.
		if (!authenticated && command!=AUTH &&
					command!=PING &&
					command!=IDENTIFY &&
					command!=END_SESSION) {
			sendNotAuthenticatedError();
			endsession=false;
			break;
		} else

		// these commands are all handled at the connection level
		if (command==AUTH) {
			// a client that never sent its protocol version
			// leaves protocolversion at its unset value
			if (protocolversion<SQLRCLIENT_PROTOCOL_VERSION_MIN) {
				sendUnsupportedProtocolError();
				endsession=false;
				break;
			}
			cont->incrementAuthCount();
			if (authCommand()) {
				authenticated=true;
				cont->beginSession();
				continue;
			}
			endsession=false;
			break;
		} else if (command==SUSPEND_SESSION) {
			cont->incrementSuspendSessionCount();
			suspendSessionCommand();
			status=CLIENTSESSIONEXITSTATUS_SUSPENDED_SESSION;
			endsession=false;
			break;
		} else if (command==END_SESSION) {
			debugStart("end session");
			debugEnd();
			cont->incrementEndSessionCount();
			status=CLIENTSESSIONEXITSTATUS_ENDED_SESSION;
			break;
		} else if (command==PING) {
			cont->incrementPingCount();
			pingCommand();
			continue;
		} else if (command==IDENTIFY) {
			cont->incrementIdentifyCount();
			identifyCommand();
			continue;
		} else if (command==SET_AUTO_COMMIT) {
			cont->incrementSetAutoCommitCount();
			setAutoCommitCommand();
			continue;
		} else if (command==GET_AUTO_COMMIT) {
			// FIXME: add this
			//cont->incrementGetAutocommitCount();
			getAutoCommitCommand();
			continue;
		} else if (command==BEGIN) {
			cont->incrementBeginCount();
			beginCommand();
			continue;
		} else if (command==COMMIT) {
			cont->incrementCommitCount();
			commitCommand();
			continue;
		} else if (command==ROLLBACK) {
			cont->incrementRollbackCount();
			rollbackCommand();
			continue;
		} else if (command==GET_IN_TRANSACTION) {
			getInTransactionCommand();
			continue;
		} else if (command==DB_VERSION) {
			cont->incrementDbVersionCount();
			dbVersionCommand();
			continue;
		} else if (command==BIND_FORMAT) {
			cont->incrementGetBindFormatCount();
			bindFormatCommand();
			continue;
		} else if (command==NEXT_VAL_FORMAT) {
			// FIXME: add this
			//cont->incrementNextvalFormatCount();
			getNextvalFormatCommand();
			continue;
		} else if (command==SERVER_VERSION) {
			cont->incrementGetServerVersionCount();
			serverVersionCommand();
			continue;
		} else if (command==SELECT_DATABASE) {
			cont->incrementSelectDatabaseCount();
			selectDatabaseCommand();
			continue;
		} else if (command==GET_CURRENT_DATABASE) {
			cont->incrementGetCurrentDatabaseCount();
			getCurrentDatabaseCommand();
			continue;
		} else if (command==GET_DATABASE_IS_SCHEMA) {
			// FIXME: add this
			//cont->incrementGetDatabaseIsSchemaCount();
			getDatabaseIsSchemaCommand();
			continue;
		} else if (command==SELECT_CATALOG) {
			// FIXME: add this
			//cont->incrementSelectCatalogCount();
			selectCatalogCommand();
			continue;
		} else if (command==GET_CURRENT_CATALOG) {
			// FIXME: add this
			//cont->incrementGetCurrentCatalogCount();
			getCurrentCatalogCommand();
			continue;
		} else if (command==SELECT_SCHEMA) {
			// FIXME: add this
			//cont->incrementSelectSchemaCount();
			selectSchemaCommand();
			continue;
		} else if (command==GET_CURRENT_SCHEMA) {
			// FIXME: add this
			//cont->incrementGetCurrentSchemaCount();
			getCurrentSchemaCommand();
			continue;
		} else if (command==GET_CURRENT_USER) {
			//cont->incrementGetIsolationLevelCount();
			getCurrentUserCommand();
			continue;
		} else if (command==GET_LAST_INSERT_ID) {
			cont->incrementGetLastInsertIdCount();
			getLastInsertIdCommand();
			continue;
		} else if (command==DB_HOST_NAME) {
			cont->incrementDbHostNameCount();
			dbHostNameCommand();
			continue;
		} else if (command==DB_IP_ADDRESS) {
			cont->incrementDbIpAddressCount();
			dbIpAddressCommand();
			continue;
		} else if (command==SET_ISOLATION_LEVEL) {
			//cont->incrementSetIsolationLevelCount();
			setIsolationLevelCommand();
			continue;
		} else if (command==GET_ISOLATION_LEVEL) {
			//cont->incrementGetIsolationLevelCount();
			getIsolationLevelCommand();
			continue;
		} else if (command==GET_DEFAULT_ISOLATION_LEVEL) {
			getDefaultIsolationLevelCommand();
			continue;
		} else if (command==SET_TRANSACTION_MODEL) {
			setTransactionModelCommand();
			continue;
		} else if (command==GET_TRANSACTION_MODEL) {
			getTransactionModelCommand();
			continue;
		} else if (command==GET_DEFAULT_TRANSACTION_MODEL) {
			getDefaultTransactionModelCommand();
			continue;
		} else if (command==GET_DATABASE_FEATURES) {
			//cont->incrementGetDatabaseFeaturesCount();
			getDatabaseFeaturesCommand();
			continue;
		}

		// For the rest of the commands,
		// the client will request a cursor
		sqlrservercursor	*cursor=getCursor(command);
		if (!cursor) {
			// Don't worry about reporting that a cursor wasn't
			// available for abort-result-set commands. Those
			// commands don't look for a response from the server
			// and it doesn't matter if a non-existent result set
			// was aborted.
			if (command!=ABORT_RESULT_SET) {
				noAvailableCursors(command);
			}
			continue;
		}

		// set the command start-time
		cont->setCommandStart(cursor,
				dt.getSecond(),dt.getMicrosecond());

		// these commands are all handled at the cursor level
		if (command==NEW_QUERY) {
			cont->incrementNewQueryCount();
			loop=newQueryCommand(cursor);
		} else if (command==RE_EXECUTE_QUERY) {
			cont->incrementReexecuteQueryCount();
			loop=reExecuteQueryCommand(cursor);
		} else if (command==FETCH_FROM_BIND_CURSOR) {
			cont->incrementFetchFromBindCursorCount();
			loop=fetchFromBindCursorCommand(cursor);
		} else if (command==FETCH_RESULT_SET) {
			cont->incrementFetchResultSetCount();
			loop=fetchResultSetCommand(cursor);
		} else if (command==ABORT_RESULT_SET) {
			cont->incrementAbortResultSetCount();
			abortResultSetCommand(cursor);
		} else if (command==SUSPEND_RESULT_SET) {
			cont->incrementSuspendResultSetCount();
			suspendResultSetCommand(cursor);
		} else if (command==RESUME_RESULT_SET) {
			cont->incrementResumeResultSetCount();
			loop=resumeResultSetCommand(cursor);
		} else if (command==GET_DATABASE_LIST) {
			cont->incrementGetDbListCount();
			loop=getDatabaseListCommand(cursor);
		} else if (command==GET_CATALOG_LIST) {
			//cont->incrementGetCatalogListCount();
			loop=getCatalogListCommand(cursor);
		} else if (command==GET_SCHEMA_LIST) {
			//cont->incrementGetSchemaListCount();
			loop=getSchemaListCommand(cursor);
		} else if (command==GET_TABLE_LIST) {
			cont->incrementGetTableListCount();
			loop=getTableListCommand(cursor);
		} else if (command==GET_TABLE_LIST_2) {
			cont->incrementGetTableListCount();
			loop=getTableList2Command(cursor);
		} else if (command==GET_TABLE_TYPE_LIST) {
			//cont->incrementGetTableTypeListCount();
			loop=getTableTypeListCommand(cursor);
		} else if (command==GET_COLUMN_LIST) {
			cont->incrementGetColumnListCount();
			loop=getColumnListCommand(cursor);
		} else if (command==GET_PRIMARY_KEYS_LIST) {
			//cont->incrementGetPrimaryKeyListCount();
			loop=getPrimaryKeysListCommand(cursor);
		} else if (command==GET_KEY_AND_INDEX_LIST) {
			//cont->incrementGetKeyAndIndexListCount();
			loop=getKeyAndIndexListCommand(cursor);
		} else if (command==GET_PROCEDURE_BIND_AND_COLUMN_LIST) {
			//cont->incrementGetProcedureParameterListCount();
			loop=getProcedureParameterListCommand(cursor);
		} else if (command==GET_TYPE_INFO_LIST) {
			//cont->incrementGetTypeInfoListCount();
			loop=getTypeInfoListCommand(cursor);
		} else if (command==GET_PROCEDURE_LIST) {
			//cont->incrementGetProcedureListCount();
			loop=getProcedureListCommand(cursor);
		} else if (command==GET_LAST_INSERT_ID_LIST) {
			//cont->incrementGetLastInsertIdListCount();
			loop=getLastInsertIdListCommand(cursor);
		} else if (command==GET_QUERY_TREE) {
			cont->incrementGetQueryTreeCount();
			loop=getQueryTreeCommand(cursor);
		} else if (command==GET_TRANSLATED_QUERY) {
			//cont->incrementGetTranslatedQueryCount();
			loop=getTranslatedQueryCommand(cursor);
		} else if (command==NEXT_RESULT_SET) {
			loop=nextResultSetCommand(cursor);
		} else {
			loop=false;
		}

		// set the command end-time
		dt.initFromSystemDateTime();
		cont->setCommandEnd(cursor,
				dt.getSecond(),dt.getMicrosecond());

		// free memory used by binds...
		// FIXME: can we move this inside of processQueryOrBindCursor?
		// verify that log/notification modules activated by
		// raise*Event calls don't still need the bind values
		cont->getBindPool(cursor)->clear();

	} while (loop);

	// close the client connection
	//
	// If an error occurred, the client could still be sending an entire
	// session's worth of data before it reads the error and closes the
	// socket.  We have to absorb all of that data.  We shouldn't just loop
	// forever though, that would provide a point of entry for a DOS attack.
	// We'll read the maximum number of bytes that could be sent.
	uint32_t	bytecount=
				// sending auth
				(sizeof(uint16_t)+
				// user/password
				2*(sizeof(uint32_t)+USERSIZE)+
				// sending query
				sizeof(uint16_t)+
				// need a cursor
				sizeof(uint16_t)+
				// executing new query
				sizeof(uint16_t)+
				// client info
				sizeof(uint64_t)+maxclientinfosize+
				// query size and query
				sizeof(uint32_t)+maxquerysize+
				// input bind var count
				sizeof(uint16_t)+
				// input bind vars
				maxbindcount*(2*sizeof(uint16_t)+
						maxbindnamesize)+
				// output bind var count
				sizeof(uint16_t)+
				// output bind vars
				maxbindcount*(2*sizeof(uint16_t)+
						maxbindnamesize)+
				// inputoutput bind var count
				sizeof(uint16_t)+
				// inputoutput bind vars
				maxbindcount*(2*sizeof(uint16_t)+
						maxbindnamesize)+
				// get column info
				sizeof(uint16_t)+
				// skip/fetch
				2*sizeof(uint32_t)
				// divide by two because we're
				// reading 2 bytes at a time
				)/2;

	debugStart("close client connection");
	debugWrite("absorbing %d bytes",bytecount);

	cont->closeClientConnection(bytecount);

	debugWrite("absorbed %d bytes",bytecount);

	// end the session if necessary
	if (endsession) {
		cont->endSession();
	}

	debugEnd();

	// return the exit status
	return status;
}

bool sqlrprotocol_sqlrclient::acceptSecurityContext() {

	if (!useKrb() && !useTls()) {
		return true;
	}

	debugStart("accepting security context");

	if (useKrb() && !gss::isSupported()) {
		cont->raiseInternalErrorEvent(NULL,
				"failed to accept gss security "
				"context (kerberos requested but "
				"not supported)");
		debugWrite("failed: kerberos requested but not supported");
		debugEnd();
		return false;
	} else if (useTls() && !tls::isSupported()) {
		cont->raiseInternalErrorEvent(NULL,
				"failed to accept tls security "
				"context (tls requested but "
				"not supported)");
		debugWrite("failed: tls requested but not supported");
		debugEnd();
		return false;
	}

	// attach the context and file descriptor to each other
	clientsock->setSocketLayer(ctx);
	ctx->setFileDescriptor(clientsock);

	// accept the security context
	bool	retval=ctx->accept();
	if (!retval) {
		cont->raiseInternalErrorEvent(NULL,
			"failed to accept security context");
		debugWrite("failed");
	} else {
		debugWrite("success");
	}

	debugEnd();

	return retval;
}

bool sqlrprotocol_sqlrclient::getCommand(uint16_t *command) {

	debugStart("get command");

	cont->setState(GET_COMMAND);

	// get the command
	ssize_t	result=clientsock->read(command,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {

		// Return false but don't consider it an error if we get a
		// timeout or a 0 (meaning that the client closed the socket)
		// as either would be natural to do here.
		if (result!=RESULT_TIMEOUT && result!=0) {
			cont->raiseClientProtocolErrorEvent(
				NULL,result,"get command failed");
		}

		*command=NO_COMMAND;

		debugWrite("get command failed");
		debugEnd();

		return false;
	}

	debugCommand(*command);
	debugEnd();

	return true;
}

sqlrservercursor *sqlrprotocol_sqlrclient::getCursor(uint16_t command) {

	debugStart("getting a cursor");

	// does the client need a cursor or does it already have one
	uint16_t	neednewcursor=DONT_NEED_NEW_CURSOR;
	if (command==NEW_QUERY ||
		command==GET_DATABASE_LIST ||
		command==GET_CATALOG_LIST ||
		command==GET_SCHEMA_LIST ||
		command==GET_TABLE_LIST ||
		command==GET_TABLE_LIST_2 ||
		command==GET_TABLE_TYPE_LIST ||
		command==GET_COLUMN_LIST ||
		command==GET_PRIMARY_KEYS_LIST ||
		command==GET_KEY_AND_INDEX_LIST ||
		command==GET_PROCEDURE_BIND_AND_COLUMN_LIST ||
		command==GET_TYPE_INFO_LIST ||
		command==GET_PROCEDURE_LIST ||
		command==GET_LAST_INSERT_ID_LIST ||
		command==ABORT_RESULT_SET ||
		command==GET_QUERY_TREE ||
		command==GET_TRANSLATED_QUERY) {
		ssize_t	result=clientsock->read(&neednewcursor,
						idleclienttimeout,0);
		if (result!=sizeof(uint16_t)) {
			cont->raiseClientProtocolErrorEvent(NULL,result,
					"get cursor failed: "
					"failed to get whether client "
					"needs  new cursor or not");
			debugWrite("failed to get whether client "
					"needs  new cursor or not");
			debugEnd();
			return NULL;
		}
	}

	sqlrservercursor	*cursor=NULL;

	if (neednewcursor==DONT_NEED_NEW_CURSOR) {

		// which cursor is the client requesting?
		uint16_t	id;
		ssize_t		result=clientsock->read(&id,
						idleclienttimeout,0);
		if (result!=sizeof(uint16_t)) {
			cont->raiseClientProtocolErrorEvent(NULL,result,
						"get cursor failed: "
						"failed to get cursor id");
			debugWrite("failed to get cursor id");
			debugEnd();
			return NULL;
		}

		// get the requested cursor
		cursor=cont->getCursor(id);

	} else {

		// find an available cursor
		cursor=cont->getCursor();
	}

	if (cursor) {
		debugWrite("cursor id: %hd",cont->getId(cursor));
	} else {
		debugWrite("cursor id: not found");
	}
	debugEnd();

	return cursor;
}

void sqlrprotocol_sqlrclient::noAvailableCursors(uint16_t command) {

	debugStart("no available cursors");

	// If no cursor was available, the client
	// could send an entire query and bind vars
	// before it reads the error and closes the
	// socket.  We have to absorb all of that
	// data.  We shouldn't just loop forever
	// though, that would provide a point of entry
	// for a DOS attack.  We'll read the maximum
	// number of bytes that could be sent.
	uint32_t	size=(
				// client info
				sizeof(uint64_t)+maxclientinfosize+
				// query size and query
				sizeof(uint32_t)+maxquerysize+
				// input bind var count
				sizeof(uint16_t)+
				// input bind vars
				maxbindcount*(2*sizeof(uint16_t)+
						maxbindnamesize)+
				// output bind var count
				sizeof(uint16_t)+
				// output bind vars
				maxbindcount*(2*sizeof(uint16_t)+
						maxbindnamesize)+
				// inputoutput bind var count
				sizeof(uint16_t)+
				// inputoutput bind vars
				maxbindcount*(2*sizeof(uint16_t)+
						maxbindnamesize)+
				// get column info
				sizeof(uint16_t)+
				// skip/fetch
				2*sizeof(uint32_t));

	debugWrite("absorbing %d bytes",size);

	clientsock->setNonBlockingMode(true);
	byte_t	*dummy=new byte_t[size];
	ssize_t	bytesread=clientsock->read(dummy,size,idleclienttimeout,0);
	clientsock->setNonBlockingMode(false);
	delete[] dummy;

	debugWrite("absorbed %lld bytes",(long long)bytesread);

	// indicate that an error has occurred
	clientsock->write((uint16_t)ERROR_OCCURRED);

	// send the error code
	clientsock->write((uint64_t)SQLR_ERROR_NOCURSORS);

	// send the error itself
	uint16_t	errsize=
			charstring::getLength(SQLR_ERROR_NOCURSORS_STRING);
	clientsock->write(errsize);
	clientsock->write(SQLR_ERROR_NOCURSORS_STRING,errsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugEnd();
}

bool sqlrprotocol_sqlrclient::authCommand() {

	debugStart("auth");

	// get the user/password from the client
	if (!getUserFromClient() || !getPasswordFromClient()) {
		debugEnd();
		return false;
	}

	// build credentials...
	sqlrcredentials	*cred=NULL;

	// try to use gss credentials
	if (useKrb()) {

		gsscontext	*ctx=getGssContext();
		if (ctx) {
			sqlrgsscredentials	*gsscred=
						new sqlrgsscredentials();
			gsscred->setInitiator(ctx->getInitiator());
			cred=gsscred;
		}

	} else

	// try to use tls credentials
	// (unless a user was passed in)
	if (useTls() && charstring::isNullOrEmpty(userbuffer)) {

		tlscontext	*ctx=getTlsContext();
		if (ctx) {
			tlscertificate	*cert=ctx->getPeerCertificate();
			if (cert) {
				sqlrtlscredentials	*tlscred=
						new sqlrtlscredentials();
				tlscred->setSubjectAlternateNames(
				cert->getSubjectAlternateNames());
				tlscred->setCommonName(
					cert->getCommonName());
				cred=tlscred;
			}
		}

	} else {

		// use user/password credentials
		sqlruserpasswordcredentials	*upcred=
					new sqlruserpasswordcredentials();
		upcred->setUser(userbuffer);
		upcred->setPassword(passwordbuffer);
		cred=upcred;
	}

	// auth
	bool	success=cont->auth(cred);

	debugWrite("auth %s",(success)?"success":"failed");
	debugEnd();

	// clean up
	delete cred;

	// success
	if (success) {
		return true;
	}

	// indicate that an error has occurred
	clientsock->write((uint16_t)ERROR_OCCURRED_DISCONNECT);
	clientsock->write((uint64_t)SQLR_ERROR_AUTHENTICATIONERROR);
	clientsock->write((uint16_t)charstring::getLength(
				SQLR_ERROR_AUTHENTICATIONERROR_STRING));
	clientsock->write(SQLR_ERROR_AUTHENTICATIONERROR_STRING);
	clientsock->flushWriteBuffer(-1,-1);

	return false;
}

void sqlrprotocol_sqlrclient::sendNotAuthenticatedError() {

	debugStart("not authenticated");
	debugEnd();

	// indicate that an error has occurred and disconnect, the same as
	// a failed auth does
	clientsock->write((uint16_t)ERROR_OCCURRED_DISCONNECT);
	clientsock->write((uint64_t)SQLR_ERROR_NOTAUTHENTICATED);
	clientsock->write((uint16_t)charstring::getLength(
				SQLR_ERROR_NOTAUTHENTICATED_STRING));
	clientsock->write(SQLR_ERROR_NOTAUTHENTICATED_STRING);
	clientsock->flushWriteBuffer(-1,-1);
}

void sqlrprotocol_sqlrclient::sendUnsupportedProtocolError() {

	debugStart("unsupported protocol");
	debugEnd();

	// indicate that an error has occurred and disconnect, the same as
	// a failed auth does
	clientsock->write((uint16_t)ERROR_OCCURRED_DISCONNECT);
	clientsock->write((uint64_t)SQLR_ERROR_UNSUPPORTED_PROTOCOL);
	clientsock->write((uint16_t)charstring::getLength(
				SQLR_ERROR_UNSUPPORTED_PROTOCOL_STRING));
	clientsock->write(SQLR_ERROR_UNSUPPORTED_PROTOCOL_STRING);
	clientsock->flushWriteBuffer(-1,-1);
}

bool sqlrprotocol_sqlrclient::getUserFromClient() {
	uint32_t	size=0;
	ssize_t		result=clientsock->read(&size,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"authentication failed: "
					"failed to get user size");
		debugWrite("failed to get user size");
		return false;
	}
	if (size>=sizeof(userbuffer)) {
		cont->raiseClientConnectionRefusedEvent(
				"authentication failed: "
				"user size too long: %d",size);
		debugWrite("user size too long: %d",size);
		return false;
	}
	result=clientsock->read(userbuffer,size,idleclienttimeout,0);
	if ((uint32_t)result!=size) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"authentication failed: "
					"failed to get user");
		debugWrite("failed to get user");
		return false;
	}
	userbuffer[size]='\0';
	debugWrite("username: \"%s\"",userbuffer);
	return true;
}

bool sqlrprotocol_sqlrclient::getPasswordFromClient() {
	uint32_t	size=0;
	ssize_t		result=clientsock->read(&size,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"authentication failed: "
					"failed to get password size");
		debugWrite("failed to get password size");
		return false;
	}
	if (size>=sizeof(passwordbuffer)) {
		cont->raiseClientConnectionRefusedEvent(
				"authentication failed: "
				"password size too long: %d",size);
		debugWrite("password size too long: %d",size);
		return false;
	}
	result=clientsock->read(passwordbuffer,size,idleclienttimeout,0);
	if ((uint32_t)result!=size) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"authentication failed: "
					"failed to get password");
		debugWrite("failed to get password");
		return false;
	}
	passwordbuffer[size]='\0';
	debugWrite("password: \"%s\"","(hidden)");
	return true;
}

void sqlrprotocol_sqlrclient::suspendSessionCommand() {

	debugStart("suspending session");

	// suspend the session
	const char	*unixsocketname=NULL;
	uint16_t	inetportnumber=0;
	cont->suspendSession(&unixsocketname,&inetportnumber);
	uint16_t	unixsocketsize=charstring::getLength(unixsocketname);

	debugWrite("unix socket name: %s",unixsocketname);
	debugWrite("inet port number: %hd",inetportnumber);

	// pass the socket info to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write(unixsocketsize);
	if (unixsocketsize) {
		clientsock->write(unixsocketname,unixsocketsize);
	}
	clientsock->write(inetportnumber);
	clientsock->flushWriteBuffer(-1,-1);

	debugEnd();
}

void sqlrprotocol_sqlrclient::pingCommand() {

	debugStart("ping");

	// ping the database
	bool	pingresult=cont->ping();

	// send result to the client
	if (pingresult) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}

	// if the ping failed, re-login
	if (!pingresult) {
		debugStart("re-login");
		cont->reLogIn(true);
		debugEnd();
	}

	debugEnd();
}

void sqlrprotocol_sqlrclient::identifyCommand() {

	debugStart("identify");

	// get the database type
	const char	*ident=cont->getDbType();

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	idsize=charstring::getLength(ident);
	clientsock->write(idsize);
	clientsock->write(ident,idsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("id: %.*s",idsize,ident);
	debugEnd();
}

void sqlrprotocol_sqlrclient::setAutoCommitCommand() {

	debugStart("set autocommit");

	// get on/off
	bool	on;
	ssize_t	result=clientsock->read(&on,idleclienttimeout,0);
	if (result!=sizeof(bool)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"get autocommit failed: "
					"failed to get autocommit setting");
		debugWrite("failed to get autocommit setting");
		debugEnd();
		return;
	}

	// set autocommit on/off
	bool	success=false;
	if (on) {
		debugWrite("on");
		success=cont->setAutoCommitOn();
	} else {
		debugWrite("off");
		success=cont->setAutoCommitOff();
	}

	// send result to the client
	if (success) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}

	debugEnd();
}

void sqlrprotocol_sqlrclient::getAutoCommitCommand() {

	debugStart("get autocommit");

	// get the current autocommit state
	bool	ac=cont->getAutoCommit();

	// send result to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write(ac);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite((ac)?"on":"off");
	debugEnd();
}

void sqlrprotocol_sqlrclient::beginCommand() {
	debugStart("begin");
	if (cont->begin()) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}
	debugEnd();
}

void sqlrprotocol_sqlrclient::commitCommand() {
	debugStart("commit");
	if (cont->commit()) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}
	debugEnd();
}

void sqlrprotocol_sqlrclient::rollbackCommand() {
	debugStart("rollback");
	if (cont->rollback()) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}
	debugEnd();
}

void sqlrprotocol_sqlrclient::getInTransactionCommand() {

	debugStart("get in-transaction");

	// get the current in-transaction state
	bool	intx=cont->getInTransaction();

	// send result to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write(intx);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite((intx)?"in transaction":"not in transaction");
	debugEnd();
}

void sqlrprotocol_sqlrclient::dbVersionCommand() {

	debugStart("db version");

	// get the db version
	const char	*dbversion=cont->getDbVersion();

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	dbvsize=charstring::getLength(dbversion);
	clientsock->write(dbvsize);
	clientsock->write(dbversion,dbvsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("db version: %.*s",dbvsize,dbversion);
	debugEnd();
}

void sqlrprotocol_sqlrclient::bindFormatCommand() {

	debugStart("bind format");

	// get the bind format
	const char	*bf=cont->getBindFormat();

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	bfsize=charstring::getLength(bf);
	clientsock->write(bfsize);
	clientsock->write(bf,bfsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("bind format: %.*s",bfsize,bf);
	debugEnd();
}

void sqlrprotocol_sqlrclient::getNextvalFormatCommand() {

	debugStart("nextval format");

	// get the nextval format
	const char	*nf=cont->getNextvalFormat();

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	nfsize=charstring::getLength(nf);
	clientsock->write(nfsize);
	clientsock->write(nf,nfsize);
	clientsock->flushWriteBuffer(-1,-1);

	// FIXME: dangerous to print because it has a %s in it
	//debugWrite("nextval format: %.*s",nfsize,nf);
	debugEnd();
}

void sqlrprotocol_sqlrclient::serverVersionCommand() {

	debugWrite("server version");

	// get the server version
	const char	*svrversion=SQLR_VERSION;

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	svrvsize=charstring::getLength(svrversion);
	clientsock->write(svrvsize);
	clientsock->write(svrversion,svrvsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("server version: %.*s",svrvsize,svrversion);
	debugEnd();
}

void sqlrprotocol_sqlrclient::selectDatabaseCommand() {

	debugStart("select database");

	// get size of db parameter
	uint32_t	dbsize;
	ssize_t		result=clientsock->read(&dbsize,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"select database failed: "
						"failed to get db size");
		debugWrite("failed to get db size");
		debugEnd();
		return;
	}

	// bounds checking
	if (dbsize>maxquerysize) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,1,
					"select database failed: "
					"client sent bad db size: %d",
					dbsize);
		debugWrite("client sent bad db size: %d",dbsize);
		debugEnd();
		return;
	}

	// read the db parameter into the buffer
	char	*db=new char[dbsize+1];
	if (dbsize) {
		result=clientsock->read(db,dbsize,idleclienttimeout,0);
		if ((uint32_t)result!=dbsize) {
			clientsock->write(false);
			clientsock->flushWriteBuffer(-1,-1);
			delete[] db;
			cont->raiseClientProtocolErrorEvent(NULL,result,
						"select database failed: "
						"failed to get database name");
			debugWrite("failed to get database name");
			debugEnd();
			return;
		}
	}
	db[dbsize]='\0';

	debugWrite("db: %.*s",dbsize,db);
	
	// select the db and send back the result.
	if (cont->selectDatabase(db)) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}

	delete[] db;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getCurrentDatabaseCommand() {

	debugStart("get current database");

	// get the current database
	char	*currentdb=cont->getCurrentDatabase();

	// FIXME: this can fail

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	currentdbsize=charstring::getLength(currentdb);
	clientsock->write(currentdbsize);
	clientsock->write(currentdb,currentdbsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("current db: %.*s",currentdbsize,currentdb);

	// clean up
	delete[] currentdb;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getDatabaseIsSchemaCommand() {

	debugStart("get database is schema");

	// get whether database is schema
	bool	databaseisschema=cont->getDatabaseIsSchema();

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write((uint16_t)(databaseisschema?1:0));
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("database is schema: %d",databaseisschema);

	debugEnd();
}

void sqlrprotocol_sqlrclient::selectCatalogCommand() {

	debugStart("select catalog");

	// get size of catalog parameter
	uint32_t	catsize;
	ssize_t		result=clientsock->read(&catsize,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"select catalog failed: "
						"failed to get catalog size");
		debugWrite("failed to get catalog size");
		debugEnd();
		return;
	}

	// bounds checking
	if (catsize>maxquerysize) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,1,
					"select catalog failed: "
					"client sent bad catalog size: %d",
					catsize);
		debugWrite("client sent bad catalog size: %d",catsize);
		debugEnd();
		return;
	}

	// read the catalog parameter into the buffer
	char	*cat=new char[catsize+1];
	if (catsize) {
		result=clientsock->read(cat,catsize,idleclienttimeout,0);
		if ((uint32_t)result!=catsize) {
			clientsock->write(false);
			clientsock->flushWriteBuffer(-1,-1);
			delete[] cat;
			cont->raiseClientProtocolErrorEvent(NULL,result,
						"select catalog failed: "
						"failed to get catalog name");
			debugWrite("failed to get catalog name");
			debugEnd();
			return;
		}
	}
	cat[catsize]='\0';

	debugWrite("catalog: %.*s",catsize,cat);

	// select the catalog and send back the result.
	if (cont->selectCatalog(cat)) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}

	delete[] cat;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getCurrentCatalogCommand() {

	debugStart("get current catalog");

	// get the current catalog
	char	*currentcatalog=cont->getCurrentCatalog();

	// FIXME: this can fail

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	currentcatalogsize=charstring::getLength(
							currentcatalog);
	clientsock->write(currentcatalogsize);
	clientsock->write(currentcatalog,currentcatalogsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("current catalog: %.*s",currentcatalogsize,currentcatalog);

	// clean up
	delete[] currentcatalog;

	debugEnd();
}

void sqlrprotocol_sqlrclient::selectSchemaCommand() {

	debugStart("select schema");

	// get size of schema parameter
	uint32_t	schsize;
	ssize_t		result=clientsock->read(&schsize,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"select schema failed: "
						"failed to get schema size");
		debugWrite("failed to get schema size");
		debugEnd();
		return;
	}

	// bounds checking
	if (schsize>maxquerysize) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,1,
					"select schema failed: "
					"client sent bad schema size: %d",
					schsize);
		debugWrite("client sent bad schema size: %d",schsize);
		debugEnd();
		return;
	}

	// read the schema parameter into the buffer
	char	*sch=new char[schsize+1];
	if (schsize) {
		result=clientsock->read(sch,schsize,idleclienttimeout,0);
		if ((uint32_t)result!=schsize) {
			clientsock->write(false);
			clientsock->flushWriteBuffer(-1,-1);
			delete[] sch;
			cont->raiseClientProtocolErrorEvent(NULL,result,
						"select schema failed: "
						"failed to get schema name");
			debugWrite("failed to get schema name");
			debugEnd();
			return;
		}
	}
	sch[schsize]='\0';

	debugWrite("schema: %.*s",schsize,sch);
	
	// select the schema and send back the result.
	if (cont->selectSchema(sch)) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}

	delete[] sch;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getCurrentSchemaCommand() {

	debugStart("get current schema");

	// get the current schema
	char	*currentschema=cont->getCurrentSchema();

	// FIXME: this can fail

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	currentschemasize=charstring::getLength(currentschema);
	clientsock->write(currentschemasize);
	clientsock->write(currentschema,currentschemasize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("current schema: %.*s",currentschemasize,currentschema);

	// clean up
	delete[] currentschema;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getCurrentUserCommand() {

	debugStart("get current user");

	// get the current user
	char	*currentuser=cont->getCurrentUser();

	// send it to the client
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	uint16_t	currentusersize=charstring::getLength(currentuser);
	clientsock->write(currentusersize);
	clientsock->write(currentuser,currentusersize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("current user: %.*s",currentusersize,currentuser);

	// clean up
	delete[] currentuser;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getLastInsertIdCommand() {

	debugStart("getting last insert id");

	uint64_t	id;
	if (cont->getLastInsertId(&id)) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->write(id);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		returnError(false);
		debugWrite("failed");
	}

	debugEnd();
}

void sqlrprotocol_sqlrclient::dbHostNameCommand() {

	debugStart("getting db host name");

	// get the db host name
	const char	*hostname=cont->getDbHostName();
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);

	// send it to the client
	uint16_t	hostnamesize=charstring::getLength(hostname);
	clientsock->write(hostnamesize);
	clientsock->write(hostname,hostnamesize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("host name: %.*s",hostnamesize,hostname);

	debugEnd();
}

void sqlrprotocol_sqlrclient::dbIpAddressCommand() {

	debugStart("getting db host name");

	// get the db ip address
	const char	*ipaddress=cont->getDbIpAddress();
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);

	// send it to the client
	uint16_t	ipaddresssize=charstring::getLength(ipaddress);
	clientsock->write(ipaddresssize);
	clientsock->write(ipaddress,ipaddresssize);
	clientsock->flushWriteBuffer(-1,-1);

	debugWrite("ip address: %.*s",ipaddresssize,ipaddress);

	debugEnd();
}

void sqlrprotocol_sqlrclient::setIsolationLevelCommand() {

	debugStart("setting isolation level");

	// get format
	uint16_t	format;
	ssize_t		result=clientsock->read(&format,
						idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"set isolation level failed: "
					"failed to get isolation level format");
		debugWrite("failed to get isolation level format");
		debugEnd();
		return;
	}

	// get size of isolation level parameter
	uint16_t	isolevelsize;
	result=clientsock->read(&isolevelsize,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"set isolation level failed: "
					"failed to get isolation level size");
		debugWrite("failed to get isolation level size");
		debugEnd();
		return;
	}

	// bounds checking
	if (isolevelsize>maxquerysize) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,1,
					"select database failed: client sent"
					"bad isolation levelsize: %d",
					isolevelsize);
		debugWrite("client sent bad isolation level size: %d",
							isolevelsize);
		debugEnd();
		return;
	}

	// read the isolation level parameter into the buffer
	char	*isolevel=new char[isolevelsize+1];
	if (isolevelsize) {
		result=clientsock->read(isolevel,isolevelsize,
						idleclienttimeout,0);
		if ((uint32_t)result!=isolevelsize) {
			clientsock->write(false);
			clientsock->flushWriteBuffer(-1,-1);
			delete[] isolevel;
			cont->raiseClientProtocolErrorEvent(NULL,result,
					"set isolation level failed: "
					"failed to get isolation level");
			debugWrite("failed to get isolation level");
			debugEnd();
			return;
		}
	}
	isolevel[isolevelsize]='\0';

	debugWrite("isolation level: %.*s",isolevelsize,isolevel);
	
	// select the isolation level and send back the result.
	if (cont->setIsolationLevel(isolevel,
			(sqlrserverisolationlevelformat_t)format)) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}

	delete[] isolevel;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getIsolationLevelCommand() {

	debugStart("getting isolation level");

	// get format
	uint16_t	format;
	ssize_t		result=clientsock->read(&format,
						idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"get isolation level failed: "
					"failed to get isolation level format");
		debugWrite("failed to get isolation level format");
		debugEnd();
		return;
	}


	// get the isolation level
	const char	*isolevel=cont->getIsolationLevel(
				(sqlrserverisolationlevelformat_t)format);
	if (isolevel) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);

		// send it to the client
		uint16_t	isolevelsize=charstring::getLength(isolevel);
		clientsock->write(isolevelsize);
		clientsock->write(isolevel,isolevelsize);
		clientsock->flushWriteBuffer(-1,-1);

		debugWrite("isolation level: %.*s",isolevelsize,isolevel);

	} else {
		debugWrite("failed");
		returnError(false);
	}

	debugEnd();
}

void sqlrprotocol_sqlrclient::getDefaultIsolationLevelCommand() {

	debugStart("getting default isolation level");

	// get format
	uint16_t	format;
	ssize_t		result=clientsock->read(&format,
						idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
				"get default isolation level failed: "
				"failed to get isolation level format");
		debugWrite("failed to get isolation level format");
		debugEnd();
		return;
	}


	// get the default isolation level
	const char	*isolevel=cont->getDefaultIsolationLevel(
				(sqlrserverisolationlevelformat_t)format);
	if (isolevel) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);

		// send it to the client
		uint16_t	isolevelsize=charstring::getLength(isolevel);
		clientsock->write(isolevelsize);
		clientsock->write(isolevel,isolevelsize);
		clientsock->flushWriteBuffer(-1,-1);

		debugWrite("default isolation level: %.*s",
						isolevelsize,isolevel);

	} else {
		debugWrite("failed");
		returnError(false);
	}

	debugEnd();
}

void sqlrprotocol_sqlrclient::setTransactionModelCommand() {

	debugStart("setting transaction model");

	// get the size of the transaction model string
	uint16_t	txmodelsize;
	ssize_t		result=clientsock->read(&txmodelsize,
						idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"set transaction model failed: "
					"failed to get transaction model size");
		debugWrite("failed to get transaction model size");
		debugEnd();
		return;
	}

	// bounds checking
	if (txmodelsize>maxquerysize) {
		clientsock->write(false);
		cont->raiseClientProtocolErrorEvent(NULL,1,
					"set transaction model failed: "
					"client sent bad transaction model "
					"size: %d",txmodelsize);
		debugWrite("client sent bad transaction model size: %d",
								txmodelsize);
		debugEnd();
		return;
	}

	// read the transaction model parameter into the buffer
	char	*txmodelstr=new char[txmodelsize+1];
	if (txmodelsize) {
		result=clientsock->read(txmodelstr,txmodelsize,
						idleclienttimeout,0);
		if ((uint32_t)result!=txmodelsize) {
			clientsock->write(false);
			clientsock->flushWriteBuffer(-1,-1);
			delete[] txmodelstr;
			cont->raiseClientProtocolErrorEvent(NULL,result,
					"set transaction model failed: "
					"failed to get transaction model");
			debugWrite("failed to get transaction model");
			debugEnd();
			return;
		}
	}
	txmodelstr[txmodelsize]='\0';

	debugWrite("transaction model: %.*s",txmodelsize,txmodelstr);

	// map the string to an enum
	sqlrtxmodel_t	txmodel;
	if (!charstring::compare(txmodelstr,"native")) {
		txmodel=cont->getNativeTransactionModel();
	} else {
		txmodel=sqlrservercontroller::stringToTransactionModel(
								txmodelstr);
	}

	// reject unknown transaction models
	if (txmodel==SQLRTXMODEL_UNKNOWN) {
		stringbuffer	errmsg;
		errmsg.append("invalid transaction model: ");
		errmsg.append(txmodelstr);
		cont->setError(errmsg.getString(),
				SQLR_ERROR_NOTIMPLEMENTED,true);
		delete[] txmodelstr;
		debugWrite("invalid transaction model");
		returnError(false);
		debugEnd();
		return;
	}

	// set the transaction model and send back the result
	if (cont->setTransactionModel(txmodel)) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->flushWriteBuffer(-1,-1);
	} else {
		debugWrite("failed");
		returnError(false);
	}

	delete[] txmodelstr;

	debugEnd();
}

void sqlrprotocol_sqlrclient::getTransactionModelCommand() {

	debugStart("getting transaction model");

	// get the transaction model
	const char	*txmodel=sqlrservercontroller::transactionModelToString(
						cont->getTransactionModel());

	debugWrite("transaction model: %s",txmodel);

	// send result to the client
	uint16_t	txmodelsize=charstring::getLength(txmodel);
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write(txmodelsize);
	clientsock->write(txmodel,txmodelsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugEnd();
}

void sqlrprotocol_sqlrclient::getDefaultTransactionModelCommand() {

	debugStart("getting default transaction model");

	// get the native transaction model
	const char	*txmodel=sqlrservercontroller::transactionModelToString(
					cont->getNativeTransactionModel());

	debugWrite("default transaction model: %s",txmodel);

	// send result to the client
	uint16_t	txmodelsize=charstring::getLength(txmodel);
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write(txmodelsize);
	clientsock->write(txmodel,txmodelsize);
	clientsock->flushWriteBuffer(-1,-1);

	debugEnd();
}

void sqlrprotocol_sqlrclient::getDatabaseFeaturesCommand() {

	debugStart("get database features");

	// get the database features
	const char * const	*features=cont->getDatabaseFeatures();

	if (features) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);

		debugWrite("%d features",(int)(FEATURE_COUNT));

		// send the number of features
		clientsock->write((uint16_t)(FEATURE_COUNT));

		// send each feature
		for (uint16_t i=0; i<FEATURE_COUNT; i++) {

			const char	*value=features[i];
			uint16_t	valuesize=charstring::getLength(value);

			debugWrite("%d:%.*s",i,valuesize,value);

			clientsock->write(valuesize);
			clientsock->write(value,valuesize);
		}
		clientsock->flushWriteBuffer(-1,-1);

	} else {
		debugWrite("failed");
		returnError(false);
	}

	debugEnd();
}

bool sqlrprotocol_sqlrclient::newQueryCommand(sqlrservercursor *cursor) {

	debugStart("new query");

	// if we're using a custom cursor then close it
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		customcursor->close();
		cursor->clearCustomQueryCursor();
	}

	// get the client info and query from the client
	bool	success=(getClientInfo(cursor) && getQuery(cursor));

	// do we need to use a custom query handler for this query?
	if (success) {
		cursor=cont->useCustomQueryCursor(cursor);
	}

	if (success &&
		getInputBinds(cursor) &&
		getOutputBinds(cursor) &&
		getInputOutputBinds(cursor) &&
		getSendColumnInfo() &&
		processQueryOrBindCursor(cursor,
				SQLRCLIENTQUERYTYPE_QUERY,
				SQLRSERVERLISTFORMAT_NULL,
				false,false)) {
		debugWrite("success");
		debugEnd();
		return true;
	}

	// The client is apparently sending us something we
	// can't handle.  Return an error if there was one,
	// instruct the client to disconnect and return false
	// to end the session on this side.
	if (cont->getErrorNumber(cursor)) {
		returnError(cursor,true);
	}
	debugWrite("failed");
	debugEnd();
	return false;
}

bool sqlrprotocol_sqlrclient::reExecuteQueryCommand(sqlrservercursor *cursor) {

	debugStart("rexecute query");

	// if we're using a custom cursor then operate on it
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		cursor=customcursor;
	}

	if (getInputBinds(cursor) &&
		getOutputBinds(cursor) &&
		getInputOutputBinds(cursor) &&
		getSendColumnInfo() &&
		processQueryOrBindCursor(cursor,
				SQLRCLIENTQUERYTYPE_QUERY,
				SQLRSERVERLISTFORMAT_NULL,
				true,false)) {
		debugWrite("success");
		debugEnd();
		return true;
	}

	// The client is apparently sending us something we
	// can't handle.  Return an error if there was one,
	// instruct the client to disconnect and return false
	// to end the session on this side.
	if (cont->getErrorNumber(cursor)) {
		returnError(cursor,true);
	}
	debugWrite("failed");
	debugEnd();
	return false;
}

bool sqlrprotocol_sqlrclient::nextResultSetCommand(sqlrservercursor *cursor) {

	debugStart("nextResultSet");

	// if we're using a custom cursor then operate on it
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		cursor=customcursor;
	}

	bool nextresultsetavailable;
	if (cont->nextResultSet(cursor,&nextresultsetavailable)) {
		debugWrite("success");
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);
		clientsock->write(nextresultsetavailable);
		if (!nextresultsetavailable) {
			clientsock->flushWriteBuffer(-1,-1);
		} else {
			cont->incrementNextResultSetAvailableCount();

			// protocol version 2 clients only expect the bool
			// above; don't send anything past it or they'll
			// desync trying to parse it as the next command's
			// response
			if (protocolversion<3) {
				clientsock->flushWriteBuffer(-1,-1);
				cont->incrementNextResultSetCount();
				debugWrite("success");
				debugEnd();
				return true;
			}

			// the new result set has its own column metadata
			// and rows; send them now so the client can refresh
			// its cached header/data instead of continuing to
			// show the previous result set's
			clientsock->flushWriteBuffer(-1,-1);
			if (!getSkipAndFetch(true,cursor)) {
				debugWrite("failed to get skip and fetch");
				debugEnd();
				return false;
			}
			clientsock->write((uint16_t)NO_ERROR_OCCURRED);
			clientsock->write(cont->getId(cursor));
			clientsock->write((uint16_t)NO_SUSPENDED_RESULT_SET);
			returnResultSetHeader(cursor,false);
			returnResultSetData(cursor,false,false);
		}
		cont->incrementNextResultSetCount();
		debugWrite("success");
		debugEnd();
		return true;
	}

	returnError(!cont->getLiveConnection());
	cont->incrementNextResultSetCount();
	debugWrite("failed");
	debugEnd();
	return false;
}

bool sqlrprotocol_sqlrclient::fetchFromBindCursorCommand(
					sqlrservercursor *cursor) {

	debugStart("fetch from bind cursor");

	// if we're using a custom cursor then close it
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		customcursor->close();
		cursor->clearCustomQueryCursor();
	}

	// get whether to get column info
	if (getSendColumnInfo() &&
		processQueryOrBindCursor(cursor,
				SQLRCLIENTQUERYTYPE_QUERY,
				SQLRSERVERLISTFORMAT_NULL,
				false,true)) {
		debugWrite("success");
		debugEnd();
		return true;
	}

	// The client is apparently sending us something we
	// can't handle.  Return an error if there was one,
	// instruct the client to disconnect and return false
	// to end the session on this side.
	if (cont->getErrorNumber(cursor)) {
		returnError(cursor,true);
	}
	debugWrite("failed");
	debugEnd();
	return false;
}

bool sqlrprotocol_sqlrclient::processQueryOrBindCursor(
					sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype,
					sqlrserverlistformat_t listformat,
					bool reexecute,
					bool bindcursor) {

	// loop here to handle down databases
	for (;;) {

		// process the query or bind cursor...
		bool	success=false;
		if (bindcursor) {
			success=cont->fetchFromBindCursor(cursor);
		} else if (reexecute) {
			success=cont->executeQuery(cursor,true,true,true,true);
		} else {
			success=(cont->prepareQuery(cursor,
					cont->getQueryBuffer(cursor),
					cont->getQuerySize(cursor),
					true,true,true,true) &&
				cont->executeQuery(cursor,true,true,true,true));
		}

		// get the skip and fetch parameters here so everything can be
		// done in one round trip without relying on buffering
		if (success) {
			success=getSkipAndFetch(true,cursor);
		}

		if (success) {

			// success...

			// indicate that no error has occurred
			clientsock->write((uint16_t)NO_ERROR_OCCURRED);

			// send the client the id of the cursor
			// that it's going to use so it can request
			// it again later for re-execute
			clientsock->write(cont->getId(cursor));

			// tell the client that this is not a
			// suspended result set
			clientsock->write((uint16_t)NO_SUSPENDED_RESULT_SET);

			// send a result set header
			// (a bind-cursor fetch doesn't re-read binds, so
			// don't send stale bind values from the previous
			// command on that cursor)
			returnResultSetHeader(cursor,!bindcursor);

			// return the result set
			return returnResultSetData(cursor,false,false);

		} else {

			// an error occurred...

			// is the db still up?
			bool	dbup=cont->getLiveConnection(cursor);

			// if the db is still up, or if we're not configured
			// to wait for them if they're down, then return the
			// error
			if (dbup || !waitfordowndb) {

				// return the error
				returnError(cursor,false);
			}

			// if the error was a dead connection
			// then re-establish the connection
			if (!dbup) {

				debugWrite("database is down...");

				cont->raiseDbErrorEvent(cursor,
						cont->getErrorBuffer(cursor));

				// Bail out if we're shutting down.
				// Otherwise this loop retries the
				// query against a logged-out
				// connection over and over, spinning
				// until the process is killed with
				// signal 9.
				if (waitfordowndb &&
					process::getShutDownFlag()) {
					return false;
				}

				cont->reLogIn(true);

				// if we're waiting for down databases then
				// loop back and try the query again
				if (waitfordowndb) {
					continue;
				}
			}
			return true;
		}
	}
}

bool sqlrprotocol_sqlrclient::getClientInfo(sqlrservercursor *cursor) {

	debugStart("getting client info");

	// init
	clientinfo[0]='\0';
	clientinfosize=0;

	// get the size of the client info
	ssize_t	result=clientsock->read(&clientinfosize);
	if (result!=sizeof(uint64_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get client info failed: "
					"failed to get clientinfo size");
		debugWrite("failed to get clientinfo size");
		debugEnd();
		return false;
	}

	// bounds checking
	if (clientinfosize>maxclientinfosize) {

		stringbuffer	err;
		err.append(SQLR_ERROR_MAXCLIENTINFOSIZE_STRING);
		err.append(" (")->append(clientinfosize)->append('>');
		err.append(maxclientinfosize)->append(')');
		cont->setError(cursor,err.getString(),
				SQLR_ERROR_MAXCLIENTINFOSIZE,true);

		cont->raiseClientProtocolErrorEvent(cursor,1,
				"get client info failed: "
				"client sent bad client info size: %lld",
				(long long)clientinfosize);
		debugWrite("client sent bad client info size: %lld",
							(long long)clientinfosize);
		debugEnd();
		return false;
	}

	// read the client info into the buffer
	result=clientsock->read(clientinfo,clientinfosize);
	if ((uint64_t)result!=clientinfosize) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
						"get client info failed: "
						"failed to get client info");
		debugWrite("failed to get client info");
		debugEnd();
		return false;
	}
	clientinfo[clientinfosize]='\0';

	debugWrite("clientinfo: %.*s",(int)clientinfosize,clientinfo);

	// FIXME: push up?
	// update the stats with the client info
	cont->setClientInfo(clientinfo,clientinfosize);

	debugEnd();

	return true;
}

bool sqlrprotocol_sqlrclient::getQuery(sqlrservercursor *cursor) {

	debugStart("getting query");

	// init
	uint32_t	querysize=0;
	char		*querybuffer=cont->getQueryBuffer(cursor);
	querybuffer[0]='\0';
	cont->setQuerySize(cursor,0);

	// get the size of the query
	ssize_t	result=clientsock->read(&querysize,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
						"get query failed: "
						"failed to get query size");
		debugWrite("failed to get query size");
		debugEnd();
		return false;
	}

	// bounds checking
	if (querysize>maxquerysize) {

		stringbuffer	err;
		err.append(SQLR_ERROR_MAXQUERYSIZE_STRING);
		err.append(" (")->append(querysize)->append('>');
		err.append(maxquerysize)->append(')');
		cont->setError(cursor,err.getString(),
				SQLR_ERROR_MAXQUERYSIZE,true);

		cont->raiseClientProtocolErrorEvent(cursor,1,
					"get query failed: "
					"client sent bad query size: %d",
					querysize);
		debugWrite("client sent bad query size: %d",querysize);
		debugEnd();
		return false;
	}

	// read the query into the buffer
	result=clientsock->read(querybuffer,querysize,idleclienttimeout,0);
	if ((uint32_t)result!=querysize) {

		querybuffer[0]='\0';

		cont->raiseClientProtocolErrorEvent(cursor,result,
						"get query failed: "
						"failed to get query");
		debugWrite("failed to get query");
		debugEnd();
		return false;
	}

	// update query buffer and size
	querybuffer[querysize]='\0';
	cont->setQuerySize(cursor,querysize);

	debugstr.clear();
	debugstr.safePrint(querybuffer,querysize);
	debugWrite("query: \"%.*s\"",(int)debugstr.getSize(),debugstr.getString());
	//debugWrite("query: \"%.*s\"",querysize,querybuffer);
	debugWrite("query size: %lld",(long long)debugstr.getSize());

	// FIXME: push up?
	// update the stats with the current query
	cont->setCurrentQuery(querybuffer,querysize);

	debugEnd();

	return true;
}

bool sqlrprotocol_sqlrclient::getInputBinds(sqlrservercursor *cursor) {

	debugStart("getting input binds");

	// get the number of input bind variable/values
	uint16_t	inbindcount=0;
	if (!getBindVarCount(cursor,&inbindcount)) {
		debugEnd();
		return false;
	}
	cont->setInputBindCount(cursor,inbindcount);

	// get the input bind buffers
	memorypool		*bindpool=cont->getBindPool(cursor);
	sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);

	// fill the buffers
	for (uint16_t i=0; i<inbindcount && i<maxbindcount; i++) {

		sqlrserverbindvar	*bv=&(inbinds[i]);

		// get the variable name and type
		if (!(getBindVarName(cursor,bv,bindpool) &&
					getBindVarType(bv))) {
			debugEnd();
			return false;
		}

		// get the value
		if (bv->type==SQLRSERVERBINDVARTYPE_NULL ||
			bv->type==SQLRSERVERBINDVARTYPE_NULLBLOB ||
			bv->type==SQLRSERVERBINDVARTYPE_NULLCLOB) {
			// getNullBind() leaves the type alone, so a null lob
			// bind keeps its lob-ness for handleBinds() to route on
			getNullBind(bv,bindpool);
		} else if (bv->type==SQLRSERVERBINDVARTYPE_STRING) {
			if (!getStringBind(cursor,bv,bindpool)) {
				debugEnd();
				return false;
			}
		} else if (bv->type==SQLRSERVERBINDVARTYPE_INTEGER) {
			if (!getIntegerBind(bv)) {
				debugEnd();
				return false;
			}
		} else if (bv->type==SQLRSERVERBINDVARTYPE_DOUBLE) {
			if (!getDoubleBind(bv)) {
				debugEnd();
				return false;
			}
		} else if (bv->type==SQLRSERVERBINDVARTYPE_DATE) {
			if (!getDateBind(bv,bindpool)) {
				debugEnd();
				return false;
			}
		} else if (bv->type==SQLRSERVERBINDVARTYPE_BLOB) {
			if (!getLobBind(cursor,bv,bindpool)) {
				debugEnd();
				return false;
			}
		} else if (bv->type==SQLRSERVERBINDVARTYPE_CLOB) {
			if (!getLobBind(cursor,bv,bindpool)) {
				debugEnd();
				return false;
			}
		} else if (bv->type==SQLRSERVERBINDVARTYPE_ARRAY) {
			// an array bind arrives as text - the elements
			// rendered as a bracketed, comma-separated list
			if (!getStringBind(cursor,bv,bindpool)) {
				debugEnd();
				return false;
			}
		}
	}

	debugEnd();
	return true;
}

bool sqlrprotocol_sqlrclient::getOutputBinds(sqlrservercursor *cursor) {

	debugStart("getting output binds");

	// get the number of output bind variable/values
	uint16_t	outbindcount=0;
	if (!getBindVarCount(cursor,&outbindcount)) {
		debugEnd();
		return false;
	}
	cont->setOutputBindCount(cursor,outbindcount);

	// get the output bind buffers
	memorypool		*bindpool=cont->getBindPool(cursor);
	sqlrserverbindvar	*outbinds=cont->getOutputBinds(cursor);

	// fill the buffers
	for (uint16_t i=0; i<outbindcount && i<maxbindcount; i++) {

		sqlrserverbindvar	*bv=&(outbinds[i]);

		// get the variable name and type
		if (!(getBindVarName(cursor,bv,bindpool) &&
					getBindVarType(bv))) {
			debugEnd();
			return false;
		}

		// get the size of the value
		if (bv->type==SQLRSERVERBINDVARTYPE_STRING) {
			bv->value.stringval=NULL;
			if (!getBindSize(cursor,bv,&maxstringbindvaluesize)) {
				debugEnd();
				return false;
			}
			// This must be a allocated and zeroed because oracle
			// gets angry if these aren't initialized to NULL's.
			// It's possible that just the first character needs to
			// be NULL, but for now I'm just going to go ahead and
			// allocate/zero.
			bv->value.stringval=
				(char *)bindpool->allocate(bv->valuesize+1);
			bytestring::zero(bv->value.stringval,bv->valuesize+1);
			debugWrite("STRING");
		} else if (bv->type==SQLRSERVERBINDVARTYPE_INTEGER) {
			debugWrite("INTEGER");
		} else if (bv->type==SQLRSERVERBINDVARTYPE_DOUBLE) {
			debugWrite("DOUBLE");
			// these don't typically get set, but they get used
			// when building debug strings, so we need to
			// initialize them
			bv->value.doubleval.precision=0;
			bv->value.doubleval.scale=0;
		} else if (bv->type==SQLRSERVERBINDVARTYPE_DATE) {
			debugWrite("DATE");
			bv->value.dateval.year=0;
			bv->value.dateval.month=0;
			bv->value.dateval.day=0;
			bv->value.dateval.hour=0;
			bv->value.dateval.minute=0;
			bv->value.dateval.second=0;
			bv->value.dateval.microsecond=0;
			bv->value.dateval.tz=NULL;
			bv->value.dateval.isnegative=false;
		} else if (bv->type==SQLRSERVERBINDVARTYPE_BLOB ||
					bv->type==SQLRSERVERBINDVARTYPE_CLOB) {
			if (!getBindSize(cursor,bv,&maxlobbindvaluesize)) {
				debugEnd();
				return false;
			}
			if (bv->type==SQLRSERVERBINDVARTYPE_BLOB) {
				debugWrite("BLOB");
			} else if (bv->type==SQLRSERVERBINDVARTYPE_CLOB) {
				debugWrite("CLOB");
			}
		} else if (bv->type==SQLRSERVERBINDVARTYPE_CURSOR) {
			debugWrite("CURSOR");
			sqlrservercursor	*curs=cont->getCursor();
			if (!curs) {
				// FIXME: set error here
				debugEnd();
				return false;
			}
			cont->setState(curs,SQLRCURSORSTATE_BUSY);
			// clear out leftover bind counts from whatever
			// this cursor was used for previously
			cont->setInputBindCount(curs,0);
			cont->setOutputBindCount(curs,0);
			cont->setInputOutputBindCount(curs,0);
			bv->value.cursorid=cont->getId(curs);
		}

		// init the null indicator
		bv->isnull=cont->getNonNullBindValue();
	}

	debugEnd();
	return true;
}

bool sqlrprotocol_sqlrclient::getInputOutputBinds(sqlrservercursor *cursor) {

	debugStart("getting input/output binds");

	if (protocolversion<2) {
		debugWrite("client protocol too old");
		debugEnd();
		return true;
	}

	// get the number of input/output bind variable/values
	uint16_t	inoutbindcount=0;
	if (!getBindVarCount(cursor,&inoutbindcount)) {
		debugEnd();
		return false;
	}
	cont->setInputOutputBindCount(cursor,inoutbindcount);

	// get the input/output bind buffers
	memorypool		*bindpool=cont->getBindPool(cursor);
	sqlrserverbindvar	*inoutbinds=cont->getInputOutputBinds(cursor);

	// fill the buffers
	for (uint16_t i=0; i<inoutbindcount && i<maxbindcount; i++) {

		sqlrserverbindvar	*bv=&(inoutbinds[i]);

		// get the variable name and type
		if (!(getBindVarName(cursor,bv,bindpool) &&
					getBindVarType(bv))) {
			debugEnd();
			return false;
		}

		// get the size of the value
		if (bv->type==SQLRSERVERBINDVARTYPE_NULL) {
			bv->type=SQLRSERVERBINDVARTYPE_STRING;
			bv->value.stringval=NULL;
			if (!getBindSize(cursor,bv,&maxstringbindvaluesize)) {
				debugEnd();
				return false;
			}
			// This must be a allocated and zeroed because oracle
			// gets angry if these aren't initialized to NULL's.
			// It's possible that just the first character needs to
			// be NULL, but for now I'm just going to go ahead and
			// allocate/zero.
			bv->value.stringval=
				(char *)bindpool->allocate(bv->valuesize+1);
			bytestring::zero(bv->value.stringval,bv->valuesize+1);
			bv->isnull=cont->getNullBindValue();
			debugWrite("NULL");
		} else if (bv->type==SQLRSERVERBINDVARTYPE_STRING) {
			bv->value.stringval=NULL;
			if (!getBindSize(cursor,bv,&maxstringbindvaluesize)) {
				debugEnd();
				return false;
			}
			// This must be a allocated and zeroed because oracle
			// gets angry if these aren't initialized to NULL's.
			// It's possible that just the first character needs to
			// be NULL, but for now I'm just going to go ahead and
			// allocate/zero.
			bv->value.stringval=
				(char *)bindpool->allocate(bv->valuesize+1);
			bytestring::zero(bv->value.stringval,bv->valuesize+1);

			// get the bind value
			ssize_t	result=clientsock->read(bv->value.stringval,
							bv->valuesize,
							idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)(bv->valuesize)) {
				bv->value.stringval[0]='\0';
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"value");
				debugWrite("failed to get bind value");
				debugEnd();
				return false;
			}
			bv->value.stringval[bv->valuesize]='\0';
			bv->isnull=cont->getNonNullBindValue();
			debugWrite("STRING");
		} else if (bv->type==SQLRSERVERBINDVARTYPE_INTEGER) {

			// get the bind value
			ssize_t	result=clientsock->read(&(bv->value.integerval),
							idleclienttimeout,0);
			if (result!=sizeof(uint64_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"value");
				debugWrite("failed to get bind value");
				debugEnd();
				return false;
			}
			bv->isnull=cont->getNonNullBindValue();
			debugWrite("INTEGER");
		} else if (bv->type==SQLRSERVERBINDVARTYPE_DOUBLE) {

			// get the bind value
			ssize_t	result=clientsock->read(
					&(bv->value.doubleval.value),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(double)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"value");
				debugWrite("failed to get bind value");
				debugEnd();
				return false;
			}

			// get the precision
			result=clientsock->read(
					&(bv->value.doubleval.precision),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint32_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get "
							"precision");
				debugWrite("failed to get precision");
				debugEnd();
				return false;
			}

			// get the scale
			result=clientsock->read(
					&(bv->value.doubleval.scale),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint32_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get "
							"scale");
				debugWrite("failed to get scale");
				debugEnd();
				return false;
			}

			bv->isnull=cont->getNonNullBindValue();
			debugWrite("DOUBLE");
		} else if (bv->type==SQLRSERVERBINDVARTYPE_DATE) {

			// get the year
			ssize_t	result=clientsock->read(
					&(bv->value.dateval.year),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint16_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"year");
				debugWrite("failed to get bind year");
				debugEnd();
				return false;
			}

			// get the month
			result=clientsock->read(
					&(bv->value.dateval.month),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint16_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"month");
				debugWrite("failed to get bind month");
				debugEnd();
				return false;
			}

			// get the day
			result=clientsock->read(
					&(bv->value.dateval.day),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint16_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"day");
				debugWrite("failed to get bind day");
				debugEnd();
				return false;
			}

			// get the hour
			result=clientsock->read(
					&(bv->value.dateval.hour),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint16_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"hour");
				debugWrite("failed to get bind hour");
				debugEnd();
				return false;
			}

			// get the minute
			result=clientsock->read(
					&(bv->value.dateval.minute),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint16_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"minute");
				debugWrite("failed to get bind minute");
				debugEnd();
				return false;
			}

			// get the second
			result=clientsock->read(
					&(bv->value.dateval.second),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint16_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"second");
				debugWrite("failed to get bind second");
				debugEnd();
				return false;
			}

			// get the microsecond
			result=clientsock->read(
					&(bv->value.dateval.microsecond),
					idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint32_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"microsecond");
				debugWrite("failed to get bind microsecond");
				debugEnd();
				return false;
			}

			// get the tz size
			uint16_t	tzsize=0;
			result=clientsock->read(&tzsize,idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)sizeof(uint16_t)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"tz size");
				debugWrite("failed to get bind tz size");
				debugEnd();
				return false;
			}

			// get the tz
			bv->value.dateval.tz=(char *)bindpool->allocate(tzsize+1);
			result=clientsock->read(bv->value.dateval.tz,tzsize,
						idleclienttimeout,0);
			if ((uint32_t)result!=(uint32_t)tzsize) {
				bv->value.dateval.tz[0]='\0';
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get bind "
							"tz");
				debugWrite("failed to get bind tz");
				debugEnd();
				return false;
			}
			bv->value.dateval.tz[tzsize]='\0';

			// get the is-negative flag
			result=clientsock->read(&bv->value.dateval.isnegative,
							idleclienttimeout,0);
			if (result!=sizeof(bool)) {
				cont->raiseClientProtocolErrorEvent(
							cursor,result,
							"get binds failed: "
							"failed to get "
							"is-negative flag");
				debugWrite("failed to get is-negative flag");
				debugEnd();
				return false;
			}

			bv->isnull=cont->getNonNullBindValue();
			debugWrite("DATE");
		} /*else if (bv->type==SQLRSERVERBINDVARTYPE_BLOB ||
					bv->type==SQLRSERVERBINDVARTYPE_CLOB) {
			if (!getBindSize(cursor,bv,&maxlobbindvaluesize)) {
				debugEnd();
				return false;
			}
			if (bv->type==SQLRSERVERBINDVARTYPE_BLOB) {
				debugWrite("BLOB");
			} else if (bv->type==SQLRSERVERBINDVARTYPE_CLOB) {
				debugWrite("CLOB");
			}
			bv->isnull=cont->getNonNullBindValue();
		}*/
	}

	debugEnd();
	return true;
}

bool sqlrprotocol_sqlrclient::getBindVarCount(sqlrservercursor *cursor,
							uint16_t *count) {

	// init
	*count=0;

	// get the number of input bind variable/values
	ssize_t	result=clientsock->read(count,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
						"get binds failed: "
						"failed to get bind count");
		debugWrite("failed to get bind count");
		*count=0;
		return false;
	}

	// bounds checking
	if (*count>maxbindcount) {

		stringbuffer	err;
		err.append(SQLR_ERROR_MAXBINDCOUNT_STRING);
		err.append(" (")->append(*count)->append('>');
		err.append(maxbindcount)->append(')');
		cont->setError(cursor,err.getString(),
				SQLR_ERROR_MAXBINDCOUNT,true);

		cont->raiseClientProtocolErrorEvent(cursor,1,
			"get binds failed: "
			"client tried to send too many binds: %hd",*count);
		debugWrite("client tried to send too many binds: %hd",*count);

		*count=0;
		return false;
	}

	return true;
}

bool sqlrprotocol_sqlrclient::getBindVarName(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						memorypool *bindpool) {

	// init
	bv->variablesize=0;
	bv->variable=NULL;

	// get the variable name size
	uint16_t	bindnamesize;
	ssize_t		result=clientsock->read(&bindnamesize,
						idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get binds failed: "
					"failed to get variable name size");
		return false;
	}

	// bounds checking
	if (bindnamesize>maxbindnamesize) {

		stringbuffer	err;
		err.append(SQLR_ERROR_MAXBINDNAMESIZE_STRING);
		err.append(" (")->append(bindnamesize)->append('>');
		err.append(maxbindnamesize)->append(')');
		cont->setError(cursor,err.getString(),
					SQLR_ERROR_MAXBINDNAMESIZE,true);

		cont->raiseClientProtocolErrorEvent(cursor,1,
				"get binds failed: "
				"bad variable name size: %hd",bindnamesize);
		return false;
	}

	// get the variable name
	bv->variablesize=bindnamesize+1;
	bv->variable=(char *)bindpool->allocate(bindnamesize+2);
	bv->variable[0]=cont->getBindFormat()[0];
	result=clientsock->read(bv->variable+1,bindnamesize,
					idleclienttimeout,0);
	if (result!=bindnamesize) {
		bv->variablesize=0;
		bv->variable[0]='\0';
		cont->raiseClientProtocolErrorEvent(cursor,result,
						"get binds failed: "
						"failed to get variable name");
		return false;
	}
	bv->variable[bindnamesize+1]='\0';

	debugWrite("%s",bv->variable);

	return true;
}

bool sqlrprotocol_sqlrclient::getBindVarType(sqlrserverbindvar *bv) {

	// get the type
	uint16_t	type;
	ssize_t	result=clientsock->read(&type,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get type");
		return false;
	}
	bv->type=(sqlrserverbindvartype_t)type;
	return true;
}

bool sqlrprotocol_sqlrclient::getBindSize(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						uint32_t *maxsize) {

	// init
	bv->valuesize=0;

	// get the size of the value
	ssize_t	result=clientsock->read(&(bv->valuesize),idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		bv->valuesize=0;
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get binds failed: "
					"failed to get bind value size");
		return false;
	}

	// bounds checking
	if (bv->valuesize>*maxsize) {
		if (maxsize==&maxstringbindvaluesize) {
			stringbuffer	err;
			err.append(SQLR_ERROR_MAXSTRINGBINDVALUESIZE_STRING);
			err.append(" (")->append(bv->valuesize)->append('>');
			err.append(*maxsize)->append(')');
			cont->setError(cursor,err.getString(),
				SQLR_ERROR_MAXSTRINGBINDVALUESIZE,true);
		} else {
			stringbuffer	err;
			err.append(SQLR_ERROR_MAXLOBBINDVALUESIZE_STRING);
			err.append(" (")->append(bv->valuesize)->append('>');
			err.append(*maxsize)->append(')');
			cont->setError(cursor,err.getString(),
				SQLR_ERROR_MAXLOBBINDVALUESIZE,true);
		}
		cont->raiseClientProtocolErrorEvent(cursor,1,
				"get binds failed: bad value size: %d",
				bv->valuesize);
		return false;
	}

	return true;
}

void sqlrprotocol_sqlrclient::getNullBind(sqlrserverbindvar *bv,
						memorypool *bindpool) {

	debugWrite("NULL");

	bv->value.stringval=(char *)bindpool->allocate(1);
	bv->value.stringval[0]='\0';
	bv->valuesize=0;
	bv->isnull=cont->getNullBindValue();

	// The bind var array is allocated once per cursor and nothing else
	// clears these, so a slot can still hold a pointer from whatever
	// segmented a bind here last.  A null bind has no segments.
	bv->segmentlengths=NULL;
	bv->segmentcount=0;
}

bool sqlrprotocol_sqlrclient::getStringBind(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						memorypool *bindpool) {

	debugWrite("%s",(bv->type==SQLRSERVERBINDVARTYPE_ARRAY)?
							"ARRAY":"STRING");

	// init
	bv->value.stringval=NULL;

	// get the size of the value
	if (!getBindSize(cursor,bv,&maxstringbindvaluesize)) {
		return false;
	}

	// allocate space to store the value
	bv->value.stringval=(char *)bindpool->allocate(bv->valuesize+1);

	// get the bind value
	ssize_t	result=clientsock->read(bv->value.stringval,
					bv->valuesize,
					idleclienttimeout,0);
	if ((uint32_t)result!=(uint32_t)(bv->valuesize)) {
		bv->value.stringval[0]='\0';
		cont->raiseClientProtocolErrorEvent(cursor,result,
						"get binds failed: "
						"failed to get bind value");
		debugWrite("failed to get bind value");
		return false;
	}
	bv->value.stringval[bv->valuesize]='\0';

	bv->isnull=cont->getNonNullBindValue();

	debugWrite("%s",bv->value.stringval);

	return true;
}

bool sqlrprotocol_sqlrclient::getIntegerBind(sqlrserverbindvar *bv) {

	debugWrite("INTEGER");

	// get the value itself
	uint64_t	value;
	ssize_t		result=clientsock->read(&value,idleclienttimeout,0);
	if (result!=sizeof(uint64_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get bind value");
		debugWrite("failed to get bind value");
		return false;
	}

	// set the value
	bv->value.integerval=(int64_t)value;

	char	*intval=charstring::parseNumber(bv->value.integerval);
	debugWrite(intval);
	delete[] intval;

	return true;
}

bool sqlrprotocol_sqlrclient::getDoubleBind(sqlrserverbindvar *bv) {

	debugWrite("DOUBLE");

	// get the value
	ssize_t	result=clientsock->read(&(bv->value.doubleval.value),
						idleclienttimeout,0);
	if (result!=sizeof(double)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get bind value");
		debugWrite("failed to get bind value");
		return false;
	}

	// get the precision
	result=clientsock->read(&(bv->value.doubleval.precision),
						idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get precision");
		debugWrite("failed to get precision");
		return false;
	}

	// get the scale
	result=clientsock->read(&(bv->value.doubleval.scale),
						idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get scale");
		debugWrite("failed to get scale");
		return false;
	}

	char	*doubleval=charstring::parseNumber(bv->value.doubleval.value);
	debugWrite(doubleval);
	delete[] doubleval;

	return true;
}

bool sqlrprotocol_sqlrclient::getDateBind(sqlrserverbindvar *bv,
						memorypool *bindpool) {

	debugWrite("DATE");

	// init
	bv->value.dateval.tz=NULL;

	uint16_t	temp;

	// get the year
	ssize_t	result=clientsock->read(&temp,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get year");
		debugWrite("failed to get year");
		return false;
	}
	bv->value.dateval.year=(int16_t)temp;

	// get the month
	result=clientsock->read(&temp,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get month");
		debugWrite("failed to get month");
		return false;
	}
	bv->value.dateval.month=(int16_t)temp;

	// get the day
	result=clientsock->read(&temp,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get day");
		debugWrite("failed to get day");
		return false;
	}
	bv->value.dateval.day=(int16_t)temp;

	// get the hour
	result=clientsock->read(&temp,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get hour");
		debugWrite("failed to get hour");
		return false;
	}
	bv->value.dateval.hour=(int16_t)temp;

	// get the minute
	result=clientsock->read(&temp,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get minute");
		debugWrite("failed to get minute");
		return false;
	}
	bv->value.dateval.minute=(int16_t)temp;

	// get the second
	result=clientsock->read(&temp,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get second");
		debugWrite("failed to get second");
		return false;
	}
	bv->value.dateval.second=(int16_t)temp;

	// get the microsecond
	uint32_t	temp32;
	result=clientsock->read(&temp32,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get microsecond");
		debugWrite("failed to get microsecond");
		return false;
	}
	bv->value.dateval.microsecond=(int32_t)temp32;

	// get the size of the time zone
	uint16_t	size;
	result=clientsock->read(&size,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get timezone size");
		debugWrite("failed to get timezone size");
		return false;
	}

	// FIXME: do bounds checking here

	// allocate space to store the time zone
	bv->value.dateval.tz=(char *)bindpool->allocate(size+1);

	// get the time zone
	result=clientsock->read(bv->value.dateval.tz,size,
					idleclienttimeout,0);
	if ((uint16_t)result!=size) {
		bv->value.dateval.tz[0]='\0';
		cont->raiseClientProtocolErrorEvent(NULL,result,
						"get binds failed: "
						"failed to get timezone");
		debugWrite("failed to get timezone");
		return false;
	}
	bv->value.dateval.tz[size]='\0';

	// get the is-negative flag
	bool	tempbool;
	result=clientsock->read(&tempbool,idleclienttimeout,0);
	if (result!=sizeof(bool)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"get binds failed: "
					"failed to get is-negative flag");
		debugWrite("failed to get is-negative flag");
		return false;
	}
	bv->value.dateval.isnegative=tempbool;

	bv->isnull=cont->getNonNullBindValue();

	debugWrite("%04hd-%02hd-%02hd %s%02hd:%02hd:%02hd.%06d %s",
			bv->value.dateval.year,
			bv->value.dateval.month,
			bv->value.dateval.day,
			(bv->value.dateval.isnegative)?"-":"",
			bv->value.dateval.hour,
			bv->value.dateval.minute,
			bv->value.dateval.second,
			bv->value.dateval.microsecond,
			bv->value.dateval.tz);

	return true;
}

bool sqlrprotocol_sqlrclient::getLobBind(sqlrservercursor *cursor,
						sqlrserverbindvar *bv,
						memorypool *bindpool) {

	// init
	bv->value.stringval=NULL;

	if (bv->type==SQLRSERVERBINDVARTYPE_BLOB) {
		debugWrite("BLOB");
	}
	if (bv->type==SQLRSERVERBINDVARTYPE_CLOB) {
		debugWrite("CLOB");
	}

	// get the size of the value
	if (!getBindSize(cursor,bv,&maxlobbindvaluesize)) {
		return false;
	}

	// allocate space to store the value
	// (the +1 is to store the NULL-terminator for CLOBS)
	bv->value.stringval=(char *)bindpool->allocate(bv->valuesize+1);

	// get the bind value
	ssize_t	result=clientsock->read(bv->value.stringval,
					bv->valuesize,
					idleclienttimeout,0);
	if ((uint32_t)result!=(uint32_t)(bv->valuesize)) {
		bv->value.stringval[0]='\0';
		cont->raiseClientProtocolErrorEvent(cursor,result,
						"get binds failed: bad value");
		debugWrite("bad value");
		return false;
	}

	// It shouldn't hurt to NULL-terminate the lob because the actual size
	// (which doesn't include the NULL terminator) should be used when
	// binding.
	bv->value.stringval[bv->valuesize]='\0';

	bv->isnull=cont->getNonNullBindValue();

	return true;
}

bool sqlrprotocol_sqlrclient::getSendColumnInfo() {

	debugStart("get send column info");

	uint16_t	sendcolumninfo;
	ssize_t	result=clientsock->read(&sendcolumninfo,idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(NULL,result,
					"get send column info failed");
		debugWrite("get send column info failed");
		debugEnd();
		return false;
	}

	if (sendcolumninfo==SEND_COLUMN_INFO) {
		debugWrite("send column info");
	} else {
		debugWrite("don't send column info");
	}

	cont->setSendColumnInfo((sendcolumninfo==SEND_COLUMN_INFO));

	debugEnd();
	return true;
}

bool sqlrprotocol_sqlrclient::getSkipAndFetch(bool initial,
						sqlrservercursor *cursor) {

	debugStart("get skip and fetch");

	ssize_t	result=0;
	if (initial) {

		// get some flags
		uint64_t	flags=0;
		result=clientsock->read(&flags,idleclienttimeout,0);
		if (result!=sizeof(uint64_t)) {
			cont->raiseClientProtocolErrorEvent(cursor,result,
					"return result set data failed: "
					"failed to get flags");
			debugWrite("failed to get flags");
			debugEnd();
			return false;
		}

		// for now the only flag is whether or not to do lazy fetches
		lazyfetch=flags;

		// in this situation, skip should always be 0
		skip=0;

	} else {

		// get the number of rows to skip
		result=clientsock->read(&skip,idleclienttimeout,0);
		if (result!=sizeof(uint64_t)) {
			cont->raiseClientProtocolErrorEvent(cursor,result,
					"return result set data failed: "
					"failed to get rows to skip");
			debugWrite("failed to get rows to skip");
			debugEnd();
			return false;
		}
	}

	// get the number of rows to fetch
	result=clientsock->read(&fetch,idleclienttimeout,0);
	if (result!=sizeof(uint64_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
				"return result set data failed: "
				"failed to get rows to fetch");
		debugWrite("failed to get rows to fetch");
		debugEnd();
		return false;
	}

	debugWrite("lazy fetch: %d",lazyfetch);
	debugWrite("skip: %lld",(long long)skip);
	debugWrite("fetch: %lld",(long long)fetch);

	debugEnd();
	return true;
}

void sqlrprotocol_sqlrclient::returnResultSetHeader(
					sqlrservercursor *cursor,
					bool sendbindvalues) {

	debugStart("returning result set header");

	// decide whether to use the cursor itself
	// or an attached custom query cursor
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		cursor=customcursor;
	}

	// return the row counts
	debugStart("returning row counts");
	sendRowCounts(cont->knowsRowCount(cursor),
			cont->rowCount(cursor),
			cont->knowsAffectedRows(cursor),
			cont->getAffectedRows(cursor));
	debugEnd();

	// tell the client whether or not the column information will be sent
	bool	sendcolumninfo=cont->getSendColumnInfo();
	clientsock->write((sendcolumninfo)?
				(uint16_t)SEND_COLUMN_INFO:
				(uint16_t)DONT_SEND_COLUMN_INFO);
	debugWrite("column info: %s",(sendcolumninfo)?"will be sent":
							"will not be sent");

	// return the column count
	uint32_t	colcount=cont->colCount(cursor);
	debugWrite("col count: %d",colcount);
	clientsock->write(colcount);

	if (sendcolumninfo) {

		// return the column type format
		uint16_t	format=cont->columnTypeFormat(cursor);
		debugWrite("format: %s",
				(format==COLUMN_TYPE_IDS)?"id's":"names");
		clientsock->write(format);

		// return the column info
		returnColumnInfo(cursor,format);
	}

	// return the output bind vars
	returnOutputBindValues(cursor,sendbindvalues);
	returnInputOutputBindValues(cursor,sendbindvalues);

	debugEnd();
}

void sqlrprotocol_sqlrclient::returnColumnInfo(sqlrservercursor *cursor,
							uint16_t format) {

	debugStart("returning column info");

	for (uint32_t i=0; i<cont->colCount(cursor); i++) {

		const char	*name=cont->getColumnName(cursor,i);
		uint16_t	namesize=cont->getColumnNameSize(cursor,i);
		uint32_t	size=cont->getColumnSize(cursor,i);
		uint32_t	precision=cont->getColumnPrecision(cursor,i);
		uint32_t	scale=cont->getColumnScale(cursor,i);
		uint16_t	nullable=cont->getColumnIsNullable(cursor,i);
		uint16_t	primarykey=
				cont->getColumnIsPrimaryKey(cursor,i);
		uint16_t	unique=cont->getColumnIsUnique(cursor,i);
		uint16_t	partofkey=cont->getColumnIsPartOfKey(cursor,i);
		uint16_t	unsignednumber=
				cont->getColumnIsUnsigned(cursor,i);
		uint16_t	zerofill=cont->getColumnIsZeroFilled(cursor,i);
		uint16_t	binary=cont->getColumnIsBinary(cursor,i);
		uint16_t	autoincrement=
				cont->getColumnIsAutoIncrement(cursor,i);
		const char	*table=cont->getColumnTable(cursor,i);
		uint16_t	tablesize=cont->getColumnTableSize(cursor,i);

		if (format==COLUMN_TYPE_IDS) {
			sendColumnDefinition(name,namesize,
					protocolAppropriateColumnType(
						cont->getColumnType(cursor,i)),
					size,precision,scale,
					nullable,primarykey,unique,partofkey,
					unsignednumber,zerofill,binary,
					autoincrement,table,tablesize);
		} else {
			sendColumnDefinitionString(name,namesize,
					cont->getColumnTypeName(cursor,i),
					cont->getColumnTypeNameSize(cursor,i),
					size,precision,scale,
					nullable,primarykey,unique,partofkey,
					unsignednumber,zerofill,binary,
					autoincrement,table,tablesize);
		}
	}

	debugEnd();
}

uint16_t sqlrprotocol_sqlrclient::protocolAppropriateColumnType(
							uint16_t coltype) {

	if (protocolversion>=2) {
		return coltype;
	}

	// these types didn't exist in earlier protocol versions
	switch (coltype) {
		// also added by mysql
		case TINYTEXT_DATATYPE:
			return TINY_BLOB_DATATYPE;
		case MEDIUMTEXT_DATATYPE:
			return MEDIUM_BLOB_DATATYPE;
		case LONGTEXT_DATATYPE:
			return LONG_BLOB_DATATYPE;
		case JSON_DATATYPE:
			return UNKNOWN_DATATYPE;
		case GEOMETRY_DATATYPE:
			return UNKNOWN_DATATYPE;
		// also added by oracle
		case SDO_GEOMETRY_DATATYPE:
			return BLOB_DATATYPE;
		// added by mssql
		case NCHAR_DATATYPE:
			return CHAR_DATATYPE;
		case NVARCHAR_DATATYPE:
			return VARCHAR_DATATYPE;
		case NTEXT_DATATYPE:
			return TEXT_DATATYPE;
		case XML_DATATYPE:
			return VARCHAR_DATATYPE;
		case DATETIMEOFFSET_DATATYPE:
			return DATETIME_DATATYPE;
		default:
			return coltype;
	}
}

void sqlrprotocol_sqlrclient::sendRowCounts(bool knowsactual,
						uint64_t actual,
						bool knowsaffected,
						uint64_t affected) {

	debugStart("sending row counts");

	// send actual rows, if that is known
	if (knowsactual) {

		char	string[30];
		charstring::printf(string,sizeof(string),
				"actual rows: %lld",(long long)actual);
		debugWrite(string);

		clientsock->write((uint16_t)ACTUAL_ROWS);
		clientsock->write(actual);

	} else {

		debugWrite("actual rows unknown");

		clientsock->write((uint16_t)NO_ACTUAL_ROWS);
	}

	
	// send affected rows, if that is known
	if (knowsaffected) {

		char	string[46];
		charstring::printf(string,46,
				"affected rows: %lld",(long long)affected);
		debugWrite(string);

		clientsock->write((uint16_t)AFFECTED_ROWS);
		clientsock->write(affected);

	} else {

		debugWrite("affected rows unknown");

		clientsock->write((uint16_t)NO_AFFECTED_ROWS);
	}

	debugEnd();
}

void sqlrprotocol_sqlrclient::returnOutputBindValues(
					sqlrservercursor *cursor,
					bool sendbindvalues) {

	debugStart("returning output bind values");
	debugWrite("count: %hd",cont->getOutputBindCount(cursor));

	// bind values are leftovers from a previous command once the bind
	// pool backing them has been cleared, don't send them in that case
	uint16_t	bindcount=(sendbindvalues)?
					cont->getOutputBindCount(cursor):0;

	// run through the output bind values, sending them back
	for (uint16_t i=0; i<bindcount; i++) {

		sqlrserverbindvar	*bv=&(cont->getOutputBinds(cursor)[i]);

		if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
			debugstr.clear();
			debugstr.append(i);
			debugstr.append(":");
		}

		if (cont->getBindValueIsNull(bv->isnull)) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("NULL");
			}

			clientsock->write((uint16_t)NULL_DATA);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_BLOB) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("BLOB:");
			}

			returnOutputBindBlob(cursor,i);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_CLOB) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("CLOB:");
			}

			returnOutputBindClob(cursor,i);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_STRING) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("STRING:");
				debugstr.append(bv->value.stringval);
			}

			clientsock->write((uint16_t)STRING_DATA);
			bv->valuesize=charstring::getLength(
						(char *)bv->value.stringval);
			clientsock->write(bv->valuesize);
			clientsock->write(bv->value.stringval,bv->valuesize);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_INTEGER) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("INTEGER:");
				debugstr.append(bv->value.integerval);
			}

			clientsock->write((uint16_t)INTEGER_DATA);
			clientsock->write((uint64_t)bv->value.integerval);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_DOUBLE) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("DOUBLE:");
				debugstr.append(bv->value.doubleval.value);
				debugstr.append("(");
				debugstr.append(bv->value.doubleval.precision);
				debugstr.append(",");
				debugstr.append(bv->value.doubleval.scale);
				debugstr.append(")");
			}

			clientsock->write((uint16_t)DOUBLE_DATA);
			clientsock->write(bv->value.doubleval.value);
			clientsock->write((uint32_t)bv->value.
						doubleval.precision);
			clientsock->write((uint32_t)bv->value.
						doubleval.scale);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_DATE) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("DATE:");
				debugstr.append(bv->value.dateval.year);
				debugstr.append("-");
				debugstr.append(bv->value.dateval.month);
				debugstr.append("-");
				debugstr.append(bv->value.dateval.day);
				debugstr.append(" ");
				if (bv->value.dateval.isnegative) {
					debugstr.append('-');
				}
				debugstr.append(bv->value.dateval.hour);
				debugstr.append(":");
				debugstr.append(bv->value.dateval.minute);
				debugstr.append(":");
				debugstr.append(bv->value.dateval.second);
				debugstr.append(":");
				debugstr.append(bv->value.dateval.microsecond);
				debugstr.append(" ");
				debugstr.append(bv->value.dateval.tz);
			}

			clientsock->write((uint16_t)DATE_DATA);
			clientsock->write((uint16_t)bv->value.dateval.year);
			clientsock->write((uint16_t)bv->value.dateval.month);
			clientsock->write((uint16_t)bv->value.dateval.day);
			clientsock->write((uint16_t)bv->value.dateval.hour);
			clientsock->write((uint16_t)bv->value.dateval.minute);
			clientsock->write((uint16_t)bv->value.dateval.second);
			clientsock->write((uint32_t)bv->value.
							dateval.microsecond);
			uint16_t	size=charstring::getLength(
							bv->value.dateval.tz);
			clientsock->write(size);
			clientsock->write(bv->value.dateval.tz,size);
			clientsock->write((bool)bv->value.dateval.isnegative);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_CURSOR) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("CURSOR:");
				debugstr.append(bv->value.cursorid);
			}

			clientsock->write((uint16_t)CURSOR_DATA);
			clientsock->write(bv->value.cursorid);
		}

		if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
			debugWrite("%s",debugstr.getString());
		}
	}

	// terminate the bind vars
	clientsock->write((uint16_t)END_BIND_VARS);

	debugEnd();
}

void sqlrprotocol_sqlrclient::returnOutputBindBlob(sqlrservercursor *cursor,
							uint16_t index) {
	sendLobOutputBind(cursor,index);
	cont->closeLobOutputBind(cursor,index);
}

void sqlrprotocol_sqlrclient::returnOutputBindClob(sqlrservercursor *cursor,
							uint16_t index) {
	sendLobOutputBind(cursor,index);
	cont->closeLobOutputBind(cursor,index);
}

#define MAX_BYTES_PER_CHAR	4

void sqlrprotocol_sqlrclient::sendLobOutputBind(sqlrservercursor *cursor,
							uint16_t index) {

	// Get lob size.  If this fails, send a NULL field.
	uint64_t	loblength;
	if (!cont->getLobOutputBindLength(cursor,index,&loblength)) {
		sendNullField();
		return;
	}

	// for lobs of 0 size
	if (!loblength) {
		startSendingLong(0);
		sendLongSegment("",0);
		endSendingLong();
		return;
	}

	// initialize sizes and status
	uint64_t	charstoread=sizeof(lobbuffer)/MAX_BYTES_PER_CHAR;
	uint64_t	charsread=0;
	uint64_t	offset=0;
	bool		start=true;

	for (;;) {

		// read a segment from the lob
		if (!cont->getLobOutputBindSegment(cursor,index,
					lobbuffer,sizeof(lobbuffer),
					offset,charstoread,&charsread) ||
					!charsread) {

			// if we fail to get a segment or got nothing...
			// if we haven't started sending yet, then send a NULL,
			// otherwise just end normally
			if (start) {
				sendNullField();
			} else {
				endSendingLong();
			}
			return;

		} else {

			// if we haven't started sending yet, then do that now
			if (start) {
				startSendingLong(loblength);
				start=false;
			}

			// send the segment we just got
			sendLongSegment(lobbuffer,charsread);

			// FIXME: or should this be charsread?
			offset=offset+charstoread;
		}
	}
}

void sqlrprotocol_sqlrclient::returnInputOutputBindValues(
						sqlrservercursor *cursor,
						bool sendbindvalues) {

	debugStart("returning input/output bind values");
	debugWrite("count: %hd",cont->getInputOutputBindCount(cursor));

	if (protocolversion<2) {
		debugWrite("client protocol too old");
		debugEnd();
		return;
	}

	// bind values are leftovers from a previous command once the bind
	// pool backing them has been cleared, don't send them in that case
	uint16_t	bindcount=(sendbindvalues)?
				cont->getInputOutputBindCount(cursor):0;

	// run through the input/output bind values, sending them back
	for (uint16_t i=0; i<bindcount; i++) {

		sqlrserverbindvar	*bv=
				&(cont->getInputOutputBinds(cursor)[i]);

		if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
			debugstr.clear();
			debugstr.append(i);
			debugstr.append(":");
		}

		if (cont->getBindValueIsNull(bv->isnull)) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("NULL");
			}

			clientsock->write((uint16_t)NULL_DATA);

		} else /*if (bv->type==SQLRSERVERBINDVARTYPE_BLOB) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("BLOB:");
			}

			returnInputOutputBindBlob(cursor,i);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_CLOB) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("CLOB:");
			}

			returnInputOutputBindClob(cursor,i);

		} else*/ if (bv->type==SQLRSERVERBINDVARTYPE_STRING) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("STRING:");
				debugstr.append(bv->value.stringval);
			}

			clientsock->write((uint16_t)STRING_DATA);
			bv->valuesize=charstring::getLength(
						(char *)bv->value.stringval);
			clientsock->write(bv->valuesize);
			clientsock->write(bv->value.stringval,bv->valuesize);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_INTEGER) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("INTEGER:");
				debugstr.append(bv->value.integerval);
			}

			clientsock->write((uint16_t)INTEGER_DATA);
			clientsock->write((uint64_t)bv->value.integerval);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_DOUBLE) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("DOUBLE:");
				debugstr.append(bv->value.doubleval.value);
				debugstr.append("(");
				debugstr.append(bv->value.doubleval.precision);
				debugstr.append(",");
				debugstr.append(bv->value.doubleval.scale);
				debugstr.append(")");
			}

			clientsock->write((uint16_t)DOUBLE_DATA);
			clientsock->write(bv->value.doubleval.value);
			clientsock->write((uint32_t)bv->value.
						doubleval.precision);
			clientsock->write((uint32_t)bv->value.
						doubleval.scale);

		} else if (bv->type==SQLRSERVERBINDVARTYPE_DATE) {

			if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
				debugstr.append("DATE:");
				debugstr.append(bv->value.dateval.year);
				debugstr.append("-");
				debugstr.append(bv->value.dateval.month);
				debugstr.append("-");
				debugstr.append(bv->value.dateval.day);
				debugstr.append(" ");
				if (bv->value.dateval.isnegative) {
					debugstr.append('-');
				}
				debugstr.append(bv->value.dateval.hour);
				debugstr.append(":");
				debugstr.append(bv->value.dateval.minute);
				debugstr.append(":");
				debugstr.append(bv->value.dateval.second);
				debugstr.append(":");
				debugstr.append(bv->value.dateval.microsecond);
				debugstr.append(" ");
				debugstr.append(bv->value.dateval.tz);
			}

			clientsock->write((uint16_t)DATE_DATA);
			clientsock->write((uint16_t)bv->value.dateval.year);
			clientsock->write((uint16_t)bv->value.dateval.month);
			clientsock->write((uint16_t)bv->value.dateval.day);
			clientsock->write((uint16_t)bv->value.dateval.hour);
			clientsock->write((uint16_t)bv->value.dateval.minute);
			clientsock->write((uint16_t)bv->value.dateval.second);
			clientsock->write((uint32_t)bv->value.
							dateval.microsecond);
			uint16_t	size=charstring::getLength(
							bv->value.dateval.tz);
			clientsock->write(size);
			clientsock->write(bv->value.dateval.tz,size);
			clientsock->write((bool)bv->value.dateval.isnegative);

		}

		if (cont->getLoggingEnabled() ||
				cont->getNotificationsEnabled()) {
			debugWrite("%s",debugstr.getString());
		}
	}

	// terminate the bind vars
	clientsock->write((uint16_t)END_BIND_VARS);

	debugEnd();
}

void sqlrprotocol_sqlrclient::sendColumnDefinition(
						const char *name,
						uint16_t namesize,
						uint16_t type, 
						uint32_t size,
						uint32_t precision,
						uint32_t scale,
						uint16_t nullable,
						uint16_t primarykey,
						uint16_t unique,
						uint16_t partofkey,
						uint16_t unsignednumber,
						uint16_t zerofill,
						uint16_t binary,
						uint16_t autoincrement,
						const char *table,
						uint16_t tablesize) {

	debugWrite("%.*s:%hd:%d (%d,%d) %s%s%s",
				namesize,name,type,size,precision,scale,
				(nullable)?"":"NOT NULL ",
				(primarykey)?"Primary key ":"",
				(unique)?"Unique":"");

	clientsock->write(namesize);
	clientsock->write(name,namesize);
	clientsock->write(type);
	clientsock->write(size);
	clientsock->write(precision);
	clientsock->write(scale);
	clientsock->write(nullable);
	clientsock->write(primarykey);
	clientsock->write(unique);
	clientsock->write(partofkey);
	clientsock->write(unsignednumber);
	clientsock->write(zerofill);
	clientsock->write(binary);
	clientsock->write(autoincrement);

	if (protocolversion<2) {
		return;
	}

	clientsock->write(tablesize);
	clientsock->write(table,tablesize);
}

void sqlrprotocol_sqlrclient::sendColumnDefinitionString(
						const char *name,
						uint16_t namesize,
						const char *type, 
						uint16_t typesize,
						uint32_t size,
						uint32_t precision,
						uint32_t scale,
						uint16_t nullable,
						uint16_t primarykey,
						uint16_t unique,
						uint16_t partofkey,
						uint16_t unsignednumber,
						uint16_t zerofill,
						uint16_t binary,
						uint16_t autoincrement,
						const char *table,
						uint16_t tablesize) {

	debugWrite("%.*s:%.*s:%d (%d,%d) %s%s%s",
				namesize,name,typesize,type,
				size,precision,scale,
				(nullable)?"":"NOT NULL ",
				(primarykey)?"Primary key ":"",
				(unique)?"Unique":"");

	clientsock->write(namesize);
	clientsock->write(name,namesize);
	clientsock->write(typesize);
	clientsock->write(type,typesize);
	clientsock->write(size);
	clientsock->write(precision);
	clientsock->write(scale);
	clientsock->write(nullable);
	clientsock->write(primarykey);
	clientsock->write(unique);
	clientsock->write(partofkey);
	clientsock->write(unsignednumber);
	clientsock->write(zerofill);
	clientsock->write(binary);
	clientsock->write(autoincrement);

	if (protocolversion<2) {
		return;
	}

	clientsock->write(tablesize);
	clientsock->write(table,tablesize);
}

bool sqlrprotocol_sqlrclient::returnResultSetData(sqlrservercursor *cursor,
						bool getskipandfetch,
						bool overridelazyfetch) {

	debugStart("returning result set data");

	// FIXME: push up?
	cont->setState(RETURN_RESULT_SET);

	// decide whether to use the cursor itself
	// or an attached custom query cursor
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		cursor=customcursor;
	}

	// get the number of rows to skip and fetch
	if (getskipandfetch) {
		if (!getSkipAndFetch(false,cursor)) {
			debugEnd();
			return false;
		}
	}

	// reinit cursor state (in case it was suspended)
	// FIXME: push up?
	cont->setState(cursor,SQLRCURSORSTATE_BUSY);

	if (!lazyfetch || overridelazyfetch) {

		bool	error=false;

		// for some queries, there are no rows to return, 
		if (cont->noRowsToReturn(cursor)) {
			clientsock->write(endresultset);
			clientsock->flushWriteBuffer(-1,-1);
			debugEnd();
			return true;
		}

		// skip the specified number of rows
		if (!cont->skipRows(cursor,skip,&error)) {
			if (error) {
				returnFetchError(cursor);
			} else {
				clientsock->write(endresultset);
				debugEnd();
			}
			clientsock->flushWriteBuffer(-1,-1);
			debugEnd();
			return true;
		}

		debugWrite("fetching %lld rows...",(long long)fetch);

		// send the specified number of rows back
		for (uint64_t i=0; (!fetch || i<fetch); i++) {
			if (cont->fetchRow(cursor,&error)) {
				returnRow(cursor);
				// FIXME: kludgy
				cont->nextRow(cursor);
			} else {
				if (error && protocolversion>=2) {
					returnFetchError(cursor);
				} else {
					clientsock->write(endresultset);
				}
				break;
			}
		}
	}
	clientsock->flushWriteBuffer(-1,-1);

	debugEnd();
	return true;
}

void sqlrprotocol_sqlrclient::returnFetchError(sqlrservercursor *cursor) {

	clientsock->write((uint16_t)FETCH_ERROR);

	debugStart("returning error");

	// FIXME: this is a little kludgy, ideally we'd just call returnError()
	// but it has some side effects

	// get the error
	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(cursor,&errorstring,&errorsize,
					&errnum,&liveconnection);

	debugWrite("error number: %lld",(long long)errnum);
	debugWrite("error string: %.*s",errorsize,errorstring);

	// send the error status
	if (!liveconnection) {
		clientsock->write((uint16_t)ERROR_OCCURRED_DISCONNECT);
	} else {
		clientsock->write((uint16_t)ERROR_OCCURRED);
	}

	// send the error code
	clientsock->write((uint64_t)errnum);

	// send the error string
	clientsock->write((uint16_t)errorsize);
	clientsock->write(errorstring,errorsize);

	debugEnd();
}

void sqlrprotocol_sqlrclient::returnRow(sqlrservercursor *cursor) {

	if (cont->getLoggingEnabled() || cont->getNotificationsEnabled()) {
		debugstr.clear();
	}

	// send fields
	uint32_t	colcount=cont->colCount(cursor);
	for (uint32_t i=0; i<colcount; i++) {

		const char	*field=NULL;
		uint64_t	fieldsize=0;
		bool		lob=false;
		bool		null=false;
		if (!cont->getField(cursor,i,&field,&fieldsize,&lob,&null)) {
			// FIXME: handle error
		}

		// send data to the client
		if (null) {
			sendNullField();
		} else if (lob) {
			sendLobField(cursor,i);
		} else {
			sendField(field,fieldsize);
		}
	}

	if (cont->getLoggingEnabled() || cont->getNotificationsEnabled()) {
		debugWrite("%s",debugstr.getString());
	}
}

void sqlrprotocol_sqlrclient::sendField(const char *data, uint32_t size) {

	if (cont->getLoggingEnabled() || cont->getNotificationsEnabled()) {
		debugstr.append("\"");
		debugstr.append(data,size);
		debugstr.append("\",");
	}

	clientsock->write((uint16_t)STRING_DATA);
	clientsock->write(size);
	clientsock->write(data,size);
}

void sqlrprotocol_sqlrclient::sendNullField() {

	if (cont->getLoggingEnabled() || cont->getNotificationsEnabled()) {
		debugstr.append("NULL,");
	}
	clientsock->write((uint16_t)NULL_DATA);
}

#define MAX_BYTES_PER_CHAR	4

void sqlrprotocol_sqlrclient::sendLobField(sqlrservercursor *cursor,
							uint32_t col) {

	// Get lob length.  If this fails, send a NULL field.
	uint64_t	loblength;
	if (!cont->getLobFieldLength(cursor,col,&loblength)) {
		sendNullField();
		cont->closeLobField(cursor,col);
		return;
	}

	// for lobs of 0 length
	if (!loblength) {
		startSendingLong(0);
		sendLongSegment("",0);
		endSendingLong();
		cont->closeLobField(cursor,col);
		return;
	}

	// initialize sizes and status
	uint64_t	charstoread=sizeof(lobbuffer)/MAX_BYTES_PER_CHAR;
	uint64_t	charsread=0;
	uint64_t	offset=0;
	bool		start=true;

	for (;;) {

		// read a segment from the lob
		if (!cont->getLobFieldSegment(cursor,col,
					lobbuffer,sizeof(lobbuffer),
					offset,charstoread,&charsread) ||
					!charsread) {

			// if we fail to get a segment or got nothing...
			// if we haven't started sending yet, then send a NULL,
			// otherwise just end normally
			if (start) {
				sendNullField();
			} else {
				endSendingLong();
			}
			cont->closeLobField(cursor,col);
			return;

		} else {

			// if we haven't started sending yet, then do that now
			if (start) {
				startSendingLong(loblength);
				start=false;
			}

			// send the segment we just got
			sendLongSegment(lobbuffer,charsread);

			// FIXME: or should this be charsread?
			offset=offset+charstoread;
		}
	}
}

void sqlrprotocol_sqlrclient::startSendingLong(uint64_t longsize) {
	clientsock->write((uint16_t)START_LONG_DATA);
	clientsock->write(longsize);
}

void sqlrprotocol_sqlrclient::sendLongSegment(const char *data, uint32_t size) {

	if (cont->getLoggingEnabled() || cont->getNotificationsEnabled()) {
		debugstr.append(data,size);
	}

	clientsock->write((uint16_t)STRING_DATA);
	clientsock->write(size);
	clientsock->write(data,size);
}

void sqlrprotocol_sqlrclient::endSendingLong() {

	if (cont->getLoggingEnabled() || cont->getNotificationsEnabled()) {
		debugstr.append(",");
	}

	clientsock->write((uint16_t)END_LONG_DATA);
}

void sqlrprotocol_sqlrclient::returnError(bool forcedisconnect) {

	debugStart("returning error");

	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(&errorstring,&errorsize,&errnum,&liveconnection);

	debugWrite("error number: %lld",(long long)errnum);
	debugWrite("error string: %.*s",errorsize,errorstring);

	// send the appropriate error status
	if (forcedisconnect || !liveconnection) {
		clientsock->write((uint16_t)ERROR_OCCURRED_DISCONNECT);
	} else {
		clientsock->write((uint16_t)ERROR_OCCURRED);
	}

	// send the error code and error string
	clientsock->write((uint64_t)errnum);

	// send the error string
	clientsock->write((uint16_t)errorsize);
	clientsock->write(errorstring,errorsize);
	clientsock->flushWriteBuffer(-1,-1);

	cont->raiseDbErrorEvent(NULL,errorstring);

	debugEnd();
}

void sqlrprotocol_sqlrclient::returnError(sqlrservercursor *cursor,
						bool forcedisconnect) {

	debugStart("returning error");

	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(cursor,&errorstring,&errorsize,
					&errnum,&liveconnection);

	debugWrite("error number: %lld",(long long)errnum);
	debugWrite("error string: %.*s",errorsize,errorstring);

	// send the appropriate error status
	if (forcedisconnect || !liveconnection) {
		clientsock->write((uint16_t)ERROR_OCCURRED_DISCONNECT);
	} else {
		clientsock->write((uint16_t)ERROR_OCCURRED);
	}

	// send the error code
	clientsock->write((uint64_t)errnum);

	// send the error string
	clientsock->write((uint16_t)errorsize);
	clientsock->write(errorstring,errorsize);

	// client will be sending skip/fetch, better get
	// it even though we're not going to use it
	uint64_t	skipfetch;
	clientsock->read(&skipfetch,idleclienttimeout,0);
	clientsock->read(&skipfetch,idleclienttimeout,0);

	// Even though there was an error, we still 
	// need to send the client the id of the 
	// cursor that it's going to use.
	clientsock->write(cont->getId(cursor));
	clientsock->flushWriteBuffer(-1,-1);

	cont->raiseDbErrorEvent(cursor,errorstring);

	debugEnd();
}

bool sqlrprotocol_sqlrclient::fetchResultSetCommand(
					sqlrservercursor *cursor) {
	debugStart("fetching result set");
	bool	retval=returnResultSetData(cursor,true,true);
	debugWrite((retval)?"success":"error");
	debugEnd();
	return retval;
}

void sqlrprotocol_sqlrclient::abortResultSetCommand(
					sqlrservercursor *cursor) {
	debugStart("aborting result set");
	cont->abort(cursor);
	cont->release(cursor);
	debugWrite("success");
	debugEnd();
}

void sqlrprotocol_sqlrclient::suspendResultSetCommand(
					sqlrservercursor *cursor) {
	debugStart("suspend result set");
	cont->suspendResultSet(cursor);
	debugWrite("success");
	debugEnd();
}

bool sqlrprotocol_sqlrclient::resumeResultSetCommand(
					sqlrservercursor *cursor) {
	debugStart("resume result set");

	bool	retval=true;

	if (cont->getState(cursor)==SQLRCURSORSTATE_SUSPENDED) {

		debugWrite("previous result set was suspended");

		// indicate that no error has occurred
		clientsock->write((uint16_t)NO_ERROR_OCCURRED);

		// send the client the id of the 
		// cursor that it's going to use
		clientsock->write(cont->getId(cursor));
		clientsock->write((uint16_t)SUSPENDED_RESULT_SET);

		// if the requested cursor really had a suspended
		// result set, send the index of the last row that
		// was fetched to the client then resume the result set
		// (FIXME: 0 will be sent if no rows were fetched or if
		// only the first row was fetched. This probably isn't
		// correct.)
		uint64_t	totalrowsfetched=
				cont->getTotalRowsFetched(cursor);
		clientsock->write((totalrowsfetched)?totalrowsfetched-1:0);

		returnResultSetHeader(cursor,false);
		retval=returnResultSetData(cursor,true,false);

	} else {

		debugWrite("previous result set was not suspended");

		// indicate that an error has occurred
		clientsock->write((uint16_t)ERROR_OCCURRED);

		// send the error code (zero for now)
		clientsock->write((uint64_t)SQLR_ERROR_RESULTSETNOTSUSPENDED);

		// send the error itself
		uint16_t	size=charstring::getLength(
				SQLR_ERROR_RESULTSETNOTSUSPENDED_STRING);
		clientsock->write(size);
		clientsock->write(SQLR_ERROR_RESULTSETNOTSUSPENDED_STRING,size);

		retval=false;
	}

	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getDatabaseListCommand(
					sqlrservercursor *cursor) {
	debugStart("get database list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_DATABASE_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getCatalogListCommand(
					sqlrservercursor *cursor) {
	debugStart("get catalog list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_CATALOG_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getSchemaListCommand(
					sqlrservercursor *cursor) {
	debugStart("get schema list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_SCHEMA_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getTableListCommand(
					sqlrservercursor *cursor) {
	debugStart("get table list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_TABLE_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getTableList2Command(
					sqlrservercursor *cursor) {
	debugStart("get table list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_TABLE_LIST_2);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getTableTypeListCommand(
					sqlrservercursor *cursor) {
	debugStart("get table type list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_TABLE_TYPE_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getColumnListCommand(
					sqlrservercursor *cursor) {
	debugStart("get column list");
	bool	retval=getComponentListCommand(cursor,
				SQLRCLIENTQUERYTYPE_COLUMN_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getPrimaryKeysListCommand(
					sqlrservercursor *cursor) {
	debugStart("get primary key list");
	bool	retval=getComponentListCommand(cursor,
				SQLRCLIENTQUERYTYPE_PRIMARY_KEYS_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getKeyAndIndexListCommand(
					sqlrservercursor *cursor) {
	debugStart("get key and index list");
	bool	retval=getComponentListCommand(cursor,
				SQLRCLIENTQUERYTYPE_KEY_AND_INDEX_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getProcedureParameterListCommand(
						sqlrservercursor *cursor) {
	debugStart("get procedure bind and column list");
	bool	retval=getComponentListCommand(cursor,
		SQLRCLIENTQUERYTYPE_PROCEDURE_PARAMETER_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getTypeInfoListCommand(
					sqlrservercursor *cursor) {
	debugStart("get type info list");
	bool	retval=getComponentListCommand(cursor,
				SQLRCLIENTQUERYTYPE_TYPE_INFO_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getProcedureListCommand(
					sqlrservercursor *cursor) {
	debugStart("get procedure list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_PROCEDURE_LIST);
	debugEnd();
	return retval;
}

bool sqlrprotocol_sqlrclient::getLastInsertIdListCommand(
					sqlrservercursor *cursor) {
	debugStart("get last insert id list");
	bool	retval=getObjectListCommand(cursor,
				SQLRCLIENTQUERYTYPE_LAST_INSERT_ID_LIST);
	debugEnd();
	return retval;
}


bool sqlrprotocol_sqlrclient::getObjectListCommand(sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype) {

	// if we're using a custom cursor then close it
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		customcursor->close();
		cursor->clearCustomQueryCursor();
	}

	// get the list format
	uint16_t	listformat;
	ssize_t		result=clientsock->read(&listformat,
						idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get list format");
		return false;
	}

	debugListFormat((sqlrserverlistformat_t)listformat);

	// get size of object parameter
	uint32_t	objectsize;
	result=clientsock->read(&objectsize,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get object size");
		return false;
	}

	// bounds checking
	if (objectsize>maxquerysize) {
		cont->raiseClientProtocolErrorEvent(cursor,1,
					"get list failed: "
					"object size too large: %d",
					objectsize);
		return false;
	}

	// read the object parameter into the buffer
	char	*object=new char[objectsize+1];
	if (objectsize) {
		result=clientsock->read(object,objectsize,idleclienttimeout,0);
		if ((uint32_t)result!=objectsize) {
			cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get object parameter");
			return false;
		}
	}
	object[objectsize]='\0';

	// some apps aren't well behaved, trim spaces off of both sides
	charstring::bothTrim(object);

	debugWrite("object: %.*s",objectsize,object);

	// read the object types
	uint16_t	objecttypes=0;
	if (querytype==SQLRCLIENTQUERYTYPE_TABLE_LIST_2) {

		result=clientsock->read(&objecttypes,idleclienttimeout,0);
		if (result!=sizeof(uint16_t)) {
			cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get object types");
			return false;
		}

		debugWrite("object types: %hd",objecttypes);
	}

	// set the values that we won't get from the client
	cont->setInputBindCount(cursor,0);
	cont->setOutputBindCount(cursor,0);
	cont->setInputOutputBindCount(cursor,0);
	cont->setSendColumnInfo(true);

	// get the list and return it
	bool	retval=getObjectList(cursor,querytype,object,
					(sqlrserverlistformat_t)listformat,
					objecttypes);

	// clean up
	delete[] object;

	return retval;
}

bool sqlrprotocol_sqlrclient::getObjectList(sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype,
					const char *object,
					sqlrserverlistformat_t listformat,
					uint16_t objecttypes) {

	// initialize flags and buffers
	bool	success=false;

	// clean up object to avoid SQL injection
	stringbuffer	objectbuf;
	escapeParameter(&objectbuf,object);
	object=objectbuf.getString();

	// split the object (catalog.schema.object) into
	// catalog, schema, and object
	char	*currentcatalog=cont->getCurrentCatalog();
	char	*currentschema=cont->getCurrentSchema();
	const char	*objecttype=NULL;
	switch (querytype) {
		case SQLRCLIENTQUERYTYPE_DATABASE_LIST:
			objecttype="database";
			break;
		case SQLRCLIENTQUERYTYPE_CATALOG_LIST:
			objecttype="catalog";
			break;
		case SQLRCLIENTQUERYTYPE_SCHEMA_LIST:
			objecttype="schema";
			break;
		default:
			objecttype="object";
			break;
	}
	const char	*catalog=NULL;
	const char	*schema=NULL;
	const char	*obj=NULL;
	cont->splitObjectName(currentcatalog,currentschema,
					objecttype,object,
					&catalog,&schema,&obj);

	if (listformat==SQLRSERVERLISTFORMAT_MYSQL) {

		// when fetching lists in mysql format...
		switch (querytype) {
			case SQLRCLIENTQUERYTYPE_CATALOG_LIST:
				// fetch all catalogs
				break;
			case SQLRCLIENTQUERYTYPE_SCHEMA_LIST:
				// only fetch for the current catalog
				catalog=currentcatalog;
				break;
			default:
				// only fetch for the current catalog/schema
				catalog=currentcatalog;
				schema=currentschema;
				break;
		}
	}

	// get the appropriate list and set the list format
	// (set*ListFormat must be called after get*List because prepareQuery
	// inside the default get*List resets the column map)
	switch (querytype) {
		case SQLRCLIENTQUERYTYPE_DATABASE_LIST:
			success=cont->getDatabaseList(cursor,obj);
			cont->setDatabaseListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_CATALOG_LIST:
			success=cont->getCatalogList(cursor,catalog);
			cont->setCatalogListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_SCHEMA_LIST:
			success=cont->getSchemaList(cursor,catalog,schema);
			cont->setSchemaListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_TABLE_LIST:
		case SQLRCLIENTQUERYTYPE_TABLE_LIST_2:
			success=cont->getTableList(cursor,catalog,schema,
							obj,objecttypes);
			cont->setTableListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_TABLE_TYPE_LIST:
			success=cont->getTableTypeList(cursor,
							catalog,schema,obj);
			cont->setTableTypeListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_PROCEDURE_LIST:
			success=cont->getProcedureList(cursor,
							catalog,schema,obj);
			cont->setProcedureListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_LAST_INSERT_ID_LIST:
			success=cont->getLastInsertIdList(cursor);
			break;
		default:
			break;
	}

	// clean up
	delete[] currentcatalog;
	delete[] currentschema;

	if (success) {
		success=getSkipAndFetch(true,cursor);
	}

	// if an error occurred...
	if (!success) {
		returnError(cursor,false);

		// this is actually OK, only return false on a network error
		return true;
	}

	// indicate that no error has occurred
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);

	// send the client the id of the 
	// cursor that it's going to use
	clientsock->write(cont->getId(cursor));

	// tell the client that this is not a
	// suspended result set
	clientsock->write((uint16_t)NO_SUSPENDED_RESULT_SET);

	// if the query processed ok then send a result set header and return...
	returnResultSetHeader(cursor,false);
	if (!returnResultSetData(cursor,false,false)) {
		return false;
	}
	return true;
}

bool sqlrprotocol_sqlrclient::getComponentListCommand(
					sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype) {

	// if we're using a custom cursor then close it
	// FIXME: push up?
	sqlrservercursor	*customcursor=cursor->getCustomQueryCursor();
	if (customcursor) {
		customcursor->close();
		cursor->clearCustomQueryCursor();
	}

	// get the list format
	uint16_t	listformat;
	ssize_t		result=clientsock->read(&listformat,
						idleclienttimeout,0);
	if (result!=sizeof(uint16_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get list format");
		return false;
	}

	debugListFormat((sqlrserverlistformat_t)listformat);

	// get size of component parameter
	uint32_t	componentsize;
	result=clientsock->read(&componentsize,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get component size");
		return false;
	}

	// bounds checking
	if (componentsize>maxquerysize) {
		cont->raiseClientProtocolErrorEvent(cursor,1,
					"get list failed: "
					"component size too large: %d",
					componentsize);
		return false;
	}

	// read the component parameter into the buffer
	char	*component=new char[componentsize+1];
	if (componentsize) {
		result=clientsock->read(component,
					componentsize,
					idleclienttimeout,0);
		if ((uint32_t)result!=componentsize) {
			cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get component parameter");
			return false;
		}
	}
	component[componentsize]='\0';

	// some apps aren't well behaved, trim spaces off of both sides
	charstring::bothTrim(component);

	debugWrite("component: %.*s",componentsize,component);

	char	*object=NULL;

	// get size of object parameter
	uint32_t	objectsize;
	result=clientsock->read(&objectsize,idleclienttimeout,0);
	if (result!=sizeof(uint32_t)) {
		cont->raiseClientProtocolErrorEvent(cursor,result,
					"get list failed: "
					"failed to get object size");
		return false;
	}

	// bounds checking
	if (objectsize>maxquerysize) {
		cont->raiseClientProtocolErrorEvent(cursor,1,
					"get list failed: "
					"object size too large: %d",
					objectsize);
		return false;
	}

	// read the object parameter into the buffer
	object=new char[objectsize+1];
	if (objectsize) {
		result=clientsock->read(object,objectsize,
					idleclienttimeout,0);
		if ((uint32_t)result!=objectsize) {
			cont->raiseClientProtocolErrorEvent(
					cursor,result,
					"get list failed: "
					"failed to get object parameter");
			return false;
		}
	}
	object[objectsize]='\0';

	// some apps aren't well behaved, trim spaces off of both sides
	charstring::bothTrim(object);

	// translate object name, if necessary
	const char	*newobject=NULL;
	if (cont->getReplacementTableName(NULL,NULL,
					object,&newobject) &&
					newobject) {
		delete[] object;
		object=charstring::duplicate(newobject);
	}

	debugWrite("object: %s",object);

	// set the values that we won't get from the client
	cont->setInputBindCount(cursor,0);
	cont->setOutputBindCount(cursor,0);
	cont->setInputOutputBindCount(cursor,0);
	cont->setSendColumnInfo(true);

	// get the list and return it
	bool	retval=getComponentList(cursor,querytype,
					object,component,
					(sqlrserverlistformat_t)listformat);

	// clean up
	delete[] component;
	delete[] object;

	return retval;
}

bool sqlrprotocol_sqlrclient::getComponentList(
					sqlrservercursor *cursor,
					sqlrclientquerytype_t querytype,
					const char *object,
					const char *component,
					sqlrserverlistformat_t listformat) {

	// initialize flags and buffers
	bool	success=false;

	// clean up object and component to avoid SQL injection
	stringbuffer	objectbuf;
	escapeParameter(&objectbuf,object);
	object=objectbuf.getString();
	stringbuffer	componentbuf;
	escapeParameter(&componentbuf,component);
	component=componentbuf.getString();

	// split the object (catalog.schema.object) into
	// catalog, schema, and object
	char		*currentcatalog=cont->getCurrentCatalog();
	char		*currentschema=cont->getCurrentSchema();
	const char	*catalog=NULL;
	const char	*schema=NULL;
	const char	*obj=NULL;
	cont->splitObjectName(currentcatalog,currentschema,"object",object,
							&catalog,&schema,&obj);

	// when fetching lists in mysql format, we only want to fetch for
	// the current database/schema
	if (listformat==SQLRSERVERLISTFORMAT_MYSQL) {
		catalog=currentcatalog;
		schema=currentschema;
	}

	// get the appropriate list and set the list format
	// (set*ListFormat must be called after get*List because prepareQuery
	// inside the default get*List resets the column map)
	switch (querytype) {
		case SQLRCLIENTQUERYTYPE_COLUMN_LIST:
			success=cont->getColumnList(cursor,
						catalog,schema,obj,component);
			cont->setColumnListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_PRIMARY_KEYS_LIST:
			success=cont->getPrimaryKeysList(cursor,
							catalog,schema,obj);
			cont->setPrimaryKeyListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_KEY_AND_INDEX_LIST:
			success=cont->getKeyAndIndexList(cursor,
							catalog,schema,obj);
			cont->setKeyAndIndexListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_PROCEDURE_PARAMETER_LIST:
			success=cont->getProcedureParameterList(cursor,
							catalog,schema,obj);
			cont->setProcedureParameterListFormat(listformat);
			break;
		case SQLRCLIENTQUERYTYPE_TYPE_INFO_LIST:
			success=cont->getTypeInfoList(cursor,
							catalog,schema,obj);
			cont->setTypeInfoListFormat(listformat);
			break;
		default:
			break;
	}

	// clean up
	delete[] currentcatalog;
	delete[] currentschema;

	if (success) {
		success=getSkipAndFetch(true,cursor);
	}

	// if an error occurred...
	if (!success) {
		returnError(cursor,false);

		// this is actually OK, only return false on a network error
		return true;
	}

	// indicate that no error has occurred
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);

	// send the client the id of the
	// cursor that it's going to use
	clientsock->write(cont->getId(cursor));

	// tell the client that this is not a
	// suspended result set
	clientsock->write((uint16_t)NO_SUSPENDED_RESULT_SET);

	// if the query processed ok then send a result set header and return...
	returnResultSetHeader(cursor,false);
	if (!returnResultSetData(cursor,false,false)) {
		return false;
	}
	return true;
}

void sqlrprotocol_sqlrclient::escapeParameter(stringbuffer *buffer,
						const char *parameter) {

	if (!parameter) {
		return;
	}

	// escape single quotes
	for (const char *ptr=parameter; *ptr; ptr++) {
		if (*ptr=='\'') {
			buffer->append('\'');
		}
		buffer->append(*ptr);
	}
}

bool sqlrprotocol_sqlrclient::getQueryTreeCommand(sqlrservercursor *cursor) {

	debugStart("getting query tree");

	// get the tree as a string
	xmldom	*tree=cont->getQueryTree(cursor);
	domnode	*root=(tree)?tree->getRootNode():NULL;
	stringbuffer	xml;
	if (root) {
		root->write(&xml);
	}

	// send the tree
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write((uint64_t)xml.getSize());
	clientsock->write(xml.getString(),xml.getSize());
	clientsock->flushWriteBuffer(-1,-1);

	debugEnd();
	return true;
}

bool sqlrprotocol_sqlrclient::getTranslatedQueryCommand(
					sqlrservercursor *cursor) {

	debugStart("getting translated query");

	// get the query
	const char	*query=cont->getTranslatedQuery(cursor);
	uint64_t	querysize=charstring::getLength(query);

	// send the tree
	clientsock->write((uint16_t)NO_ERROR_OCCURRED);
	clientsock->write(querysize);
	clientsock->write(query,querysize);
	clientsock->flushWriteBuffer(-1,-1);

	debugEnd();
	return true;
}

void sqlrprotocol_sqlrclient::debugCommand(uint16_t command) {
	debugWrite("command: %hd",command);
	switch (command) {
		case SQLRCLIENT_PROTOCOL_VERSION:
			debugWrite("SQLRCLIENT_PROTOCOL_VERSION");
			break;
		case NEW_QUERY:
			debugWrite("NEW_QUERY");
			break;
		case FETCH_RESULT_SET:
			debugWrite("FETCH_RESULT_SET");
			break;
		case ABORT_RESULT_SET:
			debugWrite("ABORT_RESULT_SET");
			break;
		case SUSPEND_RESULT_SET:
			debugWrite("SUSPEND_RESULT_SET");
			break;
		case RESUME_RESULT_SET:
			debugWrite("RESUME_RESULT_SET");
			break;
		case SUSPEND_SESSION:
			debugWrite("SUSPEND_SESSION");
			break;
		case END_SESSION:
			debugWrite("END_SESSION");
			break;
		case PING:
			debugWrite("PING");
			break;
		case IDENTIFY:
			debugWrite("IDENTIFY");
			break;
		case COMMIT:
			debugWrite("COMMIT");
			break;
		case ROLLBACK:
			debugWrite("ROLLBACK");
			break;
		case GET_IN_TRANSACTION:
			debugWrite("GET_IN_TRANSACTION");
			break;
		case AUTH:
			debugWrite("AUTH");
			break;
		case SET_AUTO_COMMIT:
			debugWrite("SET_AUTO_COMMIT");
			break;
		case GET_AUTO_COMMIT:
			debugWrite("GET_AUTO_COMMIT");
			break;
		case RE_EXECUTE_QUERY:
			debugWrite("RE_EXECUTE_QUERY");
			break;
		case FETCH_FROM_BIND_CURSOR:
			debugWrite("FETCH_FROM_BIND_CURSOR");
			break;
		case DB_VERSION:
			debugWrite("DB_VERSION");
			break;
		case BIND_FORMAT:
			debugWrite("BIND_FORMAT");
			break;
		case SERVER_VERSION:
			debugWrite("SERVER_VERSION");
			break;
		case GET_DATABASE_LIST:
			debugWrite("GET_DATABASE_LIST");
			break;
		case GET_CATALOG_LIST:
			debugWrite("GET_CATALOG_LIST");
			break;
		case GET_TABLE_LIST:
			debugWrite("GET_TABLE_LIST");
			break;
		case GET_COLUMN_LIST:
			debugWrite("GET_COLUMN_LIST");
			break;
		case SELECT_DATABASE:
			debugWrite("SELECT_DATABASE");
			break;
		case GET_CURRENT_DATABASE:
			debugWrite("GET_CURRENT_DATABASE");
			break;
		case GET_LAST_INSERT_ID:
			debugWrite("GET_LAST_INSERT_ID");
			break;
		case BEGIN:
			debugWrite("BEGIN");
			break;
		case GET_QUERY_TREE:
			debugWrite("GET_QUERY_TREE");
			break;
		case NO_COMMAND:
			debugWrite("NO_COMMAND");
			break;
		case DB_HOST_NAME:
			debugWrite("DB_HOST_NAME");
			break;
		case DB_IP_ADDRESS:
			debugWrite("DB_IP_ADDRESS");
			break;
		case GET_TRANSLATED_QUERY:
			debugWrite("GET_TRANSLATED_QUERY");
			break;
		case GET_PROCEDURE_BIND_AND_COLUMN_LIST:
			debugWrite("GET_PROCEDURE_BIND_AND_COLUMN_LIST");
			break;
		case GET_TYPE_INFO_LIST:
			debugWrite("GET_TYPE_INFO_LIST");
			break;
		case GET_PROCEDURE_LIST:
			debugWrite("GET_PROCEDURE_LIST");
			break;
		case GET_SCHEMA_LIST:
			debugWrite("GET_SCHEMA_LIST");
			break;
		case GET_TABLE_TYPE_LIST:
			debugWrite("GET_TABLE_TYPE_LIST");
			break;
		case GET_PRIMARY_KEYS_LIST:
			debugWrite("GET_PRIMARY_KEYS_LIST");
			break;
		case GET_KEY_AND_INDEX_LIST:
			debugWrite("GET_KEY_AND_INDEX_LIST");
			break;
		case SELECT_SCHEMA:
			debugWrite("SELECT_SCHEMA");
			break;
		case GET_CURRENT_SCHEMA:
			debugWrite("GET_CURRENT_SCHEMA");
			break;
		case NEXT_RESULT_SET:
			debugWrite("NEXT_RESULT_SET");
			break;
		case GET_TABLE_LIST_2:
			debugWrite("GET_TABLE_LIST_2");
			break;
		case NEXT_VAL_FORMAT:
			debugWrite("NEXT_VAL_FORMAT");
			break;
		case SET_ISOLATION_LEVEL:
			debugWrite("SET_ISOLATION_LEVEL");
			break;
		case GET_ISOLATION_LEVEL:
			debugWrite("GET_ISOLATION_LEVEL");
			break;
		case SET_TRANSACTION_MODEL:
			debugWrite("SET_TRANSACTION_MODEL");
			break;
		case GET_TRANSACTION_MODEL:
			debugWrite("GET_TRANSACTION_MODEL");
			break;
		case GET_DEFAULT_TRANSACTION_MODEL:
			debugWrite("GET_DEFAULT_TRANSACTION_MODEL");
			break;
		case GET_LAST_INSERT_ID_LIST:
			debugWrite("GET_LAST_INSERT_ID_LIST");
			break;
		case SELECT_CATALOG:
			debugWrite("SELECT_CATALOG");
			break;
		case GET_CURRENT_CATALOG:
			debugWrite("GET_CURRENT_CATALOG");
			break;
		case GET_DATABASE_IS_SCHEMA:
			debugWrite("GET_DATABASE_IS_SCHEMA");
			break;
		default:
			debugWrite("bad command");
			break;
	}
}

void sqlrprotocol_sqlrclient::debugListFormat(
				sqlrserverlistformat_t listformat) {
	debugWrite("list format: %hd",listformat);
	switch (listformat) {
		case SQLRSERVERLISTFORMAT_NULL:
			debugWrite("NULL");
			break;
		case SQLRSERVERLISTFORMAT_MYSQL:
			debugWrite("MYSQL");
			break;
		case SQLRSERVERLISTFORMAT_ODBC:
			debugWrite("ODBC");
			break;
		case SQLRSERVERLISTFORMAT_JDBC:
			debugWrite("JDBC");
			break;
		default:
			debugWrite("unknown");
			break;
	}
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrprotocol	*new_sqlrprotocol_sqlrclient(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrprotocol_sqlrclient(cont,parameters);
	}
}

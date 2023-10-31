// Copyright (c) 1999-2019 David Muse
// See the file COPYING for more information

#ifndef SQLRSERVER_H
#define SQLRSERVER_H

#include <sqlrelay/private/sqlrserverincludes.h>

enum sqlrcursorstate_t {
	SQLRCURSORSTATE_AVAILABLE=0,
	SQLRCURSORSTATE_BUSY,
	SQLRCURSORSTATE_SUSPENDED
};

enum sqlrquerytype_t {
	SQLRQUERYTYPE_SELECT=0,
	SQLRQUERYTYPE_INSERT,
	SQLRQUERYTYPE_INSERTSELECT,
	SQLRQUERYTYPE_SELECTINTO,
	SQLRQUERYTYPE_MULTIINSERT,
	SQLRQUERYTYPE_UPDATE,
	SQLRQUERYTYPE_DELETE,
	SQLRQUERYTYPE_CREATE,
	SQLRQUERYTYPE_DROP,
	SQLRQUERYTYPE_ALTER,
	SQLRQUERYTYPE_CUSTOM,
	SQLRQUERYTYPE_ETC,
	SQLRQUERYTYPE_BEGIN,
	SQLRQUERYTYPE_COMMIT,
	SQLRQUERYTYPE_ROLLBACK,
	SQLRQUERYTYPE_AUTOCOMMIT_ON,
	SQLRQUERYTYPE_AUTOCOMMIT_OFF,
	SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_ON,
	SQLRQUERYTYPE_SET_INCLUDING_AUTOCOMMIT_OFF
};

enum sqlrquerystatus_t {
	SQLRQUERYSTATUS_SUCCESS=0,
	SQLRQUERYSTATUS_ERROR,
	SQLRQUERYSTATUS_FILTER_VIOLATION
};

enum sqlrserverbindvartype_t {
	SQLRSERVERBINDVARTYPE_NULL=0,
	SQLRSERVERBINDVARTYPE_STRING,
	SQLRSERVERBINDVARTYPE_INTEGER,
	SQLRSERVERBINDVARTYPE_DOUBLE,
	SQLRSERVERBINDVARTYPE_BLOB,
	SQLRSERVERBINDVARTYPE_CLOB,
	SQLRSERVERBINDVARTYPE_CURSOR,
	SQLRSERVERBINDVARTYPE_DATE,

	// special types for bulk load
	SQLRSERVERBINDVARTYPE_DELIMITER,
	SQLRSERVERBINDVARTYPE_NEWLINE
};

enum sqlrserverlistformat_t {
	SQLRSERVERLISTFORMAT_NULL=0,
	SQLRSERVERLISTFORMAT_MYSQL,
	SQLRSERVERLISTFORMAT_POSTGRESQL,
	SQLRSERVERLISTFORMAT_ODBC,
	SQLRSERVERLISTFORMAT_JDBC
};

class SQLRSERVER_DLLSPEC sqlrserverbindvar {
	public:
		char	*variable;
		int16_t	variablesize;
		union {
			char	*stringval;
			int64_t	integerval;
			struct	{
				double		value;
				uint32_t	precision;
				uint32_t	scale;
			} doubleval;
			struct {
				int16_t		year;
				int16_t		month;
				int16_t		day;
				int16_t		hour;
				int16_t		minute;
				int16_t		second;
				int32_t		microsecond;
				char		*tz;
				bool		isnegative;
				char		*buffer;
				uint16_t	buffersize;
			} dateval;
			uint16_t	cursorid;
		} value;
		uint32_t		valuesize;
		uint32_t		resultvaluesize;
		sqlrserverbindvartype_t	type;
		byte_t			nativetype;
		int16_t			isnull;
};

class SQLRSERVER_DLLSPEC sqlrlistener {
	public:
		const char	*getId();
		sqlrpaths	*getPaths();

	#include <sqlrelay/private/sqlrlistener.h>
};

class SQLRSERVER_DLLSPEC sqlrservercontroller {
	public:
		// connection api...



		// connect string...

		/** Returns the value of parameter "variable", of the string
		 *  attribute, of the connection tag, in the config file. */
		const char	*getConnectStringValue(const char *variable);

		/** Sets the user that will be used to connect to the database
		 *  to "user".
		 * 
		 *  During initialization of the sqlr-connection, this is set
		 *  to the value of the "user" parameter, of the string
		 *  attribute, of the connection tag, in the config file.
		 *
		 *  This method may be called by a module to override that. */
		void	setUser(const char *user);

		/** Returns the user that will be used to connect to the
		 *  database.
		 *
		 *  Unless overridden by a call to setUser(), this will be the
		 *  value of the "user" parameter, of the string attribute, of
		 *  the connection tag, in the config file. */
		const char	*getUser();

		/** Sets the password that will be used to connect to the
		 *  database to "password".
		 * 
		 *  During initialization of the sqlr-connection, this is set
		 *  to the value of the "password" parameter, of the string
		 *  attribute, of the connection tag, in the config file.
		 *
		 *  This method may be called by a module to override that. */
		void	setPassword(const char *password);

		/** Returns the password that will be used to connect to the
		 *  database.
		 *
		 *  Unless overridden by a call to setPassword(), this will be
		 *  the value of the "password" parameter, of the string
		 *  attribute, of the connection tag, in the config file. */
		const char	*getPassword();

		/** Sets the connect timeout.
		 * 
		 *  During initialization of the sqlr-connection, this is set
		 *  to the value of the "connecttimeout" parameter, of the
		 *  string attribute, of the connection tag, in the config
		 *  file.
		 *
		 *  This method may be called by a module to override that. */
		void	setConnectTimeout(uint64_t connecttimeout);

		/** Returns the connect timeout.
		 *
		 *  Unless overridden by a call to setConnectTimeout(), this
		 *  will be the value of the "connecttimeout" parameter, of the
		 *  string attribute, of the connection tag, in the config
		 *  file. */
		uint64_t	getConnectTimeout();

		/** Sets the query timeout.
		 * 
		 *  During initialization of the sqlr-connection, this is set
		 *  to the value of the "querytimeout" parameter, of the
		 *  string attribute, of the connection tag, in the config
		 *  file.
		 *
		 *  This method may be called by a module to override that. */
		void	setQueryTimeout(uint64_t querytimeout);

		/** Returns the query timeout.
		 *
		 *  Unless overridden by a call to setQueryTimeout(), this
		 *  will be the value of the "querytimeout" attribute, of the
		 *  string attribute, of the connection tag, in the config
		 *  file. */
		uint64_t	getQueryTimeout();

		/** Sets whether or not to execute direct.
		 * 
		 *  During initialization of the sqlr-connection, this is set
		 *  to the value of the "executedirect" parameter, of the
		 *  string attribute, of the connection tag, in the config
		 *  file.
		 *
		 *  This method may be called by a module to override that. */
		void	setExecuteDirect(bool executedirect);

		/** Returns whether or not to execute direct.
		 *
		 *  Unless overridden by a call to setExecuteDirect(), this
		 *  will be the value of the "executedirect" parameter, of the
		 *  string attribute, of the connection tag, in the config
		 *  file. */
		bool	getExecuteDirect();



		// environment...

		/** Returns the id of the connection - the value of the id
		 *  attribute, of the instance tag, in the config file. */
		const char	*getId();

		/** Returns the connection id of the connection - the value
 		 *  of the connectionid attribute, of the connection tag,
 		 *  in the config file. */
		const char	*getConnectionId();



		// passthrough...

		/** Sends "size" bytes of "data" to the database. */
		bool	send(byte_t *data, size_t size);

		/** Receives "size" bytes from the database into buffer
 		 *  "data". */
		bool	recv(byte_t **data, size_t *size);



		// client auth...

		/** Authenticates "cred".  Returns true if authentication was
		 *  successful and false otherwise. */
		bool	auth(sqlrcredentials *cred);

		/** Logs out of the database and back in as "newuser" using
		 *  password "newpassword".  Returns true if login was
		 *  successful and false otherwise. */
		bool	changeUser(const char *newuser,
						const char *newpassword);

		/** Switches from the current proxied user to "newuser" using
		 *  password "newpassword".  Returns true if successful and
		 *  false otherwise. */
		bool	changeProxiedUser(const char *newuser,
						const char *newpassword);



		// close client connection...

		/** Attempts to read "bytes" bytes from the client, in
		 *  non-blocking mode, before closing the connection to the
		 *  client. */
		void	closeClientConnection(uint32_t bytes);



		// session management...

		/** Begins a session with a client. */
		void	beginSession();

		/** Aborts all cursors that haven't already been suspended and
		 *  opens a unix socket and inet port that the client can
		 *  connect to to resume the session later.  Returns the
		 *  unix socket in "unixsocket" and inet port in "inetport". */
		void	suspendSession(const char **unixsocket,
						uint16_t *inetport);

		/** Ends the session with a client. */
		void	endSession();



		// ping...

		/** Pings the database using the "ping query" defined by the
		 *  database connection module.  Returns true if the ping
		 *  succeeded and false if it failed. */
		bool	ping();



		// database info...

		/** Returns the type of database: oracle, mysql, postgresql,
		 *  odbc, etc. */
		const char	*getDbType();

		/** Returns the database version. */
		const char	*getDbVersion();

		/** Returns the host name of the server hosting the
		 *  database. */
		const char	*getDbHostName();

		/** Returns the IP address of the server hosting the
		 *  database. */
		const char	*getDbIpAddress();



		// bind variables...

		/** Returns a string representing the bind variable format used
		 *  by the database.  For example:
		 *
		 *  ?  - database uses a ? to represent a bind variable
		 *  @* - database uses a @ followed by any characters to
		 *       represent a bind variable
		 *  $1 - database uses a $ followed by a number to represent a
		 *       bind variable
		 *  :* - database uses a : followed by any characters to
		 *       represent a bind variable
		 *
		 *  Defaults to :* but may be overriden by a child class. */
		const char	*getBindFormat();

		/** Returns the value that the database expects or returns in
		 *  the "null indicator" for a non-null bind value.
		 *
		 *  Defaults to 0 but may be overriden by a child class of
		 *  sqlrserverconnection. */
		int16_t		getNonNullBindValue();

		/** Returns the value that the database expects or returns in
		 *  the "null indicator" for a null bind value.
		 *
		 *  Defaults to -1 but may be overriden by a child class of
		 *  sqlrserverconnection. */
		int16_t		getNullBindValue();

		/** Returns true if "isnull" matches the value that the database
		 *  expects or returns in the "null indicator" for a null bind
		 *  value. */
		bool	getBindValueIsNull(int16_t isnull);

		/** If "fake" is true then the server will fake input binds by
		 *  rewriting the query, rather than by using actual bind
		 *  variables.  If "fake" is false then the server will use
		 *  actual bind variables. */
		void		setFakeInputBinds(bool fake);

		/** Returns true if the server will fake input binds by
		 *  rewriting the query, or false otherwise. */
		bool		getFakeInputBinds();



		// sequences...

		/** Returns a string representing the format of the sequence
		 *  nextval command used in the database.  The format will
		 *  contain a %s in place of the sequence name.  For example:
		 *
		 *  (nextval for %s)
		 *  next value for %s
		 *  nextval('%s')
		 *  %s.nextval
		 *
		 *  Returns an empty string if the database does not support
		 *  sequences.
		 *
		 *  Defaults to %s.nextval but may be overriden by a child
		 *  class. */
		const char	*getNextvalFormat();



		// fetch info...

		/** Sets the number of rows to fetch at once to
		 *  "fetchatonce". */
		void	setFetchAtOnce(uint32_t fethatonce);

		/** Returns the number of rows that will be fetched at once. */
		uint32_t	getFetchAtOnce();

		/** Sets the maximum number of columns that a result set can
		 *  contain to "maxcolumncount".  Additional columns will be
		 *  truncated. */
		void	setMaxColumnCount(uint32_t maxcolumncount);

		/** Returns the number of columns that a result set can
		 *  contain. */
		uint32_t	getMaxColumnCount();

		/** Sets the maximum number of bytes that a non-LOB field can
		 *  contain to "maxfieldsize".  Additional bytes will be
		 *  truncated. */
		void	setMaxFieldSize(uint32_t maxfieldsize);

		/** Returns the maximum number of bytes that a non-LOB field
		 *  can contain. */
		uint32_t	getMaxFieldSize();



		// db selection...

		/** Selects database "db".  Returns true if selection
		 *  succeeded and false otherwise. */
		bool	selectDatabase(const char *db);

		/** Returns the current database.
		 *
		 *  Note that this method allocates a buffer for the return
		 *  value internally and returns it.  The calling method must
		 *  deallocate this buffer. */
		char	*getCurrentDatabase();

		/** Returns the current database.
		 *
		 *  Note that this method allocates a buffer for the return
		 *  value internally and returns it.  The calling method must
		 *  deallocate this buffer. */
		char	*getCurrentSchema();



		// last insert id...

		/** Gets the last insert it and populates "id" with it.
		 *
		 *  Returns true on success and false on failure. */
		bool	getLastInsertId(uint64_t *id);



		// transactions...

		/** Begins a new transaction.  Returns true on success and
		 *  false on failure. */
		bool	begin();

		/** Commits the current transaction.  Returns true on success
		 *  and false on failure. */
		bool	commit();

		/** Rolls the current transaction back.  Returns true on success
		 *  and false on failure. */
		bool	rollback();

		/** Set auto-commit on.  Returns true on success and false on
		 *  failure. */
		bool	setAutoCommitOn();

		/** Set auto-commit off.  Returns true on success and false on
		 *  failure. */
		bool	setAutoCommitOff();

		/** Sets a flag indicating whether a DML query has been run and
		 *  thus whether a commit or rollback is needed under certain
		 *  circumstances.
		 *
		 *  If the flag is set true...
		 *
		 *  During endSession(), for transactional databases, a commit
		 *  or rollback is executed, depending on whether
		 *  endofsession="commit" or endofsession="rollback" is set in
		 *  the instance tag, in the config file.
		 *
		 *  After each query, for transactional databases that don't
		 *  support transaction blocks or auto-commit, if we're faking
		 *  auto-commit then a commit is executed. */
		void	setNeedsCommitOrRollback(bool needed);

		/** Returns whether a commit or rollback is needed, as set by
		 *  setNeedsCommitOrRollback(). */
		bool	getNeedsCommitOrRollback();

		/** Sets the isolation level to "isolevel".  Returns true on
		 *  success and false on failure. */
		bool	setIsolationLevel(const char *isolevel);

		/** If "ftb" is true then transaction blocks are faked by
		 *  setting autocommit on and off as appropriate.  If "ftb" is
		 *  false then begin and commit/rollback queries are run to
		 *  begin/end transaction blocks. */
		void	setFakeTransactionBlocks(bool ftb);

		/** Returns whether transaction blocks are being faked, as set
 		 *  by setFakeTransactionBlocks(). */
		bool	getFakeTransactionBlocks();

		/** If "fac" is true then auto-commit is faked by executing a
		 *  commit after each query.  If "fac" is false then native
		 *  auto-commit is used.
		 *
		 *  See setNeedsCommitOrRollback() for caveats. */
		void	setFakeAutoCommit(bool fac);

		/** Returns whether auto-commit is being faked, as set by
		 *  setFakeAutoCommit(). */
		bool	getFakeAutoCommit();

		/** If "iac" is true then the initial auto-commit behavior is
		 *  set to auto-commit-on.  If "iac" is false then the initial
		 *  auto-commit behavior is set to auto-commit-off.
		 *
		 *  During a session, auto-commit may be turned on or off.
		 *  During endSession() and reLogIn(), the auto-commit behavior
		 *  is reset to the initial auto-commit behavior as defined
		 *  by this method. */
		void	setInitialAutoCommit(bool iac);

		/** Returns whether the initial auto-commit behavior is set to
		 *  auto-commit-on as set by setInitialAutoCommit(). */
		bool	getInitialAutoCommit();

		/** Returns true if we're currently in a transaction and false
		 *  otherwise. */
		bool	getInTransaction();

		// errors...

		/** Fetches the current connection-level error into the
		 *  connection's error buffer, unless there is already an
		 *  error saved in the buffer. */
		void	saveError();

		/** Fetches the current cursor-level error into the
		 *  cursor's error buffer, unless there is already an
		 *  error saved in the buffer.
		 *
		 *  FIXME: how is this distinct from saveError(cursor)? */
		void	saveErrorFromCursor(sqlrservercursor *cursor);

		/** Returns the error message and code by:
		 *
		 *  Setting "errorbuffer" to the connection-level error buffer.
		 *  Populating "errorsize" with the number of bytes in the
		 *  error buffer.
		 *  Populating "errorcode" with the connection-level numeric
		 *  error code.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is still up.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is down. */
		void	getError(const char **errorbuffer,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection);

		/** Returns the error message and code by:
		 *
		 *  Copying at most "errorbuffersize" bytes from the
		 *  connection-level error buffer into "errorbuffer".
		 *  Populating "errorsize" with the number of bytes in the
		 *  error buffer.
		 *  Populating "errorcode" with the connection-level numeric
		 *  error code.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is still up.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is down. */
		void	getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection);

		/** Empties the connection-level error buffer and sets a flag
		 *  indicating that the connection to the database is up. */
		void	clearError();

		/** Copies "errsize" bytes of "err" into the connection-level
		 *  error buffer, sets the connection-level numeric error code
		 *  to "errn" and sets a flag indicating that the connection to
		 *  the database is up. */
		void	setError(const char *err,
					uint32_t errsize,
					int64_t errn,
					bool liveconn);

		/** Copies "err" into the connection-level error buffer,
		 *  sets the connection-level numeric error code to "errn" and
		 *  sets a flag indicating that the connection to the database
		 *  is up. */
		void	setError(const char *err, int64_t errn, bool liveconn);

		/** Returns a pointer to the connection-level error buffer. */
		char	*getErrorBuffer();

		/** Returns the size, in bytes, of the connection-level error
		 *  buffer. */
		uint32_t	getErrorBufferSize();

		/** Sets the number of bytes currently stored in the
		 *  connection-level error buffer to "errorsize". */
		void	setErrorSize(uint32_t errorsize);

		/** Returns the number of bytes currently stored in the
		 *  connection-level error buffer, as set by setErrorSize(). */
		uint32_t	getErrorSize();

		/** Sets the connection-level numeric error code to "errnum". */
		void	setErrorNumber(uint32_t errnum);

		/** Returns the connection-level numeric error code as set by
		 *  setErrorNumber(). */
		uint32_t	getErrorNumber();

		/** Sets a flag indicating whether the connection to the
		 *  database is up to "liveconnection". */
		void	setLiveConnection(bool liveconnection);

		/** Returns the flag indicating whether the connection to the
		 *  database is up, as set by setLiveConnection(). */
		bool	getLiveConnection();



		// connection state...

		/** Sets the current state of the database connection to
		 *  "state". */
		void	setState(enum sqlrconnectionstate_t state);

		/** Returns the database connection state as set by
		 *  setState(). */
		enum sqlrconnectionstate_t	getState();



		// instance state...

		/** If "disabled" is true then the instance is marked as
		 *  disabled.  If "disabled" is false, then the instance is
		 *  marked as enabled. */
		void	setInstanceDisabled(bool disabled);

		/** Returns whether the instance is disabled or enabled, as set
		 *  by setInstanceDisabled(). */
		bool	getInstanceDisabled();



		// statistics api...

		/** Returns the current statistics index. */
		uint32_t	getStatisticsIndex();

		/** Returns the number of client connections that are currently
		 *  open. */
		uint32_t	getOpenClientConnections();

		/** Returns the number of client connections that have been
		 *  opened since the instance was started. */
		uint32_t	getOpenedClientConnections();

		/** Increments the number of client connection that are
		 *  currently open as well as the number of client connections
		 *  that have been opened since the instance was started. */
		void	incrementOpenClientConnections();

		/** Decrements the number of client connections that are
		 *  currently open. */
		void	decrementOpenClientConnections();

		/** Returns the number of persistent database connections that
		 *  are currently open. */
		uint32_t	getOpenDatabaseConnections();

		/** Returns the number of persistent database connections that
		 *  have been opened since the instance was started. */
		uint32_t	getOpenedDatabaseConnections();

		/** Increments the number of persistent database connections
		 *  that are currently open as well as the number of persistent
		 *  database connections that have been opened since the
		 *  instance was started. */
		void	incrementOpenDatabaseConnections();

		/** Decrements the number of persistent database connections
		 *  that are currently open. */
		void	decrementOpenDatabaseConnections();

		/** Returns the number of database cursors that are currently
		 *  open. */
		uint32_t	getOpenDatabaseCursors();

		/** Returns the number of database cursors that have been
		 *  opened since the instance was started. */
		uint32_t	getOpenedDatabaseCursors();

		/** Increments the number of database cursors that are
		 *  currently open as well as the number of database cursors
		 *  that have been opened since the instance was started. */
		void	incrementOpenDatabaseCursors();

		/** Decrements the number of database cursors that are
		 *  currently open. */
		void	decrementOpenDatabaseCursors();

		/** Returns the number of times a new cursor was opened and
		 *  used, as opposed to reusing an existing cursor. */
		uint32_t	getTimesNewCursorUsed();

		/** Increments the number of times a new cursor was opened and
		 *  used, as opposed to reusing an existing cursor. */
		void	incrementTimesNewCursorUsed();

		/** Returns the number of times a cursor was reused, as
		 *  opposed to opening and using a new cursor. */
		uint32_t	getTimesCursorReused();

		/** Increments the number of times a cursor was reused, as
		 *  opposed to opening and using a new cursor. */
		void	incrementTimesCursorReused();

		/** Returns the number of queries of type "querytype" that have
		 *  been executed since the instance was started. */
		uint32_t	getQueryCount(sqlrquerytype_t querytype);

		/** Returns the total number of queries that have been executed
		 *  since the instance was started. */
		uint32_t	getTotalQueryCount();

		/** Increments the number of queries of type "querytype" that
		 *  have been executed since the instance was started, as well
		 *  as the total number of queries that have been executed
		 *  since the instance was started, */
		void	incrementQueryCount(sqlrquerytype_t querytype);

		/** Returns the number of errors that have occurred since the
		 *  instance was started. */
		uint32_t	getTotalErrors();

		/** Increments the number of errors that have occurred since
		 *  the instance was started. */
		void	incrementTotalErrors();

		/** Returns the number of authentications that have occurred
		 *  since the instance was started. */
		uint32_t	getAuthCount();

		/** Increments the number of authentications that have occurred
		 *  since the instance was started. */
		void	incrementAuthCount();

		/** Returns the number of sessions that have been suspeneded
		 *  since the instance was started. */
		uint32_t	getSuspendSessionCount();

		/** Increments the number of sessions that have been suspeneded
		 *  since the instance was started. */
		void	incrementSuspendSessionCount();

		/** Returns the number of sessions that have ended normally, as
		 *  opposed to having been suspended, since the instance was
		 *  started. */
		uint32_t	getEndSessionCount();

		/** Increments the number of sessions that have ended normally,
		 *  as opposed to having been suspended, since the instance was
		 *  started. */
		void	incrementEndSessionCount();

		/** Returns the number of pings that have occurred since the
		 *  instance was started. */
		uint32_t	getPingCount();

		/** Increments the number of pings that have occurred since the
		 *  instance was started. */
		void	incrementPingCount();

		/** Returns the number of get database type commands that
		 *  have occurred since the instance was started. */
		uint32_t	getIdentifyCount();

		/** Increments the number of get database type commands that
		 *  have occurred since the instance was started. */
		void	incrementIdentifyCount();

		/** Returns the number of autocommits that have occurred
		 *  since the instance was started. */
		uint32_t	getAutocommitCount();

		/** Increments the number of autocommits that have occurred
		 *  since the instance was started. */
		void	incrementAutocommitCount();

		/** Returns the number of begins that have occurred
		 *  since the instance was started. */
		uint32_t	getBeginCount();

		/** Increments the number of begins that have occurred
		 *  since the instance was started. */
		void	incrementBeginCount();

		/** Returns the number of commits that have occurred
		 *  since the instance was started. */
		uint32_t	getCommitCount();

		/** Increments the number of commits that have occurred
		 *  since the instance was started. */
		void	incrementCommitCount();

		/** Returns the number of rollbacks that have occurred
		 *  since the instance was started. */
		uint32_t	getRollbackCount();

		/** Increments the number of rollbacks that have occurred
		 *  since the instance was started. */
		void	incrementRollbackCount();

		/** Returns the number of get database version commands that
		 *  have occurred since the instance was started. */
		uint32_t	getDbVersionCount();

		/** Increments the number of get database version commands that
		 *  have occurred since the instance was started. */
		void	incrementDbVersionCount();

		/** Returns the number of get bind format commands that
		 *  have occurred since the instance was started. */
		uint32_t	getGetBindFormatCount();

		/** Increments the number of get bind format commands that
		 *  have occurred since the instance was started. */
		void	incrementGetBindFormatCount();

		/** Returns the number of get server version commands that
		 *  have occurred since the instance was started. */
		uint32_t	getGetServerVersionCount();

		/** Increments the number of get server version commands that
		 *  have occurred since the instance was started. */
		void	incrementGetServerVersionCount();

		/** Returns the number of select database commands that
		 *  have occurred since the instance was started. */
		uint32_t	getSelectDatabaseCount();

		/** Increments the number of select database commands that
		 *  have occurred since the instance was started. */
		void	incrementSelectDatabaseCount();

		/** Returns the number of get current database commands that
		 *  have occurred since the instance was started. */
		uint32_t	getGetCurrentDatabaseCount();

		/** Increments the number of get current database commands that
		 *  have occurred since the instance was started. */
		void	incrementGetCurrentDatabaseCount();

		/** Returns the number of get last insert id commands that
		 *  have occurred since the instance was started. */
		uint32_t	getGetLastInsertIdCount();

		/** Increments the number of get last insert id commands that
		 *  have occurred since the instance was started. */
		void	incrementGetLastInsertIdCount();

		/** Returns the number of get database host name commands
		 *  that have occurred since the instance was started. */
		uint32_t	getDbHostNameCount();

		/** Increments the number of get database host name commands
		 *  that have occurred since the instance was started. */
		void	incrementDbHostNameCount();

		/** Returns the number of get database ip address commands
		 *  that have occurred since the instance was started. */
		uint32_t	getDbIpAddressCount();

		/** Increments the number of get database ip address commands
		 *  that have occurred since the instance was started. */
		void	incrementDbIpAddressCount();

		/** Returns the number of new queries, as opposed to
		 *  reexecuted queries, that have been run since the instance
		 *  was started. */
		uint32_t	getNewQueryCount();

		/** Increments the number of new queries, as opposed to
		 *  reexecuted queries, that have been run since the instance
		 *  was started. */
		void	incrementNewQueryCount();

		/** Returns the number of reexecuted queries, as opposed to
		 *  new queries, that have been run since the instance was
		 *  started. */
		uint32_t	getReexecuteQueryCount();

		/** Increments the number of reexecuted queries, as opposed to
		 *  new queries, that have been run since the instance was
		 *  started. */
		void	incrementReexecuteQueryCount();

		/** Returns the number of fetch-from-bind-cursor commands
 		 *  that have been run since the instance was started. */
		uint32_t	getFetchFromBindCursorCount();

		/** Increments the number of fetch-from-bind-cursor commands
		 *  that have been run since the instance was started. */
		void	incrementFetchFromBindCursorCount();

		/** Returns the number of result sets that have been fetched
		 *  run since the instance was started. */
		uint32_t	getFetchResultSetCount();

		/** Increments the number of result sets that have been fetched
		 *  run since the instance was started. */
		void	incrementFetchResultSetCount();

		/** Returns the number of result sets that have been aborted
		 *  since the instance was started. */
		uint32_t	getAbortResultSetCount();

		/** Increments the number of result sets that have been aborted
		 *  since the instance was started. */
		void	incrementAbortResultSetCount();

		/** Returns the number of result sets that have been
		 *  suspended since the instance was started. */
		uint32_t	getSuspendResultSetCount();

		/** Increments the number of result sets that have been
		 *  suspended since the instance was started. */
		void	incrementSuspendResultSetCount();

		/** Returns the number of result sets that have been
		 *  resumed since the instance was started. */
		uint32_t	getResumeResultSetCount();

		/** Increments the number of result sets that have been
		 *  resumed since the instance was started. */
		void	incrementResumeResultSetCount();

		/** Returns the number of get-database-list commands that
 		 *  have been run since the instance was started. */
		uint32_t	getGetDbListCount();

		/** Increments the number of get-database-list commands that
 		 *  have been run since the instance was started. */
		void	incrementGetDbListCount();

		/** Returns the number of get-table-list commands that
 		 *  have been run since the instance was started. */
		uint32_t	getGetTableListCount();

		/** Increments the number of get-table-list commands that
 		 *  have been run since the instance was started. */
		void	incrementGetTableListCount();

		/** Returns the number of get-column-list commands that
 		 *  have been run since the instance was started. */
		uint32_t	getGetColumnListCount();

		/** Increments the number of get-column-list commands that
 		 *  have been run since the instance was started. */
		void	incrementGetColumnListCount();

		/** Returns the number of get-query-tree commands that
 		 *  have been run since the instance was started. */
		uint32_t	getGetQueryTreeCount();

		/** Increments the number of get-query-tree commands that
 		 *  have been run since the instance was started. */
		void	incrementGetQueryTreeCount();

		/** Returns the number of re-logins that have occcurred
 		 *  since the instance was started. */
		uint32_t	getReLogInCount();

		/** Increments the number of re-logins that have occcurred
 		 *  since the instance was started. */
		void	incrementReLogInCount();

		/** Returns the number of get-next-result-set commands that
		 *  have been run since the instance was started. */
		uint32_t	getNextResultSetCount();

		/** Increments the number of get-next-result-set commands that
		 *  have been run since the instance was started. */
		void	incrementNextResultSetCount();

		/** Returns the number of get-next-result-set-available
		 *  commands that have been run since the instance was
		 *  started. */
		uint32_t	getNextResultSetAvailableCount();

		/** Increments the number of get-next-result-set-available
		 *  commands that have been run since the instance was
		 *  started. */
		void	incrementNextResultSetAvailableCount();

		/** Returns the current user. */
		const char	*getCurrentUser();

		/** Sets the current query by copying "querysize" bytes of
		 *  "query" into the statistics buffer. */
		void	setCurrentQuery(const char *query, uint32_t querysize);

		/** Returns the current query. */
		const char	*getCurrentQuery();

		/** Sets the current client info by copying "infosize" bytes of
		 *  "info" into the statistics buffer. */
                void    setClientInfo(const char *info, uint32_t infosize);

		/** Returns the current client info. */
		const char	*getClientInfo();

		/** Returns the address of the currently connected client. */
		const char	*getClientAddr();

		/** Tells the statistics framework that a command has been
		 *  started.  "sec" and "usec" should be the number of seconds
		 *  and microseconds since the epoch (Jan 1, 1970). */
		void		setCommandStart(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec);

		/** Returns the seconds-component of the command start time as
		 *  set by setCommandStart(). */
		uint64_t	getCommandStartSec(sqlrservercursor *cursor);

		/** Returns the microseconds-component of the command start
		 *  time as set by setCommandStart(). */
		uint64_t	getCommandStartUSec(sqlrservercursor *cursor);

		/** Tells the statistics framework that a command has ended.
		 *  "sec" and "usec" should be the number of seconds and
		 *  microseconds since the epoch (Jan 1, 1970). */
		void		setCommandEnd(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec);

		/** Returns the seconds-component of the command end time as
		 *  set by setCommandStart(). */
		uint64_t	getCommandEndSec(sqlrservercursor *cursor);

		/** Returns the microseconds-component of the command end
		 *  time as set by setCommandStart(). */
		uint64_t	getCommandEndUSec(sqlrservercursor *cursor);

		/** Tells the statistics framework that a query has been
		 *  started.  "sec" and "usec" should be the number of seconds
		 *  and microseconds since the epoch (Jan 1, 1970). */
		void		setQueryStart(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec);

		/** Returns the seconds-component of the query start time as
		 *  set by setCommandStart(). */
		uint64_t	getQueryStartSec(sqlrservercursor *cursor);

		/** Returns the microseconds-component of the query start
		 *  time as set by setCommandStart(). */
		uint64_t	getQueryStartUSec(sqlrservercursor *cursor);

		/** Tells the statistics framework that a query has ended.
		 *  "sec" and "usec" should be the number of seconds and
		 *  microseconds since the epoch (Jan 1, 1970). */
		void		setQueryEnd(sqlrservercursor *cursor,
						uint64_t sec, uint64_t usec);

		/** Returns the seconds-component of the query end time as
		 *  set by setCommandStart(). */
		uint64_t	getQueryEndSec(sqlrservercursor *cursor);

		/** Returns the microseconds-component of the query end
		 *  time as set by setCommandStart(). */
		uint64_t	getQueryEndUSec(sqlrservercursor *cursor);



		// event api...

		/** Returns true if logging is enabled and false otherwise. */
		bool	getLoggingEnabled();

		/** Returns true if notifications are enabled and false
		 *  otherwise. */
		bool	getNotificationsEnabled();

		/** Raises a debug-message event with information "info", which
		 *  may be logged, or which may trigger a notification. */
		void	raiseDebugMessageEvent(const char *info);

		/** Raises a client-connected event, which may be logged, or
		 *  which may trigger a notification. */
		void	raiseClientConnectedEvent();

		/** Raises a client-connection-refused event with information
		 *  "info", which may be logged, or which may trigger a
		 *  notification. */
		void	raiseClientConnectionRefusedEvent(const char *info);

		/** Raises a client-disconnected event with information "info",
		 *  which may be logged, or which may trigger a notification. */
		void	raiseClientDisconnectedEvent(const char *info);

		/** Raises a client-protocol-error event on cursor "cursor",
		 *  with information "info", and with result code "result"
		 *  (one of 0, indicating a closed connection, RESULT_ERROR,
		 *  RESULT_TIMEOUT, or RESULT_ABORT) which may be logged, or
		 *  which may trigger a notification. */
		void	raiseClientProtocolErrorEvent(sqlrservercursor *cursor,
							const char *info,
							ssize_t result);

		/** Raises a database-login event, which may be logged, or
		 *  which may trigger a notification. */
		void	raiseDbLogInEvent();

		/** Raises a database-logout event, which may be logged, or
		 *  which may trigger a notification. */
		void	raiseDbLogOutEvent();

		/** Raises a database-error event on cursor "cursor", with
		 *  information "info", which may be logged, or which may
		 *  trigger a notification. */
		void	raiseDbErrorEvent(sqlrservercursor *cursor,
							const char *info);

		/** Raises a database-warning event on cursor "cursor", with
		 *  information "info", which may be logged, or which may
		 *  trigger a notification. */
		void	raiseDbWarningEvent(sqlrservercursor *cursor,
							const char *info);

		/** Raises a query event on cursor "cursor", which may be
		 *  logged, or which may trigger a notification. */
		void	raiseQueryEvent(sqlrservercursor *cursor);

		/** Raises a filter-violation event on cursor "cursor", which
		 *  may be logged, or which may trigger a notification. */
		void	raiseFilterViolationEvent(sqlrservercursor *cursor);

		/** Raises an internal-error event on cursor "cursor", with
		 *  information "info", which may be logged, or which may
		 *  trigger a notification. */
		void	raiseInternalErrorEvent(sqlrservercursor *cursor,
							const char *info);

		/** Raises an internal-warning event on cursor "cursor", with
		 *  information "info", which may be logged, or which may
		 *  trigger a notification. */
		void	raiseInternalWarningEvent(sqlrservercursor *cursor,
							const char *info);

		/** Raises a schedule-violation event, with information "info",
		 *  which may be logged, or which may trigger a notification. */
		void	raiseScheduleViolationEvent(const char *info);

		/** Raises a integrity-violation event, with information "info",
		 *  which may be logged, or which may trigger a notification. */
		void	raiseIntegrityViolationEvent(const char *info);

		/** Raises a translation-failure event, with information "info",
		 *  which may be logged, or which may trigger a notification. */
		void	raiseTranslationFailureEvent(sqlrservercursor *cursor,
							const char *info);

		/** Raises a parse-failure event, with information "info",
		 *  which may be logged, or which may trigger a notification. */
		void	raiseParseFailureEvent(sqlrservercursor *cursor,
							const char *info);

		/** Raises a cursor-open event, on cursor "cursor", which may
		 *  be logged, or which may trigger a notification. */
		void	raiseCursorOpenEvent(sqlrservercursor *cursor);

		/** Raises a cursor-close event, on cursor "cursor", which may
		 *  be logged, or which may trigger a notification. */
		void	raiseCursorCloseEvent(sqlrservercursor *cursor);

		/** Raises a begin-transaction event, on cursor "cursor", which
		 *  may be logged, or which may trigger a notification. */
		void	raiseBeginTransactionEvent();

		/** Raises a commit event, on cursor "cursor", which
		 *  may be logged, or which may trigger a notification. */
		void	raiseCommitEvent();

		/** Raises a rollback event, on cursor "cursor", which
		 *  may be logged, or which may trigger a notification. */
		void	raiseRollbackEvent();


		// cursor api...

		// cursor management...

		/** Returns cursor "id" from the pool of already-open cursors
		 *  or NULL if "id" was not a valid cursor id.
		 *
		 *  The size of the cursor pool depends on the value of the
		 *  maxcursors attribute, of the instance tag, in the config
		 *  file.  Valid cursor ids range from 0 to maxcursors-1.
		 *
		 *  However, you will typically only request a cursor by id
		 *  if you already know an id from a previous call to
		 *  getId(cursor) and have a specific reason to reuse that
		 *  cursor. */
		sqlrservercursor	*getCursor(uint16_t id);

		/** Returns an available cursor from the pool of already-open
		 *  cursors from the pool of available cursors, or NULL if no
		 *  cursor was currently available.
		 *
		 *  The size of the cursor pool depends on the value of the
		 *  maxcursors attribute, of the instance tag, in the config
		 *  file. */
		sqlrservercursor	*getCursor();

		/** Allocates a cursor, outside of the cursor pool.  This
		 *  cursor will have an id >= the value of the maxcursors
		 *  attribute, of the instance tag, in the config file.
		 *
		 *  This cursor must be opened by calling open(cursor) before
		 *  it can be used and should be closed by calling close(cursor)
		 *  and deleted by calling deleteCursor(cursor) when you are
		 *  done using it.
		 *
		 *  Returns NULL if a new cursor couldn't be allocated. */
		sqlrservercursor	*newCursor();

		/** Returns the id of "cursor". */
		uint16_t	getId(sqlrservercursor *cursor);

		/** Opens "cursor".  Returns true on success and false on
		 *  failure. */
		bool	open(sqlrservercursor *cursor);

		/** Closes "cursor".  Returns true on success and false on
		 *  failure. */
		bool	close(sqlrservercursor *cursor);

		/** Closes the result set of "cursor" and clears any custom
		 *  query cursor associated with "cursor". */
		void	abort(sqlrservercursor *cursor);

		/** Releases "cursor" back to the pool of already-open cursors
		 *  and marks it available so that it may be returned by a
		 *  future call to getCursor().
		 *
		 *  Note that "cursor" should have been returned from a call to
		 *  getCursor().  This method should not be called on a cursor
		 *  returned from newCursor(). */
		void	release(sqlrservercursor *cursor);

		/** Deletes "cursor".
		 *
		 *  Note that "cursor" should have been allocated by a call to
		 *  newCursor().  This method should not be called on a cursor
		 *  returned from getCursor() */
		void	deleteCursor(sqlrservercursor *cursor);



		// query buffer...

		/** Returns a pointer to the query buffer of "cursor" */
		char		*getQueryBuffer(sqlrservercursor *cursor);

		/** Sets the size, in bytes, of the query that is currently
		 *  present in the query buffer of "cursor" to "querysize". */
		void		setQuerySize(sqlrservercursor *cursor,
							uint32_t querysize);

		/** Returns the size, in bytes, of the query that is currently
		 *  present in the query buffer of "cursor", as set by
		 *  setQuerySize(). */
		uint32_t 	getQuerySize(sqlrservercursor *cursor);



		// query status...

		/** Sets the status of the current query of "cursor"
		 *  to "status". */
		void	setQueryStatus(sqlrservercursor *cursor,
						sqlrquerystatus_t status);

		/** Returns the status of the current query of "cursor",
		 *  as set by setQueryStatus(). */
		sqlrquerystatus_t	getQueryStatus(
						sqlrservercursor *cursor);



		// query translations...

		/** Sets the tree representing the current query of "cursor" to
		 *  "tree". */
		void	setQueryTree(sqlrservercursor *cursor, xmldom *tree);

		/** Returns the tree representing the current query of "cursor"
		 *  as set by setQueryTree(), or NULL if no tree has been
		 *  set since initialization of "cursor" or since the most
		 *  recent call to clearQueryTree(). */
		xmldom	*getQueryTree(sqlrservercursor *cursor);

		/** Sets the tree representing the current query of "cursor" to
		 *  NULL. */
		void	clearQueryTree(sqlrservercursor *cursor);

		/** Returns the translated query buffer of "cursor". */
		stringbuffer	*getTranslatedQueryBuffer(
						sqlrservercursor *cursor);

		/** Returns the query currently stored in the translated quer
		 *  buffer of "cursor". */
		const char	*getTranslatedQuery(sqlrservercursor *cursor);



		// running queries...

		/** Copies "size" bytes of "query" to the query buffer of
		 *  "cursor" and prepares the query, with all directives,
		 *  translations, and filters enabled.
		 *
		 *  Returns true on success and false otherwise. */
		bool	prepareQuery(sqlrservercursor *cursor,
						const char *query,
						uint32_t size);

		/** Copies "size" bytes of "query" to the query buffer of
		 *  "cursor" and prepares the query.
		 *
		 *  Directives are enabled if "enabledirectives" is true or
		 *  disabled if it is false.
		 *  Translations are enabled if "enabletranslations" is true or
		 *  disabled if it is false.
		 *  Filters are enabled if "enablefilters" is true or disabled
		 *  if it is false.
		 *
		 *  Returns true on success and false otherwise. */
		bool	prepareQuery(sqlrservercursor *cursor,
						const char *query,
						uint32_t size,
						bool enabledirectives,
						bool enabletranslations,
						bool enablefilters);

		/** Executes the currently prepared query of "cursor", with all
		 *  directives, translations, and filters enabled.
		 *
		 *  Returns true on success and false otherwise. */
		bool	executeQuery(sqlrservercursor *cursor);

		/** Executes the currently prepraed query of "cursor".
		 *
		 *  Directives are enabled if "enabledirectives" is true or
		 *  disabled if it is false.
		 *  Translations are enabled if "enabletranslations" is true or
		 *  disabled if it is false.
		 *  Filters are enabled if "enablefilters" is true or disabled
		 *  if it is false.
		 *  Triggers are enabled if "enabletriggers" is true or disabled
		 *  if it is false.
		 *
		 *  Returns true on success and false otherwise. */
		bool	executeQuery(sqlrservercursor *cursor,
						bool enabledirectives,
						bool enabletranslations,
						bool enablefilters,
						bool enabletriggers);

		/** Fetches from bind cursor "cursor".  Returns true on sucecss
		 *  and false otherwise. */
		bool	fetchFromBindCursor(sqlrservercursor *cursor);

		/** Advances to the next result set of "cursor".
		 *
		 *  Returns true and sets "nextresultsetavailable" true if
		 *  another result set was available.
		 *
		 *  Returns true and sets "nextresultsetavailable" false if
		 *  another result set was not available.
		 *
		 *  Returns false if an error occured while checking for
		 *  another resulet set. */
		bool	nextResultSet(sqlrservercursor *cursor,
						bool *nextresultsetavailable);



		// bind variables...

		/** Returns the memory pool of "cursor" used to store bind
		 *  variable names and values. */
		memorypool	*getBindPool(sqlrservercursor *cursor);

		/** Returns the memory pool of "cursor" used to map bind
 		 *  variable names when the value of the attribute
 		 *  translatebindvariables of the instance tag in the config
 		 *  file is set to "yes". */
		memorypool	*getBindMappingsPool(sqlrservercursor *cursor);

		/** Returns the dictionary of bind variable name mappings of
		 *  "cursor".  The keys are the old (original) bind variable
		 *  names and the values are the new (translated) bind variable
		 *  names.
		 *
		 *  The dictionary is populated by prepareQuery() and remains
		 *  populated with the same key-value pairs until the next call
		 *  to prepareQuery().  If it is empty then either bind variable
		 *  translation is disabled or the most recently prepared query
		 *  contained no bind variables. */
		dictionary<char *, char *>	*getBindMappings(
						sqlrservercursor *cursor);



		// fake input binds...

		/** Sets whether to fake input binds for the current query of
		 *  "cursor", by rewriting the query, to "fake".
		 *
		 *  Note that the behavior of prepareQuery() is as follows:
		 *
		 *    * It initially configures whether input binds should be
		 *      faked or not, based on the value of the
		 *      "fakeinputbindvariables" attribute of the instance tag,
		 *      in the config file.
		 *    * It then processes filters, directives, translations,
		 *      and before-triggers, which can call this method to
		 *      override that.
		 *    * It then checks to see if the query supports native
		 *      binds and sets input binds to be faked if native binds
		 *      are not supported with this query.
		 *
		 *  So, it is possible for this method to hae no effect if:
		 *
		 *    * It is called from a module other than a filter,
		 *      directive, translation, or before-trigger.
		 *    * It sets binds to not be faked, but the query itself
		 *      doesn't support native binds.
		 */
		void	setFakeInputBindsForThisQuery(
						sqlrservercursor *cursor,
						bool fake);

		/** Returns whether or not input binds will be faked for the
		 *  current query of "cursor".  See
		 *  setFakeInputBindsForThisQuery(). */
		bool	getFakeInputBindsForThisQuery(
						sqlrservercursor *cursor);



		// input bind variables...

		/** Sets the number of valid input binds in "cursor" to
		 *  "inbindcount". */
		void	setInputBindCount(sqlrservercursor *cursor,
							uint16_t inbindcount);

		/** Returns the number of valid input binds in "cursor", as
		 *  set by setInputBindCount(). */
		uint16_t	getInputBindCount(sqlrservercursor *cursor);

		/** Returns the array of input binds in "cursor".  The total
		 *  number of bind variables in the array is equal to the value
		 *  of the maxbindcount attribute of the instance tag in the
		 *  config file.  However, only the first getInputBindCount()
		 *  bind variables are currently valid.  The state of the rest
		 *  are undefined. */
		sqlrserverbindvar	*getInputBinds(
						sqlrservercursor *cursor);



		// output bind variables...

		/** Sets the number of valid output binds in "cursor" to
		 *  "outbindcount". */
		void		setOutputBindCount(sqlrservercursor *cursor,
							uint16_t outbindcount);

		/** Returns the number of valid output binds in "cursor", as
		 *  set by setOutputBindCount(). */
		uint16_t	getOutputBindCount(sqlrservercursor *cursor);

		/** Returns the array of output binds in "cursor".  The total
		 *  number of bind variables in the array is equal to the value
		 *  of the maxbindcount attribute of the instance tag in the
		 *  config file.  However, only the first getOutputBindCount()
		 *  bind variables are currently valid.  The state of the rest
		 *  are undefined. */
		sqlrserverbindvar	*getOutputBinds(
						sqlrservercursor *cursor);

		/** Opens LOB output bind at position "index" in "cursor"
		 *  (unless it is already open) and sets "length" equal to its
		 *  length, in characters.
 		 *
 		 *  Returns true on success and false otherwise.  Will return
 		 *  false if the output bind at position "index" is not a LOB.
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  then this method will return true. */
		bool	getLobOutputBindLength(sqlrservercursor *cursor,
							uint16_t index,
							uint64_t *length);

		/** Opens LOB output bind at position "index" in "cursor"
		 *  (unless it is already open) and attempts to fetch
		 *  "charstoread" characters from position "offset" into
		 *  "buffer" of "buffersize" bytes.  Populates "charsread" with
		 *  the number of characters that were actually read.
 		 *
 		 *  Returns true on success and false otherwise.  Will return
 		 *  false if the output bind at position "index" is not a LOB.
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  then this method will return true. */
		bool	getLobOutputBindSegment(sqlrservercursor *cursor,
							uint16_t index,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread);

		/** Closes LOB output bind at position "index" of "cursor".
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  and the LOB was opened by a call to
 		 *  getLobOutputBindLength() or getLobOutputBindSegment()
 		 *  then this method will close it. */
		void	closeLobOutputBind(sqlrservercursor *cursor,
							uint16_t index);



		// input/output bind variables...

		/** Sets the number of valid input-output binds in "cursor" to
		 *  "inoutbindcount". */
		void		setInputOutputBindCount(
						sqlrservercursor *cursor,
						uint16_t inoutbindcount);

		/** Returns the number of valid input-output binds in "cursor",
 		 *  as set by setInputOutputBindCount(). */
		uint16_t	getInputOutputBindCount(
						sqlrservercursor *cursor);

		/** Returns the array of input-output binds in "cursor".  The
		 *  total number of bind variables in the array is equal to the
		 *  value of the maxbindcount attribute of the instance tag in
		 *  the config file.  However, only the first
		 *  getInputOutputBindCount() bind variables are currently
		 *  valid.  The state of the rest are undefined. */
		sqlrserverbindvar	*getInputOutputBinds(
						sqlrservercursor *cursor);

		/** Opens LOB input-output bind at position "index" in "cursor"
		 *  (unless it is already open) and sets "length" equal to its
		 *  length, in characters.
 		 *
 		 *  Returns true on success and false otherwise.  Will return
 		 *  false if the output bind at position "index" is not a LOB.
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  then this method will return true. */
		bool	getLobInputOutputBindLength(sqlrservercursor *cursor,
							uint16_t index,
							uint64_t *length);

		/** Opens LOB input-output bind at position "index" in "cursor"
		 *  (unless it is already open) and attempts to fetch
		 *  "charstoread" characters from position "offset" into
		 *  "buffer" of "buffersize" bytes.  Populates "charsread" with
		 *  the number of characters that were actually read.
 		 *
 		 *  Returns true on success and false otherwise.  Will return
 		 *  false if the output bind at position "index" is not a LOB.
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  then this method will return true. */
		bool	getLobInputOutputBindSegment(sqlrservercursor *cursor,
							uint16_t index,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread);

		/** Closes LOB input-output bind at position "index" of
		 *  "cursor".
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  and the LOB was opened by a call to
 		 *  getLobOutputBindLength() or getLobOutputBindSegment()
 		 *  then this method will close it. */
		void	closeLobInputOutputBind(sqlrservercursor *cursor,
							uint16_t index);



		// custom queries...

		/** Determines if the query currently in the query buffer of
		 *  "cursor" needs to be handled with a custom query module.
		 *  If so, it configures "cursor" to use that module and
		 *  returns the custom query cursor.  If not, it just returns
		 *  "cursor". */
		sqlrservercursor	*useCustomQueryCursor(
						sqlrservercursor *cursor);

		/** Returns true if "cursor" is handling the current query
 		 *  using a custom query module and false otherwise. */
		bool	isCustomQuery(sqlrservercursor *cursor);



		// temp tables...

		/** Adds global temporary tables defined by "gtts" to the list
		 *  of global temporary tables that will be truncated when the
		 *  client session ends.
		 *
		 *  "gtts" may be either "%", indicating that all global
		 *  temporary tables should be truncated, or a comma separated
		 *  list of tables to truncate such as
		 *  "table1,table2,table3". */
		void	addGlobalTempTables(const char *gtts);

		/** Adds "tablename" to the list of temporary tables that will
		 *  be dropped when the client session ends. */
		void	addTempTableForDrop(const char *tablename);

		/** Adds "tablename" to the list of temporary tables that will
		 *  be truncated when the client session ends. */
		void	addTempTableForTrunc(const char *tablename);



		// table name remapping...
		const char	*translateTableName(const char *table);
		bool		removeReplacementTable(const char *database,
							const char *schema,
							const char *table);
		bool		removeReplacementIndex(const char *database,
							const char *schema,
							const char *table);



		// db, table, column, procedure bind/column lists...
		bool		getListsByApiCalls();
		bool		fakePrepareAndExecuteForApiCall(
						sqlrservercursor *cursor);
		bool		getDatabaseList(sqlrservercursor *cursor,
						const char *wild);
		bool		getSchemaList(sqlrservercursor *cursor,
						const char *wild);
		bool		getTableList(sqlrservercursor *cursor,
						const char *wild,
						uint16_t objecttypes);
		bool		getTableTypeList(sqlrservercursor *cursor,
						const char *wild);
		bool		getColumnList(sqlrservercursor *cursor,
						const char *table,
						const char *wild);
		bool		getPrimaryKeyList(sqlrservercursor *cursor,
						const char *table,
						const char *wild);
		bool		getKeyAndIndexList(sqlrservercursor *cursor,
						const char *table,
						const char *wild);
		bool		getProcedureBindAndColumnList(
						sqlrservercursor *cursor,
						const char *proc,
						const char *wild);
		bool		getTypeInfoList(sqlrservercursor *cursor,
						const char *type,
						const char *wild);
		bool		getProcedureList(sqlrservercursor *cursor,
						const char *wild);
		const char	*getDatabaseListQuery(bool wild);
		const char	*getSchemaListQuery(bool wild);
		const char	*getTableListQuery(bool wild,
						uint16_t objecttypes);
		const char	*getTableTypeListQuery(bool wild);
		const char	*getGlobalTempTableListQuery();
		const char	*getColumnListQuery(const char *table,
							bool wild);
		const char	*getPrimaryKeyListQuery(const char *table,
							bool wild);
		const char	*getKeyAndIndexListQuery(const char *table,
							bool wild);
		const char	*getProcedureBindAndColumnListQuery(
							const char *proc,
							bool wild);
		const char	*getTypeInfoListQuery(const char *type,
							bool wild);
		const char	*getProcedureListQuery(bool wild);
		void		splitObjectName(const char *fqobject,
						const char *currentcatalog,
						const char **catalog,
						const char **schema,
						const char **object);



		// column info...
		bool		columnInfoIsValidAfterPrepare(
						sqlrservercursor *cursor);
		uint16_t	getSendColumnInfo();
		void		setSendColumnInfo(uint16_t sendcolumninfo);
		uint32_t	colCount(sqlrservercursor *cursor);
		uint16_t	columnTypeFormat(sqlrservercursor *cursor);
		void		setDatabaseListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setSchemaListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setTableListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setTableTypeListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setColumnListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setPrimaryKeyListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setKeyAndIndexListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setProcedureBindAndColumnListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setTypeInfoListColumnMap(
					sqlrserverlistformat_t listformat);
		void		setProcedureListColumnMap(
					sqlrserverlistformat_t listformat);
		const char	*getColumnName(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnNameSize(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnType(sqlrservercursor *cursor,
							uint32_t col);
		const char	*getColumnTypeName(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnTypeNameSize(
						sqlrservercursor *cursor,
							uint32_t col);
		uint32_t	getColumnSize(sqlrservercursor *cursor,
							uint32_t col);
		uint32_t	getColumnPrecision(sqlrservercursor *cursor,
							uint32_t col);
		uint32_t	getColumnScale(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsNullable(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsPrimaryKey(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsUnique(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsPartOfKey(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsUnsigned(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsZeroFilled(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsBinary(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnIsAutoIncrement(
						sqlrservercursor *cursor,
							uint32_t col);
		const char	*getColumnTable(sqlrservercursor *cursor,
							uint32_t col);
		uint16_t	getColumnTableSize(sqlrservercursor *cursor,
							uint32_t col);

		/** Prepares and executes "query", but doesn't fetch any
		 *  results, and appends a comma-separated list of the names of
		 *  the columns in the result set to "output".
		 *
		 *  Returns false if "query" was null or an empty string and
		 *  true otherwise.  If the query fails then nothing is
		 *  appended to "output". */
		bool	getColumnNames(const char *query, stringbuffer *output);


		/** Appends a comma-separated list of the names of the columns
		 *  in the current result of "cursor" to "output".  Nothing is
		 *  appended to "output" if no query has been executed yet, or
		 *  if the query has no result set. */
		void	getColumnNames(sqlrservercursor *cursor,
							stringbuffer *output);

		bool		handleResultSetHeader(sqlrservercursor *cursor);



		// result set navigation...
		bool	knowsRowCount(sqlrservercursor *cursor);
		uint64_t	rowCount(sqlrservercursor *cursor);
		bool	knowsAffectedRows(sqlrservercursor *cursor);
		uint64_t	affectedRows(sqlrservercursor *cursor);
		bool	noRowsToReturn(sqlrservercursor *cursor);
		bool	skipRow(sqlrservercursor *cursor, bool *error);
		bool	skipRows(sqlrservercursor *cursor, uint64_t rows,
								bool *error);
		bool	fetchRow(sqlrservercursor *cursor, bool *error);
		void	nextRow(sqlrservercursor *cursor);
		uint64_t	getTotalRowsFetched(sqlrservercursor *cursor);

		/** Suspends the result set of "cursor". */
		void	suspendResultSet(sqlrservercursor *cursor);
		void	closeResultSet(sqlrservercursor *cursor);
		void	closeAllResultSets();



		// fields...
		bool	getField(sqlrservercursor *cursor,
						uint32_t col,
						const char **field,
						uint64_t *fieldsize,
						bool *blob,
						bool *null);
		bool	getLobFieldLength(sqlrservercursor *cursor,
						uint32_t col,
						uint64_t *length);
		bool	getLobFieldSegment(sqlrservercursor *cursor,
						uint32_t col,
						char *buffer,
						uint64_t buffersize,
						uint64_t offset,
						uint64_t charstoread,
						uint64_t *charsread);
		void	closeLobField(sqlrservercursor *cursor,
						uint32_t col);
		bool	reformatField(sqlrservercursor *cursor,
						const char *name,
						uint32_t index,
						const char **field,
						uint64_t *fieldsize);
		bool	reformatRow(sqlrservercursor *cursor,
						uint32_t colcount,
						const char * const *names,
						const char ***fields,
						uint64_t **fieldsizes);
		bool	reformatDateTimes(sqlrservercursor *cursor,
						uint32_t index,
						const char *field,
						uint64_t fieldsize,
						const char **newfield,
						uint64_t *newfieldsize,
						bool ddmm, bool yyyyddmm,
						bool ignorenondatetime,
						const char *datedelimiters,
						const char *datetimeformat,
						const char *dateformat,
						const char *timeformat);



		// errors...

		/** Fetches the current cursor-level error into the
		 *  cursor's error buffer, unless there is already an
		 *  error saved in the buffer. */
		void	saveError(sqlrservercursor *cursor);

		/** Returns the error message and code by:
		 *
		 *  Setting "errorbuffer" to the cursor-level error buffer.
		 *  Populating "errorsize" with the number of bytes in the
		 *  error buffer.
		 *  Populating "errorcode" with the cursor-level numeric
		 *  error code.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is still up.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is down. */
		void	getError(sqlrservercursor *cursor,
						const char **errorbuffer,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection);

		/** Returns the error message and code by:
		 *
		 *  Copying at most "errorbuffersize" bytes from the
		 *  cursor-level error buffer into "errorbuffer".
		 *  Populating "errorsize" with the number of bytes in the
		 *  error buffer.
		 *  Populating "errorcode" with the cursor-level numeric
		 *  error code.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is still up.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is down. */
		void	getError(sqlrservercursor *cursor,
						char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection);

		/** Empties the cursor-level error buffer and sets a flag
		 *  indicating that the connection to the database is up. */
		void	clearError(sqlrservercursor *cursor);

		/** Copies "errsize" bytes of "err" into the cursor-level
		 *  error buffer, sets the cursor-level numeric error code
		 *  to "errn" and sets a flag indicating that the connection to
		 *  the database is up. */
		void	setError(sqlrservercursor *cursor,
						const char *err,
						uint32_t errsize,
						int64_t errn,
						bool liveconn);

		/** Copies "err" into the cursor-level error buffer,
		 *  sets the cursor-level numeric error code to "errn" and
		 *  sets a flag indicating that the connection to the database
		 *  is up. */
		void	setError(sqlrservercursor *cursor,
						const char *err,
						int64_t errn,
						bool liveconn);

		/** Returns a pointer to the cursor-level error buffer. */
		char	*getErrorBuffer(sqlrservercursor *cursor);

		/** Returns the size, in bytes, of the cursor-level error
		 *  buffer. */
		uint32_t	getErrorBufferSize(sqlrservercursor *cursor);

		/** Sets the number of bytes currently stored in the
		 *  cursor-level error buffer to "errorsize". */
		void	setErrorSize(sqlrservercursor *cursor,
							uint32_t errorsize);

		/** Returns the number of bytes currently stored in the
		 *  cursor-level error buffer, as set by setErrorSize(). */
		uint32_t	getErrorSize(sqlrservercursor *cursor);

		/** Sets the cursor-level numeric error code to "errnum". */
		void	setErrorNumber(sqlrservercursor *cursor,
							uint32_t errnum);

		/** Returns the cursor-level numeric error code as set by
		 *  setErrorNumber(). */
		uint32_t	getErrorNumber(sqlrservercursor *cursor);

		/** Sets a flag indicating whether the connection to the
		 *  database is up to "liveconnection". */
		void	setLiveConnection(sqlrservercursor *cursor,
							bool liveconnection);

		/** Returns the flag indicating whether the connection to the
		 *  database is up, as set by setLiveConnection(). */
		bool	getLiveConnection(sqlrservercursor *cursor);



		// bulk load..
		bool	bulkLoadBegin(const char *id,
					const char *errorfieldtable,
					const char *errorrowtable,
					uint64_t maxerrorcount,
					bool droperrortables);
		bool	bulkLoadCheckpoint(const char *id);
		bool	bulkLoadPrepareQuery(const char *query,
						uint64_t querysize,
						uint16_t inbindcount,
						sqlrserverbindvar *inbinds);
		bool	bulkLoadCreateErrorTables(const char *query,
						uint64_t querysize,
						const char *errorfieldtable,
						const char *errorrowtable);
		bool	bulkLoadCreateErrorTable1(sqlrservercursor *cursor,
						const char *query,
						uint64_t querysize,
						const char *errorfieldtable);
		bool	bulkLoadCreateErrorTable2(sqlrservercursor *cursor,
						const char *query,
						uint64_t querysize,
						const char *errorrowtable);
		bool	bulkLoadJoin(const char *id);
		bool	bulkLoadInputBind(const byte_t *data,
						uint64_t datasize);
		void	bulkLoadParseInsert(const char *query,
						uint64_t querysize,
						char **table,
                                                linkedlist<char *> *cols,
                                                linkedlist<char *> *binds);
		bool	bulkLoadExecuteQuery();
		void	bulkLoadInitBinds();
		void	bulkLoadBindRow(const byte_t *data,
						uint64_t datasize);
		void	bulkLoadError();
		bool	bulkLoadStoreError(int64_t errorcode,
						const char *error,
						uint32_t errorsize,
						const char *errorfieldtable,
						const char *errorrowtable);
		bool	bulkLoadEnd();
		bool	bulkLoadDropErrorTables(const char *errorfieldtable,
						const char *errorrowtable);



		// cursor state...
		void			setState(sqlrservercursor *cursor,
						sqlrcursorstate_t state);
		sqlrcursorstate_t	getState(sqlrservercursor *cursor);



		// memory pools...
		memorypool	*getPerTransactionMemoryPool();
		memorypool	*getPerSessionMemoryPool();



		// query parser...
		sqlrparser	*getParser();



		// gss...
		gsscontext	*getGssContext();



		// tls...
		tlscontext	*getTlsContext();



		// configuration...
		sqlrconfig	*getConfig();
		sqlrpaths	*getPaths();



		// shared memory...
		sqlrshm		*getShm();



		// module data...
		sqlrmoduledata	*getModuleData(const char *id);



		// utilities...
		bool		skipComment(const char **ptr,
						const char *endptr);
		bool		skipWhitespace(const char **ptr,
						const char *endptr);
		const char	*skipWhitespace(const char *query);
		const char	*skipComments(const char *query);
		const char	*skipWhitespaceAndComments(const char *query);

		const char	*asciiToHex(byte_t ch);
		const char	*asciiToOctal(byte_t ch);

		bool		hasBindVariables(const char *query,
							uint32_t querysize);
		uint16_t	countBindVariables(const char *query,
							uint32_t querysize);
		void		splitObjectName(const char *currentdb,
						const char *currentschema,
						const char *combinedobject,
						const char **db,
						const char **schema,
						const char **object);
		bool		parseInsert(const char *query,
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
					const char **rawvalues);

		bool	isBitType(const char *type);
		bool	isBitType(int16_t type);
		bool	isBoolType(const char *type);
		bool	isBoolType(int16_t type);
		bool	isFloatType(const char *type);
		bool	isFloatType(int16_t type);
		bool	isNumberType(const char *type);
		bool	isNumberType(int16_t type);
		bool	isBlobType(const char *type);
		bool	isBlobType(int16_t type);
		bool	isUnsignedType(const char *type);
		bool	isUnsignedType(int16_t type);
		bool	isBinaryType(const char *type);
		bool	isBinaryType(int16_t type);
		bool	isDateTimeType(const char *type);
		bool	isDateTimeType(int16_t type);

		const char * const	*dataTypeStrings();

	#include <sqlrelay/private/sqlrservercontroller.h>
};

class SQLRSERVER_DLLSPEC sqlrserverconnection {
	public:
		sqlrserverconnection(sqlrservercontroller *cont);
		virtual	~sqlrserverconnection();

		virtual bool	mustDetachBeforeLogIn();

		virtual bool	supportsAuthOnDatabase();
		virtual	void	handleConnectString();



		// passthrough...

		/** Sends "size" bytes of "data" to the database. */
		virtual	bool	send(byte_t *data, size_t size);

		/** Receives "size" bytes from the database into buffer
 		 *  "data". */
		virtual	bool	recv(byte_t **data, size_t *size);

		virtual	bool	logIn(const char **error,
					const char **warning)=0;
		virtual	void	logOut()=0;

		virtual	bool	changeUser(const char *newuser,
						const char *newpassword);
		virtual	bool	changeProxiedUser(const char *newuser,
						const char *newpassword);

		/** Set auto-commit on.  Returns true on success and false on
		 *  failure. */
		virtual bool	setAutoCommitOn();

		/** Set auto-commit off.  Returns true on success and false on
		 *  failure. */
		virtual bool	setAutoCommitOff();

		/** Returns true if the database is transactional and false
		 *  otherwise.
		 *
		 *  Defaults to true but may be overridden by a child class. */
		virtual bool	isTransactional();

		/** Returns true if the database supports begin-commit/rollback
		 *  transaction blocks (eg. the behavior of most databases) and
		 *  false it a commit/rollback just begins another transaction
		 *  (eg. the behavior of oracle databases).
		 *
		 *  Defaults to true but may be overridden by a child class. */
		virtual bool	supportsTransactionBlocks();

		/** Returns true if the database supports auto-commit and false
		 *  if it does not.
		 *
		 *  Defaults to false but may be overridden by a child class. */
		virtual bool	supportsAutoCommit();

		/** Begins a new transaction.  Returns true on success and
		 *  false on failure. */
		virtual bool	begin();
		virtual const char	*beginTransactionQuery();

		/** Commits the current transaction.  Returns true on success
		 *  and false on failure. */
		virtual bool	commit();

		/** Rolls the current transaction back.  Returns true on success
		 *  and false on failure. */
		virtual bool	rollback();

		/** Returns the error message and code by:
		 *
		 *  Copying at most "errorbuffersize" bytes from the
		 *  connection-level error buffer into "errorbuffer".
		 *  Populating "errorsize" with the number of bytes in the
		 *  error buffer.
		 *  Populating "errorcode" with the connection-level numeric
		 *  error code.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is still up.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is down. */
		virtual	void	getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection)=0;

		/** Selects database "db".  Returns true if selection
		 *  succeeded and false otherwise. */
		virtual bool		selectDatabase(const char *database);
		virtual const char	*selectDatabaseQuery();

		/** Returns the current database.
		 *
		 *  Note that this method allocates a buffer for the return
		 *  value internally and returns it.  The calling method must
		 *  deallocate this buffer. */
		virtual char		*getCurrentDatabase();
		virtual const char	*getCurrentDatabaseQuery();

		/** Returns the current database.
		 *
		 *  Note that this method allocates a buffer for the return
		 *  value internally and returns it.  The calling method must
		 *  deallocate this buffer. */
		virtual char		*getCurrentSchema();
		virtual const char	*getCurrentSchemaQuery();

		virtual bool		getLastInsertId(uint64_t *id);
		virtual const char	*getLastInsertIdQuery();
		virtual const char	*noopQuery();

		virtual bool		setIsolationLevel(const char *isolevel);
		virtual const char	*setIsolationLevelQuery();

		/** Pings the database using the "ping query" defined by the
		 *  database connection module.  Returns true if the ping
		 *  succeeded and false if it failed. */
		virtual bool		ping();
		virtual const char	*pingQuery();

		/** Returns the type of database: oracle, mysql, postgresql,
		 *  odbc, etc. */
		virtual const char	*getDbType()=0;

		/** Returns the database version. */
		virtual	const char	*getDbVersion()=0;

		virtual const char	*getDbHostNameQuery();
		virtual const char	*getDbIpAddressQuery();

		/** Returns the host name of the server hosting the
		 *  database. */
		virtual const char	*getDbHostName();

		/** Returns the IP address of the server hosting the
		 *  database. */
		virtual const char	*getDbIpAddress();

		virtual bool		cacheDbHostInfo();

		virtual bool		getListsByApiCalls();
		virtual	sqlrserverlistformat_t
					getDatabaseListFormat();
		virtual	sqlrserverlistformat_t
					getSchemaListFormat();
		virtual	sqlrserverlistformat_t
					getTableListFormat();
		virtual	sqlrserverlistformat_t
					getTableTypeListFormat();
		virtual	sqlrserverlistformat_t
					getColumnListFormat();
		virtual	sqlrserverlistformat_t
					getPrimaryKeyListFormat();
		virtual	sqlrserverlistformat_t
					getKeyAndIndexListFormat();
		virtual	sqlrserverlistformat_t
					getProcedureBindAndColumnListFormat();
		virtual	sqlrserverlistformat_t
					getTypeInfoListFormat();
		virtual	sqlrserverlistformat_t
					getProcedureListFormat();
		virtual bool		getDatabaseList(
						sqlrservercursor *cursor,
						const char *wild);
		virtual bool		getSchemaList(
						sqlrservercursor *cursor,
						const char *wild);
		virtual bool		getTableList(
						sqlrservercursor *cursor,
						const char *wild,
						uint16_t objecttypes);
		virtual bool		getTableTypeList(
						sqlrservercursor *cursor,
						const char *wild);
		virtual bool		getColumnList(
						sqlrservercursor *cursor,
						const char *table,
						const char *wild);
		virtual bool		getPrimaryKeyList(
						sqlrservercursor *cursor,
						const char *table,
						const char *wild);
		virtual bool		getKeyAndIndexList(
						sqlrservercursor *cursor,
						const char *table,
						const char *wild);
		virtual bool		getProcedureBindAndColumnList(
						sqlrservercursor *cursor,
						const char *procedure,
						const char *wild);
		virtual bool		getTypeInfoList(
						sqlrservercursor *cursor,
						const char *type,
						const char *wild);
		virtual bool		getProcedureList(
						sqlrservercursor *cursor,
						const char *wild);
		virtual const char	*getDatabaseListQuery(bool wild);
		virtual const char	*getSchemaListQuery(bool wild);
		virtual const char	*getTableListQuery(bool wild,
						uint16_t objecttypes);
		virtual const char	*getTableListQuery(bool wild,
						uint16_t objecttypes,
						const char *extrawhere);
		virtual const char	*getTableTypeListQuery(bool wild);
		virtual const char	*getGlobalTempTableListQuery();
		virtual const char	*getColumnListQuery(
						const char *table,
						bool wild);
		virtual const char	*getPrimaryKeyListQuery(
						const char *table,
						bool wild);
		virtual const char	*getKeyAndIndexListQuery(
						const char *table,
						bool wild);
		virtual const char	*getProcedureBindAndColumnListQuery(
						const char *procedure,
						bool wild);
		virtual const char	*getTypeInfoListQuery(
						const char *type,
						bool wild);
		virtual const char	*getProcedureListQuery(
						bool wild);
		virtual bool		isSynonym(const char *table);
		virtual const char	*isSynonymQuery();

		virtual sqlrservercursor	*newCursor(uint16_t id)=0;
		virtual void			deleteCursor(
						sqlrservercursor *curs)=0;

		/** Returns a string representing the bind variable format used
		 *  by the database.  For example:
		 *
		 *  ?  - database uses a ? to represent a bind variable
		 *  @* - database uses a @ followed by any characters to
		 *       represent a bind variable
		 *  $1 - database uses a $ followed by a number to represent a
		 *       bind variable
		 *  :* - database uses a : followed by any characters to
		 *       represent a bind variable
		 *
		 *  Defaults to :* but may be overriden by a child class of
		 *  sqlrserverconnection. */
		virtual	const char	*getBindFormat();

		/** Returns the value that the database expects or returns in
		 *  the "null indicator" for a non-null bind value.
		 *
		 *  Defaults to 0 but may be overriden by a child class. */
		virtual	int16_t		getNonNullBindValue();

		/** Returns the value that the database expects or returns in
		 *  the "null indicator" for a null bind value.
		 *
		 *  Defaults to -1 but may be overriden by a child class of
		 *  sqlrserverconnection. */
		virtual	int16_t		getNullBindValue();

		/** Returns true if "isnull" matches the value that the database
		 *  expects or returns in the "null indicator" for a null bind
		 *  value. */
		virtual bool		getBindValueIsNull(int16_t isnull);

		/** Returns a string representing the format of the sequence
		 *  nextval command used in the database.  The format will
		 *  contain a %s in place of the sequence name.  For example:
		 *
		 *  (nextval for %s)
		 *  next value for %s
		 *  nextval('%s')
		 *  %s.nextval
		 *
		 *  Returns an empty string if the database does not support
		 *  sequences.
		 *
		 *  Defaults to %s.nextval but may be overriden by a child
		 *  class of sqlrserverconnection. */
		virtual const char	*getNextvalFormat();

		virtual const char	*tempTableDropPrefix();
		virtual bool		tempTableTruncateBeforeDrop();

		virtual void		endSession();

		/** Returns a pointer to the connection-level error buffer. */
		char		*getErrorBuffer();

		/** Returns the size, in bytes, of the connection-level error
		 *  buffer. */
		uint32_t	getErrorBufferSize();

		/** Sets the number of bytes currently stored in the
		 *  connection-level error buffer to "errorsize". */
		void		setErrorSize(uint32_t errorsize);

		/** Returns the number of bytes currently stored in the
		 *  connection-level error buffer, as set by setErrorSize(). */
		uint32_t	getErrorSize();

		/** Sets the connection-level numeric error code to "errnum". */
		void		setErrorNumber(uint32_t errnum);

		/** Returns the connection-level numeric error code as set by
		 *  setErrorNumber(). */
		uint32_t	getErrorNumber();

		/** Sets a flag indicating whether the connection to the
		 *  database is up to "liveconnection". */
		void		setLiveConnection(bool liveconnection);

		/** Returns the flag indicating whether the connection to the
		 *  database is up, as set by setLiveConnection(). */
		bool		getLiveConnection();

		sqlrservercontroller	*cont;

	#include <sqlrelay/private/sqlrserverconnection.h>
};

class SQLRSERVER_DLLSPEC sqlrservercursor {
	public:
		sqlrservercursor(sqlrserverconnection *conn, uint16_t id);
		virtual	~sqlrservercursor();

		/** Opens the cursor.  Returns true on success and false on
		 *  failure. */
		virtual	bool	open();

		/** Closes the cursor.  Returns true on success and false on
		 *  failure. */
		virtual	bool	close();

		virtual sqlrquerytype_t	determineQueryType(
						const char *query,
						uint32_t size);

		/** Returns true if we are handling the current query
 		 *  using a custom query module and false otherwise. */
		virtual	bool	isCustomQuery();

		virtual	bool	prepareQuery(const char *query,
							uint32_t size);
		virtual	bool	supportsNativeBinds(const char *query,
							uint32_t size);
		virtual	bool	inputBind(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		virtual	bool	inputBind(const char *variable, 
						uint16_t variablesize,
						int64_t *value);
		virtual	bool	inputBind(const char *variable, 
						uint16_t variablesize,
						double *value,
						uint32_t precision,
						uint32_t scale);
		virtual void	dateToString(char *buffer,
						uint16_t buffersize,
						int16_t year,
						int16_t month,
						int16_t day,
						int16_t hour,
						int16_t minute,
						int16_t second,
						int32_t microsecond,
						const char *tz,
						bool isnegative);
		virtual bool	inputBind(const char *variable,
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
						char *buffer,
						uint16_t buffersize,
						int16_t *isnull);
		virtual	bool	inputBindBlob(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		virtual	bool	inputBindClob(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		virtual	bool	outputBind(const char *variable, 
						uint16_t variablesize,
						char *value,
						uint32_t valuesize,
						int16_t *isnull);
		virtual	bool	outputBind(const char *variable, 
						uint16_t variablesize,
						int64_t *value,
						int16_t *isnull);
		virtual	bool	outputBind(const char *variable, 
						uint16_t variablesize,
						double *value,
						uint32_t *precision,
						uint32_t *scale,
						int16_t *isnull);
		virtual bool	outputBind(const char *variable,
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
						char *buffer,
						uint16_t buffersize,
						int16_t *isnull);
		virtual	bool	outputBindBlob(const char *variable, 
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		virtual	bool	outputBindClob(const char *variable, 
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		virtual	bool	outputBindCursor(const char *variable,
						uint16_t variablesize,
						sqlrservercursor *cursor);

		virtual	bool	inputOutputBind(const char *variable, 
						uint16_t variablesize,
						char *value,
						uint32_t valuesize,
						int16_t *isnull);
		virtual	bool	inputOutputBind(const char *variable, 
						uint16_t variablesize,
						int64_t *value,
						int16_t *isnull);
		virtual	bool	inputOutputBind(const char *variable, 
						uint16_t variablesize,
						double *value,
						uint32_t *precision,
						uint32_t *scale,
						int16_t *isnull);
		virtual bool	inputOutputBind(const char *variable,
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
						char *buffer,
						uint16_t buffersize,
						int16_t *isnull);
		virtual	bool	inputOutputBindBlob(const char *variable, 
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		virtual	bool	inputOutputBindClob(const char *variable, 
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		virtual bool	getLobInputOutputBindLength(uint16_t index,
							uint64_t *length);
		virtual bool	getLobInputOutputBindSegment(uint16_t index,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread);
		virtual void	closeLobInputOutputBind(uint16_t index);
		virtual void	checkForTempTable(const char *query,
							uint32_t size);
		virtual	const char	*truncateTableQuery();
		virtual	bool		executeQuery(const char *query,
							uint32_t size);

		/** Assumes that the current cursor is a bind cursor and
		 *  fetches from it.  Returns true on sucecss and false
		 *  otherwise. */
		virtual bool	fetchFromBindCursor();

		/** Advances to the next result set.
		 *
		 *  Returns true and sets "nextresultsetavailable" true if
		 *  another result set was available.
		 *
		 *  Returns true and sets "nextresultsetavailable" false if
		 *  another result set was not available.
		 *
		 *  Returns false if an error occured while checking for
		 *  another resulet set. */
		virtual	bool	nextResultSet(bool *nextresultsetavailable);

		virtual	bool	queryIsNotSelect();
		virtual	bool	queryIsCommitOrRollback();

		/** Returns the error message and code by:
		 *
		 *  Copying at most "errorbuffersize" bytes from the
		 *  cursor-level error buffer into "errorbuffer".
		 *  Populating "errorsize" with the number of bytes in the
		 *  error buffer.
		 *  Populating "errorcode" with the cursor-level numeric
		 *  error code.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is still up.
		 *  Populating "liveconnection" with true if the connection to
		 *  the database is down. */
		virtual	void	getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection);

		virtual bool		knowsRowCount();
		virtual uint64_t	rowCount();
		virtual bool		knowsAffectedRows();
		virtual uint64_t	affectedRows();
		virtual	uint32_t	colCount();
		virtual uint16_t	columnTypeFormat();
		virtual const char	*getColumnName(uint32_t col);
		virtual uint16_t	getColumnNameSize(uint32_t col);
		virtual uint16_t	getColumnType(uint32_t col);
		virtual const char	*getColumnTypeName(uint32_t col);
		virtual uint16_t	getColumnTypeNameSize(uint32_t col);
		virtual uint32_t	getColumnSize(uint32_t col);
		virtual uint32_t	getColumnPrecision(uint32_t col);
		virtual uint32_t	getColumnScale(uint32_t col);
		virtual uint16_t	getColumnIsNullable(uint32_t col);
		virtual uint16_t	getColumnIsPrimaryKey(uint32_t col);
		virtual uint16_t	getColumnIsUnique(uint32_t col);
		virtual uint16_t	getColumnIsPartOfKey(uint32_t col);
		virtual uint16_t	getColumnIsUnsigned(uint32_t col);
		virtual uint16_t	getColumnIsZeroFilled(uint32_t col);
		virtual uint16_t	getColumnIsBinary(uint32_t col);
		virtual uint16_t	getColumnIsAutoIncrement(uint32_t col);
		virtual const char	*getColumnTable(uint32_t col);
		virtual uint16_t	getColumnTableSize(uint32_t col);
		virtual bool		ignoreDateDdMmParameter(uint32_t col,
							const char *data,
							uint32_t size);
		virtual	bool	noRowsToReturn();
		virtual	bool	skipRow(bool *error);
		virtual	bool	fetchRow(bool *error);
		virtual	void	nextRow();
		virtual void	getField(uint32_t col,
						const char **field,
						uint64_t *fieldsize,
						bool *blob,
						bool *null);
		virtual bool	getLobFieldLength(uint32_t col,
						uint64_t *length);
		virtual bool	getLobFieldSegment(uint32_t col,
						char *buffer,
						uint64_t buffersize,
						uint64_t offset,
						uint64_t charstoread,
						uint64_t *charsread);
		virtual void	closeLobField(uint32_t col);
		virtual	void	closeResultSet();

		virtual void	encodeBlob(stringbuffer *buffer,
					const char *data, uint32_t datasize);

		virtual bool	columnInfoIsValidAfterPrepare();


		uint16_t	getId();

		bool		fakeInputBinds();

		/** Returns the memory pool used to store bind variable names
		 *  and values. */
		memorypool	*getBindPool();

		/** Returns the memory pool used to map bind variable names
		 *  when the value of the attribute translatebindvariables of
		 *  the instance tag in the config file is set to "yes". */
		memorypool	*getBindMappingsPool();

		/** Returns the dictionary of bind variable name mappings.  The
		 *  keys are the old (original) bind variable names and the
		 *  values are the new (translated) bind variable names.
		 *
		 *  The dictionary is populated by
		 *  sqlrservercontroller::prepareQuery() and remains populated
		 *  with the same key-value pairs until the next call to
		 *  prepareQuery().  If it is empty then either bind variable
		 *  translation is disabled or the most recently prepared query
		 *  contained no bind variables. */
		dictionary<char *, char *>	*getBindMappings();

		/** Sets the number of input binds in "cursor" to
		 *  "inbindcount". */
		void		setInputBindCount(uint16_t inbindcount);

		/** Returns the number of input binds in "cursor", as set by
		 *  setInputBindCount(). */
		uint16_t	getInputBindCount();

		/** Returns the array of input binds in "cursor".  The total
		 *  number of bind variables in the array is equal to the value
		 *  of the maxbindcount attribute of the instance tag in the
		 *  config file.  However, only the first getInputBindCount()
		 *  bind variables are currently valid.  The state of the rest
		 *  are undefined. */
		sqlrserverbindvar	*getInputBinds();

		/** Sets the number of valid output binds to "outbindcount". */
		void		setOutputBindCount(uint16_t outbindcount);

		/** Returns the number of valid output binds, as set by
		 *  setOutputBindCount(). */
		uint16_t	getOutputBindCount();

		/** Returns the array of output binds in "cursor".  The total
		 *  number of bind variables in the array is equal to the value
		 *  of the maxbindcount attribute of the instance tag in the
		 *  config file.  However, only the first getOutputBindCount()
		 *  bind variables are currently valid.  The state of the rest
		 *  are undefined. */
		sqlrserverbindvar	*getOutputBinds();

		/** Opens LOB output bind at position "index" (unless it is
		 *  already open) and sets "length" equal to its length, in
		 *  characters.
 		 *
 		 *  Returns true on success and false otherwise.  Will return
 		 *  false if the output bind at position "index" is not a LOB.
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  then this method will return true. */
		virtual bool	getLobOutputBindLength(uint16_t index,
							uint64_t *length);

		/** Opens LOB output bind at position "index" (unless it is
		 *  already open) and attempts to fetch "charstoread"
		 *  characters from position "offset" into "buffer" of
		 *  "buffersize" bytes.  Populates "charsread" with the number
		 *  of characters that were actually read.
 		 *
 		 *  Returns true on success and false otherwise.  Will return
 		 *  false if the output bind at position "index" is not a LOB.
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  then this method will return true. */
		virtual bool	getLobOutputBindSegment(uint16_t index,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread);

		/** Closes LOB output bind at position "index".
 		 *
 		 *  For example...
 		 *
 		 *  if
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_BLOB
 		 *  or
 		 *  getOutputBinds(cursor)[index].type==
 		 *  			SQLRSERVERBINDVARTYPE_CLOB
 		 *  and the LOB was opened by a call to
 		 *  getLobOutputBindLength() or getLobOutputBindSegment()
 		 *  then this method will close it. */
		virtual void	closeLobOutputBind(uint16_t index);

		/** Sets the number of valid input-output binds to
		 *  "inoutbindcount". */
		void		setInputOutputBindCount(
					uint16_t inoutbindcount);

		/** Returns the number of valid input-output binds as set by
		 *  setInputOutputBindCount(). */
		uint16_t	getInputOutputBindCount();

		/** Returns the array of input-output binds.  The total number
		 *  of bind variables in the array is equal to the value of the
		 *  maxbindcount attribute of the instance tag in the config
		 *  file.  However, only the first getInputOutputBindCount()
		 *  bind variables are currently valid.  The state of the rest
		 *  are undefined. */
		sqlrserverbindvar	*getInputOutputBinds();

		void	performSubstitution(stringbuffer *buffer,
							int16_t index);

		/** Immediately closes the result set of "cursor". */
		void	abort();

		/** Returns a pointer to the query buffer. */
		char		*getQueryBuffer();

		/** Sets the size, in bytes, of the query that is currently
		 *  present in the query buffer to "querysize". */
		void		setQuerySize(uint32_t querysize);

		/** Returns the size, in bytes, of the query that is currently
		 *  present in the query buffer, as set by setQuerySize(). */
		uint32_t 	getQuerySize();

		/** Sets the status of the current query to "status". */
		void	setQueryStatus(sqlrquerystatus_t status);

		/** Returns the status of the current query as set by
		 *  setQueryStatus(). */
		sqlrquerystatus_t	getQueryStatus();

		/** Sets the tree representing the current query to "tree". */
		void		setQueryTree(xmldom *tree);

		/** Returns the tree representing the current query as set by
		 *  setQueryTree(), or NULL if no tree has been set since
		 *  initialization or since the most recent call to
		 *  clearQueryTree(). */
		xmldom		*getQueryTree();

		/** Sets the tree representing the current query to NULL. */
		void		clearQueryTree();

		/** Returns the translated query buffer of "cursor". */
		stringbuffer	*getTranslatedQueryBuffer();

		/** Returns the query currently stored in the translated quer
		 *  buffer of "cursor". */
		const char	*getTranslatedQuery();

		void		setCommandStart(uint64_t sec, uint64_t usec);
		uint64_t	getCommandStartSec();
		uint64_t	getCommandStartUSec();

		void		setCommandEnd(uint64_t sec, uint64_t usec);
		uint64_t	getCommandEndSec();
		uint64_t	getCommandEndUSec();

		void		setQueryStart(uint64_t sec, uint64_t usec);
		uint64_t	getQueryStartSec();
		uint64_t	getQueryStartUSec();

		void		setQueryEnd(uint64_t sec, uint64_t usec);
		uint64_t	getQueryEndSec();
		uint64_t	getQueryEndUSec();

		void		setFetchStart(uint64_t sec, uint64_t usec);
		uint64_t	getFetchStartSec();
		uint64_t	getFetchStartUSec();

		void		setFetchEnd(uint64_t sec, uint64_t usec);
		uint64_t	getFetchEndSec();
		uint64_t	getFetchEndUSec();

		void		resetFetchTime();
		void		tallyFetchTime();
		uint64_t	getFetchUSec();

		void			setState(sqlrcursorstate_t state);
		sqlrcursorstate_t	getState();

		void		setCustomQueryCursor(sqlrquerycursor *cur);
		sqlrquerycursor	*getCustomQueryCursor();
		void		clearCustomQueryCursor();

		void		clearTotalRowsFetched();
		uint64_t	getTotalRowsFetched();
		void		incrementTotalRowsFetched();

		void		setCurrentRowReformatted(bool crr);
		bool		getCurrentRowReformatted();

		/** Returns a pointer to the cursor-level error buffer. */
		char		*getErrorBuffer();

		/** Returns the size, in bytes, of the cursor-level error
		 *  buffer. */
		uint32_t	getErrorBufferSize();

		/** Sets the number of bytes currently stored in the
		 *  cursor-level error buffer to "errorsize". */
		void		setErrorSize(uint32_t errorsize);

		/** Returns the number of bytes currently stored in the
		 *  cursor-level error buffer, as set by setErrorSize(). */
		uint32_t	getErrorSize();

		/** Sets the cursor-level numeric error code to "errnum". */
		void		setErrorNumber(uint32_t errnum);

		/** Returns the cursor-level numeric error code as set by
		 *  setErrorNumber(). */
		uint32_t	getErrorNumber();

		/** Sets a flag indicating whether the connection to the
		 *  database is up to "liveconnection". */
		void		setLiveConnection(bool liveconnection);

		/** Returns the flag indicating whether the connection to the
		 *  database is up, as set by setLiveConnection(). */
		bool		getLiveConnection();

		void		setCreateTempTablePattern(
						const char *createtemp);
		const char	*skipCreateTempTableClause(
						const char *query);

		void	setColumnInfoIsValid(bool valid);
		bool	getColumnInfoIsValid();

		void	setQueryHasBeenPreProcessed(bool preprocessed);
		bool	getQueryHasBeenPreProcessed();

		void	setQueryHasBeenPrepared(bool prepared);
		bool	getQueryHasBeenPrepared();

		void	setQueryHasBeenExecuted(bool executed);
		bool	getQueryHasBeenExecuted();

		void	setQueryNeedsIntercept(bool intercept);
		bool	getQueryNeedsIntercept();

		void	setQueryWasIntercepted(bool intercepted);
		bool	getQueryWasIntercepted();

		void	setBindsWereFaked(bool faked);
		bool	getBindsWereFaked();

		/** Sets whether to fake input binds for the current query of
		 *  "cursor", by rewriting the query, to "fake".
		 *
		 *  Note that the behavior of prepareQuery() is as follows:
		 *
		 *    * It initially configures whether input binds should be
		 *      faked or not, based on the value of the
		 *      "fakeinputbindvariables" attribute of the instance tag,
		 *      in the config file.
		 *    * It then processes filters, directives, translations,
		 *      and before-triggers, which can call this method to
		 *      override that.
		 *    * It then checks to see if the query supports native
		 *      binds and sets input binds to be faked if native binds
		 *      are not supported with this query.
		 *
		 *  So, it is possible for this method to hae no effect if:
		 *
		 *    * It is called from a module other than a filter,
		 *      directive, translation, or before-trigger.
		 *    * It sets binds to not be faked, but the query itself
		 *      doesn't support native binds.
		 */
		void	setFakeInputBindsForThisQuery(bool fake);

		/** Returns whether or not input binds will be faked for the
		 *  current query of "cursor".  See
		 *  setFakeInputBindsForThisQuery(). */
		bool	getFakeInputBindsForThisQuery();

		void		setQueryType(sqlrquerytype_t querytype);
		sqlrquerytype_t	getQueryType();

		stringbuffer	*getQueryWithFakeInputBindsBuffer();

		void	allocateColumnPointers(uint32_t colcount);
		void	deallocateColumnPointers();
		void	getColumnPointers(const char ***columnnames,
					uint16_t **columnnamesizes,
					uint16_t **columntypes,
					const char ***columntypenames,
					uint16_t **columntypenamesizes,
					uint32_t **columnsizes,
					uint32_t **columnprecisions,
					uint32_t **columnscales,
					uint16_t **columnisnullables,
					uint16_t **columnisprimarykeys,
					uint16_t **columnisuniques,
					uint16_t **columnispartofkeys,
					uint16_t **columnisunsigneds,
					uint16_t **columniszerofilleds,
					uint16_t **columnisbinarys,
					uint16_t **columnisautoincrements,
					const char ***columntables,
					uint16_t **columntablesizes);
		const char	*getColumnNameFromBuffer(uint32_t col);
		uint16_t	getColumnNameSizeFromBuffer(uint32_t col);
		uint16_t	getColumnTypeFromBuffer(uint32_t col);
		const char	*getColumnTypeNameFromBuffer(uint32_t col);
		uint16_t	getColumnTypeNameSizeFromBuffer(uint32_t col);
		uint32_t	getColumnSizeFromBuffer(uint32_t col);
		uint32_t	getColumnPrecisionFromBuffer(uint32_t col);
		uint32_t	getColumnScaleFromBuffer(uint32_t col);
		uint16_t	getColumnIsNullableFromBuffer(uint32_t col);
		uint16_t	getColumnIsPrimaryKeyFromBuffer(uint32_t col);
		uint16_t	getColumnIsUniqueFromBuffer(uint32_t col);
		uint16_t	getColumnIsPartOfKeyFromBuffer(uint32_t col);
		uint16_t	getColumnIsUnsignedFromBuffer(uint32_t col);
		uint16_t	getColumnIsZeroFilledFromBuffer(uint32_t col);
		uint16_t	getColumnIsBinaryFromBuffer(uint32_t col);
		uint16_t	getColumnIsAutoIncrementFromBuffer(
							uint32_t col);
		const char	*getColumnTableFromBuffer(uint32_t col);
		uint16_t	getColumnTableSizeFromBuffer(uint32_t col);

		void	allocateFieldPointers(uint32_t colcount);
		void	deallocateFieldPointers();
		void	getFieldPointers(const char ***fieldnames,
					const char ***fields,
					uint64_t **fieldsizes,
					bool **blob,
					bool **null);

		void		setQueryTimeout(uint64_t querytimeout);
		uint64_t	getQueryTimeout();
		void		setExecuteDirect(bool executedirect);
		bool		getExecuteDirect();
		void		setExecuteRpc(bool executerpc);
		bool		getExecuteRpc();

		/** Sets the number of rows to fetch at once to
		 *  "fetchatonce". */
		void		setFetchAtOnce(uint32_t fetchatonce);

		/** Returns the number of rows that will be fetched at once. */
		uint32_t	getFetchAtOnce();

		void		setResultSetHeaderHasBeenHandled(
					bool resultsetheaderhasbeenhandled);
		bool		getResultSetHeaderHasBeenHandled();

		byte_t		*getModuleData();

		sqlrserverconnection	*conn;

	#include <sqlrelay/private/sqlrservercursor.h>
};

enum clientsessionexitstatus_t {
	CLIENTSESSIONEXITSTATUS_ERROR=0,
	CLIENTSESSIONEXITSTATUS_CLOSED_CONNECTION,
	CLIENTSESSIONEXITSTATUS_ENDED_SESSION,
	CLIENTSESSIONEXITSTATUS_SUSPENDED_SESSION
};

class SQLRSERVER_DLLSPEC sqlrprotocol {
	public:
		sqlrprotocol(sqlrservercontroller *cont,
					sqlrprotocols *ps,
					domnode *parameters);
		virtual	~sqlrprotocol();

		virtual clientsessionexitstatus_t
				clientSession(filedescriptor *clientsock)=0;

		virtual	bool		useKrb();
		virtual gsscontext	*getGssContext();

		virtual	bool		useTls();
		virtual tlscontext	*getTlsContext();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrprotocols		*getProtocols();
		domnode			*getParameters();

		void	setProtocolIsBigEndian(bool bigendian);
		bool	getProtocolIsBigEndian();

		void	read(const byte_t *rp,
					char *value,
					const byte_t **rpout);
		bool	read(const byte_t *rp,
					char *value,
					const char *name,
					char expected,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					byte_t *value,
					const byte_t **rpout);
		bool	read(const byte_t *rp,
					byte_t *value,
					const char *name,
					byte_t expected,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					char *value,
					size_t size,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					byte_t *value,
					size_t size,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					ucs2_t *value,
					size_t size,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					float *value,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					double *value,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					uint16_t *value,
					const byte_t **rpout);
		void	readLE(const byte_t *rp,
					uint16_t *value,
					const byte_t **rpout);
		bool	readLE(const byte_t *rp,
					uint16_t *value,
					const char *name,
					uint16_t expected,
					const byte_t **rpout);
		void	readBE(const byte_t *rp,
					uint16_t *value,
					const byte_t **rpout);
		bool	readBE(const byte_t *rp,
					uint16_t *value,
					const char *name,
					uint16_t expected,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					uint32_t *value,
					const byte_t **rpout);
		void	readLE(const byte_t *rp,
					uint32_t *value,
					const byte_t **rpout);
		bool	readLE(const byte_t *rp,
					uint32_t *value,
					const char *name,
					uint32_t expected,
					const byte_t **rpout);
		void	readBE(const byte_t *rp,
					uint32_t *value,
					const byte_t **rpout);
		bool	readBE(const byte_t *rp,
					uint32_t *value,
					const char *name,
					uint32_t expected,
					const byte_t **rpout);
		void	read(const byte_t *rp,
					uint64_t *value,
					const byte_t **rpout);
		void	readLE(const byte_t *rp,
					uint64_t *value,
					const byte_t **rpout);
		bool	readLE(const byte_t *rp,
					uint64_t *value,
					const char *name,
					uint64_t expected,
					const byte_t **rpout);
		void	readBE(const byte_t *rp,
					uint64_t *value,
					const byte_t **rpout);
		bool	readBE(const byte_t *rp,
					uint64_t *value,
					const char *name,
					uint64_t expected,
					const byte_t **rpout);
		uint64_t	readLenEncInt(const byte_t *in,
						const byte_t **out);

		void	write(bytebuffer *buffer, char value);
		void	write(bytebuffer *buffer, byte_t value);
		void	write(bytebuffer *buffer, const char *value);
		void	write(bytebuffer *buffer, const char *value,
								size_t size);
		void	write(bytebuffer *buffer, const byte_t *value,
								size_t size);
		void	write(bytebuffer *buffer, const ucs2_t *str,
								size_t size);
		void	write(bytebuffer *buffer, float value);
		void	write(bytebuffer *buffer, double value);
		void	write(bytebuffer *buffer, uint16_t value);
		void	writeLE(bytebuffer *buffer, uint16_t value);
		void	writeBE(bytebuffer *buffer, uint16_t value);
		void	write(bytebuffer *buffer, uint32_t value);
		void	writeLE(bytebuffer *buffer, uint32_t value);
		void	writeBE(bytebuffer *buffer, uint32_t value);
		void	write(bytebuffer *buffer, uint64_t value);
		void	writeLE(bytebuffer *buffer, uint64_t value);
		void	writeBE(bytebuffer *buffer, uint64_t value);
		void	writeLenEncInt(bytebuffer *buffer,
						uint64_t value);
		void	writeLenEncStr(bytebuffer *buffer,
						const char *string);
		void	writeLenEncStr(bytebuffer *buffer,
						const char *string,
						uint64_t size);
		void	writeTriplet(bytebuffer *buffer, uint32_t value);

		uint16_t	toHost(uint16_t value);
		uint32_t	toHost(uint32_t value);
		uint64_t	toHost(uint64_t value);
		uint16_t	leToHost(uint16_t value);
		uint32_t	leToHost(uint32_t value);
		uint64_t	leToHost(uint64_t value);
		uint16_t	beToHost(uint16_t value);
		uint32_t	beToHost(uint32_t value);
		uint64_t	beToHost(uint64_t value);

		uint16_t	hostTo(uint16_t value);
		uint32_t	hostTo(uint32_t value);
		uint64_t	hostTo(uint64_t value);
		uint16_t	hostToLE(uint16_t value);
		uint32_t	hostToLE(uint32_t value);
		uint64_t	hostToLE(uint64_t value);
		uint16_t	hostToBE(uint16_t value);
		uint32_t	hostToBE(uint32_t value);
		uint64_t	hostToBE(uint64_t value);

		bool	getDebug();

		void	debugStart(const char *title);
		void	debugStart(const char *title, uint16_t indent);
		void	debugEnd();
		void	debugEnd(uint16_t indent);

		void	debugHexDump(const byte_t *data,
						uint64_t size);
		void	debugHexDump(const byte_t *data,
						uint64_t size,
						uint16_t indent);

		sqlrservercontroller	*cont;

	#include <sqlrelay/private/sqlrprotocol.h>
};

class SQLRSERVER_DLLSPEC sqlrprotocols {
	public:
		sqlrprotocols(sqlrservercontroller *cont);
		~sqlrprotocols();

		bool		load(domnode *listeners);
		sqlrprotocol	*getProtocol(uint16_t port);

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrprotocols.h>
};

class SQLRSERVER_DLLSPEC sqlrcredentials {
	public:
		sqlrcredentials();
		virtual	~sqlrcredentials();
		virtual const char	*getType()=0;
};

class SQLRSERVER_DLLSPEC sqlruserpasswordcredentials : public sqlrcredentials {
	public:
		sqlruserpasswordcredentials();
		~sqlruserpasswordcredentials();
		const char	*getType();

		void	setUser(const char *user);
		void	setPassword(const char *password);

		const char	*getUser();
		const char	*getPassword();

	#include <sqlrelay/private/sqlruserpasswordcredentials.h>
};

class SQLRSERVER_DLLSPEC sqlrgsscredentials : public sqlrcredentials {
	public:
		sqlrgsscredentials();
		~sqlrgsscredentials();
		const char	*getType();

		void		setInitiator(const char *initiator);
		const char	*getInitiator();

	#include <sqlrelay/private/sqlrgsscredentials.h>
};

class SQLRSERVER_DLLSPEC sqlrtlscredentials : public sqlrcredentials {
	public:
		sqlrtlscredentials();
		~sqlrtlscredentials();
		const char	*getType();

		void	setCommonName(const char *commonname);
		void	setSubjectAlternateNames(
				linkedlist < char * > *subjectalternatenames);

		const char		*getCommonName();
		linkedlist< char * >	*getSubjectAlternateNames();

	#include <sqlrelay/private/sqlrtlscredentials.h>
};

class SQLRSERVER_DLLSPEC sqlrmysqlcredentials : public sqlrcredentials {
	public:
		sqlrmysqlcredentials();
		~sqlrmysqlcredentials();
		const char	*getType();

		void	setUser(const char *user);
		void	setPassword(const char *password);
		void	setPasswordSize(uint64_t passwordsize);
		void	setMethod(const char *method);
		void	setExtra(const char *extra);

		const char	*getUser();
		const char	*getPassword();
		uint64_t	getPasswordSize();
		const char	*getMethod();
		const char	*getExtra();

	#include <sqlrelay/private/sqlrmysqlcredentials.h>
};

class SQLRSERVER_DLLSPEC sqlrpostgresqlcredentials : public sqlrcredentials {
	public:
		sqlrpostgresqlcredentials();
		~sqlrpostgresqlcredentials();
		const char	*getType();

		void	setUser(const char *user);
		void	setPassword(const char *password);
		void	setPasswordSize(uint64_t passwordsize);
		void	setMethod(const char *method);
		void	setSalt(uint32_t salt);

		const char	*getUser();
		const char	*getPassword();
		uint64_t	getPasswordSize();
		const char	*getMethod();
		uint32_t	getSalt();

	#include <sqlrelay/private/sqlrpostgresqlcredentials.h>
};

class SQLRSERVER_DLLSPEC sqlroraclecredentials : public sqlrcredentials {
	public:
			sqlroraclecredentials();
		virtual	~sqlroraclecredentials();
		const char	*getType();

		void	setUser(const char *user);
		void	setPassword(const char *password);
		void	setPasswordSize(uint64_t passwordsize);
		void	setMethod(const char *method);
		void	setExtra(const char *extra);

		const char	*getUser();
		const char	*getPassword();
		uint64_t	getPasswordSize();
		const char	*getMethod();
		const char	*getExtra();

	#include <sqlrelay/private/sqlroraclecredentials.h>
};

class SQLRSERVER_DLLSPEC sqlrteradatacredentials : public sqlrcredentials {
	public:
			sqlrteradatacredentials();
		virtual	~sqlrteradatacredentials();
		const char	*getType();

		void	setClientFileDescriptor(filedescriptor *fd);

		filedescriptor	*getClientFileDescriptor();

	#include <sqlrelay/private/sqlrteradatacredentials.h>
};

class SQLRSERVER_DLLSPEC sqlrauth {
	public:
		sqlrauth(sqlrservercontroller *cont,
					sqlrauths *auths,
					sqlrpwdencs *sqlrpe,
					domnode *parameters);
		virtual	~sqlrauth();
		virtual	const char	*auth(sqlrcredentials *cred);

	protected:
		sqlrauths	*getAuths();
		sqlrpwdencs	*getPasswordEncryptions();
		domnode	*getParameters();

		sqlrservercontroller	*cont;

	#include <sqlrelay/private/sqlrauth.h>
};

class SQLRSERVER_DLLSPEC sqlrauths {
	public:
		sqlrauths(sqlrservercontroller *cont);
		~sqlrauths();

		bool		load(domnode *parameters,
					sqlrpwdencs *sqlrpe);
		const char	*auth(sqlrcredentials *cred);

		void	endSession();

	#include <sqlrelay/private/sqlrauths.h>
};

enum sqlrevent_t {
	SQLREVENT_CLIENT_CONNECTED=0,
	SQLREVENT_CLIENT_CONNECTION_REFUSED,
	SQLREVENT_CLIENT_DISCONNECTED,
	SQLREVENT_CLIENT_PROTOCOL_ERROR,
	SQLREVENT_DB_LOGIN,
	SQLREVENT_DB_LOGOUT,
	SQLREVENT_DB_ERROR,
	SQLREVENT_DB_WARNING,
	SQLREVENT_QUERY,
	SQLREVENT_FILTER_VIOLATION,
	SQLREVENT_INTERNAL_ERROR,
	SQLREVENT_INTERNAL_WARNING,
	SQLREVENT_DEBUG_MESSAGE,
	SQLREVENT_SCHEDULE_VIOLATION,
	SQLREVENT_INTEGRITY_VIOLATION,
	SQLREVENT_TRANSLATION_FAILURE,
	SQLREVENT_PARSE_FAILURE,
	SQLREVENT_CURSOR_OPEN,
	SQLREVENT_CURSOR_CLOSE,
	SQLREVENT_BEGIN_TRANSACTION,
	SQLREVENT_COMMIT,
	SQLREVENT_ROLLBACK,
	SQLREVENT_INVALID_EVENT
};

enum sqlrlogger_loglevel_t {
	SQLRLOGGER_LOGLEVEL_DEBUG=0,
	SQLRLOGGER_LOGLEVEL_INFO,
	SQLRLOGGER_LOGLEVEL_WARNING,
	SQLRLOGGER_LOGLEVEL_ERROR
};

class SQLRSERVER_DLLSPEC sqlrlogger {
	public:
		sqlrlogger(sqlrloggers *ls, domnode *parameters);
		virtual	~sqlrlogger();

		virtual bool	init(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon);
		virtual bool	run(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrlogger_loglevel_t level,
					sqlrevent_t event,
					const char *info);
		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrloggers	*getLoggers();
		domnode	*getParameters();

	#include <sqlrelay/private/sqlrlogger.h>
};

class SQLRSERVER_DLLSPEC sqlrloggers {
	public:
		sqlrloggers(sqlrpaths *sqlrpth);
		~sqlrloggers();

		bool	load(domnode *parameters);
		void	init(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon);
		void	run(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrlogger_loglevel_t level,
				sqlrevent_t event,
				const char *info);

		void	endTransaction(bool commit);
		void	endSession();

		const char	*logLevel(sqlrlogger_loglevel_t level);
		sqlrlogger_loglevel_t	logLevel(const char *level);

		const char	*eventType(sqlrevent_t event);
		sqlrevent_t	eventType(const char *event);

	#include <sqlrelay/private/sqlrloggers.h>
};

class SQLRSERVER_DLLSPEC sqlrnotification {
	public:
		sqlrnotification(sqlrnotifications *ns,
					domnode *parameters);
		virtual	~sqlrnotification();

		virtual bool	run(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrevent_t event,
					const char *info);
		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrnotifications	*getNotifications();
		domnode		*getParameters();

	#include <sqlrelay/private/sqlrnotification.h>
};

class SQLRSERVER_DLLSPEC sqlrnotifications {
	public:
		sqlrnotifications(sqlrpaths *sqlrpth);
		~sqlrnotifications();

		bool	load(domnode *parameters);
		void	run(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrevent_t event,
					const char *info);

		void	endTransaction(bool commit);
		void	endSession();

		const char	*eventType(sqlrevent_t event);
		sqlrevent_t	eventType(const char *event);

		bool	sendNotification(sqlrlistener *sqlrl,
						sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char *address,
						const char *transportid,
						const char *subject,
						const char *templatefile,
						sqlrevent_t event,
						const char *info);

		domnode	*getTransport(const char *transportid);

	#include <sqlrelay/private/sqlrnotifications.h>
};

class SQLRSERVER_DLLSPEC sqlrscheduleperiod {
	public:
		uint16_t	start;
		uint16_t	end;
};

class SQLRSERVER_DLLSPEC sqlrscheduledaypart {
	public:
		uint16_t	starthour;
		uint16_t	startminute;
		uint16_t	endhour;
		uint16_t	endminute;
};

class SQLRSERVER_DLLSPEC sqlrschedulerule {
	public:
		sqlrschedulerule(bool allow, const char *when);
		sqlrschedulerule(bool allow,
			const char *years,
			const char *months,
			const char *daysofmonth,
			const char *daysofweek,
			const char *dayparts);
		~sqlrschedulerule();

		bool	allowed(datetime *dt, bool currentlyallowed);
		
	#include <sqlrelay/private/sqlrschedulerule.h>
};

class SQLRSERVER_DLLSPEC sqlrschedule {
	public:
		sqlrschedule(sqlrservercontroller *cont,
					sqlrschedules *ss,
					domnode *parameters);
		virtual	~sqlrschedule();

		virtual bool	allowed(sqlrserverconnection *sqlrcon,
							const char *user);

		virtual	void	addRule(bool allow, const char *when);
		virtual	void	addRule(bool allow,
					const char *years,
					const char *months,
					const char *daysofmonth,
					const char *daysofweek,
					const char *dayparts);

		virtual	bool	rulesAllow(datetime *dt, bool currentlyallowed);

	protected:
		sqlrschedules	*getSchedules();
		domnode	*getParameters();

	#include <sqlrelay/private/sqlrschedule.h>
};

class SQLRSERVER_DLLSPEC sqlrschedules {
	public:
		sqlrschedules(sqlrservercontroller *cont);
		~sqlrschedules();

		bool	load(domnode *parameters);
		bool	allowed(sqlrserverconnection *sqlrcon,
						const char *user);

		void	endSession();

	#include <sqlrelay/private/sqlrschedules.h>
};

class SQLRSERVER_DLLSPEC sqlrrouter {
	public:
		sqlrrouter(sqlrservercontroller *cont,
				sqlrrouters *rs,
				domnode *parameters);
		virtual	~sqlrrouter();

		virtual const char *route(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char **err,
						int64_t *errn);

		virtual	bool	routeEntireSession();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrrouters	*getRouters();
		domnode	*getParameters();
		const char 	**getConnectionIds();
		sqlrconnection 	**getConnections();
		uint16_t	getConnectionCount();

	#include <sqlrelay/private/sqlrrouter.h>
};

class SQLRSERVER_DLLSPEC sqlrrouters {
	public:
		sqlrrouters(sqlrservercontroller *cont,
				const char **connectionids,
				sqlrconnection **connections,
				uint16_t connectioncount);
		~sqlrrouters();

		bool		load(domnode *parameters);
		const char	*route(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char **err,
						int64_t *errn);
		bool	routeEntireSession();

		void	endTransaction(bool commit);
		void	endSession();

		const char	*getCurrentConnectionId();
		const char 	**getConnectionIds();
		sqlrconnection 	**getConnections();
		uint16_t	getConnectionCount();

	#include <sqlrelay/private/sqlrrouters.h>
};

class SQLRSERVER_DLLSPEC sqlrparser {
	public:
		sqlrparser(sqlrservercontroller *cont,
				domnode *parameters);
		virtual	~sqlrparser();

		virtual	bool	parse(const char *query);
		virtual	void	useTree(xmldom *tree);
		virtual	xmldom	*getTree();
		virtual	xmldom	*detachTree();

		virtual	bool	write(stringbuffer *output);
		virtual	bool	write(domnode *node,
					stringbuffer *output,
					bool omitsiblings);
		virtual	bool	write(domnode *node, 
					stringbuffer *output);

		virtual void	getMetaData(domnode *node);

		virtual void	endSession();

	protected:
		domnode	*getParameters();

	#include <sqlrelay/private/sqlrparser.h>
};

class SQLRSERVER_DLLSPEC sqlrdirective {
	public:
		sqlrdirective(sqlrservercontroller *cont,
					sqlrdirectives *sqlts,
					domnode *parameters);
		virtual	~sqlrdirective();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query);
	protected:
		sqlrdirectives	*getDirectives();
		domnode	*getParameters();
		bool		getDirective(const char *line,
						const char **directivestart,
						uint32_t *directivesize,
						const char **newline);

	#include <sqlrelay/private/sqlrdirective.h>
};

class SQLRSERVER_DLLSPEC sqlrdirectives {
	public:
		sqlrdirectives(sqlrservercontroller *cont);
		~sqlrdirectives();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query);

	#include <sqlrelay/private/sqlrdirectives.h>
};

class SQLRSERVER_DLLSPEC sqlrquerytranslation {
	public:
		sqlrquerytranslation(sqlrservercontroller *cont,
					sqlrquerytranslations *sqlts,
					domnode *parameters);
		virtual	~sqlrquerytranslation();

		virtual bool	usesTree();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query,
					uint32_t querysize,
					stringbuffer *translatedquery);

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					xmldom *querytree);

		virtual const char	*getError();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrquerytranslations	*getQueryTranslations();
		domnode			*getParameters();

	#include <sqlrelay/private/sqlrquerytranslation.h>
};

class SQLRSERVER_DLLSPEC sqlrquerytranslations {
	public:
		sqlrquerytranslations(sqlrservercontroller *cont);
		~sqlrquerytranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						sqlrparser *sqlrp,
						const char *query,
						uint32_t querysize,
						stringbuffer *translatedquery);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

		void	setReplacementTableName(const char *database,
						const char *schema,
						const char *oldtable,
						const char *newtable);
		void	setReplacementIndexName(const char *database,
						const char *schema,
						const char *oldindex,
						const char *newindex,
						const char *table);

		bool	getReplacementTableName(const char *database,
						const char *schema,
						const char *oldtable,
						const char **newtable);
		bool	getReplacementIndexName(const char *database,
						const char *schema,
						const char *oldtable,
						const char **newtable);

		bool	removeReplacementTable(const char *database,
						const char *schema,
						const char *table);
		bool	removeReplacementIndex(const char *database,
						const char *schema,
						const char *index);

		bool	getUseOriginalOnError();

	#include <sqlrelay/private/sqlrquerytranslations.h>
};

class SQLRSERVER_DLLSPEC sqlrfilter {
	public:
		sqlrfilter(sqlrservercontroller *cont,
					sqlrfilters *fs,
					domnode *parameters);
		virtual	~sqlrfilter();

		virtual bool	usesTree();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query);

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					xmldom *querytree);

		virtual void	getError(const char **err, int64_t *errn);

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrfilters	*getFilters();
		domnode	*getParameters();

	#include <sqlrelay/private/sqlrfilter.h>
};

class SQLRSERVER_DLLSPEC sqlrfilters {
	public:
		sqlrfilters(sqlrservercontroller *cont);
		~sqlrfilters();

		bool	load(domnode *parameters);
		bool	runBeforeFilters(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						sqlrparser *sqlrp,
						const char *query,
						const char **err,
						int64_t *errn);
		bool	runAfterFilters(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						sqlrparser *sqlrp,
						const char *query,
						const char **err,
						int64_t *errn);

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrfilters.h>
};

class SQLRSERVER_DLLSPEC sqlrbindvariabletranslation {
	public:
		sqlrbindvariabletranslation(sqlrservercontroller *cont,
					sqlrbindvariabletranslations *bvts,
					domnode *parameters);
		virtual	~sqlrbindvariabletranslation();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur);

		virtual const char	*getError();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrbindvariabletranslations	*getBindVariableTranslations();
		domnode				*getParameters();

	#include <sqlrelay/private/sqlrbindvariabletranslation.h>
};

class SQLRSERVER_DLLSPEC sqlrbindvariabletranslations {
	public:
		sqlrbindvariabletranslations(sqlrservercontroller *cont);
		~sqlrbindvariabletranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrbindvariabletranslations.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsettranslation {
	public:
		sqlrresultsettranslation(sqlrservercontroller *cont,
						sqlrresultsettranslations *rs,
						domnode *parameters);
		virtual	~sqlrresultsettranslation();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *fieldname,
					uint32_t fieldindex,
					const char **field,
					uint64_t *fieldsize);

		virtual const char	*getError();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrresultsettranslations	*getResultSetTranslations();
		domnode			*getParameters();

	#include <sqlrelay/private/sqlrresultsettranslation.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsettranslations {
	public:
		sqlrresultsettranslations(sqlrservercontroller *cont);
		~sqlrresultsettranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char *fieldname,
						uint32_t fieldindex,
						const char **field,
						uint64_t *fieldsize);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrresultsettranslations.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowtranslation {
	public:
		sqlrresultsetrowtranslation(
					sqlrservercontroller *cont,
					sqlrresultsetrowtranslations *rs,
					domnode *parameters);
		virtual	~sqlrresultsetrowtranslation();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames,
					const char ***fields,
					uint64_t **fieldsizes);

		virtual const char	*getError();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrresultsetrowtranslations	*getResultSetRowTranslations();
		domnode			*getParameters();

	#include <sqlrelay/private/sqlrresultsetrowtranslation.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowtranslations {
	public:
		sqlrresultsetrowtranslations(sqlrservercontroller *cont);
		~sqlrresultsetrowtranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						uint32_t colcount,
						const char * const *fieldnames,
						const char ***fields,
						uint64_t **fieldsizes);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrresultsetrowtranslations.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowblocktranslation {
	public:
		sqlrresultsetrowblocktranslation(
					sqlrservercontroller *cont,
					sqlrresultsetrowblocktranslations *rs,
					domnode *parameters);
		virtual	~sqlrresultsetrowblocktranslation();

		virtual bool	setRow(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames,
					const char * const *fields,
					uint64_t *fieldsizes,
					bool *blobs,
					bool *nulls);
		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames);
		virtual bool	getRow(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char ***fields,
					uint64_t **fieldsizes,
					bool **blobs,
					bool **nulls);

		virtual const char	*getError();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrresultsetrowblocktranslations
					*getResultSetRowBlockTranslations();
		domnode			*getParameters();

	#include <sqlrelay/private/sqlrresultsetrowblocktranslation.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowblocktranslations {
	public:
		sqlrresultsetrowblocktranslations(sqlrservercontroller *cont);
		~sqlrresultsetrowblocktranslations();

		bool	load(domnode *parameters);

		uint64_t	getRowBlockCount();

		bool	setRow(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames,
					const char * const *fields,
					uint64_t *fieldsizes,
					bool *blobs,
					bool *nulls);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames);
		bool	getRow(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char ***fields,
					uint64_t **fieldsizes,
					bool **blobs,
					bool **nulls);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrresultsetrowblocktranslations.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsetheadertranslation {
	public:
		sqlrresultsetheadertranslation(
					sqlrservercontroller *cont,
					sqlrresultsetheadertranslations *rs,
					domnode *parameters);
		virtual	~sqlrresultsetheadertranslation();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char ***columnnames,
					uint16_t **columnnamesizes,
					uint16_t **columntypes,
					const char ***columntypenames,
					uint16_t **columntypenamesizes,
					uint32_t **columnsizes,
					uint32_t **columnprecisions,
					uint32_t **columnscales,
					uint16_t **columnisnullables,
					uint16_t **columnisprimarykeys,
					uint16_t **columnisuniques,
					uint16_t **columnispartofkeys,
					uint16_t **columnisunsigneds,
					uint16_t **columniszerofilleds,
					uint16_t **columnisbinarys,
					uint16_t **columnisautoincrements,
					const char ***columntables,
					uint16_t **columntablesizes);

		virtual const char	*getError();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrresultsetheadertranslations
					*getResultSetHeaderTranslations();
		domnode		*getParameters();

	#include <sqlrelay/private/sqlrresultsetheadertranslation.h>
};

class SQLRSERVER_DLLSPEC sqlrresultsetheadertranslations {
	public:
		sqlrresultsetheadertranslations(sqlrservercontroller *cont);
		~sqlrresultsetheadertranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char ***columnnames,
					uint16_t **columnnamesizes,
					uint16_t **columntypes,
					const char ***columntypenames,
					uint16_t **columntypenamesizes,
					uint32_t **columnsizes,
					uint32_t **columnprecisions,
					uint32_t **columnscales,
					uint16_t **columnisnullables,
					uint16_t **columnisprimarykeys,
					uint16_t **columnisuniques,
					uint16_t **columnispartofkeys,
					uint16_t **columnisunsigneds,
					uint16_t **columniszerofilleds,
					uint16_t **columnisbinarys,
					uint16_t **columnisautoincrements,
					const char ***columntables,
					uint16_t **columntablesizes);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrresultsetheadertranslations.h>
};

class SQLRSERVER_DLLSPEC sqlrerrortranslation {
	public:
		sqlrerrortranslation(sqlrservercontroller *cont,
					sqlrerrortranslations *sqlts,
					domnode *parameters);
		virtual	~sqlrerrortranslation();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					int64_t errornumber,
					const char *error,
					uint32_t errorsize,
					int64_t *translatederrornumber,
					stringbuffer *translatederror);

		virtual const char	*getError();

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrerrortranslations	*getErrorTranslations();
		domnode			*getParameters();

	#include <sqlrelay/private/sqlrerrortranslation.h>
};

class SQLRSERVER_DLLSPEC sqlrerrortranslations {
	public:
		sqlrerrortranslations(sqlrservercontroller *cont);
		~sqlrerrortranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					int64_t errornumber,
					const char *error,
					uint32_t errorsize,
					int64_t *translatederrornumber,
					stringbuffer *translatederror);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrerrortranslations.h>
};

class SQLRSERVER_DLLSPEC sqlrtrigger {
	public:
		sqlrtrigger(sqlrservercontroller *cont,
					sqlrtriggers *ts,
					domnode *parameters);
		virtual	~sqlrtrigger();

		virtual bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					bool before,
					bool *success);

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrtriggers	*getTriggers();
		domnode	*getParameters();

	#include <sqlrelay/private/sqlrtrigger.h>
};

class SQLRSERVER_DLLSPEC sqlrtriggers {
	public:
		sqlrtriggers(sqlrservercontroller *cont);
		~sqlrtriggers();

		bool	load(domnode *parameters);
		void	runBeforeTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		void	runAfterTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						bool *success);

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrtriggers.h>
};

class SQLRSERVER_DLLSPEC sqlrquery {
	public:
		sqlrquery(sqlrservercontroller *cont,
				sqlrqueries *qs,
				domnode *parameters);
		virtual	~sqlrquery();

		virtual bool	match(const char *querystring,
						uint32_t querysize);
		virtual sqlrquerycursor	*newCursor(	
						sqlrserverconnection *sqlrcon,
						uint16_t id);

		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		sqlrqueries	*getQueries();
		domnode	*getParameters();

	#include <sqlrelay/private/sqlrquery.h>
};

class SQLRSERVER_DLLSPEC sqlrquerycursor : public sqlrservercursor {
	public:
		sqlrquerycursor(sqlrserverconnection *conn,
					sqlrquery *q,
					domnode *parameters,
					uint16_t id);
		virtual	~sqlrquerycursor();
		virtual sqlrquerytype_t	determineQueryType(
						const char *query,
						uint32_t size);
		bool	isCustomQuery();

	protected:
		sqlrquery	*getQuery();
		sqlrqueries	*getQueries();
		domnode	*getParameters();

	#include <sqlrelay/private/sqlrquerycursor.h>
};

class SQLRSERVER_DLLSPEC sqlrqueries {
	public:
		sqlrqueries(sqlrservercontroller *cont);
		~sqlrqueries();

		bool		load(domnode *parameters);
		sqlrquerycursor	*match(sqlrserverconnection *sqlrcon,
						const char *querystring,
						uint32_t querysize,
						uint16_t id);

		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrqueries.h>
};

class SQLRSERVER_DLLSPEC sqlrmoduledata {
	public:
		sqlrmoduledata(domnode *parameters);
		virtual	~sqlrmoduledata();

		const char	*getModuleType();
		const char	*getId();

		domnode		*getParameters();

		virtual void	closeResultSet(sqlrservercursor *sqlrcur);
		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	#include <sqlrelay/private/sqlrmoduledata.h>
};

class SQLRSERVER_DLLSPEC sqlrmoduledatas {
	public:
		sqlrmoduledatas(sqlrservercontroller *cont);
		~sqlrmoduledatas();

		bool	load(domnode *parameters);

		sqlrmoduledata	*getModuleData(const char *id);

		void	closeResultSet(sqlrservercursor *sqlrcur);
		void	endTransaction(bool commit);
		void	endSession();

	#include <sqlrelay/private/sqlrmoduledatas.h>
};

class SQLRSERVER_DLLSPEC sqlrmoduledata_tag : public sqlrmoduledata {
	public:
		sqlrmoduledata_tag(domnode *parameters);
		~sqlrmoduledata_tag();
		
		void	addTag(uint16_t cursorid, const char *tag);
		void	addTag(uint16_t cursorid, const char *tag, size_t size);
		avltree<char *>	*getTags(uint16_t cursorid);
		bool	tagExists(uint16_t cursorid, const char *tag);

		void	closeResultSet(sqlrservercursor *sqlrcur);

	#include <sqlrelay/private/sqlrmoduledata_tag.h>
};

#endif

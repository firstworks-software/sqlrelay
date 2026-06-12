// Copyright (c) David Muse
// See the file COPYING for more information.

class SQLRConnection {
	public:
		/** Initiates a connection to "server" on "port"
		 *  or to the unix "socket" on the local machine
		 *  and auths with "user" and "password".
		 *  Failed connections will be retried for 
		 *  "tries" times, waiting "retrytime" seconds
		 *  between each try.  If "tries" is 0 then retries
		 *  will continue forever.  If "retrytime" is 0 then
		 *  retries will be attempted on a default interval.
		 *
		 *  If "server" is a comma-separated list of hosts, then an
		 *  attempt will be made to connect to each until the attempt
		 *  succeeds, or there are no more hosts left to try.
		 *
		 *  If the "socket" parameter is neither 
		 *  NULL nor "" then an attempt will be made to 
		 *  connect through it before attempting to 
		 *  connect to "server" on "port".  If it is 
		 *  NULL or "" then no attempt will be made to 
		 *  connect through the socket. */
		SQLRConnection(var server, var port,
					var socket,
					var user, var password,
					var retrytime, var tries);



		/** Sets the server connect timeout in seconds and
		 *  microseconds.  Setting either parameter to -1 disables the
		 *  timeout.  You can also set this timeout using the
		 *  SQLR_CLIENT_CONNECT_TIMEOUT environment variable. */
		function setConnectTimeout(var timeoutsec, var timeoutusec);

		/** Gets the server connect timeout in seconds. */
		function getConnectTimeoutSeconds();

		/** Gets the server connect timeout in microseconds. */
		function getConnectTimeoutMicroseconds();

		/** Sets the response timeout (for queries, commits, rollbacks,
		 *  pings, etc.) in seconds and microseconds.  Setting either
		 *  parameter to -1 disables the timeout.  You can also set
		 *  this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
		 *  environment variable. */
		function setResponseTimeout(var timeoutsec, var timeoutusec);

		/** Gets the response timeout in seconds. */
		function getResponseTimeoutSeconds();

		/** Gets the response timeout in microseconds. */
		function getResponseTimeoutMicroseconds();



		/** Sets which delimiters are used to identify bind variables
		 *  in countBindVariables() and validateBinds().  Valid
		 *  delimiters include ?,:,@, and $.  Defaults to "?:@$" */
		function setBindVariableDelimiters(var delimiters);

		/** Returns true if question marks (?) are considered to be
		 *  valid bind variable delimiters. */
		function getBindVariableDelimiterQuestionMarkSupported();

		/** Returns true if colons (:) are considered to be
		 *  valid bind variable delimiters. */
		function getBindVariableDelimiterColonSupported();

		/** Returns true if at-signs (@) are considered to be
		 *  valid bind variable delimiters. */
		function getBindVariableDelimiterAtSignSupported();

		/** Returns true if dollar signs ($) are considered to be
		 *  valid bind variable delimiters. */
		function getBindVariableDelimiterDollarSignSupported();



		/** Enables Kerberos authentication and encryption.
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
		function enableKerberos(var service, var mech, var flags);

		/** Enables TLS/SSL encryption, and optionally authentication.
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
		function enableTls(var version,
					var cert, var password, var ciphers,
					var validate, var ca, var depth);

		/** Disables encryption. */
		function disableEncryption();



		/** Ends the session. */
		function endSession();

		/** Disconnects this connection from the current
		 *  session but leaves the session open so 
		 *  that another connection can connect to it 
		 *  using resumeSession(). */
		function suspendSession();

		/** Returns the inet port that the connection is 
		 *  communicating over. This parameter may be 
		 *  passed to another connection for use in
		 *  the resumeSession() method.
		 *  Note: The value this method returns is only
		 *  valid after a call to suspendSession(). */
		function getConnectionPort();

		/** Returns the unix socket that the connection 
		 *  is communicating over. This parameter may be 
		 *  passed to another connection for use in
		 *  the resumeSession() method.
		 *  Note: The value this method returns is only
		 *  valid after a call to suspendSession(). */
		function getConnectionSocket();

		/** Resumes a session previously left open 
		 *  using suspendSession().
		 *  Returns true on success and false on failure. */
		function resumeSession(var port, var socket);



		/** Returns true if the database is up and false
		 *  if it's down. */
		function ping();

		/** Returns the type of database: 
		 *  oracle, postgresql, mysql, etc. */
		function identify();

		/** Returns the version of the database */
		function dbVersion();

		/** Returns the host name of the database */
		function dbHostName();

		/** Returns the ip address of the database */
		function dbIpAddress();

		/** Returns the version of the sqlrelay server software. */
		function serverVersion();

		/** Returns the version of the sqlrelay client software. */
		function clientVersion();

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
		 */
		function bindFormat();

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
		 *  sequences. */
		function nextvalFormat();



		/** Sets the current database to "database".
		 *
		 *  May set the current catalog or schema, depending on
		 *  whether the backend database equates "database" with
		 *  catalog or schema.
		 *
		 *  See getDatabaseIsSchema(). */
		function selectDatabase(var database);

		/** Returns the database that is currently in use.
		 *
		 *  May return the current catalog or schema, depending on
		 *  whether the backend database equates "database" with
		 *  catalog or schema.
		 *
		 *  See getDatabaseIsSchema(). */
		function getCurrentDatabase();

		/** Returns true if the backend database equates
		 *  "database" with "schema", and false if it equates
		 *  "database" with "catalog". */
		function getDatabaseIsSchema();

		/** Sets the current catalog to "catalog" */
		function selectCatalog(var catalog);

		/** Returns the catalog that is currently in use. */
		function getCurrentCatalog();

		/** Sets the current schema to "schema" */
		function selectSchema(var schema);

		/** Returns the schema that is currently in use. */
		function getCurrentSchema();

		/** Returns the user that sqlrelay is currently logged in to
		 *  the database as, or NULL if no user could be determined
		 *  or if an error occurred. */
		function getCurrentUser();



		/** Returns the value of the autoincrement
		 *  column for the last insert */
		function getLastInsertId();



		/** Instructs the database to perform a commit
		 *  after every successful query. */
		function autoCommitOn();

		/** Instructs the database to wait for the
		 *  client to tell it when to commit. */
		function autoCommitOff();

		/** Returns true if auto-commit is currently on,
		 *  false otherwise. */
		function getAutoCommit();


		/** Begins a transaction.  Returns true if the begin
		 *  succeeded, false if it failed.  If the database
		 *  automatically begins a new transaction when a
		 *  commit or rollback is issued then this doesn't
		 *  do anything unless SQL Relay is faking transaction
		 *  blocks. */
		function begin();

		/** Commits a transaction.  Returns true if the commit
		 *  succeeded, false if it failed. */
		function commit();

		/** Rolls back a transaction.  Returns true if the rollback
		 *  succeeded, false if it failed. */
		function rollback();

		/** Returns true if the session is currently inside a
		 *  transaction, false otherwise. */
		function getInTransaction();


		/** Returns the database's native transaction model.  See
		 *  setTranscationModel() for a list of potential return
		 *  values.  Returns NULL if an error occurred. */
		function getDefaultTransactionModel();

		/** Sets the current transaction model to "txmodel" which
		 *  should be one of:
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
		 *  Returns true on success and false on failure. */
		function setTransactionModel(var txmodel);

		/** Returns the current transaction model.  See
		 *  setTranscationModel() for a list of potential return
		 *  values.  Returns NULL if an error occurred. */
		function getTransactionModel();

		/** Returns the database-specific default isolation level,
		 *  or NULL if an error occurred. */
		function getDefaultIsolationLevel();

		/** Sets the transaction isolation level to "isolationlevel".
		 *  The string is the database-specific (native) name and is
		 *  matched case-insensitively.
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
		 *  Returns true if setting the isolation level succeeded,
		 *  false if it failed. */
		function setIsolationLevel(var isolationlevel);

		/** Returns the database-specific isolation level, "unknown"
		 *  if the isolation level is unknown, or NULL if an error
		 *  occurred. */
		function getIsolationLevel();

		/** Returns the value of the specified database "feature".
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
		 *   * list - SELECT_EXPLICIT,ROW_COUNT_EXPLICIT,SELECT_PROC,...
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
		function getDatabaseFeature(feature);


		/** If an operation failed and generated an
		 *  error, the error message is available here.
		 *  If there is no error then this method 
		 *  returns NULL. */
		function errorMessage();

		/** If an operation failed and generated an
		 *  error, the error number is available here.
		 *  If there is no error then this method 
		 *  returns 0. */
		function errorNumber();



		/** Causes verbose debugging information to be 
		 *  sent to standard output.  Another way to do
		 *  this is to start a query with "-- debug\n".
		 *  Yet another way is to set the environment
		 *  variable SQLR_CLIENT_DEBUG to "ON" */
		function debugOn();

		/** Turns debugging off. */
		function debugOff();

		/** Returns false if debugging is off and true
		 *  if debugging is on. */
		function getDebug();



		/** Allows you to specify a file to write debug to.
		 *  Setting "filename" to NULL or an empty string causes debug
		 *  to be written to standard output (the default). */
		function setDebugFile(var filename);


		/** Allows you to set a string that will be passed to the
		 *  server and ultimately included in server-side logging
		 *  along with queries that were run by this instance of
		 *  the client. */
		function setClientInfo(var clientinfo);

		/** Returns the string that was set by setClientInfo(). */
		function getClientInfo();
};


class SQLRCursor {
	public:
		/** Creates a cursor to run queries and fetch result
		 *  sets using connecton "sqlrc". */
		SQLRCursor(var sqlrc);


		/** Sets the number of rows of the result set
		 *  to buffer at a time.  0 (the default)
		 *  means buffer the entire result set. */
		function setResultSetBufferSize(var rows);

		/** Returns the number of result set rows that 
		 *  will be buffered at a time or 0 for the
		 *  entire result set. */
		function getResultSetBufferSize();



		/** Tells the server not to send any column
		 *  info (names, types, sizes).  If you don't
		 *  need that info, you should call this
		 *  method to improve performance. */
		function dontGetColumnInfo();

		/** Tells the server to send column info. */
		function getColumnInfo();


		/** Columns names are returned in the same
		 *  case as they are defined in the database.
		 *  This is the default. */
		function mixedCaseColumnNames();

		/** Columns names are converted to upper case. */
		function upperCaseColumnNames();

		/** Columns names are converted to lower case. */
		function lowerCaseColumnNames();



		/** Sets query caching on.  Future queries
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
		function cacheToFile(var filename);

		/** Sets the time-to-live for cached result
		 *  sets. The sqlr-cachemanger will remove each 
		 *  cached result set "ttl" seconds after it's 
		 *  created, provided it's scanning the directory
		 *  containing the cache files. */
		function setCacheTtl(var ttl);

		/** Returns the name of the file containing the
		 *  cached result set. */
		function getCacheFileName();

		/** Sets query caching off. */
		function cacheOff();



		/** Generates a result set containing databases that match the
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
		function getDatabaseList(var wild);

		/** Generates a result set containing catalogs that match the
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
		function getCatalogList(var wild);

		/** Generates a result set containing schemas that match the
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
		function getSchemaList(var schemas);

		/** Generates a result set containing supported table types.
		 *
		 *  The result set will contain the following columns:
		 *  * table_type
		 *
		 *  If SQL Relay doesn't support getting a list of table types
		 *  for the current database backend (or the database doesn't)
		 *  then an empty result set will be returned. */
		function getTableTypeList();

		/** Generates a result set containing the tables in the current
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
		function getTableList(var wild);

		/** Generates a result set containing data type information for
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
		function getTypeInfoList(var type);

		/** Generates a result set containing the columns of "table",
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
		function getColumnList(var table, var wild);

		/** Generates a result set containing the primary keys of
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
		function getPrimaryKeysList(var table, var columns);

		/** Generates a result set containing the keys and indexes of
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
		function getKeyAndIndexList(var table, var qualifier);

		/** Generates a result set containing procedures that match the
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
		function getProcedureList(var procedures);

		/** Generates a result set containing the parameters of
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
		function getProcedureParameterList(var procedure, var parameters);



		/** Sends "query" directly and gets a result set. */
		function sendQuery(var query);

		/** Sends "query" with length "length" directly
		 *  and gets a result set. This method must be used
		 *  if the query contains binary data. */
		function sendQuery(var query, var length);

		/** Sends the query in file "path"/"filename" directly
		 *  and gets a result set. */
		function sendFileQuery(var path, var filename); 



		/** Prepare to execute "query". */
		function prepareQuery(var query);

		/** Prepare to execute "query" with length 
		 *  "length".  This method must be used if the
		 *  query contains binary data. */
		function prepareQuery(var query, var length);

		/** Prepare to execute the contents 
		 *  of "path"/"filename".  Returns false if the
		 * // file couldn't be opened. */
		function prepareFileQuery(var path,
						var filename);



		/** Defines a string substitution variable. */
		function substitution(var variable, var value);

		/** Defines an integer substitution variable. */
		function substitution(var variable, var value);

		/** Defines a decimal substitution variable. */
		function substitution(var variable, var value, 
							var precision, 
							var scale);

		/** Defines an array of string substitution variables. */
		function substitutions(var variables,
						var values);

		/** Defines an array of integer substitution variables. */
		function substitutions(var variables,
						var values);

		/** Defines an array of decimal substitution variables. */
		function substitutions(var variables,
						var values,
						var precisions, 
						var scales);



		/** Defines a string input bind variable. */
		function inputBind(var variable, var value);

		/** Defines a string input bind variable. */
		function inputBind(var variable, var value, var valuelength);

		/** Defines a integer input bind variable. */
		function inputBind(var variable, var value);

		/** Defines a decimal input bind variable.
		  * (If you don't have the precision and scale then set
		  * them both 0.  However in that case you may get
		  * unexpected rounding behavior if the server is faking
		  * binds.) */
		function inputBind(var variable, var value, 
							var precision, 
							var scale);

		/** Defines a date input bind variable.  "day" and "month"
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
		 *  time intervals and ignore "isnegative".
		 *  */
		function inputBind(var variable,
				var year, var month, var day,
				var hour, var minute, var second,
				var microsecond, var tz,
				var isnegative);

		/** Defines a binary lob input bind variable. */
		function inputBindBlob(var variable,
						var value,
						var size);

		/** Defines a character lob input bind variable. */
		function inputBindClob(var variable,
						var value,
						var size);

		/** Defines an array of string input bind variables. */
		function inputBinds(var variables, var values);

		/** Defines an array of integer input bind variables. */
		function inputBinds(var variables,
					const var values);

		/** Defines an array of decimal input bind variables. */
		function inputBinds(var variables,
					const var values, 
					const var precisions, 
					const var scales);



		/** Defines an output bind variable.
		 *  "bufferlength" bytes will be reserved
		 *  to store the value. */
		function defineOutputBindString(var variable,
						var bufferlength);

		/** Defines an integer output bind variable. */
		function defineOutputBindInteger(var variable);

		/** Defines a decimal output bind variable. */
		function defineOutputBindDouble(var variable);

		/** Defines a date output bind variable. */
		function defineOutputBindDate(var variable);

		/** Defines a binary lob output bind variable. */
		function defineOutputBindBlob(var variable);

		/** Defines a character lob output bind variable. */
		function defineOutputBindClob(var variable);

		/** Defines a cursor output bind variable. */
		function defineOutputBindCursor(var variable);



		/** Clears all bind variables. */
		function clearBinds();

		/** Parses the previously prepared query,
		 *  counts the number of bind variables defined
		 *  in it and returns that number. */
		function countBindVariables();

		/** If you are binding to any variables that 
		 *  might not actually be in your query, call 
		 *  this to ensure that the database won't try 
		 *  to bind them unless they really are in the 
		 *  query.  There is a performance penalty for
		 *  calling this method. */
		function validateBinds();

		/** Returns true if "variable" was a valid
		 *  bind variable of the query. */
		function validBind(var variable);



		/** Execute the query that was previously 
		 *  prepared and bound. */
		function executeQuery();

		/** Fetch from a cursor that was returned as
		 *  an output bind variable. */
		function fetchFromBindCursor();



		/** Get the value stored in a previously
		 *  defined string output bind variable. */
		function getOutputBindString(var variable);

		/** Get the value stored in a previously
		 *  defined integer output bind variable. */
		function getOutputBindInteger(var variable);

		/** Get the value stored in a previously
		 *  defined decimal output bind variable. */
		function getOutputBindDouble(var variable);

		/** Get the value stored in a previously
		 *  defined date output bind variable. */
		function getOutputBindDate(var variable,
							var year,
							var month,
							var day,
							var hour,
							var minute,
							var second,
							var microsecond,
							var tz);

		/** Get the value stored in a previously
		 *  defined binary lob output bind variable. */
		function getOutputBindBlob(var variable);

		/** Get the value stored in a previously
		 *  defined character lob output bind variable. */
		function getOutputBindClob(var variable);

		/** Get the length of the value stored in a
		 *  previously defined output bind variable. */
		function getOutputBindLength(var variable);

		/** Get the cursor associated with a previously
		 *  defined output bind variable. */
		function getOutputBindCursor(var variable);

		/** Get the year from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateYear(var variable);

		/** Get the month from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateMonth(var variable);

		/** Get the day from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateDay(var variable);

		/** Get the hour from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateHour(var variable);

		/** Get the minute from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateMinute(var variable);

		/** Get the second from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateSecond(var variable);

		/** Get the microsecond from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateMicrosecond(var variable);

		/** Get the time zone from a previously defined
		 *  date output bind variable. */
		function getOutputBindDateTz(var variable);

		/** Get whether the value is negative from a
		 *  previously defined date output bind variable. */
		function getOutputBindDateIsNegative(var variable);



		/** Opens a cached result set.
		 *  Returns true on success and false on failure. */
		function openCachedResultSet(var filename);



		/** Returns the number of columns in the current
		 *  result set. */
		function colCount();

		/** Returns the number of rows in the current 
		 *  result set (if the result set is being
		 *  stepped through, this returns the number
		 *  of rows processed so far). */
		function rowCount();

		/** Returns the total number of rows that will 
		 *  be returned in the result set.  Not all 
		 *  databases support this call.  Don't use it 
		 *  for applications which are designed to be 
		 *  portable across databases.  0 is returned
		 *  by databases which don't support this option. */
		function totalRows();

		/** Returns the number of rows that were 
		 *  updated, inserted or deleted by the query.
		 *  Not all databases support this call.  Don't 
		 *  use it for applications which are designed 
		 *  to be portable across databases.  0 is 
		 *  returned by databases which don't support 
		 *  this option. */
		function affectedRows();

		/** Returns the index of the first buffered row.
		 *  This is useful when buffering only part of
		 *  the result set at a time. */
		function firstRowIndex();

		/** Returns false if part of the result set is
		 *  still pending on the server and true if not.
		 *  This method can only return false if 
		 *  setResultSetBufferSize() has been called
		 *  with a parameter other than 0. */
		function endOfResultSet();

		/** Returns true and acts like executeQuery()
		 *  when there is another result set available
		 *  from the server. */
		function nextResultSet();



		/** If a query failed and generated an error,
		 *  the error message is available here.  If 
		 *  the query succeeded then this method 
		 *  returns NULL. */
		function errorMessage();

		/** If a query failed and generated an
		 *  error, the error number is available here.
		 *  If there is no error then this method 
		 *  returns 0. */
		function errorNumber();



		/** Tells the connection to return NULL fields
		 *  and output bind variables as empty strings. 
		 *  This is the default. */
		function getNullsAsEmptyStrings();

		/** Tells the connection to return NULL fields
		 *  and output bind variables as NULL's rather
		 *  than as empty strings. */
		function getNullsAsNulls();



		/** Returns the specified field as a string. */
		function getField(var row, var col);

		/** Returns the specified field as a string */
		function getField(var row, var col);

		/** Returns the specified field as a string,
		 *  ignoring the case of "col". */
		function getFieldIgnoringCase(var row, var col);

		/** Returns the specified field as an integer. */
		function getFieldAsInteger(var row, var col);

		/** Returns the specified field as an integer. */
		function getFieldAsInteger(var row, var col);

		/** Returns the specified field as a decimal. */
		function getFieldAsDouble(var row, var col);

		/** Returns the specified field as a decimal. */
		function getFieldAsDouble(var row, var col);

		/** Returns the specified field as a boolean. */
		function getFieldAsBoolean(var row, var col);

		/** Returns the specified field as a boolean. */
		function getFieldAsBoolean(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the year component. */
		function getFieldAsDateYear(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the year component.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateYear(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the month component. */
		function getFieldAsDateMonth(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the month component.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateMonth(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the day component. */
		function getFieldAsDateDay(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the day component.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateDay(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the hour component. */
		function getFieldAsDateHour(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the hour component.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateHour(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the minute component. */
		function getFieldAsDateMinute(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the minute component.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateMinute(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the second component. */
		function getFieldAsDateSecond(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the second component.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateSecond(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the microsecond component. */
		function getFieldAsDateMicrosecond(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the microsecond component.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateMicrosecond(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns whether the hour component
		 *  is negative. */
		function getFieldAsDateIsNegative(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns whether the hour component
		 *  is negative.
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateIsNegative(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Returns the specified field as an integer,
		 *  ignoring case of "col". */
		function getFieldAsIntegerIgnoringCase(var row, var col);

		/** Returns the specified field as a decimal,
		 *  ignoring case of "col". */
		function getFieldAsDoubleIgnoringCase(var row, var col);

		/** Returns the specified field as a boolean,
		 *  ignoring case of "col". */
		function getFieldAsBooleanIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the year component,
		 *  ignoring case of "col". */
		function getFieldAsDateYearIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the year component,
		 *  ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateYearIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the month component,
		 *  ignoring case of "col". */
		function getFieldAsDateMonthIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the month component,
		 *  ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateMonthIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the day component,
		 *  ignoring case of "col". */
		function getFieldAsDateDayIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the day component,
		 *  ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateDayIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the hour component,
		 *  ignoring case of "col". */
		function getFieldAsDateHourIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the hour component,
		 *  ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateHourIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the minute component,
		 *  ignoring case of "col". */
		function getFieldAsDateMinuteIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the minute component,
		 *  ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateMinuteIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the second component,
		 *  ignoring case of "col". */
		function getFieldAsDateSecondIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the second component,
		 *  ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateSecondIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the microsecond component,
		 *  ignoring case of "col". */
		function getFieldAsDateMicrosecondIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns the microsecond component,
		 *  ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateMicrosecondIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns whether the hour component
		 *  is negative, ignoring case of "col". */
		function getFieldAsDateIsNegativeIgnoringCase(var row, var col);

		/** Interprets the specified field as a date
		 *  and returns whether the hour component
		 *  is negative, ignoring case of "col".
		 *
		 *  If "ddmm" is set true then the date format
		 *  is assumed to be dd/mm/yyyy rather than
		 *  mm/dd/yyyy when a date with a trailing year
		 *  is encountered.
		 *
		 *  If "yyyyddmm" is set true then the date
		 *  format is assumed to be yyyy/dd/mm rather
		 *  than yyyy/mm/dd when a date with a leading
		 *  year is encountered.
		 *
		 *  "datedelimiters" may be set to a set of
		 *  valid date delimiters and may contain any
		 *  combination of '/', '-', '.', and ':'.
		 *  Eg. "/-" would mean that only '/' and '-'
		 *  are valid date delimiters.  If left NULL
		 *  then it defaults to "/-.:". */
		function getFieldAsDateIsNegativeIgnoringCase(var row, var col,
					var ddmm, var yyyyddmm,
					var datedelimiters);



		/** Returns the length of the specified field. */
		function getFieldLength(var row, var col);

		/** Returns the length of the specified field. */
		function getFieldLength(var row, var col);



		/** Returns a null terminated array of the 
		 *  values of the fields in the specified row. */
		function  getRow(var row);

		/** Returns a null terminated array of the 
		 *  lengths of the fields in the specified row. */
		function getRowLengths(var row);

		/** Returns a null terminated array of the 
		 *  column names of the current result set. */
		function  getColumnNames();

		/** Returns the name of the specified column. */
		function getColumnName(var col);

		/** Returns the type of the specified column. */
		function getColumnType(var col);

		/** Returns the type of the specified column. */
		function getColumnType(var col);

		/** Returns the number of bytes required on
		 *  the server to store the data for the specified column */
		function getColumnLength(var col);

		/** Returns the number of bytes required on
		 *  the server to store the data for the specified column */
		function getColumnLength(var col);

		/** Returns the precision of the specified
		 *  column.
		 *  Precision is the total number of digits in
		 *  a number.  eg: 123.45 has a precision of 5.
		 *  For non-numeric types, it's the number of
		 *  characters in the string. */
		function getColumnPrecision(var col);

		/** Returns the precision of the specified
		 *  column.
		 *  Precision is the total number of digits in
		 *  a number.  eg: 123.45 has a precision of 5.
		 *  For non-numeric types, it's the number of
		 *  characters in the string. */
		function getColumnPrecision(var col);

		/** Returns the scale of the specified column.
		 *  Scale is the total number of digits to the
		 *  right of the decimal point in a number.
		 *  eg: 123.45 has a scale of 2. */
		function getColumnScale(var col);

		/** Returns the scale of the specified column.
		 *  Scale is the total number of digits to the
		 *  right of the decimal point in a number.
		 *  eg: 123.45 has a scale of 2. */
		function getColumnScale(var col);

		/** Returns true if the specified column can
		 *  contain nulls and false otherwise. */
		function getColumnIsNullable(var col);

		/** Returns true if the specified column can
		 *  contain nulls and false otherwise. */
		function getColumnIsNullable(var col);

		/** Returns true if the specified column is a
		 *  primary key and false otherwise. */
		function getColumnIsPrimaryKey(var col);

		/** Returns true if the specified column is a
		 *  primary key and false otherwise. */
		function getColumnIsPrimaryKey(var col);

		/** Returns true if the specified column is
		 *  unique and false otherwise. */
		function getColumnIsUnique(var col);

		/** Returns true if the specified column is
		 *  unique and false otherwise. */
		function getColumnIsUnique(var col);

		/** Returns true if the specified column is
		 *  part of a composite key and false otherwise. */
		function getColumnIsPartOfKey(var col);

		/** Returns true if the specified column is
		 *  part of a composite key and false otherwise. */
		function getColumnIsPartOfKey(var col);

		/** Returns true if the specified column is
		 *  an unsigned number and false otherwise. */
		function getColumnIsUnsigned(var col);

		/** Returns true if the specified column is
		 *  an unsigned number and false otherwise. */
		function getColumnIsUnsigned(var col);

		/** Returns true if the specified column was
		 *  created with the zero-fill flag and false
		 *  otherwise. */
		function getColumnIsZeroFilled(var col);

		/** Returns true if the specified column was
		 *  created with the zero-fill flag and false
		 *  otherwise. */
		function getColumnIsZeroFilled(var col);

		/** Returns true if the specified column
		 *  contains binary data and false
		 *  otherwise. */
		function getColumnIsBinary(var col);

		/** Returns true if the specified column
		 *  contains binary data and false
		 *  otherwise. */
		function getColumnIsBinary(var col);

		/** Returns true if the specified column
		 *  auto-increments and false otherwise. */
		function getColumnIsAutoIncrement(var col);

		/** Returns true if the specified column
		 *  auto-increments and false otherwise. */
		function getColumnIsAutoIncrement(var col);

		/** Returns the length of the longest field
		 *  in the specified column. */
		function getLongest(var col);

		/** Returns the length of the longest field
		 *  in the specified column. */
		function getLongest(var col);



		/** Tells the server to leave this result
		 *  set open when the connection calls 
		 *  suspendSession() so that another connection 
		 *  can connect to it using resumeResultSet() 
		 *  after it calls resumeSession(). */
		function suspendResultSet();

		/** Returns the internal ID of this result set.
		 *  This parameter may be passed to another 
		 *  cursor for use in the resumeResultSet() 
		 *  method.
		 *  Note: The value this method returns is only
		 *  valid after a call to suspendResultSet(). */
		function getResultSetId();

		/** Resumes a result set previously left open 
		 *  using suspendSession().
		 *  Returns true on success and false on failure. */
		function resumeResultSet(var id);

		/** Resumes a result set previously left open
		 *  using suspendSession() and continues caching
		 *  the result set to "filename".
		 *  Returns true on success and false on failure. */
		function resumeCachedResultSet(var id,
						var filename);

		/** Closes the current result set, if one is open.  Data
		 *  that has been fetched already is still available but
		 *  no more data may be fetched.  Server side resources
		 *  for the result set are freed as well. */
		function closeResultSet();
};

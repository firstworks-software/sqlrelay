// Copyright (c) David Muse
// See the file COPYING for more information.

#ifndef SQLRCLIENT_H
#define SQLRCLIENT_H

#include <sqlrelay/private/sqlrclientincludes.h>

class SQLRCLIENT_DLLSPEC sqlrconnection : public object {
	public:
		/** Initiates a connection to "server" on "port" or to the
		 *  unix "socket" on the local machine and auths with "user"
		 *  and "password".  Failed connections will be retried for
		 *  "tries" times, waiting "retrytime" seconds between each
		 *  try.  If "tries" is 0 then retries will continue forever.
		 *  If "retrytime" is 0 then retries will be attempted on a
		 *  default interval.
		 *
		 *  If "server" is a comma-separated list of hosts, then an
		 *  attempt will be made to connect to each until the attempt
		 *  succeeds, or there are no more hosts left to try.
		 *
		 *  If the "socket" parameter is neither NULL nor "" then an
		 *  attempt will be made to connect through it before
		 *  attempting to connect to "server" on "port".  If it is NULL
		 *  or "" then no attempt will be made to connect through the
		 *  socket. */
		sqlrconnection(const char *server, uint16_t port,
				const char *socket,
				const char *user, const char *password,
				int32_t retrytime, int32_t tries);


		/** Disconnects and ends the session if it hasn't been ended
		 *  already. */
		~sqlrconnection();



		/** Sets the server connect timeout in seconds and
		 *  microseconds.  Setting either parameter to -1 disables the
		 *  timeout.  You can also set this timeout using the
		 *  SQLR_CLIENT_CONNECT_TIMEOUT environment variable. */
		void	setConnectTimeout(int32_t timeoutsec,
						int32_t timeoutusec);

		/** Gets the server connect timeout in seconds and
		 *  microseconds. */
		void	getConnectTimeout(int32_t *timeoutsec,
						int32_t *timeoutusec);

		/** Gets the server connect timeout in seconds. */
		int32_t	getConnectTimeoutSeconds();

		/** Gets the server connect timeout in microseconds. */
		int32_t	getConnectTimeoutMicroseconds();

		/** Sets the response timeout (for queries, commits, rollbacks,
		 *  pings, etc.) in seconds and microseconds.  Setting either
		 *  parameter to -1 disables the timeout.  You can also set
		 *  this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
		 *  environment variable. */
		void	setResponseTimeout(int32_t timeoutsec,
						int32_t timeoutusec);

		/** Gets the response timeout in seconds and
		 *  microseconds. */
		void	getResponseTimeout(int32_t *timeoutsec,
						int32_t *timeoutusec);

		/** Gets the response timeout in seconds. */
		int32_t	getResponseTimeoutSeconds();

		/** Gets the response timeout in microseconds. */
		int32_t	getResponseTimeoutMicroseconds();



		/** Sets which delimiters are used to identify bind variables
		 *  in countBindVariables() and validateBinds().  Valid
		 *  delimiters include ?,:,@, and $.  Defaults to "?:@$" */
		void	setBindVariableDelimiters(const char *delimiters);

		/** Returns true if question marks (?) are considered to be
		 *  valid bind variable delimiters. */
		bool	getBindVariableDelimiterQuestionMarkSupported();

		/** Returns true if colons (:) are considered to be
		 *  valid bind variable delimiters. */
		bool	getBindVariableDelimiterColonSupported();

		/** Returns true if at-signs (@) are considered to be
		 *  valid bind variable delimiters. */
		bool	getBindVariableDelimiterAtSignSupported();

		/** Returns true if dollar signs ($) are considered to be
		 *  valid bind variable delimiters. */
		bool	getBindVariableDelimiterDollarSignSupported();



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
		void	enableKerberos(const char *service,
					const char *mech,
					const char *flags);

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
		void	enableTls(const char *version,
					const char *cert,
					const char *password,
					const char *ciphers,
					const char *validate,
					const char *ca,
					uint16_t depth);

		/** Disables encryption. */
		void	disableEncryption();



		/** Ends the session. */
		void	endSession();

		/** Disconnects this connection from the current
		 *  session but leaves the session open so 
		 *  that another connection can connect to it 
		 *  using resumeSession(). */
		bool	suspendSession();

		/** Returns the inet port that the connection is 
		 *  communicating over. This parameter may be 
		 *  passed to another connection for use in
		 *  the resumeSession() method.
		 *  Note: The value this method returns is only
		 *  valid after a call to suspendSession(). */
		uint16_t	getConnectionPort();

		/** Returns the unix socket that the connection 
		 *  is communicating over. This parameter may be 
		 *  passed to another connection for use in
		 *  the resumeSession() method.
		 *  Note: The value this method returns is only
		 *  valid after a call to suspendSession(). */
		const char	*getConnectionSocket();

		/** Resumes a session previously left open 
		 *  using suspendSession().
		 *  Returns true on success and false on failure. */
		bool	resumeSession(uint16_t port, const char *socket);



		/** Returns true if the database is up and false
		 *  if it's down. */
		bool		ping();

		/** Returns the type of database: 
		 *  oracle, postgresql, mysql, etc. */
		const char	*identify();

		/** Returns the version of the database */
		const char	*dbVersion();

		/** Returns the host name of the database */
		const char	*dbHostName();

		/** Returns the ip address of the database */
		const char	*dbIpAddress();

		/** Returns the version of the sqlrelay server software. */
		const char	*serverVersion();

		/** Returns the version of the sqlrelay client software. */
		const char	*clientVersion();

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
		const char	*bindFormat();

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
		const char	*nextvalFormat();



		/** Sets the current database (catalog) to "database" */
		bool	selectDatabase(const char *database);

		/** Returns the database (catalog) that is currently in use. */
		const char	*getCurrentDatabase();

		/** Sets the current schema to "schema" */
		bool	selectSchema(const char *schema);

		/** Returns the schema that is currently in use. */
		const char	*getCurrentSchema();



		/** Returns the value of the autoincrement
		 *  column for the last insert */
		uint64_t	getLastInsertId();



		/** Instructs the database to perform a commit
		 *  after every successful query. */
		bool	autoCommitOn();

		/** Instructs the database to wait for the 
		 *  client to tell it when to commit. */
		bool	autoCommitOff();

		/** Returns true if auto-commit is currently on,
		 *  false otherwise. */
		bool	getAutoCommit();



		/** Begins a transaction.  Returns true if the begin
		 *  succeeded, false if it failed.  If the database
		 *  automatically begins a new transaction when a
		 *  commit or rollback is issued then this doesn't
		 *  do anything unless SQL Relay is faking transaction
		 *  blocks. */
		bool	begin();

		/** Commits a transaction.  Returns true if the commit
		 *  succeeded, false if it failed. */
		bool	commit();

		/** Rolls back a transaction.  Returns true if the rollback
		 *  succeeded, false if it failed. */
		bool	rollback();



		/** Sets the isolation level to "isolationlevel", the
		 *  database-secific isolation level.  Returns true if setting
		 *  the isolation level succeeded, false if it failed. */
		bool	setIsolationLevel(const char *isolationlevel);

		/** Returns the database-specific isolation level, "unknown"
		 *  if the isolation level is unknown, or NULL if an error
		 *  occurred. */
		const char	*getIsolationLevel();



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
		 *  * default_isolation_level
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
		 *   * list - LOWER,MIXED,UPPER
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
		 *  * mixed_case_identifiers
		 *   * list - IDENTIFIERS,QUOTED_IDENTIFIERS
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
		 *   * list - LOWER,MIXED,UPPER
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
		const char	*getDatabaseFeature(const char *feature);



		/** Allows you to set a string that will be passed to the
		 *  server and ultimately included in server-side logging
		 *  along with queries that were run by this instance of
		 *  the client. */
		void		setClientInfo(const char *clientinfo);

		/** Returns the string that was set by setClientInfo(). */
		const char	*getClientInfo();



		/** If an operation failed and generated an
		 *  error, the error message is available here.
		 *  If there is no error then this method 
		 *  returns NULL. */
		const char	*errorMessage();

		/** If an operation failed and generated an
		 *  error, the error number is available here.
		 *  If there is no error then this method 
		 *  returns 0. */
		int64_t		errorNumber();



		/** Causes verbose debugging information to be 
		 *  sent to standard output.  Another way to do
		 *  this is to start a query with "-- debug\n".
		 *  Yet another way is to set the environment
		 *  variable SQLR_CLIENT_DEBUG to "ON" */
		void	debugOn();

		/** Turns debugging off. */
		void	debugOff();

		/** Returns false if debugging is off and true
		 *  if debugging is on. */
		bool	getDebug();

		/** Allows you to replace the function used
		 *  to print debug messages with your own
		 *  function.  The function is expected to take
		 *  arguments like printf(). */
		void	debugPrintFunction(int (*printfunction)
							(const char *,...));

		/** Allows you to specify a file to write debug to.
		 *  Setting "filename" to NULL or an empty string causes debug
		 *  to be written to standard output (the default). */
		void	setDebugFile(const char *filename);

	#include <sqlrelay/private/sqlrconnection.h>
};


class SQLRCLIENT_DLLSPEC sqlrcursor : public object {
	public:
			/** Creates a cursor to run queries and fetch result
			 *  sets using connecton "sqlrc". */
			sqlrcursor(sqlrconnection *sqlrc);

			/** Destroys the cursor and cleans up all associated
			 *  result set data. */
			~sqlrcursor();



		/** Sets the number of rows of the result set
		 *  to buffer at a time.  0 (the default)
		 *  means buffer the entire result set. */
		void	setResultSetBufferSize(uint64_t rows);

		/** Returns the number of result set rows that 
		 *  will be buffered at a time or 0 for the
		 *  entire result set. */
		uint64_t	getResultSetBufferSize();



		/** Tells the server not to send any column
		 *  info (names, types, sizes).  If you don't
		 *  need that info, you should call this
		 *  method to improve performance. */
		void	dontGetColumnInfo();

		/** Tells the server to send column info. */
		void	getColumnInfo();


		/** Columns names are returned in the same
		 *  case as they are defined in the database.
		 *  This is the default. */
		void	mixedCaseColumnNames();

		/** Columns names are converted to upper case. */
		void	upperCaseColumnNames();

		/** Columns names are converted to lower case. */
		void	lowerCaseColumnNames();



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
		void	cacheToFile(const char *filename);

		/** Sets the time-to-live for cached result
		 *  sets. The sqlr-cachemanger will remove each 
		 *  cached result set "ttl" seconds after it's 
		 *  created, provided it's scanning the directory
		 *  containing the cache files. */
		void	setCacheTtl(uint32_t ttl);

		/** Returns the name of the file containing the
		 *  cached result set. */
		const char	*getCacheFileName();

		/** Sets query caching off. */
		void	cacheOff();



		/** Sends a query that returns a list of
		 *  databases/schemas matching "wild".  If wild is empty
		 *  or NULL then a list of all databases/schemas will be
		 *  returned. */
		bool	getDatabaseList(const char *wild);

		/** Sends a query that returns a list of tables
		 *  matching "wild".  If wild is empty or NULL then
		 *  a list of all tables will be returned. */
		bool	getTableList(const char *wild);

		/** Sends a query that returns a list of columns
		 *  in the table specified by the "table" parameter
		 *  matching "wild".  If wild is empty or NULL then
		 *  a list of all columns will be returned. */
		bool	getColumnList(const char *table, const char *wild);



		/** Sends "query" directly and gets a result set. */
		bool	sendQuery(const char *query);

		/** Sends "query" with length "length" directly
 		 *  and gets a result set. This method must be used
 		 *  if the query contains binary data. */
		bool	sendQuery(const char *query, uint32_t length);

		/** Sends the query in file "path"/"filename" directly
		 *  and gets a result set. */
		bool	sendFileQuery(const char *path, const char *filename); 



		/** Prepare to execute "query". */
		void	prepareQuery(const char *query);

		/** Prepare to execute "query" with length 
		 *  "length".  This method must be used if the
		 *  query contains binary data. */
		void	prepareQuery(const char *query, uint32_t length);

		/** Prepare to execute the contents 
		 *  of "path"/"filename".  Returns false if the
		 * // file couldn't be opened. */
		bool	prepareFileQuery(const char *path,
						const char *filename);



		/** Defines a string substitution variable. */
		void	substitution(const char *variable, const char *value);

		/** Defines an integer substitution variable. */
		void	substitution(const char *variable, int64_t value);

		/** Defines a decimal substitution variable. */
		void	substitution(const char *variable, double value, 
							uint32_t precision, 
							uint32_t scale);

		/** Defines an array of string substitution variables. */
		void	substitutions(const char **variables,
						const char **values);

		/** Defines an array of integer substitution variables. */
		void	substitutions(const char **variables,
						const int64_t *values);

		/** Defines an array of decimal substitution variables. */
		void	substitutions(const char **variables,
					const double *values,
					const uint32_t *precisions, 
					const uint32_t *scales);



		/** Defines a string input bind variable. */
		void	inputBind(const char *variable, const char *value);

		/** Defines a string input bind variable. */
		void	inputBind(const char *variable, const char *value,
							uint32_t valuelength);

		/** Defines a integer input bind variable. */
		void	inputBind(const char *variable, int64_t value);

		/** Defines a decimal input bind variable.
		  * (If you don't have the precision and scale then set
		  * them both 0.  However in that case you may get
		  * unexpected rounding behavior if the server is faking
		  * binds.) */
		void	inputBind(const char *variable, double value, 
							uint32_t precision, 
							uint32_t scale);

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
		void	inputBind(const char *variable,
				int16_t year, int16_t month, int16_t day,
				int16_t hour, int16_t minute, int16_t second,
				int32_t microsecond, const char *tz,
				bool isnegative);

		/** Defines a binary lob input bind variable. */
		void	inputBindBlob(const char *variable,
						const char *value,
						uint32_t size);

		/** Defines a character lob input bind variable. */
		void	inputBindClob(const char *variable,
						const char *value,
						uint32_t size);

		/** Defines an array of string input bind variables. */
		void	inputBinds(const char **variables, const char **values);

		/** Defines an array of integer input bind variables. */
		void	inputBinds(const char **variables,
					const int64_t *values);

		/** Defines an array of decimal input bind variables. */
		void	inputBinds(const char **variables,
					const double *values, 
					const uint32_t *precisions, 
					const uint32_t *scales);



		/** Defines an output bind variable.
		 *  "bufferlength" bytes will be reserved
		 *  to store the value. */
		void	defineOutputBindString(const char *variable,
						uint32_t bufferlength);

		/** Defines an integer output bind variable. */
		void	defineOutputBindInteger(const char *variable);

		/** Defines a decimal output bind variable. */
		void	defineOutputBindDouble(const char *variable);

		/** Defines a date output bind variable. */
		void	defineOutputBindDate(const char *variable);

		/** Defines a binary lob output bind variable. */
		void	defineOutputBindBlob(const char *variable);

		/** Defines a character lob output bind variable. */
		void	defineOutputBindClob(const char *variable);

		/** Defines a cursor output bind variable. */
		void	defineOutputBindCursor(const char *variable);



		/** Clears all bind variables. */
		void	clearBinds();

		/** Parses the previously prepared query,
		 *  counts the number of bind variables defined
		 *  in it and returns that number. */
		uint16_t	countBindVariables();

		/** If you are binding to any variables that 
		 *  might not actually be in your query, call 
		 *  this to ensure that the database won't try 
		 *  to bind them unless they really are in the 
		 *  query.  There is a performance penalty for
		 *  calling this method. */
		void	validateBinds();

		/** Returns true if "variable" was a valid
		 *  bind variable of the query. */
		bool	validBind(const char *variable);



		/** Execute the query that was previously 
		 *  prepared and bound. */
		bool	executeQuery();

		/** Fetch from a cursor that was returned as
		 *  an output bind variable. */
		bool	fetchFromBindCursor();



		/** Get the value stored in a previously
		 *  defined string output bind variable. */
		const char	*getOutputBindString(const char *variable);

		/** Get the value stored in a previously
		 *  defined integer output bind variable. */
		int64_t		getOutputBindInteger(const char *variable);

		/** Get the value stored in a previously
		 *  defined decimal output bind variable. */
		double		getOutputBindDouble(const char *variable);

		/** Get the value stored in a previously
		 *  defined date output bind variable. */
		bool		getOutputBindDate(const char *variable,
							int16_t *year,
							int16_t *month,
							int16_t *day,
							int16_t *hour,
							int16_t *minute,
							int16_t *second,
							int32_t *microsecond,
							const char **tz,
							bool *isnegative);

		/** Get the year from a previously
		 *  defined date output bind variable. */
		int16_t		getOutputBindDateYear(const char *variable);

		/** Get the month from a previously
		 *  defined date output bind variable. */
		int16_t		getOutputBindDateMonth(const char *variable);

		/** Get the day from a previously
		 *  defined date output bind variable. */
		int16_t		getOutputBindDateDay(const char *variable);

		/** Get the hour from a previously
		 *  defined date output bind variable. */
		int16_t		getOutputBindDateHour(const char *variable);

		/** Get the minute from a previously
		 *  defined date output bind variable. */
		int16_t		getOutputBindDateMinute(const char *variable);

		/** Get the second from a previously
		 *  defined date output bind variable. */
		int16_t		getOutputBindDateSecond(const char *variable);

		/** Get the microsecond from a previously
		 *  defined date output bind variable. */
		int32_t		getOutputBindDateMicrosecond(
							const char *variable);

		/** Get the time zone from a previously
		 *  defined date output bind variable. */
		const char	*getOutputBindDateTz(const char *variable);

		/** Get whether the value is negative from a
		 *  previously defined date output bind variable. */
		bool		getOutputBindDateIsNegative(
							const char *variable);

		/** Get the value stored in a previously
		 *  defined binary lob output bind variable. */
		const char	*getOutputBindBlob(const char *variable);

		/** Get the value stored in a previously
		 *  defined character lob output bind variable. */
		const char	*getOutputBindClob(const char *variable);

		/** Get the length of the value stored in a
		 *  previously defined output bind variable. */
		uint32_t	getOutputBindLength(const char *variable);

		/** Get the cursor associated with a previously
		 *  defined output bind variable. */
		sqlrcursor	*getOutputBindCursor(const char *variable);


		
		/** Opens a cached result set.
		 *  Returns true on success and false on failure. */
		bool		openCachedResultSet(const char *filename);



		/** Returns the number of columns in the current
		 *  result set. */
		uint32_t	colCount();

		/** Returns the number of rows in the current 
		 *  result set (if the result set is being
		 *  stepped through, this returns the number
		 *  of rows processed so far). */
		uint64_t	rowCount();

		/** Returns the total number of rows that will 
		 *  be returned in the result set.  Not all 
		 *  databases support this call.  Don't use it 
		 *  for applications which are designed to be 
		 *  portable across databases.  0 is returned
		 *  by databases which don't support this option. */
		uint64_t	totalRows();

		/** Returns the number of rows that were 
		 *  updated, inserted or deleted by the query.
		 *  Not all databases support this call.  Don't 
		 *  use it for applications which are designed 
		 *  to be portable across databases.  0 is 
		 *  returned by databases which don't support 
		 *  this option. */
		uint64_t	affectedRows();

		/** Returns the index of the first buffered row.
		 *  This is useful when buffering only part of
		 *  the result set at a time. */
		uint64_t	firstRowIndex();

		/** Returns false if part of the result set is
		 *  still pending on the server and true if not.
		 *  This method can only return false if 
		 *  setResultSetBufferSize() has been called
		 *  with a parameter other than 0. */
		bool		endOfResultSet();
		
		

		/** If a query failed and generated an error, 
		 *  the error message is available here.  If 
		 *  the query succeeded then this method 
		 *  returns NULL. */
		const char	*errorMessage();

		/** If a query failed and generated an
		 *  error, the error number is available here.
		 *  If there is no error then this method 
		 *  returns 0. */
		int64_t		errorNumber();



		/** Tells the connection to return NULL fields
		 *  and output bind variables as empty strings. 
		 *  This is the default. */
		void	getNullsAsEmptyStrings();

		/** Tells the connection to return NULL fields
		 *  and output bind variables as NULL's rather
		 *  than as empty strings. */
		void	getNullsAsNulls();



		/** Returns the specified field as a string. */
		const char	*getField(uint64_t row, uint32_t col);

		/** Returns the specified field as a string */
		const char	*getField(uint64_t row, const char *col);

		/** Returns the specified field as an integer. */
		int64_t	getFieldAsInteger(uint64_t row, uint32_t col);

		/** Returns the specified field as an integer. */
		int64_t	getFieldAsInteger(uint64_t row, const char *col);

		/** Returns the specified field as a decimal. */
		double	getFieldAsDouble(uint64_t row, uint32_t col);

		/** Returns the specified field as a decimal. */
		double	getFieldAsDouble(uint64_t row, const char *col);

		/** Returns the specified field as a boolean. */
		bool	getFieldAsBoolean(uint64_t row, uint32_t col);

		/** Returns the specified field as a boolean. */
		bool	getFieldAsBoolean(uint64_t row, const char *col);

		/** Interprets the specified field as a date and
		 *  populates its broken-down parts.
		 *
		 *  Returns true if it was able to interpret the
		 *  field as a date, and false otherwise. */
		bool	getFieldAsDate(uint64_t row, uint32_t col,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					bool *isnegative);

		/** Interprets the specified field as a date and
		 *  populates its broken-down parts.
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
		 *  then it defaults to "/-.:".
		 *
		 *  Returns true if it was able to interpret the
		 *  field as a date, and false otherwise. */
		bool	getFieldAsDate(uint64_t row, uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					bool *isnegative);

		/** Interprets the specified field as a date and
		 *  populates its broken-down parts.
		 *
		 *  Returns true if it was able to interpret the
		 *  field as a date, and false otherwise. */
		bool	getFieldAsDate(uint64_t row, const char *col,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					bool *isnegative);

		/** Interprets the specified field as a date and
		 *  populates its broken-down parts.
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
		 *  then it defaults to "/-.:".
		 *
		 *  Returns true if it was able to interpret the
		 *  field as a date, and false otherwise. */
		bool	getFieldAsDate(uint64_t row, const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					bool *isnegative);

		/** Interprets the specified field as a date
		 *  and returns the year component. */
		int16_t	getFieldAsDateYear(uint64_t row, uint32_t col);

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
		int16_t	getFieldAsDateYear(uint64_t row, uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the year component. */
		int16_t	getFieldAsDateYear(uint64_t row,
					const char *col);

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
		int16_t	getFieldAsDateYear(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the month component. */
		int16_t	getFieldAsDateMonth(uint64_t row, uint32_t col);

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
		int16_t	getFieldAsDateMonth(uint64_t row, uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the month component. */
		int16_t	getFieldAsDateMonth(uint64_t row,
					const char *col);

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
		int16_t	getFieldAsDateMonth(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the day component. */
		int16_t	getFieldAsDateDay(uint64_t row, uint32_t col);

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
		int16_t	getFieldAsDateDay(uint64_t row, uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the day component. */
		int16_t	getFieldAsDateDay(uint64_t row,
					const char *col);

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
		int16_t	getFieldAsDateDay(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the hour component. */
		int16_t	getFieldAsDateHour(uint64_t row, uint32_t col);

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
		int16_t	getFieldAsDateHour(uint64_t row, uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the hour component. */
		int16_t	getFieldAsDateHour(uint64_t row,
					const char *col);

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
		int16_t	getFieldAsDateHour(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the minute component. */
		int16_t	getFieldAsDateMinute(uint64_t row, uint32_t col);

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
		int16_t	getFieldAsDateMinute(uint64_t row, uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the minute component. */
		int16_t	getFieldAsDateMinute(uint64_t row,
					const char *col);

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
		int16_t	getFieldAsDateMinute(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the second component. */
		int16_t	getFieldAsDateSecond(uint64_t row, uint32_t col);

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
		int16_t	getFieldAsDateSecond(uint64_t row, uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the second component. */
		int16_t	getFieldAsDateSecond(uint64_t row,
					const char *col);

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
		int16_t	getFieldAsDateSecond(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the microsecond component. */
		int32_t	getFieldAsDateMicrosecond(uint64_t row,
						uint32_t col);

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
		int32_t	getFieldAsDateMicrosecond(uint64_t row,
					uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns the microsecond component. */
		int32_t	getFieldAsDateMicrosecond(uint64_t row,
						const char *col);

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
		int32_t	getFieldAsDateMicrosecond(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns whether the hour component
		 *  is negative. */
		bool	getFieldAsDateIsNegative(uint64_t row,
						uint32_t col);

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
		bool	getFieldAsDateIsNegative(uint64_t row,
					uint32_t col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);

		/** Interprets the specified field as a date
		 *  and returns whether the hour component
		 *  is negative. */
		bool	getFieldAsDateIsNegative(uint64_t row,
						const char *col);

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
		bool	getFieldAsDateIsNegative(uint64_t row,
					const char *col,
					bool ddmm, bool yyyyddmm,
					const char *datedelimiters);



		/** Returns the length of the specified field. */
		uint32_t	getFieldLength(uint64_t row, uint32_t col);

		/** Returns the length of the specified field. */
		uint32_t	getFieldLength(uint64_t row, const char *col);



		/** Returns a null terminated array of the 
		 *  values of the fields in the specified row. */
		const char * const *getRow(uint64_t row);

		/** Returns a null terminated array of the 
		 *  lengths of the fields in the specified row. */
		uint32_t	*getRowLengths(uint64_t row);

		/** Returns a null terminated array of the 
		 *  column names of the current result set. */
		const char * const *getColumnNames();

		/** Returns the name of the specified column. */
		const char	*getColumnName(uint32_t col);

		/** Returns the type of the specified column. */
		const char	*getColumnType(uint32_t col);

		/** Returns the type of the specified column. */
		const char	*getColumnType(const char *col);

		/** Returns the number of bytes required on
		 *  the server to store the data for the specified column */
		uint32_t	getColumnLength(uint32_t col);

		/** Returns the number of bytes required on
		 *  the server to store the data for the specified column */
		uint32_t	getColumnLength(const char *col);

		/** Returns the precision of the specified
		 *  column.
		 *  Precision is the total number of digits in
		 *  a number.  eg: 123.45 has a precision of 5.
		 *  For non-numeric types, it's the number of
		 *  characters in the string. */
		uint32_t	getColumnPrecision(uint32_t col);

		/** Returns the precision of the specified
		 *  column.
		 *  Precision is the total number of digits in
		 *  a number.  eg: 123.45 has a precision of 5.
		 *  For non-numeric types, it's the number of
		 *  characters in the string. */
		uint32_t	getColumnPrecision(const char *col);

		/** Returns the scale of the specified column.
		 *  Scale is the total number of digits to the
		 *  right of the decimal point in a number.
		 *  eg: 123.45 has a scale of 2. */
		uint32_t	getColumnScale(uint32_t col);

		/** Returns the scale of the specified column.
		 *  Scale is the total number of digits to the
		 *  right of the decimal point in a number.
		 *  eg: 123.45 has a scale of 2. */
		uint32_t	getColumnScale(const char *col);

		/** Returns true if the specified column can
		 *  contain nulls and false otherwise. */
		bool		getColumnIsNullable(uint32_t col);

		/** Returns true if the specified column can
		 *  contain nulls and false otherwise. */
		bool		getColumnIsNullable(const char *col);

		/** Returns true if the specified column is a
		 *  primary key and false otherwise. */
		bool		getColumnIsPrimaryKey(uint32_t col);

		/** Returns true if the specified column is a
		 *  primary key and false otherwise. */
		bool		getColumnIsPrimaryKey(const char *col);

		/** Returns true if the specified column is
 		 *  unique and false otherwise. */
		bool		getColumnIsUnique(uint32_t col);

		/** Returns true if the specified column is
 		 *  unique and false otherwise. */
		bool		getColumnIsUnique(const char *col);

		/** Returns true if the specified column is
		 *  part of a composite key and false otherwise. */
		bool		getColumnIsPartOfKey(uint32_t col);

		/** Returns true if the specified column is
		 *  part of a composite key and false otherwise. */
		bool		getColumnIsPartOfKey(const char *col);

		/** Returns true if the specified column is
		 *  an unsigned number and false otherwise. */
		bool		getColumnIsUnsigned(uint32_t col);

		/** Returns true if the specified column is
		 *  an unsigned number and false otherwise. */
		bool		getColumnIsUnsigned(const char *col);

		/** Returns true if the specified column was
		 *  created with the zero-fill flag and false
		 *  otherwise. */
		bool		getColumnIsZeroFilled(uint32_t col);

		/** Returns true if the specified column was
		 *  created with the zero-fill flag and false
		 *  otherwise. */
		bool		getColumnIsZeroFilled(const char *col);

		/** Returns true if the specified column
		 *  contains binary data and false
		 *  otherwise. */
		bool		getColumnIsBinary(uint32_t col);

		/** Returns true if the specified column
		 *  contains binary data and false
		 *  otherwise. */
		bool		getColumnIsBinary(const char *col);

		/** Returns true if the specified column
		 *  auto-increments and false otherwise. */
		bool		getColumnIsAutoIncrement(uint32_t col);

		/** Returns true if the specified column
		 *  auto-increments and false otherwise. */
		bool		getColumnIsAutoIncrement(const char *col);

		/** Returns the length of the longest field
		 *  in the specified column. */
		uint32_t	getLongest(uint32_t col);

		/** Returns the length of the longest field
		 *  in the specified column. */
		uint32_t	getLongest(const char *col);



		/** Tells the server to leave this result
		 *  set open when the connection calls 
		 *  suspendSession() so that another connection 
		 *  can connect to it using resumeResultSet() 
		 *  after it calls resumeSession(). */
		void	suspendResultSet();

		/** Returns the internal ID of this result set.
		 *  This parameter may be passed to another 
		 *  cursor for use in the resumeResultSet() 
		 *  method.
		 *  Note: The value this method returns is only
		 *  valid after a call to suspendResultSet(). */
		uint16_t	getResultSetId();

		/** Resumes a result set previously left open 
		 *  using suspendSession().
		 *  Returns true on success and false on failure. */
		bool	resumeResultSet(uint16_t id);

		/** Resumes a result set previously left open
		 *  using suspendSession() and continues caching
		 *  the result set to "filename".
		 *  Returns true on success and false on failure. */
		bool	resumeCachedResultSet(uint16_t id,
						const char *filename);


		/** Returns true and acts like executeQuery()
		 *  when there is another result set available
		 *  from the server. */
		bool	nextResultSet();


		/** Closes the current result set, if one is open.  Data
		 *  that has been fetched already is still available but
		 *  no more data may be fetched.  Server side resources
		 *  for the result set are freed as well. */
		void	closeResultSet();

	#include <sqlrelay/private/sqlrcursor.h>
};

#endif

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



		/** Sets the current database (catalog) to "database" */
		function selectDatabase(var database);

		/** Returns the database (catalog) that is currently in use. */
		function getCurrentDatabase();

		/** Sets the current schema to "schema" */
		function selectSchema(var schema);

		/** Returns the schema that is currently in use. */
		function getCurrentSchema();



		/** Returns the value of the autoincrement
		 *  column for the last insert */
		function getLastInsertId();



		/** Instructs the database to perform a commit
		 *  after every successful query. */
		function autoCommitOn();

		/** Instructs the database to wait for the 
		 *  client to tell it when to commit. */
		function autoCommitOff();


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


		/** Sets the isolation level to "isolationlevel", the
		 *  database-secific isolation level.  Returns true if setting
		 *  the isolation level succeeded, false if it failed. */
		function setIsolationLevel(var isolationlevel);

		/** Returns the database-specific isolation level, "unknown"
		 *  if the isolation level is unknown, or NULL if an error
		 *  occurred. */
		function getIsolationLevel();

		/** Returns the value of the specified database "feature".
		 *
		 *  Valid features include:
		 *  * "all_procedures_are_callable"
		 *  * "all_tables_are_selectable"
		 *  * "auto_commit_failure_closes_all_result_sets"
		 *  * "catalog_separator"
		 *  * "catalog_term"
		 *  * "collation_seq"
		 *  * "data_definition_causes_transaction_commit"
		 *  * "data_definition_ignored_in_transactions"
		 *  * "default_isolation_level"
		 *  * "deletes_are_detected"
		 *  * "does_max_row_size_include_blobs"
		 *  * "extra_name_characters"
		 *  * "generated_key_always_returned"
		 *  * "identifier_quote_string"
		 *  * "index_keywords"
		 *  * "info_schema_views"
		 *  * "inserts_are_detected"
		 *  * "is_catalog_at_start"
		 *  * "is_read_only"
		 *  * "locators_update_copy"
		 *  * "max_binary_literal_length"
		 *  * "max_catalog_name_length"
		 *  * "max_char_literal_length"
		 *  * "max_column_name_length"
		 *  * "max_columns_in_group_by"
		 *  * "max_columns_in_index"
		 *  * "max_columns_in_order_by"
		 *  * "max_columns_in_select"
		 *  * "max_columns_in_table"
		 *  * "max_connections"
		 *  * "max_cursor_name_length"
		 *  * "max_identifier_length"
		 *  * "max_index_length"
		 *  * "max_procedure_name_length"
		 *  * "max_row_size"
		 *  * "max_schema_name_length"
		 *  * "max_statement_length"
		 *  * "max_statements"
		 *  * "max_table_name_length"
		 *  * "max_tables_in_select"
		 *  * "max_user_name_length"
		 *  * "need_long_data_length"
		 *  * "null_plus_non_null_is_null"
		 *  * "nulls_are_sorted_at_end"
		 *  * "nulls_are_sorted_at_start"
		 *  * "nulls_are_sorted_high"
		 *  * "nulls_are_sorted_low"
		 *  * "numeric_functions"
		 *  * "others_deletes_are_visible"
		 *  * "others_inserts_are_visible"
		 *  * "others_updates_are_visible"
		 *  * "own_deletes_are_visible"
		 *  * "own_inserts_are_visible"
		 *  * "own_updates_are_visible"
		 *  * "procedure_term"
		 *  * "result_set_holdability"
		 *  * "row_id_lifetime"
		 *  * "schema_term"
		 *  * "search_string_escape"
		 *  * "sql_keywords"
		 *  * "sql_state_type"
		 *  * "stores_lower_case_identifiers"
		 *  * "stores_lower_case_quoted_identifiers"
		 *  * "stores_mixed_case_identifiers"
		 *  * "stores_mixed_case_quoted_identifiers"
		 *  * "stores_upper_case_identifiers"
		 *  * "stores_upper_case_quoted_identifiers"
		 *  * "string_functions"
		 *  * "supported_foreign_key_delete_rules"
		 *  * "supported_foreign_key_update_rules"
		 *  * "supported_predicates"
		 *  * "supported_relational_join_operators"
		 *  * "supported_row_value_constructor_expressions"
		 *  * "supported_value_expressions"
		 *  * "supports_aggregate_functions"
		 *  * "supports_alter_domain"
		 *  * "supports_alter_table_with_add_column"
		 *  * "supports_alter_table_with_drop_column"
		 *  * "supports_ansi92_entry_level_sql"
		 *  * "supports_ansi92_full_sql"
		 *  * "supports_ansi92_intermediate_sql"
		 *  * "supports_batch_updates"
		 *  * "supports_catalogs_in_data_manipulation"
		 *  * "supports_catalogs_in_index_definitions"
		 *  * "supports_catalogs_in_privilege_definitions"
		 *  * "supports_catalogs_in_procedure_calls"
		 *  * "supports_catalogs_in_table_definitions"
		 *  * "supports_column_aliasing"
		 *  * "supports_convert"
		 *  * "supports_core_sql_grammar"
		 *  * "supports_correlated_subqueries"
		 *  * "supports_create_assertion"
		 *  * "supports_create_character_set"
		 *  * "supports_create_collation"
		 *  * "supports_create_domain"
		 *  * "supports_create_schema"
		 *  * "supports_create_table"
		 *  * "supports_create_translation"
		 *  * "supports_create_view"
		 *  * "supports_data_definition_and_data_manipulation_transactions"
		 *  * "supports_data_manipulation_transactions_only"
		 *  * "supports_ddl_index"
		 *  * "supports_describe_parameter"
		 *  * "supports_different_table_correlation_names"
		 *  * "supports_drop_assertion"
		 *  * "supports_drop_character_set"
		 *  * "supports_drop_collation"
		 *  * "supports_drop_domain"
		 *  * "supports_drop_schema"
		 *  * "supports_drop_table"
		 *  * "supports_drop_translation"
		 *  * "supports_drop_view"
		 *  * "supports_expressions_in_order_by"
		 *  * "supports_extended_sql_grammar"
		 *  * "supports_full_outer_joins"
		 *  * "supports_get_generated_keys"
		 *  * "supports_grant"
		 *  * "supports_group_by"
		 *  * "supports_group_by_beyond_select"
		 *  * "supports_group_by_unrelated"
		 *  * "supports_insert_statement"
		 *  * "supports_integrity_enhancement_facility"
		 *  * "supports_like_escape_clause"
		 *  * "supports_limited_outer_joins"
		 *  * "supports_lock_types"
		 *  * "supports_minimum_sql_grammar"
		 *  * "supports_mixed_case_identifiers"
		 *  * "supports_mixed_case_quoted_identifiers"
		 *  * "supports_multiple_result_sets"
		 *  * "supports_multiple_transactions"
		 *  * "supports_named_parameters"
		 *  * "supports_non_nullable_columns"
		 *  * "supports_open_cursors_across_commit"
		 *  * "supports_open_cursors_across_rollback"
		 *  * "supports_open_statements_across_commit"
		 *  * "supports_open_statements_across_rollback"
		 *  * "supports_order_by_unrelated"
		 *  * "supports_outer_joins"
		 *  * "supports_positioned_delete"
		 *  * "supports_positioned_update"
		 *  * "supports_result_set_concurrency"
		 *  * "supports_result_set_holdability"
		 *  * "supports_result_set_type"
		 *  * "supports_revoke"
		 *  * "supports_savepoints"
		 *  * "supports_schemas_in_data_manipulation"
		 *  * "supports_schemas_in_index_definitions"
		 *  * "supports_schemas_in_privilege_definitions"
		 *  * "supports_schemas_in_procedure_calls"
		 *  * "supports_schemas_in_table_definitions"
		 *  * "supports_select_for_update"
		 *  * "supports_stored_functions_using_call_syntax"
		 *  * "supports_stored_procedures"
		 *  * "supports_subqueries_in_comparisons"
		 *  * "supports_subqueries_in_exists"
		 *  * "supports_subqueries_in_ins"
		 *  * "supports_subqueries_in_quantifieds"
		 *  * "supports_table_correlation_names"
		 *  * "supports_transaction_isolation_level"
		 *  * "supports_transactions"
		 *  * "supports_union"
		 *  * "supports_union_all"
		 *  * "system_functions"
		 *  * "table_term"
		 *  * "time_date_add_intervals"
		 *  * "time_date_diff_intervals"
		 *  * "time_date_functions"
		 *  * "time_date_literals"
		 *  * "updates_are_detected"
		 *  * "uses_local_file_per_table"
		 *  * "uses_local_files"
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



		/** Sends a query that returns a list of
		 *  databases/schemas matching "wild".  If wild is empty
		 *  or NULL then a list of all databases/schemas will be
		 *  returned. */
		function getDatabaseList(var wild);

		/** Sends a query that returns a list of tables
		 *  matching "wild".  If wild is empty or NULL then
		 *  a list of all tables will be returned. */
		function getTableList(var wild);

		/** Sends a query that returns a list of columns
		 *  in the table specified by the "table" parameter
		 *  matching "wild".  If wild is empty or NULL then
		 *  a list of all columns will be returned. */
		function getColumnList(var table, var wild);



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

		/** Returns the specified field as an integer. */
		function getFieldAsInteger(var row, var col);

		/** Returns the specified field as an integer. */
		function getFieldAsInteger(var row, var col);

		/** Returns the specified field as a decimal. */
		function getFieldAsDouble(var row, var col);

		/** Returns the specified field as a decimal. */
		function getFieldAsDouble(var row, var col);

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

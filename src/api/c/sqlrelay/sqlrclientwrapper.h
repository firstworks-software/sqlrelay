/* Copyright (c) David Muse
 See the file COPYING for more information */

#ifndef SQLRCLIENTWRAPPER_H
#define SQLRCLIENTWRAPPER_H

#include <sqlrelay/private/sqlrclientwrapperincludes.h>

/** @file
 *  @defgroup sqlrclientwrapper sqlrclientwrapper */

typedef	struct sqlrconnection *sqlrcon;
typedef	struct sqlrcursor *sqlrcur;

/** @ingroup sqlrclientwrapper
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
 *  If the "socket" parameter is nether NULL nor "" then an attempt will be
 *  made to connect through it before attempting to connect to "server" on
 *  "port".  If it is NULL or "" then no attempt will be made to connect
 *  through the socket.*/
SQLRCLIENT_DLLSPEC
sqlrcon	sqlrcon_alloc(const char *server, uint16_t port, const char *socket,
					const char *user, const char *password, 
					int32_t retrytime, int32_t tries);

/** @ingroup sqlrclientwrapper
 *  Disconnects and ends the session if it hasn't been terminated already. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_free(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Sets the server connect timeout in seconds and
 *  milliseconds.  Setting either parameter to -1 disables the
 *  timeout.  You can also set this timeout using the
 *  SQLR_CLIENT_CONNECT_TIMEOUT environment variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_setConnectTimeout(sqlrcon sqlrconref,
				int32_t timeoutsec, int32_t timeoutusec);

/** @ingroup sqlrclientwrapper
 *  Gets the server connect timeout in seconds and microseconds. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_getConnectTimeout(sqlrcon sqlrconref,
				int32_t *timeoutsec, int32_t *timeoutusec);

/** @ingroup sqlrclientwrapper
 *  Gets the server connect timeout in seconds. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcon_getConnectTimeoutSeconds(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Gets the server connect timeout in microseconds. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcon_getConnectTimeoutMicroseconds(sqlrcon sqlrconref);


/** @ingroup sqlrclientwrapper
 *  Sets the response timeout (for queries, commits, rollbacks,
 *  pings, etc.) in seconds and milliseconds.  Setting either
 *  parameter to -1 disables the timeout.  You can also set
 *  this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
 *  environment variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_setResponseTimeout(sqlrcon sqlrconref,
				int32_t timeoutsec, int32_t timeoutusec);

/** @ingroup sqlrclientwrapper
 *  Gets the response timeout in seconds and microseconds. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_getResponseTimeout(sqlrcon sqlrconref,
				int32_t *timeoutsec, int32_t *timeoutusec);

/** @ingroup sqlrclientwrapper
 *  Gets the response timeout in seconds. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcon_getResponseTimeoutSeconds(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Gets the response timeout in microseconds. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcon_getResponseTimeoutMicroseconds(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Sets which delimiters are used to identify bind variables
 *  in countBindVariables() and validateBinds().  Valid
 *  delimiters include ?,:,@, and $.  Defaults to "?:@$" */
void	sqlrcon_setBindVariableDelimiters(sqlrcon sqlrconref,
						const char *delimiters);

/** @ingroup sqlrclientwrapper
 *  Returns true if question marks (?) are considered to be
 *  valid bind variable delimiters. */
int	sqlrcon_getBindVariableDelimiterQuestionMarkSupported(
						sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns true if colons (:) are considered to be
 *  valid bind variable delimiters. */
int	sqlrcon_getBindVariableDelimiterColonSupported(
						sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns true if at-signs (@) are considered to be
 *  valid bind variable delimiters. */
int	sqlrcon_getBindVariableDelimiterAtSignSupported(
						sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns true if dollar signs ($) are considered to be
 *  valid bind variable delimiters. */
int	sqlrcon_getBindVariableDelimiterDollarSignSupported(
						sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
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
SQLRCLIENT_DLLSPEC
void	sqlrcon_enableKerberos(sqlrcon sqlrconref,
					const char *service,
					const char *mech,
					const char *flags);

/** @ingroup sqlrclientwrapper
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
SQLRCLIENT_DLLSPEC
void	sqlrcon_enableTls(sqlrcon sqlrconref,
				const char *version,
				const char *cert,
				const char *password,
				const char *ciphers,
				const char *validate,
				const char *ca,
				uint16_t depth);

/** @ingroup sqlrclientwrapper
  * Disables encryption. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_disableEncryption(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Ends the session. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_endSession(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Disconnects this connection from the current session but leaves the session
 *  open so that another connection can connect to it using
 *  sqlrcon_resumeSession(). */
SQLRCLIENT_DLLSPEC
int	sqlrcon_suspendSession(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the inet port that the connection is communicating over.  This
 *  parameter may be passed to another connection for use in the
 *  sqlrcon_resumeSession() command.  Note: The result this function returns
 *  is only valid after a call to suspendSession(). */
SQLRCLIENT_DLLSPEC
uint16_t	sqlrcon_getConnectionPort(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the unix socket that the connection is communicating over.  This
 *  parameter may be passed to another connection for use in the
 *  sqlrcon_resumeSession() command.  Note: The result this function returns
 *  is only valid after a call to suspendSession(). */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_getConnectionSocket(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Resumes a session previously left open using sqlrcon_suspendSession().
 *  Returns 1 on success and 0 on failure. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_resumeSession(sqlrcon sqlrconref, uint16_t port,
							const char *socket);



/** @ingroup sqlrclientwrapper
 *  Returns 1 if the database is up and 0 if it's down. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_ping(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the type of database: oracle, postgresql, mysql, etc. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_identify(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the version of the database */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_dbVersion(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the host name of the database */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_dbHostName(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the ip address of the database */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_dbIpAddress(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the version of the sqlrelay server software. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_serverVersion(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the version of the sqlrelay client software. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_clientVersion(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns a string representing the bind variable format used
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
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_bindFormat(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
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
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_nextvalFormat(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Sets the current database (catalog) to "database" */
SQLRCLIENT_DLLSPEC
int	sqlrcon_selectDatabase(sqlrcon sqlrconref, const char *database);

/** @ingroup sqlrclientwrapper
 *  Returns the database (catalog) that is currently in use. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_getCurrentDatabase(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Sets the current schema to "schema" */
SQLRCLIENT_DLLSPEC
int	sqlrcon_selectSchema(sqlrcon sqlrconref, const char *schema);

/** @ingroup sqlrclientwrapper
 *  Returns the schema that is currently in use. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_getCurrentSchema(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Returns the value of the autoincrement column for the last insert */
SQLRCLIENT_DLLSPEC
uint64_t	sqlrcon_getLastInsertId(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Instructs the database to perform a commit after every successful query. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_autoCommitOn(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Instructs the database to wait for the client to tell it when to commit. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_autoCommitOff(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Begins a transaction.  Returns 1 if the begin
 *  succeeded, 0 if it failed.  If the database
 *  automatically begins a new transaction when a
 *  commit or rollback is issued then this doesn't
 *  do anything unless SQL Relay is faking transaction
 *  blocks. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_begin(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Issues a commit.  Returns 1 if the commit succeeded, 0 if it failed. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_commit(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Issues a rollback.  Returns 1 if the rollback succeeded, 0 if it failed. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_rollback(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Sets the isolation level to "isolationlevel", the database-secific
 *  isolation level.  Returns 1 if setting the isolation level succeeded,
 *  0 if it failed. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_setIsolationLevel(sqlrcon sqlrconref,
					const char *isolationlevel);

/** @ingroup sqlrclientwrapper
 *  Returns the database-specific isolation level, "unknown" if the
 *  isolation level is unknown, or NULL if an error occurred. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_getIsolationLevel(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns the value of the specified database "feature".
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
 *  * "sql_keywords"
 *  * "sql_state_type"
 *  * "schema_term"
 *  * "search_string_escape"
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
 *  * "supports_ansi92_entry_level_sql"
 *  * "supports_ansi92_full_sql"
 *  * "supports_ansi92_intermediate_sql"
 *  * "supports_alter_table_with_add_column"
 *  * "supports_alter_table_with_drop_column"
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
 *  * "supports_drop_assertion"
 *  * "supports_drop_character_set"
 *  * "supports_drop_collation"
 *  * "supports_drop_domain"
 *  * "supports_drop_schema"
 *  * "supports_drop_table"
 *  * "supports_drop_translation"
 *  * "supports_drop_view"
 *  * "supports_different_table_correlation_names"
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
 *  * "supports_result_set_concurrency_fo_ro"
 *  * "supports_result_set_concurrency_fo_u"
 *  * "supports_result_set_concurrency_si_ro"
 *  * "supports_result_set_concurrency_si_u"
 *  * "supports_result_set_concurrency_ss_ro"
 *  * "supports_result_set_concurrency_ss_u"
 *  * "supports_result_set_holdability_ccac"
 *  * "supports_result_set_holdability_hcac"
 *  * "supports_result_set_type_fo"
 *  * "supports_result_set_type_si"
 *  * "supports_result_set_type_ss"
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
 *  * "supports_transaction_isolation_level_n"
 *  * "supports_transaction_isolation_level_ru"
 *  * "supports_transaction_isolation_level_rc"
 *  * "supports_transaction_isolation_level_rr"
 *  * "supports_transaction_isolation_level_s"
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
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_getDatabaseFeature(sqlrcon sqlrconref,
						const char *feature);



/** @ingroup sqlrclientwrapper
 *  If an operation failed and generated an error, the error message is
 *  available here.  If there is no error then this method returns NULL */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_errorMessage(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  If an operation failed and generated an error, the error number is
 *  available here.  If there is no error then this method returns 0. */
SQLRCLIENT_DLLSPEC
int64_t		sqlrcon_errorNumber(sqlrcon sqlrconref);


/** @ingroup sqlrclientwrapper
 *  Causes verbose debugging information to be sent to standard output.
 *  Another way to do this is to start a query with "-- debug\n".
 *  Yet another way is to set the environment variable SQLR_CLIENT_DEBUG
 *  to "ON" */
SQLRCLIENT_DLLSPEC
void	sqlrcon_debugOn(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Turns debugging off. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_debugOff(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Returns 0 if debugging is off and 1 if debugging is on. */
SQLRCLIENT_DLLSPEC
int	sqlrcon_getDebug(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Allows you to replace the function used to print debug messages with your
 *  own function.  The function is expected to take arguments like printf. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_debugPrintFunction(sqlrcon sqlrconref, 
					int (*printfunction)(const char *,...));

/** @ingroup sqlrclientwrapper
 *  Allows you to specify a file to write debug to.
 *  Setting "filename" to NULL or an empty string causes debug
 *  to be written to standard output (the default). */
SQLRCLIENT_DLLSPEC
void	sqlrcon_setDebugFile(sqlrcon sqlrconref, const char *filename);

/** @ingroup sqlrclientwrapper
 *  Allows you to set a string that will be passed to the server and ultimately
 *  included in server-side logging along with queries that were run by this
 *  instance of the client. */
SQLRCLIENT_DLLSPEC
void	sqlrcon_setClientInfo(sqlrcon sqlrconref, const char *clientinfo);

/** @ingroup sqlrclientwrapper
 *  Returns the string that was set by sqlrcon_setClientInfo(). */
SQLRCLIENT_DLLSPEC
const char	*sqlrcon_getClientInfo(sqlrcon sqlrconref);



/** @ingroup sqlrclientwrapper
 *  Creates a cursor to run queries and fetch
 *  result sets using connection "sqlrconref" */
SQLRCLIENT_DLLSPEC
sqlrcur	sqlrcur_alloc(sqlrcon sqlrconref);

/** @ingroup sqlrclientwrapper
 *  Destroys the cursor and cleans up all associated result set data. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_free(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  Sets the number of rows of the result set to buffer at a time.
 *  0 (the default) means buffer the entire result set. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_setResultSetBufferSize(sqlrcur sqlrcurref, uint64_t rows);

/** @ingroup sqlrclientwrapper
 *  Returns the number of result set rows that will be buffered at a time or
 *  0 for the entire result set. */
SQLRCLIENT_DLLSPEC
uint64_t	sqlrcur_getResultSetBufferSize(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  Tells the server not to send any column info (names, types, sizes).  If
 *  you don't need that info, you should call this function to improve
 *  performance. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_dontGetColumnInfo(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Tells the server to send column info. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_getColumnInfo(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  Columns names are returned in the same case as they are defined in the
 *  database.  This is the default. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_mixedCaseColumnNames(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Columns names are converted to upper case. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_upperCaseColumnNames(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Columns names are converted to lower case. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_lowerCaseColumnNames(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  Sets query caching on.  Future queries will be cached to the
 *  file "filename".
 * 
 *  A default time-to-live of 10 minutes is also set.
 * 
 *  Note that once sqlrcur_cacheToFile() is called, the result sets of all
 *  future queries will be cached to that file until another call to
 *  sqlrcur_cacheToFile() changes which file to cache to or a call to
 *  sqlrcur_cacheOff() turns off caching. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_cacheToFile(sqlrcur sqlrcurref, const char *filename);

/** @ingroup sqlrclientwrapper
 *  Sets the time-to-live for cached result sets. The sqlr-cachemanger will
 *  remove each cached result set "ttl" seconds after it's created, provided
 *  it's scanning the directory containing the cache files. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_setCacheTtl(sqlrcur sqlrcurref, uint32_t ttl);

/** @ingroup sqlrclientwrapper
 *  Returns the name of the file containing
 *  the most recently cached result set. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getCacheFileName(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Sets query caching off. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_cacheOff(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  Sends a query that returns a list of databases/schemas matching "wild".
 *  If wild is empty or NULL then a list of all databases/schemas will be
 *  returned. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getDatabaseList(sqlrcur sqlrcurref, const char *wild);

/** @ingroup sqlrclientwrapper
 *  Sends a query that returns a list of tables matching "wild".  If wild is
 *  empty or NULL then a list of all tables will be returned. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getTableList(sqlrcur sqlrcurref, const char *wild);

/** @ingroup sqlrclientwrapper
 *  Sends a query that returns a list of columns in the table specified by the
 *  "table" parameter matching "wild".  If wild is empty or NULL then a list of
 *  all columns will be returned. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnList(sqlrcur sqlrcurref,
				const char *table, const char *wild);



/** @ingroup sqlrclientwrapper
 *  Sends "query" directly and gets a result set. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_sendQuery(sqlrcur sqlrcurref, const char *query);

/** @ingroup sqlrclientwrapper
 *  Sends "query" with length "length" directly and gets a result set. This
 *  function must be used if the query contains binary data. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_sendQueryWithLength(sqlrcur sqlrcurref, const char *query,
							uint32_t length);

/** @ingroup sqlrclientwrapper
 *  Sends the query in file "path"/"filename" and gets a result set. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_sendFileQuery(sqlrcur sqlrcurref,
				const char *path, const char *filename);



/** @ingroup sqlrclientwrapper
 *  Prepare to execute "query". */
SQLRCLIENT_DLLSPEC
void	sqlrcur_prepareQuery(sqlrcur sqlrcurref, const char *query);

/** @ingroup sqlrclientwrapper
 *  Prepare to execute "query" with length "length".  This function must be
 *  used if the query contains binary data. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_prepareQueryWithLength(sqlrcur sqlrcurref,
						const char *query,
						uint32_t length);

/** @ingroup sqlrclientwrapper
 *  Prepare to execute the contents of "path"/"filename". */
SQLRCLIENT_DLLSPEC
void	sqlrcur_prepareFileQuery(sqlrcur sqlrcurref, 
					const char *path, const char *filename);



/** @ingroup sqlrclientwrapper
 *  Defines a string substitution variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_subString(sqlrcur sqlrcurref,
				const char *variable, const char *value);

/** @ingroup sqlrclientwrapper
 *  Defines a integer substitution variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_subLong(sqlrcur sqlrcurref,
				const char *variable, int64_t value);

/** @ingroup sqlrclientwrapper
 *  Defines a decimal substitution variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_subDouble(sqlrcur sqlrcurref,
				const char *variable, double value,
				uint32_t precision, uint32_t scale);

/** @ingroup sqlrclientwrapper
 *  Defines an array of string substitution variables. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_subStrings(sqlrcur sqlrcurref,
				const char **variables, const char **values);

/** @ingroup sqlrclientwrapper
 *  Defines an array of integer substitution variables. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_subLongs(sqlrcur sqlrcurref,
				const char **variables, const int64_t *values);

/** @ingroup sqlrclientwrapper
 *  Defines an array of decmial substitution variables. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_subDoubles(sqlrcur sqlrcurref,
				const char **variables, const double *values,
				const uint32_t *precisions,
				const uint32_t *scales);



/** @ingroup sqlrclientwrapper
 *  Defines a string input bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindString(sqlrcur sqlrcurref, 
				const char *variable, const char *value);

/** @ingroup sqlrclientwrapper
 *  Defines a string input bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindStringWithLength(sqlrcur sqlrcurref, 
				const char *variable,
				const char *value, uint32_t valuelength);

/** @ingroup sqlrclientwrapper
 *  Defines a integer input bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindLong(sqlrcur sqlrcurref, const char *variable, 
							int64_t value);

/** @ingroup sqlrclientwrapper
 *  Defines a decimal input bind variable.
 * (If you don't have the precision and scale then set
 * them both to 0.  However in that case you may get
 * unexpected rounding behavior if the server is faking
 * binds.) */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindDouble(sqlrcur sqlrcurref, 
					const char *variable, double value,
					uint32_t precision, 
					uint32_t scale);

/** @ingroup sqlrclientwraper
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
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindDate(sqlrcur sqlrcurref,
				const char *variable,
				int16_t year, int16_t month, int16_t day,
				int16_t hour, int16_t minute, int16_t second,
				int32_t microsecond, const char *tz,
				int isnegative);

/** @ingroup sqlrclientwrapper
 *  Defines a binary lob input bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindBlob(sqlrcur sqlrcurref, 
					const char *variable, const char *value,
					uint32_t size);

/** @ingroup sqlrclientwrapper
 *  Defines a character lob input bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindClob(sqlrcur sqlrcurref, 
					const char *variable, const char *value,
					uint32_t size);

/** @ingroup sqlrclientwrapper
 *  Defines an array of string input bind variables. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindStrings(sqlrcur sqlrcurref, 
					const char **variables,
					const char **values);

/** @ingroup sqlrclientwrapper
 *  Defines an array of integer input bind variables. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindLongs(sqlrcur sqlrcurref, 
					const char **variables, 
					const int64_t *values);

/** @ingroup sqlrclientwrapper
 *  Defines an array of decimal input bind variables. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_inputBindDoubles(sqlrcur sqlrcurref, 
					const char **variables,
					const double *values,
					const uint32_t *precisions, 
					const uint32_t *scales);



/** @ingroup sqlrclientwrapper
 *  Defines a string output bind variable.
 *  "length" bytes will be reserved to store the value. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_defineOutputBindString(sqlrcur sqlrcurref,
					const char *variable, uint32_t length);

/** @ingroup sqlrclientwrapper
 *  Defines an integer output bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_defineOutputBindInteger(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Defines an decimal output bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_defineOutputBindDouble(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *   Defines a date output bind variable. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_defineOutputBindDate(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Defines a binary lob output bind variable */
SQLRCLIENT_DLLSPEC
void	sqlrcur_defineOutputBindBlob(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Defines a character lob output bind variable */
SQLRCLIENT_DLLSPEC
void	sqlrcur_defineOutputBindClob(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Defines a cursor output bind variable */
SQLRCLIENT_DLLSPEC
void	sqlrcur_defineOutputBindCursor(sqlrcur sqlrcurref,
					const char *variable);



/** @ingroup sqlrclientwrapper
 *  Clears all bind variables. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_clearBinds(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Parses the previously prepared query, counts the number of bind variables
 *  defined in it and returns that number. */
SQLRCLIENT_DLLSPEC
uint16_t	sqlrcur_countBindVariables(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  If you are binding to any variables that might not actually be in your
 *  query, call this to ensure that the database won't try to bind them unless
 *  they really are in the query.  There is a performance penalty for calling
 *  this function */
SQLRCLIENT_DLLSPEC
void	sqlrcur_validateBinds(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns true if "variable" was a valid bind variable of the query. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_validBind(sqlrcur sqlrcurref, const char *variable);



/** @ingroup sqlrclientwrapper
 *  Execute the query that was previously prepared and bound. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_executeQuery(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Fetch from a cursor that was returned as an output bind variable. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_fetchFromBindCursor(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  Get the value stored in a previously defined
 *  string output bind variable. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getOutputBindString(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the value stored in a previously defined
 *  integer output bind variable. */
SQLRCLIENT_DLLSPEC
int64_t	sqlrcur_getOutputBindInteger(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the value stored in a previously defined
 *  decimal output bind variable. */
SQLRCLIENT_DLLSPEC
double	sqlrcur_getOutputBindDouble(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the value stored in a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getOutputBindDate(sqlrcur sqlrcurref,
				const char *variable,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute, int16_t *second,
				int32_t *microsecond, const char **tz,
				int *isnegative);

/** @ingroup sqlrclientwrapper
 *  Get the year from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getOutputBindDateYear(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the month from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getOutputBindDateMonth(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the day from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getOutputBindDateDay(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the hour from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getOutputBindDateHour(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the minute from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getOutputBindDateMinute(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the second from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getOutputBindDateSecond(sqlrcur sqlrcurref,
					const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the microsecond from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcur_getOutputBindDateMicrosecond(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the time zone from a previously
 *  defined date output bind variable. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getOutputBindDateTz(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get whether the value is negative from a
 *  previously defined date output bind variable. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getOutputBindDateIsNegative(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the value stored in a previously defined
 *  binary lob output bind variable. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getOutputBindBlob(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the value stored in a previously defined
 *  character lob output bind variable. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getOutputBindClob(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the length of the value stored in a previously
 *  defined output bind variable. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getOutputBindLength(sqlrcur sqlrcurref,
						const char *variable);

/** @ingroup sqlrclientwrapper
 *  Get the cursor associated with a previously defined output bind variable. */
SQLRCLIENT_DLLSPEC
sqlrcur	sqlrcur_getOutputBindCursor(sqlrcur sqlrcurref, const char *variable);



/** @ingroup sqlrclientwrapper
 *  Opens a cached result set.  Returns 1 on success and 0 on failure. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_openCachedResultSet(sqlrcur sqlrcurref, const char *filename);



/** @ingroup sqlrclientwrapper
 *  Returns the number of columns in the current result set. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_colCount(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns the number of rows in the current result set. */
SQLRCLIENT_DLLSPEC
uint64_t	sqlrcur_rowCount(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns the total number of rows that will be returned in the result set.
 *  Not all databases support this call.  Don't use it for applications which
 *  are designed to be portable across databases.  -1 is returned by databases
 *  which don't support this option. */
SQLRCLIENT_DLLSPEC
uint64_t	sqlrcur_totalRows(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns the number of rows that were updated, inserted or deleted by the
 *  query.  Not all databases support this call.  Don't use it for applications
 *  which are designed to be portable across databases.  -1 is returned by
 *  databases which don't support this option. */
SQLRCLIENT_DLLSPEC
uint64_t	sqlrcur_affectedRows(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns the index of the first buffered row.  This is useful when buffering
 *  only part of the result set at a time. */
SQLRCLIENT_DLLSPEC
uint64_t	sqlrcur_firstRowIndex(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns 0 if part of the result set is still pending on the server and 1 if
 *  not.  This function can only return 0 if setResultSetBufferSize() has been
 *  called with a parameter other than 0. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_endOfResultSet(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns true and acts like executeQuery() when there is another result set
 *  available from the server. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_nextResultSet(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  If a query failed and generated an error, the error message is available
 *  here.  If the query succeeded then this function returns a NULL. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_errorMessage(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  If a query failed and generated an error, the error number is available
 *  here.  If there is no error then this method returns 0. */
SQLRCLIENT_DLLSPEC
int64_t		sqlrcur_errorNumber(sqlrcur sqlrcurref);


/** @ingroup sqlrclientwrapper
 *  Tells the connection to return NULL fields and output bind variables as
 *  empty strings.  This is the default. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_getNullsAsEmptyStrings(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Tells the connection to return NULL fields
 *  and output bind variables as NULL's. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_getNullsAsNulls(sqlrcur sqlrcurref);



/** @ingroup sqlrclientwrapper
 *  Returns the specified field as a string. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getFieldByIndex(sqlrcur sqlrcurref,
						uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as a string. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getFieldByName(sqlrcur sqlrcurref,
						uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as an integer. */
SQLRCLIENT_DLLSPEC
int64_t	sqlrcur_getFieldAsIntegerByIndex(sqlrcur sqlrcurref,
						uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as an integer. */
SQLRCLIENT_DLLSPEC
int64_t	sqlrcur_getFieldAsIntegerByName(sqlrcur sqlrcurref,
						uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as an decimal. */
SQLRCLIENT_DLLSPEC
double	sqlrcur_getFieldAsDoubleByIndex(sqlrcur sqlrcurref,
						uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as an decimal. */
SQLRCLIENT_DLLSPEC
double	sqlrcur_getFieldAsDoubleByName(sqlrcur sqlrcurref,
						uint64_t row, const char *col);



/** @ingroup sqlrclientwrapper
 *  Returns the specified field as a date/time. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					int *isnegative);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as a date/time.
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
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					int *isnegative);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as a date/time. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					int *isnegative);

/** @ingroup sqlrclientwrapper
 *  Returns the specified field as a date/time.
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
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters,
					int16_t *year, int16_t *month,
					int16_t *day, int16_t *hour,
					int16_t *minute, int16_t *second,
					int32_t *microsecond,
					int *isnegative);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the year component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateYearByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the year component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateYearByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the year component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateYearByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the year component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateYearByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the month component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMonthByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the month component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMonthByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the month component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMonthByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the month component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMonthByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the day component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateDayByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the day component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateDayByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the day component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateDayByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the day component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateDayByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the hour component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateHourByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the hour component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateHourByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the hour component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateHourByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the hour component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateHourByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the minute component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMinuteByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the minute component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMinuteByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the minute component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMinuteByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the minute component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateMinuteByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the second component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateSecondByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the second component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateSecondByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the second component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateSecondByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the second component. */
SQLRCLIENT_DLLSPEC
int16_t	sqlrcur_getFieldAsDateSecondByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the microsecond component. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcur_getFieldAsDateMicrosecondByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the microsecond component. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcur_getFieldAsDateMicrosecondByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the microsecond component. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcur_getFieldAsDateMicrosecondByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns the microsecond component. */
SQLRCLIENT_DLLSPEC
int32_t	sqlrcur_getFieldAsDateMicrosecondByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns whether the hour component
 *  is negative. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateIsNegativeByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns whether the hour component
 *  is negative. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateIsNegativeByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns whether the hour component
 *  is negative. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateIsNegativeByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col);

/** @ingroup sqlrclientwrapper
 *  Interprets the specified field as a date
 *  and returns whether the hour component
 *  is negative. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getFieldAsDateIsNegativeByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters);



/** @ingroup sqlrclientwrapper
 *  Returns the length of the specified row and column. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getFieldLengthByIndex(sqlrcur sqlrcurref,
						uint64_t row, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the length of the specified row and column. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getFieldLengthByName(sqlrcur sqlrcurref,
						uint64_t row, const char *col);



/** @ingroup sqlrclientwrapper
 *  Returns a null terminated array of the values
 *  of the fields in the specified row. */
SQLRCLIENT_DLLSPEC
const char * const *sqlrcur_getRow(sqlrcur sqlrcurref, uint64_t row);

/** @ingroup sqlrclientwrapper
 *  Returns a null terminated array of the lengths
 *  of the fields in the specified row. */
SQLRCLIENT_DLLSPEC
uint32_t	*sqlrcur_getRowLengths(sqlrcur sqlrcurref, uint64_t row);

/** @ingroup sqlrclientwrapper
 *  Returns a null terminated array of the
 *  column names of the current result set. */
SQLRCLIENT_DLLSPEC
const char * const *sqlrcur_getColumnNames(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns the name of the specified column. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getColumnName(sqlrcur sqlrcurref, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the type of the specified column. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getColumnTypeByIndex(sqlrcur sqlrcurref, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the type of the specified column. */
SQLRCLIENT_DLLSPEC
const char	*sqlrcur_getColumnTypeByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns the length of the specified column. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getColumnLengthByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the length of the specified column. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getColumnLengthByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns the precision of the specified column.  Precision is the total
 *  number of digits in a number.  eg: 123.45 has a precision of 5.  For
 *  non-numeric types, it's the number of characters in the string. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getColumnPrecisionByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the precision of the specified column.  Precision is the total
 *  number of digits in a number.  eg: 123.45 has a precision of 5.  For
 *  non-numeric types, it's the number of characters in the string. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getColumnPrecisionByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns the scale of the specified column.  Scale is the total number of
 *  digits to the right of the decimal point in a number.  eg: 123.45 has a
 *  scale of 2. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getColumnScaleByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the scale of the specified column.  Scale is the total number of
 *  digits to the right of the decimal point in a number.  eg: 123.45 has a 
 *  scale of 2. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getColumnScaleByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns the scale of the specified column.  Scale is the total number of
 *  digits to the right of the decimal point in a number.  eg: 123.45 has a
 *  scale of 2. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsNullableByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column can contain nulls and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsNullableByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is a primary key and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsPrimaryKeyByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is a primary key and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsPrimaryKeyByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is unique and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int		sqlrcur_getColumnIsUniqueByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is unique and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsUniqueByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is part of a composite key and 0
 *  otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsPartOfKeyByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is part of a composite key and 0
 *  otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsPartOfKeyByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is an unsigned number and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsUnsignedByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column is an unsigned number and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsUnsignedByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column was created
 *  with the zero-fill flag and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsZeroFilledByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column was created
 *  with the zero-fill flag and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsZeroFilledByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column contains binary data and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsBinaryByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column contains binary data and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsBinaryByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column auto-increments and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsAutoIncrementByIndex(sqlrcur sqlrcurref,
							uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns 1 if the specified column auto-increments and 0 otherwise. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_getColumnIsAutoIncrementByName(sqlrcur sqlrcurref,
							const char *col);

/** @ingroup sqlrclientwrapper
 *  Returns the length of the longest field in the specified column. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getLongestByIndex(sqlrcur sqlrcurref, uint32_t col);

/** @ingroup sqlrclientwrapper
 *  Returns the length of the longest field in the specified column. */
SQLRCLIENT_DLLSPEC
uint32_t	sqlrcur_getLongestByName(sqlrcur sqlrcurref, const char *col);



/** @ingroup sqlrclientwrapper
 *  Tells the server to leave this result set open when the connection calls
 *  suspendSession() so that another connection can connect to it using
 *  resumeResultSet() after it calls resumeSession(). */
SQLRCLIENT_DLLSPEC
void	sqlrcur_suspendResultSet(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Returns the internal ID of this result set.  This parameter may be passed
 *  to another statement for use in the resumeResultSet() function.  Note: The
 *  value this function returns is only valid after a call to
 *  suspendResultSet().*/
SQLRCLIENT_DLLSPEC
uint16_t	sqlrcur_getResultSetId(sqlrcur sqlrcurref);

/** @ingroup sqlrclientwrapper
 *  Resumes a result set previously left open using suspendSession().
 *  Returns 1 on success and 0 on failure. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_resumeResultSet(sqlrcur sqlrcurref, uint16_t id);

/** @ingroup sqlrclientwrapper
 *  Resumes a result set previously left open using suspendSession() and
 *  continues caching the result set to "filename".  Returns 1 on success and 0
 *  on failure. */
SQLRCLIENT_DLLSPEC
int	sqlrcur_resumeCachedResultSet(sqlrcur sqlrcurref, 
					uint16_t id, const char *filename);

/** @ingroup sqlrclientwrapper
 *  Closes the current result set, if one is open.  Data
 *  that has been fetched already is still available but
 *  no more data may be fetched.  Server side resources
 *  for the result set are freed as well. */
SQLRCLIENT_DLLSPEC
void	sqlrcur_closeResultSet(sqlrcur sqlrcurref);

#include <sqlrelay/private/sqlrclientwrapper.h>

#endif

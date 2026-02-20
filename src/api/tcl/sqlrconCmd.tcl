# Copyright (c) 2003 Takeshi Taguchi
# See the file COPYING for more information

# Initiates a connection to "server" on "port"
# or to the unix "socket" on the local machine
# and auths with "user" and "password".
# Failed connections will be retried for 
# "tries" times, waiting "retrytime" seconds
# between each try.  If "tries" is 0 then retries
# will continue forever.  If "retrytime" is 0 then
# retries will be attempted on a default interval.
#
# If "server" is a comma-separated list of hosts, then an
# attempt will be made to connect to each until the attempt
# succeeds, or there are no more hosts left to try.
#
# If the "socket" parameter is neither 
# NULL nor "" then an attempt will be made to 
# connect through it before attempting to 
# connect to "server" on "port".  If it is 
# NULL or "" then no attempt will be made to 
# connect through the socket.
proc sqlrconCmd {server port socket user password retrytime tries} 


# Disconnects and ends the session if
# it hasn't been ended already.
proc sqlrconDelete {} 



# Sets the server connect timeout in seconds and
# microseconds.  Setting either parameter to -1 disables the
# timeout.  You can also set this timeout using the
# SQLR_CLIENT_CONNECT_TIMEOUT environment variable.
proc setConnectTimeout {timeoutsec timeoutusec}

# Sets the response timeout (for queries, commits, rollbacks,
# pings, etc.) in seconds and microseconds.  Setting either
# parameter to -1 disables the timeout.  You can also set
# this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
# environment variable.
proc setResponseTimeout {timeoutsec timeoutusec}

# Gets the server connect timeout in seconds.
proc getConnectTimeoutSeconds {}

# Gets the server connect timeout in microseconds.
proc getConnectTimeoutMicroseconds {}

# Gets the response timeout in seconds.
proc getResponseTimeoutSeconds {}

# Gets the response timeout in microseconds.
proc getResponseTimeoutMicroseconds {}



# Sets which delimiters are used to identify bind variables
# in countBindVariables() and validateBinds().  Valid
# delimiters include ?,:,@, and $.  Defaults to "?:@$"
proc setBindVariableDelimiters {delimiters}

# Returns true if question marks (?) are considered to be
# valid bind variable delimiters.
proc getBindVariableDelimiterQuestionMarkSupported {}

# Returns true if colons (:) are considered to be
# valid bind variable delimiters.
proc getBindVariableDelimiterColonSupported {}

# Returns true if at-signs (@) are considered to be
# valid bind variable delimiters.
proc getBindVariableDelimiterAtSignSupported {}

# Returns true if dollar signs ($) are considered to be
# valid bind variable delimiters.
proc getBindVariableDelimiterDollarSignSupported {}



# Enables Kerberos authentication and encryption.
#
# "service" indicates the Kerberos service name of the
# SQL Relay server.  If left empty or NULL then the service
# name "sqlrelay" will be used. "sqlrelay" is the default
# service name of the SQL Relay server.  Note that on Windows
# platforms the service name must be fully qualified,
# including the host and realm name.  For example:
# "sqlrelay/sqlrserver.firstworks.com@AD.FIRSTWORKS.COM".
#
# "mech" indicates the specific Kerberos mechanism to use.
# On Linux/Unix platforms, this should be a string
# representation of the mechnaism's OID, such as:
#     { 1 2 840 113554 1 2 2 }
# On Windows platforms, this should be a string like:
#     Kerberos
# If left empty or NULL then the default mechanism will be
# used.  Only set this if you know that you have a good
# reason to.
#
# "flags" indicates what Kerberos flags to use.  Multiple
# flags may be specified, separated by commas.  If left
# empty or NULL then a defalt set of flags will be used.
# Only set this if you know that you have a good reason to.
#
# Valid flags include:
#  * GSS_C_MUTUAL_FLAG
#  * GSS_C_REPLAY_FLAG
#  * GSS_C_SEQUENCE_FLAG
#  * GSS_C_CONF_FLAG
#  * GSS_C_INTEG_FLAG
#
# For a full list of flags, consult the GSSAPI documentation,
# though note that only the flags listed above are supported
# on Windows.
proc enableKerberos {service mech flags}

# Enables TLS/SSL encryption, and optionally authentication.
#
# "version" specifies the TLS/SSL protocol version that the
# client will attempt to use.  Valid values include SSL2,
# SSL3, TLS1, TLS1.1, TLS1.2 or any more recent version of
# TLS, as supported by and enabled in the underlying TLS/SSL
# library.  If left blank or empty then the highest supported
# version will be negotiated.
#
# "cert" is the file name of the certificate chain file to
# send to the SQL Relay server.  This is only necessary if
# the SQL Relay server is configured to authenticate and
# authorize clients by certificate.
#
# If "cert" contains a password-protected private key, then
# "password" may be supplied to access it.  If the private
# key is not password-protected, then this argument is
# ignored, and may be left empty or NULL.
#
# "ciphers" is a list of ciphers to allow.  Ciphers may be
# separated by spaces, commas, or colons.  If "ciphers" is
# empty or NULL then a default set is used.  Only set this if
# you know that you have a good reason to.
#
# For a list of valid ciphers on Linux/Unix platforms, see:
#     man ciphers
#
# For a list of valid ciphers on Windows platforms, see:
#     https://msdn.microsoft.com/en-us/library/windows/desktop/aa375549%28v=vs.85%29.aspx
# On Windows platforms, the ciphers (alg_id's) should omit
# CALG_ and may be given with underscores or dashes.
# For example: 3DES_112
#
# "validate" indicates whether to validate the SQL Relay's
# server certificate, and may be set to one of the following:
#     "no" - Don't validate the server's certificate.
#     "ca" - Validate that the server's certificate was
#            signed by a trusted certificate authority.
#     "ca+host" - Perform "ca" validation and also validate
#            that one of the subject altenate names (or the
#            common name if no SANs are present) in the
#            certificate matches the host parameter.
#            (Falls back to "ca" validation when a unix
#            socket is used.)
#     "ca+domain" - Perform "ca" validation and also validate
#            that the domain name of one of the subject
#            alternate names (or the common name if no SANs
#            are present) in the certificate matches the
#            domain name of the host parameter.  (Falls back
#            to "ca" validation when a unix socket is used.)
#
# "ca" is the location of a certificate authority file to
# use, in addition to the system's root certificates, when
# validating the SQL Relay server's certificate.  This is
# useful if the SQL Relay server's certificate is self-signed.
#
# On Windows, "ca" must be a file name.
#
# On non-Windows systems, "ca" can be either a file or
# directory name.  If it is a directory name, then all
# certificate authority files found in that directory will be
# used.  If it a file name, then only that file will be used.
#
#
# Note that the supported "cert" and "ca" file formats may
# vary between platforms.  A variety of file formats are
# generally supported on Linux/Unix platfoms (.pem, .pfx,
# etc.) but only the .pfx format is currently supported on
# Windows.
proc enableTls {version cert password ciphers validate ca depth}

# Disables encryption.
proc disableEncryption {}



# Ends the session.
proc endSession {} 

# Disconnects this connection from the current
# session but leaves the session open so
# that another connection can connect to it
# using resumeSession().
proc suspendSession {}

# Returns the inet port that the connection is
# communicating over. This parameter may be
# passed to another connection for use in
# the resumeSession() method.
# Note: The value this method returns is only
# valid after a call to suspendSession().
proc getConnectionPort {}

# Returns the unix socket that the connection
# is communicating over. This parameter may be
# passed to another connection for use in
# the resumeSession() method.
# Note: The value this method returns is only
# valid after a call to suspendSession().
proc getConnectionSocket {}

# Resumes a session previously left open
# using suspendSession().
# Returns true on success and false on failure.
proc resumeSession {port socket}



# Returns true if the database is up and false
# if it's down.
proc ping {} 

# Returns the type of database:
# oracle, postgresql, mysql, etc.
proc identify {} 

# Returns the version of the database
proc dbVersion {} 

# Returns the host name of the database
proc dbHostName {} 

# Returns the ip address of the database
proc dbIpAddress {} 

# Returns the version of the sqlrelay server software.
proc serverVersion {} 

# Returns the version of the sqlrelay client software.
proc clientVersion {} 

# Returns a string representing the bind variable format used
# by the database.  For example:
#
# ?  - database uses a ? to represent a bind variable
# @* - database uses a @ followed by any characters to
#      represent a bind variable
# $1 - database uses a $ followed by a number to represent a
#      bind variable
# :* - database uses a : followed by any characters to
#      represent a bind variable
proc bindFormat {} 

# Returns a string representing the format of the sequence
# nextval command used in the database.  The format will
# contain a %s in place of the sequence name.  For example:
#
# (nextval for %s)
# next value for %s
# nextval('%s')
# %s.nextval
#
# Returns an empty string if the database does not support
# sequences.
proc nextvalFormat {} 



# Sets the current database (catalog) to "database"
proc selectDatabase {database}

# Returns the database (catalog) that is currently in use.
proc getCurrentDatabase {}

# Sets the current schema to "schema"
proc selectSchema {schema}

# Returns the schema that is currently in use.
proc getCurrentSchema {}



# Returns the value of the autoincrement
# column for the last insert
proc getLastInsertId {} 



# Instructs the database to perform a commit
# after every successful query.
proc autoCommitOn {} 

# Instructs the database to wait for the 
# client to tell it when to commit.
proc autoCommitOff {} 



# Begins a transaction.  Returns true if the begin
# succeeded, false if it failed.  If the database
# automatically begins a new transaction when a
# commit or rollback is issued then this doesn't
# do anything unless SQL Relay is faking transaction
# blocks.
proc begin {} 

# Commits a transaction.  Returns true if the commit
# succeeded, false if it failed.
proc commit {}

# Rolls back a transaction.  Returns true if the rollback
# succeeded, false if it failed.
proc rollback {}


# Sets the isolation level to "isolationlevel", the
# database-secific isolation level.  Returns true if setting
# the isolation level succeeded, false if it failed.
proc setIsolationLevel {isolationlevel}

# Returns the database-specific isolation level, "unknown"
# if the isolation level is unknown, or NULL if an error
# occurred.
proc getIsolationLevel {}

# Returns the value of the specified database "feature".
#
#  Valid features include:
#  * "all_procedures_are_callable"
#  * "all_tables_are_selectable"
#  * "auto_commit_failure_closes_all_result_sets"
#  * "catalog_separator"
#  * "catalog_term"
#  * "collation_seq"
#  * "data_definition_causes_transaction_commit"
#  * "data_definition_ignored_in_transactions"
#  * "default_isolation_level"
#  * "deletes_are_detected"
#  * "does_max_row_size_include_blobs"
#  * "extra_name_characters"
#  * "generated_key_always_returned"
#  * "identifier_quote_string"
#  * "index_keywords"
#  * "info_schema_views"
#  * "inserts_are_detected"
#  * "is_catalog_at_start"
#  * "is_read_only"
#  * "locators_update_copy"
#  * "max_binary_literal_length"
#  * "max_catalog_name_length"
#  * "max_char_literal_length"
#  * "max_column_name_length"
#  * "max_columns_in_group_by"
#  * "max_columns_in_index"
#  * "max_columns_in_order_by"
#  * "max_columns_in_select"
#  * "max_columns_in_table"
#  * "max_connections"
#  * "max_cursor_name_length"
#  * "max_identifier_length"
#  * "max_index_length"
#  * "max_procedure_name_length"
#  * "max_row_size"
#  * "max_schema_name_length"
#  * "max_statement_length"
#  * "max_statements"
#  * "max_table_name_length"
#  * "max_tables_in_select"
#  * "max_user_name_length"
#  * "need_long_data_length"
#  * "null_plus_non_null_is_null"
#  * "nulls_are_sorted_at_end"
#  * "nulls_are_sorted_at_start"
#  * "nulls_are_sorted_high"
#  * "nulls_are_sorted_low"
#  * "numeric_functions"
#  * "others_deletes_are_visible"
#  * "others_inserts_are_visible"
#  * "others_updates_are_visible"
#  * "own_deletes_are_visible"
#  * "own_inserts_are_visible"
#  * "own_updates_are_visible"
#  * "procedure_term"
#  * "result_set_holdability"
#  * "row_id_lifetime"
#  * "schema_term"
#  * "search_string_escape"
#  * "sql_keywords"
#  * "sql_state_type"
#  * "stores_lower_case_identifiers"
#  * "stores_lower_case_quoted_identifiers"
#  * "stores_mixed_case_identifiers"
#  * "stores_mixed_case_quoted_identifiers"
#  * "stores_upper_case_identifiers"
#  * "stores_upper_case_quoted_identifiers"
#  * "string_functions"
#  * "supported_foreign_key_delete_rules"
#  * "supported_foreign_key_update_rules"
#  * "supported_predicates"
#  * "supported_relational_join_operators"
#  * "supported_row_value_constructor_expressions"
#  * "supported_value_expressions"
#  * "supports_aggregate_functions"
#  * "supports_alter_domain"
#  * "supports_alter_table_with_add_column"
#  * "supports_alter_table_with_drop_column"
#  * "supports_ansi92_entry_level_sql"
#  * "supports_ansi92_full_sql"
#  * "supports_ansi92_intermediate_sql"
#  * "supports_batch_updates"
#  * "supports_catalogs_in_data_manipulation"
#  * "supports_catalogs_in_index_definitions"
#  * "supports_catalogs_in_privilege_definitions"
#  * "supports_catalogs_in_procedure_calls"
#  * "supports_catalogs_in_table_definitions"
#  * "supports_column_aliasing"
#  * "supports_convert"
#  * "supports_core_sql_grammar"
#  * "supports_correlated_subqueries"
#  * "supports_create_assertion"
#  * "supports_create_character_set"
#  * "supports_create_collation"
#  * "supports_create_domain"
#  * "supports_create_schema"
#  * "supports_create_table"
#  * "supports_create_translation"
#  * "supports_create_view"
#  * "supports_data_definition_and_data_manipulation_transactions"
#  * "supports_data_manipulation_transactions_only"
#  * "supports_ddl_index"
#  * "supports_describe_parameter"
#  * "supports_different_table_correlation_names"
#  * "supports_drop_assertion"
#  * "supports_drop_character_set"
#  * "supports_drop_collation"
#  * "supports_drop_domain"
#  * "supports_drop_schema"
#  * "supports_drop_table"
#  * "supports_drop_translation"
#  * "supports_drop_view"
#  * "supports_expressions_in_order_by"
#  * "supports_extended_sql_grammar"
#  * "supports_full_outer_joins"
#  * "supports_get_generated_keys"
#  * "supports_grant"
#  * "supports_group_by"
#  * "supports_group_by_beyond_select"
#  * "supports_group_by_unrelated"
#  * "supports_insert_statement"
#  * "supports_integrity_enhancement_facility"
#  * "supports_like_escape_clause"
#  * "supports_limited_outer_joins"
#  * "supports_lock_types"
#  * "supports_minimum_sql_grammar"
#  * "supports_mixed_case_identifiers"
#  * "supports_mixed_case_quoted_identifiers"
#  * "supports_multiple_result_sets"
#  * "supports_multiple_transactions"
#  * "supports_named_parameters"
#  * "supports_non_nullable_columns"
#  * "supports_open_cursors_across_commit"
#  * "supports_open_cursors_across_rollback"
#  * "supports_open_statements_across_commit"
#  * "supports_open_statements_across_rollback"
#  * "supports_order_by_unrelated"
#  * "supports_outer_joins"
#  * "supports_positioned_delete"
#  * "supports_positioned_update"
#  * "supports_result_set_concurrency"
#  * "supports_result_set_holdability"
#  * "supports_result_set_type"
#  * "supports_revoke"
#  * "supports_savepoints"
#  * "supports_schemas_in_data_manipulation"
#  * "supports_schemas_in_index_definitions"
#  * "supports_schemas_in_privilege_definitions"
#  * "supports_schemas_in_procedure_calls"
#  * "supports_schemas_in_table_definitions"
#  * "supports_select_for_update"
#  * "supports_stored_functions_using_call_syntax"
#  * "supports_stored_procedures"
#  * "supports_subqueries_in_comparisons"
#  * "supports_subqueries_in_exists"
#  * "supports_subqueries_in_ins"
#  * "supports_subqueries_in_quantifieds"
#  * "supports_table_correlation_names"
#  * "supports_transaction_isolation_level"
#  * "supports_transactions"
#  * "supports_union"
#  * "supports_union_all"
#  * "system_functions"
#  * "table_term"
#  * "time_date_add_intervals"
#  * "time_date_diff_intervals"
#  * "time_date_functions"
#  * "time_date_literals"
#  * "updates_are_detected"
#  * "uses_local_file_per_table"
#  * "uses_local_files"
#
#  Returns the value of the feature as a string, or NULL if
#  an error occurred or an invalid feature was requested.
proc getDatabaseFeature {feature}



# If an operation failed and generated an
# error, the error message is available here.
# If there is no error then this method
# returns NULL.
proc errorMessage {}



# If an operation failed and generated an
# error, the error number is available here.
# If there is no error then this method 
# returns 0.
proc errorNumber {}



# Causes verbose debugging information to be
# sent to standard output.  Another way to do
# this is to start a query with "-- debug\n".
# Yet another way is to set the environment
# variable SQLR_CLIENT_DEBUG to "ON"
proc debugOn {}

# Turns debugging off.
proc debugOff {} 

# Returns false if debugging is off and true
# if debugging is on.
proc getDebug {} 

# Allows you to specify a file to write debug to.
# Setting "filename" to NULL or an empty string causes debug
# to be written to standard output (the default).
proc setDebugFile {debugfilename}

# Allows you to set a string that will be passed to the
# server and ultimately included in server-side logging
# along with queries that were run by this instance of
# the client.
proc setClientInfo {clientinfo} 

# Returns the string that was set by setClientInfo().
proc getClientInfo {} 

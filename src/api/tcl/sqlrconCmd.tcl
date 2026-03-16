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

# Sets the current catalog to "catalog"
proc selectCatalog {catalog}

# Returns the catalog that is currently in use.
proc getCurrentCatalog {}

# Sets the current schema to "schema"
proc selectSchema {schema}

# Returns the schema that is currently in use.
proc getCurrentSchema {}

# Returns true if the backend database equates "database" with
# "schema", and false if it equates "database" with "catalog".
proc getDatabaseIsSchema {}



# Returns the value of the autoincrement
# column for the last insert
proc getLastInsertId {} 



# Instructs the database to perform a commit
# after every successful query.
proc autoCommitOn {} 

# Instructs the database to wait for the 
# client to tell it when to commit.
proc autoCommitOff {}

# Returns true if auto-commit is currently on,
# false otherwise.
proc getAutoCommit {}



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
#  * aggregate_functions
#   * list - ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM
#  * all_procedures_are_callable
#   * true/false
#  * all_tables_are_selectable
#   * true/false
#  * alter_domain_clauses
#   * list - ADD_DOMAIN_CONSTRAINT,ADD_DOMAIN_DEFAULT,...
#  * alter_table_operations
#   * list - ADD_COLUMN,DROP_COLUMN
#  * ansi92_sql_levels
#   * list - ENTRY_LEVEL,FULL,INTERMEDIATE
#  * auto_commit_failure_closes_all_result_sets
#   * true/false
#  * batch_operations
#   * list - SELECT_EXPLICIT,ROW_COUNT_EXPLICIT,SELECT_PROC,ROW_COUNT_PROC
#  * batch_row_counts
#   * list - PROCEDURES,EXPLICIT,ROLLED_UP
#  * catalog_separator
#   * string
#  * catalog_term
#   * string
#  * catalog_usage
#   * list - DATA_MANIPULATION,INDEX_DEFINITIONS,...
#  * collation_seq
#   * string
#  * create_assertion_clauses
#   * list - CREATE_ASSERTION,CONSTRAINT_INITIALLY_DEFERRED,...
#  * create_character_set_clauses
#   * list - CREATE_CHARACTER_SET,COLLATE_CLAUSE,...
#  * create_collation_clauses
#   * list - CREATE_COLLATION
#  * create_domain_clauses
#   * list - CREATE_DOMAIN,CONSTRAINT_NAME_DEFINITION,...
#  * create_schema_clauses
#   * list - CREATE_SCHEMA,AUTHORIZATION,DEFAULT_CHARACTER_SET
#  * create_table_clauses
#   * list - CREATE_TABLE,TABLE_CONSTRAINT,...
#  * create_translation_clauses
#   * list - CREATE_TRANSLATION
#  * create_view_clauses
#   * list - CREATE_VIEW,CHECK_OPTION,CASCADED,LOCAL
#  * data_definition_transaction_behavior
#   * list - CAUSES_COMMIT,IGNORED_IN_TRANSACTIONS
#  * ddl_index_operations
#   * list - CREATE_INDEX,DROP_INDEX
#   * string
#  * default_result_set_holdability
#   * string
#  * deletes_are_detected
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * does_max_row_size_include_blobs
#   * true/false
#  * drop_assertion_clauses
#   * list - DROP_ASSERTION
#  * drop_character_set_clauses
#   * list - DROP_CHARACTER_SET
#  * drop_collation_clauses
#   * list - DROP_COLLATION
#  * drop_domain_clauses
#   * list - DROP_DOMAIN,CASCADE,RESTRICT
#  * drop_schema_clauses
#   * list - DROP_SCHEMA,CASCADE,RESTRICT
#  * drop_table_clauses
#   * list - DROP_TABLE,CASCADE,RESTRICT
#  * drop_translation_clauses
#   * list - DROP_TRANSLATION
#  * drop_view_clauses
#   * list - DROP_VIEW,CASCADE,RESTRICT
#  * extra_name_characters
#   * string
#  * foreign_key_delete_rules
#   * list - CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL
#  * foreign_key_update_rules
#   * list - CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL
#  * forward_only_cursor_attributes
#   * list - NEXT,ABSOLUTE,RELATIVE,BOOKMARK,...
#  * generated_key_always_returned
#   * true/false
#  * grant_clauses
#   * list - DELETE_TABLE,INSERT_COLUMN,INSERT_TABLE,...
#  * group_by_clauses
#   * list - BASIC,BEYOND_SELECT,UNRELATED
#  * identifier_case_storage
#   * list - LOWER,MIXED,UPPER
#  * identifier_quote_string
#   * string
#  * index_keywords
#   * list - ASC,DESC
#  * info_schema_views
#   * list - ASSERTIONS,CHARACTER_SETS,CHECK_CONSTRAINTS,...
#  * insert_operations
#   * list - INSERT_LITERALS,INSERT_SEARCHED,SELECT_INTO
#  * inserts_are_detected
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * is_catalog_at_start
#   * true/false
#  * isolation_levels
#   * list - READ_UNCOMMITTED,READ_COMMITTED,...
#  * local_file_usage
#   * list - LOCAL_FILE_PER_TABLE,LOCAL_FILES
#  * locators_update_copy
#   * true/false
#  * lock_types
#   * list - NO_CHANGE,EXCLUSIVE,UNLOCK
#  * max_binary_literal_length
#   * number
#  * max_catalog_name_length
#   * number
#  * max_char_literal_length
#   * number
#  * max_column_name_length
#   * number
#  * max_columns_in_group_by
#   * number
#  * max_columns_in_index
#   * number
#  * max_columns_in_order_by
#   * number
#  * max_columns_in_select
#   * number
#  * max_columns_in_table
#   * number
#  * max_connections
#   * number
#  * max_cursor_name_length
#   * number
#  * max_identifier_length
#   * number
#  * max_index_length
#   * number
#  * max_procedure_name_length
#   * number
#  * max_row_size
#   * number
#  * max_schema_name_length
#   * number
#  * max_statement_length
#   * number
#  * max_statements
#   * number
#  * max_table_name_length
#   * number
#  * max_tables_in_select
#   * number
#  * max_user_name_length
#   * number
#  * mixed_case_identifiers
#   * list - IDENTIFIERS,QUOTED_IDENTIFIERS
#  * need_long_data_length
#   * true/false
#  * null_plus_non_null_is_null
#   * true/false
#  * null_sort_order
#   * list - AT_END,AT_START,HIGH,LOW
#  * numeric_functions
#   * list - ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,COT,...
#  * open_cursors_across
#   * list - COMMIT,ROLLBACK
#  * open_statements_across
#   * list - COMMIT,ROLLBACK
#  * others_deletes_are_visible
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * others_inserts_are_visible
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * others_updates_are_visible
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * outer_joins
#   * list - BASIC,FULL,LIMITED
#  * own_deletes_are_visible
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * own_inserts_are_visible
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * own_updates_are_visible
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * predicates
#   * list - BETWEEN,COMPARISON,EXISTS,IN,ISNOTNULL,ISNULL,...
#  * procedure_term
#   * string
#  * quoted_identifier_case_storage
#   * list - LOWER,MIXED,UPPER
#  * relational_join_operators
#   * list - CORRESPONDING_CLAUSE,CROSS_JOIN,EXCEPT_JOIN,...
#  * result_set_concurrencies
#   * list - FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,...
#  * result_set_holdabilities
#   * list - CLOSE_CURSORS_AT_COMMIT,HOLD_CURSORS_OVER_COMMIT
#  * result_set_types
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * revoke_clauses
#   * list - CASCADE,DELETE_TABLE,GRANT_OPTION_FOR,...
#  * row_id_lifetime
#   * string
#  * row_value_constructor_expressions
#   * list - VALUE_EXPRESSION,NULL,DEFAULT,ROW_SUBQUERY
#  * schema_term
#   * string
#  * schema_usage
#   * list - DATA_MANIPULATION,INDEX_DEFINITIONS,...
#  * scroll_concurrencies
#   * list - READ_ONLY,LOCK,OPT_ROWVER,OPT_VALUES
#  * search_string_escape
#   * string
#  * sql_grammar_levels
#   * list - CORE,EXTENDED,MINIMUM
#  * sql_keywords
#   * list - ACCESS,ADD,ALTER,AUDIT,CLUSTER,COLUMN,COMMENT,...
#  * sql_state_type
#   * number
#  * static_cursor_attributes
#   * list - NEXT,ABSOLUTE,RELATIVE,BOOKMARK,...
#  * stored_programs
#   * list - FUNCTIONS,PROCEDURES
#  * string_functions
#   * list - CONCAT,INSERT,LEFT,LTRIM,LENGTH,LOCATE,LCASE,...
#  * subquery_usage
#   * list - COMPARISONS,EXISTS,INS,QUANTIFIEDS
#  * supports_batch_updates
#   * true/false
#  * supports_column_aliasing
#   * true/false
#  * supports_convert
#   * true/false
#  * supports_correlated_subqueries
#   * true/false
#  * supports_describe_parameter
#   * true/false
#  * supports_expressions_in_order_by
#   * true/false
#  * supports_get_generated_keys
#   * true/false
#  * supports_integrity_enhancement_facility
#   * true/false
#  * supports_like_escape_clause
#   * true/false
#  * supports_multiple_result_sets
#   * true/false
#  * supports_multiple_transactions
#   * true/false
#  * supports_named_parameters
#   * true/false
#  * supports_non_nullable_columns
#   * true/false
#  * supports_order_by_unrelated
#   * true/false
#  * supports_savepoints
#   * true/false
#  * supports_select_for_update
#   * true/false
#  * supports_transactions
#   * true/false
#  * system_functions
#   * list - USER,DBNAME,IFNULL
#  * table_correlation_names
#   * list - BASIC,DIFFERENT
#  * table_term
#   * string
#  * time_date_add_intervals
#   * list - FRAC_SECOND,SECOND,MINUTE,HOUR,DAY,WEEK,MONTH,...
#  * time_date_diff_intervals
#   * list - FRAC_SECOND,SECOND,MINUTE,HOUR,DAY,WEEK,MONTH,...
#  * time_date_functions
#   * list - NOW,CURDATE,DAYOFMONTH,DAYOFWEEK,DAYOFYEAR,...
#  * time_date_literals
#   * list - DATE,TIME,TIMESTAMP,INTERVAL_YEAR,...
#  * transaction_ddl_dml
#   * list - DDL_AND_DML,DML_ONLY
#  * union_clauses
#   * list - UNION,UNION_ALL
#  * updates_are_detected
#   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
#  * value_expressions
#   * list - CASE,CAST,COALESCE,NULLIF
#  * where_current_of_operations
#   * list - DELETE,UPDATE
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

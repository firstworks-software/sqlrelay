<?php

/** 
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
 *  If the "socket" parameter is neither NULL nor "" then an attempt will be
 *  made to connect through it before attempting to connect to "server" on
 *  "port".
 *  If it is NULL or "" then no attempt will be made to connect through the
 *  socket.*/
function sqlrcon_alloc($server, $port, $socket, $user, $password, $retrytime, $tries){}

/** 
 *  Disconnects and ends the session if it hasn't been ended already. */
function sqlrcon_free($sqlrconref){}



/** 
 *  Sets the server connect timeout in seconds and
 *  microseconds.  Setting either parameter to -1 disables the
 *  timeout.  You can also set this timeout using the
 *  SQLR_CLIENT_CONNECT_TIMEOUT environment variable. */
function sqlrcon_setConnectTimeout($sqlrconref, $timeoutsec, $timeoutusec){}

/**
 *  Gets the server connect timeout in seconds. */
function sqlrcon_getConnectTimeoutSeconds($sqlrconref){}

/**
 *  Gets the server connect timeout in microseconds. */
function sqlrcon_getConnectTimeoutMicroseconds($sqlrconref){}

/**
 *  Sets the response timeout (for queries, commits, rollbacks,
 *  pings, etc.) in seconds and microseconds.  Setting either
 *  parameter to -1 disables the timeout.  You can also set
 *  this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
 *  environment variable. */
function sqlrcon_setResponseTimeout($sqlrconref, $timeoutsec, $timeoutusec){}

/**
 *  Gets the response timeout in seconds. */
function sqlrcon_getResponseTimeoutSeconds($sqlrconref){}

/**
 *  Gets the response timeout in microseconds. */
function sqlrcon_getResponseTimeoutMicroseconds($sqlrconref){}

/**
 *  Sets which delimiters are used to identify bind variables
 *  in countBindVariables() and validateBinds().  Valid
 *  delimiters include ?,:,@, and $.  Defaults to "?:@$" */
function sqlrcon_setBindVariableDelimiters($sqlrconref, $delimiters){}

/**
 *  Returns true if question marks (?) are considered to be
 *  valid bind variable delimiters. */
function sqlrcon_getBindVariableDelimiterQuestionMarkSupported($sqlrconref){}

/**
 *  Returns true if colons (:) are considered to be
 *  valid bind variable delimiters. */
function sqlrcon_getBindVariableDelimiterColonSupported($sqlrconref){}

/**
 *  Returns true if at-signs (@) are considered to be
 *  valid bind variable delimiters. */
function sqlrcon_getBindVariableDelimiterAtSignSupported($sqlrconref){}

/**
 *  Returns true if dollar signs ($) are considered to be
 *  valid bind variable delimiters. */
function sqlrcon_getBindVariableDelimiterDollarSignSupported($sqlrconref){}

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
function sqlrcon_enableKerberos($sqlrconref, $service, $mech, $flags){}

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
function sqlrcon_enableTls($sqlrconref, $version, $cert, $password, $ciphers, $validate, $ca, $depth){}

/** Disables encryption. */
function sqlrcon_disableEncryption($sqlrconref){}

/** 
 *  Ends the session. */
function sqlrcon_endSession($sqlrconref){}

/** 
 *  Disconnects this connection from the current session but leaves the session
 *  open so that another connection can connect to it using
 *  sqlrcon_resumeSession(). */
function sqlrcon_suspendSession($sqlrconref){}

/**
 *  Returns the inet port that the connection is communicating over.  This
 *  parameter may be passed to another connection for use in the
 *  resumeSession() method.  Note: The value this method returns
 *  is only valid after a call to suspendSession(). */
function sqlrcon_getConnectionPort($sqlrconref){}

/**
 *  Returns the unix socket that the connection is communicating over.  This
 *  parameter may be passed to another connection for use in the
 *  resumeSession() method.  Note: The value this method returns
 *  is only valid after a call to suspendSession(). */
function sqlrcon_getConnectionSocket($sqlrconref){}

/** 
 *  Resumes a session previously left open using sqlrcon_suspendSession().
 *  Returns 1 on success and 0 on failure. */
function sqlrcon_resumeSession($sqlrconref, $port, $socket){}



/** 
 *  Returns 1 if the database is up and 0 if it's down. */
function sqlrcon_ping($sqlrconref){}

/** 
 *  Returns the type of database: oracle, postgresql, mysql, etc. */
function sqlrcon_identify($sqlrconref){}

/** 
 *  Returns the version of the database */
function sqlrcon_dbVersion($sqlrconref){}

/** 
 *  Returns the host name of the database */
function sqlrcon_dbHostName($sqlrconref){}

/** 
 *  Returns the ip address of the database */
function sqlrcon_dbIpAddress($sqlrconref){}

/** 
 *  Returns the version of the sqlrelay server software. */
function sqlrcon_serverVersion($sqlrconref){}

/** 
 *  Returns the version of the sqlrelay client software. */
function sqlrcon_clientVersion($sqlrconref){}

/** 
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
function sqlrcon_bindFormat($sqlrconref){}

/**
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
function sqlrcon_nextvalFormat($sqlrconref){}



/** 
 *  Sets the current database to "database".
 *
 *  May set the current catalog or schema, depending on
 *  whether the backend database equates "database" with
 *  catalog or schema.
 *
 *  See getDatabaseIsSchema(). */
function sqlrcon_selectDatabase($sqlrconref, $database){}

/**
 *  Returns the database that is currently in use.
 *
 *  May return the current catalog or schema, depending on
 *  whether the backend database equates "database" with
 *  catalog or schema.
 *
 *  See getDatabaseIsSchema(). */
function sqlrcon_getCurrentDatabase($sqlrconref){}

/**
 *  Sets the current catalog to "catalog" */
function sqlrcon_selectCatalog($sqlrconref, $catalog){}

/**
 *  Returns the catalog that is currently in use. */
function sqlrcon_getCurrentCatalog($sqlrconref){}

/**
 *  Sets the current schema to "schema" */
function sqlrcon_selectSchema($sqlrconref, $schema){}

/**
 *  Returns the schema that is currently in use. */
function sqlrcon_getCurrentSchema($sqlrconref){}

/**
 *  Returns true if the backend database equates "database" with
 *  "schema", and false if it equates "database" with "catalog". */
function sqlrcon_getDatabaseIsSchema($sqlrconref){}



/**
 *  Returns the user that sqlrelay is currently logged in to
 *  the database as, or NULL if no user could be determined
 *  or if an error occurred. */
function sqlrcon_getCurrentUser($sqlrconref){}

/**
 *  Returns the value of the autoincrement column for the last insert */
function sqlrcon_getLastInsertId($sqlrconref){}



/** 
 *  Instructs the database to perform a commit after every successful query. */
function sqlrcon_autoCommitOn($sqlrconref){}

/**
 *  Instructs the database to wait for the client to tell it when to commit. */
function sqlrcon_autoCommitOff($sqlrconref){}

/**
 *  Returns 1 if auto-commit is currently on, 0 otherwise. */
function sqlrcon_getAutoCommit($sqlrconref){}


/**
 *  Begins a transaction.  Returns 1 if the begin succeeded, 0 if it failed.
 *  If the database automatically begins a new transaction when a commit or
 *  rollback is issued then this doesn't do anything unless SQL Relay is faking
 *  transaction blocks. */
function sqlrcon_begin($sqlrconref){}

/**
 *  Commits a transaction.  Returns 1 if the commit succeeded, 0 if it
 *  failed. */
function sqlrcon_commit($sqlrconref){}

/**
 *  Rolls back a transaction.  Returns 1 if the rollback succeeded, 0 if it
 *  failed. */
function sqlrcon_rollback($sqlrconref){}

/**
 *  Returns 1 if the session is currently inside a transaction,
 *  0 otherwise. */
function sqlrcon_getInTransaction($sqlrconref){}


/** Returns the database's native transaction model.  See
 *  sqlrcon_setTranscationModel() for a list of potential return values.
 *  Returns NULL if an error occurred. */
function sqlrcon_getDefaultTransactionModel($sqlrconref){}

/**
 *  Sets the current transaction model to "txmodel" which should be one of:
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
 *      * while in a transaction, autocommit on/off take effect at next
 *        commit/rollback
 *  * "explicit-error"
 *      * not in a transaction when the session begins
 *      * begin required to start a new transaction
 *      * commit/rollback does not start a new transcaction
 *      * while in a transaction, autocommit on/off throw error
 *
 *  Returns 1 on success and 0 on failure. */
function sqlrcon_setTransactionModel($sqlrconref, $txmodel){}

/**
 *  Returns the current transaction model.  See sqlrcon_setTranscationModel()
 *  for a list of potential return values.  Returns NULL if an error
 *  occurred. */
function sqlrcon_getTransactionModel($sqlrconref){}

/** Returns the database-specific default isolation level,
 *  or NULL if an error occurred. */
function sqlrcon_getDefaultIsolationLevel($sqlrconref){}

/**
 *  Sets the isolation level to "isolationlevel", the database-secific
 *  isolation level.  Returns 1 if setting the isolation level succeeded,
 *  0 if it failed. */
function sqlrcon_setIsolationLevel($sqlrconref, $isolationlevel){}

/**
 *  Returns the database-specific isolation level, "unknown" if the isolation
 *  level is unknown, or NULL if an error occurred. */
function sqlrcon_getIsolationLevel($sqlrconref){}

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
 *   * list - SELECT_EXPLICIT,ROW_COUNT_EXPLICIT,SELECT_PROC,ROW_COUNT_PROC
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
function sqlrcon_getDatabaseFeature($sqlrconref,$feature){}


/**
 *  If an operation failed and generated an error, the error message is
 *  available here.  If there is no error then this method returns NULL. */
function sqlrcon_errorMessage($sqlrconref){}


/**
 *  If an operation failed and generated an error, the error number is
 *  available here.  If there is no error then this method returns 0. */
function sqlrcon_errorNumber($sqlrconref){}



/** 
 *  Causes verbose debugging information to be sent to standard output.
 *  Another way to do this is to start a query with "-- debug\n".
 *  Yet another way is to set the environment variable SQLR_CLIENT_DEBUG
 *  to "ON" */
function sqlrcon_debugOn($sqlrconref){}

/** 
 *  Turns debugging off. */
function sqlrcon_debugOff($sqlrconref){}

/** 
 *  Returns 0 if debugging is off and 1 if debugging is on. */
function sqlrcon_getDebug($sqlrconref){}

/** Allows you to specify a file to write debug to.
 *  Setting "filename" to NULL or an empty string causes debug
 *  to be written to standard output (the default). */
function sqlrcon_setDebugFile($sqlrconref, $filename){}

/** Allows you to set a string that will be passed to the
 *  server and ultimately included in server-side logging
 *  along with queries that were run by this instance of
 *  the client. */
function sqlrcon_setClientInfo($sqlrconref, $clientinfo){}

/** Returns the string that was set by setClientInfo(). */
function sqlrcon_getClientInfo($sqlrconref){}





/** 
 *  Creates a cursor to run queries and fetch
 *  result sets using connection "sqlrconref" */
function sqlrcur_alloc($sqlrconref){}

/** 
 *  Destroys the cursor and cleans up all associated result set data. */
function sqlrcur_free($sqlrcurref){}



/** 
 *  Sets the number of rows of the result set to buffer at a time.
 *  0 (the default) means buffer the entire result set. */
function sqlrcur_setResultSetBufferSize($sqlrcurref, $rows){}

/** 
 *  Returns the number of result set rows that will be buffered at a time or
 *  0 for the entire result set. */
function sqlrcur_getResultSetBufferSize($sqlrcurref){}



/** 
 *  Tells the server not to send any column info (names, types, sizes).  If
 *  you don't need that info, you should call this function to improve
 *  performance. */
function sqlrcur_dontGetColumnInfo($sqlrcurref){}

/** 
 *  Tells the server to send column info. */
function sqlrcur_getColumnInfo($sqlrcurref){}



/** 
 *  Columns names are returned in the same case as they are defined in the
 *  database.  This is the default. */
function sqlrcur_mixedCaseColumnNames($sqlrcurref){}

/** 
 *  Columns names are converted to upper case. */
function sqlrcur_upperCaseColumnNames($sqlrcurref){}

/** 
 *  Columns names are converted to lower case. */
function sqlrcur_lowerCaseColumnNames($sqlrcurref){}



/** 
 *  Sets query caching on.  Future queries will be cached to the
 *  file "filename".
 * 
 *  A default time-to-live of 10 minutes is also set.
 * 
 *  Note that once sqlrcur_cacheToFile() is called, the result sets of all
 *  future queries will be cached to that file until another call to
 *  sqlrcur_cacheToFile() changes which file to cache to or a call to
 *  sqlrcur_cacheOff() turns off caching. */
function sqlrcur_cacheToFile($sqlrcurref, $filename){}

/** 
 *  Sets the time-to-live for cached result sets. The sqlr-cachemanger will
 *  remove each cached result set "ttl" seconds after it's created, provided
 *  it's scanning the directory containing the cache files. */
function sqlrcur_setCacheTtl($sqlrcurref, $ttl){}

/** 
 *  Returns the name of the file containing the
 *  cached result set. */
function sqlrcur_getCacheFileName($sqlrcurref){}

/** 
 *  Sets query caching off. */
function sqlrcur_cacheOff($sqlrcurref){}



/**
 *  Generates a result set containing
 *  databases that match the pattern "wild".
 *
 *  The result set will contain the following columns:
 *  * Database
 *
 *  If "wild" is empty or NULL then a result set containing
 *  all databases will be returned.
 *
 *  If SQL Relay doesn't support getting a list of databases
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
function sqlrcur_getDatabaseList($sqlrcurref, $wild){}

function sqlrcur_getCatalogList($sqlrcurref, $wild){}

/**
 *  Generates a result set containing the
 *  tables in the current database and schema that match the
 *  pattern "wild".
 *
 *  The result set will contain the following columns:
 *  * Tables_in_xxx
 *
 *  If "wild" is empty or NULL then a result set containing
 *  all tables in the current database/schema will be returned.
 *
 *  If SQL Relay doesn't support getting a list of tables
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
function sqlrcur_getTableList($sqlrcurref, $wild){}

/**
 *  Generates a result set containing the
 *  columns of "table", which match the pattern "wild".
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
 *  If "wild" is empty or NULL then a list of all columns
 *  of "table" will be returned.
 *
 *  If SQL Relay doesn't support getting a list of columns
 *  for the current database backend (or the database doesn't)
 *  then an empty result set will be returned. */
function sqlrcur_getColumnList($sqlrcurref, $table, $wild){}



/** 
 *  Sends "query" directly and gets a result set. */
function sqlrcur_sendQuery($sqlrcurref, $query){}

/** 
 *  Sends "query" with length "length" directly and gets a result set. This
 *  function must be used if the query contains binary data. */
function sqlrcur_sendQueryWithLength($sqlrcurref, $query, $length){}

/** 
 *  Sends the query in file "path"/"filename" directly and gets a result set. */
function sqlrcur_sendFileQuery($sqlrcurref, $path, $filename){}



/** 
 *  Prepare to execute "query". */
function sqlrcur_prepareQuery($sqlrcurref, $query){}

/** 
 *  Prepare to execute "query" with length "length".  This function must be
 *  used if the query contains binary data. */
function sqlrcur_prepareQueryWithLength($sqlrcurref, $query, $length){}

/** 
 *  Prepare to execute the contents of "path"/"filename". */
function sqlrcur_prepareFileQuery($sqlrcurref, $path, $filename){}

/** 
 *  Defines a substitution variable.  The value may be a string,
 *  integer or decimal.  If it is a decimal, then precision and scale may
 *  also be specified */
function sqlrcur_substitution($sqlrcurref, $variable, $value, $precision, $scale){}

/** 
 *  Defines an array of substitution variables.  The values may be
 *  strings, integers or decimals.  If they are decimals, then precisions and
 *  scales may also be specified */
function sqlrcur_substitution($sqlrcurref, $variable, $value){}

/** 
 *  Defines an input bind variable.  The value may be a string,
 *  integer or decimal.  If the value is a decimal, then precision and scale may
 *  also be specified.  If you don't have the precision and scale then set them
 *  both to 0.  However in that case you may get unexpected rounding behavior
 *  if the server is faking binds. */
function sqlrcur_inputBind($sqlrcurref, $variable, $value, $precision, $scale){}

/** 
 *  Defines an input bind variables.  The values may be a strings,
 *  integers or decimals.  If they are a decimals, then precisions and
 *  scales may also be specified */
function sqlrcur_inputBind($sqlrcurref, $variable, $value, $precision, $scale){}

/**
 *  Defines a date input bind variable. */
function sqlrcur_inputBindDate($sqlrcurref, $variable, $year, $month, $day, $hour, $minute, $second, $microsecond, $tz, $isnegative){}

/**
 *  Defines a binary lob input bind variable. */
function sqlrcur_inputBindBlob($sqlrcurref, $variable, $value, $size){}

/** 
 *  Defines a character lob input bind variable. */
function sqlrcur_inputBindClob($sqlrcurref, $variable, $value, $size){}



/** 
 *  Defines a string output bind variable.
 *  "length" bytes will be reserved to store the value. */
function sqlrcur_defineOutputBindString($sqlrcurref, $variable, $length){}

/** 
 *  Defines an integer output bind variable. */
function sqlrcur_defineOutputBindInteger($sqlrcurref, $variable){}

/** 
 *  Defines a decimal output bind variable. */
function sqlrcur_defineOutputBindDouble($sqlrcurref, $variable){}

/**
 *  Defines a date output bind variable */
function sqlrcur_defineOutputBindDate($sqlrcurref, $variable){}

/**
 *  Defines a binary lob output bind variable */
function sqlrcur_defineOutputBindBlob($sqlrcurref, $variable){}

/** 
 *  Defines a character lob output bind variable */
function sqlrcur_defineOutputBindClob($sqlrcurref, $variable){}

/** 
 *  Defines a cursor output bind variable */
function sqlrcur_defineOutputBindCursor($sqlrcurref, $variable){}



/** 
 *  Clears all bind variables. */
function sqlrcur_clearBinds($sqlrcurref){}

/** 
 *  Parses the previously prepared query, counts the number of bind variables
 *  defined in it and returns that number. */
function sqlrcur_countBindVariables($sqlrcurref){}

/** 
 *  If you are binding to any variables that might not actually be in your
 *  query, call this to ensure that the database won't try to bind them unless
 *  they really are in the query.  There is a performance penalty for calling
 *  this function */
function sqlrcur_validateBinds($sqlrcurref){}

/** 
 *  Returns true if "variable" was a valid bind variable of the query. */
function sqlrcur_validBind($sqlrcurref, $variable){}



/** 
 *  Execute the query that was previously prepared and bound. */
function sqlrcur_executeQuery($sqlrcurref){}

/** 
 *  Fetch from a cursor that was returned as an output bind variable. */
function sqlrcur_fetchFromBindCursor($sqlrcurref){}



/** 
 *  Get the value stored in a previously defined
 *  string output bind variable. */
function sqlrcur_getOutputBindString($sqlrcurref, $variable){}

/** 
 *  Get the value stored in a previously defined
 *  integer output bind variable. */
function sqlrcur_getOutputBindInteger($sqlrcurref, $variable){}

/** 
 *  Get the value stored in a previously defined
 *  decimal output bind variable. */
function sqlrcur_getOutputBindDouble($sqlrcurref, $variable){}

/** 
 *  Get the value stored in a previously defined
 *  binary lob output bind variable. */
function sqlrcur_getOutputBindBlob($sqlrcurref, $variable){}

/** 
 *  Get the value stored in a previously defined
 *  character lob output bind variable. */
function sqlrcur_getOutputBindClob($sqlrcurref, $variable){}

/** 
 *  Get the length of the value stored in a previously
 *  defined output bind variable. */
function sqlrcur_getOutputBindLength($sqlrcurref, $variable){}

/**
 *  Get the cursor associated with a previously defined output bind variable. */
function sqlrcur_getOutputBindCursor($sqlrcurref, $variable){}

/**
 *  Get the year from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateYear($sqlrcurref, $variable){}

/**
 *  Get the month from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateMonth($sqlrcurref, $variable){}

/**
 *  Get the day from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateDay($sqlrcurref, $variable){}

/**
 *  Get the hour from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateHour($sqlrcurref, $variable){}

/**
 *  Get the minute from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateMinute($sqlrcurref, $variable){}

/**
 *  Get the second from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateSecond($sqlrcurref, $variable){}

/**
 *  Get the microsecond from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateMicrosecond($sqlrcurref, $variable){}

/**
 *  Get the time zone from a previously defined
 *  date output bind variable. */
function sqlrcur_getOutputBindDateTz($sqlrcurref, $variable){}

/**
 *  Get whether the value is negative from a
 *  previously defined date output bind variable. */
function sqlrcur_getOutputBindDateIsNegative($sqlrcurref, $variable){}



/**
 *  Opens a cached result set.  Returns 1 on success and 0 on failure. */
function sqlrcur_openCachedResultSet($sqlrcurref, $filename){}



/** 
 *  Returns the number of columns in the current result set. */
function sqlrcur_colCount($sqlrcurref){}

/**
 *  Returns the number of rows in the current result set (if the result set is
 *  being stepped through, this returns the number of rows processed so far). */
function sqlrcur_rowCount($sqlrcurref){}

/**
 *  Returns the total number of rows that will be returned in the result set.
 *  Not all databases support this call.  Don't use it for applications which
 *  are designed to be portable across databases.  0 is returned by databases
 *  which don't support this option. */
function sqlrcur_totalRows($sqlrcurref){}

/**
 *  Returns the number of rows that were updated, inserted or deleted by the
 *  query.  Not all databases support this call.  Don't use it for applications
 *  which are designed to be portable across databases.  0 is returned by
 *  databases which don't support this option. */
function sqlrcur_affectedRows($sqlrcurref){}

/** 
 *  Returns the index of the first buffered row.  This is useful when buffering
 *  only part of the result set at a time. */
function sqlrcur_firstRowIndex($sqlrcurref){}

/** 
 *  Returns 0 if part of the result set is still pending on the server and 1 if
 *  not.  This function can only return 0 if setResultSetBufferSize() has been
 *  called with a parameter other than 0. */
function sqlrcur_endOfResultSet($sqlrcurref){}

/**
 *  Returns true and acts like executeQuery() when there is another result set
 *  available from the server. */
function sqlrcur_nextResultSet($sqlrcurref){}



/**
 *  If a query failed and generated an error, the error message is available
 *  here.  If the query succeeded then this function returns a NULL. */
function sqlrcur_errorMessage($sqlrcurref){}


/**
 *  If a query failed and generated an error, the error number is available
 *  here.  If there is no error then this method returns 0. */
function sqlrcur_errorNumber($sqlrcurref){}



/** 
 *  Tells the connection to return NULL fields and output bind variables as
 *  empty strings.  This is the default. */
function sqlrcur_getNullsAsEmptyStrings($sqlrcurref){}

/**
 *  Tells the connection to return NULL fields
 *  and output bind variables as NULL's rather
 *  than as empty strings. */
function sqlrcur_getNullsAsNulls($sqlrcurref){}



/** 
 *  Returns the specified field as a string. "col" may be specified as the
 *  column name or number. */
function sqlrcur_getField($sqlrcurref, $row, $col){}

/**
 *  Returns the specified field as a string, ignoring the case of "col".
 *  "col" must be a column name. */
function sqlrcur_getFieldIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Returns the specified field as an integer. "col" may be specified as the
 *  column name or number. */
function sqlrcur_getFieldAsInteger($sqlrcurref, $row, $col){}

/** 
 *  Returns the specified field as a decimal. "col" may be specified as the
 *  column name or number. */
function sqlrcur_getFieldAsDouble($sqlrcurref, $row, $col){}

/**
 *  Returns the specified field as a boolean. "col" may be specified as the
 *  column name or number. */
function sqlrcur_getFieldAsBoolean($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the year component.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getFieldAsDateYear($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the month component.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getFieldAsDateMonth($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the day component.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getFieldAsDateDay($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the hour component.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getFieldAsDateHour($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the minute component.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getFieldAsDateMinute($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the second component.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getFieldAsDateSecond($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the microsecond
 *  component.  "col" may be specified as the column name or number. */
function sqlrcur_getFieldAsDateMicrosecond($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns whether the hour
 *  component is negative.  "col" may be specified as the column name or
 *  number. */
function sqlrcur_getFieldAsDateIsNegative($sqlrcurref, $row, $col){}

/**
 *  Returns the specified field as an integer, ignoring case of "col". */
function sqlrcur_getFieldAsIntegerIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Returns the specified field as a decimal, ignoring case of "col". */
function sqlrcur_getFieldAsDoubleIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Returns the specified field as a boolean, ignoring case of "col". */
function sqlrcur_getFieldAsBooleanIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the year component,
 *  ignoring case of "col". */
function sqlrcur_getFieldAsDateYearIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the month component,
 *  ignoring case of "col". */
function sqlrcur_getFieldAsDateMonthIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the day component,
 *  ignoring case of "col". */
function sqlrcur_getFieldAsDateDayIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the hour component,
 *  ignoring case of "col". */
function sqlrcur_getFieldAsDateHourIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the minute component,
 *  ignoring case of "col". */
function sqlrcur_getFieldAsDateMinuteIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the second component,
 *  ignoring case of "col". */
function sqlrcur_getFieldAsDateSecondIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns the microsecond
 *  component, ignoring case of "col". */
function sqlrcur_getFieldAsDateMicrosecondIgnoringCase($sqlrcurref, $row, $col){}

/**
 *  Interprets the specified field as a date and returns whether the hour
 *  component is negative, ignoring case of "col". */
function sqlrcur_getFieldAsDateIsNegativeIgnoringCase($sqlrcurref, $row, $col){}



/**
 *  Returns the length of the specified row and column. "col" may be
 *  specified as the column name or number. */
function sqlrcur_getFieldLength($sqlrcurref, $row, $col){}



/** 
 *  Returns an array of the values of the fields in the specified row. */
function sqlrcur_getRow($sqlrcurref, $row){}

/** 
 *  Returns an associative array of the
 *  values of the fields in the specified row. */
function sqlrcur_getRowAssoc($sqlrcurref, $row){}

/** 
 *  Returns an array of the lengths of the fields in the specified row. */
function sqlrcur_getRowLengths($sqlrcurref, $row){}

/** 
 *  Returns an associative array of the
 *  lengths of the fields in the specified row. */
function sqlrcur_getRowLenghtsAssoc($sqlrcurref, $row){}

/** 
 *  Returns an array of the column names of the current result set. */
function sqlrcur_getColumnNames($sqlrcurref){}

/** 
 *  Returns the name of the specified column. */
function sqlrcur_getColumnName($sqlrcurref, $col){}

/** 
 *  Returns the type of the specified column.  "col" may be specified as the
 *  column name or number. */
function sqlrcur_getColumnType($sqlrcurref, $col){}

/**
 *  Returns the number of bytes required on the server to store the data for
 *  the specified column.  "col" may be specified as the column name or
 *  number. */
function sqlrcur_getColumnLength($sqlrcurref, $col){}

/** 
 *  Returns the precision of the specified column.  Precision is the total
 *  number of digits in a number.  eg: 123.45 has a precision of 5.  For
 *  non-numeric types, it's the number of characters in the string.  "col"
 * may be specified as the column name or number. */
function sqlrcur_getColumnPrecision($sqlrcurref, $col){}

/** 
 *  Returns the scale of the specified column.  Scale is the total number of
 *  digits to the right of the decimal point in a number.  eg: 123.45 has a
 *  scale of 2.  "col" may be specified as the column name or number. */
function sqlrcur_getColumnScale($sqlrcurref, $col){}

/**
 *  Returns 1 if the specified column can contain nulls and 0 otherwise.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getColumnIsNullable($sqlrcurref, $col){}

/** 
 *  Returns 1 if the specified column is a primary key and 0 otherwise.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getColumnIsPrimaryKey($sqlrcurref, $col){}

/** 
 *  Returns 1 if the specified column is unique and 0 otherwise.  "col"
 *  may be specified as the column name or number. */
function sqlrcur_getColumnIsUnique($sqlrcurref, $col){}

/** 
 *  Returns 1 if the specified column is part of a composite key and 0
 *  otherwise.  "col" may be specified as the column name or number. */
function sqlrcur_getColumnIsPartOfKey($sqlrcurref, $col){}

/** 
 *  Returns 1 if the specified column is an unsigned number and 0 otherwise.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getColumnIsUnsigned($sqlrcurref, $col){}

/**
 *  Returns 1 if the specified column was created with the zero-fill flag and
 *  0 otherwise.  "col" may be specified as the column name or number. */
function sqlrcur_getColumnIsZeroFilled($sqlrcurref, $col){}

/** 
 *  Returns 1 if the specified column contains binary data and 0 otherwise.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getColumnIsBinary($sqlrcurref, $col){}

/** 
 *  Returns 1 if the specified column auto-increments and 0 otherwise.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getColumnIsAutoIncrement($sqlrcurref, $col){}

/** 
 *  Returns the length of the longest field in the specified column.
 *  "col" may be specified as the column name or number. */
function sqlrcur_getLongest($sqlrcurref, $col){}



/** 
 *  Tells the server to leave this result set open when the connection calls
 *  suspendSession() so that another connection can connect to it using
 *  resumeResultSet() after it calls resumeSession(). */
function sqlrcur_suspendResultSet($sqlrcurref){}

/** 
 *  Returns the internal ID of this result set.  This parameter may be passed
 *  to another cursor for use in the resumeResultSet() method.  Note: The
 *  value this method returns is only valid after a call to
 *  suspendResultSet(). */
function sqlrcur_getResultSetId($sqlrcurref){}

/** 
 *  Resumes a result set previously left open using suspendSession().
 *  Returns 1 on success and 0 on failure. */
function sqlrcur_resumeResultSet($sqlrcurref, $id){}

/** 
 *  Resumes a result set previously left open using suspendSession() and
 *  continues caching the result set to "filename".  Returns 1 on success and 0
 *  on failure. */
function sqlrcur_resumeCachedResultSet($sqlrcurref, $id, $filename){}

/**
 *  Closes the current result set, if one is open.  Data
 *  that has been fetched already is still available but
 *  no more data may be fetched.  Server side resources
 *  for the result set are freed as well. */
function sqlrcur_closeResultSet($sqlrcurref){}

?>

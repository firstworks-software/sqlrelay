# Copyright (c) 2000 Roman Milner
# See the file COPYING for more information

from SQLRelay import CSQLRelay

def getNumericFieldsAsStrings():
    """
    Instructs the API to return numeric fields as strings.  This is the
    default setting.  Truncation cannot occur if this setting is used.
    """
    CSQLRelay.getNumericFieldsAsStrings()

def getNumericFieldsAsNumbers():
    """
    Instructs the API to return numeric fields as numbers.  Integer fields will
    be returned as integers and floating point fields will be returned as
    Decimals if that class is avialable or floats otherwise.  The drawback to
    using numerics is that under some circumstances truncation can occur.
    """
    CSQLRelay.getNumericFieldsAsNumbers()


class sqlrconnection:

    """
    A wrapper for the sqlrelay connection API.  Closely follows the C++ API.
    """

    def __init__(self, host, port, socket, user, password, retrytime=0, tries=1):
        """ 
        Initiates a connection to "server" on "port" or to the unix "socket" on
        the local machine and auths with "user" and "password".  Failed
        connections will be retried for "tries" times, waiting "retrytime"
        seconds between each try.  If "tries" is 0 then retries will continue
        forever.  If "retrytime" is 0 then retries will be attempted on a
        default interval.

        If "server" is a comma-separated list of hosts, then an attempt will be
        made to connect to each until the attempt succeeds, or there are no
        more hosts left to try.

        If the "socket" parameter is neither NULL nor "" then an
        attempt will be made to connect through it before
        attempting to connect to "server" on "port".  If it is NULL
        or "" then no attempt will be made to connect through the
        socket.
        """
        self.connection = CSQLRelay.sqlrcon_alloc(host, port, socket, user, password, retrytime, tries)


    def __del__(self):
        CSQLRelay.sqlrcon_free(self.connection)

    def setConnectTimeout(self, timeoutsec, timeoutusec):
        """
        Sets the server connect timeout in seconds and
        milliseconds.  Setting either parameter to -1 disables the
        timeout.  You can also set this timeout using the
        SQLR_CLIENT_CONNECT_TIMEOUT environment variable.
        """
        return CSQLRelay.setConnectTimeout(self.connection, timeoutsec, timeoutusec)

    def getConnectTimeoutSeconds(self):
        """
        Gets the server connect timeout in seconds.
        """
        return CSQLRelay.getConnectTimeoutSeconds(self.connection)

    def getConnectTimeoutMicroseconds(self):
        """
        Gets the server connect timeout in microseconds.
        """
        return CSQLRelay.getConnectTimeoutMicroseconds(self.connection)

    def setResponseTimeout(self, timeoutsec, timeoutusec):
        """
        Sets the response timeout (for queries, commits, rollbacks,
        pings, etc.) in seconds and milliseconds.  Setting either
        parameter to -1 disables the timeout.  You can also set
        this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
        environment variable.
        """
        return CSQLRelay.setResponseTimeout(self.connection, timeoutsec, timeoutusec)

    def getResponseTimeoutSeconds(self):
        """
        Gets the response timeout in seconds.
        """
        return CSQLRelay.getResponseTimeoutSeconds(self.connection)

    def getResponseTimeoutMicroseconds(self):
        """
        Gets the response timeout in microseconds.
        """
        return CSQLRelay.getResponseTimeoutMicroseconds(self.connection)

    def setBindVariableDelimiters(self, delimiters):
        """
        Sets which delimiters are used to identify bind variables
        in countBindVariables() and validateBinds().  Valid
        delimiters include ?,:,@, and $.  Defaults to "?:@$"
        """
        return CSQLRelay.setBindVariableDelimiters(self.connection, delimiters)

    def getBindVariableDelimiterQuestionMarkSupported(self):
        """
        Returns true if question marks (?) are considered to be
        valid bind variable delimiters.
        """
        return CSQLRelay.getBindVariableDelimiterQuestionMarkSupported(self.connection)

    def getBindVariableDelimiterColonSupported(self):
        """
        Returns true if colons (:) are considered to be
        valid bind variable delimiters.
        """
        return CSQLRelay.getBindVariableDelimiterColonSupported(self.connection)

    def getBindVariableDelimiterAtSignSupported(self):
        """
        Returns true if at-signs (@) are considered to be
        valid bind variable delimiters.
        """
        return CSQLRelay.getBindVariableDelimiterAtSignSupported(self.connection)

    def getBindVariableDelimiterDollarSignSupported(self):
        """
        Returns true if dollar signs ($) are considered to be
        valid bind variable delimiters.
        """
        return CSQLRelay.getBindVariableDelimiterDollarSignSupported(self.connection)

    def enableKerberos(self, service, mech, flags):
        """
        Enables Kerberos authentication and encryption.

        "service" indicates the Kerberos service name of the
        SQL Relay server.  If left empty or NULL then the service
        name "sqlrelay" will be used. "sqlrelay" is the default
        service name of the SQL Relay server.  Note that on Windows
        platforms the service name must be fully qualified,
        including the host and realm name.  For example:
        "sqlrelay/sqlrserver.firstworks.com@AD.FIRSTWORKS.COM".
      
        "mech" indicates the specific Kerberos mechanism to use.
        On Linux/Unix platforms, this should be a string
        representation of the mechnaism's OID, such as:
            { 1 2 840 113554 1 2 2 }
        On Windows platforms, this should be a string like:
            Kerberos
        If left empty or NULL then the default mechanism will be
        used.  Only set this if you know that you have a good
        reason to.
      
        "flags" indicates what Kerberos flags to use.  Multiple
        flags may be specified, separated by commas.  If left
        empty or NULL then a defalt set of flags will be used.
        Only set this if you know that you have a good reason to.
      
        Valid flags include:
         * GSS_C_MUTUAL_FLAG
         * GSS_C_REPLAY_FLAG
         * GSS_C_SEQUENCE_FLAG
         * GSS_C_CONF_FLAG
         * GSS_C_INTEG_FLAG
      
        For a full list of flags, consult the GSSAPI documentation,
        though note that only the flags listed above are supported
        on Windows.
        """
        return CSQLRelay.enableKerberos(self.connection, service, mech, flags)

    def enableTls(self, version, cert, password, ciphers, validate, ca, depth):
        """
        Enables TLS/SSL encryption, and optionally authentication.

        "version" specifies the TLS/SSL protocol version that the
        client will attempt to use.  Valid values include SSL2,
        SSL3, TLS1, TLS1.1, TLS1.2 or any more recent version of
        TLS, as supported by and enabled in the underlying TLS/SSL
        library.  If left blank or empty then the highest supported
        version will be negotiated.
        
        "cert" is the file name of the certificate chain file to
        send to the SQL Relay server.  This is only necessary if
        the SQL Relay server is configured to authenticate and
        authorize clients by certificate.
        
        If "cert" contains a password-protected private key, then
        "password" may be supplied to access it.  If the private
        key is not password-protected, then this argument is
        ignored, and may be left empty or NULL.
        
        "ciphers" is a list of ciphers to allow.  Ciphers may be
        separated by spaces, commas, or colons.  If "ciphers" is
        empty or NULL then a default set is used.  Only set this if
        you know that you have a good reason to.
        
        For a list of valid ciphers on Linux/Unix platforms, see:
            man ciphers
        
        For a list of valid ciphers on Windows platforms, see:
            https://msdn.microsoft.com/en-us/library/windows/desktop/aa375549%28v=vs.85%29.aspx
        On Windows platforms, the ciphers (alg_id's) should omit
        CALG_ and may be given with underscores or dashes.
        For example: 3DES_112
        
        "validate" indicates whether to validate the SQL Relay's
        server certificate, and may be set to one of the following:
            "no" - Don't validate the server's certificate.
            "ca" - Validate that the server's certificate was
                   signed by a trusted certificate authority.
            "ca+host" - Perform "ca" validation and also validate
                   that one of the subject altenate names (or the
                   common name if no SANs are present) in the
                   certificate matches the host parameter.
                   (Falls back to "ca" validation when a unix
                   socket is used.)
            "ca+domain" - Perform "ca" validation and also validate
                   that the domain name of one of the subject
                   alternate names (or the common name if no SANs
                   are present) in the certificate matches the
                   domain name of the host parameter.  (Falls back
                   to "ca" validation when a unix socket is used.)
        
        "ca" is the location of a certificate authority file to
        use, in addition to the system's root certificates, when
        validating the SQL Relay server's certificate.  This is
        useful if the SQL Relay server's certificate is self-signed.
        
        On Windows, "ca" must be a file name.
        
        On non-Windows systems, "ca" can be either a file or
        directory name.  If it is a directory name, then all
        certificate authority files found in that directory will be
        used.  If it a file name, then only that file will be used.
        
        
        Note that the supported "cert" and "ca" file formats may
        vary between platforms.  A variety of file formats are
        generally supported on Linux/Unix platfoms (.pem, .pfx,
        etc.) but only the .pfx format is currently supported on
        Windows. */
        """
        return CSQLRelay.enableTls(self.connection, version, cert, password, ciphers, validate, ca, depth)

    def disableEncryption(self):
        """
        Disables encryption.
        """
        return CSQLRelay.disableEncryption(self.connection)

    def endSession(self):
        """
        Ends the current session.
        """
        return CSQLRelay.endSession(self.connection)

    def suspendSession(self):
        """
        Disconnects this connection from the current
        session but leaves the session open so 
        that another connection can connect to it 
        using resumeSession().
        """
        return CSQLRelay.suspendSession(self.connection)

    def getConnectionPort(self):
        """
        Returns the inet port that the connection is 
        communicating over. This parameter may be 
        passed to another connection for use in
        the resumeSession() method.
        Note: the value returned by this method is
        only valid after a call to suspendSession().
        """
        return CSQLRelay.getConnectionPort(self.connection)

    def getConnectionSocket(self):
        """
        Returns the unix socket that the connection is 
        communicating over. This parameter may be 
        passed to another connection for use in
        the resumeSession() method.
        Note: the value returned by this method is
        only valid after a call to suspendSession().
        """
        return CSQLRelay.getConnectionSocket(self.connection)

    def resumeSession(self, port, socket):
        """
        Resumes a session previously left open 
        using suspendSession().
        Returns 1 on success and 0 on failure.
        """
        return CSQLRelay.resumeSession(self.connection, port, socket)

    def ping(self):
        """
        Returns 1 if the database is up and 0
        if it's down.
        """
        return CSQLRelay.ping(self.connection)

    def identify(self):
        """
        Returns the type of database: 
          oracle, postgresql, mysql, etc.
        """
        return CSQLRelay.identify(self.connection)

    def dbVersion(self):
        """
        Returns the version of the database
        """
        return CSQLRelay.dbVersion(self.connection)

    def dbHostName(self):
        """
        Returns the host name of the database
        """
        return CSQLRelay.dbHostName(self.connection)

    def dbIpAddress(self):
        """
        Returns the ip address of the database
        """
        return CSQLRelay.dbIpAddress(self.connection)


    def serverVersion(self):
        """
        Returns the version of the SQL Relay server version
        """
        return CSQLRelay.serverVersion(self.connection)


    def clientVersion(self):
        """
        Returns the version of the SQL Relay client version
        """
        return CSQLRelay.clientVersion(self.connection)

    def bindFormat(self):
        """
        Returns a string representing the bind variable format used
        by the database.  For example:
        
          ?  - database uses a ? to represent a bind variable
          @* - database uses a @ followed by any characters to
               represent a bind variable
          $1 - database uses a $ followed by a number to represent a
               bind variable
          :* - database uses a : followed by any characters to
               represent a bind variable
        """
        return CSQLRelay.bindFormat(self.connection)

    def nextvalFormat(self):
        """
        Returns a string representing the format of the sequence
        nextval command used in the database.  The format will
        contain a %s in place of the sequence name.  For example:
        
          (nextval for %s)
          next value for %s
          nextval('%s')
          %s.nextval
    
        Returns an empty string if the database does not support
        sequences.
        """
        return CSQLRelay.nextvalFormat(self.connection)

    def selectDatabase(self,database):
        """
        Sets the current database (catalog) to "database"
        """
        return CSQLRelay.selectDatabase(self.connection,database)

    def getCurrentDatabase(self):
        """
        Returns the database (catalog) that is currently in use.
        """
        return CSQLRelay.getCurrentDatabase(self.connection)

    def selectSchema(self,schema):
        """
        Sets the current schema to "schema"
        """
        return CSQLRelay.selectSchema(self.connection,schema)

    def getCurrentSchema(self):
        """
        Returns the schema that is currently in use.
        """
        return CSQLRelay.getCurrentSchema(self.connection)

    def getLastInsertId(self):
        """
        Returns the value of the autoincrement column for the last insert
        """
        return CSQLRelay.getLastInsertId(self.connection)

    def autoCommitOn(self):
        """
        Instructs the database to perform a commit
        after every successful query.
        """
        return CSQLRelay.autoCommitOn(self.connection)

    def autoCommitOff(self):
        """
        Instructs the database to wait for the
        client to tell it when to commit.
        """
        return CSQLRelay.autoCommitOff(self.connection)

    def getAutoCommit(self):
        """
        Returns true if auto-commit is currently on,
        false otherwise.
        """
        return CSQLRelay.getAutoCommit(self.connection)

    def begin(self):
        """
        Begins a transaction.  Returns true if the begin
        succeeded, false if it failed.  If the database
        automatically begins a new transaction when a
        commit or rollback is issued then this doesn't
        do anything unless SQL Relay is faking transaction
        blocks.
        """
        return CSQLRelay.begin(self.connection)

    def commit(self):
        """
        Issues a commit, returns true if the commit
        succeeded, false if it failed.
        """
        return CSQLRelay.commit(self.connection)

    def rollback(self):
        """
        Issues a rollback, returns true if the rollback
        succeeded, false if it failed.
        """
        return CSQLRelay.rollback(self.connection)

    def setIsolationLevel(self, isolationlevel):
        """
        Sets the isolation level to "isolationlevel", the
        database-secific isolation level.  Returns true if setting
        the isolation level succeeded, false if it failed.
        """
        return CSQLRelay.setIsolationLevel(self.connection, isolationlevel)

    def getIsolationLevel(self):
        """
        Returns the database-specific isolation level, "unknown" if
        the isolation level is unknown, or None if an error occurred.
        """
        return CSQLRelay.getIsolationLevel(self.connection)

    def getDatabaseFeature(self, feature):
        """
        Returns the value of the specified database "feature".

        Valid features include:
        * aggregate_functions
         * list - ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM
        * all_procedures_are_callable
         * true/false
        * all_tables_are_selectable
         * true/false
        * alter_domain_clauses
         * list - ADD_DOMAIN_CONSTRAINT,ADD_DOMAIN_DEFAULT,...
        * alter_table_operations
         * list - ADD_COLUMN,DROP_COLUMN
        * ansi92_sql_levels
         * list - ENTRY_LEVEL,FULL,INTERMEDIATE
        * auto_commit_failure_closes_all_result_sets
         * true/false
        * batch_operations
         * list - SELECT_EXPLICIT,ROW_COUNT_EXPLICIT,SELECT_PROC,ROW_COUNT_PROC
        * batch_row_counts
         * list - PROCEDURES,EXPLICIT,ROLLED_UP
        * catalog_separator
         * string
        * catalog_term
         * string
        * catalog_usage
         * list - DATA_MANIPULATION,INDEX_DEFINITIONS,...
        * collation_seq
         * string
        * create_assertion_clauses
         * list - CREATE_ASSERTION,CONSTRAINT_INITIALLY_DEFERRED,...
        * create_character_set_clauses
         * list - CREATE_CHARACTER_SET,COLLATE_CLAUSE,...
        * create_collation_clauses
         * list - CREATE_COLLATION
        * create_domain_clauses
         * list - CREATE_DOMAIN,CONSTRAINT_NAME_DEFINITION,...
        * create_schema_clauses
         * list - CREATE_SCHEMA,AUTHORIZATION,DEFAULT_CHARACTER_SET
        * create_table_clauses
         * list - CREATE_TABLE,TABLE_CONSTRAINT,...
        * create_translation_clauses
         * list - CREATE_TRANSLATION
        * create_view_clauses
         * list - CREATE_VIEW,CHECK_OPTION,CASCADED,LOCAL
        * data_definition_transaction_behavior
         * list - CAUSES_COMMIT,IGNORED_IN_TRANSACTIONS
        * ddl_index_operations
         * list - CREATE_INDEX,DROP_INDEX
        * default_isolation_level
         * string
        * default_result_set_holdability
         * string
        * deletes_are_detected
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * does_max_row_size_include_blobs
         * true/false
        * drop_assertion_clauses
         * list - DROP_ASSERTION
        * drop_character_set_clauses
         * list - DROP_CHARACTER_SET
        * drop_collation_clauses
         * list - DROP_COLLATION
        * drop_domain_clauses
         * list - DROP_DOMAIN,CASCADE,RESTRICT
        * drop_schema_clauses
         * list - DROP_SCHEMA,CASCADE,RESTRICT
        * drop_table_clauses
         * list - DROP_TABLE,CASCADE,RESTRICT
        * drop_translation_clauses
         * list - DROP_TRANSLATION
        * drop_view_clauses
         * list - DROP_VIEW,CASCADE,RESTRICT
        * extra_name_characters
         * string
        * foreign_key_delete_rules
         * list - CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL
        * foreign_key_update_rules
         * list - CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL
        * forward_only_cursor_attributes
         * list - NEXT,ABSOLUTE,RELATIVE,BOOKMARK,...
        * generated_key_always_returned
         * true/false
        * grant_clauses
         * list - DELETE_TABLE,INSERT_COLUMN,INSERT_TABLE,...
        * group_by_clauses
         * list - BASIC,BEYOND_SELECT,UNRELATED
        * identifier_case_storage
         * list - LOWER,MIXED,UPPER
        * identifier_quote_string
         * string
        * index_keywords
         * list - ASC,DESC
        * info_schema_views
         * list - ASSERTIONS,CHARACTER_SETS,CHECK_CONSTRAINTS,...
        * insert_operations
         * list - INSERT_LITERALS,INSERT_SEARCHED,SELECT_INTO
        * inserts_are_detected
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * is_catalog_at_start
         * true/false
        * isolation_levels
         * list - READ_UNCOMMITTED,READ_COMMITTED,...
        * local_file_usage
         * list - LOCAL_FILE_PER_TABLE,LOCAL_FILES
        * locators_update_copy
         * true/false
        * lock_types
         * list - NO_CHANGE,EXCLUSIVE,UNLOCK
        * max_binary_literal_length
         * number
        * max_catalog_name_length
         * number
        * max_char_literal_length
         * number
        * max_column_name_length
         * number
        * max_columns_in_group_by
         * number
        * max_columns_in_index
         * number
        * max_columns_in_order_by
         * number
        * max_columns_in_select
         * number
        * max_columns_in_table
         * number
        * max_connections
         * number
        * max_cursor_name_length
         * number
        * max_identifier_length
         * number
        * max_index_length
         * number
        * max_procedure_name_length
         * number
        * max_row_size
         * number
        * max_schema_name_length
         * number
        * max_statement_length
         * number
        * max_statements
         * number
        * max_table_name_length
         * number
        * max_tables_in_select
         * number
        * max_user_name_length
         * number
        * mixed_case_identifiers
         * list - IDENTIFIERS,QUOTED_IDENTIFIERS
        * need_long_data_length
         * true/false
        * null_plus_non_null_is_null
         * true/false
        * null_sort_order
         * list - AT_END,AT_START,HIGH,LOW
        * numeric_functions
         * list - ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,COT,...
        * open_cursors_across
         * list - COMMIT,ROLLBACK
        * open_statements_across
         * list - COMMIT,ROLLBACK
        * others_deletes_are_visible
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * others_inserts_are_visible
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * others_updates_are_visible
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * outer_joins
         * list - BASIC,FULL,LIMITED
        * own_deletes_are_visible
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * own_inserts_are_visible
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * own_updates_are_visible
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * predicates
         * list - BETWEEN,COMPARISON,EXISTS,IN,ISNOTNULL,ISNULL,...
        * procedure_term
         * string
        * quoted_identifier_case_storage
         * list - LOWER,MIXED,UPPER
        * relational_join_operators
         * list - CORRESPONDING_CLAUSE,CROSS_JOIN,EXCEPT_JOIN,...
        * result_set_concurrencies
         * list - FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,...
        * result_set_holdabilities
         * list - CLOSE_CURSORS_AT_COMMIT,HOLD_CURSORS_OVER_COMMIT
        * result_set_types
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * revoke_clauses
         * list - CASCADE,DELETE_TABLE,GRANT_OPTION_FOR,...
        * row_id_lifetime
         * string
        * row_value_constructor_expressions
         * list - VALUE_EXPRESSION,NULL,DEFAULT,ROW_SUBQUERY
        * schema_term
         * string
        * schema_usage
         * list - DATA_MANIPULATION,INDEX_DEFINITIONS,...
        * scroll_concurrencies
         * list - READ_ONLY,LOCK,OPT_ROWVER,OPT_VALUES
        * search_string_escape
         * string
        * sql_grammar_levels
         * list - CORE,EXTENDED,MINIMUM
        * sql_keywords
         * list - ACCESS,ADD,ALTER,AUDIT,CLUSTER,COLUMN,COMMENT,...
        * sql_state_type
         * number
        * static_cursor_attributes
         * list - NEXT,ABSOLUTE,RELATIVE,BOOKMARK,...
        * stored_programs
         * list - FUNCTIONS,PROCEDURES
        * string_functions
         * list - CONCAT,INSERT,LEFT,LTRIM,LENGTH,LOCATE,LCASE,...
        * subquery_usage
         * list - COMPARISONS,EXISTS,INS,QUANTIFIEDS
        * supports_batch_updates
         * true/false
        * supports_column_aliasing
         * true/false
        * supports_convert
         * true/false
        * supports_correlated_subqueries
         * true/false
        * supports_describe_parameter
         * true/false
        * supports_expressions_in_order_by
         * true/false
        * supports_get_generated_keys
         * true/false
        * supports_integrity_enhancement_facility
         * true/false
        * supports_like_escape_clause
         * true/false
        * supports_multiple_result_sets
         * true/false
        * supports_multiple_transactions
         * true/false
        * supports_named_parameters
         * true/false
        * supports_non_nullable_columns
         * true/false
        * supports_order_by_unrelated
         * true/false
        * supports_savepoints
         * true/false
        * supports_select_for_update
         * true/false
        * supports_transactions
         * true/false
        * system_functions
         * list - USER,DBNAME,IFNULL
        * table_correlation_names
         * list - BASIC,DIFFERENT
        * table_term
         * string
        * time_date_add_intervals
         * list - FRAC_SECOND,SECOND,MINUTE,HOUR,DAY,WEEK,MONTH,...
        * time_date_diff_intervals
         * list - FRAC_SECOND,SECOND,MINUTE,HOUR,DAY,WEEK,MONTH,...
        * time_date_functions
         * list - NOW,CURDATE,DAYOFMONTH,DAYOFWEEK,DAYOFYEAR,...
        * time_date_literals
         * list - DATE,TIME,TIMESTAMP,INTERVAL_YEAR,...
        * transaction_ddl_dml
         * list - DDL_AND_DML,DML_ONLY
        * union_clauses
         * list - UNION,UNION_ALL
        * updates_are_detected
         * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
        * value_expressions
         * list - CASE,CAST,COALESCE,NULLIF
        * where_current_of_operations
         * list - DELETE,UPDATE

        Returns the value of the feature as a string, or None if
        an error occurred or an invalid feature was requested.
        """
        return CSQLRelay.getDatabaseFeature(self.connection, feature)

    def errorMessage(self):
        """
        If the operation failed, the error message will be returned
        from this method.  Otherwise, it returns None.
        """
        return CSQLRelay.connectionErrorMessage(self.connection)

    def errorNumber(self):
        """
        If an operation failed and generated an
        error, the error number is available here.
        If there is no error then this method 
        returns 0.
        """
        return CSQLRelay.connectionErrorNumber(self.connection)

    def debugOn(self):
        """
        Turn verbose debugging on.
        Another way to do this is to start a query with "-- debug\n".
        Yet another way is to set the environment variable SQLR_CLIENT_DEBUG
        to "ON"
        """
        return CSQLRelay.debugOn(self.connection)

    def debugOff(self):
        """
        Turn verbose debugging off.
        """
        return CSQLRelay.debugOff(self.connection)

    def getDebug(self):
        """
        Returns 1 if debugging is turned on and 0 if debugging is turned off.
        """
        return CSQLRelay.getDebug(self.connection)

    def setDebugFile(self,filename):
        """
        Allows you to specify a file to write debug to.
        Setting "filename" to NULL or an empty string causes debug
        to be written to standard output (the default).
        """
        return CSQLRelay.setDebugFile(self.connection,filename)

    def setClientInfo(self,clientinfo):
        """
        Allows you to set a string that will be passed to the
        server and ultimately included in server-side logging
        along with queries that were run by this instance of
        the client.
        """
        return CSQLRelay.setClientInfo(self.connection,clientinfo)

    def getClientInfo(self):
        """
        Returns the string that was set by setClientInfo().
        """
        return CSQLRelay.getClientInfo(self.connection)




class sqlrcursor:


    """
    A wrapper for the sqlrelay cursor API.  Closely follows the C++ API.
    """

    def __init__(self, sqlrcon):
        self.connection = sqlrcon
        self.cursor = CSQLRelay.sqlrcur_alloc(sqlrcon.connection)

    def __del__(self):
        CSQLRelay.sqlrcur_free(self.cursor)

    def setResultSetBufferSize(self, rows):
        """
        Sets the number of rows of the result set
        to buffer at a time.  0 (the default)
        means buffer the entire result set.
        """
        return CSQLRelay.setResultSetBufferSize(self.cursor, rows)

    def getResultSetBufferSize(self):
        """
        Returns the number of result set rows that 
        will be buffered at a time or 0 for the
        entire result set.
        """
        return CSQLRelay.getResultSetBufferSize(self.cursor)

    def dontGetColumnInfo(self):
        """
        Tells the server not to send any column
        info (names, types, sizes).  If you don't
        need that info, you should call this
        method to improve performance.
        """
        return CSQLRelay.dontGetColumnInfo(self.cursor)

    def mixedCaseColumnNames(self):
        """
        Columns names are returned in the same
        case as they are defined in the database.
        This is the default.
        """
        return CSQLRelay.mixedCaseColumnNames(self.cursor)

    def upperCaseColumnNames(self):
        """
        Columns names are converted to upper case.
        """
        return CSQLRelay.upperCaseColumnNames(self.cursor)

    def lowerCaseColumnNames(self):
        """
        Columns names are converted to lower case.
        """
        return CSQLRelay.lowerCaseColumnNames(self.cursor)

    def getColumnInfo(self):
        """
        Tells the server to send column info.
        """
        return CSQLRelay.getColumnInfo(self.cursor)

    def cacheToFile(self, filename):
        """
        Sets query caching on.  Future queries
        will be cached to the file "filename".
        The full pathname of the file can be
        retrieved using getCacheFileName().
        
        A default time-to-live of 10 minutes is
        also set.
        
        Note that once cacheToFile() is called,
        the result sets of all future queries will
        be cached to that file until another call 
        to cacheToFile() changes which file to
        cache to or a call to cacheOff() turns off
        caching.
        """
        return CSQLRelay.cacheToFile(self.cursor,filename)

    def setCacheTtl(self, ttl):
        """
        Sets the time-to-live for cached result
        sets. The sqlr-cachemanger will remove each 
        cached result set "ttl" seconds after it's 
        created.
        """
        return CSQLRelay.setCacheTtl(self.cursor,ttl)

    def getCacheFileName(self):
        """
        Returns the name of the file containing the most
        recently cached result set.
        """
        return CSQLRelay.getCacheFileName(self.cursor)

    def cacheOff(self):
        """
        Sets query caching off.
        """
        return CSQLRelay.cacheOff(self.cursor)

    def getDatabaseList(self,wild):
        """
        Sends a query that returns a list of
        databases/schemas matching "wild".  If wild is empty
        or NULL then a list of all databases/schemas will be
        returned.
        """
        return CSQLRelay.setDatabaseList(self.cursor,wild)

    def getTableList(self,wild):
        """
        Sends a query that returns a list of tables
        matching "wild".  If wild is empty or NULL then
        a list of all tables will be returned.
        """
        return CSQLRelay.setTableList(self.cursor,wild)

    def getColumnList(sefl,table,wild):
        """
        Sends a query that returns a list of columns
        in the table specified by the "table" parameter
        matching "wild".  If wild is empty or NULL then
        a list of all columns will be returned.
        """
        return CSQLRelay.setColumnList(self.cursor,table,wild)

    """
    If you don't need to use substitution or bind variables
    in your queries, use these two methods.
    """
    def sendQuery(self, query):
        """
        Send a SQL query to the server and
        gets a result set.
        """
        return CSQLRelay.sendQuery(self.cursor, query)

    def sendQueryWithLength(self, query, length):
        """
        Sends "query" with length "length" and gets
        a result set. This method must be used if
        the query contains binary data.
        """
        return CSQLRelay.sendQueryWithLength(self.cursor, query, length)

    def sendFileQuery(self, path, file):
        """
        Send the SQL query in path/file to the server and
        gets a result set.
        """
        return CSQLRelay.sendFileQuery(self.cursor, path, file)

    """
    If you need to use substitution or bind variables, in your
    queries use the following methods.  See the API documentation
    for more information about substitution and bind variables.
    """
    def prepareQuery(self, query):
        """
        Prepare to execute query.
        """
        return CSQLRelay.prepareQuery(self.cursor, query)

    def prepareQueryWithLength(self, query, length):
        """
        Prepare to execute "query" with length 
        "length".  This method must be used if the
        query contains binary data.
        """
        return CSQLRelay.prepareQueryWithLength(self.cursor, query, length)

    def prepareFileQuery(self, path, file):
        """
        Prepare to execute the contents of path/filename.
        """
        return CSQLRelay.prepareFileQuery(self.cursor, path, file)

    def substitution(self, variable, value, precision=0, scale=0):
        """
        Define a substitution variable.
        Returns true if the variable was successfully substituted or false if
        the variable isn't a string, integer or floating point number, or if
        precision and scale aren't provided for a floating point number.
        """
        return CSQLRelay.substitution(self.cursor,variable,value,precision,scale)

    def clearBinds(self):
        """
        Clear all binds variables.
        """
        return CSQLRelay.clearBinds(self.cursor)

    def countBindVariables(self):
        """
        Parses the previously prepared query,
        counts the number of bind variables defined
        in it and returns that number.
        """
        return CSQLRelay.countBindVariables(self.cursor)

    def inputBind(self, variable, value, precision=0, scale=0):
        """
        Define an input bind varaible.
        Returns true if the variable was successfully bound or false if the
        variable isn't a string, integer or decimal.  If the value is a decimal
        then precision and scale may also be specified.  If you don't have the
        precision and scale then set them both to 0.  However in that case you
        may get unexpected rounding behavior if the server is faking binds.
        """
        return CSQLRelay.inputBind(self.cursor, variable, value, precision, scale)

    def inputBindDate(self, variable, year, month, day, hour, minute, second, microsecond, tz, isnegative):
        """
        Define a date input bind variable.
        """
        return CSQLRelay.inputBindDate(self.cursor, variable, year, month, day, hour, minute, second, microsecond, tz, isnegative)

    def inputBindBlob(self, variable, value, length):
        """
        Define an input bind varaible.
        """
        return CSQLRelay.inputBindBlob(self.cursor, variable, value, length)

    def inputBindClob(self, variable, value, length):
        """
        Define an input bind varaible.
        """
        return CSQLRelay.inputBindClob(self.cursor, variable, value, length)

    def defineOutputBindString(self, variable, length):
        """
        Define a string output bind varaible.
        """
        return CSQLRelay.defineOutputBindString(self.cursor, variable, length)

    def defineOutputBindInteger(self, variable):
        """
        Define an integer output bind varaible.
        """
        return CSQLRelay.defineOutputBindInteger(self.cursor, variable)

    def defineOutputBindDouble(self, variable):
        """
        Define a double precision floating point output bind varaible.
        """
        return CSQLRelay.defineOutputBindDouble(self.cursor, variable)

    def defineOutputBindDate(self, variable):
        """
        Define a date output bind variable.
        """
        return CSQLRelay.defineOutputBindDate(self.cursor, variable)

    def defineOutputBindBlob(self, variable):
        """
        Define an output bind varaible.
        """
        return CSQLRelay.defineOutputBindBlob(self.cursor, variable)

    def defineOutputBindClob(self, variable):
        """
        Define an output bind varaible.
        """
        return CSQLRelay.defineOutputBindClob(self.cursor, variable)

    def defineOutputBindCursor(self, variable):
        """
        Define an output bind varaible.
        """
        return CSQLRelay.defineOutputBindCursor(self.cursor, variable)

    def substitutions(self, variables, values, precisions=None, scales=None):
        """
        Define substitution variables.
        Returns true if the variables were successfully substituted or false if
        one of the variables wasn't a string, integer or floating point number,
        or if precision and scale weren't provided for a floating point number.
        """
        return CSQLRelay.substitutions(self.cursor,variables,values,precisions,scales)

    def inputBinds(self, variables, values, precisions=None, scales=None):
        """
        Define input bind variables.
        Returns true if the variables were successfully bound or false if one
        of the variables wasn't a string, integer or floating point number,
        or if precision and scale weren't provided for a floating point number.
        """
        return CSQLRelay.inputBinds(self.cursor,variables,values,precisions,scales)
        

    def validateBinds(self):
        """
        If you are binding to any variables that 
        might not actually be in your query, call 
        this to ensure that the database won't try 
        to bind them unless they really are in the 
        query.
        """
        return CSQLRelay.validateBinds(self.cursor)
        

    def validBind(self,variable):
        """
        Returns true if "variable" was a valid
        input bind variable of the query.
        """
        return CSQLRelay.validBind(self.cursor,variable)
        

    def executeQuery(self):
        """
        Execute the query that was previously
        prepared and bound.
        """
        return CSQLRelay.executeQuery(self.cursor)

    def fetchFromBindCursor(self):
        """
        Fetch from a cursor that was returned as
        an output bind variable.
        """
        return CSQLRelay.fetchFromBindCursor(self.cursor)

    def getOutputBindString(self, variable):
        """
        Get the value stored in a previously
        defined output bind variable.
        """
        return CSQLRelay.getOutputBindString(self.cursor, variable)

    def getOutputBindBlob(self, variable):
        """
        Get the value stored in a previously
        defined output bind variable.
        """
        return CSQLRelay.getOutputBindBlob(self.cursor, variable)

    def getOutputBindClob(self, variable):
        """
        Get the value stored in a previously
        defined output bind variable.
        """
        return CSQLRelay.getOutputBindClob(self.cursor, variable)

    def getOutputBindInteger(self, variable):
        """
        Get the value stored in a previously
        defined output bind variable as a long
        integer.
        """
        return CSQLRelay.getOutputBindInteger(self.cursor, variable)

    def getOutputBindDouble(self, variable):
        """
        Get the value stored in a previously
        defined output bind variable as a double
        precision floating point number.
        """
        return CSQLRelay.getOutputBindDouble(self.cursor, variable)

    def getOutputBindLength(self, variable):
        """
        Retrieve the length of an output bind variable.
        """
        return CSQLRelay.getOutputBindLength(self.cursor, variable)

    def getOutputBindDateYear(self, variable):
        """
        Get the year from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateYear(self.cursor, variable)

    def getOutputBindDateMonth(self, variable):
        """
        Get the month from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateMonth(self.cursor, variable)

    def getOutputBindDateDay(self, variable):
        """
        Get the day from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateDay(self.cursor, variable)

    def getOutputBindDateHour(self, variable):
        """
        Get the hour from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateHour(self.cursor, variable)

    def getOutputBindDateMinute(self, variable):
        """
        Get the minute from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateMinute(self.cursor, variable)

    def getOutputBindDateSecond(self, variable):
        """
        Get the second from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateSecond(self.cursor, variable)

    def getOutputBindDateMicrosecond(self, variable):
        """
        Get the microsecond from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateMicrosecond(self.cursor, variable)

    def getOutputBindDateTz(self, variable):
        """
        Get the timezone from a previously defined
        date output bind variable.
        """
        return CSQLRelay.getOutputBindDateTz(self.cursor, variable)

    def getOutputBindDateIsNegative(self, variable):
        """
        Get whether the value of a previously defined
        date output bind variable is negative.
        """
        return CSQLRelay.getOutputBindDateIsNegative(self.cursor, variable)

    def getOutputBindCursor(self, variable):
        """
        Get the cursor associated with a previously
        defined output bind variable.
        """
        bindcursorid=CSQLRelay.getOutputBindCursorId(self.cursor, variable)
        if bindcursorid==-1:
                return None
        bindcursor=sqlrcursor(self.connection)
        CSQLRelay.attachToBindCursor(bindcursor.cursor, bindcursorid)
        return bindcursor

    def openCachedResultSet(self, filename):
        """
        Open a result set after a sendCachedQeury
        """
        return CSQLRelay.openCachedResultSet(self.cursor, filename)

    def colCount(self):
        """
        Returns the number of columns in the current result set.
        """
        return CSQLRelay.colCount(self.cursor)

    def rowCount(self):
        """
        Returns the number of rows in the current result set.
        """
        return CSQLRelay.rowCount(self.cursor)

    def totalRows(self):
        """
        Returns the total number of rows that will 
        be returned in the result set.  Not all 
        databases support this call.  Don't use it 
        for applications which are designed to be 
        portable across databases.  -1 is returned
        by databases which don't support this option.
        """
        return CSQLRelay.totalRows(self.cursor)

    def affectedRows(self):
        """
        Returns the number of rows that were 
        updated, inserted or deleted by the query.
        Not all databases support this call.  Don't 
        use it for applications which are designed 
        to be portable across databases.  -1 is 
        returned by databases which don't support 
        this option.
        """
        return CSQLRelay.affectedRows(self.cursor)

    def firstRowIndex(self):
        """
        Returns the index of the first buffered row.
        This is useful when buffering only part of
        the result set at a time.
        """
        return CSQLRelay.firstRowIndex(self.cursor)

    def endOfResultSet(self):
        """
        Returns 0 if part of the result set is still
        pending on the server and 1 if not.  This
        method can only return 0 if 
        setResultSetBufferSize() has been called
        with a parameter other than 0.
        """
        return CSQLRelay.endOfResultSet(self.cursor)

    def nextResultSet(self):
        """
        Returns true and acts like executeQuery()
        when there is another result set available
        from the server.
        """
        return CSQLRelay.nextResultSet(self.cursor)

    def errorMessage(self):
        """
        If the query failed, the error message will be returned
        from this method.  Otherwise, it returns None.
        """
        return CSQLRelay.cursorErrorMessage(self.cursor)

    def errorNumber(self):
        """
        If the query failed and generated an
        error, the error number is available here.
        If there is no error then this method 
        returns 0.
        """
        return CSQLRelay.cursorErrorNumber(self.cursor)

    def getNullsAsEmptyStrings(self):
        """
        Tells the cursor to return NULL fields and output
        bind variables as empty strings.
        This is the default.
        """
        return CSQLRelay.getNullsAsEmptyStrings(self.cursor)

    def getNullsAsNone(self):
        """
        Tells the cursor to return NULL fields and output
        bind variables as NULL's.
        """
        return CSQLRelay.getNullsAsNone(self.cursor)

    def getField(self, row, col):
        """
        Returns the value of the specified row and
        column.  col may be a column name or number.
        """
        return CSQLRelay.getField(self.cursor, row, col)

    def getFieldAsInteger(self, row, col):
        """
        Returns the specified field as a long integer.
        """
        return CSQLRelay.getFieldAsInteger(self.cursor, row, col)

    def getFieldAsDouble(self, row, col):
        """
        Returns the specified field as a double precision
        floating point number.
        """
        return CSQLRelay.getFieldAsDouble(self.cursor, row, col)

    def getFieldAsBoolean(self, row, col):
        """
        Returns the specified field as a boolean.
        """
        return CSQLRelay.getFieldAsBoolean(self.cursor, row, col)

    def getFieldAsDateYear(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns the year component.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateYear(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateYear(
			self.cursor, row, col)

    def getFieldAsDateMonth(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns the month component.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateMonth(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateMonth(
			self.cursor, row, col)

    def getFieldAsDateDay(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns the day component.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateDay(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateDay(
			self.cursor, row, col)

    def getFieldAsDateHour(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns the hour component.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateHour(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateHour(
			self.cursor, row, col)

    def getFieldAsDateMinute(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns the minute component.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateMinute(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateMinute(
			self.cursor, row, col)

    def getFieldAsDateSecond(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns the second component.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateSecond(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateSecond(
			self.cursor, row, col)

    def getFieldAsDateMicrosecond(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns the microsecond component.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateMicrosecond(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateMicrosecond(
			self.cursor, row, col)

    def getFieldAsDateIsNegative(self, row, col,
			ddmm=None, yyyyddmm=None,
			datedelimiters=None):
        """
        Interprets the specified field as a date
        and returns whether the hour component
        is negative.
        """
        if ddmm is not None:
            return CSQLRelay.getFieldAsDateIsNegative(
			self.cursor, row, col,
			ddmm, yyyyddmm, datedelimiters)
        return CSQLRelay.getFieldAsDateIsNegative(
			self.cursor, row, col)

    def getFieldLength(self, row, col):
        """
        Returns the length of the specified row and
        column.  col may be a column name or number.
        """
        return CSQLRelay.getFieldLength(self.cursor, row, col)

    def getRow(self, row):
        """
        Returns a list of values in the given row.
        """
        return CSQLRelay.getRow(self.cursor, row)

    def getRowDictionary(self, row):
        """
        Returns the requested row as values in a dictionary
        with column names for keys.
        """
        return CSQLRelay.getRowDictionary(self.cursor, row)

    def getRowRange(self, beg, end):
        """
        Returns a list of lists of the rows between beg and end.
        Note: this function has no equivalent in other SQL Relay API's.
        """
        return CSQLRelay.getRowRange(self.cursor, beg, end)

    def getRowLengths(self, row):
        """
        Returns a list of lengths in the given row.
        """
        return CSQLRelay.getRowLengths(self.cursor, row)

    def getRowLengthsDictionary(self, row):
        """
        Returns the requested row lengths as values in a dictionary
        with column names for keys.
        """
        return CSQLRelay.getRowLengthsDictionary(self.cursor, row)

    def getRowLengthsRange(self, beg, end):
        """
        Returns a list of lists of the lengths of rows between beg and end.
        Note: this function has no equivalent in other SQL Relay API's.
        """
        return CSQLRelay.getRowLengthsRange(self.cursor, beg, end)

    def getColumnName(self, col):
        """
        Returns the name of column number col.
        """
        return CSQLRelay.getColumnName(self.cursor, col)

    def getColumnNames(self):
        """
        Returns a list of column names in the current result set.
        """
        return CSQLRelay.getColumnNames(self.cursor)

    def getColumnType(self, col):
        """
        Returns the type of the specified column.  col may
        be a name or number.
        """
        return CSQLRelay.getColumnType(self.cursor, col)

    def getColumnLength(self, col):
        """
        Returns the length of the specified column.  col may
        be a name or number.
        """
        return CSQLRelay.getColumnLength(self.cursor, col)

    def getColumnPrecision(self, col):
        """
        Returns the precision of the specified column.
        Precision is the total number of digits in a number.
        eg: 123.45 has a precision of 5.  For non-numeric
        types, it's the number of characters in the string.
        """
        return CSQLRelay.getColumnPrecision(self.cursor, col)

    def getColumnScale(self, col):
        """
        Returns the scale of the specified column.  Scale is
        the total number of digits to the right of the decimal
        point in a number.  eg: 123.45 has a scale of 2.
        """
        return CSQLRelay.getColumnScale(self.cursor, col)

    def getColumnIsNullable(self, col):
        """
        Returns 1 if the specified column can contain nulls and
        0 otherwise.
        """
        return CSQLRelay.getColumnIsNullable(self.cursor, col)

    def getColumnIsPrimaryKey(self, col):
        """
        Returns 1 if the specified column is a primary key and
        0 otherwise.
        """
        return CSQLRelay.getColumnIsPrimaryKey(self.cursor, col)

    def getColumnIsUnique(self, col):
        """
        Returns 1 if the specified column is unique and
        0 otherwise.
        """
        return CSQLRelay.getColumnIsUnique(self.cursor, col)

    def getColumnIsPartOfKey(self, col):
        """
        Returns 1 if the specified column is part of a composite
        key and 0 otherwise.
        """
        return CSQLRelay.getColumnIsPartOfKey(self.cursor, col)

    def getColumnIsUnsigned(self, col):
        """
        Returns 1 if the specified column is an unsigned number
        and 0 otherwise.
        """
        return CSQLRelay.getColumnIsUnsigned(self.cursor, col)

    def getColumnIsZeroFilled(self, col):
        """
        Returns 1 if the specified column was created with the
        zero-fill flag and 0 otherwise.
        """
        return CSQLRelay.getColumnIsZeroFilled(self.cursor, col)

    def getColumnIsBinary(self, col):
        """
        Returns 1 if the specified column contains binary data
        and 0 otherwise.
        """
        return CSQLRelay.getColumnIsBinary(self.cursor, col)

    def getColumnIsAutoIncrement(self, col):
        """
        Returns 1 if the specified column auto-increments and
        0 otherwise.
        """
        return CSQLRelay.getColumnIsAutoIncrement(self.cursor, col)

    def getLongest(self, col):
        """
        Returns the length of the specified column.  col may
        be a name or number.
        """
        return CSQLRelay.getLongest(self.cursor, col)

    def suspendResultSet(self):
        """
        Tells the server to leave this result
        set open when the cursor calls 
        suspendSession() so that another cursor can 
        connect to it using resumeResultSet() after 
        it calls resumeSession().
        """
        return CSQLRelay.suspendResultSet(self.cursor)

    def getResultSetId(self):
        """
        Returns the internal ID of this result set.
        This parameter may be passed to another 
        cursor for use in the resumeResultSet() 
        method.
        Note: the value returned by this method is
        only valid after a call to suspendResultSet().
        """
        return CSQLRelay.getResultSetId(self.cursor)

    def resumeResultSet(self, id):
        """
        Resumes a result set previously left open 
        using suspendSession().
        Returns 1 on success and 0 on failure.
        """
        return CSQLRelay.resumeResultSet(self.cursor, id)

    def resumeCachedResultSet(self, id, filename):
        """
        Resumes a result set previously left open
        using suspendSession() and continues caching
        the result set to "filename".
        Returns 1 on success and 0 on failure.
        """
        return CSQLRelay.resumeCachedResultSet(self.cursor, id, filename)

    def closeResultSet():
        """
        Closes the current result set, if one is open.  Data
        that has been fetched already is still available but
        no more data may be fetched.  Server side resources
        for the result set are freed as well.
        """
        return CSQLRelay.closeResultSet(self.cursor)

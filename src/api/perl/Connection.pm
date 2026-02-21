# Copyright (c) David Muse
# See the file COPYING for more information

package SQLRelay::Connection;

require DynaLoader;
@ISA = 'DynaLoader';

bootstrap SQLRelay::Connection;

1;
__END__

=head1 NAME

    SQLRelay::Connection - Perl API for SQL Relay

=head1 SYNOPSIS

        use SQLRelay::Connection;
        use SQLRelay::Cursor;

        my $sc=SQLRelay::Connection->new("testhost",9000,"",
                                          "testuser","testpassword",0,1);
        my $ss=SQLRelay::Cursor->new($sc);

        $ss->sendQuery("select table_name from user_tables");
        $sc->endSession();

        for (my $i=0; $i<$ss->rowCount(); $i++) {
                print $ss->getField($i,"table_name"), "\n";
        }

=head1 DESCRIPTION

    SQLRelay::Connection

        new(server, port, socket, user, password, retrytime, tries);
            # Initiates a connection to "server" on "port"
            # or to the unix "socket" on the local machine
            # and auths with "user" and "password".
            # Failed connections will be retried for 
            # "tries" times, waiting "retrytime" seconds between each
            # try.  If "tries" is 0 then retries will continue forever.
            # If "retrytime" is 0 then retries will be attempted on
            # a default interval.
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

        DESTROY();
            # Disconnects and ends the session if
            # it hasn't been ended already.

        setConnectTimeout(timeoutsec, timeoutusec);
            # Sets the server connect timeout in seconds and
            # milliseconds.  Setting either parameter to -1 disables the
            # timeout.  You can also set this timeout using the
            # SQLR_CLIENT_CONNECT_TIMEOUT environment variable.

        getConnectTimeoutSeconds();
            # Gets the server connect timeout in seconds.

        getConnectTimeoutMicroseconds();
            # Gets the server connect timeout in microseconds.

        setResponseTimeout(timeoutsec, timeoutusec);
            # Sets the response timeout (for queries, commits, rollbacks,
            # pings, etc.) in seconds and milliseconds.  Setting either
            # parameter to -1 disables the timeout.  You can also set
            # this timeout using the SQLR_CLIENT_RESPONSE_TIMEOUT
            # environment variable.

        getResponseTimeoutSeconds();
            # Gets the response timeout in seconds.

        getResponseTimeoutMicroseconds();
            # Gets the response timeout in microseconds.

        setBindVariableDelimiters(delimiters);
            # Sets which delimiters are used to identify bind variables
            # in countBindVariables() and validateBinds().  Valid
            # delimiters include ?,:,@, and $.  Defaults to "?:@$" */

        getBindVariableDelimiterQuestionMarkSupported();
            # Returns true if question marks (?) are considered to be
            # valid bind variable delimiters. */

        getBindVariableDelimiterColonSupported();
            # Returns true if colons (:) are considered to be
            # valid bind variable delimiters. */

        getBindVariableDelimiterAtSignSupported();
            # Returns true if at-signs (@) are considered to be
            # valid bind variable delimiters. */

        getBindVariableDelimiterDollarSignSupported();
            # Returns true if dollar signs ($) are considered to be
            # valid bind variable delimiters. */

        enableKerberos(service, mech, flags);
            # Enables Kerberos authentication and encryption.
            #
            #  "service" indicates the Kerberos service name of the
            #  SQL Relay server.  If left empty or NULL then the service
            #  name "sqlrelay" will be used. "sqlrelay" is the default
            #  service name of the SQL Relay server.  Note that on Windows
            #  platforms the service name must be fully qualified,
            #  including the host and realm name.  For example:
            #  "sqlrelay/sqlrserver.firstworks.com@AD.FIRSTWORKS.COM".
            #
            #  "mech" indicates the specific Kerberos mechanism to use.
            #  On Linux/Unix platforms, this should be a string
            #  representation of the mechnaism's OID, such as:
            #      { 1 2 840 113554 1 2 2 }
            #  On Windows platforms, this should be a string like:
            #      Kerberos
            #  If left empty or NULL then the default mechanism will be
            #  used.  Only set this if you know that you have a good
            #  reason to.
            #
            #  "flags" indicates what Kerberos flags to use.  Multiple
            #  flags may be specified, separated by commas.  If left
            #  empty or NULL then a defalt set of flags will be used.
            #  Only set this if you know that you have a good reason to.
            #
            #  Valid flags include:
            #   * GSS_C_MUTUAL_FLAG
            #   * GSS_C_REPLAY_FLAG
            #   * GSS_C_SEQUENCE_FLAG
            #   * GSS_C_CONF_FLAG
            #   * GSS_C_INTEG_FLAG
            #
            #  For a full list of flags, consult the GSSAPI documentation,
            #  though note that only the flags listed above are supported
            #  on Windows.

        enableTls(version, cert, password, ciphers, validate, ca, depth);
            # Enables TLS/SSL encryption, and optionally authentication.
            #
            #  "version" specifies the TLS/SSL protocol version that the
            #  client will attempt to use.  Valid values include SSL2,
            #  SSL3, TLS1, TLS1.1, TLS1.2 or any more recent version of
            #  TLS, as supported by and enabled in the underlying TLS/SSL
            #  library.  If left blank or empty then the highest supported
            #  version will be negotiated.
            #
            #  "cert" is the file name of the certificate chain file to
            #  send to the SQL Relay server.  This is only necessary if
            #  the SQL Relay server is configured to authenticate and
            #  authorize clients by certificate.
            #
            #  If "cert" contains a password-protected private key, then
            #  "password" may be supplied to access it.  If the private
            #  key is not password-protected, then this argument is
            #  ignored, and may be left empty or NULL.
            #
            #  "ciphers" is a list of ciphers to allow.  Ciphers may be
            #  separated by spaces, commas, or colons.  If "ciphers" is
            #  empty or NULL then a default set is used.  Only set this if
            #  you know that you have a good reason to.
            #
            #  For a list of valid ciphers on Linux/Unix platforms, see:
            #      man ciphers
            #
            #  For a list of valid ciphers on Windows platforms, see:
            #      https://msdn.microsoft.com/en-us/library/windows/desktop/aa375549%28v=vs.85%29.aspx
            #  On Windows platforms, the ciphers (alg_id's) should omit
            #  CALG_ and may be given with underscores or dashes.
            #  For example: 3DES_112
            #
            #  "validate" indicates whether to validate the SQL Relay's
            #  server certificate, and may be set to one of the following:
            #      "no" - Don't validate the server's certificate.
            #      "ca" - Validate that the server's certificate was
            #             signed by a trusted certificate authority.
            #      "ca+host" - Perform "ca" validation and also validate
            #             that one of the subject altenate names (or the
            #             common name if no SANs are present) in the
            #             certificate matches the host parameter.
            #             (Falls back to "ca" validation when a unix
            #             socket is used.)
            #      "ca+domain" - Perform "ca" validation and also validate
            #             that the domain name of one of the subject
            #             alternate names (or the common name if no SANs
            #             are present) in the certificate matches the
            #             domain name of the host parameter.  (Falls back
            #             to "ca" validation when a unix socket is used.)
            #
            #  "ca" is the location of a certificate authority file to
            #  use, in addition to the system's root certificates, when
            #  validating the SQL Relay server's certificate.  This is
            #  useful if the SQL Relay server's certificate is self-signed.
            #
            #  On Windows, "ca" must be a file name.
            #
            #  On non-Windows systems, "ca" can be either a file or
            #  directory name.  If it is a directory name, then all
            #  certificate authority files found in that directory will be
            #  used.  If it a file name, then only that file will be used.
            #
            #
            #  Note that the supported "cert" and "ca" file formats may
            #  vary between platforms.  A variety of file formats are
            #  generally supported on Linux/Unix platfoms (.pem, .pfx,
            #  etc.) but only the .pfx format is currently supported on
            #  Windows. */

        disableEncryption();
            # Disables encryption.

        endSession();
            # Ends the session.

        suspendSession();
            # Leaves the session open so another client
            # can connect to it.
            
        getConnectionPort();
            # Returns the inet port that the client is 
            # communicating over. This parameter may be 
            # passed to another client for use in
            # the resumeSession() command below.
            # Note: the value returned by this method is
            # only valid after a call to suspendSession().

        getConnectionSocket();
            # Returns the unix socket that the client is 
            # communicating over. This parameter may be 
            # passed to another client for use in
            # the resumeSession() command below.
            # Note: the value returned by this method is
            # only valid after a call to suspendSession().

        resumeSession(port,socket);
            # Resumes a session previously left open 
            # using suspendSession().
            # Returns true on success and false on failure.


        ping();
            # Returns true if the database is up and false
            # if it's down.

        identify();
            # Returns the type of database:
            #   oracle, postgresql, mysql, etc.

        dbVersion();
            # Returns the version of the database

        dbHostName();
            # Returns the host name of the database

        dbIpAddress();
            # Returns the ip address of the database

        serverVersion();
            # Returns the version of the SQL Relay server software

        clientVersion();
            # Returns the version of the SQL Relay client software

        bindFormat();
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

        nextvalFormat();
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


        selectDatabase(database);
            # Sets the current database (catalog) to "database"
        getCurrentDatabase();
            # Returns the database (catalog) that is currently in use.

        selectSchema(schema);
            # Sets the current schema to "schema"
        getCurrentSchema();
            # Returns the schema that is currently in use.

        getLastInsertId();
            # Returns the value of the autoincrement
            # column for the last insert

        autoCommitOn();
            # Instructs the database to perform a commit
            # after every successful query.
            # Returns true if setting autocommit on succeeded
            # and false if it failed.

        autoCommitOff();
            # Instructs the database to wait for the 
            # client to tell it when to commit.
            # Returns true if setting autocommit off succeeded
            # and false if it failed.

        begin();
            # Begins a transaction.  Returns true if the begin
            # succeeded, false if it failed.  If the database
            # automatically begins a new transaction when a
            # commit or rollback is issued then this doesn't
            # do anything unless SQL Relay is faking transaction
            # blocks.

        commit();
            # Issues a commit.  Returns true if the commit
            # succeeded, false if it failed.

        rollback();
            # Issues a rollback.  Returns true if the rollback
            # succeeded, false if it failed.

        setIsolationLevel(isolationlevel);
            # Sets the isolation level to "isolationlevel", the
            # database-secific isolation level.  Returns true if
            # setting the isolation level succeeded, false if it
            # failed.

        getIsolationLevel();
            # Returns the database-specific isolation level,
            # "unknown" if the isolation level is unknown, or undef
            # if an error occurred.

        getDatabaseFeature(feature);
            # Returns the value of the specified database "feature".
            #
            #  Valid features include:
            #  * all_procedures_are_callable
            #   * true/false
            #  * all_tables_are_selectable
            #   * true/false
            #  * auto_commit_failure_closes_all_result_sets
            #   * true/false
            #  * catalog_separator
            #   * string
            #  * catalog_term
            #   * string
            #  * collation_seq
            #   * string
            #  * data_definition_causes_transaction_commit
            #   * true/false
            #  * data_definition_ignored_in_transactions
            #   * true/false
            #  * default_isolation_level
            #   * string
            #  * deletes_are_detected
            #   * list
            #  * does_max_row_size_include_blobs
            #   * true/false
            #  * extra_name_characters
            #   * string
            #  * generated_key_always_returned
            #   * true/false
            #  * identifier_quote_string
            #   * string
            #  * index_keywords
            #   * list - ASC,DESC
            #  * info_schema_views
            #   * list
            #  * inserts_are_detected
            #   * list
            #  * is_catalog_at_start
            #   * true/false
            #  * is_read_only
            #   * true/false
            #  * locators_update_copy
            #   * true/false
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
            #  * need_long_data_length
            #   * true/false
            #  * null_plus_non_null_is_null
            #   * true/false
            #  * nulls_are_sorted_at_end
            #   * true/false
            #  * nulls_are_sorted_at_start
            #   * true/false
            #  * nulls_are_sorted_high
            #   * true/false
            #  * nulls_are_sorted_low
            #   * true/false
            #  * numeric_functions
            #   * list - ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,EXP,FLOOR,...
            #  * others_deletes_are_visible
            #   * list - SCROLL_SENSITIVE
            #  * others_inserts_are_visible
            #   * list
            #  * others_updates_are_visible
            #   * list - SCROLL_SENSITIVE
            #  * own_deletes_are_visible
            #   * list - SCROLL_INSENSITIVE,SCROLL_SENSITIVE
            #  * own_inserts_are_visible
            #   * list
            #  * own_updates_are_visible
            #   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
            #  * procedure_term
            #   * string
            #  * result_set_holdability
            #   * number
            #  * row_id_lifetime
            #   * string
            #  * schema_term
            #   * string
            #  * search_string_escape
            #   * string
            #  * sql_keywords
            #   * list - ACCESS,ADD,ALTER,AUDIT,CLUSTER,COLUMN,COMMENT,...
            #  * sql_state_type
            #   * number
            #  * stores_lower_case_identifiers
            #   * true/false
            #  * stores_lower_case_quoted_identifiers
            #   * true/false
            #  * stores_mixed_case_identifiers
            #   * true/false
            #  * stores_mixed_case_quoted_identifiers
            #   * true/false
            #  * stores_upper_case_identifiers
            #   * true/false
            #  * stores_upper_case_quoted_identifiers
            #   * true/false
            #  * string_functions
            #   * list - ASCII,CHAR,CHAR_LENGTH,CHARACTER_LENGTH,CONCAT,...
            #  * supported_foreign_key_delete_rules
            #   * list - CASCADE,NO_ACTION,SET_NULL
            #  * supported_foreign_key_update_rules
            #   * list - CASCADE,NO_ACTION,SET_NULL
            #  * supported_predicates
            #   * list - BETWEEN,COMPARISON,EXISTS,IN,ISNOTNULL,ISNULL,...
            #  * supported_relational_join_operators
            #   * list - CROSS_JOIN,FULL_OUTER_JOIN,INNER_JOIN,...
            #  * supported_row_value_constructor_expressions
            #   * list - VALUE_EXPRESSION,NULL,DEFAULT,ROW_SUBQUERY
            #  * supported_value_expressions
            #   * list - CASE,CAST,COALESCE,NULLIF
            #  * supports_aggregate_functions
            #   * list - ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM
            #  * supports_alter_domain
            #   * list
            #  * supports_alter_table_with_add_column
            #   * true/false
            #  * supports_alter_table_with_drop_column
            #   * true/false
            #  * supports_ansi92_entry_level_sql
            #   * true/false
            #  * supports_ansi92_full_sql
            #   * true/false
            #  * supports_ansi92_intermediate_sql
            #   * true/false
            #  * supports_batch_updates
            #   * true/false
            #  * supports_catalogs_in_data_manipulation
            #   * true/false
            #  * supports_catalogs_in_index_definitions
            #   * true/false
            #  * supports_catalogs_in_privilege_definitions
            #   * true/false
            #  * supports_catalogs_in_procedure_calls
            #   * true/false
            #  * supports_catalogs_in_table_definitions
            #   * true/false
            #  * supports_column_aliasing
            #   * true/false
            #  * supports_convert
            #   * true/false
            #  * supports_core_sql_grammar
            #   * true/false
            #  * supports_correlated_subqueries
            #   * true/false
            #  * supports_create_assertion
            #   * list
            #  * supports_create_character_set
            #   * list
            #  * supports_create_collation
            #   * list
            #  * supports_create_domain
            #   * list
            #  * supports_create_schema
            #   * list - CREATE_SCHEMA,AUTHORIZATION
            #  * supports_create_table
            #   * list - CREATE_TABLE,TABLE_CONSTRAINT,...
            #  * supports_create_translation
            #   * list
            #  * supports_create_view
            #   * list - CREATE_VIEW,CHECK_OPTION,LOCAL
            #  * supports_data_definition_and_data_manipulation_transactions
            #   * true/false
            #  * supports_data_manipulation_transactions_only
            #   * true/false
            #  * supports_ddl_index
            #   * list - CREATE_INDEX,DROP_INDEX
            #  * supports_describe_parameter
            #   * true/false
            #  * supports_different_table_correlation_names
            #   * true/false
            #  * supports_drop_assertion
            #   * list
            #  * supports_drop_character_set
            #   * list
            #  * supports_drop_collation
            #   * list
            #  * supports_drop_domain
            #   * list
            #  * supports_drop_schema
            #   * list - DROP_SCHEMA,CASCADE,RESTRICT
            #  * supports_drop_table
            #   * list - DROP_TABLE,CASCADE,RESTRICT
            #  * supports_drop_translation
            #   * list
            #  * supports_drop_view
            #   * list - DROP_VIEW,CASCADE,RESTRICT
            #  * supports_expressions_in_order_by
            #   * true/false
            #  * supports_extended_sql_grammar
            #   * true/false
            #  * supports_full_outer_joins
            #   * true/false
            #  * supports_get_generated_keys
            #   * true/false
            #  * supports_grant
            #   * list - DELETE_TABLE,INSERT_COLUMN,INSERT_TABLE,...
            #  * supports_group_by
            #   * true/false
            #  * supports_group_by_beyond_select
            #   * true/false
            #  * supports_group_by_unrelated
            #   * true/false
            #  * supports_insert_statement
            #   * list - INSERT_LITERALS,INSERT_SEARCHED,SELECT_INTO
            #  * supports_integrity_enhancement_facility
            #   * true/false
            #  * supports_like_escape_clause
            #   * true/false
            #  * supports_limited_outer_joins
            #   * true/false
            #  * supports_lock_types
            #   * list - NO_CHANGE,EXCLUSIVE,UNLOCK
            #  * supports_minimum_sql_grammar
            #   * true/false
            #  * supports_mixed_case_identifiers
            #   * true/false
            #  * supports_mixed_case_quoted_identifiers
            #   * true/false
            #  * supports_multiple_result_sets
            #   * true/false
            #  * supports_multiple_transactions
            #   * true/false
            #  * supports_named_parameters
            #   * true/false
            #  * supports_non_nullable_columns
            #   * true/false
            #  * supports_open_cursors_across_commit
            #   * true/false
            #  * supports_open_cursors_across_rollback
            #   * true/false
            #  * supports_open_statements_across_commit
            #   * true/false
            #  * supports_open_statements_across_rollback
            #   * true/false
            #  * supports_order_by_unrelated
            #   * true/false
            #  * supports_outer_joins
            #   * true/false
            #  * supports_positioned_delete
            #   * true/false
            #  * supports_positioned_update
            #   * true/false
            #  * supports_result_set_concurrency
            #   * list - FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,...
            #  * supports_result_set_holdability
            #   * list - CLOSE_CURSORS_AT_COMMIT,HOLD_CURSORS_OVER_COMMIT
            #  * supports_result_set_type
            #   * list - FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE
            #  * supports_revoke
            #   * list - CASCADE,DELETE_TABLE,GRANT_OPTION_FOR,...
            #  * supports_savepoints
            #   * true/false
            #  * supports_schemas_in_data_manipulation
            #   * true/false
            #  * supports_schemas_in_index_definitions
            #   * true/false
            #  * supports_schemas_in_privilege_definitions
            #   * true/false
            #  * supports_schemas_in_procedure_calls
            #   * true/false
            #  * supports_schemas_in_table_definitions
            #   * true/false
            #  * supports_select_for_update
            #   * true/false
            #  * supports_stored_functions_using_call_syntax
            #   * true/false
            #  * supports_stored_procedures
            #   * true/false
            #  * supports_subqueries_in_comparisons
            #   * true/false
            #  * supports_subqueries_in_exists
            #   * true/false
            #  * supports_subqueries_in_ins
            #   * true/false
            #  * supports_subqueries_in_quantifieds
            #   * true/false
            #  * supports_table_correlation_names
            #   * true/false
            #  * supports_transaction_isolation_level
            #   * list - READ_COMMITTED,SERIALIZABLE
            #  * supports_transactions
            #   * true/false
            #  * supports_union
            #   * true/false
            #  * supports_union_all
            #   * true/false
            #  * system_functions
            #   * list - USER
            #  * table_term
            #   * string
            #  * time_date_add_intervals
            #   * list
            #  * time_date_diff_intervals
            #   * list
            #  * time_date_functions
            #   * list - CURRENT_DATE,CURRENT_TIMESTAMP,CURDATE,EXTRACT,...
            #  * time_date_literals
            #   * list - DATE,TIMESTAMP,INTERVAL_YEAR_TO_MONTH,...
            #  * updates_are_detected
            #   * list
            #  * uses_local_file_per_table
            #   * true/false
            #  * uses_local_files
            #   * true/false
            #
            #  Returns the value of the feature as a string, or undef if
            #  an error occurred or an invalid feature was requested.

        errorMessage();
            # If an operation failed and generated an error, the
            # error message is available here.  If there is no
            # error then this method returns NULL.

        errorNumber();
            # If an operation failed and generated an
            # error, the error number is available here.
            # If there is no error then this method 
            # returns 0.


        debugOn();
            # Causes verbose debugging information to be 
            # sent to standard output.  Another way to do 
            # this is to start a query with "-- debug\n".
            # Yet another way is to set the environment
            # variable SQLR_CLIENT_DEBUG to "ON"

        debugOff();
            # Turns debugging off.

        getDebug();
            # Returns true if debugging is currently on and false
            # if debugging is currently off.

        setDebugFile(filename);
            # Allows you to specify a file to write debug to.
            # Setting "filename" to NULL or an empty string causes debug
            # to be written to standard output (the default).

        setClientInfo(clientinfo);
            # Allows you to set a string that will be passed to the
            # server and ultimately included in server-side logging
            # along with queries that were run by this instance of
            # the client.

        getClientInfo();
            # Returns the string that was set by setClientInfo().

=head1 AUTHOR

    David Muse
    david.muse@firstworks.com

=cut

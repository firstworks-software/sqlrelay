// Copyright (c) David Muse
// See the file COPYING for more information

using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace SQLRClient
{

public class SQLRConnection : IDisposable
{
    /** Initiates a connection to "server" on "port" or to the unix "socket" on
     *  the local machine and auths with "user" and "password".  Failed
     *  connections will be retried for "tries" times, waiting "retrytime"
     *  seconds between each try.  If "tries" is 0 then retries will continue
     *  forever.  If "retrytime" is 0 then retries will be attempted on a
     *  default interval.
     *
     *  If "server" is a comma-separated list of hosts, then an attempt will be
     *  made to connect to each until the attempt succeeds, or there are no
     *  more hosts left to try.
     *
     *  If the "socket" parameter is nether NULL nor "" then an attempt will be
     *  made to connect through it before attempting to connect to "server" on
     *  "port".  If it is NULL or "" then no attempt will be made to connect
     *  through the socket.*/
    public SQLRConnection(String server, UInt16 port, String socket, String user, String password, Int32 retrytime, Int32 tries)
    {
        sqlrconref = sqlrcon_alloc_copyrefs(server, port, socket, user, password, retrytime, tries, 1);
    }
    
    /** Dispose framework */
    private Boolean disposed = false;
    public void Dispose()
    {
        Dispose(true);
        GC.SuppressFinalize(this);
    }
    protected virtual void Dispose(Boolean disposing)
    {
        if (!disposed)
        {
            sqlrcon_free(sqlrconref);
            disposed = true;
        }
    }

    /** Disconnects and ends the session if it hasn't been terminated
     *  already. */
    ~SQLRConnection()
    {
        Dispose(false);
    }
    
    
    
    /** Sets the server connect timeout in seconds and milliseconds.  Setting
     *  either parameter to -1 disables the timeout.  You can also set this
     *  timeout using the SQLR_CLIENT_CONNECT_TIMEOUT environment variable. */
    public void setConnectTimeout(Int32 timeoutsec, Int32 timeoutusec)
    {
        sqlrcon_setConnectTimeout(sqlrconref, timeoutsec, timeoutusec);
    }

    /** Gets the server connect timeout in seconds. */
    public Int32 getConnectTimeoutSeconds()
    {
        return sqlrcon_getConnectTimeoutSeconds(sqlrconref);
    }

    /** Gets the server connect timeout in microseconds. */
    public Int32 getConnectTimeoutMicroseconds()
    {
        return sqlrcon_getConnectTimeoutMicroseconds(sqlrconref);
    }

    

    /** Sets the response timeout (for queries, commits, rollbacks, pings,
      * etc.) in seconds and milliseconds.  Setting either parameter to -1
      * disables the timeout.  You can also set this timeout using the
      * SQLR_CLIENT_RESPONSE_TIMEOUT environment variable. */
    public void setResponseTimeout(Int32 timeoutsec, Int32 timeoutusec)
    {
        sqlrcon_setResponseTimeout(sqlrconref, timeoutsec, timeoutusec);
    }

    /** Gets the response timeout in seconds. */
    public Int32 getResponseTimeoutSeconds()
    {
        return sqlrcon_getResponseTimeoutSeconds(sqlrconref);
    }

    /** Gets the response timeout in microseconds. */
    public Int32 getResponseTimeoutMicroseconds()
    {
        return sqlrcon_getResponseTimeoutMicroseconds(sqlrconref);
    }



    /** Sets which delimiters are used to identify bind variables
     *  in countBindVariables() and validateBinds().  Valid
     *  delimiters include ?,:,@, and $.  Defaults to "?:@$" */
    public void setBindVariableDelimiters(String delimiters)
    {
        sqlrcon_setBindVariableDelimiters(sqlrconref, delimiters);
    }

    /** Returns true if question marks (?) are considered to be
    *  valid bind variable delimiters. */
    public Boolean getBindVariableDelimiterQuestionMarkSupported()
    {
        return sqlrcon_getBindVariableDelimiterQuestionMarkSupported(sqlrconref)!=0;
    }

    /** Returns true if colons (:) are considered to be
     *  valid bind variable delimiters. */
    public Boolean getBindVariableDelimiterColonSupported()
    {
        return sqlrcon_getBindVariableDelimiterColonSupported(sqlrconref)!=0;
    }

    /** Returns true if at-signs (@) are considered to be
     *  valid bind variable delimiters. */
    public Boolean getBindVariableDelimiterAtSignSupported()
    {
        return sqlrcon_getBindVariableDelimiterAtSignSupported(sqlrconref)!=0;
    }

    /** Returns true if dollar signs ($) are considered to be
     *  valid bind variable delimiters. */
    public Boolean getBindVariableDelimiterDollarSignSupported()
    {
        return sqlrcon_getBindVariableDelimiterDollarSignSupported(sqlrconref)!=0;
    }



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
    public void enableKerberos(String service, String mech, String flags)
    {
        sqlrcon_enableKerberos(sqlrconref, service, mech, flags);
    }

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
    public void enableTls(String version, String cert, String password, String ciphers, String validate, String ca, UInt16 depth)
    {
        sqlrcon_enableTls(sqlrconref, version, cert, password, ciphers, validate, ca, depth);
    }

    /** Disables encryption. */
    public void disableEncryption()
    {
        sqlrcon_disableEncryption(sqlrconref);
    }



    /** Ends the session. */
    public void endSession()
    {
        sqlrcon_endSession(sqlrconref);
    }
    
    /** Disconnects this connection from the current session but leaves the
     *  session open so that another connection can connect to it using
     *  sqlrcon_resumeSession(). */
    public Boolean suspendSession()
    {
        return sqlrcon_suspendSession(sqlrconref)!=0;
    }
    
    /** Returns the inet port that the connection is communicating over.  This
     *  parameter may be passed to another connection for use in the
     *  sqlrcon_resumeSession() command.  Note: The result this function returns
     *  is only valid after a call to suspendSession(). */
    public UInt16 getConnectionPort()
    {
        return sqlrcon_getConnectionPort(sqlrconref);
    }
    
    /** Returns the unix socket that the connection is communicating over.  This
     *  parameter may be passed to another connection for use in the
     *  sqlrcon_resumeSession() command.  Note: The result this function returns
     *  is only valid after a call to suspendSession(). */
    public String getConnectionSocket()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getConnectionSocket(sqlrconref));
    }
    
    /** Resumes a session previously left open using sqlrcon_suspendSession().
     *  Returns true on success and false on failure. */
    public Boolean resumeSession(UInt16 port, String socket)
    {
        return sqlrcon_resumeSession(sqlrconref, port, socket)!=0;
    }
    
    
    
    /** Returns true if the database is up and false if it's down. */
    public Boolean ping()
    {
        return sqlrcon_ping(sqlrconref)!=0;
    }
    
    /** Returns the type of database: oracle, postgresql, mysql, etc. */
    public String identify()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_identify(sqlrconref));
    }
    
    /** Returns the version of the database */
    public String dbVersion()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_dbVersion(sqlrconref));
    }
    
    /** Returns the host name of the database */
    public String dbHostName()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_dbHostName(sqlrconref));
    }
    
    /** Returns the ip address of the database */
    public String dbIpAddress()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_dbIpAddress(sqlrconref));
    }
    
    /** Returns the version of the sqlrelay server software. */
    public String serverVersion()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_serverVersion(sqlrconref));
    }
    
    /** Returns the version of the sqlrelay client software. */
    public String clientVersion()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_clientVersion(sqlrconref));
    }
    
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
    public String bindFormat()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_bindFormat(sqlrconref));
    }

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
    public String nextvalFormat()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_nextvalFormat(sqlrconref));
    }
    
    
    
    /** Sets the current database to "database".
     *
     *  May set the current catalog or schema, depending on
     *  whether the backend database equates "database" with
     *  catalog or schema.
     *
     *  See getDatabaseIsSchema(). */
    public Boolean selectDatabase(String database)
    {
        return sqlrcon_selectDatabase(sqlrconref, database)!=0;
    }

    /** Returns the database that is currently in use.
     *
     *  May return the current catalog or schema, depending on
     *  whether the backend database equates "database" with
     *  catalog or schema.
     *
     *  See getDatabaseIsSchema(). */
    public String getCurrentDatabase()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getCurrentDatabase(sqlrconref));
    }

    /** Returns true if the backend database equates "database" with
     *  "schema", and false if it equates "database" with "catalog". */
    public Boolean getDatabaseIsSchema()
    {
        return sqlrcon_getDatabaseIsSchema(sqlrconref)!=0;
    }

    /** Sets the current catalog to "catalog" */
    public Boolean selectCatalog(String catalog)
    {
        return sqlrcon_selectCatalog(sqlrconref, catalog)!=0;
    }

    /** Returns the catalog that is currently in use. */
    public String getCurrentCatalog()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getCurrentCatalog(sqlrconref));
    }

    /** Sets the current schema to "schema" */
    public Boolean selectSchema(String schema)
    {
        return sqlrcon_selectSchema(sqlrconref, schema)!=0;
    }

    /** Returns the schema that is currently in use. */
    public String getCurrentSchema()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getCurrentSchema(sqlrconref));
    }



    /** Returns the user that sqlrelay is currently logged in to
     *  the database as, or null if no user could be determined
     *  or if an error occurred. */
    public String getCurrentUser()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getCurrentUser(sqlrconref));
    }

    /** Returns the value of the autoincrement column for the last insert */
    public UInt64 getLastInsertId()
    {
        return sqlrcon_getLastInsertId(sqlrconref);
    }
    
    
    
    /** Instructs the database to perform a commit after every successful
     *  query. */
    public Boolean autoCommitOn()
    {
        return sqlrcon_autoCommitOn(sqlrconref)!=0;
    }
    
    /** Instructs the database to wait for the client to tell it when to
     *  commit. */
    public Boolean autoCommitOff()
    {
        return sqlrcon_autoCommitOff(sqlrconref)!=0;
    }

    /** Returns true if auto-commit is currently on, false otherwise. */
    public Boolean getAutoCommit()
    {
        return sqlrcon_getAutoCommit(sqlrconref)!=0;
    }

    /** Begins a transaction.  Returns true if the begin
     *  succeeded, false if it failed.  If the database
     *  automatically begins a new transaction when a
     *  commit or rollback is issued then this doesn't
     *  do anything unless SQL Relay is faking transaction
     *  blocks. */
    public Boolean begin()
    {
        return (sqlrcon_begin(sqlrconref) == 1);
    }
    
    
    /** Issues a commit.  Returns true if the commit succeeded and false if it failed. */
    public Boolean commit()
    {
        return (sqlrcon_commit(sqlrconref) == 1);
    }
    
    /** Issues a rollback.  Returns true if the rollback succeeded, false if it failed. */
    public Boolean rollback()
    {
        return (sqlrcon_rollback(sqlrconref) == 1);
    }

    /** Returns the database-specific default isolation level,
     *  or null if an error occurred. */
    public String getDefaultIsolationLevel()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getDefaultIsolationLevel(sqlrconref));
    }

    /** Sets the isolation level to "isolationlevel", the database-secific
     *  isolation level.  Returns true if setting the isolation level
     *  succeeded, false if it failed. */
    public Boolean setIsolationLevel(String isolationlevel)
    {
        return (sqlrcon_setIsolationLevel(sqlrconref, isolationlevel) == 1);
    }

    /** Returns the database-specific isolation level, "unknown" if the
     *  isolation level is unknown, or null if an error occurred. */
    public String getIsolationLevel()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getIsolationLevel(sqlrconref));
    }

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
     *  Returns the value of the feature as a string, or null if
     *  an error occurred or an invalid feature was requested. */
    public String getDatabaseFeature(String feature)
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getDatabaseFeature(sqlrconref, feature));
    }



    /** If an operation failed and generated an error, the error message is
     *  available here.  If there is no error then this method returns NULL */
    public String errorMessage()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_errorMessage(sqlrconref));
    }
    
    /** If an operation failed and generated an error, the error number is
     *  available here.  If there is no error then this method returns 0. */
    public Int64 errorNumber()
    {
        return sqlrcon_errorNumber(sqlrconref);
    }
    
    
    /** Causes verbose debugging information to be sent to standard output.
     *  Another way to do this is to start a query with "-- debug\n".
     *  Yet another way is to set the environment variable SQLR_CLIENT_DEBUG
     *  to "ON" */
    public void debugOn()
    {
        sqlrcon_debugOn(sqlrconref);
    }
    
    /** Turns debugging off. */
    public void debugOff()
    {
        sqlrcon_debugOff(sqlrconref);
    }
    
    /** Returns false if debugging is off and true if debugging is on. */
    public Boolean getDebug()
    {
        return sqlrcon_getDebug(sqlrconref)!=0;
    }

    /** Allows you to specify a file to write debug to.
     *  Setting "filename" to NULL or an empty string causes debug
     *  to be written to standard output (the default). */
    public void setDebugFile(String filename)
    {
        sqlrcon_setDebugFile(sqlrconref,filename);
    }

    /** Allows you to set a string that will be passed to the server and
     *  ultimately included in server-side logging along with queries that were
     *  run by this instance of the client. */
    public void setClientInfo(String clientinfo)
    {
        sqlrcon_setClientInfo(sqlrconref,clientinfo);
    }

    /** Returns the string that was set by setClientInfo(). */
    public String getClientInfo()
    {
        return Marshal.PtrToStringAnsi(sqlrcon_getClientInfo(sqlrconref));
    }

    /** Returns a pointer to the internal connection structure */
    public IntPtr getInternalConnectionStructure()
    {
        return sqlrconref;
    }

    public static Boolean isYes(String str)
    {
        return (sqlrcon_isYes(str) != 0);
    }

    public static Boolean isNo(String str)
    {
        return (sqlrcon_isNo(str) != 0);
    }

    private IntPtr sqlrconref;

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_alloc_copyrefs(String server, UInt16 port, String socket, String user, String password, Int32 retrytime, Int32 tries, Int32 copyreferences);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_free(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_setConnectTimeout(IntPtr sqlrconref, Int32 timeoutsec, Int32 timeoutusec);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getConnectTimeoutSeconds(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getConnectTimeoutMicroseconds(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_setResponseTimeout(IntPtr sqlrconref, Int32 timeoutsec, Int32 timeoutusec);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getResponseTimeoutSeconds(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getResponseTimeoutMicroseconds(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_setBindVariableDelimiters(IntPtr sqlrconref, String delimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getBindVariableDelimiterQuestionMarkSupported(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getBindVariableDelimiterColonSupported(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getBindVariableDelimiterAtSignSupported(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getBindVariableDelimiterDollarSignSupported(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_enableKerberos(IntPtr sqlrconref, String service, String mech, String flags);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_enableTls(IntPtr sqlrconref, String versoin, String cert, String password, String ciphers, String validate, String ca, UInt16 depth);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_disableEncryption(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_endSession(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_suspendSession(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt16 sqlrcon_getConnectionPort(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getConnectionSocket(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_resumeSession(IntPtr sqlrconref, UInt16 port, String socket);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_ping(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_identify(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_dbVersion(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_dbHostName(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_dbIpAddress(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_serverVersion(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_clientVersion(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_bindFormat(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_nextvalFormat(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_selectDatabase(IntPtr sqlrconref, String database);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getCurrentDatabase(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getDatabaseIsSchema(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_selectCatalog(IntPtr sqlrconref, String catalog);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getCurrentCatalog(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_selectSchema(IntPtr sqlrconref, String schema);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getCurrentSchema(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getCurrentUser(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt64 sqlrcon_getLastInsertId(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_autoCommitOn(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_autoCommitOff(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getAutoCommit(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_begin(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_commit(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_rollback(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getDefaultIsolationLevel(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_setIsolationLevel(IntPtr sqlrconref, String isolationlevel);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getIsolationLevel(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getDatabaseFeature(IntPtr sqlrconref, String feature);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_errorMessage(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int64 sqlrcon_errorNumber(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_debugOn(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_debugOff(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_getDebug(IntPtr sqlrconref);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_setDebugFile(IntPtr sqlrconref, String filename);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcon_setClientInfo(IntPtr sqlrconref, String clientinfo);
    
    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcon_getClientInfo(IntPtr sqlrconref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_isYes(String str);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcon_isNo(String str);
}

}

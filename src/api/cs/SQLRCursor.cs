// Copyright (c) David Muse
// See the file COPYING for more information

using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using SQLRClient;

namespace SQLRClient
{

public class SQLRCursor : IDisposable
{

    /** Creates a cursor to run queries and fetch result
     *  sets using connecton "conn". */
    public SQLRCursor(SQLRConnection conn)
    {
        sqlrcurref = sqlrcur_alloc_copyrefs(conn.getInternalConnectionStructure(), 1);
    }

    private SQLRCursor(IntPtr sqlrcurref)
    {
        this.sqlrcurref = sqlrcurref;
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
            sqlrcur_free(sqlrcurref);
            disposed = true;
        }
    }

    /** Destroys the cursor and cleans up all associated result set data. */
    ~SQLRCursor()
    {
        Dispose(false);
    }



    /** Sets the number of rows of the result set to buffer at a time.
     *  0 (the default) means buffer the entire result set. */
    public void setResultSetBufferSize(UInt64 rows)
    {
        sqlrcur_setResultSetBufferSize(sqlrcurref, rows);
    }

    /** Returns the number of result set rows that will be buffered at a time or
     *  0 for the entire result set. */
    public UInt64 getResultSetBufferSize()
    {
        return sqlrcur_getResultSetBufferSize(sqlrcurref);
    }



    /** Tells the server not to send any column
     *  info (names, types, sizes).  If you don't
     *  need that info, you should call this
     *  method to improve performance. */
    public void dontGetColumnInfo()
    {
        sqlrcur_dontGetColumnInfo(sqlrcurref);
    }

    /** Tells the server to send column info. */
    public void getColumnInfo()
    {
        sqlrcur_getColumnInfo(sqlrcurref);
    }



    /** Columns names are returned in the same case as they are defined in the
     *  database.  This is the default. */
    public void mixedCaseColumnNames()
    {
        sqlrcur_mixedCaseColumnNames(sqlrcurref);
    }

    /** Columns names are converted to upper case. */
    public void upperCaseColumnNames()
    {
        sqlrcur_upperCaseColumnNames(sqlrcurref);
    }

    /** Columns names are converted to lower case. */
    public void lowerCaseColumnNames()
    {
        sqlrcur_lowerCaseColumnNames(sqlrcurref);
    }




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
    public void cacheToFile(String filename)
    {
        sqlrcur_cacheToFile(sqlrcurref, filename);
    }

    /** Sets the time-to-live for cached result sets. The sqlr-cachemanger will
     *  remove each cached result set "ttl" seconds after it's created, provided
     *  it's scanning the directory containing the cache files. */
    public void setCacheTtl(UInt32 ttl)
    {
        sqlrcur_setCacheTtl(sqlrcurref, ttl);
    }

    /** Returns the name of the file containing the
     *  cached result set. */
    public String getCacheFileName()
    {
        return Marshal.PtrToStringAnsi(sqlrcur_getCacheFileName(sqlrcurref));
    }

    /** Sets query caching off. */
    public void cacheOff()
    {
        sqlrcur_cacheOff(sqlrcurref);
    }



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
    public Boolean getDatabaseList(String databases)
    {
        return sqlrcur_getDatabaseList(sqlrcurref, databases) != 0;
    }

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
    public Boolean getCatalogList(String catalogs)
    {
        return sqlrcur_getCatalogList(sqlrcurref, catalogs) != 0;
    }

    /** Generates a result set containing
     *  schemas that match the pattern "schemas".
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
    public Boolean getSchemaList(String schemas)
    {
        return sqlrcur_getSchemaList(sqlrcurref, schemas) != 0;
    }

    /** Generates a result set containing
     *  supported table types.
     *
     *  The result set will contain the following columns:
     *  * table_type
     *
     *  If SQL Relay doesn't support getting a list of table types
     *  for the current database backend (or the database doesn't)
     *  then an empty result set will be returned. */
    public Boolean getTableTypeList()
    {
        return sqlrcur_getTableTypeList(sqlrcurref) != 0;
    }

    /** Generates a result set containing the
     *  tables in the current database and schema that match the
     *  pattern "tables".
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
    public Boolean getTableList(String tables)
    {
        return sqlrcur_getTableList(sqlrcurref, tables) != 0;
    }

    /** Generates a result set containing data
     *  type information for "type".
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
    public Boolean getTypeInfoList(String type)
    {
        return sqlrcur_getTypeInfoList(sqlrcurref, type) != 0;
    }

    /** Generates a result set containing the
     *  columns of "table", which match the pattern "columns".
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
    public Boolean getColumnList(String table, String columns)
    {
        return sqlrcur_getColumnList(sqlrcurref, table, columns) != 0;
    }

    /** Generates a result set containing the
     *  primary keys of "table", which match the pattern
     *  "columns".
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
    public Boolean getPrimaryKeysList(String table, String columns)
    {
        return sqlrcur_getPrimaryKeysList(sqlrcurref, table, columns) != 0;
    }

    /** Generates a result set containing the
     *  keys and indexes of "table", which match the pattern
     *  "qualifier".
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
    public Boolean getKeyAndIndexList(String table, String qualifier)
    {
        return sqlrcur_getKeyAndIndexList(sqlrcurref, table, qualifier) != 0;
    }

    /** Generates a result set containing
     *  procedures that match the pattern "procedures".
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
    public Boolean getProcedureList(String procedures)
    {
        return sqlrcur_getProcedureList(sqlrcurref, procedures) != 0;
    }

    /** Generates a result set containing the
     *  parameters of "procedure", which match the pattern
     *  "parameters".
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
    public Boolean getProcedureParameterList(String procedure, String parameters)
    {
        return sqlrcur_getProcedureParameterList(sqlrcurref, procedure, parameters) != 0;
    }



    /** Sends "query" directly and gets a result set. */
    public Boolean sendQuery(String query)
    {
        return sqlrcur_sendQueryWithLength(sqlrcurref, query, (uint)System.Text.Encoding.Default.GetByteCount(query)) != 0;
    }

    /** Sends "query" with length "length" directly
     *  and gets a result set. This method must be used
     *  if the query contains binary data. */
    public Boolean sendQuery(String query, UInt32 length)
    {
        return sqlrcur_sendQueryWithLength(sqlrcurref, query, length) != 0;
    }

    /** Sends "query" (a byte array) with length "length" directly and
     *  gets a result set.  This form is byte-safe - use it when the
     *  query contains bytes outside the ASCII range or embedded NUL. */
    public Boolean sendQuery(Byte[] query, UInt32 length)
    {
        return sqlrcur_sendQueryWithLengthBytes(sqlrcurref, query, length) != 0;
    }

    /** Sends the query in file "path"/"filename" directly
     *  and gets a result set. */
    public Boolean sendFileQuery(String path, String filename)
    {
        return sqlrcur_sendFileQuery(sqlrcurref, path, filename) != 0;
    }



    /** Prepare to execute "query". */
    public void prepareQuery(String query)
    {
        sqlrcur_prepareQueryWithLength(sqlrcurref, query, (uint)System.Text.Encoding.Default.GetByteCount(query));
    }

    /** Prepare to execute "query" with length
     *  "length".  This method must be used if the
     *  query contains binary data. */
    public void prepareQuery(String query, UInt32 length)
    {
        sqlrcur_prepareQueryWithLength(sqlrcurref, query, length);
    }

    /** Prepare to execute the contents of "path"/"filename". */
    public void prepareFileQuery(String path, String filename)
    {
        sqlrcur_prepareFileQuery(sqlrcurref, path, filename);
    }



    /** Defines a string substitution variable. */
    public void substitution(String variable, String val)
    {
        sqlrcur_subString(sqlrcurref, variable, val);
    }

    /** Defines an integer substitution variable. */
    public void substitution(String variable, Int64 val)
    {
        sqlrcur_subLong(sqlrcurref, variable, val);
    }

    /** Defines a decimal substitution variable. */
    public void substitution(String variable, Double val, UInt32 precision, UInt32 scale)
    {
        sqlrcur_subDouble(sqlrcurref, variable, val, precision, scale);
    }

    /** Defines an array of string substitution variables. */
    public void substitutions(String[] variables, String[] vals)
    {
        for (Int32 i = 0; i < variables.Length; i++)
        {
            sqlrcur_subString(sqlrcurref, variables[i], vals[i]);
        }
    }

    /** Defines an array of integer substitution variables. */
    public void substitution(String[] variables, Int64[] vals)
    {
        for (Int32 i = 0; i < variables.Length; i++)
        {
            sqlrcur_subLong(sqlrcurref, variables[i], vals[i]);
        }
    }

    /** Defines an array of decimal substitution variables. */
    public void substitution(String[] variables, Double[] vals, UInt32[] precisions, UInt32[] scales)
    {
        for (Int32 i = 0; i < variables.Length; i++)
        {
            sqlrcur_subDouble(sqlrcurref, variables[i], vals[i], precisions[i], scales[i]);
        }
    }



    /** Defines a string input bind variable. */
    public void inputBind(String variable, String val)
    {
        sqlrcur_inputBindStringWithLength(sqlrcurref, variable, val, (val != null) ? (uint)System.Text.Encoding.Default.GetByteCount(val) : 0);
    }

    /** Defines a string input bind variable. */
    public void inputBind(String variable, String val, UInt32 vallength)
    {
        sqlrcur_inputBindStringWithLength(sqlrcurref, variable, val, vallength);
    }

    /** Defines a integer input bind variable. */
    public void inputBind(String variable, Int64 val)
    {
        sqlrcur_inputBindLong(sqlrcurref, variable, val);
    }

    /** Defines a decimal input bind variable.
      * (If you don't have the precision and scale then set
      * them both 0.  However in that case you may get
      * unexpected rounding behavior if the server is faking
      * binds.) */
    public void inputBind(String variable, Double val, UInt32 precision, UInt32 scale)
    {
        sqlrcur_inputBindDouble(sqlrcurref, variable, val, precision, scale);
    }

    /** Defines an array of string input bind variables. */
    public void inputBind(String[] variables, String[] vals)
    {
        for (UInt32 i = 0; i < variables.Length; i++)
        {
            sqlrcur_inputBindString(sqlrcurref, variables[i], vals[i]);
        }
    }

    /** Defines an array of integer input bind variables. */
    public void inputBind(String[] variables, Int64[] vals)
    {
        for (UInt32 i = 0; i < variables.Length; i++)
        {
            sqlrcur_inputBindLong(sqlrcurref, variables[i], vals[i]);
        }
    }

    /** Defines an array of decimal input bind variables. */
    public void inputBinds(String[] variables, Double[] vals, UInt32[] precisions, UInt32[] scales)
    {
        for (UInt32 i = 0; i < variables.Length; i++)
        {
            sqlrcur_inputBindDouble(sqlrcurref, variables[i], vals[i], precisions[i], scales[i]);
        }
    }

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
    public void inputBind(String variable, Int16 year, Int16 month, Int16 day, Int16 hour, Int16 minute, Int16 second, Int32 microsecond, String tz, Boolean isnegative)
    {
        sqlrcur_inputBindDate(sqlrcurref, variable, year, month, day, hour, minute, second, microsecond, tz, (isnegative) ? 1 : 0);
    }

    /** Defines a binary lob input bind variable. */
    public void inputBindBlob(String variable, Byte[] val, UInt32 size)
    {
        sqlrcur_inputBindBlob(sqlrcurref, variable, val, size);
    }

    /** Defines a character lob input bind variable. */
    public void inputBindClob(String variable, String val, UInt32 size)
    {
        sqlrcur_inputBindClob(sqlrcurref, variable, val, size);
    }



    /** Defines an output bind variable.
     *  "length" bytes will be reserved
     *  to store the value. */
    public void defineOutputBindString(String variable, UInt32 length)
    {
        sqlrcur_defineOutputBindString(sqlrcurref, variable, length);
    }

    /** Defines an integer output bind variable. */
    public void defineOutputBindInteger(String variable)
    {
        sqlrcur_defineOutputBindInteger(sqlrcurref, variable);
    }

    /** Defines a decimal output bind variable. */
    public void defineOutputBindDouble(String variable)
    {
        sqlrcur_defineOutputBindDouble(sqlrcurref, variable);
    }

    /** Defines a date output bind variable. */
    public void defineOutputBindDate(String variable)
    {
        sqlrcur_defineOutputBindDate(sqlrcurref, variable);
    }

    /** Defines a binary lob output bind variable. */
    public void defineOutputBindBlob(String variable)
    {
        sqlrcur_defineOutputBindBlob(sqlrcurref, variable);
    }

    /** Defines a character lob output bind variable. */
    public void defineOutputBindClob(String variable)
    {
        sqlrcur_defineOutputBindClob(sqlrcurref, variable);
    }

    /** Defines a cursor output bind variable. */
    public void defineOutputBindCursor(String variable)
    {
        sqlrcur_defineOutputBindCursor(sqlrcurref, variable);
    }



    /** Clears all bind variables. */
    public void clearBinds()
    {
        sqlrcur_clearBinds(sqlrcurref);
    }

    /** Parses the previously prepared query, counts the number of bind
     *  variables defined in it and returns that number. */
    public UInt16 countBindVariables()
    {
        return sqlrcur_countBindVariables(sqlrcurref);
    }

    /** If you are binding to any variables that
     *  might not actually be in your query, call
     *  this to ensure that the database won't try
     *  to bind them unless they really are in the
     *  query.  There is a performance penalty for
     *  calling this method. */
    public void validateBinds()
    {
        sqlrcur_validateBinds(sqlrcurref);
    }

    /** Returns true if "variable" was a valid bind variable of the query. */
    public Boolean validBind(String variable)
    {
        return sqlrcur_validBind(sqlrcurref, variable) != 0;
    }



    /** Execute the query that was previously prepared and bound. */
    public Boolean executeQuery()
    {
        return sqlrcur_executeQuery(sqlrcurref) != 0;
    }

    /** Fetch from a cursor that was returned as an output bind variable. */
    public Boolean fetchFromBindCursor()
    {
        return sqlrcur_fetchFromBindCursor(sqlrcurref) != 0;
    }



    /** Get the value stored in a previously defined
     *  String output bind variable. */
    public String getOutputBindString(String variable)
    {
        return Marshal.PtrToStringAnsi(sqlrcur_getOutputBindString(sqlrcurref, variable));
    }

    /** Get the value stored in a previously defined
     *  integer output bind variable. */
    public Int64 getOutputBindInteger(String variable)
    {
        return sqlrcur_getOutputBindInteger(sqlrcurref, variable);
    }

    /** Get the value stored in a previously defined
     *  decimal output bind variable. */
    public double getOutputBindDouble(String variable)
    {
        return sqlrcur_getOutputBindDouble(sqlrcurref, variable);
    }

    /** Get the value stored in a previously
     *  defined date output bind variable. */
    public Boolean getOutputBindDate(String variable, out Int16 year, out Int16 month, out Int16 day, out Int16 hour, out Int16 minute, out Int16 second, out Int32 microsecond, out String tz, out Boolean isnegative)
    {
        year = -1;
        month = -1;
        day = -1;
        hour = -1;
        minute = -1;
        second = -1;
        microsecond = -1;
        tz = "";
        isnegative = false;
        IntPtr tzptr = (IntPtr)0;
        Int32 isneg = 0;
        sqlrcur_getOutputBindDate(sqlrcurref, variable, ref year, ref month, ref day, ref hour, ref minute, ref second, ref microsecond, ref tzptr, ref isneg);
        tz = Marshal.PtrToStringAnsi(tzptr);
        isnegative = (isneg!=0);
        return false;
    }

    /** Get the year from a previously defined
     *  date output bind variable. */
    public Int16 getOutputBindDateYear(String variable)
    {
        return sqlrcur_getOutputBindDateYear(sqlrcurref, variable);
    }

    /** Get the month from a previously defined
     *  date output bind variable. */
    public Int16 getOutputBindDateMonth(String variable)
    {
        return sqlrcur_getOutputBindDateMonth(sqlrcurref, variable);
    }

    /** Get the day from a previously defined
     *  date output bind variable. */
    public Int16 getOutputBindDateDay(String variable)
    {
        return sqlrcur_getOutputBindDateDay(sqlrcurref, variable);
    }

    /** Get the hour from a previously defined
     *  date output bind variable. */
    public Int16 getOutputBindDateHour(String variable)
    {
        return sqlrcur_getOutputBindDateHour(sqlrcurref, variable);
    }

    /** Get the minute from a previously defined
     *  date output bind variable. */
    public Int16 getOutputBindDateMinute(String variable)
    {
        return sqlrcur_getOutputBindDateMinute(sqlrcurref, variable);
    }

    /** Get the second from a previously defined
     *  date output bind variable. */
    public Int16 getOutputBindDateSecond(String variable)
    {
        return sqlrcur_getOutputBindDateSecond(sqlrcurref, variable);
    }

    /** Get the microsecond from a previously defined
     *  date output bind variable. */
    public Int32 getOutputBindDateMicrosecond(String variable)
    {
        return sqlrcur_getOutputBindDateMicrosecond(sqlrcurref, variable);
    }

    /** Get the time zone from a previously
     *  defined date output bind variable. */
    public String getOutputBindDateTz(String variable)
    {
        return Marshal.PtrToStringAnsi(sqlrcur_getOutputBindDateTz(sqlrcurref, variable));
    }

    /** Get whether the value is negative from a
     *  previously defined date output bind variable. */
    public Boolean getOutputBindDateIsNegative(String variable)
    {
        return sqlrcur_getOutputBindDateIsNegative(sqlrcurref, variable) != 0;
    }

    /** Get the value stored in a previously defined
     *  binary lob output bind variable. */
    public Byte[] getOutputBindBlob(String variable)
    {
        Int32 size = (Int32)sqlrcur_getOutputBindLength(sqlrcurref, variable);
        if (size == 0)
        {
            return null;
        }
        Byte[] retval = new Byte[size];
        Marshal.Copy(sqlrcur_getOutputBindBlob(sqlrcurref, variable), retval, 0, size);
        return retval;
    }

    /** Get the value stored in a previously defined
     *  character lob output bind variable. */
    public String getOutputBindClob(String variable)
    {
        return Marshal.PtrToStringAnsi(sqlrcur_getOutputBindClob(sqlrcurref, variable));
    }

    /** Get the length of the value stored in a previously
     *  defined output bind variable. */
    public UInt32 getOutputBindLength(String variable)
    {
        return sqlrcur_getOutputBindLength(sqlrcurref, variable);
    }

    /** Get the cursor associated with a previously defined output bind
     *  variable. */
    public SQLRCursor getOutputBindCursor(String variable)
    {
        return new SQLRCursor(sqlrcur_getOutputBindCursor_copyrefs(sqlrcurref, variable, 1));
    }



    /** Opens a cached result set.
     *  Returns true on success and false on failure. */
    public Boolean openCachedResultSet(String filename)
    {
        return sqlrcur_openCachedResultSet(sqlrcurref, filename)!=0;
    }



    /** Returns the number of columns in the current result set. */
    public UInt32 colCount()
    {
        return sqlrcur_colCount(sqlrcurref);
    }

    /** Returns the number of rows in the current
     *  result set (if the result set is being
     *  stepped through, this returns the number
     *  of rows processed so far). */
    public UInt64 rowCount()
    {
        return sqlrcur_rowCount(sqlrcurref);
    }

    /** Returns the total number of rows that will
     *  be returned in the result set.  Not all
     *  databases support this call.  Don't use it
     *  for applications which are designed to be
     *  portable across databases.  0 is returned
     *  by databases which don't support this option. */
    public UInt64 totalRows()
    {
        return sqlrcur_totalRows(sqlrcurref);
    }

    /** Returns the number of rows that were
     *  updated, inserted or deleted by the query.
     *  Not all databases support this call.  Don't
     *  use it for applications which are designed
     *  to be portable across databases.  0 is
     *  returned by databases which don't support
     *  this option. */
    public UInt64 affectedRows()
    {
        return sqlrcur_affectedRows(sqlrcurref);
    }

    /** Returns the index of the first buffered row.  This is useful when
     *  buffering only part of the result set at a time. */
    public UInt64 firstRowIndex()
    {
        return sqlrcur_firstRowIndex(sqlrcurref);
    }

    /** Returns false if part of the result set is
     *  still pending on the server and true if not.
     *  This method can only return false if
     *  setResultSetBufferSize() has been called
     *  with a parameter other than 0. */
    public Boolean endOfResultSet()
    {
        return sqlrcur_endOfResultSet(sqlrcurref)!=0;
    }

    /** Returns true and acts like executeQuery() when there is another result
     *  set available from the server. */
    public Boolean nextResultSet()
    {
        return sqlrcur_nextResultSet(sqlrcurref)!=0;
    }



    /** If a query failed and generated an error,
     *  the error message is available here.  If
     *  the query succeeded then this method
     *  returns NULL. */
    public String errorMessage()
    {
        return Marshal.PtrToStringAnsi(sqlrcur_errorMessage(sqlrcurref));
    }

    /** If a query failed and generated an error, the error number is available
     *  here.  If there is no error then this method returns 0. */
    public Int64 errorNumber()
    {
        return sqlrcur_errorNumber(sqlrcurref);
    }


    /** Tells the connection to return NULL fields and output bind variables as
     *  empty strings.  This is the default. */
    public void getNullsAsEmptyStrings()
    {
        sqlrcur_getNullsAsEmptyStrings(sqlrcurref);
    }

    /** Tells the connection to return NULL fields
     *  and output bind variables as NULL's rather
     *  than as empty strings. */
    public void getNullsAsNulls()
    {
        sqlrcur_getNullsAsNulls(sqlrcurref);
    }



    /** Returns the specified field as a string. */
    public String getField(UInt64 row, UInt32 col)
    {
        // if we're getting nulls as nulls or we've run off the end of the result set,
        // return a null for a null field
        if (sqlrcur_getFieldByIndex(sqlrcurref, row, col) == IntPtr.Zero)
        {
            return null;
        }

        // if we're getting nulls as empty strings, return an empty string for a null field
        Byte[] field = getFieldAsByteArray(row,col);
        if (field == null)
        {
            return "";
        }

        // if we didn't get a null field, return an actual string
        return System.Text.Encoding.Default.GetString(field);
    }

    /** Returns the specified field as a string. */
    public String getField(UInt64 row, String col)
    {
        // if we're getting nulls as nulls or we've run off the end of the result set,
        // return a null for a null field
        if (sqlrcur_getFieldByName(sqlrcurref, row, col) == IntPtr.Zero)
        {
            return null;
        }

        // if we're getting nulls as empty strings, return an empty string for a null field
        Byte[] field = getFieldAsByteArray(row, col);
        if (field == null)
        {
            return "";
        }

        // if we didn't get a null field, return an actual string
        return System.Text.Encoding.Default.GetString(field);
    }

    /** Returns the specified field as a string, ignoring the case of "col". */
    public String getFieldIgnoringCase(UInt64 row, String col)
    {
        // if we're getting nulls as nulls or we've run off the end of the result set,
        // return a null for a null field
        IntPtr ptr = sqlrcur_getFieldByNameIgnoringCase(sqlrcurref, row, col);
        if (ptr == IntPtr.Zero)
        {
            return null;
        }

        // get the field length to handle binary data
        Int32 size = (Int32)sqlrcur_getFieldLengthByName(sqlrcurref, row, col);
        if (size == 0)
        {
            return "";
        }

        // copy the field data
        Byte[] field = new Byte[size];
        Marshal.Copy(ptr, field, 0, size);
        return System.Text.Encoding.Default.GetString(field);
    }

    /** Returns the specified field as an integer. */
    public Int64 getFieldAsInteger(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsIntegerByIndex(sqlrcurref, row, col);
    }

    /** Returns the specified field as an integer. */
    public Int64 getFieldAsInteger(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsIntegerByName(sqlrcurref, row, col);
    }

    /** Returns the specified field as an integer, ignoring the case of "col". */
    public Int64 getFieldAsIntegerIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsIntegerByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Returns the specified field as a decimal. */
    public Double getFieldAsDouble(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDoubleByIndex(sqlrcurref, row, col);
    }

    /** Returns the specified field as a decimal. */
    public Double getFieldAsDouble(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDoubleByName(sqlrcurref, row, col);
    }

    /** Returns the specified field as a boolean. */
    public Boolean getFieldAsBoolean(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsBooleanByIndex(sqlrcurref, row, col) != 0;
    }

    /** Returns the specified field as a boolean. */
    public Boolean getFieldAsBoolean(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsBooleanByName(sqlrcurref, row, col) != 0;
    }

    /** Returns the specified field as a boolean, ignoring the case of "col". */
    public Boolean getFieldAsBooleanIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsBooleanByNameIgnoringCase(sqlrcurref, row, col) != 0;
    }

    /** Interprets the specified field as a date and returns the year component. */
    public Int16 getFieldAsDateYear(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateYearByIndex(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the year component. */
    public Int16 getFieldAsDateYear(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateYearByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the year component. */
    public Int16 getFieldAsDateYear(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateYearByName(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the year component, ignoring the case of "col". */
    public Int16 getFieldAsDateYearIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateYearByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the year component. */
    public Int16 getFieldAsDateYear(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateYearByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the year component, ignoring the case of "col". */
    public Int16 getFieldAsDateYearIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateYearByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the month component. */
    public Int16 getFieldAsDateMonth(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateMonthByIndex(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the month component. */
    public Int16 getFieldAsDateMonth(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMonthByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the month component. */
    public Int16 getFieldAsDateMonth(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateMonthByName(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the month component, ignoring the case of "col". */
    public Int16 getFieldAsDateMonthIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateMonthByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the month component. */
    public Int16 getFieldAsDateMonth(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMonthByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the month component, ignoring the case of "col". */
    public Int16 getFieldAsDateMonthIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMonthByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the day component. */
    public Int16 getFieldAsDateDay(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateDayByIndex(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the day component. */
    public Int16 getFieldAsDateDay(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateDayByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the day component. */
    public Int16 getFieldAsDateDay(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateDayByName(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the day component, ignoring the case of "col". */
    public Int16 getFieldAsDateDayIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateDayByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the day component. */
    public Int16 getFieldAsDateDay(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateDayByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the day component, ignoring the case of "col". */
    public Int16 getFieldAsDateDayIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateDayByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the hour component. */
    public Int16 getFieldAsDateHour(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateHourByIndex(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the hour component. */
    public Int16 getFieldAsDateHour(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateHourByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the hour component. */
    public Int16 getFieldAsDateHour(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateHourByName(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the hour component, ignoring the case of "col". */
    public Int16 getFieldAsDateHourIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateHourByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the hour component. */
    public Int16 getFieldAsDateHour(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateHourByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the hour component, ignoring the case of "col". */
    public Int16 getFieldAsDateHourIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateHourByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the minute component. */
    public Int16 getFieldAsDateMinute(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateMinuteByIndex(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the minute component. */
    public Int16 getFieldAsDateMinute(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMinuteByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the minute component. */
    public Int16 getFieldAsDateMinute(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateMinuteByName(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the minute component, ignoring the case of "col". */
    public Int16 getFieldAsDateMinuteIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateMinuteByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the minute component. */
    public Int16 getFieldAsDateMinute(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMinuteByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the minute component, ignoring the case of "col". */
    public Int16 getFieldAsDateMinuteIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMinuteByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the second component. */
    public Int16 getFieldAsDateSecond(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateSecondByIndex(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the second component. */
    public Int16 getFieldAsDateSecond(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateSecondByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the second component. */
    public Int16 getFieldAsDateSecond(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateSecondByName(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the second component, ignoring the case of "col". */
    public Int16 getFieldAsDateSecondIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateSecondByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the second component. */
    public Int16 getFieldAsDateSecond(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateSecondByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the second component, ignoring the case of "col". */
    public Int16 getFieldAsDateSecondIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateSecondByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the microsecond component. */
    public Int32 getFieldAsDateMicrosecond(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateMicrosecondByIndex(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the microsecond component. */
    public Int32 getFieldAsDateMicrosecond(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMicrosecondByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the microsecond component. */
    public Int32 getFieldAsDateMicrosecond(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateMicrosecondByName(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the microsecond component, ignoring the case of "col". */
    public Int32 getFieldAsDateMicrosecondIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateMicrosecondByNameIgnoringCase(sqlrcurref, row, col);
    }

    /** Interprets the specified field as a date and returns the microsecond component. */
    public Int32 getFieldAsDateMicrosecond(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMicrosecondByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date and returns the microsecond component, ignoring the case of "col". */
    public Int32 getFieldAsDateMicrosecondIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters);
    }

    /** Interprets the specified field as a date
     *  and returns whether the hour component
     *  is negative. */
    public Boolean getFieldAsDateIsNegative(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldAsDateIsNegativeByIndex(sqlrcurref, row, col) != 0;
    }

    /** Interprets the specified field as a date
     *  and returns whether the hour component
     *  is negative. */
    public Boolean getFieldAsDateIsNegative(UInt64 row, UInt32 col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateIsNegativeByIndexWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters) != 0;
    }

    /** Interprets the specified field as a date
     *  and returns whether the hour component
     *  is negative. */
    public Boolean getFieldAsDateIsNegative(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateIsNegativeByName(sqlrcurref, row, col) != 0;
    }

    /** Interprets the specified field as a date
     *  and returns whether the hour component
     *  is negative, ignoring the case of "col". */
    public Boolean getFieldAsDateIsNegativeIgnoringCase(UInt64 row, String col)
    {
        return sqlrcur_getFieldAsDateIsNegativeByNameIgnoringCase(sqlrcurref, row, col) != 0;
    }

    /** Interprets the specified field as a date
     *  and returns whether the hour component
     *  is negative. */
    public Boolean getFieldAsDateIsNegative(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateIsNegativeByNameWithDdMm(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters) != 0;
    }

    /** Interprets the specified field as a date
     *  and returns whether the hour component
     *  is negative, ignoring the case of "col". */
    public Boolean getFieldAsDateIsNegativeIgnoringCase(UInt64 row, String col, Boolean ddmm, Boolean yyyyddmm, String datedelimiters)
    {
        return sqlrcur_getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase(sqlrcurref, row, col, (ddmm) ? 1 : 0, (yyyyddmm) ? 1 : 0, datedelimiters) != 0;
    }

    /** Returns the specified field as a string. */
    public Byte[] getFieldAsByteArray(UInt64 row, UInt32 col)
    {
        Int32 size = (Int32)sqlrcur_getFieldLengthByIndex(sqlrcurref, row, col);
        if (size == 0)
        {
            return null;
        }
        Byte[] retval = new Byte[size];
        Marshal.Copy(sqlrcur_getFieldByIndex(sqlrcurref, row, col), retval, 0, size);
        return retval;
    }

    /** Returns the specified field as a string. */
    public Byte[] getFieldAsByteArray(UInt64 row, String col)
    {
        Int32 size = (Int32)sqlrcur_getFieldLengthByName(sqlrcurref, row, col);
        if (size == 0)
        {
            return null;
        }
        Byte[] retval = new Byte[size];
        Marshal.Copy(sqlrcur_getFieldByName(sqlrcurref, row, col), retval, 0, size);
        return retval;
    }



    /** Returns the length of the specified field. */
    public UInt32 getFieldLength(UInt64 row, UInt32 col)
    {
        return sqlrcur_getFieldLengthByIndex(sqlrcurref, row, col);
    }

    /** Returns the length of the specified field. */
    public UInt32 getFieldLength(UInt64 row, String col)
    {
        return sqlrcur_getFieldLengthByName(sqlrcurref, row, col);
    }

    /** Returns an array of the values of the fields in the
     *  specified row, or null if the row is past the end of the
     *  result set. */
    public String[] getRow(UInt64 row)
    {
        // bail if past the end of the result set
        if (sqlrcur_getRow(sqlrcurref, row) == IntPtr.Zero)
        {
            return null;
        }
        UInt32 colcount = sqlrcur_colCount(sqlrcurref);
        String[] retval = new String[colcount];
        for (UInt32 i = 0; i < colcount; i++)
        {
            retval[i] = getField(row, i);
        }
        return retval;
    }

    /** Returns an array of the lengths of the fields in the
     *  specified row, or null if the row is past the end of the
     *  result set. */
    public UInt32[] getRowLengths(UInt64 row)
    {
        if (sqlrcur_getRowLengths(sqlrcurref, row) == IntPtr.Zero)
        {
            return null;
        }
        UInt32 colcount = sqlrcur_colCount(sqlrcurref);
        UInt32[] retval = new UInt32[colcount];
        for (UInt32 i = 0; i < colcount; i++)
        {
            retval[i] = getFieldLength(row, i);
        }
        return retval;
    }

    /** Returns the column name list of the current
     *  result set. */
    public String[] getColumnNames()
    {
        UInt32 colcount = sqlrcur_colCount(sqlrcurref);
        String[] retval = new String[colcount];
        for (UInt32 i = 0; i < colcount; i++)
        {
            retval[i] = getColumnName(i);
        }
        return retval;
    }

    /** Returns the name of the specified column. */
    public String getColumnName(UInt32 col)
    {
        return Marshal.PtrToStringAnsi(sqlrcur_getColumnName(sqlrcurref, col));
    }

    /** Returns the type of the specified column. */
    public String getColumnType(UInt32 col)
    {
        return Marshal.PtrToStringAnsi(sqlrcur_getColumnTypeByIndex(sqlrcurref, col));
    }

    /** Returns the type of the specified column. */
    public String getColumnType(String col)
    {
        return Marshal.PtrToStringAnsi(sqlrcur_getColumnTypeByName(sqlrcurref, col));
    }

    /** Returns the number of bytes required on
     *  the server to store the data for the specified column */
    public UInt32 getColumnLength(UInt32 col)
    {
        return sqlrcur_getColumnLengthByIndex(sqlrcurref, col);
    }

    /** Returns the number of bytes required on
     *  the server to store the data for the specified column */
    public UInt32 getColumnLength(String col)
    {
        return sqlrcur_getColumnLengthByName(sqlrcurref, col);
    }

    /** Returns the precision of the specified
     *  column.
     *  Precision is the total number of digits in
     *  a number.  eg: 123.45 has a precision of 5.
     *  For non-numeric types, it's the number of
     *  characters in the string. */
    public UInt32 getColumnPrecision(UInt32 col)
    {
        return sqlrcur_getColumnPrecisionByIndex(sqlrcurref, col);
    }

    /** Returns the precision of the specified
     *  column.
     *  Precision is the total number of digits in
     *  a number.  eg: 123.45 has a precision of 5.
     *  For non-numeric types, it's the number of
     *  characters in the string. */
    public UInt32 getColumnPrecision(String col)
    {
        return sqlrcur_getColumnPrecisionByName(sqlrcurref, col);
    }

    /** Returns the scale of the specified column.
     *  Scale is the total number of digits to the
     *  right of the decimal point in a number.
     *  eg: 123.45 has a scale of 2. */
    public UInt32 getColumnScale(UInt32 col)
    {
        return sqlrcur_getColumnScaleByIndex(sqlrcurref, col);
    }

    /** Returns the scale of the specified column.
     *  Scale is the total number of digits to the
     *  right of the decimal point in a number.
     *  eg: 123.45 has a scale of 2. */
    public UInt32 getColumnScale(String col)
    {
        return sqlrcur_getColumnScaleByName(sqlrcurref, col);
    }

    /** Returns true if the specified column can contain
     *  nulls and false otherwise. */
    public Boolean getColumnIsNullable(UInt32 col)
    {
        return sqlrcur_getColumnIsNullableByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column can contain
     *  nulls and false otherwise. */
    public Boolean getColumnIsNullable(String col)
    {
        return sqlrcur_getColumnIsNullableByName(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is a
     *  primary key and false otherwise. */
    public Boolean getColumnIsPrimaryKey(UInt32 col)
    {
        return sqlrcur_getColumnIsPrimaryKeyByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is a
     *  primary key and false otherwise. */
    public Boolean getColumnIsPrimaryKey(String col)
    {
        return sqlrcur_getColumnIsPrimaryKeyByName(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is unique and false otherwise. */
    public Boolean getColumnIsUnique(UInt32 col)
    {
        return sqlrcur_getColumnIsUniqueByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is unique and false otherwise. */
    public Boolean getColumnIsUnique(String col)
    {
        return sqlrcur_getColumnIsUniqueByName(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is part of a composite key and
     *  false otherwise. */
    public Boolean getColumnIsPartOfKey(UInt32 col)
    {
        return sqlrcur_getColumnIsPartOfKeyByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is part of a composite key and
     *  false otherwise. */
    public Boolean getColumnIsPartOfKey(String col)
    {
        return sqlrcur_getColumnIsPartOfKeyByName(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is an unsigned number and false
     *  otherwise. */
    public Boolean getColumnIsUnsigned(UInt32 col)
    {
        return sqlrcur_getColumnIsUnsignedByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column is an unsigned number and false
     *  otherwise. */
    public Boolean getColumnIsUnsigned(String col)
    {
        return sqlrcur_getColumnIsUnsignedByName(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column was created
     *  with the zero-fill flag and false otherwise. */
    public Boolean getColumnIsZeroFilled(UInt32 col)
    {
        return sqlrcur_getColumnIsZeroFilledByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column was created
     *  with the zero-fill flag and false otherwise. */
    public Boolean getColumnIsZeroFilled(String col)
    {
        return sqlrcur_getColumnIsZeroFilledByName(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column contains binary data and false
     *  otherwise. */
    public Boolean getColumnIsBinary(UInt32 col)
    {
        return sqlrcur_getColumnIsBinaryByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column contains binary data and false
     *  otherwise. */
    public Boolean getColumnIsBinary(String col)
    {
        return sqlrcur_getColumnIsBinaryByName(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column auto-increments
     *  and false otherwise. */
    public Boolean getColumnIsAutoIncrement(UInt32 col)
    {
        return sqlrcur_getColumnIsAutoIncrementByIndex(sqlrcurref, col)!=0;
    }

    /** Returns true if the specified column auto-increments
     *  and false otherwise. */
    public Boolean getColumnIsAutoIncrement(String col)
    {
        return sqlrcur_getColumnIsAutoIncrementByName(sqlrcurref, col)!=0;
    }

    /** Returns the length of the longest field in the specified column. */
    public UInt32 getLongest(UInt32 col)
    {
        return sqlrcur_getLongestByIndex(sqlrcurref, col);
    }

    /** Returns the length of the longest field in the specified column. */
    public UInt32 getLongest(String col)
    {
        return sqlrcur_getLongestByName(sqlrcurref, col);
    }



    /** Tells the server to leave this result
     *  set open when the connection calls
     *  suspendSession() so that another connection
     *  can connect to it using resumeResultSet()
     *  after it calls resumeSession(). */
    public void suspendResultSet()
    {
        sqlrcur_suspendResultSet(sqlrcurref);
    }

    /** Returns the internal ID of this result set.
     *  This parameter may be passed to another
     *  cursor for use in the resumeResultSet()
     *  method.
     *  Note: The value this method returns is only
     *  valid after a call to suspendResultSet(). */
    public UInt16 getResultSetId()
    {
        return sqlrcur_getResultSetId(sqlrcurref);
    }

    /** Resumes a result set previously left open using suspendSession().
     *  Returns true on success and false on failure. */
    public Boolean resumeResultSet(UInt16 id)
    {
        return sqlrcur_resumeResultSet(sqlrcurref, id)!=0;
    }

    /** Resumes a result set previously left open
     *  using suspendSession() and continues caching
     *  the result set to "filename".
     *  Returns true on success and false on failure. */
    public Boolean resumeCachedResultSet(UInt16 id, String filename)
    {
        return sqlrcur_resumeCachedResultSet(sqlrcurref, id, filename)!=0;
    }

    /** Closes the current result set, if one is open.  Data
     *  that has been fetched already is still available but
     *  no more data may be fetched.  Server side resources
     *  for the result set are freed as well. */
    public void closeResultSet()
    {
        sqlrcur_closeResultSet(sqlrcurref);
    }

    private IntPtr sqlrcurref;

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_alloc_copyrefs(IntPtr sqlrconref, Int32 copyrefs);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_free(IntPtr sqlrcurref);


    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_setResultSetBufferSize(IntPtr sqlrcurref, UInt64 rows);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt64 sqlrcur_getResultSetBufferSize(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_dontGetColumnInfo(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_getColumnInfo(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_mixedCaseColumnNames(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_upperCaseColumnNames(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_lowerCaseColumnNames(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_cacheToFile(IntPtr sqlrcurref, String filename);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_setCacheTtl(IntPtr sqlrcurref, UInt32 ttl);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getCacheFileName(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_cacheOff(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getDatabaseList(IntPtr sqlrcurref, String databases);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getCatalogList(IntPtr sqlrcurref, String catalogs);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getSchemaList(IntPtr sqlrcurref, String schemas);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getTableTypeList(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getTableList(IntPtr sqlrcurref, String tables);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getTypeInfoList(IntPtr sqlrcurref, String type);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnList(IntPtr sqlrcurref, String table, String columns);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getPrimaryKeysList(IntPtr sqlrcurref, String table, String columns);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getKeyAndIndexList(IntPtr sqlrcurref, String table, String qualifier);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getProcedureList(IntPtr sqlrcurref, String procedures);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getProcedureParameterList(IntPtr sqlrcurref, String procedure, String parameters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_sendQuery(IntPtr sqlrcurref, String query);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_sendQueryWithLength(IntPtr sqlrcurref, String query, UInt32 length);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "sqlrcur_sendQueryWithLength")]
    private static extern Int32 sqlrcur_sendQueryWithLengthBytes(IntPtr sqlrcurref, Byte[] query, UInt32 length);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_sendFileQuery(IntPtr sqlrcurref, String path, String filename);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_prepareQuery(IntPtr sqlrcurref, String query);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_prepareQueryWithLength(IntPtr sqlrcurref, String query, UInt32 length);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_prepareFileQuery(IntPtr sqlrcurref, String path, String filename);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_subString(IntPtr sqlrcurref, String variable, String val);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_subLong(IntPtr sqlrcurref, String variable, Int64 val);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_subDouble(IntPtr sqlrcurref, String variable, Double val, UInt32 precision, UInt32 scale);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_inputBindString(IntPtr sqlrcurref, String variable, String val);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_inputBindStringWithLength(IntPtr sqlrcurref, String variable, String val, UInt32 vallength);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_inputBindLong(IntPtr sqlrcurref, String variable, Int64 val);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_inputBindDouble(IntPtr sqlrcurref, String variable, Double val, UInt32 precision, UInt32 scale);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_inputBindDate(IntPtr sqlrcurref, String variable, Int16 year, Int16 month, Int16 day, Int16 hour, Int16 minute, Int16 second, Int32 microsecond, String tz, Int32 isnegative);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_inputBindBlob(IntPtr sqlrcurref, String variable, Byte[] val, UInt32 size);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_inputBindClob(IntPtr sqlrcurref, String variable, String val, UInt32 size);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_defineOutputBindString(IntPtr sqlrcurref, String variable, UInt32 length);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_defineOutputBindInteger(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_defineOutputBindDouble(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_defineOutputBindDate(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_defineOutputBindBlob(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_defineOutputBindClob(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_defineOutputBindCursor(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_clearBinds(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt16 sqlrcur_countBindVariables(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_validateBinds(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_validBind(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_executeQuery(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_fetchFromBindCursor(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getOutputBindString(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int64 sqlrcur_getOutputBindInteger(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Double sqlrcur_getOutputBindDouble(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getOutputBindDate(IntPtr sqlrcurref, String variable, ref Int16 year, ref Int16 month, ref Int16 day, ref Int16 hour, ref Int16 minute, ref Int16 second, ref Int32 microsecond, ref IntPtr tz, ref Int32 isnegative);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getOutputBindDateYear(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getOutputBindDateMonth(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getOutputBindDateDay(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getOutputBindDateHour(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getOutputBindDateMinute(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getOutputBindDateSecond(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getOutputBindDateMicrosecond(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getOutputBindDateTz(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getOutputBindDateIsNegative(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getOutputBindBlob(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getOutputBindClob(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getOutputBindLength(IntPtr sqlrcurref, String variable);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getOutputBindCursor_copyrefs(IntPtr sqlrcurref, String variable, Int32 copyrefs);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_openCachedResultSet(IntPtr sqlrcurref, String filename);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_colCount(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt64 sqlrcur_rowCount(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt64 sqlrcur_totalRows(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt64 sqlrcur_affectedRows(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt64 sqlrcur_firstRowIndex(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_endOfResultSet(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_nextResultSet(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_errorMessage(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int64 sqlrcur_errorNumber(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_getNullsAsEmptyStrings(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_getNullsAsNulls(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getFieldByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getRow(IntPtr sqlrcurref, UInt64 row);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getRowLengths(IntPtr sqlrcurref, UInt64 row);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getFieldByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getFieldByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int64 sqlrcur_getFieldAsIntegerByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int64 sqlrcur_getFieldAsIntegerByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int64 sqlrcur_getFieldAsIntegerByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Double sqlrcur_getFieldAsDoubleByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Double sqlrcur_getFieldAsDoubleByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Double sqlrcur_getFieldAsDoubleByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsBooleanByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsBooleanByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsBooleanByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateYearByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateYearByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateYearByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateYearByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateYearByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateYearByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMonthByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMonthByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMonthByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMonthByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMonthByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMonthByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateDayByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateDayByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateDayByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateDayByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateDayByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateDayByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateHourByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateHourByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateHourByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateHourByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateHourByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateHourByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMinuteByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMinuteByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMinuteByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMinuteByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMinuteByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateMinuteByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateSecondByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateSecondByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateSecondByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateSecondByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateSecondByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int16 sqlrcur_getFieldAsDateSecondByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateMicrosecondByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateMicrosecondByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateMicrosecondByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateMicrosecondByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateMicrosecondByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateIsNegativeByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateIsNegativeByIndexWithDdMm(IntPtr sqlrcurref, UInt64 row, UInt32 col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateIsNegativeByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateIsNegativeByNameIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateIsNegativeByNameWithDdMm(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase(IntPtr sqlrcurref, UInt64 row, String col, Int32 ddmm, Int32 yyyyddmm, String datedelimiters);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getFieldLengthByIndex(IntPtr sqlrcurref, UInt64 row, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getFieldLengthByName(IntPtr sqlrcurref, UInt64 row, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getColumnName(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getColumnTypeByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr sqlrcur_getColumnTypeByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getColumnLengthByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getColumnLengthByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getColumnPrecisionByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getColumnPrecisionByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getColumnScaleByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getColumnScaleByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsNullableByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsNullableByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsPrimaryKeyByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsPrimaryKeyByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsUniqueByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsUniqueByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsPartOfKeyByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsPartOfKeyByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsUnsignedByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsUnsignedByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsZeroFilledByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsZeroFilledByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsBinaryByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsBinaryByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsAutoIncrementByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_getColumnIsAutoIncrementByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getLongestByIndex(IntPtr sqlrcurref, UInt32 col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt32 sqlrcur_getLongestByName(IntPtr sqlrcurref, String col);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_suspendResultSet(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt16 sqlrcur_getResultSetId(IntPtr sqlrcurref);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_resumeResultSet(IntPtr sqlrcurref, UInt16 id);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern Int32 sqlrcur_resumeCachedResultSet(IntPtr sqlrcurref, UInt16 id, String filename);

    [DllImport("libsqlrclientwrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void sqlrcur_closeResultSet(IntPtr sqlrcurref);
}

}

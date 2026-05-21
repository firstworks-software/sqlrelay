// Copyright (c) David Muse
// See the file COPYING for more information.
package com.firstworks.sqlrelay;

public class SQLRCursor {

	static {
		System.loadLibrary("SQLRCursor");
	}

	/** Creates a cursor to run queries and fetch result
	 *  sets using connecton "sqlrc". */
	public SQLRCursor(SQLRConnection con) {
		connection=con;
		cursor=alloc(con.connection);
	}
	/** Destroys the cursor and cleans up all associated
	 *  result set data. */
	public native void	delete();


	/** Sets the number of rows of the result set
	 *  to buffer at a time.  0 (the default)
	 *  means buffer the entire result set.  */
	public native void	setResultSetBufferSize(long rows);
	/** Returns the number of result set rows that 
	 *  will be buffered at a time or 0 for the
	 *  entire result set.  */
	public native long	getResultSetBufferSize();


	/** Tells the server not to send any column
	 *  info (names, types, sizes).  If you don't
	 *  need that info, you should call this
	 *  method to improve performance.  */
	public native void	dontGetColumnInfo();
	/** Tells the server to send column info.  */
	public native void	getColumnInfo();


	/** Columns names are returned in the same
	 *  case as they are defined in the database.
	 *  This is the default. */
	public native void	mixedCaseColumnNames();
	/** Columns names are converted to upper case. */
	public native void	upperCaseColumnNames();
	/** Columns names are converted to lower case. */
	public native void	lowerCaseColumnNames();

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
	 *  caching.  */
	public native void	cacheToFile(String filename);
	/** Sets the time-to-live for cached result
	 *  sets. The sqlr-cachemanger will remove each 
	 *  cached result set "ttl" seconds after it's 
	 *  created, provided it's scanning the directory
	 *  containing the cache files.  */
	public native void	setCacheTtl(int ttl);
	/** Returns the name of the file containing the
	 *  cached result set.  */
	public native String	getCacheFileName();
	/** Sets query caching off.  */
	public native void	cacheOff();


	/** Generates a result set containing databases that match the
	 *  pattern "databases".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>Database</li>
	 *  </ul>
	 *
	 *  If "databases" is empty or null then a result set
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
	public native boolean	getDatabaseList(String databases);
	/** Generates a result set containing catalogs that match the
	 *  pattern "catalog".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>Database</li>
	 *  </ul>
	 *
	 *  If "catalog" is empty or null then a result set containing
	 *  all catalogs will be returned.
	 *
	 *  If SQL Relay doesn't support getting a list of catalogs
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getCatalogList(String catalogs);
	/** Generates a result set containing
	 *  schemas that match the pattern "schemas".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>Database</li>
	 *  </ul>
	 *
	 *  (The column name is a bit of a misnomer, the results are
	 *  schemas, not databases.)
	 *
	 *  If "schemas" is empty or null then a result set containing
	 *  all schemas in the current database will be returned.
	 *
	 *  If SQL Relay doesn't support getting a list of schemas
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getSchemaList(String schemas);
	/** Generates a result set containing
	 *  supported table types.
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>table_type</li>
	 *  </ul>
	 *
	 *  If SQL Relay doesn't support getting a list of table types
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getTableTypeList();
	/** Generates a result set containing the
	 *  tables in the current database and schema that match the
	 *  pattern "tables".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>Tables_in_xxx</li>
	 *  </ul>
	 *
	 *  If "tables" is empty or null then a result set containing
	 *  all tables in the current database/schema will be returned.
	 *
	 *  If SQL Relay doesn't support getting a list of tables
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getTableList(String tables);
	/** Generates a result set containing data
	 *  type information for "type".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>type_name</li>
	 *  <li>data_type</li>
	 *  <li>precision</li>
	 *  <li>literal_prefix</li>
	 *  <li>literal_suffix</li>
	 *  <li>create_params</li>
	 *  <li>nullable</li>
	 *  <li>case_sensitive</li>
	 *  <li>searchable</li>
	 *  <li>unsigned_attribute</li>
	 *  <li>fixed_prec_scale</li>
	 *  <li>auto_increment</li>
	 *  <li>local_type_name</li>
	 *  <li>minumum_scale</li>
	 *  <li>maxiumm_scale</li>
	 *  <li>sql_data_type</li>
	 *  <li>sql_datetime_sub</li>
	 *  <li>num_prec_radix</li>
	 *  <li>interval_precision</li>
	 *  </ul>
	 *
	 *  If "type" is empty or null then a result set containing
	 *  all data types in the current databas/schema will be
	 *  returned.
	 *
	 *  If SQL Relay doesn't support getting type info
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getTypeInfoList(String type);
	/** Generates a result set containing the
	 *  columns of "table", which match the pattern "columns".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>column_name</li>
	 *  <li>data_type</li>
	 *  <li>character_maximum_length</li>
	 *  <li>numeric_precision</li>
	 *  <li>numeric_scale</li>
	 *  <li>is_nullable</li>
	 *  <li>column_key</li>
	 *  <li>column_default</li>
	 *  <li>extra</li>
	 *  </ul>
	 *
	 *  If "columns" is empty or null then a list of all columns
	 *  of "table" will be returned.
	 *
	 *  If SQL Relay doesn't support getting a list of columns
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getColumnList(String table, String columns);
	/** Generates a result set containing the
	 *  primary keys of "table", which match the pattern
	 *  "columns".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>table</li>
	 *  <li>non_unique</li>
	 *  <li>key_name</li>
	 *  <li>seq_in_index</li>
	 *  <li>column_name</li>
	 *  <li>collation</li>
	 *  <li>cardinality</li>
	 *  <li>sub_part</li>
	 *  <li>packed</li>
	 *  <li>null</li>
	 *  <li>index_type</li>
	 *  <li>comment</li>
	 *  <li>index_comment</li>
	 *  </ul>
	 *
	 *  If "columns" is empty or null then a result set containing
	 *  all primary keys of "table" will be returned.
	 *
	 *  If SQL Relay doesn't support getting a list of primary keys
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getPrimaryKeysList(String table, String columns);
	/** Generates a result set containing the
	 *  keys and indexes of "table", which match the pattern
	 *  "qualifier".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>table</li>
	 *  <li>non_unique</li>
	 *  <li>key_name</li>
	 *  <li>seq_in_index</li>
	 *  <li>column_name</li>
	 *  <li>collation</li>
	 *  <li>cardinality</li>
	 *  <li>sub_part</li>
	 *  <li>packed</li>
	 *  <li>null</li>
	 *  <li>index_type</li>
	 *  <li>comment</li>
	 *  <li>index_comment</li>
	 *  </ul>
	 *
	 *  If "qualifier" is empty or null then a result set
	 *  containing all keys and indexes of "table" will be
	 *  returned.
	 *
	 *  If SQL Relay doesn't support getting a list of keys and
	 *  indexes for the current database backend (or the database
	 *  doesn't) then an empty result set will be returned. */
	public native boolean	getKeyAndIndexList(String table, String qualifier);
	/** Generates a result set containing
	 *  procedures that match the pattern "procedures".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>routine_catalog</li>
	 *  <li>routine_schema</li>
	 *  <li>routine_name</li>
	 *  <li>data_type</li>
	 *  </ul>
	 *
	 *  If "procedures" is empty or null then a result set
	 *  containing all procedures in the current database/schema
	 *  will be returned.
	 *
	 *  If SQL Relay doesn't support getting a list of procedures
	 *  for the current database backend (or the database doesn't)
	 *  then an empty result set will be returned. */
	public native boolean	getProcedureList(String procedures);
	/** Generates a result set containing the
	 *  parameters of "procedure", which match the pattern
	 *  "parameters".
	 *
	 *  The result set will contain the following columns:
	 *  <ul>
	 *  <li>parameter_name</li>
	 *  <li>parameter_mode</li>
	 *  <li>data_type</li>
	 *  <li>character_maximum_length</li>
	 *  <li>ordinal_position</li>
	 *  </ul>
	 *
	 *  If "parameters" is empty or null then a result set
	 *  containing all parameters of "procedure" will be returned.
	 *
	 *  If SQL Relay doesn't support getting a list of procedure
	 *  parameters for the current database backend (or the
	 *  database doesn't) then an empty result set will be
	 *  returned. */
	public native boolean	getProcedureParameterList(String procedure, String parameters);


	/** Sends "query" directly and gets a result set. */
	public native boolean	sendQuery(String query);
	/** Sends "query" with length "length" directly
	 *  and gets a result set. This method must be used
	 *  if the query contains binary data. */
	public native boolean	sendQuery(String query, int length);
	/** Sends the query in file "path"/"filename" directly
	 *  and gets a result set. */
	public native boolean	sendFileQuery(String path, String filename);


	/** Prepare to execute "query". */
	public native void	prepareQuery(String query);
	/** Prepare to execute "query" with length
	 *  "length".  This method must be used if the
	 *  query contains binary data. */
	public native void	prepareQuery(String query, int length);
	/** Prepare to execute the contents
	 *  of "path"/"filename".  Returns false if the
	 *  file couldn't be opened. */
	public native boolean	prepareFileQuery(String path, String filename);

	/** Clears all bind variables. */
	public native void	clearBinds();

	/** Defines a string substitution variable. */
	public native void	substitution(String variable, String value);
	/** Defines an integer substitution variable. */
	public native void	substitution(String variable, long value);
	/** Defines a decimal substitution variable. */
	public native void	substitution(String variable, double value,
					int precision, int scale);

	/** Parses the previously prepared query,
	 *  counts the number of bind variables defined
	 *  in it and returns that number. */
	public native short	countBindVariables();

	/** Defines a string input bind variable. */
	public native void	inputBind(String variable, String value);
	/** Defines a string input bind variable. */
	public native void	inputBind(String variable,
						String value, int length);
	/** Defines a integer input bind variable. */
	public native void	inputBind(String variable, long value);
	/** Defines a decimal input bind variable.
	  * (If you don't have the precision and scale then set
	  * them both 0.  However in that case you may get
	  * unexpected rounding behavior if the server is faking
	  * binds.) */
	public native void	inputBind(String variable, double value,
					int precision, int scale);
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
	 *  "tz" is the timezone abbreviation, and may be left null.
	 *  Most databases ignore "tz".
	 *
	 *  Set "isnegative" may be set to true to represent a negative
	 *  time interval.  However, few databases support negative
	 *  time intervals and ignore "isnegative".
	 *  */
	public native void	inputBind(String variable,
					short year, short month, short day,
					short hour, short minute, short second,
					int microsecond, String tz,
					boolean isnegative);
	/** Defines a binary lob input bind variable. */
	public native void	inputBindBlob(String variable, byte[] value,
								long size);
	/** Defines a character lob input bind variable. */
	public native void	inputBindClob(String variable, String value,
								long size);
	/** Defines an output bind variable.
	 *  "bufferlength" bytes will be reserved
	 *  to store the value. */
	public native void	defineOutputBindString(String variable,
							int bufferlength);
	/** Defines an integer output bind variable. */
	public native void	defineOutputBindInteger(String variable);
	/** Defines a decimal output bind variable. */
	public native void	defineOutputBindDouble(String variable);
	/** Defines a binary lob output bind variable. */
	public native void	defineOutputBindBlob(String variable);
	/** Defines a character lob output bind variable. */
	public native void	defineOutputBindClob(String variable);
	/** Defines a cursor output bind variable. */
	public native void	defineOutputBindCursor(String variable);
	/** Defines a date output bind variable. */
	public native void	defineOutputBindDate(String variable);

	/** Defines an array of string substitution variables. */
	public native void	substitutions(String[] variables,
							String[] values);

	/** Defines an array of integer substitution variables. */
	public native void	substitutions(String[] variables,
							long[] values);

	/** Defines an array of decimal substitution variables. */
	public native void	substitutions(String[] variables,
					double[] values,
					int[] precisions, int[] scales);

	/** Defines an array of string input bind variables. */
	public native void	inputBinds(String[] variables, String[] values);

	/** Defines an array of integer input bind variables. */
	public native void	inputBinds(String[] variables, long[] values);

	/** Defines an array of decimal input bind variables. */
	public native void	inputBinds(String[] variables,
					double[] values,
					int[] precisions, int[] scales);

	/** If you are binding to any variables that 
	 *  might not actually be in your query, call 
	 *  this to ensure that the database won't try 
	 *  to bind them unless they really are in the 
	 *  query.  There is a performance penalty for
	 *  calling this method.  */
	public native void	validateBinds();

	/** Returns true if "variable" was a valid
	 *  bind variable of the query. */
	public native boolean	validBind(String variable);

	/** Execute the query that was previously 
	 *  prepared and bound.  */
	public native boolean	executeQuery();

	/** Fetch from a cursor that was returned as
	 *  an output bind variable.  */
	public native boolean	fetchFromBindCursor();

	/** Get the value stored in a previously
	 *  defined string output bind variable. */
	public native String	getOutputBindString(String variable);
	/** Get the value stored in a previously
	 *  defined binary lob output bind variable. */
	public native byte[]	getOutputBindBlob(String variable);
	/** Get the value stored in a previously
	 *  defined character lob output bind variable. */
	public native String	getOutputBindClob(String variable);
	/** Get the length of the value stored in a
	 *  previously defined output bind variable. */
	public native byte[]	getOutputBindAsByteArray(String variable);
	/** Get the value stored in a previously
	 *  defined integer output bind variable. */
	public native long	getOutputBindInteger(String variable);
	/** Get the value stored in a previously
	 *  defined decimal output bind variable. */
	public native double	getOutputBindDouble(String variable);
	/** Get the length of the value stored in a
	 *  previously defined output bind variable. */
	public native long	getOutputBindLength(String variable);
	/** Get the year from a previously
	 *  defined date output bind variable.  */
	public native short	getOutputBindDateYear(String variable);
	/** Get the month from a previously
	 *  defined date output bind variable.  */
	public native short	getOutputBindDateMonth(String variable);
	/** Get the day from a previously
	 *  defined date output bind variable.  */
	public native short	getOutputBindDateDay(String variable);
	/** Get the hour from a previously
	 *  defined date output bind variable.  */
	public native short	getOutputBindDateHour(String variable);
	/** Get the minute from a previously
	 *  defined date output bind variable.  */
	public native short	getOutputBindDateMinute(String variable);
	/** Get the second from a previously
	 *  defined date output bind variable.  */
	public native short	getOutputBindDateSecond(String variable);
	/** Get the microsecond from a previously
	 *  defined date output bind variable.  */
	public native int	getOutputBindDateMicrosecond(String variable);
	/** Get the time zone from a previously
	 *  defined date output bind variable.  */
	public native String	getOutputBindDateTz(String variable);
	/** Get whether the value is negative from a
	 *  previously defined date output bind variable.  */
	public native boolean	getOutputBindDateIsNegative(String variable);
	/** Get the cursor associated with a previously
	 *  defined output bind variable. */
	public SQLRCursor	getOutputBindCursor(String variable) {
		SQLRCursor	bindcur=new SQLRCursor(connection);
		bindcur.cursor=getOutputBindCursorInternal(variable);
		return bindcur;
	}


	/** Opens a cached result set.
	 *  Returns true on success and false on failure. */
	public native boolean	openCachedResultSet(String filename);

	/** Returns the number of columns in the current
	 *  result set.  */
	public native int	colCount();
	/** Returns the number of rows in the current 
	 *  result set (if the result set is being
	 *  stepped through, this returns the number
	 *  of rows processed so far).  */
	public native long	rowCount();
	/** Returns the total number of rows that will
	 *  be returned in the result set.  Not all
	 *  databases support this call.  Don't use it
	 *  for applications which are designed to be
	 *  portable across databases.  0 is returned
	 *  by databases which don't support this option. */
	public native long	totalRows();
	/** Returns the number of rows that were
	 *  updated, inserted or deleted by the query.
	 *  Not all databases support this call.  Don't
	 *  use it for applications which are designed
	 *  to be portable across databases.  0 is
	 *  returned by databases which don't support
	 *  this option. */
	public native long	affectedRows();
	/** Returns the index of the first buffered row.
	 *  This is useful when buffering only part of
	 *  the result set at a time.  */
	public native long	firstRowIndex();
	/** Returns false if part of the result set is
	 *  still pending on the server and true if not.
	 *  This method can only return false if
	 *  setResultSetBufferSize() has been called
	 *  with a parameter other than 0. */
	public native boolean	endOfResultSet();

	/** Returns true and acts like executeQuery()
	 *  when there is another result set available
	 *  from the server.  */
	public native boolean	nextResultSet();


	/** If a query failed and generated an error,
	 *  the error message is available here.  If 
	 *  the query succeeded then this method 
	 *  returns NULL.  */
	public native String	errorMessage();

	/** If a query failed and generated an
	 *  error, the error number is available here.
	 *  If there is no error then this method 
	 *  returns 0. */
	public native long	errorNumber();


	/** Tells the connection to return NULL fields
	 *  and output bind variables as empty strings. 
	 *  This is the default.  */
	public native void	getNullsAsEmptyStrings();
	/** Tells the connection to return NULL fields
	 *  and output bind variables as NULL's rather
	 *  than as empty strings.  */
	public native void	getNullsAsNulls();


	/** Returns the specified field as a string. */
	public native String	getField(long row, int col);
	/** Returns the specified field as a string. */
	public native String	getField(long row, String col);
	/** Returns the specified field as a string, ignoring
	 *  the case of "col". */
	public native String	getFieldIgnoringCase(long row, String col);
	/** Returns the specified field as an integer. */
	public native long	getFieldAsInteger(long row, int col);
	/** Returns the specified field as an integer. */
	public native long	getFieldAsInteger(long row, String col);
	/** Returns the specified field as an integer, ignoring
	 *  the case of "col". */
	public native long	getFieldAsIntegerIgnoringCase(long row, String col);
	/** Returns the specified field as a decimal. */
	public native double	getFieldAsDouble(long row, int col);
	/** Returns the specified field as a decimal. */
	public native double	getFieldAsDouble(long row, String col);
	/** Returns the specified field as a decimal, ignoring
	 *  the case of "col". */
	public native double	getFieldAsDoubleIgnoringCase(long row, String col);
	/** Returns the specified field as a boolean. */
	public native boolean	getFieldAsBoolean(long row, int col);
	/** Returns the specified field as a boolean. */
	public native boolean	getFieldAsBoolean(long row, String col);
	/** Returns the specified field as a boolean, ignoring
	 *  the case of "col". */
	public native boolean	getFieldAsBooleanIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the year component. */
	public native short	getFieldAsDateYear(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns the year component. */
	public native short	getFieldAsDateYear(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the year component. */
	public native short	getFieldAsDateYear(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the year component. */
	public native short	getFieldAsDateYear(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the year component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateYearIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the year component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateYearIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the month component. */
	public native short	getFieldAsDateMonth(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns the month component. */
	public native short	getFieldAsDateMonth(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the month component. */
	public native short	getFieldAsDateMonth(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the month component. */
	public native short	getFieldAsDateMonth(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the month component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateMonthIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the month component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateMonthIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the day component. */
	public native short	getFieldAsDateDay(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns the day component. */
	public native short	getFieldAsDateDay(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the day component. */
	public native short	getFieldAsDateDay(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the day component. */
	public native short	getFieldAsDateDay(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the day component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateDayIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the day component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateDayIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the hour component. */
	public native short	getFieldAsDateHour(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns the hour component. */
	public native short	getFieldAsDateHour(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the hour component. */
	public native short	getFieldAsDateHour(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the hour component. */
	public native short	getFieldAsDateHour(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the hour component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateHourIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the hour component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateHourIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the minute component. */
	public native short	getFieldAsDateMinute(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns the minute component. */
	public native short	getFieldAsDateMinute(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the minute component. */
	public native short	getFieldAsDateMinute(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the minute component. */
	public native short	getFieldAsDateMinute(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the minute component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateMinuteIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the minute component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateMinuteIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the second component. */
	public native short	getFieldAsDateSecond(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns the second component. */
	public native short	getFieldAsDateSecond(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the second component. */
	public native short	getFieldAsDateSecond(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the second component. */
	public native short	getFieldAsDateSecond(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the second component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateSecondIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the second component,
	 *  ignoring the case of "col". */
	public native short	getFieldAsDateSecondIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the microsecond component. */
	public native int	getFieldAsDateMicrosecond(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns the microsecond component. */
	public native int	getFieldAsDateMicrosecond(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the microsecond component. */
	public native int	getFieldAsDateMicrosecond(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the microsecond component. */
	public native int	getFieldAsDateMicrosecond(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns the microsecond component,
	 *  ignoring the case of "col". */
	public native int	getFieldAsDateMicrosecondIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns the microsecond component,
	 *  ignoring the case of "col". */
	public native int	getFieldAsDateMicrosecondIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns whether the hour component
	 *  is negative. */
	public native boolean	getFieldAsDateIsNegative(long row, int col);
	/** Interprets the specified field as a date
	 *  and returns whether the hour component
	 *  is negative. */
	public native boolean	getFieldAsDateIsNegative(long row, int col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns whether the hour component
	 *  is negative. */
	public native boolean	getFieldAsDateIsNegative(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns whether the hour component
	 *  is negative. */
	public native boolean	getFieldAsDateIsNegative(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Interprets the specified field as a date
	 *  and returns whether the hour component
	 *  is negative, ignoring the case of "col". */
	public native boolean	getFieldAsDateIsNegativeIgnoringCase(long row, String col);
	/** Interprets the specified field as a date
	 *  and returns whether the hour component
	 *  is negative, ignoring the case of "col". */
	public native boolean	getFieldAsDateIsNegativeIgnoringCase(long row, String col,
					boolean ddmm, boolean yyyyddmm,
					String datedelimiters);
	/** Returns the specified field as a byte array. */
	public native byte[]	getFieldAsByteArray(long row, int col);
	/** Returns the specified field as a byte array. */
	public native byte[]	getFieldAsByteArray(long row, String col);
	/** Returns the length of the specified field. */
	public native long	getFieldLength(long row, int col);
	/** Returns the length of the specified field. */
	public native long	getFieldLength(long row, String col);
	/** Returns a null terminated array of the 
	 *  values of the fields in the specified row.  */
	public native String[]	getRow(long row);
	/** Returns a null terminated array of the 
	 *  lengths of the fields in the specified row.  */
	public native long[]	getRowLengths(long row);
	/** Returns a null terminated array of the 
	 *  column names of the current result set.  */
	public native String[]	getColumnNames();
	/** Returns the name of the specified column.  */
	public native String	getColumnName(int col);
	/** Returns the type of the specified column.  */
	public native String	getColumnType(int col);
	/** Returns the type of the specified column.  */
	public native String	getColumnType(String col);
	/** Returns the precision of the specified
	 *  column.
	 *  Precision is the total number of digits in
	 *  a number.  eg: 123.45 has a precision of 5.
	 *  For non-numeric types, it's the number of
	 *  characters in the string. */
	public native long	getColumnPrecision(int col);
	/** Returns the precision of the specified
	 *  column.
	 *  Precision is the total number of digits in
	 *  a number.  eg: 123.45 has a precision of 5.
	 *  For non-numeric types, it's the number of
	 *  characters in the string. */
	public native long	getColumnPrecision(String col);
	/** Returns the scale of the specified column.
	 *  Scale is the total number of digits to the
	 *  right of the decimal point in a number.
	 *  eg: 123.45 has a scale of 2. */
	public native long	getColumnScale(int col);
	/** Returns the scale of the specified column.
	 *  Scale is the total number of digits to the
	 *  right of the decimal point in a number.
	 *  eg: 123.45 has a scale of 2. */
	public native long	getColumnScale(String col);
	/** Returns true if the specified column can
	 *  contain nulls and false otherwise. */
	public native boolean	getColumnIsNullable(int col);
	/** Returns true if the specified column can
	 *  contain nulls and false otherwise. */
	public native boolean	getColumnIsNullable(String col);
	/** Returns true if the specified column is a
	 * primary key and false otherwise. */
	public native boolean	getColumnIsPrimaryKey(int col);
	/** Returns true if the specified column is a
	 * primary key and false otherwise. */
	public native boolean	getColumnIsPrimaryKey(String col);
	/** Returns true if the specified column is
	 * unique and false otherwise. */
	public native boolean	getColumnIsUnique(int col);
	/** Returns true if the specified column is
	 * unique and false otherwise. */
	public native boolean	getColumnIsUnique(String col);
	/** Returns true if the specified column is
	 * part of a composite key and false otherwise. */
	public native boolean	getColumnIsPartOfKey(int col);
	/** Returns true if the specified column is
	 * part of a composite key and false otherwise. */
	public native boolean	getColumnIsPartOfKey(String col);
	/** Returns true if the specified column is
	 * an unsigned number and false otherwise. */
	public native boolean	getColumnIsUnsigned(int col);
	/** Returns true if the specified column is
	 * an unsigned number and false otherwise. */
	public native boolean	getColumnIsUnsigned(String col);
	/** Returns true if the specified column was
	 * created with the zero-fill flag and false
	 * otherwise. */
	public native boolean	getColumnIsZeroFilled(int col);
	/** Returns true if the specified column was
	 * created with the zero-fill flag and false
	 * otherwise. */
	public native boolean	getColumnIsZeroFilled(String col);
	/** Returns true if the specified column
	 * contains binary data and false
	 * otherwise. */
	public native boolean	getColumnIsBinary(int col);
	/** Returns true if the specified column
	 * contains binary data and false
	 * otherwise. */
	public native boolean	getColumnIsBinary(String col);
	/** Returns true if the specified column
	 * auto-increments and false otherwise. */
	public native boolean	getColumnIsAutoIncrement(int col);
	/** Returns true if the specified column
	 * auto-increments and false otherwise. */
	public native boolean	getColumnIsAutoIncrement(String col);
	/** Returns the number of bytes required on
	 *  the server to store the data for the specified column */
	public native int	getColumnLength(int col);
	/** Returns the number of bytes required on
	 *  the server to store the data for the specified column */
	public native int	getColumnLength(String col);
	/** Returns the length of the longest field
	 *  in the specified column.  */
	public native int	getLongest(int col);
	/** Returns the length of the longest field
	 *  in the specified column.  */
	public native int	getLongest(String col);


	/** Tells the server to leave this result
	 *  set open when the connection calls 
	 *  suspendSession() so that another connection 
	 *  can connect to it using resumeResultSet() 
	 *  after it calls resumeSession(). */
	public native void	suspendResultSet();
	/** Returns the internal ID of this result set.
	 *  This parameter may be passed to another
	 *  cursor for use in the resumeResultSet()
	 *  method.
	 *  Note: The value this method returns is only
	 *  valid after a call to suspendResultSet(). */
	public native short	getResultSetId();
	/** Resumes a result set previously left open
	 *  using suspendSession().
	 *  Returns true on success and false on failure. */
	public native boolean	resumeResultSet(short id);
	/** Resumes a result set previously left open
	 *  using suspendSession() and continues caching
	 *  the result set to "filename".
	 *  Returns true on success and false on failure. */
	public native boolean	resumeCachedResultSet(short id,
							String filename);
	/** Closes the current result set, if one is open.  Data
 	 *  that has been fetched already is still available but
 	 *  no more data may be fetched.  Server side resources
 	 *  for the result set are freed as well. */
	public native void	closeResultSet();


	/** cursor and connection are used internally, they're just
	 *  public to make the JNI wrapper work faster.  */
	public long		cursor;
	public SQLRConnection	connection;
	private native long	alloc(long con);
	private native long	getOutputBindCursorInternal(String variable);
	public native boolean	getDatabaseListWithFormat(
						String databases,
						int listformat);
	public native boolean	getCatalogListWithFormat(
						String catalogs,
						int listformat);
	public native boolean	getSchemaListWithFormat(
						String schemas,
						int listformat);
	public native boolean	getTableListWithFormat(
						String tables,
						int listformat,
						int objecttypes);
	public native boolean	getTableTypeListWithFormat(
						int listformat);
	public native boolean	getColumnListWithFormat(
						String table,
						String columns,
						int listformat);
	public native boolean	getPrimaryKeysListWithFormat(
						String table,
						String columns,
						int listformat);
	public native boolean	getKeyAndIndexListWithFormat(
						String table,
						String qualifier,
						int listformat);
	public native boolean	getTypeInfoListWithFormat(
						String type,
						int listformat);
	public native boolean	getProcedureListWithFormat(
						String procedures,
						int listformat);
	public native boolean	getProcedureParameterListWithFormat(
						String procedure,
						String parameters,
						int listformat);
	public native boolean	getLastInsertIdList();
	public native boolean	isNumberType(String type);
	public native boolean	isDateTimeType(String type);
	public native boolean	isBinaryType(String type);
	public native boolean	isUnsignedType(String type);
	public native boolean	isClobType(String type);
	public native boolean	isBlobType(String type);
}

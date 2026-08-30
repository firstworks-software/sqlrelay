# Copyright (c) David Muse
# See the file COPYING for more information

package SQLRelay::Cursor;

require DynaLoader;
@ISA = 'DynaLoader';

bootstrap SQLRelay::Cursor;

sub getRowLengths {
	($this,$row)=@_;
	if (validRow($this,$row)==0) {
		return ();
	}
	my @lengths;
	for (my $col=0; $col<colCount($this); $col++) {
		$lengths[$col]=getFieldLength($this,$row,$col);
	}
	return @lengths;
}

sub getRowLengthsHash {
	($this,$row)=@_;
	if (validRow($this,$row)==0) {
		return ();
	}
	my %hash;
	for (my $col=0; $col<colCount($this); $col++) {
		$hash{getColumnName($this,$col)}=
			getFieldLength($this,$row,$col);
	}
	return %hash;
}

sub getRow {
	($this,$row)=@_;
	if (validRow($this,$row)==0) {
		return ();
	}
	my @fields;
	for (my $col=0; $col<colCount($this); $col++) {
		$fields[$col]=getField($this,$row,$col);
	}
	return @fields;
}

sub getRowHash {
        ($this,$row)=@_;
	if (validRow($this,$row)==0) {
		return ();
	}
        my %hash;
        for (my $col=0; $col<colCount($this); $col++) {
		$hash{getColumnName($this,$col)}=getField($this,$row,$col);
        }
        return %hash;
}

sub substitutions {
        ($this,$variables,$values,$precisions,$scales)=@_;
	@vars=@$variables;
	@vals=@$values;
	@precs=@$precisions;
	@scls=@$scales;
	for (my $i=0; $i<=$#vars; $i++) {
		if (defined($precs[$i]) and defined($scls[$i])) {
			substitution($this,$vars[$i],$vals[$i],
						$precs[$i],$scls[$i]);
		} else {
			substitution($this,$vars[$i],$vals[$i]);
		}
	}
}

sub inputBinds {
        ($this,$variables,$values,$precisions,$scales)=@_;
	@vars=@$variables;
	@vals=@$values;
	@precs=@$precisions;
	@scls=@$scales;
	for (my $i=0; $i<=$#vars; $i++) {
		if (defined($precs[$i]) and defined($scls[$i])) {
			inputBind($this,$vars[$i],$vals[$i],
						$precs[$i],$scls[$i]);
		} else {
			inputBind($this,$vars[$i],$vals[$i]);
		}
	}
}

1;
__END__

=head1 NAME

    SQLRelay::Cursor - Perl API for SQL Relay

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

    SQLRelay::Cursor

        new(sqlrclient);

        DESTROY();

        setResultSetBufferSize(rows);
            # Sets the number of rows of the result set
            # to buffer at a time.  0 (the default)
            # means buffer the entire result set.

        getResultSetBufferSize();
            # Returns the number of result set rows that 
            # will be buffered at a time or 0 for the
            # entire result set.

        dontGetColumnInfo();
            # Tells the server not to send any column
            # info (names, types, sizes).  If you don't
            # need that info, you should call this
            # method to improve performance.

        getColumnInfo();
            # Tells the server to send column info.

        mixedCaseColumnNames();
            # Columns names are returned in the same
            # case as they are defined in the database.
            # This is the default.

        upperCaseColumnNames();
            # Columns names are converted to upper case.

        lowerCaseColumnNames();
            # Columns names are converted to lower case.


        cacheToFile(char *filename);
            # Sets query caching on.  Future queries
            # will be cached to the file "filename".
            #
            # A default time-to-live of 10 minutes is
            # also set.
            #
            # Note that once cacheToFile() is called,
            # the result sets of all future queries will
            # be cached to that file until another call 
            # to cacheToFile() changes which file to
            # cache to or a call to cacheOff() turns off
            # caching.

        setCacheTtl(int ttl);
            # Sets the time-to-live for cached result
            # sets. The sqlr-cachemanager will remove each
            # cached result set "ttl" seconds after it's 
            # created, provided it's scanning the directory
            # containing the cache files.

        getCacheFileName();
            # Returns the name of the file containing the
            # cached result set.

        cacheOff();
            # Sets query caching off.

        getDatabaseList(databases);
            # Generates a result set containing databases that match the
            # pattern "databases".
            #
            # The result set will contain the following columns:
            # * Database
            #
            # If "databases" is empty or undef then a result set
            # containing all databases will be returned.
            #
            # May actually return a result set of catalogs or schemas,
            # depending on whether the backend database equates
            # "database" with catalog or schema.
            #
            # See getDatabaseIsSchema().
            #
            # If SQL Relay doesn't support getting a list of databases
            # for the current database backend (or the database doesn't)
            # then an empty result set will be returned.

        getCatalogList(catalog);
            # Generates a result set containing catalogs that match the
            # pattern "catalog".
            #
            # The result set will contain the following columns:
            # * Database
            #
            # If "catalog" is empty or undef then a result set containing
            # all catalogs will be returned.
            #
            # If SQL Relay doesn't support getting a list of catalogs
            # for the current database backend (or the database doesn't)
            # then an empty result set will be returned.

        getSchemaList(schemas);
            # Generates a result set containing
            # schemas that match the pattern "schemas".
            #
            # The result set will contain the following columns:
            # * Database
            #
            # (The column name is a bit of a misnomer, the results
            # are schemas, not databases.)
            #
            # If "schemas" is empty or undef then a result set
            # containing all schemas in the current database will
            # be returned.
            #
            # If SQL Relay doesn't support getting a list of
            # schemas for the current database backend (or the
            # database doesn't) then an empty result set will be
            # returned.
        getTableTypeList();
            # Generates a result set containing
            # supported table types.
            #
            # The result set will contain the following columns:
            # * table_type
            #
            # If SQL Relay doesn't support getting a list of
            # table types for the current database backend (or
            # the database doesn't) then an empty result set will
            # be returned.
        getTableList(tables);
            # Generates a result set containing
            # the tables in the current database and schema that
            # match the pattern "tables".
            #
            # The result set will contain the following columns:
            # * Tables_in_xxx
            #
            # If "tables" is empty or undef then a result set
            # containing all tables in the current
            # database/schema will be returned.
            #
            # If SQL Relay doesn't support getting a list of
            # tables for the current database backend (or the
            # database doesn't) then an empty result set will be
            # returned.
        getTypeInfoList(type);
            # Generates a result set containing
            # data type information for "type".
            #
            # The result set will contain the following columns:
            # * type_name
            # * data_type
            # * precision
            # * literal_prefix
            # * literal_suffix
            # * create_params
            # * nullable
            # * case_sensitive
            # * searchable
            # * unsigned_attribute
            # * fixed_prec_scale
            # * auto_increment
            # * local_type_name
            # * minumum_scale
            # * maxiumm_scale
            # * sql_data_type
            # * sql_datetime_sub
            # * num_prec_radix
            # * interval_precision
            #
            # If "type" is empty or undef then a result set
            # containing all data types in the current
            # database/schema will be returned.
            #
            # If SQL Relay doesn't support getting type info
            # for the current database backend (or the database
            # doesn't) then an empty result set will be returned.
        getColumnList(table,columns);
            # Generates a result set containing
            # the columns of "table", which match the pattern
            # "columns".
            #
            # The result set will contain the following columns:
            # * column_name
            # * data_type
            # * character_maximum_length
            # * numeric_precision
            # * numeric_scale
            # * is_nullable
            # * column_key
            # * column_default
            # * extra
            #
            # If "columns" is empty or undef then a list of all
            # columns of "table" will be returned.
            #
            # If SQL Relay doesn't support getting a list of
            # columns for the current database backend (or the
            # database doesn't) then an empty result set will be
            # returned.
        getPrimaryKeysList(table,columns);
            # Generates a result set containing
            # the primary keys of "table", which match the
            # pattern "columns".
            #
            # The result set will contain the following columns:
            # * table
            # * non_unique
            # * key_name
            # * seq_in_index
            # * column_name
            # * collation
            # * cardinality
            # * sub_part
            # * packed
            # * null
            # * index_type
            # * comment
            # * index_comment
            #
            # If "columns" is empty or undef then a result set
            # containing all primary keys of "table" will be
            # returned.
            #
            # If SQL Relay doesn't support getting a list of
            # primary keys for the current database backend (or
            # the database doesn't) then an empty result set
            # will be returned.
        getKeyAndIndexList(table,qualifier);
            # Generates a result set containing
            # the keys and indexes of "table", which match the
            # pattern "qualifier".
            #
            # The result set will contain the following columns:
            # * table
            # * non_unique
            # * key_name
            # * seq_in_index
            # * column_name
            # * collation
            # * cardinality
            # * sub_part
            # * packed
            # * null
            # * index_type
            # * comment
            # * index_comment
            #
            # If "qualifier" is empty or undef then a result set
            # containing all keys and indexes of "table" will be
            # returned.
            #
            # If SQL Relay doesn't support getting a list of keys
            # and indexes for the current database backend (or
            # the database doesn't) then an empty result set will
            # be returned.
        getProcedureList(procedures);
            # Generates a result set containing
            # procedures that match the pattern "procedures".
            #
            # The result set will contain the following columns:
            # * routine_catalog
            # * routine_schema
            # * routine_name
            # * data_type
            #
            # If "procedures" is empty or undef then a result set
            # containing all procedures in the current
            # database/schema will be returned.
            #
            # If SQL Relay doesn't support getting a list of
            # procedures for the current database backend (or the
            # database doesn't) then an empty result set will be
            # returned.
        getProcedureParameterList(procedure,parameters);
            # Generates a result set containing
            # the parameters of "procedure", which match the
            # pattern "parameters".
            #
            # The result set will contain the following columns:
            # * parameter_name
            # * parameter_mode
            # * data_type
            # * character_maximum_length
            # * ordinal_position
            #
            # If "parameters" is empty or undef then a result set
            # containing all parameters of "procedure" will be
            # returned.
            #
            # If SQL Relay doesn't support getting a list of
            # procedure parameters for the current database
            # backend (or the database doesn't) then an empty
            # result set will be returned.

        setCursorName(cursorname);
            # Sets the name of this cursor to "cursorname",
            # replacing any previously set name.  The name is
            # sent to the server immediately, and the server
            # hands it to the database when the next query is
            # prepared.  Named cursors are mainly useful for
            # positioned updates and deletes, as in:
            # UPDATE ... WHERE CURRENT OF "cursorname".
            #
            # Not all databases support this call.  Don't use
            # it for applications which are designed to be
            # portable across databases.  The name is ignored
            # by databases which don't support this option.

        getCursorName();
            # Returns the name most recently passed to
            # setCursorName(), or undef if setCursorName() was
            # never called.  This is a local copy of the name;
            # no request is made to the server.  It is not
            # necessarily the name that the database ended up
            # using, as not all databases support named
            # cursors.
            #
            # Not all databases support this call.  Don't use
            # it for applications which are designed to be
            # portable across databases.


        # If you don't need to use substitution or bind variables
        # in your queries, use these two methods.
        sendQuery(query);
            # Sends "query" directly and gets a result set.

        sendQueryWithLength(query,length);
            # Sends "query" with length "length" directly
            # and gets a result set. This method must be used
            # if the query contains binary data.

        sendFileQuery(path,filename);
            # Sends the query in file "path"/"filename" directly
            # and gets a result set.




        # If you need to use substitution or bind variables, in your
        # queries use the following methods.  See the API documentation
        # for more information about substitution and bind variables.
        prepareQuery(query);
            # Prepare to execute "query".

        prepareQueryWithLength(query,length);
            # Prepare to execute "query" with length 
            # "length".  This method must be used if the
            # query contains binary data.

        prepareFileQuery(path,filename);
            # Prepare to execute the contents
            # of "path"/"filename".  Returns false if the
            # file couldn't be opened.

        substitution(variable,value);
            # Defines a substitution variable.

        clearBinds();
            # Clears all bind variables.

        countBindVariables();
            # Parses the previously prepared query,
            # counts the number of bind variables defined
            # in it and returns that number.

        inputBind(variable,value);
        inputBind(variable,value,length);
        inputBind(variable,value,precision,scale);
        inputBindDate(variable,year,month,day,hour,minute,second,microsecond,tz,isnegative);
        inputBindBlob(variable,value,size);
        inputBindClob(variable,value,size);
            # Defines an input bind variable.
            # (For floating point values, if you don't have the precision and
            # scale then they may both be set to 0.  However in that case you
            # may get unexpected rounding behavior if the server is faking
            # binds.)

        defineOutputBindString(variable,bufferlength);
            # Defines an output bind variable.
            # "bufferlength" bytes will be reserved
            # to store the value.
        defineOutputBindDate(variable);
            # Defines a date output bind variable.
        defineOutputBindBlob(variable);
            # Defines a binary lob output bind variable.
        defineOutputBindClob(variable);
            # Defines a character lob output bind variable.
        defineOutputBindCursor(variable);
            # Defines a cursor output bind variable.

        substitutions(variables,values);
            # Defines an array of substitution variables.

        inputBinds(variables,values);
            # Defines an array of input bind variables.

        validateBinds();
            # If you are binding to any variables that
            # might not actually be in your query, call
            # this to ensure that the database won't try
            # to bind them unless they really are in the
            # query.  There is a performance penalty for
            # calling this method.

        validBind(variable);
            # Returns true if "variable" was a valid
            # bind variable of the query.

        executeQuery();
            # Execute the query that was previously 
            # prepared and bound.

        fetchFromBindCursor();
            # Fetch from a cursor that was returned as
            # an output bind variable.


        getOutputBindString(variable);
            # Get the value stored in a previously
            # defined string output bind variable.

	getOutputBindBlob(variable);
            # Get the value stored in a previously
            # defined binary lob output bind variable.

	getOutputBindClob(variable);
            # Get the value stored in a previously
            # defined character lob output bind variable.

	getOutputBindLength(variable);
            # Get the length of the value stored in a
            # previously defined output bind variable.

	getOutputBindCursor(variable);
            # Get the cursor associated with a previously
            # defined output bind variable.

	getOutputBindDateYear(variable);
            # Get the year from a previously
            # defined date output bind variable.

	getOutputBindDateMonth(variable);
            # Get the month from a previously
            # defined date output bind variable.

	getOutputBindDateDay(variable);
            # Get the day from a previously
            # defined date output bind variable.

	getOutputBindDateHour(variable);
            # Get the hour from a previously
            # defined date output bind variable.

	getOutputBindDateMinute(variable);
            # Get the minute from a previously
            # defined date output bind variable.

	getOutputBindDateSecond(variable);
            # Get the second from a previously
            # defined date output bind variable.

	getOutputBindDateMicrosecond(variable);
            # Get the microsecond from a previously
            # defined date output bind variable.

	getOutputBindDateTz(variable);
            # Get the time zone from a previously
            # defined date output bind variable.

	getOutputBindDateIsNegative(variable);
            # Get whether the value is negative from a
            # previously defined date output bind variable.



        openCachedResultSet(filename);
            # Opens a cached result set.
            # Returns true on success and false on failure.



        colCount();
            # Returns the number of columns in the current
            # result set.

        rowCount();
            # Returns the number of rows in the current
            # result set (if the result set is being
            # stepped through, this returns the number
            # of rows processed so far).

        totalRows();
            # Returns the total number of rows that will
            # be returned in the result set.  Not all
            # databases support this call.  Don't use it
            # for applications which are designed to be
            # portable across databases.  0 is returned
            # by databases which don't support this option.

        affectedRows();
            # Returns the number of rows that were
            # updated, inserted or deleted by the query.
            # Not all databases support this call.  Don't
            # use it for applications which are designed
            # to be portable across databases.  0 is
            # returned by databases which don't support
            # this option.

        firstRowIndex();
            # Returns the index of the first buffered row.
            # This is useful when buffering only part of
            # the result set at a time.

        endOfResultSet();
            # Returns false if part of the result set is still
            # pending on the server and true if not.  This
            # method can only return false if
            # setResultSetBufferSize() has been called
            # with a parameter other than 0.

        nextResultSet();
            # Returns true and acts like executeQuery()
            # when there is another result set available
            # from the server.

        errorMessage();
            # If a query failed and generated an error, the
            # error message is available here.  If the 
            # query succeeded then this method returns NULL.

        errorNumber();
            # If a query failed and generated an
            # error, the error number is available here.
            # If there is no error then this method 
            # returns 0.

        getNullsAsEmptyStrings();
            # Tells the connection to return NULL fields
            # and output bind variables as empty strings.
            # This is the default.

        getNullsAsUndefined();
            # Tells the connection to return NULL fields
            # and output bind variables as undef rather
            # than as empty strings.

        getField(row, col);
            # Returns the specified field as a string.

        getFieldLength(row, col);
            # Returns the length of the specified field.

        getRow(row);
            # Returns an array of the values of the
            # fields in the specified row or an empty
            # list if the requested row is past the
            # end of the result set.

        getRowHash(row);
            # Returns the requested row as values in a
            # hash with column names for keys or an
            # empty list if the requested row is past
            # the end of the result set.

        getRowLengths(row);
            # Returns a null terminated array of the
            # lengths of the fields in the specified row
            # or an empty list if the requested row is
            # past the end of the result set.

        getRowLengthsHash(row);
            # Returns the requested row lengths as values 
            # in a hash with column names for keys or an
            # empty list if the requested row is past
            # the end of the result set.

        getColumnNames();
            # Returns a null terminated array of the 
            # column names of the current result set.

        getColumnName(col);
            # Returns the name of the specified column.

        getColumnType(col);
            # Returns the type of the specified column.

        getColumnLength(col);
            # Returns the number of bytes required on
            # the server to store the data for the specified column

        getColumnPrecision(col);
            # Returns the precision of the specified
            # column.
            # Precision is the total number of digits in
            # a number.  eg: 123.45 has a precision of 5.
            # For non-numeric types, it's the number of
            # characters in the string.

        getColumnScale(col);
            # Returns the scale of the specified column.
            # Scale is the total number of digits to the
            # right of the decimal point in a number.
            # eg: 123.45 has a scale of 2.

        getColumnIsNullable(col);
            # Returns true if the specified column can
            # contain nulls and false otherwise.

        getColumnIsPrimaryKey(col);
            # Returns true if the specified column is a
            # primary key and false otherwise.

        getColumnIsUnique(col);
            # Returns true if the specified column is
            # unique and false otherwise.

        getColumnIsPartOfKey(col);
            # Returns true if the specified column is
            # part of a composite key and false otherwise.

        getColumnIsUnsigned(col);
            # Returns true if the specified column is
            # an unsigned number and false otherwise.

        getColumnIsZeroFilled(col);
            # Returns true if the specified column was
            # created with the zero-fill flag and false
            # otherwise.

        getColumnIsBinary(col);
            # Returns true if the specified column
            # contains binary data and false
            # otherwise.

        getColumnIsAutoIncrement(col);
            # Returns true if the specified column
            # auto-increments and false otherwise.

        getLongest(col);
            # Returns the length of the longest field
            # in the specified column.

        suspendResultSet();
            # Tells the server to leave this result
            # set open when the connection calls
            # suspendSession() so that another connection
            # can connect to it using resumeResultSet()
            # after it calls resumeSession().

        getResultSetId();
            # Returns the internal ID of this result set.
            # This parameter may be passed to another
            # cursor for use in the resumeResultSet()
            # method.
            # Note: The value this method returns is only
            # valid after a call to suspendResultSet().

        resumeResultSet(int id);
            # Resumes a result set previously left open 
            # using suspendSession().
            # Returns true on success and false on failure.

        resumeCachedResultSet(int id, char *filename);
            # Resumes a result set previously left open
            # using suspendSession() and continues caching
            # the result set to "filename".
            # Returns true on success and false on failure.

        closeResultSet();
            # Closes the current result set, if one is open.  Data
            # that has been fetched already is still available but
            # no more data may be fetched.  Server side resources
            # for the result set are freed as well.

=head1 AUTHOR

    David Muse
    david.muse@firstworks.com

=cut

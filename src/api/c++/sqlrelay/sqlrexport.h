// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORT_H
#define SQLREXPORT_H

#include <sqlrelay/private/sqlrexportincludes.h>

/** The sqlrexport class provides a base class for child classes that wish to
 *  implement export of data from a database, via SQL Relay.  It provides
 *  various common methods.  Each child class should implement at least one of
 *  exportToFile(), exportToTable(), or exportToJsonDomNode(). */
class SQLRCLIENT_DLLSPEC sqlrexport {
	public:
		/** Creates an instance of the sqlrexport class. */
		sqlrexport();

		/** Destroys this instance of the sqlrexport class. */
		virtual	~sqlrexport();

		/** Sets the instance of sqlrconnection that this instance
		 *  will use to connect to the database. */
		void	setSqlrConnection(sqlrconnection *sqlrcon);

		/** Sets the instance of sqlrcursor that this instance
		 *  will use to export data. */
		void	setSqlrCursor(sqlrcursor *sqlrcur);

		/** Returns the instance of sqlrconnection that this instance
 		 *  is configured to use to connect to the database. */
		sqlrconnection	*getSqlrConnection();

		/** Returns the instance of sqlrursor that this instance
 		 *  is configured to use to connect to export data. */
		sqlrcursor	*getSqlrCursor();

		/** If "ignorecolumns" is set false, then column information
		 *  will be exported (eg. to the CSV header, XML tags inside
		 *  of the file, etc.).
		 *
		 *  If "ignorecolumns" is set true, then column information
		 *  will not be exported.
		 *
		 *  Defaults to false. */
		void	setIgnoreColumns(bool ignorecolumns);

		/** Returns whether or not column information will be
		 *  exported. */
		bool	getIgnoreColumns();

		/** If "columnstoignore" is a null terminated array of column
		 *  names, then those columns will not be exported.  If
		 *  "columnstoignore" is NULL then all columns will be
		 *  exported. */
		void	setColumnsToIgnore(const char * const *columnstoignore);

		/** Returns the current set of columns that will not be
		 *  exported as a NULL-terminated array. */
		const char * const *getColumnsToIgnore();

		/** Sets the logger to use when logging progress to "lg".
		 *  If "lg" is set to NULL then progress will not be logged.
		 *  Defaults to NULL. */
		void	setLogger(logger *lg);

		/** Returns the logger that is set to use when logging
 		 *  progress or NULL if no logger is set. */
		logger	*getLogger();

		/** Sets the coarse log level.  General log messages will be
		 *  logged at this level.  If the log level of "lg" (set by
		 *  setLogger() above) is set equal to or greater than
		 *  "coarseloglevel" then general log messages will be logged.
		 *  Defaults to 0. */
		void	setCoarseLogLevel(uint8_t coarseloglevel);

		/** Returns the coarse log level. */
		uint8_t		getCoarseLogLevel();

		/** Sets the fine log level.  Detailed log messages will be
		 *  logged at this level.  If the log level of "lg" (set by
		 *  setLogger() above) is set equal to or greater than
		 *  "coarseloglevel" then general log messages will be logged.
		 *  Defaults to 9. */
		void	setFineLogLevel(uint8_t fineloglevel);

		/** Returns the fine log level. */
		uint8_t		getFineLogLevel();

		/** Sets the log indent level to "logindent".  Defaults to 0. */
		void	setLogIndent(uint32_t logindent);

		/** Returns the log indent. */
		uint32_t	getLogIndent();

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to file
		 *  "filename" or to standard output if "filename" is NULL or
		 *  empty.
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  calls exportToFile(filename,NULL).  Child classes that
		 *  support export to a file should override that
		 *  exportToFile() method. */
		virtual	bool	exportToFile(const char *filename);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to file
		 *  "filename" or to standard output if "filename" is NULL or
		 *  empty.
		 *
		 *  If "table" is non-null, then the result set is presumed
		 *  to be a (possibly partial) dump of that table, and the
		 *  table name is embedded in the export, if the export format
		 *  supports this.
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  returns true.  Child classes that support export to a file
		 *  should override this method.
		 *
		 *  The method implemented by the child class should call the
		 *  various Start()/End() methods below at the appropriate
		 *  time.  If any of the Start()/End() methods return false,
		 *  then export should stop and this method should return
		 *  false.  */
		virtual	bool	exportToFile(const char *filename,
							const char *table);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  database table "table" using "sqlrcon".  A commit is
		 *  called every "commitcount" rows.  No commit is called if
		 *  "commitcount" is set to 0.
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  uses sqlrcon to allocate an sqlrcursor, calls
		 *  exportToTable(sqlrcon,sqlrcur,table), then frees the
		 *  cursor.  Child classes that support export to a database
		 *  table should override that exportToTable() method. */
		virtual	bool	exportToTable(sqlrconnection *sqlrcon,
							const char *table,
							uint64_t commitcount);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  database table "table" using "sqlrcon" and "sqlrcur".
		 *  A commit is called every "commitcount" rows.  No commit is
		 *  called if "commitcount" is set to 0.
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  returns true.  Child classes that support export to a
		 *  database table should override this method.
		 *
		 *  The method implemented by the child class should call the
		 *  various Start()/End() methods below at the appropriate
		 *  time.  If any of the Start()/End() methods return false,
		 *  then export should stop and this method should return
		 *  false.  */
		virtual	bool	exportToTable(sqlrconnection *sqlrcon,
							sqlrcursor *sqlrcur,
							const char *table,
							uint64_t commitcount);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  JSON domnode "jsondomnode".
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  calls exportToJsonDomNode(jsondomnode,NULL).  Child classes
		 *  tha  support export to a JSON domnode should override that
		 *  exportToJsonDomNode() method. */
		virtual	bool	exportToJsonDomNode(domnode *jsondomnode);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  JSON domnode "jsondomnode".
		 *
		 *  If "table" is non-null, then the result set is presumed
		 *  to be a (possibly partial) dump of that table, and the
		 *  table name is embedded in the export, if the export format
		 *  supports this.
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  returns true.  Child classes that support export to a
		 *  JSON domnode should override this method.
		 *
		 *  The method implemented by the child class should call the
		 *  various Start()/End() methods below at the appropriate
		 *  time.  If any of the Start()/End() methods return false,
		 *  then export should stop and this method should return
		 *  false.  */
		virtual	bool	exportToJsonDomNode(domnode *jsondomnode,
							const char *table);

		/** This method should be called by implementations of the
		 *  exportTo*() methods, prior to the export of the columns of
		 *  the result set.
		 *
		 *  Note that it should be called whether or not columns are
		 *  ignored.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true
		 *  * getExportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return 0
		 *  * getCurrentColumnName() should return the name of column 0
		 *  * getCurrentField() should also return the name of column 0
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	columnsStart();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, prior to the export of each column of
		 *  the result set.
		 *
		 *  Note that it should be called for each column, whether or
		 *  not columns are ignored, and whether or not this particular
		 *  column is ignored.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true
		 *  * getExportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return the index of the
		 *    column that we're starting
		 *  * getCurrentColumnName() should return the name of the
		 *    column that we're starting
		 *  * getCurrentField() should also return the name of the
		 *    column that we're starting
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	columnStart();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, after the export of each column of the
		 *  result set.
		 *
		 *  Note that it should be called for each column, whether or
		 *  not columns are ignored, and whether or not this particular
		 *  column is ignored.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true
		 *  * getExportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return the index of the
		 *    column that we're ending
		 *  * getCurrentColumnName() should return the name of the
		 *    column that we're ending
		 *  * getCurrentField() should also return the name of the
		 *    column that we're ending
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	columnEnd();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, after the export of the columns of
		 *  the result set.
		 *
		 *  Note that it should be called whether or not columns are
		 *  ignored.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true
		 *  * getExportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return one more than the index
		 *    of the last column
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should also return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	columnsEnd();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, prior to the export of the rows of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true
		 *  * getExportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return 0
		 *  * getCurrentColumnName() should return the name of column 0
		 *  * getCurrentField() should return the value of the field in
		 *    row 0, column 0
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	rowsStart();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, prior to the export of each row of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true
		 *  * getExportedRowCount() should return the number of rows
		 *    that have been exported - it should not include the
		 *    row that we are starting
		 *  * getCurrentRow() should return the index of the row that
		 *    we're starting
		 *  * getCurrentColumn() should return 0
		 *  * getCurrentColumnName() should return the name of column 0
		 *  * getCurrentField() should return the value of the field in
		 *    row 0, column 0
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  This is a good place to call setExportRow(false) if you
		 *  don't want this row to be exported.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	rowStart();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, prior to the export of each field of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true or false, as
		 *    appropriate for this row
		 *  * getExportedRowCount() should return the number of rows
		 *    that have been exported
		 *  * getCurrentRow() should return the index of the row that
		 *    we previously started
		 *  * getCurrentColumn() should return the index of the column
		 *    that corresponds to the index of the field that we're
		 *    starting
		 *  * getCurrentColumnName() should return the name of column
		 *    that corresponds to the index of the field that we're
		 *    starting
		 *  * getCurrentField() should return the value of the field
		 *    that we're starting
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  This is a good place to call setCurrentField(...) if you
		 *  want to modify the value of the field that is going to
		 *  be exported.  Note that if the value of the field is
		 *  replaced, then the memory allocated to store the new value
		 *  should persist until fieldEnd() is called.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	fieldStart();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, after the export of each field of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true or false, as
		 *    appropriate for this row
		 *  * getExportedRowCount() should return the number of rows
		 *    that have been exported
		 *  * getCurrentRow() should return the index of the row that
		 *    we previously started
		 *  * getCurrentColumn() should return the index of the column
		 *    that corresponds to the index of the field that we're
		 *    ending
		 *  * getCurrentColumnName() should return the name of column
		 *    that corresponds to the index of the field that we're
		 *    ending
		 *  * getCurrentField() should return the value of the field
		 *    that we're ending
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  If you called setCurrentField(...) in fieldStart(), and had
		 *  to allocate memory for the new value, then this is a good
		 *  place to free that memory.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	fieldEnd();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, after the export of each row of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true or false, as
		 *    appropriate for this row
		 *  * getExportedRowCount() should return the number of rows
		 *    that have been exported - it should not include the
		 *    row that we are ending
		 *  * getCurrentRow() should return the index of the row that
		 *    we're ending
		 *  * getCurrentColumn() should return one more than the index
		 *    of the last column
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	rowEnd();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, after the export of the rows of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getExportRow() should return true or false,
		 *    as appropriate for the last row
		 *  * getExportedRowCount() should return the number of rows
		 *    that were exported
		 *  * getCurrentRow() should return one more than the index
		 *    of the last row
		 *  * getCurrentColumn() should return one more than the index
		 *    of the last column
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	rowsEnd();

		/** Returns the number of rows that were exported by the most
		 *  recent call to one of the exportTo*() methods, or the
		 *  current number of rows that have been exported, if called
		 *  from inside one of the Start()/End() methods. */
		uint64_t	getExportedRowCount();

	protected:

		/** Sets whether the current row of the result set will be
		 *  exported or not.
		 *  
		 *  Should be called by implementations of exportTo*().  May
		 *  also be called by rowStart().  Not commonly called by other
		 *  *Start/End() methods. */
		void	setExportRow(bool exportrow);

		/** Gets whether the current row of the result set will be
		 *  exported or not.
		 *  
		 *  May be called by implementations of exportTo*() or by
		 *  implementations of the *Start/End() methods. */
		bool	getExportRow();

		/** Sets the index of the row of the result set that is
		 *  currently being exported.
		 *  
		 *  Should be called by implementations of exportTo*().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentRow(uint64_t currentrow);

		/** Gets the index of the row of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportTo*() or by
		 *  implementations of the *Start/End() methods. */
		uint64_t	getCurrentRow();

		/** Sets the index of the column of the result set that is
		 *  currently being exported.
		 *  
		 *  Should be called by implementations of exportTo*().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentColumn(uint32_t currentcol);

		/** Gets the index of the column of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportTo*() or by
		 *  implementations of the *Start/End() methods. */
		uint32_t	getCurrentColumn();

		/** Sets the name of the column of the result set that is
		 *  currently being exported.
		 *
		 *  Should be called by implementations of exportTo*().  May
		 *  also be called by columnStart().  Not commonly called by
		 *  other *Start/End() methods. */
		void	setCurrentColumnName(const char *currentcolname);

		/** Gets the name of the column of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportTo*() or by
		 *  implementations of the *Start/End() methods. */
		const char	*getCurrentColumnName();

		/** Sets the value of the field of the result set that is
		 *  currently being exported.
		 *
		 *  Should be called by implementations of exportTo*().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentField(const char *currentfield);

		/** Gets the value of the field of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportTo*() or by
		 *  implementations of the *Start/End() methods. */
		const char	*getCurrentField();

		/** Sets whether the data type of the column of the result set
		 *  in position "index" is a numeric type or not.  If "numeric"
		 *  is true then the type of the column is set to numeric.  If
		 *  "numeric" is false then the type of the column is set to
		 *  non-numeric.
		 *
		 *  Should be called by implementations of exportTo*().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setIsNumericColumn(uint64_t index, bool numeric);

		/** Get whether the data type of the column of the result set
		 *  in position "index" is a numeric type or not.
		 *
		 *  May be called by implementations of exportTo*() or by
		 *  implementations of the *Start/End() methods. */
		bool	getIsNumericColumn(uint64_t index);

		/** Clears the data types of all columns of the result set,
		 *  setting them to to non-numeric.
		 *
		 *  Should be called by implementations of exportTo*().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	clearAreNumericColumns();

		/** Sets the number of rows that have been exported.
		 *
		 *  Should be called by implementations of exportTo*().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setExportedRowCount(uint64_t exportedrowcount);

		/** Captures the name of the file to which data will be
		 *  exported, making it available to the *Start/End()
		 *  methods.
		 *
		 *  Should be called by implementations of exportToFile().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setFileName(const char *filename);

		/** Gets the name of the file to which data is being exported.
		 *
		 *  May be called by implementations of exportToFile() or by
		 *  implementations of the *Start/End() methods. */
		const char	*getFileName();

		/** Sets the file descriptor to which export data will be
 		 *  exported.
 		 *
		 *  Should be called by implementations of exportToFile().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setFileDescriptor(filedescriptor *fd);

		/** Gets the file descriptor to which data is being exported.
		 *
		 *  May be called by implementations of exportToFile() or by
		 *  implementations of the *Start/End() methods. */
		filedescriptor	*getFileDescriptor();

		/** Captures the name of the table associated with the export,
		 *  making it available to the *Start/End() methods.
		 *
		 *  In the case of exportToFile() or exportToJsonDomNode(), this
		 *  is the table that is being exported - commonly NULL or
		 *  empty.
		 *
		 *  In the case of exportToTable(), this is the table that data
		 *  will be exported to.
		 *
		 *  Should be called by implementations of exportTo*().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setTable(const char *table);

		/** Gets the name of the table associated with the export.
		 *
		 *  In the case of exportToFile() or exportToJsonDomNode(), this
		 *  is the table that is being exported - commonly NULL or
		 *  empty.
		 *
		 *  In the case of exportToTable(), this is the table that data
		 *  will be exported to.
		 *
		 *  May be called by implementations of exportToTable() or by
		 *  implementations of the *Start/End() methods. */
		const char	*getTable();

		/** Gets the buffer that the insert query is written when
		 *  exporting data to a table.
		 *
		 *  Should be called by implementations of exportToTable().
		 *  May also be called by implementations of the *Start/End()
		 *  methods. */
		stringbuffer	*getInsertQueryBuffer();

		/** Captures the commit count, making it available to the
		 *  *Start/End() methods.
		 *
		 *  Should be called by implementations of exportToTable().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCommitCount(uint64_t commitcount);

		/** Gets the commit count.
		 *
		 *  May be called by implementations of exportToTable() or by
		 *  implementations of the *Start/End() methods. */
		uint64_t	getCommitCount();

		/** Captures the top-level domnode, making it available to
		 *  *Start/End() methods.
		 *
		 *  Should be called by implementations of
		 *  exportToJsonDomNode().  Not commonly called by
		 *  implementations of the *Start/End() methods. */
		void	setJsonDomNode(domnode *dn);

		/** Gets the top-level domnode.
		 *
		 *  May be called by implementations of exportToJsonDomNode()
		 *  or by implementations of the *Start/End() methods. */
		domnode	*getJsonDomNode();

		/** Set the domnode that columns will be written to.
		 *
		 *  Should be called by implementations of
		 *  exportToJsonDomNode().  Not commonly called by
		 *  implementations of the *Start/End() methods. */
		void	setColumnsDomNode(domnode *dn);

		/** Gets the domnode that columns are being written to.
		 *
		 *  May be called by implementations of exportToJsonDomNode()
		 *  or by implementations of the *Start/End() methods. */
		domnode	*getColumnsDomNode();

		/** Sets the domnode that the current column will be written to.
		 * 
		 *  Should be called by implementations of
		 *  exportToJsonDomNode().  Not commonly called by
		 *  implementations of the *Start/End() methods. */
		void	setCurrentColumnDomNode(domnode *dn);

		/** Gets the domnode that the current column is being written
		 *  to.
		 *
		 *  May be called by implementations of exportToJsonDomNode()
		 *  or by implementations of the *Start/End() methods. */
		domnode	*getCurrentColumnDomNode();

		/** Sets the domnode that rows are being written to.
		 * 
		 *  Should be called by implementations of
		 *  exportToJsonDomNode().  Not commonly called by
		 *  implementations of the *Start/End() methods. */
		void	setRowsDomNode(domnode *dn);

		/** Gets the domnode that rows are being written to.
		 *
		 *  May be called by implementations of exportToJsonDomNode()
		 *  or by implementations of the *Start/End() methods. */
		domnode	*getRowsDomNode();

		/** Sets the domnode that the current row will be written to.
		 * 
		 *  Should be called by implementations of
		 *  exportToJsonDomNode().  Not commonly called by
		 *  implementations of the *Start/End() methods. */
		void	setCurrentRowDomNode(domnode *dn);

		/** Gets the domnode that the current row is being written to.
		 *
		 *  May be called by implementations of exportToJsonDomNode()
		 *  or by implementations of the *Start/End() methods. */
		domnode	*getCurrentRowDomNode();

		/** Sets the domnode that the current field will be written to.
		 * 
		 *  Should be called by implementations of
		 *  exportToJsonDomNode().  Not commonly called by
		 *  implementations of the *Start/End() methods. */
		void	setCurrentFieldDomNode(domnode *dn);

		/** Gets the domnode that the current field is being written to.
		 *
		 *  May be called by implementations of exportToJsonDomNode()
		 *  or by implementations of the *Start/End() methods. */
		domnode	*getCurrentFieldDomNode();

	#include <sqlrelay/private/sqlrexport.h>
};

#endif

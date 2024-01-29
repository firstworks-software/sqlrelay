// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLREXPORT_H
#define SQLREXPORT_H

#include <sqlrelay/private/sqlrexportincludes.h>

/** The sqlrexport class provides a base class for child classes that wish to
 *  implement export of data from a database, via SQL Relay.  It provides
 *  various common methods.  Each child class should implement the
 *  exportData() method. */
class SQLRCLIENT_DLLSPEC sqlrexport {
	public:
		/** Creates an instance of the sqlrexport class. */
		sqlrexport();

		/** Destroys this instance of the sqlrexport class. */
		virtual	~sqlrexport();

		/** Sets the instance of sqlrconnection that this instance
		 *  will use to fetch data for the export. */
		void	setSqlrConnection(sqlrconnection *sqlrcon);

		/** Sets the instance of sqlrcursor that this instance
		 *  will use to fetch data for the export. */
		void	setSqlrCursor(sqlrcursor *sqlrcur);

		/** Returns the instance of sqlrconnection that this instance
 		 *  is configured to use to fetch data for the export. */
		sqlrconnection	*getSqlrConnection();

		/** Returns the instance of sqlrursor that this instance
 		 *  is configured to use to fetch data for the export. */
		sqlrcursor	*getSqlrCursor();

		/** Sets the name of the table associated with the export.
		 *
		 *  This may be used differently by different child classes.
		 *  Eg. it may be the name of the table being exported, or the
		 *  name of the table that data is being exported to. */
		void	setTable(const char *table);

		/** Gets the name of the table associated with the export.
		 *
		 *  This may be used differently by different child classes.
		 *  Eg. it may be the name of the table being exported, or the
		 *  name of the table that data is being exported to. */
		const char	*getTable();

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
		 *  set by the most recent call to setSqlrCursor().
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  returns true.  Child classes that should override this
		 *  method. */
		virtual	bool	exportData();

		/** This method should be called by implementations of
		 *  exportData(), at the beginning of the export process.
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
		 *  * Nothing should have been written to files, domnodes,
		 *    or tables
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual bool	exportStart();

		/** This method should be called by implementations of
		 *  exportData(), prior to the export of the columns of the
		 *  result set.
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

		/** This method should be called by implementations of
		 *  exportData(), prior to the export of each column of the
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

		/** This method should be called by implementations of
		 *  exportData(), after the export of each column of the
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

		/** This method should be called by implementations of
		 *  exportData(), after the export of the columns of the
		 *  result set.
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

		/** This method should be called by implementations of
		 *  exportData(), prior to the export of the rows of the
		 *  result set.
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

		/** This method should be called by implementations of
		 *  exportData(), prior to the export of each row of the
		 *  result set.
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

		/** This method should be called by implementations of
		 *  exportData(), prior to the export of each field of the
		 *  result set.
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
		 *  If you want to modify the value of the field that is going
		 *  to be exported, this is a good method to override to call
		 *  setCurrentField(...)
		 *
		 *  Note, however...
		 *
		 *  The memory allocated to store the new value must persist
		 *  until the value is actually exported.  When this is depends
		 *  on the implementation of the exportData() method.
		 *
		 *  Implementations that export to a file or json domnode
		 *  typically export the field before calling fieldEnd().  As
		 *  such, storage for the updated field value may be freed
		 *  inside of your implementation of fieldEnd().
		 *
		 *  However, implementations that export to a table tend to
		 *  build an insert statement, bind values before calling
		 *  fieldEnd(), and then execute the statement before calling
		 *  rowEnd().  In this case, storage for the updated field
		 *  value should be freed inside of your implemenatation of
		 *  rowEnd().
		 *
		 *  Be sure to verify how the various exportData() methods that
		 *  you are using were written, and use care when freeing 
		 *  storage allocated for updated field values.
		 *
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	fieldStart();

		/** This method should be called by implementations of
		 *  exportData(), after the export of each field of the
		 *  result set.
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

		/** This method should be called by implementations of
		 *  exportData(), after the export of each row of the
		 *  result set.
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

		/** This method should be called by implementations of
		 *  exportData(), after the export of the rows of the
		 *  result set.
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

		/** This method should be called by implementations of
		 *  exportData(), before a begin().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	beginStart();

		/** This method should be called by implementations of
		 *  exportData(), after a begin().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	beginEnd();

		/** This method should be called by implementations of
		 *  exportData(), before a commit().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	commitStart();

		/** This method should be called by implementations of
		 *  exportData(), after a commit().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	commitEnd();

		/** This method should be called by implementations of
		 *  exportData() methods, if a commit(), begin(),
		 *  executeQuery(), or other database operation fails.
		 *
		 *  This implementation just returns false but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true if export may continue, or false if 
		 *  should stop. */
		virtual	bool	error(int64_t errornumber,
					const char *errormessage);

		/** This method should be called by implementations of
		 *  exportData(), at the end of the export process.
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
		 *  * There should be nothing left to write to files, domnodes,
		 *    or tables
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual bool	exportEnd();

		/** Returns the number of rows that were exported by the most
		 *  recent call to exportData(), or the current number of rows
		 *  that have been exported, if called from inside one of the
		 *  Start()/End() methods. */
		uint64_t	getExportedRowCount();

	protected:

		/** Sets whether the current row of the result set will be
		 *  ignored or not.  Rows that are ignored are not exported.
		 *  
		 *  Should be called by implementations of exportData().  May
		 *  also be called by rowStart().  Not commonly called by other
		 *  *Start/End() methods. */
		void	setIgnoreRow(bool ignorerow);

		/** Gets whether the current row of the result set will be
		 *  ignored or not.  Rows that are ignored are not exported.
		 *  
		 *  May be called by implementations of exportData() or by
		 *  implementations of the *Start/End() methods. */
		bool	getIgnoreRow();

		/** Sets the index of the row of the result set that is
		 *  currently being exported.
		 *  
		 *  Should be called by implementations of exportData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentRow(uint64_t currentrow);

		/** Gets the index of the row of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportData() or by
		 *  implementations of the *Start/End() methods. */
		uint64_t	getCurrentRow();

		/** Sets the index of the column of the result set that is
		 *  currently being exported.
		 *  
		 *  Should be called by implementations of exportData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentColumn(uint32_t currentcol);

		/** Gets the index of the column of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportData() or by
		 *  implementations of the *Start/End() methods. */
		uint32_t	getCurrentColumn();

		/** Sets the name of the column of the result set that is
		 *  currently being exported.
		 *
		 *  Should be called by implementations of exportData().  May
		 *  also be called by columnStart().  Not commonly called by
		 *  other *Start/End() methods. */
		void	setCurrentColumnName(const char *currentcolname);

		/** Gets the name of the column of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportData() or by
		 *  implementations of the *Start/End() methods. */
		const char	*getCurrentColumnName();

		/** Sets the value of the field of the result set that is
		 *  currently being exported.
		 *
		 *  Should be called by implementations of exportData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentField(const char *currentfield);

		/** Gets the value of the field of the result set that is
		 *  currently being exported.
		 *
		 *  May be called by implementations of exportData() or by
		 *  implementations of the *Start/End() methods. */
		const char	*getCurrentField();

		/** Sets whether the data type of the column of the result set
		 *  in position "index" is a numeric type or not.  If "numeric"
		 *  is true then the type of the column is set to numeric.  If
		 *  "numeric" is false then the type of the column is set to
		 *  non-numeric.
		 *
		 *  Should be called by implementations of exportData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setIsNumericColumn(uint64_t index, bool numeric);

		/** Get whether the data type of the column of the result set
		 *  in position "index" is a numeric type or not.
		 *
		 *  May be called by implementations of exportData() or by
		 *  implementations of the *Start/End() methods. */
		bool	getIsNumericColumn(uint64_t index);

		/** Clears the data types of all columns of the result set,
		 *  setting them to to non-numeric.
		 *
		 *  Should be called by implementations of exportData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	clearAreNumericColumns();

		/** Sets the number of rows that have been exported.
		 *
		 *  Should be called by implementations of exportData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setExportedRowCount(uint64_t exportedrowcount);

	#include <sqlrelay/private/sqlrexport.h>
};

#endif

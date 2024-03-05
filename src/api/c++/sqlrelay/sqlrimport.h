// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLRIMPORT_H
#define SQLRIMPORT_H

#include <sqlrelay/private/sqlrimportincludes.h>

/** The sqlrimport class provides a base class for child classes that wish to
 *  implement import of data from a file into a database, via SQL Relay.  It
 *  provides various common methods.  Each child class must implement the
 *  importData() method. */
class SQLRCLIENT_DLLSPEC sqlrimport {
	public:
		/** Creates an instance of the sqlrimport class. */
		sqlrimport();

		/** Destroys this instance of the sqlrimport class. */
		virtual	~sqlrimport();

		/** Sets the instance of sqlrconnection that this instance
		 *  will use to insert data for the import. */
		void	setSqlrConnection(sqlrconnection *sqlrcon);

		/** Sets the instance of sqlrcursor that this instance
		 *  will use to insert data for the import. */
		void	setSqlrCursor(sqlrcursor *sqlrcur);

		/** Returns the instance of sqlrconnection that this instance
 		 *  is configured to use to insert data for the import. */
		sqlrconnection	*getSqlrConnection();

		/** Returns the instance of sqlrursor that this instance
 		 *  is configured to use to insert data for the import. */
		sqlrcursor	*getSqlrCursor();

		/** Sets the database type, which impacts how things like
		 *  escaping, sequences, and auto-increment fields are handled.
		 *  Should be one of "postgresql", "mysql", "firebird",
		 *  "oracle", "db2", or "informix".  Or may be left empty or
		 *  NULL for generic handling.  Defaults to NULL. */
		void	setDbType(const char *dbtype);

		/** Returns the database type as set by setDbType(). */
		const char	*getDbType();

		/** By default, the name of the table or sequence to import
		 *  data into is derived from the import file (eg. from the
		 *  CSV file name, or from an XML tag inside of the file).
		 *  This method may be used to explicitly override that name,
		 *  or provide one if none can be derived. */
		void	setObjectName(const char *objectname);

		/** Returns the object name as set by setObjectName(). */
		const char	*getObjectName();

		/** If "ignorecolumns" is set false, then column information
		 *  will be read from the import (eg. from the CSV header, from
		 *  XML tags inside of the file, etc.) and used to define the
		 *  column-order of the import data, which may be different
		 *  from the column-order of the table, and may exclude
		 *  nullable columns.
		 *
		 *  If "ignorecolumns" is set true, then any column information
		 *  included in the import will be ignored.  Import data will
		 *  be assumed to be in the same column-order as the
		 *  column-order of the table.  This is useful, for example,
		 *  when a CSV header contains different column names than the
		 *  table.
		 *
		 *  Defaults to false. */
		void	setIgnoreColumns(bool ignorecolumns);

		/** Returns whether or not column information will be
		 *  ignored. */
		bool	getIgnoreColumns();

		/** Maps column name "from" to "to".  If "to" is NULL then
		 *  the column is unmapped. */
		void	mapColumnName(const char *from, const char *to);

		/** Returns the name that "from" is mapped to, or NULL if
		 *  "from" is not mapped to anything. */
		const char	*getMappedColumnName(const char *from);

		/** Leaves column names as-is. */
		void	setMixedCaseColumnNames();

		/** Returns true if column names are left as-is and false
		 *  otherwise. */
		bool	getMixedCaseColumnNames();

		/** Lower-cases colum names. */
		void	setLowerCaseColumnNames();

		/** Returns true if column names are lower-cased and false
		 *  otherwise. */
		bool	getLowerCaseColumnNames();

		/** Upper-cases colum names. */
		void	setUpperCaseColumnNames();

		/** Returns true if column names are upper-cased and false
		 *  otherwise. */
		bool	getUpperCaseColumnNames();

		/** Maps field value "from" to "to".  If "to" is NULL then
		 *  the field is unmapped. */
		void	mapFieldValue(const char *from, const char *to);

		/** Returns the value that "from" is mapped to, or NULL if
		 *  "from" is not mapped to anything. */
		const char	*getMappedFieldValue(const char *from);

		/** If "reformatdatetime" is set true then date/time fields
		 *  will be reformatted prior to import according to rules
		 *  defined by setDdMm(), setYyyyDdMm(),setDateDelimiters(),
		 *  setNoCenturyThreshold(), setLastCenturyThreshold(), and
		 *  setDateTimeFormat().
		 *
		 *  If "reformatdatetime" is set false then date/time fields
		 *  will be not reformatted prior to import.
		 *
		 *  Defaults to false. */
		void	setReformatDateTime(bool reformatdatetime);

		/** Returns whether or not date/time fields will be reformatted
		 *  prior to import. */
		bool	getReformatDateTime();

		/** If setReformatDateTime(true) has been called, then...
		 *
		 *  If "ddmm" is set true then date/time fields in formats
		 *  like xx/xx/xxxx, xx-xx-xxxx, xx.xx.xxxx, etc. will be
		 *  interpreted as DD/MM/YYYY rather than MM/DD/YYYY.
		 *
		 *  If "ddmm" is set false then date/time fields in formats
		 *  like xx/xx/xxxx, xx-xx-xxxx, xx.xx.xxxx, etc. will be
		 *  interpreted as MM/DD/YYYY rather than MM/DD/YYYY.
		 *
		 *  Defaults to false.
		 *
		 *  Has no effect if setReformatDateTime(false) has been called
		 *  or if setReformatDateTime() has never been called. */
		void	setDdMm(bool ddmm);

		/** Returns the value of the most recent call to setDdMm() or
		 *  false if setDdMm() has never been called. */
		bool	getDdMm();

		/** If setReformatDateTime(true) has been called, then...
		 *
		 *  If "yyyyddmm" is set true then date/time fields in formats
		 *  like xxxx/xx/xx, xxxx-xx-xx, xxxx.xx.xx, etc. will be
		 *  interpreted as YYYY/DD/MM rather than YYYY/MM/DD.
		 *
		 *  If "yyyyddmm" is set false then date/time fields in formats
		 *  like xxxx/xx/xx, xxxx-xx-xx, xxxx.xx.xx, etc. will be
		 *  interpreted as YYYY/MM/DD rather than YYYY/MM/DD.
		 *
		 *  Defaults to false.
		 *
		 *  Has no effect if setReformatDateTime(false) has been called
		 *  or if setReformatDateTime() has never been called. */
		void	setYyyyDdMm(bool yyyyddmm);

		/** Returns the value of the most recent call to setYyyyDdMm()
		 *  or false if setYyyyDdMm() has never been called. */
		bool	getYyyyDdMm();

		/** If setReformatDateTime(true) has been called, then...
		 *
		 *  Sets the set of characters that will be used to parse
		 *  dates to "datedelimiters".
		 *
		 *  Defaults to NULL which implies "/-.:".
		 *
		 *  Has no effect if setReformatDateTime(false) has been called
		 *  or if setReformatDateTime() has never been called. */
		void	setDateDelimiters(const char *datedelimiters);

		/** Returns the value of the most recent call to
		 *  setDateDelimiters() or NULL if setDateDelimiters() has
		 *  never been called. */
		const char	*getDateDelimiters();

		/** If setReformatDateTime(true) has been called, then...
		 *
		 *  Sets the threshold for detecting that a date does not
		 *  contain a century.
		 *
		 *  Works in conjunction with setLastCenturyThreshold().
		 *
		 *  Eg. if set to 100 then
		 *  * any date who's year component is < 100 will be
		 *    considered not to have a century
		 *  * a date like 99 or 90 will not be considered to actually
		 *    be the years 99 or 90, but rather 1999, 1990, 2099, 2099,
		 *    etc., depending on setLastCenturyThreshold()
		 *
		 *  Eg. if set to 0 then
		 *  * the date's year component will be taken literally
		 *  * a date like 99 or 90 will be considerd to actually be the
		 *    year 99 or 90
		 *
		 *  Defaults to 100.
		 *
		 *  Has no effect if setReformatDateTime(false) has been called
		 *  or if setReformatDateTime() has never been called. */
		void	setNoCenturyThreshold(uint16_t nocenturythreshold);

		/** Returns the value of the most recent call to
		 *  setNoCenturyThreshold() or 100 if setNoCenturyThreshold()
		 *  has never been called. */
		uint16_t	getNoCenturyThreshold();

		/** If setReformatDateTime(true) has been called, then...
		 *
		 *  Sets the threshold for detecting whether a date that does
		 *  not contain a century is meant to refer to the current or
		 *  previous century.
		 *
		 *  Works in conjunction with setNoCenturyThreshold().
		 *
		 *  Eg. if set to 10 then
		 *  * any date determined not to have a century that is > 10
		 *    years from the current date will be considered to be
		 *    in the previous century
		 *  * if the current year is 2020 then a date of 10, 20 or 30
		 *    will be considered to be 2010, 2020, or 2030
		 *  * a date of 31, 40, 50, etc. will be considered to be
		 *    1931, 1940, 1950, etc.
		 *
		 *  Eg. if set to 0 then
		 *  * all dates will be considered to be in the current century
		 *  * if the current year is 2020, then a date of 10, 20, 30,
		 *    40, 50, 100, etc. will be considered to be 2010, 2020,
		 *    2030, 2040, 2050, 2100, etc.
		 *
		 *  Defaults to 10.
		 *
		 *  Has no effect if setReformatDateTime(false) has been called
		 *  or if setReformatDateTime() has never been called. */
		void	setLastCenturyThreshold(uint16_t lastcenturythreshold);

		/** Returns the value of the most recent call to
		 *  setLastCenturyThreshold() or 10 if setLastCenturyThreshold()
		 *  has never been called. */
		uint16_t	getLastCenturyThreshold();

		/** If setReformatDateTime(true) has been called, then...
		 *
		 *  Sets the format to use when reformatting date/times to
		 *  "datetimeformat".
		 *
		 *  Defaults to "YYYY-MM-DD HH24:MI:SS".
		 *
		 *  Has no effect if setReformatDateTime(false) has been called
		 *  or if setReformatDateTime() has never been called. */
		void	setDateTimeFormat(const char *datetimeformat);

		/** Returns the value of the most recent call to
		 *  setDateTimeFormat() or "YYYY-MM-DD HH24:MI:SS" if
		 *  setDateTimeFormat() has never been called. */
		const char	*getDateTimeFormat();

		/** Call commit after every "commitcount" inserts.  If set to 0
		 *  then no commits will be called and the commit behavior will
		 *  depend on the behavior of the instance of sqlrelay that we
		 *  are connecting to.  Defaults to 0. */
		void	setCommitCount(uint64_t commitcount);

		/** Returns the commit count. */
		uint64_t getCommitCount();

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

		/** If "logerrors" is set true then SQL errors will be logged
		 *  at the coarse log level.  If set false then SQL errors will
		 *  not be logged.  Defaults to false. */
		void	setLogErrors(bool logerrors);

		/** Returns true if SQL errors will be logged at the coarse log
		 *  level and false otherwise. */
		bool	getLogErrors();

		/** Imports data.
		 *
		 *  If setCommitCount() was called with a non-zero value then a
		 *  commit will be called after every "commitcount" rows is
		 *  inserted.  No commit will be called if setCommitCount() was
		 *  never called, or if setCommitCount(0) was called.
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that that default implementation of this method just
		 *  returns true.  Child classes should override this method. */
		virtual	bool	importData();

		/** This method should be called by implementations of
		 *  importData(), at the beginning of the import process.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true
		 *  * getImportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return 0
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should also return NULL
		 *  * getIsNumericColumn() should return false for all columns
		 *  * getIsDateTimeColumn() should return false for all columns
		 *  * No data should have been imported.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual bool	importStart();

		/** This method should be called by implementations of the
		 *  importData(), prior to the import of the columns of the
		 *  data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true
		 *  * getImportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return 0
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should also return NULL
		 *  * getIsNumericColumn() should return false for all columns
		 *  * getIsDateTimeColumn() should return false for all columns
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnsStart();

		/** This method should be called by implementations of the
		 *  importData(), prior to the import of each column of the
		 *  data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true
		 *  * getImportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return the index of the
		 *    column that we're starting
		 *  * getCurrentColumnName() should return the name of the
		 *    column that we're starting
		 *  * getCurrentField() should also return the name of the
		 *    column that we're starting
		 *  * getIsNumericColumn() should return false for all columns
		 *  * getIsDateTimeColumn() should return false for all columns
		 *
		 *  If you want to modify the value of the column name that is
		 *  going to be imported, this is a good method to override and
		 *  call setCurrentColumnName().  Note that
		 *  setCurrentColumnName() takes a char *, not a const char *
		 *  argument.  A buffer must be allocated, populated, and
		 *  passed in to it, and that buffer will eventually be
		 *  deallocated by this class.  Plan accordingly.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnStart();

		/** This method should be called by implementations of the
		 *  importData(), after the import of each column of the data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true
		 *  * getImportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return the index of the
		 *    column that we're ending
		 *  * getCurrentColumnName() should return the name of the
		 *    column that we're ending
		 *  * getCurrentField() should also return the name of the
		 *    column that we're ending
		 *  * getIsNumericColumn() should return false for all columns
		 *  * getIsDateTimeColumn() should return false for all columns
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnEnd();

		/** This method should be called by implementations of the
		 *  importData(), after the import of the columns of the data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true
		 *  * getImportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return one more than the index
		 *    of the last column
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should also return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnsEnd();

		/** This method should be called by implementations of the
		 *  importData(), prior to the import of the rows of the data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true
		 *  * getImportedRowCount() should return 0
		 *  * getCurrentRow() should return 0
		 *  * getCurrentColumn() should return 0
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowsStart();

		/** This method should be called by implementations of the
		 *  importData(), prior to the import of each row of the data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true
		 *  * getImportedRowCount() should return the number of rows
		 *    that have been exported - it should not include the
		 *    row that we are starting
		 *  * getCurrentRow() should return the index of the row that
		 *    we're starting
		 *  * getCurrentColumn() should return 0
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowStart();

		/** This method should be called by implementations of the
		 *  importData(), prior to the import of each field of the
		 *  data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true or false, as
		 *    appropriate for this row
		 *  * getImportedRowCount() should return the number of rows
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
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *
		 *  If you want to modify the value of the field that is going
		 *  to be imported, this is a good method to override and call
		 *  setCurrentField().  Note that setCurrentField() takes a
		 *  char *, not a const char * argument.  A buffer must be
		 *  allocated, populated, and passed in to it, and that buffer
		 *  will eventually be deallocated by this class.  Plan
		 *  accordingly.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	fieldStart();

		/** This method should be called by implementations of the
		 *  importData(), after the import of each field of the data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true or false, as
		 *    appropriate for this row
		 *  * getImportedRowCount() should return the number of rows
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
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	fieldEnd();

		/** This method should be called by implementations of the
		 *  importData(), after the import of each row of the data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true or false, as
		 *    appropriate for this row
		 *  * getImportedRowCount() should return the number of rows
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
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowEnd();

		/** This method should be called by implementations of the
		 *  importData(), after the import of the rows of the data.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true or false,
		 *    as appropriate for the last row
		 *  * getImportedRowCount() should return the number of rows
		 *    that were exported
		 *  * getCurrentRow() should return one more than the index
		 *    of the last row
		 *  * getCurrentColumn() should return one more than the index
		 *    of the last column
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowsEnd();

		/** This method should be called by implementations of
		 *  importData(), before a begin().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	beginStart();

		/** This method should be called by implementations of
		 *  importData(), after a begin().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	beginEnd();

		/** This method should be called by implementations of
		 *  importData(), before a commit().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	commitStart();

		/** This method should be called by implementations of
		 *  importData(), after a commit().
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	commitEnd();

		/** This method should be called by implementations of
		 *  importData() methods, if a commit(), begin(),
		 *  executeQuery(), or other database operation fails.
		 *
		 *  This implementation just returns false but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true if import may continue, or false if 
		 *  should stop. */
		virtual	bool	error(int64_t errornumber,
					const char *errormessage);

		/** This method should be called by implementations of
		 *  importData(), at the end of the import process.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  At this point...
		 *  * getImportRow() should return true or false,
		 *    as appropriate for the last row
		 *  * getImportedRowCount() should return the number of rows
		 *    that were exported
		 *  * getCurrentRow() should return one more than the index
		 *    of the last row
		 *  * getCurrentColumn() should return one more than the index
		 *    of the last column
		 *  * getCurrentColumnName() should return NULL
		 *  * getCurrentField() should return NULL
		 *  * getIsNumericColumn() should return true/false correctly
		 *    for each column
		 *  * getIsDateTimeColumn() should return true/false correctly
		 *    for each column
		 *  * There should be nothing left to write to import.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual bool	importEnd();

		/** Returns the number of rows that were imported by the most
		 *  recent call to one of the importData(). */
		uint64_t	getImportedRowCount();

	protected:

		/** Sets whether the current row of the data will be ignored or
		 *  not.  Rows that are ignored are not imported.
		 *  
		 *  Should be called by implementations of importData().  May
		 *  also be called by rowStart().  Not commonly called by other
		 *  *Start/End() methods. */
		void	setIgnoreRow(bool ignorerow);

		/** Gets whether the current row of the data will be ignored or
		 *  not.  Rows that are ignored are not imported.
		 *  
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		bool	getIgnoreRow();

		/** Sets the index of the row of data that is currently being
		 *  imported.
		 *  
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentRow(uint64_t currentrow);

		/** Gets the index of the row of data that is currently being
 		 *  imported.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		uint64_t	getCurrentRow();

		/** Sets the index of the column of the data that is currently
		 *  being imported.
		 *  
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentColumn(uint32_t currentcol);

		/** Gets the index of the column of the data that is currently
		 *  being imported.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		uint32_t	getCurrentColumn();

		/** Sets the name of the column of the data that is currently
		 *  being imported.
		 *
		 *  Note that "currentcolname" is a char *, not a const char *.
		 *  Whatever value it is set to will eventually be freed by
		 *  this class.  Set "currentcolname" accordingly.
		 *
		 *  Should be called by implementations of importData().  May
		 *  also be called by columnStart().  Not commonly called by
		 *  other *Start/End() methods. */
		void	setCurrentColumnName(char *currentcolname);

		/** Gets the name of the column of the data that is currently
		 *  being imported.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		char	*getCurrentColumnName();

		/** Sets the value of the field of the data that is currently
		 *  being imported.
		 *
		 *  Note that "currentfield" is a char *, not a const char *
		 *  argument.  A buffer must be allocated, populated, and
		 *  passed in to it, and that buffer will eventually be
		 *  deallocated by this class.  Plan accordingly.
		 *
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setCurrentField(char *currentfield);

		/** Gets the value of the field of the data that is currently
		 *  being imported.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		char	*getCurrentField();

		/** Sets whether the data type of the column of the data in
		 *  position "index" is a numeric type or not.  If "numeric"
		 *  is true then the type of the column is set to numeric.  If
		 *  "numeric" is false then the type of the column is set to
		 *  non-numeric.
		 *
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setIsNumericColumn(uint64_t index, bool numeric);

		/** Get whether the data type of the column of the data in
		 *  position "index" is a numeric type or not.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		bool	getIsNumericColumn(uint64_t index);

		/** Clears the data types of all columns of the data setting
		 *  them to to non-numeric.
		 *
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	clearAreNumericColumns();

		/** Sets whether the data type of the column of the data in
		 *  position "index" is a date/time type or not.  If "datetime"
		 *  is true then the type of the column is set to date/time.  If
		 *  "datetime" is false then the type of the column is set to
		 *  non-date/time
		 *
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setIsDateTimeColumn(uint64_t index, bool datetime);

		/** Get whether the data type of the column of the data in
		 *  position "index" is a date/time type or not.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		bool	getIsDateTimeColumn(uint64_t index);

		/** Clears the data types of all columns of the data setting
		 *  them to to non-date/time.
		 *
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	clearAreDateTimeColumns();

		/** Sets the number of rows that have been imported.
		 *
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void setImportedRowCount(uint64_t importedrowcount);

	#include <sqlrelay/private/sqlrimport.h>
};

#endif

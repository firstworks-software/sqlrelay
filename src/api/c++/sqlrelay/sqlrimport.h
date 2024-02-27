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
		 *  will use to connect to the database. */
		void	setSqlrConnection(sqlrconnection *sqlrcon);

		/** Sets the instance of sqlrcursor that this instance
		 *  will use to run queries. */
		void	setSqlrCursor(sqlrcursor *sqlrcur);

		/** Returns the instance of sqlrconnection that this instance
 		 *  is configured to use to connect to the database. */
		sqlrconnection	*getSqlrConnection();

		/** Returns the instance of sqlrursor that this instance
 		 *  is configured to use to connect to import data. */
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
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that that default implementation of this method just
		 *  returns true.  Child classes should override this method. */
		virtual	bool	importData();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, prior to the import of the columns of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnsStart();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, prior to the import of each column of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnStart();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, after the import of each column of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnEnd();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, after the import of the columns of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	columnsEnd();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, prior to the import of the rows of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowsStart();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, prior to the import of each row of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowStart();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, prior to the import of each field of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	fieldStart();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, after the import of each field of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	fieldEnd();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, after the import of each row of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowEnd();

		/** This method should be called by implementations of the
		 *  importFrom*() methods, after the import of the rows of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and import should stop if this method return
		 *  false. */
		virtual	bool	rowsEnd();

		/** Returns the number of rows that were imported by the most
		 *  recent call to one of the importFrom*() methods. */
		uint64_t	getImportedRowCount();

	protected:

		/** This method may be called by implementations of the
		 *  importFrom*() methods, or by implementations of the various
		 *  Start()/End() methods to set the index of the row that is
		 *  currently being imported. */
		void	setCurrentRow(uint64_t currentrow);

		/** This method may be called by implementations of the
		 *  importFrom*() methods, or by implementations of the various
		 *  Start()/End() methods to get the index of the row that is
		 *  currently being imported. */
		uint64_t	getCurrentRow();

		/** This method may be called by implementations of the
		 *  importFrom*() methods, or by implementations of the various
		 *  Start()/End() methods to set the index of the column that is
		 *  currently being imported. */
		void	setCurrentColumn(uint32_t currentcol);

		/** This method may be called by implementations of the
		 *  importFrom*() methods, or by implementations of the various
		 *  Start()/End() methods to get the index of the column that is
		 *  currently being imported. */
		uint32_t	getCurrentColumn();

		/** This method may be called by implementations of the
		 *  importFrom*() methods, or by implementations of the various
		 *  Start()/End() methods to set the name of the field that is
		 *  currently being imported. */
		void	setCurrentField(const char *currentfield);

		/** This method may be called by implementations of the
		 *  importFrom*() methods, or by implementations of the various
		 *  Start()/End() methods to get the name of the field that is
		 *  currently being imported. */
		const char	*getCurrentField();

		/** This method may be called by implementations of the
		 *  importFrom*() methods, or by implementations of the various
		 *  Start()/End() methods to set the number of rows that have
		 *  been imported. */
		void setImportedRowCount(uint64_t importedrowcount);

	#include <sqlrelay/private/sqlrimport.h>
};

#endif

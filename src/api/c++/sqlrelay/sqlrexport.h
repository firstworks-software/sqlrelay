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
		 *  If "lg" is set to NULL then progress will not be logged. */
		void	setLogger(logger *lg);

		/** Returns the logger that is set to use when logging
 		 *  progress or NULL if no logger is set. */
		logger		*getLogger();

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
		 *  "filename".
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
		 *  "filename".
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
		 *  database table "table" using "sqlrcon" and "sqlrcur".
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
							const char *table);

		/** Exports the result set of the cursor currently in use as
		 *  set by the most recent call to setSqlrCursor() to the
		 *  JSON domnode "jsondomnode".
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method just
		 *  calls exportToJsonDomNode(jsondomnode,NULL).  Child classes
		 *  that  support export to a JSON domnode should override that
		 *  exportToFile() method. */
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
		 *  exportTo*() methods, prior to the export of the result set
		 *  header.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	headerStart();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, prior to the export of each column of
		 *  the result set header.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	columnStart();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, after the export of each column of the
		 *  result set header.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	columnEnd();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, after the export of the result set
		 *  header.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
		 *
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	headerEnd();

		/** This method should be called by implementations of the
		 *  exportTo*() methods, prior to the export of the rows of
		 *  the result set.
		 *
		 *  This implementation just returns true but a child class may
		 *  override this method to do something else.
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
		 *  Should return true on success and false if an error
		 *  occurred and export should stop if this method return
		 *  false. */
		virtual	bool	rowsEnd();

		/** Returns the number of rows that were exported by the most
		 *  recent call to one of the exportTo*() methods. */
		virtual uint64_t	getExportedRowCount();

	protected:

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to indicate whether the current row
		 *  should be exported or not. */
		void	setExportRow(bool exportrow);

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to determine whether the current row
		 *  should be exported or not. */
		bool	getExportRow();

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to set the index of the row that is
		 *  currently being exported. */
		void		setCurrentRow(uint64_t currentrow);

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to get the index of the row that is
		 *  currently being exported. */
		uint64_t	getCurrentRow();

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to set the index of the column that is
		 *  currently being exported. */
		void		setCurrentColumn(uint32_t currentcol);

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to get the index of the column that is
		 *  currently being exported. */
		uint32_t	getCurrentColumn();

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to set the name of the field that is
		 *  currently being exported. */
		void		setCurrentField(const char *currentfield);

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to get the name of the field that is
		 *  currently being exported. */
		const char	*getCurrentField();

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to set whether the data type of the
		 *  column in position "index" is a numeric type or not. */
		void	setNumberColumn(uint64_t index, bool value);

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to get whether the data type of the
		 *  column in position "index" is a numeric type or not. */
		bool	getNumberColumn(uint64_t index);

		/** This method may be called by implementations of the
		 *  exportTo*() methods, or by implementations of the various
		 *  Start()/End() methods to clear the data types of all
		 *  columns to non-numeric. */
		void	clearNumberColumns();

		/** This method may be called by implementations of
 		 *  exportToFile() to set the file descriptor to which to
 		 *  export data. */
		void		setFileDescriptor(filedescriptor *fd);

		/** This method may be called by implementations of
		 *  exportToFile() to get the file descriptor to which data is
		 *  being exported. */
		filedescriptor	*getFileDescriptor();

	#include <sqlrelay/private/sqlrexport.h>
};

#endif

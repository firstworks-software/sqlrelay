// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#ifndef SQLRIMPORTCSV_H
#define SQLRIMPORTCSV_H

#include <sqlrelay/private/sqlrimportcsvincludes.h>

/** The sqlrimportcsv class implements sqlrimport for CSV files. */
class SQLRCLIENT_DLLSPEC sqlrimportcsv : virtual public sqlrimportfile,
							virtual public csvsax {
	public:

		/** Creates an instance of the sqlrimportcsv class. */
		sqlrimportcsv();

		/** Destroys this instance of the sqlrimportcsv class. */
		virtual ~sqlrimportcsv();

		/** Inserts a primary key at "primarykeycolumnindex".
		 *
		 *  If setIgnoreColumns(false) is set (the default) then
		 *  "primarykeycolumnname" must be supplied.  Otherwise it can
		 *  be set to NULL or an empty string.
		 *
		 *  If "primarykeycolumnsequence" is non-empty and non-null
		 *  then nextval('"primarykeycolumnsequence"') will be used to
		 *  generate the key.  Otherwise a NULL will be used in an
		 *  attempt to trigger an autoincrement/serial column to
		 *  generate a key. */
		void	insertPrimaryKey(const char *primarykeycolumnname,
						uint32_t primarykeycolumnindex,
						const char *primarykeysequence);

		/** Removes any primary key configuaration set by a prior call
		 *  to insertPrimaryKey(). */
		void	removePrimaryKey();

		/** Inserts static value "value" at "columnindex" for all
		 *  records.
		 *
		 *  If setIgnoreColumns(false) is set (the default) then
		 *  "columnname" must be supplied.  Otherwise it can be set to
		 *  NULL or an empty string. */
		void	insertStaticValue(const char *columnname,
						uint32_t columnindex,
						const char *value);

		/** Removes any static value configuaration at "columnindex"
		 *  set by a prior call to insertStaticValue(). */
		void	removeStaticValue(uint32_t columnindex);

		/** If "ignorecolumnswithemptynames" is set true, then columns
		 *  with empty column names will be completely ignored.  It
		 *  will be as if those columns are completely absent from the
		 *  CSV, which may be important to keep in mind when specifying
		 *  indexes for primary keys or static values.
		 *
		 *  Note that "ignorecolumsnwithemptynames" is observed even if
		 *  setIgnoreColumns(true) is set. */
		void	setIgnoreColumnsWithEmptyNames(
					bool ignorecolumnswithemptynames);

		/** Configures the instance to ignore empty records. */
		void	setIgnoreEmptyRecords(bool ignoreemptyrecords);

		/** Imports data from the file set by the most recent call to
		 *  setFileName().  The table (or sequence) to import the data
		 *  into will be derived from the import file name, and may be
		 *  overridden using setObjectName().
		 *
		 *  If setCommitCount() was called with a non-zero value then a
		 *  commit will be called after every "commitcount" rows is
		 *  inserted.  No commit will be called if setCommitCount() was
		 *  never called, or if setCommitCount(0) was called.
		 *
		 *  Returns true on success and false if an error occurred. */
		virtual bool	importData();

	#include <sqlrelay/private/sqlrimportcsv.h>
};

#endif

// Copyright (c) David Muse
// See the file COPYING for more information

#ifndef SQLRIMPORTFILE_H
#define SQLRIMPORTFILE_H

#include <sqlrelay/private/sqlrimportfileincludes.h>

/** The sqlrimportfile class provides a base class for child classes that wish
 *  to implement import of data from a database, via SQL Relay, to a file.  It
 *  provides various common methods.  Each child class should implement the
 *  importData() method. */
class SQLRCLIENT_DLLSPEC sqlrimportfile : virtual public sqlrimport {
	public:
		/** Creates an instance of the sqlrimportfile class. */
		sqlrimportfile();

		/** Destroys this instance of the sqlrimportfile class. */
		virtual ~sqlrimportfile();

		/** Sets the name of the file from which data will be imported.
		 *
		 *  A NULL "filename" means import from standard input.  In
		 *  that case there is no file name to derive the name of the
		 *  table or sequence from, so setObjectName() must be called
		 *  too, or importData() will fail.
		 *
		 *  Note, however, that this class does not read any data
		 *  itself, and no child class that currently exists can import
		 *  from standard input.  Both sqlrimportcsv and sqlrimportxml
		 *  require a real file.  Their importData() methods report an
		 *  error and return false if "filename" is NULL or empty.
		 *
		 *  Not commonly called by implementations of importData() or
		 *  of the *Start/End() methods. */
		void	setFileName(const char *filename);

		/** Gets the name of the file from which data will be be
		 *  imported.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		const char	*getFileName();

		/** Imports data from the file set by the most recent call to
		 *  setFileName().
		 *
		 *  If setCommitCount() was called with a non-zero value then a
		 *  commit will be called after every "commitcount" rows is
		 *  inserted.  No commit will be called if setCommitCount() was
		 *  never called, or if setCommitCount(0) was called.
		 *
		 *  If setObjectName() was never called then the name of the
		 *  table or sequence to import into is derived from the base
		 *  name of the file, with the file extension of the format
		 *  being imported removed, if the file name has it.  An error
		 *  is reported, and false returned, if no name was set and
		 *  none can be derived, either because there is no file name
		 *  to derive it from or because nothing is left of the base
		 *  name once the extension is removed.
		 *
		 *  Returns true on success and false if an error occurred.
		 *
		 *  Note that the default implementation of this method only
		 *  resolves the object name.  It does not read any data.
		 *  Child classes that support import from a file should
		 *  override this method and call this implementation first.
		 *
		 *  Note also that this implementation accepts a NULL file
		 *  name, since a NULL file name means import from standard
		 *  input, but no child class that currently exists can do
		 *  that.  sqlrimportcsv and sqlrimportxml both reject a NULL
		 *  or empty file name after calling this implementation. */
		bool	importData();

	protected:
		/** Sets the file descriptor from which data is being imported.
 		 *
		 *  Should be called by implementations of importData().  Not
		 *  commonly called by implementations of the *Start/End()
		 *  methods. */
		void	setFileDescriptor(filedescriptor *fd);

		/** Gets the file descriptor from which data is being imported.
		 *
		 *  May be called by implementations of importData() or by
		 *  implementations of the *Start/End() methods. */
		filedescriptor	*getFileDescriptor();

	#include <sqlrelay/private/sqlrimportfile.h>
};

#endif

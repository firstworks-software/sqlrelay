// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

		virtual void	clearFlagsAndCounts();
		virtual bool	systemError();

	protected:
		bool		getInsertPrimaryKey();
		const char	*getPrimaryKeyColumnName();
		uint32_t	getPrimaryKeyColumnIndex();
		const char	*getPrimaryKeySequence();

	private:
		sqlrconnection	*sqlrcon;
		sqlrcursor	*sqlrcur;


		bool		insertprimarykey;
		char		*primarykeycolumnname;
		uint32_t	primarykeycolumnindex;
		char		*primarykeysequence;

protected:
		dictionary<uint32_t, char *>	staticvaluecolumnnames;
		dictionary<uint32_t, char *>	staticvaluecolumnvalues;
private:

		char		*dbtype;
		char		*objectname;

		bool		ignorecolumns;
		bool		ignorecolumnswithemptynames;
		bool		ignoreemptyrows;

		bool		lowercasecolumnnames;
		bool		uppercasecolumnnames;

		bool		reformatdatetime;
		bool		ddmm;
		bool		yyyyddmm;
		const char	*datedelimiters;
		uint16_t	nocenturythreshold;
		uint16_t	lastcenturythreshold;
		const char	*datetimeformat;

		uint64_t	commitcount;

		logger		*lg;
		uint8_t		coarseloglevel;
		uint8_t		fineloglevel;
		uint32_t	logindent;
		bool		logerrors;

		bool		ignorerow;
		uint64_t	currentrow;
		uint32_t	currentcol;
		char		*currentcolname;
		char		*currentfield;
		uint64_t	importedrowcount;

		dynamicarray<bool>	numericcolumn;
		dynamicarray<bool>	datetimecolumn;

	protected:
		stringbuffer		query;
		dynamicarray<char *>	columns;
		dynamicarray<char *>	fields;
		dynamicarray<bool>	quotefield;
		dictionary<const char *, const char *>	columnmap;
		dictionary<const char *, const char *>	fieldmap;

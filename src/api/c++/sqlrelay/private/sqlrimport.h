// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

		virtual void	clearFlagsAndCounts();

		void	setEmptyRow(bool emptyrow);
		bool	getEmptyRow();

		virtual bool	startProcessingColumns();
		virtual bool	processColumnName(char **cname);
		virtual bool	processPrimaryKeyAndStaticColumns();
		virtual bool	determineColumnTypes();
		virtual bool	endProcessingColumns();

		virtual bool	startProcessingRows();
		virtual bool	startProcessingRow();
		virtual bool	initialBegin();
		virtual bool	processField(char **value);
		virtual char	*massageValue(const char *value,
							bool isnumeric,
							bool isdatetime);
		virtual char	*unescapeValue(const char *value);
		virtual bool	endProcessingRow();
		virtual bool	insertRow();
		virtual bool	periodicCommit();
		virtual bool	finalCommit();
		virtual bool	endProcessingRows();

		virtual bool	systemError();

	private:
		sqlrconnection	*sqlrcon;
		sqlrcursor	*sqlrcur;

		bool		insertprimarykey;
		char		*primarykeycolumnname;
		uint32_t	primarykeycolumnindex;
		char		*primarykeysequence;

		dictionary<uint32_t, char *>	staticvaluecolumnnames;
		dictionary<uint32_t, char *>	staticvalues;

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

		logger		*lg;
		uint8_t		coarseloglevel;
		uint8_t		fineloglevel;
		uint32_t	logindent;
		bool		logerrors;

		bool			ignorerow;
		uint64_t		currentrow;
		uint32_t		currentcol;
		char			*currentcolname;
		dynamicarray<bool>	numericcolumn;
		dynamicarray<bool>	datetimecolumn;
		char			*currentfield;
		bool			emptyrow;
		uint64_t		importedrowcount;
		uint64_t		commitcount;

	protected:
		stringbuffer		query;
		dynamicarray<char *>	columns;
		dynamicarray<char *>	fields;
		dynamicarray<bool>	quotefield;
		dictionary<const char *, const char *>	columnmap;
		dictionary<const char *, const char *>	fieldmap;

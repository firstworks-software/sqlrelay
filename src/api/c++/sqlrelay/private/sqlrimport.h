// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

		virtual void	clearFlagsAndCounts();
		virtual bool	systemError();

	private:
		sqlrconnection		*sqlrcon;
		sqlrcursor		*sqlrcur;

		char			*dbtype;
		char			*objectname;

		bool			ignorecolumns;
protected:
		dictionary<const char *, const char *>	columnmap;
private:
		bool			lowercasecolumnnames;
		bool			uppercasecolumnnames;

protected:
		dictionary<const char *, const char *>	fieldmap;
private:
		bool		reformatdatetime;
		bool		ddmm;
		bool		yyyyddmm;
		const char	*datedelimiters;
		uint16_t	nocenturythreshold;
		uint16_t	lastcenturythreshold;
		const char	*datetimeformat;

		uint64_t		commitcount;

		logger			*lg;
		uint8_t			coarseloglevel;
		uint8_t			fineloglevel;
		uint32_t		logindent;
		bool			logerrors;

		bool			ignorerow;
		uint64_t		currentrow;
		uint32_t		currentcol;
		char			*currentcolname;
		char			*currentfield;
		uint64_t		importedrowcount;
		dynamicarray<bool>	numericcolumn;
		dynamicarray<bool>	datetimecolumn;

protected:
		stringbuffer		query;
		dynamicarray<char *>	columns;
		dynamicarray<char *>	fields;
		dynamicarray<bool>	quotefield;

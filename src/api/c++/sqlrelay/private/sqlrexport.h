// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

	private:
		void	clearOutput();

		sqlrconnection		*sqlrcon;
		sqlrcursor		*sqlrcur;

		bool			ignorecolumns;
		const char * const	*columnstoignore;

		const char		*filename;
		filedescriptor		*fd;

		sqlrconnection		*exportcon;
		sqlrcursor		*exportcur;
		const char		*table;
		stringbuffer		insertquery;
		uint64_t		commitcount;

		domnode			*jsondomnode;
		domnode			*columnsdomnode;
		domnode			*currentcolumndomnode;
		domnode			*rowsdomnode;
		domnode			*currentrowdomnode;
		domnode			*currentfielddomnode;

		bool			ignorerow;
		uint64_t		currentrow;
		uint32_t		currentcol;
		const char		*currentcolname;
		const char		*currentfield;
		uint64_t		exportedrowcount;
		dynamicarray<bool>	numericcolumn;

		logger			*lg;
		uint8_t			coarseloglevel;
		uint8_t			fineloglevel;
		uint32_t		logindent;

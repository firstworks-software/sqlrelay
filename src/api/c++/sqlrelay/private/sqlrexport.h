// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

		virtual void	clearOutput();
		virtual bool	systemError();

	private:

		sqlrconnection		*sqlrcon;
		sqlrcursor		*sqlrcur;

		const char		*table;

		bool			ignorecolumns;
		const char * const	*columnstoignore;

		logger			*lg;
		uint8_t			coarseloglevel;
		uint8_t			fineloglevel;
		uint32_t		logindent;

		bool			ignorerow;
		uint64_t		currentrow;
		uint32_t		currentcol;
		const char		*currentcolname;
		const char		*currentfield;
		uint64_t		exportedrowcount;
		dynamicarray<bool>	numericcolumn;

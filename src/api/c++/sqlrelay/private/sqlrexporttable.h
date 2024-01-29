// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

		virtual void	clearOutput();

	private:
		sqlrconnection		*exportcon;
		sqlrcursor		*exportcur;
		stringbuffer		insertquery;
		uint64_t		commitcount;

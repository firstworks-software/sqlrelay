// Copyright (c) David Muse
// See the file COPYING for more information

		virtual void	clearFlagsAndCounts();

		virtual bool	sanityCheck();

	private:

		domnode			*topdomnode;
		domnode			*columnsdomnode;
		domnode			*currentcolumndomnode;
		domnode			*rowsdomnode;
		domnode			*currentrowdomnode;
		domnode			*currentfielddomnode;

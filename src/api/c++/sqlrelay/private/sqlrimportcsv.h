// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

	private:
		bool	headerStart();
		bool	column(const char *name, bool quoted);
		bool	headerEnd();
		bool	bodyStart();
		bool	recordStart();
		bool	field(const char *value, bool quoted);
		bool	recordEnd();
		bool	bodyEnd();

		void	appendField(stringbuffer *strb,
					const char *value,
					bool isnumeric,
					bool isdatetime);

		uint32_t	currenttablecol;
		bool		foundfieldtext;
		uint32_t	fieldcount;
		bool		emptyrecord;
		uint64_t	recordcount;
		uint64_t	committedcount;

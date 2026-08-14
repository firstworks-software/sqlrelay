// Copyright (c) David Muse
// See the file COPYING for more information

	private:
		bool	headerStart();
		bool	column(const char *name, bool quoted);
		bool	headerEnd();
		bool	bodyStart();
		bool	recordStart();
		bool	field(const char *value, size_t valuelength,
								bool quoted);
		bool	recordEnd();
		bool	bodyEnd();

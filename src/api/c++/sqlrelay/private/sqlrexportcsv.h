// Copyright (c) David Muse
// See the file COPYING for more information

	private:

		bool	exportColumnName(bool first);
		bool	endProcessingColumns();
		bool	exportField(bool first);
		bool	endProcessingRow();

		bool	needsQuotes(const char *value, uint32_t length);

		bool	escapeValue(filedescriptor *fd, const char *field);
		bool	escapeValue(filedescriptor *fd,
					const char *field, uint32_t length);

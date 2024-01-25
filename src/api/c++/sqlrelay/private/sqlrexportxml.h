// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

		virtual	bool	exportToFile(const char *filename);
	private:
		void	escapeField(filedescriptor *fd, const char *field);

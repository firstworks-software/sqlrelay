// Copyright (c) David Muse
// See the file COPYING for more information.

		void		setExtension(const char *extension);
		const char	*getExtension();

	private:
		const char	*filename;
		filedescriptor	*fd;
		const char	*extension;

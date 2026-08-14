// Copyright (c) David Muse
// See the file COPYING for more information

	private:
		const char	*user;
		const char	*password;
		uint64_t	passwordsize;
		const char	*method;
		const char	*extra;
		// unlike the members above, this one is owned
		byte_t		*sessionkey;
		uint64_t	sessionkeysize;

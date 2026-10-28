// Copyright (c) David Muse
// See the file COPYING for more information.

		void	setValue(const char *v);
		bool	clear();

	private:
		sqlrscalarprivate	*pvt;

		// copying is not allowed (pvt is owned)
		sqlrscalar(const sqlrscalar &x);
		sqlrscalar &operator=(const sqlrscalar &x);

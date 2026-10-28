// Copyright (c) David Muse
// See the file COPYING for more information.

		void	setValue(uint64_t row, uint64_t col, const char *value);
		void	setColumnName(uint64_t col, const char *name);
		bool	clear();

	private:
		sqlrresultsettableprivate	*pvt;

		// copying is not allowed (pvt is owned)
		sqlrresultsettable(const sqlrresultsettable &x);
		sqlrresultsettable &operator=(const sqlrresultsettable &x);

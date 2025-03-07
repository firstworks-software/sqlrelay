// Copyright (c) David Muse
// See the file COPYING for more information

	private:
		domnode	*getTransport(const char *transportid);
		char	*substitutions(sqlrlistener *sqlrl,
						sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char *str,
						const char *event,
						const char *info);

		sqlrnotificationprivate	*pvt;

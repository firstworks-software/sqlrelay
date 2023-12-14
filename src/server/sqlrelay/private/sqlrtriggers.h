// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

	private:
		void	unload();
		sqlrtriggerplugin	*loadTrigger(domnode *trigger);
		bool	runBefore(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrtriggerplugin * > *list);
		bool	runAfter(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrtriggerplugin * > *list);

		sqlrtriggersprivate	*pvt;

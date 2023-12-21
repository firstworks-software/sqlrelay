// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

	public:
		sqlrservercontroller();
		~sqlrservercontroller();

		bool	init(int argc, const char **argv);
		bool	listen();

		void	reLogIn();

	private:
		void	setUserAndGroup();

		sqlrserverconnection	*initConnection(const char *dbase);

		bool	handlePidFile();

		void	initDatabaseAvailableFileName();

		bool	attemptLogIn(bool printerrors);
		bool	logIn(bool printerrors);
		void	logOut();

		void	setAutoCommit(bool ac);

		bool			initCursors(uint16_t count);
		sqlrservercursor	*newCursor(uint16_t id);

		void	incrementConnectionCount();
		void	decrementConnectionCount();

		void	markDatabaseAvailable();
		void	markDatabaseUnavailable();

		bool	openSockets();

		void	waitForAvailableDatabase();

		void	initSession();

		bool	announceAvailability(const char *connectionid);

		bool	registerForHandoff();
		void	deRegisterForHandoff();

		int32_t	waitForClient();
		bool	getProtocol();
		void	clientSession();

		bool	beginFakeTransactionBlock();
		void	endTransaction(bool commit);
		void	clearColumnCaches();
		bool	endFakeTransactionBlock();
		bool	checkInterceptQuery(sqlrservercursor *cursor);
		bool	interceptQuery(sqlrservercursor *cursor);

		void	translateBindVariablesFromMappings(
						sqlrservercursor *cursor);
		bool	applyDirectives(sqlrservercursor *cursor);
		bool	translateQuery(sqlrservercursor *cursor);
		bool	translateQueryWithParser(sqlrservercursor *cursor,
						stringbuffer *translatedquery);
		bool	translateQueryWithoutParser(sqlrservercursor *cursor,
						stringbuffer *translatedquery);
		void	translateBindVariables(sqlrservercursor *cursor);
		bool	matchesNativeBindFormat(const char *bind);
		void	translateBindVariableInStringAndMap(
					sqlrservercursor *cursor,
					stringbuffer *currentbind,
					uint16_t bindindex,
					stringbuffer *newquery);
		void	mapBindVariable(sqlrservercursor *cursor,
					const char *variablename,
					uint64_t variablenamesize,
					uint16_t bindindex);

		void	translateBeginTransaction(sqlrservercursor *cursor);

		bool	filterQuery(sqlrservercursor *cursor, bool before);

		bool	handleBinds(sqlrservercursor *cursor);

		void		buildColumnMaps();
		uint32_t	mapColumn(uint32_t col);
		uint32_t	mapColumnCount(uint32_t colcount);

		void	commitOrRollback(sqlrservercursor *cursor);

		void	dropTempTables(sqlrservercursor *cursor);
		void	dropTempTable(sqlrservercursor *cursor,
						const char *tablename);
		void	truncateTempTables(sqlrservercursor *cursor);
		void	truncateTempTable(sqlrservercursor *cursor,
						const char *tablename);

		void	closeSuspendedSessionSockets();

		void	shutDown();

		void	closeCursors(bool destroy);

		bool	createSharedMemoryAndSemaphores(const char *id);

		void	decrementConnectedClientCount();

		bool	acquireAnnounceMutex();
		void	releaseAnnounceMutex();

		void	signalListenerToRead();
		void	unSignalListenerToRead();
		bool	waitForListenerToFinishReading();
		void	signalListenerToHandoff();

		void	acquireConnectionCountMutex();
		void	releaseConnectionCountMutex();

		void	signalScalerToRead();

		void	initConnStats();
		void	clearConnStats();

		sqlrparser	*newParser();

		void	setClientSessionStartTime();
		void	setCurrentUser(const char *user, uint32_t usersize);
		void	setClientAddr();


		void	sessionStartQueries();
		void	sessionEndQueries();
		void	sessionQuery(const char *query);

		void	getColumnsInTable(const char *table, 
					linkedlist<char *> **columns,
					const char **autoinccolumn,
					const char **primarykeycolumn);
		void	getColumnsFromInsertQuery(
					const char *start,
					const char *end,
					linkedlist<char *> *columns);
		void	getFirstValuesFromInsertQuery(
					const char *start,
					const char *end,
					linkedlist<char *> *values,
					bool *mutiinsert);
		void	deriveColumnsFromInsertQuery(
					linkedlist<char *> *values,
					linkedlist<char *> *allcolumns,
					linkedlist<char *> *columns);

		sqlrdatabaseobject *createDatabaseObject(
						const char *database,
						const char *schema,
						const char *name,
						const char *dependency);
		void	setReplacementName(
				dictionary< sqlrdatabaseobject *, char *> *dict,
				sqlrdatabaseobject *oldobject,
				const char *newobject);
		bool	getReplacementName(
				dictionary< sqlrdatabaseobject *, char *> *dict,
				const char *database,
				const char *schema,
				const char *oldobject,
				const char **newobject);
		bool	removeReplacement(
				dictionary< sqlrdatabaseobject *, char *> *dict,
				const char *database,
				const char *schema,
				const char *object);

		bool	fakePrepareAndExecuteForApiCall(
						sqlrservercursor *cursor);

		static void     alarmHandler(int32_t signum);

		sqlrservercontrollerprivate	*pvt;

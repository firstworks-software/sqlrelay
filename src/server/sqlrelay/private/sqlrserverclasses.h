// Copyright (c) 1999-2019 David Muse
// See the file COPYING for more information

class SQLRSERVER_DLLSPEC sqlrprotocols {
	public:
		sqlrprotocols(sqlrservercontroller *cont);
		~sqlrprotocols();

		bool		load(domnode *listeners);
		sqlrprotocol	*getProtocol(uint16_t port);

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadProtocol(uint16_t index, domnode *listener);

		sqlrprotocolsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrauths {
	public:
		sqlrauths(sqlrservercontroller *cont);
		~sqlrauths();

		bool		load(domnode *parameters,
					sqlrpwdencs *sqlrpe);
		const char	*auth(sqlrcredentials *cred);

		void	endSession();

	private:
		void	unload();
		void	loadAuth(domnode *auth, sqlrpwdencs *sqlrpe);

		sqlrauthsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrloggers {
	public:
		sqlrloggers(sqlrpaths *sqlrpth);
		~sqlrloggers();

		bool	load(domnode *parameters);
		void	init(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon);
		void	run(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event,
				const char *info);

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void		unload();
		void		loadLogger(domnode *logger);

		sqlrloggersprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrnotifications {
	public:
		sqlrnotifications(sqlrpaths *sqlrpth);
		~sqlrnotifications();

		bool	load(domnode *parameters);
		void	run(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrevent_t event,
					const char *info);

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void		unload();
		void		loadNotification(domnode *notification);

		sqlrnotificationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrschedules {
	public:
		sqlrschedules(sqlrservercontroller *cont);
		~sqlrschedules();

		bool	load(domnode *parameters);
		bool	allowed(sqlrserverconnection *sqlrcon,
						const char *user);

		void	endSession();

	private:
		void		unload();
		void		loadSchedule(domnode *schedule);

		sqlrschedulesprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrrouters {
	friend class routerconnection;
	friend class routercursor;
	public:
		sqlrrouters(sqlrservercontroller *cont,
				const char **connectionids,
				sqlrconnection **connections,
				uint16_t connectioncount);
		~sqlrrouters();

		bool		load(domnode *parameters);
		const char	*route(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char **err,
						int64_t *errn);
		bool	routeEntireSession();

		void	endTransaction(bool commit);
		void	endSession();

		const char	*getCurrentConnectionId();
		const char	**getConnectionIds();
		sqlrconnection	**getConnections();
		uint16_t	getConnectionCount();

	private:
		void		unload();
		void		loadRouter(domnode *route);

		void	setCurrentConnectionId(const char *connid);

		sqlrroutersprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrdirectives {
	public:
		sqlrdirectives(sqlrservercontroller *cont);
		~sqlrdirectives();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query);

	private:
		void	unload();
		void	loadDirective(domnode *directive);

		sqlrdirectivesprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrquerytranslations {
	public:
		sqlrquerytranslations(sqlrservercontroller *cont);
		~sqlrquerytranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						sqlrparser *sqlrp,
						const char *query,
						uint32_t querysize,
						stringbuffer *translatedquery);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

		bool	getUseOriginalOnError();

	private:
		void	unload();
		void	loadTranslation(domnode *translation);

		sqlrdatabaseobject *createDatabaseObject(
						const char *database,
						const char *schema,
						const char *object,
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
				const char *oldobject);

		sqlrquerytranslationsprivate	*pvt;
};

typedef sqlrquerytranslations sqlrtranslations;

class SQLRSERVER_DLLSPEC sqlrfilters {
	public:
		sqlrfilters(sqlrservercontroller *cont);
		~sqlrfilters();

		bool	load(domnode *parameters);
		bool	runBeforeFilters(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						sqlrparser *sqlrp,
						const char *query,
						const char **err,
						int64_t *errn);
		bool	runAfterFilters(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						sqlrparser *sqlrp,
						const char *query,
						const char **err,
						int64_t *errn);

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadFilter(domnode *filter,
				singlylinkedlist< sqlrfilterplugin * > *list);
		bool	run(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrparser *sqlrp,
				const char *query,
				const char **err,
				int64_t *errn,
				singlylinkedlist< sqlrfilterplugin * > *list);

		sqlrfiltersprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrbindvariabletranslations {
	public:
		sqlrbindvariabletranslations(sqlrservercontroller *cont);
		~sqlrbindvariabletranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadBindVariableTranslation(
					domnode *bindvariabletranslation);

		sqlrbindvariabletranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsettranslations {
	public:
		sqlrresultsettranslations(sqlrservercontroller *cont);
		~sqlrresultsettranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char *fieldname,
						uint32_t fieldindex,
						const char **field,
						uint64_t *fieldsize);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadResultSetTranslation(
					domnode *resultsettranslation);

		sqlrresultsettranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowtranslations {
	public:
		sqlrresultsetrowtranslations(sqlrservercontroller *cont);
		~sqlrresultsetrowtranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						uint32_t colcount,
						const char * const *fieldnames,
						const char ***fields,
						uint64_t **fieldsizes);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadResultSetRowTranslation(
					domnode *resultsetrowtranslation);

		sqlrresultsetrowtranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowblocktranslations {
	public:
		sqlrresultsetrowblocktranslations(sqlrservercontroller *cont);
		~sqlrresultsetrowblocktranslations();

		bool	load(domnode *parameters);

		uint64_t	getRowBlockCount();

		bool	setRow(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames,
					const char * const *fields,
					uint64_t *fieldsizes,
					bool *lobs,
					bool *nulls);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames);
		bool	getRow(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char ***fields,
					uint64_t **fieldsizes,
					bool **lobs,
					bool **nulls);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadResultSetRowBlockTranslation(
					domnode *resultsetrowblocktranslation);

		sqlrresultsetrowblocktranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsetheadertranslations {
	public:
		sqlrresultsetheadertranslations(sqlrservercontroller *cont);
		~sqlrresultsetheadertranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char ***columnnames,
					uint16_t **columnnamesizes,
					uint16_t **columntypes,
					const char ***columntypenames,
					uint16_t **columntypenamesizes,
					uint32_t **columnsizes,
					uint32_t **columnprecisions,
					uint32_t **columnscales,
					uint16_t **columnisnullables,
					uint16_t **columnisprimarykeys,
					uint16_t **columnisuniques,
					uint16_t **columnispartofkeys,
					uint16_t **columnisunsigneds,
					uint16_t **columniszerofilleds,
					uint16_t **columnisbinarys,
					uint16_t **columnisautoincrements,
					const char ***columntables,
					uint16_t **columntablesizes);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadResultSetHeaderTranslation(
					domnode *resultsetheadertranslation);

		sqlrresultsetheadertranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrerrortranslations {
	public:
		sqlrerrortranslations(sqlrservercontroller *cont);
		~sqlrerrortranslations();

		bool	load(domnode *parameters);
		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					int64_t errornumber,
					const char *error,
					uint32_t errorsize,
					int64_t *translatederrornumber,
					stringbuffer *translatederror);

		const char	*getError();

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadErrorTranslation(domnode *errortranslation);

		sqlrerrortranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrtriggers {
	public:
		sqlrtriggers(sqlrservercontroller *cont);
		~sqlrtriggers();

		bool	load(domnode *parameters);
		bool	runBeforeTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runAfterTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);

		void	endTransaction(bool commit);
		void	endSession();

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
};

class SQLRSERVER_DLLSPEC sqlrqueries {
	public:
		sqlrqueries(sqlrservercontroller *cont);
		~sqlrqueries();

		bool		load(domnode *parameters);
		sqlrquerycursor	*match(sqlrserverconnection *sqlrcon,
						const char *querystring,
						uint32_t querysize,
						uint16_t id);

		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadQuery(domnode *logger);

		sqlrqueriesprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrmoduledatas {
	public:
		sqlrmoduledatas(sqlrservercontroller *cont);
		~sqlrmoduledatas();

		bool	load(domnode *parameters);

		sqlrmoduledata	*getModuleData(const char *id);

		void	closeResultSet(sqlrservercursor *sqlrcur);
		void	endTransaction(bool commit);
		void	endSession();

	private:
		void	unload();
		void	loadModuleData(domnode *moduledata);

		sqlrmoduledatasprivate	*pvt;
};

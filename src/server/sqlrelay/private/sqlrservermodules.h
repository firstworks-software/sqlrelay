// Copyright (c) David Muse
// See the file COPYING for more information

class SQLRSERVER_DLLSPEC sqlrmoduleplugin {
	public:
		sqlrservermodule	*m;
		dynamiclib		*dl;
		const char		*module;
};

class sqlrservermodulesprivate;

class SQLRSERVER_DLLSPEC sqlrservermodules : public sqlrserverbase {
	public:
		sqlrservermodules(sqlrservercontroller *cont,
						domnode *parameters);
		virtual	~sqlrservermodules();

		domnode	*getParameters();
		bool	isModuleDisabled(domnode *parameters);

		void	debugStart(const char *title, ...);
		void	debugWrite(const char *string, ...);
		void	debugEnd();

		virtual bool	load();
		virtual void	unload();
		virtual void	endTransaction(bool commit);
		virtual void	endSession();

	protected:
		virtual void	loadModule(domnode *parameters);
		const char	*getModuleName(domnode *parameters);

		singlylinkedlist< sqlrmoduleplugin * >	blist;
		singlylinkedlist< sqlrmoduleplugin * >	alist;

		sqlrservercontroller	*cont;

		sqlrservermodulesprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrprotocols : public sqlrservermodules {
	public:
		sqlrprotocols(sqlrservercontroller *cont, domnode *parameters);
		~sqlrprotocols();

		bool	load();
		sqlrprotocol	*getProtocol(uint16_t port);

	private:
		void	loadModule(domnode *parameters, uint16_t index);

		sqlrprotocolsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrauths : public sqlrservermodules {
	public:
		sqlrauths(sqlrservercontroller *cont, domnode *parameters);
		~sqlrauths();

		bool	load();
		const char	*auth(sqlrcredentials *cred);

	private:
		void	loadModule(domnode *parameters);

		sqlrauthsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrloggers : public sqlrservermodules {
	public:
		sqlrloggers(sqlrpaths *sqlrpth, domnode *parameters);
		~sqlrloggers();

		void	init(sqlrlistener *sqlrl,
				sqlrservercontroller *sqlrc);
		void	start(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event,
				const char *info);
		void	write(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event,
				const char *info);
		void	end(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event);

	private:
		void	loadModule(domnode *parameters);

		sqlrloggersprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrnotifications : public sqlrservermodules {
	public:
		sqlrnotifications(sqlrpaths *sqlrpth, domnode *parameters);
		~sqlrnotifications();

		void	run(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrevent_t event,
					const char *info);

	private:
		void	loadModule(domnode *parameters);

		sqlrnotificationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrschedules : public sqlrservermodules {
	public:
		sqlrschedules(sqlrservercontroller *cont, domnode *parameters);
		~sqlrschedules();

		bool	allowed(sqlrserverconnection *sqlrcon,
						const char *user);

	private:
		void	loadModule(domnode *parameters);

		sqlrschedulesprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrdirectives : public sqlrservermodules {
	public:
		sqlrdirectives(sqlrservercontroller *cont, domnode *parameters);
		~sqlrdirectives();

		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query);

	private:
		void	loadModule(domnode *parameters);

		sqlrdirectivesprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrquerytranslations : public sqlrservermodules {
	public:
		sqlrquerytranslations(sqlrservercontroller *cont,
						domnode *parameters);
		~sqlrquerytranslations();

		bool	load();
		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						sqlrparser *sqlrp,
						const char *query,
						uint32_t querysize,
						stringbuffer *translatedquery);

		const char	*getError();
		bool	getUseOriginalOnError();

	private:
		void	loadModule(domnode *parameters);

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

class SQLRSERVER_DLLSPEC sqlrfilters : public sqlrservermodules {
	public:
		sqlrfilters(sqlrservercontroller *cont, domnode *parameters);
		~sqlrfilters();

		bool	load();
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

	private:
		void	loadModule(domnode *parameters,
				singlylinkedlist< sqlrmoduleplugin * > *list);
		bool	run(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrparser *sqlrp,
				const char *query,
				const char **err,
				int64_t *errn,
				singlylinkedlist< sqlrmoduleplugin * > *list);

		sqlrfiltersprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrbindvariabletranslations :
						public sqlrservermodules {
	public:
		sqlrbindvariabletranslations(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrbindvariabletranslations();

		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur);
		const char	*getError();

	private:
		void	loadModule(domnode *parameters);

		sqlrbindvariabletranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsettranslations : public sqlrservermodules {
	public:
		sqlrresultsettranslations(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrresultsettranslations();

		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						const char *fieldname,
						uint32_t fieldindex,
						const char **field,
						uint64_t *fieldsize);

		const char	*getError();

	private:
		void	loadModule(domnode *parameters);

		sqlrresultsettranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowtranslations :
						public sqlrservermodules {
	public:
		sqlrresultsetrowtranslations(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrresultsetrowtranslations();

		bool	run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						uint32_t colcount,
						const char * const *fieldnames,
						const char ***fields,
						uint64_t **fieldsizes);

		const char	*getError();

	private:
		void	loadModule(domnode *parameters);

		sqlrresultsetrowtranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsetrowblocktranslations :
						public sqlrservermodules {
	public:
		sqlrresultsetrowblocktranslations(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrresultsetrowblocktranslations();

		bool	load();

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

	private:
		void	loadModule(domnode *parameters);

		sqlrresultsetrowblocktranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrresultsetheadertranslations :
						public sqlrservermodules {
	public:
		sqlrresultsetheadertranslations(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrresultsetheadertranslations();

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

	private:
		void	loadModule(domnode *parameters);

		sqlrresultsetheadertranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrerrortranslations : public sqlrservermodules {
	public:
		sqlrerrortranslations(sqlrservercontroller *cont,
						domnode *parameters);
		~sqlrerrortranslations();

		bool	run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					int64_t errornumber,
					const char *error,
					uint32_t errorsize,
					int64_t *translatederrornumber,
					stringbuffer *translatederror);

		const char	*getError();

	private:
		void	loadModule(domnode *parameters);

		sqlrerrortranslationsprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrtriggers : public sqlrservermodules {
	public:
		sqlrtriggers(sqlrservercontroller *cont, domnode *parameters);
		~sqlrtriggers();

		bool	load();
		bool	runBeforePrepareTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runAfterPrepareTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runBeforeExecuteTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);
		bool	runAfterExecuteTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur);

	private:
		void	loadModule(domnode *trigger, sqlrmoduleplugin **plugin);
		bool	runBeforePrepare(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list);
		bool	runAfterPrepare(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list);
		bool	runBeforeExecute(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list);
		bool	runAfterExecute(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list);

		singlylinkedlist< sqlrmoduleplugin * >	bplist;
		singlylinkedlist< sqlrmoduleplugin * >	aplist;

		sqlrtriggersprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrqueries : public sqlrservermodules {
	public:
		sqlrqueries(sqlrservercontroller *cont, domnode *parameters);
		~sqlrqueries();

		sqlrquerycursor	*match(sqlrserverconnection *sqlrcon,
						const char *querystring,
						uint32_t querysize,
						uint16_t id);

	private:
		void	loadModule(domnode *parameters);

		sqlrqueriesprivate	*pvt;
};

class SQLRSERVER_DLLSPEC sqlrmoduledatas : public sqlrservermodules {
	public:
		sqlrmoduledatas(sqlrservercontroller *cont,
						domnode *parameters);
		~sqlrmoduledatas();

		sqlrmoduledata	*getModuleData(const char *id);

		void	closeResultSet(sqlrservercursor *sqlrcur);

	private:
		void	loadModule(domnode *parameters);

		sqlrmoduledatasprivate	*pvt;
};

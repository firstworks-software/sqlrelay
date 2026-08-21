// Copyright (c) David Muse
// See the file COPYING for more information
#include <rudiments/commandline.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>
#include <rudiments/signalclasses.h>
#include <rudiments/dictionary.h>
#include <rudiments/linkedlist.h>
#include <rudiments/file.h>
#include <rudiments/permissions.h>
#include <rudiments/dynamiclib.h>
#include <rudiments/snooze.h>

#include "sqlrbench.h"

#define ORACLE_SID "(DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = oracle)(PORT = 1521)) (CONNECT_DATA = (SERVER = DEDICATED) (SERVICE_NAME = ora1)))"

// the sqlrelay instance that each database is tested against
// (see test/sqlrelay.conf.d/*.conf.in - the socket is /tmp/<socket>.socket)
struct dbinstance {
	const char	*db;
	const char	*socket;
	uint16_t	port;
};

static const dbinstance	dbinstances[]={
	{"oracle","oracle",9001},
	{"oracle8","oracle",9001},
	{"mysql","mysql",9002},
	{"mysqlssl","mysql",9002},
	{"postgresql","postgresql",9003},
	{"postgresqlssl","postgresql",9003},
	{"sqlite","sqlite",9004},
	{"freetds","freetds-sap",9005},
	{"sap","sap",9006},
	{"sybase","sap",9006},
	{"db2","db2",9008},
	{"firebird","firebird",9009},
	{"informix","informix",9010},
	{NULL,NULL,0}
};

void shutDown(int32_t signum);
void graphStats(const char *graph, const char *db,
		dictionary< float, linkedlist< float > *> *stats);

sqlrbench	*bm;
bool		stop;

int main(int argc, const char **argv) {

	// process the command line
	commandline	cmdl(argc,argv);

	// show usage?
	bool	usage=false;

	// default parameters
	const char	*db="oracle";
	const char	*dbconnectstring=NULL;
	const char	*proxyconnectstring=NULL;
	const char	*sqlrconnectstring=NULL;
	const char	*sqlr="local";
	uint64_t	queries=30;
	uint64_t	rows=256;
	uint32_t	cols=16;
	uint32_t	colsize=32;
	uint16_t	samples=10;
	uint64_t	rsbs=0;
	bool		benchdb=true;
	bool		benchproxy=false;
	bool		benchsqlrelay=true;
	bool		selectqueries=true;
	bool		dmlqueries=true;
	bool		debug=false;
	const char	*selectsgraph=NULL;
	const char	*dmlgraph=NULL;
	bool		nosettle=false;

	// override defaults with command line parameters
	if (cmdl.isFound("db")) {
		db=cmdl.getValue("db");
	}
	if (cmdl.isFound("dbconnectstring")) {
		dbconnectstring=cmdl.getValue("dbconnectstring");
	}
	if (cmdl.isFound("proxyconnectstring")) {
		proxyconnectstring=cmdl.getValue("proxyconnectstring");
	}
	if (cmdl.isFound("sqlrconnectstring")) {
		sqlrconnectstring=cmdl.getValue("sqlrconnectstring");
	}
	if (cmdl.isFound("sqlr")) {
		sqlr=cmdl.getValue("sqlr");
		if (charstring::compare(sqlr,"local") &&
			charstring::compare(sqlr,"remote") &&
			charstring::compare(sqlr,"db")) {
			usage=true;
		}
	}
	if (cmdl.isFound("queries")) {
		queries=charstring::convertToInteger(cmdl.getValue("queries"));
	}
	if (cmdl.isFound("rows")) {
		rows=charstring::convertToInteger(cmdl.getValue("rows"));
	}
	if (cmdl.isFound("cols")) {
		cols=charstring::convertToInteger(cmdl.getValue("cols"));
	}
	if (cmdl.isFound("colsize")) {
		colsize=charstring::convertToInteger(cmdl.getValue("colsize"));
	}
	if (cmdl.isFound("samples")) {
		samples=charstring::convertToInteger(cmdl.getValue("samples"));
	}
	if (cmdl.isFound("rsbs")) {
		rsbs=charstring::convertToInteger(cmdl.getValue("rsbs"));
	}
	if (cmdl.isFound("bench")) {
		const char	*bench=cmdl.getValue("bench");
		benchsqlrelay=charstring::contains(bench,"sqlrelay");
		benchproxy=charstring::contains(bench,"proxy");
		benchdb=charstring::contains(bench,"db");
	}
	if (cmdl.isFound("querytypes")) {
		const char	*queries=cmdl.getValue("querytypes");
		selectqueries=charstring::contains(queries,"selects");
		dmlqueries=charstring::contains(queries,"dml");
	}
	if (cmdl.isFound("debug")) {
		debug=true;
	}
	if (cmdl.isFound("nosettle")) {
		nosettle=true;
	}
	stringbuffer	selectsgraphname;
	if (cmdl.isFound("selectsgraph")) {
		selectsgraph=cmdl.getValue("selectsgraph");
		selectsgraphname.append(selectsgraph);
		if (charstring::compare(
			charstring::findFirst(selectsgraph,".png"),".png")) {
			selectsgraphname.append(".png");
		}
	} else {
		selectsgraphname.append(db)->append("-selects-bench.png");
	}
	selectsgraph=selectsgraphname.getString();
	stringbuffer	dmlgraphname;
	if (cmdl.isFound("dmlgraph")) {
		dmlgraph=cmdl.getValue("dmlgraph");
		dmlgraphname.append(dmlgraph);
		if (charstring::compare(
			charstring::findFirst(dmlgraph,".png"),".png")) {
			dmlgraphname.append(".png");
		}
	} else {
		dmlgraphname.append(db)->append("-dml-bench.png");
	}
	dmlgraph=dmlgraphname.getString();
	if (cmdl.isFound("help","h")) {
		usage=true;
	}

	// usage info
	if (usage) {
		stdoutput.printf(
			"usage: sqlr-bench \\\n"
			"	[-db db2|informix|firebird|freetds|mysql|"
			"oracle|postgresql|sap|sqlite|odbc] \\\n"
			"	[-dbconnectstring dbconnectstring] \\\n"
			"	[-proxyconnectstring proxyconnectstring] \\\n"
			"	[-sqlrconnectstring sqlrconnectstring] \\\n"
			"	[-sqlr [local|remote|db]] \\\n"
			"	[-queries total-query-count] \\\n"
			"	[-rows rows-per-query] \\\n"
			"	[-cols columns-per-row] \\\n"
			"	[-colsize characters-per-column] \\\n"
			"	[-samples samples-per-test] \\\n"
			"	[-rsbs result-set-buffer-size] \\\n"
			"	[-querytypes [selects],[dml]] \\\n"
			"	[-bench [sqlrelay],[proxy],[db]] \\\n"
			"	[-debug] \\\n"
			"	[-selectsgraph selects-graph-file-name] \\\n"
			"	[-dmlgraph dml-graph-file-name] \\\n"
			"	[-nosettle]\n");
		process::exit(1);
	}

	// handle signals
	bm=NULL;
	stop=false;
	process::setShutDownHandler(shutDown);
	process::setCrashHandler(shutDown);

	// init stats
	dictionary< float, linkedlist< float > *>	selectstats;
	dictionary< float, linkedlist< float > *>	dmlstats;
	selectstats.setManageValues(true);
	dmlstats.setManageValues(true);

	// for each database...
	dynamiclib	sqlrdl;
	dynamiclib	proxydl;
	dynamiclib	dbdl;
	dynamiclib	*dl;

	// find the sqlrelay instance that this database is tested against
	const char	*sqlrsocket=NULL;
	uint16_t	sqlrport=0;
	for (const dbinstance *dbi=dbinstances; dbi->db; dbi++) {
		if (!charstring::compare(db,dbi->db)) {
			sqlrsocket=dbi->socket;
			sqlrport=dbi->port;
			break;
		}
	}
	if (benchsqlrelay && !sqlrconnectstring && !sqlrsocket) {
		stdoutput.printf("no sqlrelay instance is defined for "
					"database %s, "
					"use -sqlrconnectstring\n",db);
		process::exit(1);
	}

	// default sqlrelay connect string
	stringbuffer	sqlrc;
	if (sqlrsocket) {
		if (!charstring::compare(sqlr,"local")) {
			sqlrc.append("socket=/tmp/")->append(sqlrsocket);
			sqlrc.append(".socket;");
		} else if (!charstring::compare(sqlr,"remote")) {
			sqlrc.append("host=sqlrelay;port=");
			sqlrc.append(sqlrport)->append(";");
		} else if (!charstring::compare(sqlr,"db")) {
			sqlrc.append("host=")->append(db)->append(";port=");
			sqlrc.append(sqlrport)->append(";");
		}
		if (!charstring::compare(db,"db2")) {
			sqlrc.append("user=db2inst1;");
		} else {
			sqlrc.append("user=testuser;");
		}
		sqlrc.append("password=testpassword;debug=no");
	}

	// first sqlrelay, then proxy, then direct
	for (uint16_t i=0; i<3; i++) {

		bool	sqlrelay=(i==0);
		bool	proxy=(i==1);
		bool	direct=(i==2);

		// skip tests we don't want to run
		if ((!benchsqlrelay && i==0) ||
			(!benchproxy && i==1) ||
			(!benchdb && i==2)) {
			continue;
		}

		// skip proxy tests except for mysql
		if (proxy &&  charstring::compare(db,"mysql")) {
			continue;
		}

		if (sqlrelay) {
			stdoutput.printf("\nbenchmarking "
						"%s via sqlrelay:\n\n",db);
			dl=&sqlrdl;
		} else if (proxy) {
			stdoutput.printf("\nbenchmarking "
						"%s via proxy:\n\n",db);
			dl=&proxydl;
		} else {
			stdoutput.printf("\nbenchmarking "
						"%s directly:\n\n",db);
			dl=&dbdl;
		}

		// default connect strings
		if (sqlrelay) {
			if (!sqlrconnectstring) {
				sqlrconnectstring=sqlrc.getString();
			}
		} else if (proxy) {
			if (!charstring::compare(db,"mysql")) {
				if (!proxyconnectstring) {
					proxyconnectstring=
					"socket=/tmp/mysqlprotocol.socket;"
					"db=testdb;"
					"user=testuser;"
					"password=testpassword;";
				}
			}
		} else if (!charstring::compare(db,"db2")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"db=testdb;lang=C;"
					"user=db2inst1;"
					"password=testpassword;";
			}
		} else if (!charstring::compare(db,"informix")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"informixdir=/opt/informix;"
					"servername=ol_informix1210;"
					"db=testdb;"
					"user=testuser;"
					"password=testpassword;";
			}
		} else if (!charstring::compare(db,"firebird")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"user=testuser;"
					"password=testpassword;"
					"db=firebird:"
					"/opt/firebird/testdb.gdb;"
					"dialect=3";
			}
		} else if (!charstring::compare(db,"freetds")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"sybase=/etc;"
					"server=mssql;"
					"db=testdb;"
					"user=testuser;"
					"password=testpassword;";
			}
		} else if (!charstring::compare(db,"mysql")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"host=mysql;"
					"db=testdb;"
					"user=testuser;"
					"password=testpassword;";
			}
		} else if (!charstring::compare(db,"mysqlssl")) {
			if (!dbconnectstring) {
				dbconnectstring=
				"host=mysql;"
				"db=testdb;"
				"user=testuser;"
				"password=testpassword;"
				"sslca=/etc/mysql-ssl/ca-cert.pem;"
				"sslcert=/etc/mysql-ssl/client-cert.pem;"
				"sslkey=/etc/mysql-ssl/client-key.pem";
			}
		} else if (!charstring::compare(db,"oracle") ||
				!charstring::compare(db,"oracle8")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"sid=" ORACLE_SID ";"
					"user=testuser;"
					"password=testpassword;";
			}
		} else if (!charstring::compare(db,"postgresql")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"user=testuser;"
					"password=testpassword;"
					"db=testdb;"
					"host=postgresql;";
			}
		} else if (!charstring::compare(db,"postgresqlssl")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"user=testuser;"
					"password=testpassword;"
					"db=testdb;"
					"host=postgresql;"
					"sslmode=verify-ca;";
			}
		} else if (!charstring::compare(db,"sqlite")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"db=../sqlrelay.conf.d/sqlite/sqlite;";
			}
		} else if (!charstring::compare(db,"sap") ||
				!charstring::compare(db,"sybase")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"sybase=/opt/sap;"
					"lang=en_US;"
					"server=SAP;"
					"db=testdb;"
					"user=testuser;"
					"password=testpassword;";
			}
		} else if (!charstring::compare(db,"odbc")) {
			if (!dbconnectstring) {
				dbconnectstring=
					"db=testdsn;"
					"user=testuser;"
					"password=testpassword;";
			}
		}

		// init bench
		stringbuffer	modulename;
		modulename.append(".libs/sqlrbench_");
		modulename.append((sqlrelay)?"sqlrelay":db)->append(".so");
		if (!dl->open(modulename.getString(),true,true)) {
			stdoutput.printf("failed to load "
					"bench module: %s\n",
					modulename.getString());
			char	*error=dl->getError();
			stdoutput.printf("%s\n",error);
			delete[] error;
			continue;
		}
		stringbuffer	functionname;
		functionname.append("new_")->append((sqlrelay)?"sqlrelay":db);
		functionname.append("bench");
		sqlrbench	*(*newBm)(const char *,
						const char *,
						uint64_t,
						uint64_t,
						uint32_t,
						uint32_t,
						uint16_t,
						uint64_t,
						bool)=
			(sqlrbench	*(*)(const char *,
						const char *,
						uint64_t,
						uint64_t,
						uint32_t,
						uint32_t,
						uint16_t,
						uint64_t,
						bool))
			dl->getSymbol(functionname.getString());
		if (!newBm) {
			stdoutput.printf("failed to load bench\n");
			char	*error=dl->getError();
			stdoutput.printf("%s\n",error);
			delete[] error;
			continue;
		}

		const char	*cstring=sqlrconnectstring;
		if (proxy) {
			cstring=proxyconnectstring;
		} else if (direct) {
			cstring=dbconnectstring;
		}
		bm=newBm(cstring,db,
				queries,rows,cols,colsize,
				samples,rsbs,debug);
		if (!bm) {
			stdoutput.printf("error creating bench\n");
			continue;
		}

		// run the benchmarks
		stop=!bm->run((selectqueries)?&selectstats:NULL,
				(dmlqueries)?&dmlstats:NULL,nosettle);

		delete bm;
		bm=NULL;
	}

	// graph stats
	if (!stop) {
		graphStats(selectsgraph,db,&selectstats);
		graphStats(dmlgraph,db,&dmlstats);
	}

	// exit
	process::exit(0);
}

void shutDown(int32_t signum) {
	stdoutput.printf("shutting down, please wait...\n");
	if (bm) {
		bm->shutDown();
	}
	stop=true;
}

void graphStats(const char *graph, const char *db,
		dictionary< float, linkedlist< float > *> *stats) {

	stdoutput.printf("\ngraphing stats to %s...\n",graph);

	if (charstring::isNullOrEmpty(graph) || !stats->getCount()) {
		stdoutput.printf("no stats to graph\n");
		return;
	}

	// rename db
	if (!charstring::compare(db,"db2")) {
		db="IBM DB2";
	} else if (!charstring::compare(db,"informix")) {
		db="Informix";
	} else if (!charstring::compare(db,"firebird")) {
		db="Firebird";
	} else if (!charstring::compare(db,"freetds")) {
		db="FreeTDS";
	} else if (!charstring::compare(db,"mysql")) {
		db="MySQL";
	} else if (!charstring::compare(db,"oracle") ||
			!charstring::compare(db,"oracle8")) {
		db="Oracle";
	} else if (!charstring::compare(db,"postgresql")) {
		db="PostgreSQL";
	} else if (!charstring::compare(db,"sqlite")) {
		db="SQLite";
	} else if (!charstring::compare(db,"sap") ||
			!charstring::compare(db,"sybase")) {
		db="SAP/Sybase";
	} else if (!charstring::compare(db,"odbc")) {
		db="ODBC";
	}

	// write out the stats to temp.csv
	file	f;
	f.open("temp.csv",O_WRONLY|O_TRUNC|O_CREAT,
			permissions::parsePermString("rw-r--r--"));

	uint32_t	count=0;
	for (listnode< float > *node=stats->getKeys()->getFirst();
						node; node=node->getNext()) {
		f.printf("%f",node->getValue());
		linkedlist< float >	*l=stats->getValue(node->getValue());
		count=l->getCount();
		for (listnode< float > *lnode=l->getFirst();
						lnode; lnode=lnode->getNext()) {
			f.printf(",%f",lnode->getValue());
		}
		f.printf("\n");
	}

	// use gnuplot to create temp.png
	stringbuffer	gnuplot;
	gnuplot.append("plot")->append(count)->append(".gnu");
	stringbuffer	dbvar;
	dbvar.append("db='")->append(db)->append("'");
	const char	*args[]={
		"gnuplot","-e",dbvar.getString(),gnuplot.getString(),NULL
	};
	pid_t	pid=process::spawn("gnuplot",args,false);
	process::wait(pid);

	// move temp.png to the specified graph file name
	file::rename("temp.png",graph);

	// move temp.csv to a similar file name as the graph
	stringbuffer	tempcsv;
	char	*base=file::getBaseName(graph,".png");
	tempcsv.append(base)->append(".csv");
	delete[] base;
	file::rename("temp.csv",tempcsv.getString());

	stdoutput.printf("done\n");
}

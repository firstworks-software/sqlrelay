top_builddir = ../..

include ../../config.mk

all: db2.class \
	firebird.class \
	freetds_mssql.class \
	freetds_sap.class \
	informix.class \
	mysql.class \
	odbc_mssql.class \
	oracle.class \
	postgresql.class \
	sap.class \
	sqlite.class \
	run.bat

clean:
	$(RM) *.class cachefile* sqlnet.log run.bat

db2.class: sqlrtest.java db2.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java db2.java

firebird.class: sqlrtest.java firebird.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java firebird.java

freetds_mssql.class: sqlrtest.java freetds-mssql.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java freetds-mssql.java

freetds_sap.class: sqlrtest.java freetds-sap.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java freetds-sap.java

informix.class: sqlrtest.java informix.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java informix.java

mysql.class: sqlrtest.java mysql.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java mysql.java

odbc_mssql.class: sqlrtest.java odbc-mssql.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java odbc-mssql.java

oracle.class: sqlrtest.java oracle.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java oracle.java

postgresql.class: sqlrtest.java postgresql.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java postgresql.java

sap.class: sqlrtest.java sap.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java sap.java

sqlite.class: sqlrtest.java sqlite.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar$(CLASSPATHSEP)$(javadir)/sqlrelayjdbc.jar sqlrtest.java sqlite.java

run.bat: run.batch
	$(CP) run.batch run.bat

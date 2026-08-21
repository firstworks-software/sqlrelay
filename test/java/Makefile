top_builddir = ../..

include ../../config.mk

all: db2.class\
	freetds_mssql.class \
	freetds_sap.class \
	firebird.class \
	informix.class \
	mysql.class \
	oracle.class \
	postgresql.class \
	sqlite.class \
	sap.class \
	krb.class \
	tls.class \
	run.bat

clean:
	$(RM) *.class cachefile* sqlnet.log run.bat

db2.class: sqlrtest.java db2.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java db2.java

freetds_mssql.class: sqlrtest.java freetds-mssql.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java freetds-mssql.java

freetds_sap.class: sqlrtest.java freetds-sap.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java freetds-sap.java

firebird.class: sqlrtest.java firebird.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java firebird.java

informix.class: sqlrtest.java informix.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java informix.java

mysql.class: sqlrtest.java mysql.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java mysql.java

oracle.class: sqlrtest.java oracle.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java oracle.java

postgresql.class: sqlrtest.java postgresql.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java postgresql.java

sqlite.class: sqlrtest.java sqlite.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java sqlite.java

sap.class: sqlrtest.java sap.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java sap.java

tls.class: sqlrtest.java tls.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java tls.java

krb.class: sqlrtest.java krb.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar sqlrtest.java krb.java

run.bat: run.batch
	$(CP) run.batch run.bat

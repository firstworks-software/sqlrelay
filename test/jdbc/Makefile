top_builddir = ../..

include ../../config.mk

all: oracle.class \
	run.bat

clean:
	$(RM) *.class cachefile* sqlnet.log run.bat

oracle.class: sqlrtest.java oracle.java
	$(JAVAC) -classpath $(javadir)/sqlrelay.jar:$(javadir)/sqlrelayjdbc.jar sqlrtest.java oracle.java

run.bat: run.batch
	$(CP) run.batch run.bat

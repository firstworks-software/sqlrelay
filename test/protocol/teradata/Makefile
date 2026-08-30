top_builddir = ../../..

include ../../../config.mk

# port of the teradataprotocol listener that odbc.ini's [sqlr-teradata] DSN
# points at.  it defaults to what
# test/sqlrelay.conf.d/teradataprotocol.conf.in's @TERADATAPROTOCOLPORT1@
# defaults to, so a plain make writes the same odbc.ini as before.  set it in
# the environment, or on the make command line, to point the suite at an
# instance on another port:
#
#	make TERADATAPROTOCOLPORT1=11025 odbc.ini
#
TERADATAPROTOCOLPORT1 ?= 1025

all: teradata.class odbc.ini

odbc.ini: odbc.ini.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TERADATAPROTOCOLPORT1@ "$(TERADATAPROTOCOLPORT1)" $@.out1 > $@.out2
	$(MV) $@.out2 $@
	$(RM) $@.out1

teradata.class: teradata.java
	$(JAVAC) $<

clean:
	$(RM) *.lo *.o *.obj *.class *.out1 *.out2 odbc.ini
	$(RMTREE) .libs

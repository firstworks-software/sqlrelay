top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .lo .obj .cpp

# ports of the oracleprotocol listeners that tnsnames.ora's sqlrelay* aliases
# point at.  they default to what
# test/sqlrelay.conf.d/oracleprotocol.conf.in's @ORACLEPROTOCOLPORTn@ tokens
# default to, so a plain make writes the same tnsnames.ora as before.  set
# them in the environment, or on the make command line, to point the suite at
# an instance on other ports:
#
#	make ORACLEPROTOCOLPORT1=11521 tnsnames.ora
#
ORACLEPROTOCOLPORT1 ?= 1521
ORACLEPROTOCOLPORT2 ?= 1522
ORACLEPROTOCOLPORT3 ?= 1523
ORACLEPROTOCOLPORT4 ?= 1524
ORACLEPROTOCOLPORT5 ?= 1525

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACLEINCLUDES) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACLEINCLUDES) $(COMPILE) $<

# oracle.class only builds if a jdk is configured - see
# ORACLEPROTOCOLTESTTARGETS in config.mk.in
#
# oracledescribeonly, oraclelegacyfetch, oracleclrchunks and
# oracleiterations0 only need rudiments (raw sockets), so they build whether
# or not OCI is configured - they speak the oracle wire protocol themselves
# rather than going through a client library
all: oracle ociselect oracledescribeonly oraclelegacyfetch oracleclrchunks oracleiterations0 $(ORACLEPROTOCOLTESTTARGETS) tnsnames.ora

tnsnames.ora: tnsnames.ora.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT1@ "$(ORACLEPROTOCOLPORT1)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT2@ "$(ORACLEPROTOCOLPORT2)" $@.out2 > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT3@ "$(ORACLEPROTOCOLPORT3)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT4@ "$(ORACLEPROTOCOLPORT4)" $@.out2 > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT5@ "$(ORACLEPROTOCOLPORT5)" $@.out1 > $@.out2
	$(MV) $@.out2 $@
	$(RM) $@.out1

oracle.class: oracle.java
	$(JAVAC) $<

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracle.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

ociselect: ociselect.cpp ociselect.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) ociselect.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oracledescribeonly.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oracledescribeonly.$(OBJ): oracleprotocolclient.cpp

oracledescribeonly: oracledescribeonly.cpp oracleprotocolclient.cpp oracledescribeonly.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracledescribeonly.$(OBJ) $(RUDIMENTSLIBS)

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oraclelegacyfetch.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oraclelegacyfetch.$(OBJ): oracleprotocolclient.cpp

oraclelegacyfetch: oraclelegacyfetch.cpp oracleprotocolclient.cpp oraclelegacyfetch.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oraclelegacyfetch.$(OBJ) $(RUDIMENTSLIBS)

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oracleclrchunks.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oracleclrchunks.$(OBJ): oracleprotocolclient.cpp

oracleclrchunks: oracleclrchunks.cpp oracleprotocolclient.cpp oracleclrchunks.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracleclrchunks.$(OBJ) $(RUDIMENTSLIBS)

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oracleiterations0.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oracleiterations0.$(OBJ): oracleprotocolclient.cpp

oracleiterations0: oracleiterations0.cpp oracleprotocolclient.cpp oracleiterations0.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracleiterations0.$(OBJ) $(RUDIMENTSLIBS)

clean:
	$(RM) *.lo *.o *.obj *.class *.out1 *.out2 oracle$(EXE) ociselect$(EXE) oracledescribeonly$(EXE) oraclelegacyfetch$(EXE) oracleclrchunks$(EXE) oracleiterations0$(EXE) tnsnames.ora
	$(RMTREE) .libs

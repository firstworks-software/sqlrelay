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
ORACLEPROTOCOLPORT7 ?= 1527

# OCI7INCLUDES is for oci7.cpp specifically - it's blank unless the
# legacy OCI7 call interface was detected at configure time, so this
# is a no-op on everything else in this directory
.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACLEINCLUDES) $(OCI7INCLUDES) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACLEINCLUDES) $(OCI7INCLUDES) $(COMPILE) $<

# oracle.class only builds if a jdk is configured - see
# ORACLEPROTOCOLTESTTARGETS in config.mk.in
#
# oci7, oci7describe and oci7bind only build if the legacy OCI7 call
# interface was detected at configure time - also gated through
# ORACLEPROTOCOLTESTTARGETS in config.mk.in, but via HAVE_OCI7 rather
# than HAVE_JAVA
#
# oracledescribeonly, oracledescribe, oraclemidfetchdescribe,
# oraclelegacyfetch, oracleclrchunks, oracleiterations0,
# oraclebigchunkclroff, oraclerefcursorreuse and oraclesplitrequest only
# need rudiments (raw sockets), so they build whether or not OCI is
# configured - they speak the oracle wire protocol themselves rather than
# going through a client library
all: oci8 ociselect oracledescribeonly oracledescribe oraclemidfetchdescribe oraclelegacyfetch oracleclrchunks oracleiterations0 oraclebigchunkclroff oraclerefcursorreuse oraclesplitrequest $(ORACLEPROTOCOLTESTTARGETS) tnsnames.ora

tnsnames.ora: tnsnames.ora.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT1@ "$(ORACLEPROTOCOLPORT1)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT2@ "$(ORACLEPROTOCOLPORT2)" $@.out2 > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT3@ "$(ORACLEPROTOCOLPORT3)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT4@ "$(ORACLEPROTOCOLPORT4)" $@.out2 > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT5@ "$(ORACLEPROTOCOLPORT5)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT7@ "$(ORACLEPROTOCOLPORT7)" $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2

oracle.class: oracle.java
	$(JAVAC) $<

oci8: oci8.cpp oci8.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oci8.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

oci7: oci7.cpp oci7.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oci7.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

oci7describe: oci7describe.cpp oci7describe.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oci7describe.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

oci7bind: oci7bind.cpp oci7bind.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oci7bind.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

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
# oracledescribe.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oracledescribe.$(OBJ): oracleprotocolclient.cpp

oracledescribe: oracledescribe.cpp oracleprotocolclient.cpp oracledescribe.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracledescribe.$(OBJ) $(RUDIMENTSLIBS)

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oraclemidfetchdescribe.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oraclemidfetchdescribe.$(OBJ): oracleprotocolclient.cpp

oraclemidfetchdescribe: oraclemidfetchdescribe.cpp oracleprotocolclient.cpp oraclemidfetchdescribe.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oraclemidfetchdescribe.$(OBJ) $(RUDIMENTSLIBS)

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

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oraclebigchunkclroff.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oraclebigchunkclroff.$(OBJ): oracleprotocolclient.cpp

oraclebigchunkclroff: oraclebigchunkclroff.cpp oracleprotocolclient.cpp oraclebigchunkclroff.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oraclebigchunkclroff.$(OBJ) $(RUDIMENTSLIBS)

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oraclerefcursorreuse.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oraclerefcursorreuse.$(OBJ): oracleprotocolclient.cpp

oraclerefcursorreuse: oraclerefcursorreuse.cpp oracleprotocolclient.cpp oraclerefcursorreuse.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oraclerefcursorreuse.$(OBJ) $(RUDIMENTSLIBS)

# oracleprotocolclient.cpp isn't compiled on its own - it's #include'd by
# the test's main, the same way asserts.cpp is.  the explicit dependency
# below is needed because the .cpp.$(OBJ) suffix rule only depends on
# oraclesplitrequest.cpp - without it, editing oracleprotocolclient.cpp
# alone would relink but not recompile, leaving a stale object (see
# AGENTS.md's note on this build tracking no header dependencies)
oraclesplitrequest.$(OBJ): oracleprotocolclient.cpp

oraclesplitrequest: oraclesplitrequest.cpp oracleprotocolclient.cpp oraclesplitrequest.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oraclesplitrequest.$(OBJ) $(RUDIMENTSLIBS)

clean:
	$(RM) *.lo *.o *.obj *.class *.out1 *.out2 oci8$(EXE) oci7$(EXE) oci7describe$(EXE) oci7bind$(EXE) ociselect$(EXE) oracledescribeonly$(EXE) oracledescribe$(EXE) oraclemidfetchdescribe$(EXE) oraclelegacyfetch$(EXE) oracleclrchunks$(EXE) oracleiterations0$(EXE) oraclebigchunkclroff$(EXE) oraclerefcursorreuse$(EXE) oraclesplitrequest$(EXE) tnsnames.ora
	$(RMTREE) .libs

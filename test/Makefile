top_builddir = ..

include ../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) $<

all: $(TESTALLSUBDIRS) $(TESTSCRIPTS) freeport$(EXE)
	$(MKINSTALLDIRS) log/extensions

# test.sh -randomports shells out to this to pick listener ports.  it only
# needs rudiments, so it builds everywhere, regardless of which databases and
# apis are configured
freeport: freeport.cpp freeport.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) freeport.$(OBJ) $(RUDIMENTSLIBS)

all-c:
	cd c $(AND) $(MAKE) all

all-cpp:
	cd c++ $(AND) $(MAKE) all

all-extensions:
	cd extensions $(AND) $(MAKE) all

all-cs:
	cd cs $(AND) $(MAKE) all

all-adonet:
	cd ado.net $(AND) $(MAKE) all

all-java:
	cd java $(AND) $(MAKE) all

all-jdbc:
	cd jdbc $(AND) $(MAKE) all

all-odbc:
	cd odbc $(AND) $(MAKE) all

all-tcl:
	cd tcl $(AND) $(MAKE) all

all-erlang:
	cd erlang $(AND) $(MAKE) all

all-legacy:
	cd legacy $(AND) $(MAKE) all

all-protocol:
	cd protocol $(AND) $(MAKE) all

all-crud:
	cd crud $(AND) $(MAKE) all

all-stress:
	cd stress $(AND) $(MAKE) all

clean:
	cd ado.net $(AND) $(MAKE) clean
	cd c $(AND) $(MAKE) clean
	cd c++ $(AND) $(MAKE) clean
	cd cs $(AND) $(MAKE) clean
	cd erlang $(AND) $(MAKE) clean
	cd extensions $(AND) $(MAKE) clean
	cd java $(AND) $(MAKE) clean
	cd jdbc $(AND) $(MAKE) clean
	cd nodejs $(AND) $(MAKE) clean
	cd odbc $(AND) $(MAKE) clean
	cd perl $(AND) $(MAKE) clean
	cd php $(AND) $(MAKE) clean
	cd python $(AND) $(MAKE) clean
	cd ruby $(AND) $(MAKE) clean
	cd tcl $(AND) $(MAKE) clean
	cd bench $(AND) $(MAKE) clean
	cd crud $(AND) $(MAKE) clean
	cd legacy $(AND) $(MAKE) clean
	cd protocol $(AND) $(MAKE) clean
	cd stress $(AND) $(MAKE) clean
	$(RM) sqlr-*.*.bt sybinit.err sqlrelay.conf.d/sqlite/sqlite.db temp1 temp2 temp3 testexitstatus-*.tmp $(TESTSCRIPTS) *.out1 *.out2 *.lo *.o *.obj freeport$(EXE)
	$(RMTREE) log .libs stagedmodules

tests: all
	$(SCRIPTINT) $(THISDIR)test$(SCRIPTEXT)

# freeport rides along here, rather than only in all, so that a plain
# top-level make leaves test.sh -randomports usable.  it only needs
# rudiments, so it builds whatever else is or isn't configured
testscripts: $(TESTSCRIPTS) freeport$(EXE)

test.sh: test.sh.in
	$(REPLACE) @abs_top_builddir@ "$(abs_top_builddir)" $@.in > $@.out1
	$(REPLACE) @prefix@ "$(prefix)" $@.out1 > $@.out2
	$(REPLACE) @PYTHON@ "$(PYTHON)" $@.out2 > $@.out1
	$(REPLACE) @TESTINSTANCES@ "$(TESTINSTANCES)" $@.out1 > $@.out2
	$(REPLACE) @TESTAPIS@ "$(TESTAPIS)" $@.out2 > $@.out1
	$(REPLACE) @HOSTNAME@ "$(HOSTNAME)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT1@ "$(ORACLEPROTOCOLPORT1)" $@.out2 > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT2@ "$(ORACLEPROTOCOLPORT2)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT3@ "$(ORACLEPROTOCOLPORT3)" $@.out2 > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT4@ "$(ORACLEPROTOCOLPORT4)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT5@ "$(ORACLEPROTOCOLPORT5)" $@.out2 > $@.out1
	$(REPLACE) @FIREBIRDPROTOCOLPORT1@ "$(FIREBIRDPROTOCOLPORT1)" $@.out1 > $@.out2
	$(REPLACE) @FIREBIRDPROTOCOLPORT2@ "$(FIREBIRDPROTOCOLPORT2)" $@.out2 > $@.out1
	$(REPLACE) @FIREBIRDPROTOCOLPORT3@ "$(FIREBIRDPROTOCOLPORT3)" $@.out1 > $@.out2
	$(REPLACE) @MYSQLPROTOCOLPORT1@ "$(MYSQLPROTOCOLPORT1)" $@.out2 > $@.out1
	$(REPLACE) @MYSQLPROTOCOLSOCKET@ "$(MYSQLPROTOCOLSOCKET)" $@.out1 > $@.out2
	$(REPLACE) @POSTGRESQLPROTOCOLPORT1@ "$(POSTGRESQLPROTOCOLPORT1)" $@.out2 > $@.out1
	$(REPLACE) @POSTGRESQLPROTOCOLPORT2@ "$(POSTGRESQLPROTOCOLPORT2)" $@.out1 > $@.out2
	$(REPLACE) @TDSPROTOCOLPORT1@ "$(TDSPROTOCOLPORT1)" $@.out2 > $@.out1
	$(REPLACE) @TDSSAPPROTOCOLPORT1@ "$(TDSSAPPROTOCOLPORT1)" $@.out1 > $@.out2
	$(REPLACE) @TDSSAPPROTOCOLPORT2@ "$(TDSSAPPROTOCOLPORT2)" $@.out2 > $@.out1
	$(REPLACE) @TERADATAPROTOCOLPORT1@ "$(TERADATAPROTOCOLPORT1)" $@.out1 > $@.out2
	$(REPLACE) @MYSQLPROTOCOLTLSPORT1@ "$(MYSQLPROTOCOLTLSPORT1)" $@.out2 > $@.out1
	$(REPLACE) @MYSQLPROTOCOLTLSSERVERAUTHPORT1@ "$(MYSQLPROTOCOLTLSSERVERAUTHPORT1)" $@.out1 > $@.out2
	$(REPLACE) @MYSQLPROTOCOLTLSMUTUALAUTHPORT1@ "$(MYSQLPROTOCOLTLSMUTUALAUTHPORT1)" $@.out2 > $@.out1
	$(REPLACE) @POSTGRESQLPROTOCOLTLSPORT1@ "$(POSTGRESQLPROTOCOLTLSPORT1)" $@.out1 > $@.out2
	$(REPLACE) @POSTGRESQLPROTOCOLTLSSERVERAUTHPORT1@ "$(POSTGRESQLPROTOCOLTLSSERVERAUTHPORT1)" $@.out2 > $@.out1
	$(REPLACE) @POSTGRESQLPROTOCOLTLSMUTUALAUTHPORT1@ "$(POSTGRESQLPROTOCOLTLSMUTUALAUTHPORT1)" $@.out1 > $@.out2
	$(REPLACE) @TDSPROTOCOLTLSPORT1@ "$(TDSPROTOCOLTLSPORT1)" $@.out2 > $@.out1
	$(REPLACE) @TDSPROTOCOLTLSSERVERAUTHPORT1@ "$(TDSPROTOCOLTLSSERVERAUTHPORT1)" $@.out1 > $@.out2
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

test.vbs: test.vbs.in
	$(REPLACE) @prefix@ $(prefix) $@.in > $@.out1
	$(REPLACE) @TESTINSTANCES@ "$(TESTINSTANCES)" $@.out1 > $@.out2
	$(REPLACE) @TESTAPIS@ "$(TESTAPIS)" $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2

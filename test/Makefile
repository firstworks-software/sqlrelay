top_builddir = ..

include ../config.mk

all: $(TESTALLSUBDIRS) $(TESTSCRIPTS) 

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
	$(RM) sqlr-*.*.bt sybinit.err sqlrelay.conf.d/sqlite/sqlite.db temp1 temp2 temp3 testexitstatus.tmp $(TESTSCRIPTS) *.out1 *.out2

tests: all
	$(SCRIPTINT) $(THISDIR)test$(SCRIPTEXT)

testscripts: $(TESTSCRIPTS)

test.sh: test.sh.in
	$(REPLACE) @abs_top_builddir@ "$(abs_top_builddir)" $@.in > $@.out1
	$(REPLACE) @prefix@ "$(prefix)" $@.out1 > $@.out2
	$(REPLACE) @PYTHON@ "$(PYTHON)" $@.out2 > $@.out1
	$(REPLACE) @TESTDBS@ "$(TESTDBS)" $@.out1 > $@.out2
	$(REPLACE) @TESTAPIS@ "$(TESTAPIS)" $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

# pass $(prefix) unquoted - it already carries its own quotes (config_windows.mk),
# and doubling them makes cscript split on the space in the path (#8627);
# TESTDBS/TESTAPIS are space-separated, so they pass through fine
test.vbs: test.vbs.in
	$(REPLACE) @prefix@ $(prefix) $@.in > $@.out1
	$(REPLACE) @TESTDBS@ "$(TESTDBS)" $@.out1 > $@.out2
	$(REPLACE) @TESTAPIS@ "$(TESTAPIS)" $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2

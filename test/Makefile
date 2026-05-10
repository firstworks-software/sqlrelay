top_builddir = ..

include ../config.mk

all: $(TESTALLSUBDIRS) 

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
	$(RM) sqlr-*.*.bt sybinit.err sqlrelay.conf.d/sqlite/sqlite.db temp1 temp2 temp3

tests: all
	$(SCRIPTINT) $(THISDIR)testall$(SCRIPTEXT)

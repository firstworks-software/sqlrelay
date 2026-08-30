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

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACLEINCLUDES) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACEINCLUDES) $(COMPILE) $<

all: oracle ociselect oracle.class tnsnames.ora

tnsnames.ora: tnsnames.ora.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT1@ "$(ORACLEPROTOCOLPORT1)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT2@ "$(ORACLEPROTOCOLPORT2)" $@.out2 > $@.out1
	$(REPLACE) @ORACLEPROTOCOLPORT3@ "$(ORACLEPROTOCOLPORT3)" $@.out1 > $@.out2
	$(REPLACE) @ORACLEPROTOCOLPORT4@ "$(ORACLEPROTOCOLPORT4)" $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2

oracle.class: oracle.java
	$(JAVAC) $<

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracle.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

ociselect: ociselect.cpp ociselect.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) ociselect.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

clean:
	$(RM) *.lo *.o *.obj *.class *.out1 *.out2 oracle$(EXE) ociselect$(EXE) tnsnames.ora
	$(RMTREE) .libs

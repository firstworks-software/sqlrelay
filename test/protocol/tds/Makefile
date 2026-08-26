top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .lo .obj .cpp

# tdsdialectguard and tdsbyteorder only need rudiments (raw sockets), so
# they build regardless of whether SAP ct-lib and/or FreeTDS are configured
# - unlike tds-freetds/tds-sap, they aren't gated behind
# $(TDSTESTALLTARGETS)
all: $(TDSTESTALLTARGETS) tdsdialectguard tdsbyteorder

tds-freetds.lo: tds.cpp
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(TDSTESTFREETDSCPPFLAGS) $(COMPILE) tds.cpp $(OUT)$@

tds-freetds.obj: tds.cpp
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(TDSTESTFREETDSCPPFLAGS) $(COMPILE) tds.cpp $(OUT)$@

tds-sap.lo: tds.cpp
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(TDSTESTSAPCPPFLAGS) $(COMPILE) tds.cpp $(OUT)$@

tds-sap.obj: tds.cpp
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(TDSTESTSAPCPPFLAGS) $(COMPILE) tds.cpp $(OUT)$@

tds-freetds: tds.cpp tds-freetds.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) tds-freetds.$(OBJ) $(TDSTESTFREETDSLIBS)

tds-sap: tds.cpp tds-sap.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) tds-sap.$(OBJ) $(TDSTESTSAPLIBS)

tdsdialectguard.lo: tdsdialectguard.cpp
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) tdsdialectguard.cpp $(OUT)$@

tdsdialectguard.obj: tdsdialectguard.cpp
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) tdsdialectguard.cpp $(OUT)$@

tdsdialectguard: tdsdialectguard.cpp tdsdialectguard.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) tdsdialectguard.$(OBJ) $(RUDIMENTSLIBS)

tdsbyteorder.lo: tdsbyteorder.cpp
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) tdsbyteorder.cpp $(OUT)$@

tdsbyteorder.obj: tdsbyteorder.cpp
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) tdsbyteorder.cpp $(OUT)$@

tdsbyteorder: tdsbyteorder.cpp tdsbyteorder.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) tdsbyteorder.$(OBJ) $(RUDIMENTSLIBS)

clean:
	$(RM) *.lo *.o *.obj *.class tds$(EXE) tds-freetds$(EXE) tds-sap$(EXE) tdsdialectguard$(EXE) tdsbyteorder$(EXE)
	$(RMTREE) .libs

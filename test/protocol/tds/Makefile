top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .lo .obj .cpp

all: $(TDSTESTALLTARGETS)

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

clean:
	$(RM) *.lo *.o *.obj *.class tds$(EXE) tds-freetds$(EXE) tds-sap$(EXE)
	$(RMTREE) .libs

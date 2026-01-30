top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $<

all: mssql oracle putdata

clean:
	$(RM) *.lo *.o *.obj mssql$(EXE) oracle$(EXE) putdata$(EXE) cachefile* sqlnet.log
	$(RMTREE) .libs

mssql: mssql.cpp mssql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ mssql.$(OBJ) $(ODBCTESTLIBS)

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ oracle.$(OBJ) $(ODBCTESTLIBS)

putdata: putdata.cpp putdata.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ putdata.$(OBJ) $(ODBCTESTLIBS)

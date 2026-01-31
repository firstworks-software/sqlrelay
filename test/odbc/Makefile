top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $<

all: oracle mysql mssql putdata

clean:
	$(RM) *.lo *.o *.obj oracle$(EXE) mssql$(EXE) mysql$(EXE) putdata$(EXE) cachefile* sqlnet.log
	$(RMTREE) .libs

mysql: mysql.cpp mysql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ mysql.$(OBJ) $(ODBCTESTLIBS)

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ oracle.$(OBJ) $(ODBCTESTLIBS)

mssql: mssql.cpp mssql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ mssql.$(OBJ) $(ODBCTESTLIBS)

putdata: putdata.cpp putdata.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ putdata.$(OBJ) $(ODBCTESTLIBS)

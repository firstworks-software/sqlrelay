top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(ODBCTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(ODBCTESTCPPFLAGS) $(COMPILE) $<

all: oracle mysql postgresql db2 sap freetds-sap freetds-mssql informix firebird sqlite

clean:
	$(RM) *.lo *.o *.obj oracle$(EXE) mysql$(EXE) postgresql$(EXE) db2$(EXE) sap$(EXE) freetds-sap$(EXE) freetds-mssql$(EXE) informix$(EXE) firebird$(EXE) sqlite$(EXE) cachefile* sqlnet.log
	$(RMTREE) .libs

mysql: mysql.cpp mysql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ mysql.$(OBJ) $(ODBCTESTLIBS)

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ oracle.$(OBJ) $(ODBCTESTLIBS)

postgresql: postgresql.cpp postgresql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ postgresql.$(OBJ) $(ODBCTESTLIBS)

db2: db2.cpp db2.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ db2.$(OBJ) $(ODBCTESTLIBS)

sap: sap.cpp sap.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ sap.$(OBJ) $(ODBCTESTLIBS)

freetds-sap: freetds-sap.cpp freetds-sap.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ freetds-sap.$(OBJ) $(ODBCTESTLIBS)

freetds-mssql: freetds-mssql.cpp freetds-mssql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ freetds-mssql.$(OBJ) $(ODBCTESTLIBS)

informix: informix.cpp informix.$(OBJ)
	$(LTLINK) $(LINK) -export-dynamic $(LDFLAGS) $(OUT)$@ informix.$(OBJ) $(ODBCTESTLIBS) $(INFORMIXLIBS)

firebird: firebird.cpp firebird.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ firebird.$(OBJ) $(ODBCTESTLIBS)

sqlite: sqlite.cpp sqlite.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ sqlite.$(OBJ) $(ODBCTESTLIBS)

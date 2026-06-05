top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $<

all: oracle mysql postgresql db2 sap freetds informix

clean:
	$(RM) *.lo *.o *.obj oracle$(EXE) mysql$(EXE) postgresql$(EXE) db2$(EXE) sap$(EXE) freetds$(EXE) informix$(EXE) cachefile* sqlnet.log
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

freetds: freetds.cpp freetds.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ freetds.$(OBJ) $(ODBCTESTLIBS)

informix: informix.cpp informix.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ informix.$(OBJ) $(ODBCTESTLIBS)

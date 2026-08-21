top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) $<

all: db2 \
	freetds-mssql \
	odbc-mssql \
	freetds-sap \
	firebird \
	informix \
	mysql \
	oracle \
	postgresql \
	sqlite \
	sap \
	krb \
	tls

clean:
	$(RM) *.lo *.o *.obj db2$(EXE) freetds-mssql$(EXE) odbc-mssql$(EXE) freetds-sap$(EXE) firebird$(EXE) informix$(EXE) mysql$(EXE) oracle$(EXE) postgresql$(EXE) sqlite$(EXE) sap$(EXE) krb$(EXE) tls$(EXE) *.csv cachefile* sqlnet.log
	$(RMTREE) .libs

db2: db2.cpp db2.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) db2.$(OBJ) $(CPPTESTLIBS)

freetds-mssql: freetds-mssql.cpp freetds-mssql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) freetds-mssql.$(OBJ) $(CPPTESTLIBS)

odbc-mssql: odbc-mssql.cpp odbc-mssql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) odbc-mssql.$(OBJ) $(CPPTESTLIBS)

freetds-sap: freetds-sap.cpp freetds-sap.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) freetds-sap.$(OBJ) $(CPPTESTLIBS)

firebird: firebird.cpp firebird.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) firebird.$(OBJ) $(CPPTESTLIBS)

informix: informix.cpp informix.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) informix.$(OBJ) $(CPPTESTLIBS)

mysql: mysql.cpp mysql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysql.$(OBJ) $(CPPTESTLIBS)

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracle.$(OBJ) $(CPPTESTLIBS)

postgresql: postgresql.cpp postgresql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) postgresql.$(OBJ) $(CPPTESTLIBS)

sqlite: sqlite.cpp sqlite.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) sqlite.$(OBJ) $(CPPTESTLIBS)

sap: sap.cpp sap.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) sap.$(OBJ) $(CPPTESTLIBS)

krb: krb.cpp krb.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) krb.$(OBJ) $(CPPTESTLIBS)

tls: tls.cpp tls.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) tls.$(OBJ) $(CPPTESTLIBS)

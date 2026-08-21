top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj

.c.lo:
	$(LTCOMPILE) $(CC) $(CFLAGS) $(CTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.c.obj:
	$(CC) $(CFLAGS) $(CTESTCPPFLAGS) $(COMPILE) $<

all: db2 freetds-mssql freetds-sap firebird informix mysql oracle postgresql sqlite sap krb tls

clean:
	$(RM) *.lo *.o *.obj db2$(EXE) freetds-mssql$(EXE) freetds-sap$(EXE) firebird$(EXE) informix$(EXE) mysql$(EXE) oracle$(EXE) postgresql$(EXE) sqlite$(EXE) sap$(EXE) krb$(EXE) tls$(EXE) cachefile* sqlnet.log
	$(RMTREE) .libs

db2: db2.c db2.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) db2.$(OBJ) $(CTESTLIBS)

freetds-mssql: freetds-mssql.c freetds-mssql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) freetds-mssql.$(OBJ) $(CTESTLIBS)

freetds-sap: freetds-sap.c freetds-sap.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) freetds-sap.$(OBJ) $(CTESTLIBS)

firebird: firebird.c firebird.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) firebird.$(OBJ) $(CTESTLIBS)

informix: informix.c informix.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) informix.$(OBJ) $(CTESTLIBS)

mysql: mysql.c mysql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysql.$(OBJ) $(CTESTLIBS)

oracle: oracle.c oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracle.$(OBJ) $(CTESTLIBS)

postgresql: postgresql.c postgresql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) postgresql.$(OBJ) $(CTESTLIBS)

sqlite: sqlite.c sqlite.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) sqlite.$(OBJ) $(CTESTLIBS)

sap: sap.c sap.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) sap.$(OBJ) $(CTESTLIBS)

krb: krb.c krb.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) krb.$(OBJ) $(CTESTLIBS)

tls: tls.c tls.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) tls.$(OBJ) $(CTESTLIBS)

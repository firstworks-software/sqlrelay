top_builddir = ../..

include ../../config.mk

all: local \
	db2.exe \
	firebird.exe \
	freetds-sap.exe \
	informix.exe \
	krb.exe \
	mysql.exe \
	oracle.exe \
	postgresql.exe \
	sap.exe \
	sqlite.exe \
	tls.exe

clean:
	$(RM) *.exe *.dll *.dll.config *.pdb cachefile*

local:
	$(CP) ../../src/api/cs/SQLRClient.dll .
	$(CP) ../../src/api/cs/SQLRClient.dll.config .

db2.exe: sqlrtest.cs db2.cs
	$(CSC) $(CSCFLAGS) /out:db2.exe sqlrtest.cs db2.cs /reference:SQLRClient.dll

firebird.exe: sqlrtest.cs firebird.cs
	$(CSC) $(CSCFLAGS) /out:firebird.exe sqlrtest.cs firebird.cs /reference:SQLRClient.dll

freetds-sap.exe: sqlrtest.cs freetds-sap.cs
	$(CSC) $(CSCFLAGS) /out:freetds-sap.exe sqlrtest.cs freetds-sap.cs /reference:SQLRClient.dll

informix.exe: sqlrtest.cs informix.cs
	$(CSC) $(CSCFLAGS) /out:informix.exe sqlrtest.cs informix.cs /reference:SQLRClient.dll

krb.exe: sqlrtest.cs krb.cs
	$(CSC) $(CSCFLAGS) /out:krb.exe sqlrtest.cs krb.cs /reference:SQLRClient.dll

mysql.exe: sqlrtest.cs mysql.cs
	$(CSC) $(CSCFLAGS) /out:mysql.exe sqlrtest.cs mysql.cs /reference:SQLRClient.dll

oracle.exe: sqlrtest.cs oracle.cs
	$(CSC) $(CSCFLAGS) /out:oracle.exe sqlrtest.cs oracle.cs /reference:SQLRClient.dll

postgresql.exe: sqlrtest.cs postgresql.cs
	$(CSC) $(CSCFLAGS) /out:postgresql.exe sqlrtest.cs postgresql.cs /reference:SQLRClient.dll

sap.exe: sqlrtest.cs sap.cs
	$(CSC) $(CSCFLAGS) /out:sap.exe sqlrtest.cs sap.cs /reference:SQLRClient.dll

sqlite.exe: sqlrtest.cs sqlite.cs
	$(CSC) $(CSCFLAGS) /out:sqlite.exe sqlrtest.cs sqlite.cs /reference:SQLRClient.dll

tls.exe: sqlrtest.cs tls.cs
	$(CSC) $(CSCFLAGS) /out:tls.exe sqlrtest.cs tls.cs /reference:SQLRClient.dll

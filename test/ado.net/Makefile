top_builddir = ../..

include ../../config.mk

all: local oracle.exe postgresql.exe krb.exe tls.exe unicode.exe

clean:
	$(RM) *.exe *.dll *.dll.config *.pdb

local:
	$(CP) ../../src/api/cs/SQLRClient.dll .
	$(CP) ../../src/api/cs/SQLRClient.dll.config .

oracle.exe: sqlrtest.cs oracle.cs
	$(CSC) $(CSCFLAGS) /out:oracle.exe sqlrtest.cs oracle.cs /reference:SQLRClient.dll

postgresql.exe: sqlrtest.cs postgresql.cs
	$(CSC) $(CSCFLAGS) /out:postgresql.exe sqlrtest.cs postgresql.cs /reference:SQLRClient.dll

krb.exe: sqlrtest.cs krb.cs
	$(CSC) $(CSCFLAGS) /out:krb.exe sqlrtest.cs krb.cs /reference:SQLRClient.dll

tls.exe: sqlrtest.cs tls.cs
	$(CSC) $(CSCFLAGS) /out:tls.exe sqlrtest.cs tls.cs /reference:SQLRClient.dll

unicode.exe: sqlrtest.cs unicode.cs
	$(CSC) $(CSCFLAGS) /out:unicode.exe sqlrtest.cs unicode.cs /reference:SQLRClient.dll

postgresqlbinds.exe: postgresqlbinds.cs
	$(CSC) $(CSCFLAGS) /out:postgresqlbinds.exe postgresqlbinds.cs /reference:Npgsql.dll

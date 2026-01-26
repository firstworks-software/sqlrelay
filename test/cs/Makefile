top_builddir = ../..

include ../../config.mk

all: local oracle krb tls unicode

clean:
	$(RM) *.exe *.dll *.dll.config *.pdb

local:
	$(CP) ../../src/api/cs/SQLRClient.dll .
	$(CP) ../../src/api/cs/SQLRClient.dll.config .

oracle: sqlrtest.cs oracle.cs
	$(CSC) $(CSCFLAGS) /out:oracle.exe sqlrtest.cs oracle.cs /reference:SQLRClient.dll

krb: sqlrtest.cs krb.cs
	$(CSC) $(CSCFLAGS) /out:krb.exe sqlrtest.cs krb.cs /reference:SQLRClient.dll

tls: sqlrtest.cs tls.cs
	$(CSC) $(CSCFLAGS) /out:tls.exe sqlrtest.cs tls.cs /reference:SQLRClient.dll

unicode: sqlrtest.cs unicode.cs
	$(CSC) $(CSCFLAGS) /out:unicode.exe sqlrtest.cs unicode.cs /reference:SQLRClient.dll

postgresqlbinds: postgresqlbinds.cs
	$(CSC) $(CSCFLAGS) /out:postgresqlbinds.exe postgresqlbinds.cs /reference:Npgsql.dll

top_builddir = ../..

include ../../config.mk

all: local oracle krb tls unicode

clean:
	$(RM) *.exe *.dll *.dll.config *.pdb

local:
	$(CP) ../../src/api/cs/SQLRClient.dll .
	$(CP) ../../src/api/cs/SQLRClient.dll.config .

oracle: sqlradaptertest.cs oracle.cs
	$(CSC) $(CSCFLAGS) /out:oracle.exe sqlradaptertest.cs oracle.cs /reference:SQLRClient.dll

krb: sqlradaptertest.cs krb.cs
	$(CSC) $(CSCFLAGS) /out:krb.exe sqlradaptertest.cs krb.cs /reference:SQLRClient.dll

tls: sqlradaptertest.cs tls.cs
	$(CSC) $(CSCFLAGS) /out:tls.exe sqlradaptertest.cs tls.cs /reference:SQLRClient.dll

unicode: sqlradaptertest.cs unicode.cs
	$(CSC) $(CSCFLAGS) /out:unicode.exe sqlradaptertest.cs unicode.cs /reference:SQLRClient.dll

postgresqlbinds: postgresqlbinds.cs
	$(CSC) $(CSCFLAGS) /out:postgresqlbinds.exe sqlradaptertest.cs postgresqlbinds.cs /reference:Npgsql.dll

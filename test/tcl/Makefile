top_builddir = ../..

include ../../config.mk

.SUFFIXES: .tcl .in

all: db2.tcl \
	firebird.tcl \
	freetds-sap.tcl \
	informix.tcl \
	mysql.tcl \
	oracle.tcl \
	postgresql.tcl \
	sqlite.tcl \
	sap.tcl \
	tls.tcl \
	krb.tcl

db2.tcl: db2.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

firebird.tcl: firebird.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

freetds-sap.tcl: freetds-sap.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

informix.tcl: informix.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

mysql.tcl: mysql.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

oracle.tcl: oracle.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

postgresql.tcl: postgresql.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

sqlite.tcl: sqlite.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

sap.tcl: sap.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

tls.tcl: tls.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

krb.tcl: krb.tcl.in
	$(REPLACE) @DONOTEDIT@ "$(DONOTEDIT)" $@.in > $@.out1
	$(REPLACE) @TCLLIBSPATH@ "$(TCLLIBSPATH)" $@.out1 > $@.out2
	$(REPLACE) @SOSUFFIX@ $(SOSUFFIX) $@.out2 > $@.out1
	$(REPLACE) @SLASH@ $(SLASH) $@.out1 > $@.out2
	$(REPLACE) C:\Tcl\lib C:\\Tcl\\lib $@.out2 > $@.out1
	$(MV) $@.out1 $@
	$(RM) $@.out2
	$(CHMOD) 755 $@

clean:
	$(RM) *.out1 *.out2 db2.tcl firebird.tcl freetds-sap.tcl informix.tcl mysql.tcl oracle.tcl postgresql.tcl sqlite.tcl sap.tcl tls.tcl krb.tcl cachefile* sqlnet.log

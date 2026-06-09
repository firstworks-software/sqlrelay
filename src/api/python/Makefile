top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(PYTHONCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(PYTHONCPPFLAGS) $(COMPILE) $<

all: CSQLRelay.$(LIBEXT) PySQLRDB.py

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest PySQLRDB.py PySQLRDB.py.out1 PySQLRDB.py.out2 *.pyc *.ii
	$(RMTREE) .libs __pycache__

CSQLRelay.$(LIBEXT): CSQLRelay.cpp CSQLRelay.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ CSQLRelay.$(OBJ) $(LDFLAGS) $(PYTHONLIBS) $(MODLINKFLAGS)

PySQLRDB.py: PySQLRDB.py.in
	$(REPLACE) @IMPORTEXCEPTIONS@ "$(IMPORTEXCEPTIONS)" PySQLRDB.py.in > PySQLRDB.py.out1
	$(REPLACE) @EXCEPTIONSSTANDARDERROR@ "$(EXCEPTIONSSTANDARDERROR)" PySQLRDB.py.out1 > PySQLRDB.py.out2
	$(MV) PySQLRDB.py.out2 PySQLRDB.py
	$(RM) PySQLRDB.py.out1

install: $(INSTALLLIB)
	$(CP) PySQLRClient.py $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay
	$(CHMOD) 644 $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay/PySQLRClient.py
	$(CP) PySQLRDB.py $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay
	$(CHMOD) 644 $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay/PySQLRDB.py
	$(CP) __init__.py $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay
	$(CHMOD) 644 $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay/__init__.py

installdll:
	$(MKINSTALLDIRS) $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay
	$(LTINSTALL) $(CP) CSQLRelay.$(LIBEXT) $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay/CSQLRelay.$(PYTHONSUFFIX)

installlib:
	$(MKINSTALLDIRS) $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay
	$(LTINSTALL) $(CP) CSQLRelay.$(LIBEXT) $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay
	$(RM) $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay/CSQLRelay.$(LIBEXT)
	$(RM) $(DESTDIR)$(PYTHONSITEDIR)/SQLRelay/CSQLRelay.a

uninstall:
	$(RMTREE) $(PYTHONSITEDIR)/SQLRelay
	$(RMTREE) $(PYTHONDIR)/site-packages/SQLRelay
	$(RMTREE) $(PYTHONDIR)/dist-packages/SQLRelay

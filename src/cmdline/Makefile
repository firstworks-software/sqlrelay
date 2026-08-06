top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CMDLINECPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CMDLINECPPFLAGS) $(COMPILE) $<

all: $(SQLR)sh$(EXE) \
	$(SQLR)-export$(EXE) \
	$(SQLR)-import$(EXE) \
	$(SQLR)-pwdenc$(EXE)

clean:
	$(RM) *.lo *.o *.obj *.ii $(SQLR)sh$(EXE) $(SQLR)-export$(EXE) $(SQLR)-import$(EXE) $(SQLR)-pwdenc$(EXE)
	$(RMTREE) .libs

$(SQLR)sh$(EXE): sqlrsh.cpp sqlrsh.$(OBJ) 
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ sqlrsh.$(OBJ) $(CMDLINELIBS)

$(SQLR)-export$(EXE): sqlr-export.cpp sqlr-export.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ sqlr-export.$(OBJ) $(CMDLINELIBS)

$(SQLR)-import$(EXE): sqlr-import.cpp sqlr-import.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ sqlr-import.$(OBJ) $(CMDLINELIBS)

$(SQLR)-pwdenc$(EXE): sqlr-pwdenc.cpp sqlr-pwdenc.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ sqlr-pwdenc.$(OBJ) $(CMDLINELIBS)

install: install-sh \
		install-export \
		install-import \
		install-pwdenc

install-sh:
	$(MKINSTALLDIRS) $(bindir)
	$(LTINSTALL) $(CP) $(SQLR)sh$(EXE) $(bindir)

install-export:
	$(MKINSTALLDIRS) $(bindir)
	$(LTINSTALL) $(CP) $(SQLR)-export$(EXE) $(bindir)

install-import:
	$(MKINSTALLDIRS) $(bindir)
	$(LTINSTALL) $(CP) $(SQLR)-import$(EXE) $(bindir)

install-pwdenc:
	$(MKINSTALLDIRS) $(bindir)
	$(LTINSTALL) $(CP) $(SQLR)-pwdenc$(EXE) $(bindir)

uninstall: uninstall-sh \
		uninstall-export \
		uninstall-import \
		uninstall-pwdenc
	$(RM) $(bindir)/sqlrsh$(EXE) \
		$(bindir)/sqlr-export$(EXE) \
		$(bindir)/sqlr-import$(EXE) \
		$(bindir)/sqlr-pwdenc$(EXE) \
		$(bindir)/$(SQLR)-fields$(EXE) \
		$(bindir)/$(SQLR)-query$(EXE) \
		$(bindir)/sqlr-query$(EXE) \
		$(bindir)/sqlr-fields$(EXE) \
		$(bindir)/fields$(EXE) \
		$(bindir)/query$(EXE)

uninstall-sh:
	$(RM) $(bindir)/$(SQLR)sh$(EXE)

uninstall-export:
	$(RM) $(bindir)/$(SQLR)-export$(EXE)

uninstall-import:
	$(RM) $(bindir)/$(SQLR)-import$(EXE)

uninstall-pwdenc:
	$(RM) $(bindir)/$(SQLR)-pwdenc$(EXE)

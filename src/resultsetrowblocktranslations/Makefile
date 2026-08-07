top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)resultsetrowblocktranslation_test.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)resultsetrowblocktranslation_test.$(LIBEXT): test.cpp test.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ test.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-test

installdll-test:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)resultsetrowblocktranslation_test.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-test

installsharedlib-test:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)resultsetrowblocktranslation_test.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)resultsetrowblocktranslation_test.a
	$(RM) $(libexecdir)/$(SQLR)resultsetrowblocktranslation_test.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)resultsetrowblocktranslation_test.so so $(MODULESUFFIX)

uninstall: uninstall-test
	$(RM) $(libexecdir)/sqlrresultsetrowblocktranslation_test.*

uninstall-test:
	$(RM) $(libexecdir)/$(SQLR)resultsetrowblocktranslation_test.*

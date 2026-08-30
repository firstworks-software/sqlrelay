top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)resultsetrowtranslation_test.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)resultsetrowtranslation_test.$(LIBEXT): test.cpp test.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ test.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-test

installdll-test:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)resultsetrowtranslation_test.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-test

installsharedlib-test:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)resultsetrowtranslation_test.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)resultsetrowtranslation_test.a
	$(RM) $(libexecdir)/$(SQLR)resultsetrowtranslation_test.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)resultsetrowtranslation_test.so so $(MODULESUFFIX)

uninstall: uninstall-test
	$(RM) $(libexecdir)/sqlrresultsetrowtranslation_test.*

uninstall-test:
	$(RM) $(libexecdir)/$(SQLR)resultsetrowtranslation_test.*

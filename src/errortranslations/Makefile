top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)errortranslation_patterns.$(LIBEXT) \
	$(SQLR)errortranslation_renumber.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)errortranslation_patterns.$(LIBEXT): patterns.cpp patterns.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ patterns.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)errortranslation_renumber.$(LIBEXT): renumber.cpp renumber.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ renumber.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-patterns \
		installdll-renumber

installdll-patterns:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)errortranslation_patterns.$(LIBEXT) $(libexecdir)

installdll-renumber:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)errortranslation_renumber.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-patterns \
		installsharedlib-renumber

installsharedlib-patterns:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)errortranslation_patterns.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)errortranslation_patterns.a
	$(RM) $(libexecdir)/$(SQLR)errortranslation_patterns.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)errortranslation_patterns.so so $(MODULESUFFIX)

installsharedlib-renumber:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)errortranslation_renumber.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)errortranslation_renumber.a
	$(RM) $(libexecdir)/$(SQLR)errortranslation_renumber.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)errortranslation_renumber.so so $(MODULESUFFIX)

uninstall: uninstall-patterns \
		uninstall-renumber

uninstall-patterns:
	$(RM) $(libexecdir)/$(SQLR)errortranslation_patterns.*

uninstall-renumber:
	$(RM) $(libexecdir)/$(SQLR)errortranslation_renumber.*

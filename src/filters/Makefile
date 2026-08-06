top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)filter_regex.$(LIBEXT) \
	$(SQLR)filter_string.$(LIBEXT) \
	$(SQLR)filter_patterns.$(LIBEXT) \
	$(SQLR)filter_tag.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)filter_regex.$(LIBEXT): regex.cpp regex.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ regex.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)filter_string.$(LIBEXT): string.cpp string.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ string.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)filter_patterns.$(LIBEXT): patterns.cpp patterns.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ patterns.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)filter_tag.$(LIBEXT): tag.cpp tag.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ tag.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-regex \
		installdll-string \
		installdll-patterns \
		installdll-tag

installdll-regex:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_regex.$(LIBEXT) $(libexecdir)

installdll-string:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_string.$(LIBEXT) $(libexecdir)

installdll-patterns:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_patterns.$(LIBEXT) $(libexecdir)

installdll-tag:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_tag.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-regex \
		installsharedlib-string \
		installsharedlib-patterns \
		installsharedlib-tag

installsharedlib-regex:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_regex.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)filter_regex.a
	$(RM) $(libexecdir)/$(SQLR)filter_regex.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)filter_regex.so so $(MODULESUFFIX)

installsharedlib-string:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_string.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)filter_string.a
	$(RM) $(libexecdir)/$(SQLR)filter_string.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)filter_string.so so $(MODULESUFFIX)

installsharedlib-patterns:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_patterns.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)filter_patterns.a
	$(RM) $(libexecdir)/$(SQLR)filter_patterns.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)filter_patterns.so so $(MODULESUFFIX)

installsharedlib-tag:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)filter_tag.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)filter_tag.a
	$(RM) $(libexecdir)/$(SQLR)filter_tag.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)filter_tag.so so $(MODULESUFFIX)

uninstall: uninstall-regex \
		uninstall-string \
		uninstall-patterns \
		uninstall-tag
	$(RM) $(libexecdir)/sqlrfilter_regex.* \
		$(libexecdir)/sqlrfilter_string.* \
		$(libexecdir)/sqlrfilter_patterns.*

uninstall-regex:
	$(RM) $(libexecdir)/$(SQLR)filter_regex.*

uninstall-string:
	$(RM) $(libexecdir)/$(SQLR)filter_string.*

uninstall-patterns:
	$(RM) $(libexecdir)/$(SQLR)filter_patterns.*

uninstall-tag:
	$(RM) $(libexecdir)/$(SQLR)filter_tag.*

top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)logger_debug.$(LIBEXT) \
	$(SQLR)logger_slowqueries.$(LIBEXT) \
	$(SQLR)logger_stalecursors.$(LIBEXT) \
	$(SQLR)logger_sql.$(LIBEXT) \
	$(SQLR)logger_custom_nw.$(LIBEXT) \
	$(SQLR)logger_custom_sc.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)logger_debug.$(LIBEXT): debug.cpp debug.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ debug.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)logger_slowqueries.$(LIBEXT): slowqueries.cpp slowqueries.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ slowqueries.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)logger_stalecursors.$(LIBEXT): stalecursors.cpp stalecursors.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ stalecursors.$(OBJ) $(LDFLAGS) $(SQLRLOGGER_STALECURSORSLIBS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)logger_sql.$(LIBEXT): sql.cpp sql.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sql.$(OBJ) $(LDFLAGS) $(SQLRLOGGER_STALECURSORSLIBS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)logger_custom_nw.$(LIBEXT): custom_nw.cpp custom_nw.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ custom_nw.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)logger_custom_sc.$(LIBEXT): custom_sc.cpp custom_sc.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ custom_sc.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-debug \
		installdll-slowqueries \
		installdll-stalecursors \
		installdll-sql \
		installdll-custom_nw \
		installdll-custom_sc

installdll-debug:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_debug.$(LIBEXT) $(libexecdir)

installdll-slowqueries:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_slowqueries.$(LIBEXT) $(libexecdir)

installdll-stalecursors:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_stalecursors.$(LIBEXT) $(libexecdir)

installdll-sql:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_sql.$(LIBEXT) $(libexecdir)

installdll-custom_nw:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_custom_nw.$(LIBEXT) $(libexecdir)

installdll-custom_sc:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_custom_sc.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-debug \
		installsharedlib-slowqueries \
		installsharedlib-stalecursors \
		installsharedlib-sql \
		installsharedlib-custom_nw \
		installsharedlib-custom_sc

installsharedlib-debug:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_debug.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)logger_debug.a
	$(RM) $(libexecdir)/$(SQLR)logger_debug.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)logger_debug.so so $(MODULESUFFIX)

installsharedlib-slowqueries:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_slowqueries.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)logger_slowqueries.a
	$(RM) $(libexecdir)/$(SQLR)logger_slowqueries.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)logger_slowqueries.so so $(MODULESUFFIX)

installsharedlib-stalecursors:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_stalecursors.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)logger_stalecursors.a
	$(RM) $(libexecdir)/$(SQLR)logger_stalecursors.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)logger_stalecursors.so so $(MODULESUFFIX)

installsharedlib-sql:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_sql.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)logger_sql.a
	$(RM) $(libexecdir)/$(SQLR)logger_sql.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)logger_sql.so so $(MODULESUFFIX)

installsharedlib-custom_nw:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_custom_nw.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)logger_custom_nw.a
	$(RM) $(libexecdir)/$(SQLR)logger_custom_nw.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)logger_custom_nw.so so $(MODULESUFFIX)

installsharedlib-custom_sc:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)logger_custom_sc.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)logger_custom_sc.a
	$(RM) $(libexecdir)/$(SQLR)logger_custom_sc.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)logger_custom_sc.so so $(MODULESUFFIX)

uninstall: uninstall-debug \
		uninstall-slowqueries \
		uninstall-stalecursors \
		uninstall-sql \
		uninstall-custom_nw \
		uninstall-custom_sc
	$(RM) $(libexecdir)/sqlrlogger_custom_nw.* \
		$(libexecdir)/sqlrlogger_custom_sc.* \
		$(libexecdir)/sqlrlogger_debug.* \
		$(libexecdir)/sqlrlogger_slowqueries.* \
		$(libexecdir)/sqlrlogger_stalecursors.* \
		$(libexecdir)/sqlrlogger_sql.*

uninstall-debug:
	$(RM) $(libexecdir)/$(SQLR)logger_debug.*

uninstall-slowqueries:
	$(RM) $(libexecdir)/$(SQLR)logger_slowqueries.*

uninstall-stalecursors:
	$(RM) $(libexecdir)/$(SQLR)logger_stalecursors.*

uninstall-sql:
	$(RM) $(libexecdir)/$(SQLR)logger_sql.*

uninstall-custom_nw:
	$(RM) $(libexecdir)/$(SQLR)logger_custom_nw.*

uninstall-custom_sc:
	$(RM) $(libexecdir)/$(SQLR)logger_custom_sc.*

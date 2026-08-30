top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)trigger_replay.$(LIBEXT) \
	$(SQLR)trigger_upsert.$(LIBEXT) \
	$(SQLR)trigger_splitmultiinsert.$(LIBEXT) \
	$(SQLR)trigger_savepoints.$(LIBEXT) \
	$(SQLR)trigger_globaltemptables.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)trigger_replay.$(LIBEXT): replay.cpp replay.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ replay.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)trigger_upsert.$(LIBEXT): upsert.cpp upsert.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ upsert.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)trigger_splitmultiinsert.$(LIBEXT): splitmultiinsert.cpp splitmultiinsert.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ splitmultiinsert.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)trigger_savepoints.$(LIBEXT): savepoints.cpp savepoints.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ savepoints.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)trigger_globaltemptables.$(LIBEXT): globaltemptables.cpp globaltemptables.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ globaltemptables.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-replay \
		installdll-upsert \
		installdll-splitmultiinsert \
		installdll-savepoints \
		installdll-globaltemptables

installdll-replay:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_replay.$(LIBEXT) $(libexecdir)

installdll-upsert:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_upsert.$(LIBEXT) $(libexecdir)

installdll-splitmultiinsert:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_splitmultiinsert.$(LIBEXT) $(libexecdir)

installdll-savepoints:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_savepoints.$(LIBEXT) $(libexecdir)

installdll-globaltemptables:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_globaltemptables.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-replay \
		installsharedlib-upsert \
		installsharedlib-splitmultiinsert \
		installsharedlib-savepoints \
		installsharedlib-globaltemptables

installsharedlib-replay:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_replay.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)trigger_replay.a
	$(RM) $(libexecdir)/$(SQLR)trigger_replay.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)trigger_replay.so so $(MODULESUFFIX)

installsharedlib-upsert:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_upsert.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)trigger_upsert.a
	$(RM) $(libexecdir)/$(SQLR)trigger_upsert.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)trigger_upsert.so so $(MODULESUFFIX)

installsharedlib-splitmultiinsert:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_splitmultiinsert.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)trigger_splitmultiinsert.a
	$(RM) $(libexecdir)/$(SQLR)trigger_splitmultiinsert.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)trigger_splitmultiinsert.so so $(MODULESUFFIX)

installsharedlib-savepoints:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_savepoints.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)trigger_savepoints.a
	$(RM) $(libexecdir)/$(SQLR)trigger_savepoints.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)trigger_savepoints.so so $(MODULESUFFIX)

installsharedlib-globaltemptables:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)trigger_globaltemptables.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)trigger_globaltemptables.a
	$(RM) $(libexecdir)/$(SQLR)trigger_globaltemptables.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)trigger_globaltemptables.so so $(MODULESUFFIX)

uninstall: uninstall-replay \
		uninstall-upsert \
		uninstall-splitmultiinsert \
		uninstall-savepoints \
		uninstall-globaltemptables
	$(RM) $(libexecdir)/$(SQLR)trigger_deadlock_replay.*

uninstall-replay:
	$(RM) $(libexecdir)/$(SQLR)trigger_replay.*

uninstall-upsert:
	$(RM) $(libexecdir)/$(SQLR)trigger_upsert.*

uninstall-splitmultiinsert:
	$(RM) $(libexecdir)/$(SQLR)trigger_splitmultiinsert.*

uninstall-savepoints:
	$(RM) $(libexecdir)/$(SQLR)trigger_savepoints.*

uninstall-globaltemptables:
	$(RM) $(libexecdir)/$(SQLR)trigger_globaltemptables.*

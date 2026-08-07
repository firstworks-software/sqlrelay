top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)schedule_cron_userlist.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)schedule_cron_userlist.$(LIBEXT): cron_userlist.cpp cron_userlist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ cron_userlist.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-cron_userlist

installdll-cron_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)schedule_cron_userlist.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-cron_userlist

installsharedlib-cron_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)schedule_cron_userlist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)schedule_cron_userlist.a
	$(RM) $(libexecdir)/$(SQLR)schedule_cron_userlist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)schedule_cron_userlist.so so $(MODULESUFFIX)

uninstall: uninstall-cron_userlist

uninstall-cron_userlist:
	$(RM) $(libexecdir)/$(SQLR)schedule_cron_userlist.*

top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)router_regex.$(LIBEXT) \
	$(SQLR)router_userlist.$(LIBEXT) \
	$(SQLR)router_clientiplist.$(LIBEXT) \
	$(SQLR)router_clientinfolist.$(LIBEXT) \
	$(SQLR)router_usedatabase.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)router_regex.$(LIBEXT): regex.cpp regex.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ regex.$(OBJ) $(LDFLAGS) $(ROUTERPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)router_userlist.$(LIBEXT): userlist.cpp userlist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ userlist.$(OBJ) $(LDFLAGS) $(ROUTERPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)router_clientiplist.$(LIBEXT): clientiplist.cpp clientiplist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ clientiplist.$(OBJ) $(LDFLAGS) $(ROUTERPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)router_clientinfolist.$(LIBEXT): clientinfolist.cpp clientinfolist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ clientinfolist.$(OBJ) $(LDFLAGS) $(ROUTERPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)router_usedatabase.$(LIBEXT): usedatabase.cpp usedatabase.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ usedatabase.$(OBJ) $(LDFLAGS) $(ROUTERPLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-regex \
		installdll-userlist \
		installdll-clientiplist \
		installdll-clientinfolist \
		installdll-usedatabase

installdll-regex:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_regex.$(LIBEXT) $(libexecdir)

installdll-userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_userlist.$(LIBEXT) $(libexecdir)

installdll-clientiplist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_clientiplist.$(LIBEXT) $(libexecdir)

installdll-clientinfolist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_clientinfolist.$(LIBEXT) $(libexecdir)

installdll-usedatabase:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_usedatabase.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-regex \
		installsharedlib-userlist \
		installsharedlib-clientiplist \
		installsharedlib-clientinfolist \
		installsharedlib-usedatabase

installsharedlib-regex:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_regex.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)router_regex.a
	$(RM) $(libexecdir)/$(SQLR)router_regex.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)router_regex.so so $(MODULESUFFIX)

installsharedlib-userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_userlist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)router_userlist.a
	$(RM) $(libexecdir)/$(SQLR)router_userlist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)router_userlist.so so $(MODULESUFFIX)

installsharedlib-clientiplist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_clientiplist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)router_clientiplist.a
	$(RM) $(libexecdir)/$(SQLR)router_clientiplist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)router_clientiplist.so so $(MODULESUFFIX)

installsharedlib-clientinfolist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_clientinfolist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)router_clientinfolist.a
	$(RM) $(libexecdir)/$(SQLR)router_clientinfolist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)router_clientinfolist.so so $(MODULESUFFIX)

installsharedlib-usedatabase:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)router_usedatabase.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)router_usedatabase.a
	$(RM) $(libexecdir)/$(SQLR)router_usedatabase.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)router_usedatabase.so so $(MODULESUFFIX)

uninstall: uninstall-regex \
		uninstall-userlist \
		uninstall-clientiplist \
		uninstall-clientinfolist \
		uninstall-usedatabase

uninstall-regex:
	$(RM) $(libexecdir)/$(SQLR)router_regex.*

uninstall-userlist:
	$(RM) $(libexecdir)/$(SQLR)router_userlist.*

uninstall-clientiplist:
	$(RM) $(libexecdir)/$(SQLR)router_clientiplist.*

uninstall-clientinfolist:
	$(RM) $(libexecdir)/$(SQLR)router_clientinfolist.*

uninstall-usedatabase:
	$(RM) $(libexecdir)/$(SQLR)router_usedatabase.*

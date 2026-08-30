top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)notification_events.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)notification_events.$(LIBEXT): events.cpp events.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ events.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-events

installdll-events:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)notification_events.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-events

installsharedlib-events:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)notification_events.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)notification_events.a
	$(RM) $(libexecdir)/$(SQLR)notification_events.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)notification_events.so so $(MODULESUFFIX)

uninstall: uninstall-events

uninstall-events:
	$(RM) $(libexecdir)/$(SQLR)notification_events.*

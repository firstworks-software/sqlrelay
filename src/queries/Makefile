top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)query_sqlrcmdgstat.$(LIBEXT) \
	$(SQLR)query_sqlrcmdcstat.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)query_sqlrcmdgstat.$(LIBEXT): sqlrcmdgstat.cpp sqlrcmdgstat.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrcmdgstat.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)query_sqlrcmdcstat.$(LIBEXT): sqlrcmdcstat.cpp sqlrcmdcstat.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrcmdcstat.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-sqlrcmdgstat \
		installdll-sqlrcmdcstat

installdll-sqlrcmdgstat:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)query_sqlrcmdgstat.$(LIBEXT) $(libexecdir)

installdll-sqlrcmdcstat:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)query_sqlrcmdcstat.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-sqlrcmdgstat \
		installsharedlib-sqlrcmdcstat

installsharedlib-sqlrcmdgstat:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)query_sqlrcmdgstat.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)query_sqlrcmdgstat.a
	$(RM) $(libexecdir)/$(SQLR)query_sqlrcmdgstat.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)query_sqlrcmdgstat.so so $(MODULESUFFIX)

installsharedlib-sqlrcmdcstat:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)query_sqlrcmdcstat.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)query_sqlrcmdcstat.a
	$(RM) $(libexecdir)/$(SQLR)query_sqlrcmdcstat.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)query_sqlrcmdcstat.so so $(MODULESUFFIX)

uninstall: uninstall-sqlrcmdgstat \
		uninstall-sqlrcmdcstat
	$(RM) $(libexecdir)/sqlrquery_sqlrcmdgstat.* \
		$(libexecdir)/sqlrquery_sqlrcmdcstat.*

uninstall-sqlrcmdgstat:
	$(RM) $(libexecdir)/$(SQLR)query_sqlrcmdgstat.*

uninstall-sqlrcmdcstat:
	$(RM) $(libexecdir)/$(SQLR)query_sqlrcmdcstat.*

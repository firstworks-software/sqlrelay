top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)resultsettranslation_reformatdatetime.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)resultsettranslation_reformatdatetime.$(LIBEXT): reformatdatetime.cpp reformatdatetime.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ reformatdatetime.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-reformatdatetime

installdll-reformatdatetime:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)resultsettranslation_reformatdatetime.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-reformatdatetime

installsharedlib-reformatdatetime:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)resultsettranslation_reformatdatetime.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)resultsettranslation_reformatdatetime.a
	$(RM) $(libexecdir)/$(SQLR)resultsettranslation_reformatdatetime.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)resultsettranslation_reformatdatetime.so so $(MODULESUFFIX)

uninstall: uninstall-reformatdatetime
	$(RM) $(libexecdir)/sqlrresultsettranslation_reformatdatetime.*

uninstall-reformatdatetime:
	$(RM) $(libexecdir)/$(SQLR)resultsettranslation_reformatdatetime.*

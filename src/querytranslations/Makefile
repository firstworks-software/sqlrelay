top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)querytranslation_normalize.$(LIBEXT) \
	$(SQLR)querytranslation_patterns.$(LIBEXT) \
	$(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT) \

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)querytranslation_normalize.$(LIBEXT): normalize.cpp normalize.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ normalize.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)querytranslation_patterns.$(LIBEXT): patterns.cpp patterns.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ patterns.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT): listagg_to_string_agg.cpp listagg_to_string_agg.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ listagg_to_string_agg.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_normalize.$(LIBEXT) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_patterns.$(LIBEXT) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_normalize.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_normalize.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_normalize.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_normalize.so so $(MODULESUFFIX)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_patterns.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_patterns.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_patterns.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_patterns.so so $(MODULESUFFIX)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.so so $(MODULESUFFIX)

uninstall:
	$(RM) $(libexecdir)/$(SQLR)querytranslation_normalize.* \
		$(libexecdir)/$(SQLR)querytranslation_patterns.* \
		$(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.* \
		$(libexecdir)/$(SQLR)translation_normalize.* \
		$(libexecdir)/$(SQLR)translation_patterns.* \
		$(libexecdir)/sqlrtranslation_normalize.*

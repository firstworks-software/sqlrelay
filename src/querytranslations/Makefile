top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)querytranslation_normalize.$(LIBEXT) \
	$(SQLR)querytranslation_patterns.$(LIBEXT) \
	$(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT) \
	$(SQLR)querytranslation_sequence_nextval.$(LIBEXT) \
	$(SQLR)querytranslation_keep_dense_rank_to_array_agg.$(LIBEXT) \

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

stagemodules: all
	$(STAGEMODULES) $(STAGEDIR) $(MODULESUFFIX)

$(SQLR)querytranslation_normalize.$(LIBEXT): normalize.cpp normalize.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ normalize.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)querytranslation_patterns.$(LIBEXT): patterns.cpp patterns.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ patterns.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT): listagg_to_string_agg.cpp listagg_to_string_agg.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ listagg_to_string_agg.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)querytranslation_sequence_nextval.$(LIBEXT): sequence_nextval.cpp sequence_nextval.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sequence_nextval.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)querytranslation_keep_dense_rank_to_array_agg.$(LIBEXT): keep_dense_rank_to_array_agg.cpp keep_dense_rank_to_array_agg.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ keep_dense_rank_to_array_agg.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-normalize \
		installdll-patterns \
		installdll-listagg_to_string_agg \
		installdll-sequence_nextval \
		installdll-keep_dense_rank_to_array_agg

installdll-normalize:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_normalize.$(LIBEXT) $(libexecdir)

installdll-patterns:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_patterns.$(LIBEXT) $(libexecdir)

installdll-listagg_to_string_agg:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT) $(libexecdir)

installdll-sequence_nextval:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_sequence_nextval.$(LIBEXT) $(libexecdir)

installdll-keep_dense_rank_to_array_agg:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_keep_dense_rank_to_array_agg.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-normalize \
		installsharedlib-patterns \
		installsharedlib-listagg_to_string_agg \
		installsharedlib-sequence_nextval \
		installsharedlib-keep_dense_rank_to_array_agg

installsharedlib-normalize:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_normalize.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_normalize.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_normalize.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_normalize.so so $(MODULESUFFIX)

installsharedlib-patterns:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_patterns.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_patterns.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_patterns.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_patterns.so so $(MODULESUFFIX)

installsharedlib-listagg_to_string_agg:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.so so $(MODULESUFFIX)

installsharedlib-sequence_nextval:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_sequence_nextval.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_sequence_nextval.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_sequence_nextval.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_sequence_nextval.so so $(MODULESUFFIX)

installsharedlib-keep_dense_rank_to_array_agg:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)querytranslation_keep_dense_rank_to_array_agg.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)querytranslation_keep_dense_rank_to_array_agg.a
	$(RM) $(libexecdir)/$(SQLR)querytranslation_keep_dense_rank_to_array_agg.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)querytranslation_keep_dense_rank_to_array_agg.so so $(MODULESUFFIX)

uninstall: uninstall-normalize \
		uninstall-patterns \
		uninstall-listagg_to_string_agg \
		uninstall-sequence_nextval \
		uninstall-keep_dense_rank_to_array_agg
	$(RM) $(libexecdir)/$(SQLR)translation_normalize.* \
		$(libexecdir)/$(SQLR)translation_patterns.* \
		$(libexecdir)/sqlrtranslation_normalize.*

uninstall-normalize:
	$(RM) $(libexecdir)/$(SQLR)querytranslation_normalize.*

uninstall-patterns:
	$(RM) $(libexecdir)/$(SQLR)querytranslation_patterns.*

uninstall-listagg_to_string_agg:
	$(RM) $(libexecdir)/$(SQLR)querytranslation_listagg_to_string_agg.*

uninstall-sequence_nextval:
	$(RM) $(libexecdir)/$(SQLR)querytranslation_sequence_nextval.*

uninstall-keep_dense_rank_to_array_agg:
	$(RM) $(libexecdir)/$(SQLR)querytranslation_keep_dense_rank_to_array_agg.*

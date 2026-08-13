top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(COMPILE) $<

all: $(SQLR)protocol_sqlrclient.$(LIBEXT) \
	$(SQLR)protocol_mysql.$(LIBEXT) \
	$(SQLR)protocol_postgresql.$(LIBEXT) \
	$(SQLR)protocol_firebird.$(LIBEXT) \
	$(SQLR)protocol_tds.$(LIBEXT) \
	$(SQLR)protocol_oracle.$(LIBEXT) \
	$(SQLR)protocol_teradata.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)protocol_sqlrclient.$(LIBEXT): sqlrclient.cpp sqlrclient.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrclient.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)protocol_mysql.$(LIBEXT): mysql.cpp mysql.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ mysql.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)protocol_postgresql.$(LIBEXT): postgresql.cpp postgresql.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ postgresql.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)protocol_firebird.$(LIBEXT): firebird.cpp firebird.$(OBJ) \
					firebirdsdl.cpp firebirdsdl.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ firebird.$(OBJ) firebirdsdl.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)protocol_tds.$(LIBEXT): tds.cpp tds.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ tds.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)protocol_oracle.$(LIBEXT): oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ oracle.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)protocol_teradata.$(LIBEXT): teradata.cpp teradata.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ teradata.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: $(PROTOCOLSINSTALLTARGETS) \
		installdll-sqlrclient \
		installdll-mysql \
		installdll-postgresql \
		installdll-firebird

installdll-sqlrclient:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_sqlrclient.$(LIBEXT) $(libexecdir)

installdll-mysql:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_mysql.$(LIBEXT) $(libexecdir)

installdll-postgresql:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_postgresql.$(LIBEXT) $(libexecdir)

installdll-firebird:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_firebird.$(LIBEXT) $(libexecdir)

installdll-tds:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_tds.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: $(PROTOCOLSINSTALLTARGETS) \
		installsharedlib-sqlrclient \
		installsharedlib-mysql \
		installsharedlib-postgresql \
		installsharedlib-firebird

installsharedlib-sqlrclient:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_sqlrclient.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)protocol_sqlrclient.a
	$(RM) $(libexecdir)/$(SQLR)protocol_sqlrclient.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)protocol_sqlrclient.so so $(MODULESUFFIX)

installsharedlib-mysql:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_mysql.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)protocol_mysql.a
	$(RM) $(libexecdir)/$(SQLR)protocol_mysql.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)protocol_mysql.so so $(MODULESUFFIX)

installsharedlib-postgresql:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_postgresql.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)protocol_postgresql.a
	$(RM) $(libexecdir)/$(SQLR)protocol_postgresql.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)protocol_postgresql.so so $(MODULESUFFIX)

installsharedlib-firebird:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_firebird.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)protocol_firebird.a
	$(RM) $(libexecdir)/$(SQLR)protocol_firebird.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)protocol_firebird.so so $(MODULESUFFIX)

installsharedlib-oracle:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_oracle.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)protocol_oracle.a
	$(RM) $(libexecdir)/$(SQLR)protocol_oracle.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)protocol_oracle.so so $(MODULESUFFIX)

installsharedlib-tds:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_tds.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)protocol_tds.a
	$(RM) $(libexecdir)/$(SQLR)protocol_tds.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)protocol_tds.so so $(MODULESUFFIX)

installsharedlib-teradata:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)protocol_teradata.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)protocol_teradata.a
	$(RM) $(libexecdir)/$(SQLR)protocol_teradata.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)protocol_teradata.so so $(MODULESUFFIX)

uninstall: uninstall-sqlrclient \
		uninstall-mysql \
		uninstall-postgresql \
		uninstall-firebird \
		uninstall-oracle \
		uninstall-tds \
		uninstall-teradata
	$(RM) $(libexecdir)/sqlrprotocol_sqlrclient.* \
		$(libexecdir)/sqlrprotocol_mysql.* \
		$(libexecdir)/sqlrprotocol_postgresql.* \
		$(libexecdir)/sqlrprotocol_firebird.* \
		$(libexecdir)/sqlrprotocol_oracle.* \
		$(libexecdir)/sqlrprotocol_tds.* \
		$(libexecdir)/sqlrprotocol_teradata.*

uninstall-sqlrclient:
	$(RM) $(libexecdir)/$(SQLR)protocol_sqlrclient.*

uninstall-mysql:
	$(RM) $(libexecdir)/$(SQLR)protocol_mysql.*

uninstall-postgresql:
	$(RM) $(libexecdir)/$(SQLR)protocol_postgresql.*

uninstall-firebird:
	$(RM) $(libexecdir)/$(SQLR)protocol_firebird.*

uninstall-oracle:
	$(RM) $(libexecdir)/$(SQLR)protocol_oracle.*

uninstall-tds:
	$(RM) $(libexecdir)/$(SQLR)protocol_tds.*

uninstall-teradata:
	$(RM) $(libexecdir)/$(SQLR)protocol_teradata.*

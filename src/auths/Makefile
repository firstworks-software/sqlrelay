top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(AUTHCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(AUTHCPPFLAGS) $(COMPILE) $<

all: all-base $(AUTHALLOPTIONALTARGETS)

all-base: $(SQLR)auth_sqlrclient_connectstrings.$(LIBEXT) \
		$(SQLR)auth_sqlrclient_userlist.$(LIBEXT) \
		$(SQLR)auth_sqlrclient_database.$(LIBEXT) \
		$(SQLR)auth_sqlrclient_proxied.$(LIBEXT) \
		$(SQLR)auth_mysql_connectstrings.$(LIBEXT) \
		$(SQLR)auth_mysql_userlist.$(LIBEXT) \
		$(SQLR)auth_mysql_database.$(LIBEXT) \
		$(SQLR)auth_postgresql_connectstrings.$(LIBEXT) \
		$(SQLR)auth_postgresql_userlist.$(LIBEXT) \
		$(SQLR)auth_tds_connectstrings.$(LIBEXT) \
		$(SQLR)auth_firebird_connectstrings.$(LIBEXT) \
		$(SQLR)auth_oracle_connectstrings.$(LIBEXT) \
		$(SQLR)auth_oracle_userlist.$(LIBEXT) \
		$(SQLR)auth_oracle_database.$(LIBEXT) \
		$(SQLR)auth_teradata_sidechannel.$(LIBEXT) \

all-sqlrelay: $(SQLR)auth_sqlrclient_sqlrelay.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)auth_sqlrclient_connectstrings.$(LIBEXT): sqlrclient_connectstrings.cpp sqlrclient_connectstrings.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrclient_connectstrings.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_sqlrclient_userlist.$(LIBEXT): sqlrclient_userlist.cpp sqlrclient_userlist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrclient_userlist.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_sqlrclient_database.$(LIBEXT): sqlrclient_database.cpp sqlrclient_database.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrclient_database.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_sqlrclient_proxied.$(LIBEXT): sqlrclient_proxied.cpp sqlrclient_proxied.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrclient_proxied.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_sqlrclient_sqlrelay.$(LIBEXT): sqlrclient_sqlrelay.cpp sqlrclient_sqlrelay.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sqlrclient_sqlrelay.$(OBJ) $(LDFLAGS) $(SQLRAUTH_SQLRELAYLIBS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_mysql_connectstrings.$(LIBEXT): mysql_connectstrings.cpp mysql_connectstrings.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ mysql_connectstrings.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_mysql_userlist.$(LIBEXT): mysql_userlist.cpp mysql_userlist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ mysql_userlist.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_mysql_database.$(LIBEXT): mysql_database.cpp mysql_database.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ mysql_database.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_postgresql_connectstrings.$(LIBEXT): postgresql_connectstrings.cpp postgresql_connectstrings.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ postgresql_connectstrings.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_postgresql_userlist.$(LIBEXT): postgresql_userlist.cpp postgresql_userlist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ postgresql_userlist.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_tds_connectstrings.$(LIBEXT): tds_connectstrings.cpp tds_connectstrings.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ tds_connectstrings.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_firebird_connectstrings.$(LIBEXT): firebird_connectstrings.cpp firebird_connectstrings.$(OBJ) firebirdsrp.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ firebird_connectstrings.$(OBJ) firebirdsrp.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

# the srp crypto lives with the firebird protocol module, which needs it too,
# so it's compiled here rather than duplicated
# (one rule per object suffix, because the suffix rules above can't reach
# outside of this directory)
firebirdsrp.lo: ../protocols/firebirdsrp.cpp ../protocols/firebirdsrp.h
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(AUTHCPPFLAGS) $(COMPILE) ../protocols/firebirdsrp.cpp $(OUT)$@

firebirdsrp.obj: ../protocols/firebirdsrp.cpp ../protocols/firebirdsrp.h
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(AUTHCPPFLAGS) $(COMPILE) ../protocols/firebirdsrp.cpp

$(SQLR)auth_oracle_connectstrings.$(LIBEXT): oracle_connectstrings.cpp oracle_connectstrings.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ oracle_connectstrings.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_oracle_userlist.$(LIBEXT): oracle_userlist.cpp oracle_userlist.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ oracle_userlist.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_oracle_database.$(LIBEXT): oracle_database.cpp oracle_database.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ oracle_database.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)auth_teradata_sidechannel.$(LIBEXT): teradata_sidechannel.cpp teradata_sidechannel.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ teradata_sidechannel.$(OBJ) $(LDFLAGS) $(PLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-sqlrclient_connectstrings \
		installdll-sqlrclient_userlist \
		installdll-sqlrclient_database \
		installdll-sqlrclient_proxied \
		installdll-sqlrclient_sqlrelay \
		installdll-mysql_connectstrings \
		installdll-mysql_userlist \
		installdll-mysql_database \
		installdll-postgresql_connectstrings \
		installdll-postgresql_userlist \
		installdll-tds_connectstrings \
		installdll-firebird_connectstrings \
		installdll-oracle_connectstrings \
		installdll-oracle_userlist \
		installdll-oracle_database

installdll-sqlrclient_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_connectstrings.$(LIBEXT) $(libexecdir)

installdll-sqlrclient_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_userlist.$(LIBEXT) $(libexecdir)

installdll-sqlrclient_database:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_database.$(LIBEXT) $(libexecdir)

installdll-sqlrclient_proxied:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_proxied.$(LIBEXT) $(libexecdir)

installdll-sqlrclient_sqlrelay:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_sqlrelay.$(LIBEXT) $(libexecdir)

installdll-mysql_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_mysql_connectstrings.$(LIBEXT) $(libexecdir)

installdll-mysql_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_mysql_userlist.$(LIBEXT) $(libexecdir)

installdll-mysql_database:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_mysql_database.$(LIBEXT) $(libexecdir)

installdll-postgresql_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_postgresql_connectstrings.$(LIBEXT) $(libexecdir)

installdll-postgresql_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_postgresql_userlist.$(LIBEXT) $(libexecdir)

installdll-tds_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_tds_connectstrings.$(LIBEXT) $(libexecdir)

installdll-firebird_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_firebird_connectstrings.$(LIBEXT) $(libexecdir)

installdll-oracle_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_oracle_connectstrings.$(LIBEXT) $(libexecdir)

installdll-oracle_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_oracle_userlist.$(LIBEXT) $(libexecdir)

installdll-oracle_database:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_oracle_database.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB) $(AUTHINSTALLSHAREDLIBOPTIONALTARGETS)

installsharedlib: installsharedlib-sqlrclient_connectstrings \
		installsharedlib-sqlrclient_userlist \
		installsharedlib-sqlrclient_database \
		installsharedlib-sqlrclient_proxied \
		installsharedlib-mysql_connectstrings \
		installsharedlib-mysql_userlist \
		installsharedlib-mysql_database \
		installsharedlib-postgresql_connectstrings \
		installsharedlib-postgresql_userlist \
		installsharedlib-tds_connectstrings \
		installsharedlib-firebird_connectstrings \
		installsharedlib-oracle_connectstrings \
		installsharedlib-oracle_userlist \
		installsharedlib-oracle_database

installsharedlib-sqlrclient_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_connectstrings.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_connectstrings.a
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_connectstrings.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_sqlrclient_connectstrings.so so $(MODULESUFFIX)

installsharedlib-sqlrclient_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_userlist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_userlist.a
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_userlist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_sqlrclient_userlist.so so $(MODULESUFFIX)

installsharedlib-sqlrclient_database:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_database.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_database.a
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_database.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_sqlrclient_database.so so $(MODULESUFFIX)

installsharedlib-sqlrclient_proxied:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_proxied.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_proxied.a
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_proxied.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_sqlrclient_proxied.so so $(MODULESUFFIX)

installsharedlib-mysql_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_mysql_connectstrings.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_connectstrings.a
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_connectstrings.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_mysql_connectstrings.so so $(MODULESUFFIX)

installsharedlib-mysql_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_mysql_userlist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_userlist.a
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_userlist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_mysql_userlist.so so $(MODULESUFFIX)

installsharedlib-mysql_database:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_mysql_database.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_database.a
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_database.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_mysql_database.so so $(MODULESUFFIX)

installsharedlib-postgresql_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_postgresql_connectstrings.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_postgresql_connectstrings.a
	$(RM) $(libexecdir)/$(SQLR)auth_postgresql_connectstrings.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_postgresql_connectstrings.so so $(MODULESUFFIX)

installsharedlib-postgresql_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_postgresql_userlist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_postgresql_userlist.a
	$(RM) $(libexecdir)/$(SQLR)auth_postgresql_userlist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_postgresql_userlist.so so $(MODULESUFFIX)

installsharedlib-tds_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_tds_connectstrings.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_tds_connectstrings.a
	$(RM) $(libexecdir)/$(SQLR)auth_tds_connectstrings.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_tds_connectstrings.so so $(MODULESUFFIX)

installsharedlib-firebird_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_firebird_connectstrings.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_firebird_connectstrings.a
	$(RM) $(libexecdir)/$(SQLR)auth_firebird_connectstrings.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_firebird_connectstrings.so so $(MODULESUFFIX)

installsharedlib-sqlrclient_sqlrelay:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_sqlrclient_sqlrelay.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_sqlrelay.a
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_sqlrelay.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_sqlrclient_sqlrelay.so so $(MODULESUFFIX)

installsharedlib-sqlrelay: installsharedlib-sqlrclient_sqlrelay

installsharedlib-oracle_connectstrings:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_oracle_connectstrings.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_connectstrings.a
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_connectstrings.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_oracle_connectstrings.so so $(MODULESUFFIX)

installsharedlib-oracle_database:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_oracle_database.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_database.a
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_database.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_oracle_database.so so $(MODULESUFFIX)

installsharedlib-oracle_userlist:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_oracle_userlist.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_userlist.a
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_userlist.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_oracle_userlist.so so $(MODULESUFFIX)

installsharedlib-oracle: installsharedlib-oracle_connectstrings \
		installsharedlib-oracle_database \
		installsharedlib-oracle_userlist

installsharedlib-teradata_sidechannel:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)auth_teradata_sidechannel.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)auth_teradata_sidechannel.a
	$(RM) $(libexecdir)/$(SQLR)auth_teradata_sidechannel.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)auth_teradata_sidechannel.so so $(MODULESUFFIX)

installsharedlib-teradata: installsharedlib-teradata_sidechannel

uninstall: uninstall-sqlrclient_connectstrings \
		uninstall-sqlrclient_userlist \
		uninstall-sqlrclient_database \
		uninstall-sqlrclient_proxied \
		uninstall-mysql_connectstrings \
		uninstall-mysql_userlist \
		uninstall-mysql_database \
		uninstall-postgresql_connectstrings \
		uninstall-postgresql_userlist \
		uninstall-tds_connectstrings \
		uninstall-firebird_connectstrings \
		uninstall-sqlrclient_sqlrelay \
		uninstall-oracle_connectstrings \
		uninstall-oracle_database \
		uninstall-oracle_userlist \
		uninstall-teradata_sidechannel
	$(RM) $(libexecdir)/$(SQLR)auth_default.* \
		$(libexecdir)/$(SQLR)auth_krb_userlist.* \
		$(libexecdir)/$(SQLR)auth_tls_userlist.* \
		$(libexecdir)/$(SQLR)auth_kerberos_userlist.* \
		$(libexecdir)/$(SQLR)auth_connectstrings.* \
		$(libexecdir)/$(SQLR)auth_userlist.* \
		$(libexecdir)/$(SQLR)auth_database.* \
		$(libexecdir)/$(SQLR)auth_proxied.* \
		$(libexecdir)/$(SQLR)auth_sqlrelay.* \
		$(libexecdir)/sqlrauth_default.* \
		$(libexecdir)/sqlrauth_userlist.* \
		$(libexecdir)/sqlrauth_database.* \
		$(libexecdir)/sqlrauth_proxied.* \
		$(libexecdir)/sqlrauth_krb_userlist.* \
		$(libexecdir)/sqlrauth_kerberos_userlist.* \
		$(libexecdir)/sqlrauth_sqlrelay.*

uninstall-sqlrclient_connectstrings:
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_connectstrings.*

uninstall-sqlrclient_userlist:
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_userlist.*

uninstall-sqlrclient_database:
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_database.*

uninstall-sqlrclient_proxied:
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_proxied.*

uninstall-mysql_connectstrings:
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_connectstrings.*

uninstall-mysql_userlist:
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_userlist.*

uninstall-mysql_database:
	$(RM) $(libexecdir)/$(SQLR)auth_mysql_database.*

uninstall-postgresql_connectstrings:
	$(RM) $(libexecdir)/$(SQLR)auth_postgresql_connectstrings.*

uninstall-postgresql_userlist:
	$(RM) $(libexecdir)/$(SQLR)auth_postgresql_userlist.*

uninstall-tds_connectstrings:
	$(RM) $(libexecdir)/$(SQLR)auth_tds_connectstrings.*

uninstall-firebird_connectstrings:
	$(RM) $(libexecdir)/$(SQLR)auth_firebird_connectstrings.*

uninstall-sqlrclient_sqlrelay:
	$(RM) $(libexecdir)/$(SQLR)auth_sqlrclient_sqlrelay.*

uninstall-oracle_connectstrings:
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_connectstrings.*

uninstall-oracle_database:
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_database.*

uninstall-oracle_userlist:
	$(RM) $(libexecdir)/$(SQLR)auth_oracle_userlist.*

uninstall-teradata_sidechannel:
	$(RM) $(libexecdir)/$(SQLR)auth_teradata_sidechannel.*

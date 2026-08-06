top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(DIRCPPFLAG) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WERROR) $(PLUGINCPPFLAGS) $(DIRCPPFLAG) $(COMPILE) $<

all: $(SQLR)pwdenc_rot.$(LIBEXT) \
	$(SQLR)pwdenc_md5.$(LIBEXT) \
	$(SQLR)pwdenc_sha1.$(LIBEXT) \
	$(SQLR)pwdenc_sha256.$(LIBEXT) \
	$(SQLR)pwdenc_des.$(LIBEXT) \
	$(SQLR)pwdenc_aes128.$(LIBEXT)

clean:
	$(RM) *.lo *.o *.obj *.$(LIBEXT) *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

$(SQLR)pwdenc_rot.$(LIBEXT): rot.cpp rot.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ rot.$(OBJ) $(LDFLAGS) $(UTILPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)pwdenc_md5.$(LIBEXT): md5.cpp md5.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ md5.$(OBJ) $(LDFLAGS) $(UTILPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)pwdenc_sha1.$(LIBEXT): sha1.cpp sha1.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sha1.$(OBJ) $(LDFLAGS) $(UTILPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)pwdenc_sha256.$(LIBEXT): sha256.cpp sha256.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ sha256.$(OBJ) $(LDFLAGS) $(UTILPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)pwdenc_des.$(LIBEXT): des.cpp des.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ des.$(OBJ) $(LDFLAGS) $(UTILPLUGINLIBS) $(MODLINKFLAGS)

$(SQLR)pwdenc_aes128.$(LIBEXT): aes128.cpp aes128.$(OBJ)
	$(LTLINK) $(LINK) $(OUT)$@ aes128.$(OBJ) $(LDFLAGS) $(UTILPLUGINLIBS) $(MODLINKFLAGS)

install: $(INSTALLLIB)

installdll: installdll-rot \
		installdll-md5 \
		installdll-sha1 \
		installdll-sha256 \
		installdll-des \
		installdll-aes128

installdll-rot:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_rot.$(LIBEXT) $(libexecdir)

installdll-md5:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_md5.$(LIBEXT) $(libexecdir)

installdll-sha1:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_sha1.$(LIBEXT) $(libexecdir)

installdll-sha256:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_sha256.$(LIBEXT) $(libexecdir)

installdll-des:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_des.$(LIBEXT) $(libexecdir)

installdll-aes128:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_aes128.$(LIBEXT) $(libexecdir)

installlib: $(INSTALLSHAREDLIB)

installsharedlib: installsharedlib-rot \
		installsharedlib-md5 \
		installsharedlib-sha1 \
		installsharedlib-sha256 \
		installsharedlib-des \
		installsharedlib-aes128

installsharedlib-rot:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_rot.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)pwdenc_rot.a
	$(RM) $(libexecdir)/$(SQLR)pwdenc_rot.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)pwdenc_rot.so so $(MODULESUFFIX)

installsharedlib-md5:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_md5.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)pwdenc_md5.a
	$(RM) $(libexecdir)/$(SQLR)pwdenc_md5.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)pwdenc_md5.so so $(MODULESUFFIX)

installsharedlib-sha1:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_sha1.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)pwdenc_sha1.a
	$(RM) $(libexecdir)/$(SQLR)pwdenc_sha1.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)pwdenc_sha1.so so $(MODULESUFFIX)

installsharedlib-sha256:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_sha256.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)pwdenc_sha256.a
	$(RM) $(libexecdir)/$(SQLR)pwdenc_sha256.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)pwdenc_sha256.so so $(MODULESUFFIX)

installsharedlib-des:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_des.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)pwdenc_des.a
	$(RM) $(libexecdir)/$(SQLR)pwdenc_des.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)pwdenc_des.so so $(MODULESUFFIX)

installsharedlib-aes128:
	$(MKINSTALLDIRS) $(libexecdir)
	$(LTINSTALL) $(CP) $(SQLR)pwdenc_aes128.$(LIBEXT) $(libexecdir)
	$(RM) $(libexecdir)/$(SQLR)pwdenc_aes128.a
	$(RM) $(libexecdir)/$(SQLR)pwdenc_aes128.$(LIBEXT)
	$(MODULERENAME) $(libexecdir)/$(SQLR)pwdenc_aes128.so so $(MODULESUFFIX)

uninstall: uninstall-rot \
		uninstall-md5 \
		uninstall-sha1 \
		uninstall-sha256 \
		uninstall-des \
		uninstall-aes128
	$(RM) $(libexecdir)/$(SQLR)pwdenc_crypt.* \
		$(libexecdir)/sqlrpwdenc_rot.* \
		$(libexecdir)/sqlrpwdenc_md5.* \
		$(libexecdir)/sqlrpwdenc_sha1.* \
		$(libexecdir)/sqlrpwdenc_sha256.* \
		$(libexecdir)/sqlrpwdenc_crypt.*

uninstall-rot:
	$(RM) $(libexecdir)/$(SQLR)pwdenc_rot.*

uninstall-md5:
	$(RM) $(libexecdir)/$(SQLR)pwdenc_md5.*

uninstall-sha1:
	$(RM) $(libexecdir)/$(SQLR)pwdenc_sha1.*

uninstall-sha256:
	$(RM) $(libexecdir)/$(SQLR)pwdenc_sha256.*

uninstall-des:
	$(RM) $(libexecdir)/$(SQLR)pwdenc_des.*

uninstall-aes128:
	$(RM) $(libexecdir)/$(SQLR)pwdenc_aes128.*

top_builddir = ../..

include ../../config.mk

VDPROJ   = setupx86.vdproj
SKEL     = setupx86.vdproj.skel
DAT      = setupx86-vdproj.dat
STAGE    = $(MAKEDIR)\stage
PRODROOT = $(STAGE)\C\Program Files\Firstworks

# .vdproj installers build only under devenv, and only vs 2010 handles .vdproj
# without an extension, so pin vs 2010's devenv (%VS100COMNTOOLS%), not PATH's
DEVENV = "$(VS100COMNTOOLS)..\IDE\devenv.com"

all: release

release: $(VDPROJ)
	$(DEVENV) setupx86.sln /build Release

debug: $(VDPROJ)
	$(DEVENV) setupx86.sln /build Debug

# stage a make install into an in-tree dir (DESTDIR is honored by cp.vbs and
# mkinstalldirs.vbs), then generate the .vdproj by walking it, so added/removed
# files, executables, libraries, modules and module types are picked up
# automatically.  the two setup-local files and the icon are copied into the
# staged tree so the generator can key the custom actions and the arp icon to
# them.  PERLINSTALLMAN= skips the perl man-page step (pod2man), which is not a
# file copy and is not part of the installer.
$(VDPROJ): $(SKEL) $(DAT) gen-vdproj.vbs
	-$(RMTREE) stage
	cd /d $(top_builddir) $(AND) set DESTDIR=$(STAGE)$(AND) $(MAKE) install PERLINSTALLMAN=
	$(MKINSTALLDIRS) "$(PRODROOT)\share\sqlrelay"
	$(CP) pathinstall.vbs "$(PRODROOT)\share\sqlrelay"
	$(CP) pathuninstall.vbs "$(PRODROOT)\share\sqlrelay"
	$(MKINSTALLDIRS) "$(PRODROOT)\doc"
	$(CP) $(top_builddir)\doc\images\sqlrelay-icon.ico "$(PRODROOT)\doc"
	cscript /nologo gen-vdproj.vbs $(SKEL) $(DAT) stage $(VDPROJ) $(SQLR_VERSION)

clean:
	-$(RMTREE) Release Debug stage
	-$(RM) $(VDPROJ)

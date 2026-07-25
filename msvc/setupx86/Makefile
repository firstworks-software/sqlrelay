top_builddir = ../..

include ../../config.mk

SETUP = setupx86
ARCH = x86
SLN = $(SETUP).sln
VDPROJ = $(SETUP).vdproj
SKEL = $(SETUP).vdproj.skel
DAT = $(SETUP)-vdproj.dat
MSI = sqlrelay-$(SQLR_VERSION)-$(ARCH).msi
STAGE = $(MAKEDIR)\stage
PRODROOT = $(STAGE)\C\Program Files\Firstworks
DEVENV = "$(VS100COMNTOOLS)..\IDE\devenv.com"

# scripts run as custom actions, moved into the msi's Binary table after the
# build so that deleting the installed copy can't break the uninstall (#8682)
CASCRIPTS = pathinstall.vbs pathuninstall.vbs

all: release

release: $(VDPROJ)
	$(DEVENV) $(SLN) /build Release
	cscript /nologo binaryca.vbs Release\$(MSI) $(CASCRIPTS)

debug: $(VDPROJ)
	$(DEVENV) $(SLN) /build Debug
	cscript /nologo binaryca.vbs Debug\$(MSI) $(CASCRIPTS)

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

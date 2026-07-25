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

all: release

release: $(VDPROJ)
	$(DEVENV) $(SLN) /build Release
	cscript /nologo binaryca.vbs Release\$(MSI)

debug: $(VDPROJ)
	$(DEVENV) $(SLN) /build Debug
	cscript /nologo binaryca.vbs Debug\$(MSI)

$(VDPROJ): $(SKEL) $(DAT) gen-vdproj.vbs
	-$(RMTREE) stage
	cd /d $(top_builddir) $(AND) set DESTDIR=$(STAGE)$(AND) $(MAKE) install PERLINSTALLMAN=
	$(MKINSTALLDIRS) "$(PRODROOT)\doc"
	$(CP) $(top_builddir)\doc\images\sqlrelay-icon.ico "$(PRODROOT)\doc"
	cscript /nologo gen-vdproj.vbs $(SKEL) $(DAT) stage $(VDPROJ) $(SQLR_VERSION)

clean:
	-$(RMTREE) Release Debug stage
	-$(RM) $(VDPROJ)

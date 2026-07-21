top_builddir = ../..

include ../../config.mk

SETUP = setupx86
SLN = $(SETUP).sln
VDPROJ = $(SETUP).vdproj
SKEL = $(SETUP).vdproj.skel
DAT = $(SETUP)-vdproj.dat
STAGE = $(MAKEDIR)\stage
PRODROOT = $(STAGE)\C\Program Files\Firstworks
DEVENV = "$(VS100COMNTOOLS)..\IDE\devenv.com"

all: release

release: $(VDPROJ)
	$(DEVENV) $(SLN) /build Release

debug: $(VDPROJ)
	$(DEVENV) $(SLN) /build Debug

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

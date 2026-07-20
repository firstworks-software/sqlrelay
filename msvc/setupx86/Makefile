top_builddir = ../..

include ../../config.mk

# .vdproj installers build only under devenv, and only vs 2010 handles .vdproj
# without an extension, so pin vs 2010's devenv (%VS100COMNTOOLS%), not PATH's
all:
	"$(VS100COMNTOOLS)..\IDE\devenv.com" setupx86.sln /build Release

clean:
	$(RMTREE) Release Debug

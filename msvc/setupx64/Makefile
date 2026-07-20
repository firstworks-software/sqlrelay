top_builddir = ../..

include ../../config.mk

# only devenv builds a .vdproj deployment project - nmake and msbuild cannot
all:
	devenv.com setupx64.sln /build Release

clean:
	$(RMTREE) Release Debug

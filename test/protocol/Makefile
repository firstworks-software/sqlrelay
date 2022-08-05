top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

all: all-mysql all-postgresql

all-mysql: 
	cd mysql $(AND) $(MAKE) all

all-postgresql:
	cd postgresql $(AND) $(MAKE) all

clean:
	cd mysql $(AND) $(MAKE) clean
	cd postgresql $(AND) $(MAKE) clean

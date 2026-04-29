top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(ODBCTESTCPPFLAGS) $(COMPILE) $<

all: oracle mysql postgresql

clean:
	$(RM) *.lo *.o *.obj oracle$(EXE) mysql$(EXE) postgresql$(EXE) \
		cachefile* sqlnet.log
	$(RMTREE) .libs

mysql: mysql.cpp mysql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ mysql.$(OBJ) $(ODBCTESTLIBS)

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ oracle.$(OBJ) $(ODBCTESTLIBS)

postgresql: postgresql.cpp postgresql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ postgresql.$(OBJ) $(ODBCTESTLIBS)

top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(POSTGRESQLINCLUDES) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(POSTGRESQLINCLUDES) $(COMPILE) $<

all: postgresql lob

postgresql: postgresql.cpp postgresql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) postgresql.$(OBJ) $(POSTGRESQLLIBS) $(RUDIMENTSLIBS)

lob: lob.cpp lob.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) lob.$(OBJ) $(POSTGRESQLLIBS) $(RUDIMENTSLIBS)

clean:
	$(RM) *.lo *.o *.obj *.class postgresql$(EXE) lob$(EXE)
	$(RMTREE) .libs

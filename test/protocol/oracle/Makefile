top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACLEINCLUDES) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(ORACEINCLUDES) $(COMPILE) $<

all: oracle ociselect oracle.class

oracle.class: oracle.java
	$(JAVAC) $<

oracle: oracle.cpp oracle.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracle.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

ociselect: ociselect.cpp ociselect.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) ociselect.$(OBJ) $(ORACLELIBS) $(RUDIMENTSLIBS)

clean:
	$(RM) *.lo *.o *.obj *.class oracle$(EXE) ociselect$(EXE)
	$(RMTREE) .libs

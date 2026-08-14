top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(COMPILE) $<

all: arc4test

# pure cipher-correctness unit test - firebirdarc4 has no firebird client
# library or database dependency, so this links against rudiments only, not
# CPPTESTLIBS (which pulls in sqlrclient)
arc4test: arc4test.cpp arc4test.$(OBJ) firebirdarc4.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) arc4test.$(OBJ) firebirdarc4.$(OBJ) $(RUDIMENTSLIBS)

# compiled straight out of src/protocols/ - this test exists to cover that
# file, not a copy of it
firebirdarc4.$(OBJ): ../../src/protocols/firebirdarc4.cpp \
			../../src/protocols/firebirdarc4.h
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) \
		$(COMPILE) ../../src/protocols/firebirdarc4.cpp $(OUT)$@

arc4test.$(OBJ): ../../src/protocols/firebirdarc4.h

clean:
	$(RM) *.lo *.o *.obj arc4test$(EXE)
	$(RMTREE) .libs

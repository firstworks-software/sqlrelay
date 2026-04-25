top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CPPTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CPPTESTCPPFLAGS) $(COMPILE) $<

all: db26 \
	db27 \
	oracle7 \
	oracle8

clean:
	$(RM) *.lo *.o *.obj db26$(EXE) db27$(EXE) oracle7$(EXE) oracle8$(EXE) *.csv cachefile* sqlnet.log
	$(RMTREE) .libs

db26: db26.cpp db26.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) db26.$(OBJ) $(CPPTESTLIBS)

db27: db27.cpp db27.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) db27.$(OBJ) $(CPPTESTLIBS)

oracle7: oracle7.cpp oracle7.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracle7.$(OBJ) $(CPPTESTLIBS)

oracle8: oracle8.cpp oracle8.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) oracle8.$(OBJ) $(CPPTESTLIBS)

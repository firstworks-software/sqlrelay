top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(MYSQLINCLUDES) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(CPPTESTCPPFLAGS) $(MYSQLINCLUDES) $(COMPILE) $<

all: mysql bind mysqldatetodatetime lob

mysql: mysql.cpp mysql.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysql.$(OBJ) $(MYSQLLIBS) $(RUDIMENTSLIBS)

bind: bind.cpp bind.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) bind.$(OBJ) $(MYSQLLIBS) $(RUDIMENTSLIBS)

mysqldatetodatetime: mysqldatetodatetime.cpp mysqldatetodatetime.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysqldatetodatetime.$(OBJ) $(MYSQLLIBS) $(RUDIMENTSLIBS)

lob: lob.cpp lob.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) lob.$(OBJ) $(MYSQLLIBS) $(RUDIMENTSLIBS)

clean:
	$(RM) *.lo *.o *.obj *.class mysql$(EXE) mysqldatetodatetime$(EXE) bind$(EXE) lob$(EXE)
	$(RMTREE) .libs

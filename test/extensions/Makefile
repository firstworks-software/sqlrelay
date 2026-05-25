top_builddir = ../..

include ../../config.mk

.SUFFIXES: .lo .obj .cpp

.cpp.lo:
	$(LTCOMPILE) $(CXX) $(CXXFLAGS) $(CPPTESTCPPFLAGS) $(COMPILE) $< $(OUT)$@

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CPPTESTCPPFLAGS) $(COMPILE) $<

all: router \
	extensions \
	mysqlupsert \
	mysqldeadlockreplay \
	mysqlsplitmultiinsert \
	mysqlimportexport \
	emoji \
	postgresqlupsert \
	postgresqlsavepoint \
	postgresqlglobaltemptables \
	postgresqlstringagg

clean:
	$(RM) *.lo *.o *.obj router$(EXE) extensions$(EXE) mysqldeadlockreplay$(EXE) mysqlsplitmultiinsert$(EXE) mysqlupsert$(EXE) mysqlexport$(EXE) mysqlimportexport$(EXE) emoji$(EXE) postgresqlupsert$(EXE) postgresqlsavepoint$(EXE) postgresqlglobaltemptables$(EXE) postgresqlstringagg$(EXE) *.csv cachefile* sqlnet.log
	$(RMTREE) .libs

router: router.cpp router.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) router.$(OBJ) $(CPPTESTLIBS)

extensions: extensions.cpp extensions.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) extensions.$(OBJ) $(CPPTESTLIBS)

mysqlupsert: mysqlupsert.cpp mysqlupsert.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysqlupsert.$(OBJ) $(CPPTESTLIBS)

mysqldeadlockreplay: mysqldeadlockreplay.cpp mysqldeadlockreplay.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysqldeadlockreplay.$(OBJ) $(CPPTESTLIBS)

mysqlsplitmultiinsert: mysqlsplitmultiinsert.cpp mysqlsplitmultiinsert.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysqlsplitmultiinsert.$(OBJ) $(CPPTESTLIBS)

mysqlimportexport: mysqlimportexport.cpp mysqlimportexport.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) mysqlimportexport.$(OBJ) $(CPPTESTLIBS)

emoji: emoji.cpp emoji.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) emoji.$(OBJ) $(CPPTESTLIBS)

postgresqlupsert: postgresqlupsert.cpp postgresqlupsert.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) postgresqlupsert.$(OBJ) $(CPPTESTLIBS)

postgresqlsavepoint: postgresqlsavepoint.cpp postgresqlsavepoint.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) postgresqlsavepoint.$(OBJ) $(CPPTESTLIBS)

postgresqlglobaltemptables: postgresqlglobaltemptables.cpp postgresqlglobaltemptables.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) postgresqlglobaltemptables.$(OBJ) $(CPPTESTLIBS)

postgresqlstringagg: postgresqlstringagg.cpp postgresqlstringagg.$(OBJ)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@$(EXE) postgresqlstringagg.$(OBJ) $(CPPTESTLIBS)

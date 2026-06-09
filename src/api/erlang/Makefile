top_builddir = ../../..

include ../../../config.mk

SRCS = erl_comm.c relay_driver.c
LOBJS = erl_comm.$(OBJ) relay_driver.$(OBJ)
 
.SUFFIXES: .lo .obj .erl .beam

.c.lo:
	$(LTCOMPILE) $(CC) $(CCFLAGS) $(ERLANGCPPFLAGS) $(COMPILE) $< $(OUT)$@

.c.obj:
	$(LTCOMPILE) $(CC) $(CCFLAGS) $(ERLANGCPPFLAGS) $(COMPILE) $<

all: sqlrelay.beam sqlrelay

clean:
	$(RM) *.lo *.o *.obj sqlrelay.erl sqlrelay.erl.out1 sqlrelay *.beam *.lib *.exp *.idb *.pdb *.manifest *.ii
	$(RMTREE) .libs

sqlrelay.erl: sqlrelay.erl.in
	$(REPLACE) @SQLRELAY_PATH@ "$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/priv/bin" $< > $@.out1
	$(MV) $@.out1 $@

sqlrelay.beam: sqlrelay.erl
	$(ERLC) $(ERLCFLAGS) -W $<

sqlrelay: $(SRCS) $(LOBJS)
	$(LTLINK) $(LINK) $(LDFLAGS) $(OUT)$@ $(LOBJS) $(ERLANGLIBS)

install:
	$(MKINSTALLDIRS) $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/src
	$(MKINSTALLDIRS) $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/ebin
	$(MKINSTALLDIRS) $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/priv/bin
	$(LTINSTALL) $(CP) sqlrelay.erl $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/src
	$(CHMOD) 644 $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/src/sqlrelay.erl
	$(LTINSTALL) $(CP) sqlrelay.beam $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/ebin
	$(CHMOD) 644 $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/ebin/sqlrelay.beam
	$(LTINSTALL) $(CP) sqlrelay $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)/priv/bin

uninstall:
	$(RMTREE) $(DESTDIR)$(ERLANG_ROOT_DIR)/lib/sqlrelay-$(SQLR_VERSION)

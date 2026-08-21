top_builddir = ../..

include ../../config.mk

all: asserts.beam \
	db2.beam \
	firebird.beam \
	freetds-mssql.beam \
	freetds-sap.beam \
	informix.beam \
	krb.beam \
	mysql.beam \
	oracle.beam \
	postgresql.beam \
	sap.beam \
	sqlite.beam \
	tls.beam

asserts.beam: asserts.erl
	$(ERLC) asserts.erl

db2.beam: db2.erl asserts.beam
	$(ERLC) db2.erl

firebird.beam: firebird.erl asserts.beam
	$(ERLC) firebird.erl

freetds-mssql.beam: freetds-mssql.erl asserts.beam
	$(ERLC) freetds-mssql.erl

freetds-sap.beam: freetds-sap.erl asserts.beam
	$(ERLC) freetds-sap.erl

informix.beam: informix.erl asserts.beam
	$(ERLC) informix.erl

krb.beam: krb.erl asserts.beam
	$(ERLC) krb.erl

mysql.beam: mysql.erl asserts.beam
	$(ERLC) mysql.erl

oracle.beam: oracle.erl asserts.beam
	$(ERLC) oracle.erl

postgresql.beam: postgresql.erl asserts.beam
	$(ERLC) postgresql.erl

sap.beam: sap.erl asserts.beam
	$(ERLC) sap.erl

sqlite.beam: sqlite.erl asserts.beam
	$(ERLC) sqlite.erl

tls.beam: tls.erl asserts.beam
	$(ERLC) tls.erl

clean:
	$(RM) *.beam cachefile* sqlnet.log erl_crash.dump

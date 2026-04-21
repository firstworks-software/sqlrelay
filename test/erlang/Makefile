top_builddir = ../..

include ../../config.mk

TESTS = db2 firebird freetds informix krb mysql oracle postgresql sap sqlite tls
BEAMS = $(TESTS:=.beam)

all: asserts.beam $(BEAMS)

asserts.beam: asserts.erl
	$(ERLC) asserts.erl

%.beam: %.erl asserts.beam
	$(ERLC) $<

clean:
	$(RM) *.beam cachefile* sqlnet.log erl_crash.dump

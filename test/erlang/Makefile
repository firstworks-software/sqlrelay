top_builddir = ../..

include ../../config.mk

all: asserts.beam oracle.beam

asserts.beam: asserts.erl
	$(ERLC) asserts.erl

oracle.beam: oracle.erl asserts.beam
	$(ERLC) oracle.erl

clean:
	$(RM) *.beam cachefile* sqlnet.log erl_crash.dump

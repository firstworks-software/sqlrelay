top_builddir = ../..

include ../../config.mk

all: oracle.beam

oracle.beam: oracle.erl
	erlc oracle.erl

clean:
	$(RM) *.beam cachefile* sqlnet.log erl_crash.dump

top_builddir = ../..

include ../../config.mk

.SUFFIXES: .wt .html

SRCS = connectionpooling.wt \
	backends.wt \
	datatranslation.wt \
	dbabstractionlayers.wt \
	ha.wt \
	languages.wt \
	multirowfetches.wt \
	protocols.wt \
	proxying.wt \
	queryfiltering.wt \
	queryrouting.wt \
	resultsetcaching.wt \
	schedules.wt \
	substitutionandbindvars.wt \
	suspendedtx.wt \
	throttling.wt

OBJS = connectionpooling.html \
	backends.html \
	datatranslation.html \
	dbabstractionlayers.html \
	ha.html \
	languages.html \
	multirowfetches.html \
	protocols.html \
	proxying.html \
	queryfiltering.html \
	queryrouting.html \
	resultsetcaching.html \
	schedules.html \
	substitutionandbindvars.html \
	suspendedtx.html \
	throttling.html

.wt.html:
	wt -i $< -o $@

all: $(OBJS)
	$(CHMOD) 755 *.html

clean:
	$(RM) $(OBJS)

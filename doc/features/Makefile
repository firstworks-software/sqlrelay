top_builddir = ../..

include ../../config.mk

.SUFFIXES: .wt .html

SRCS = connectionpooling.wt \
	backends.wt \
	datatranslation.wt \
	dbabstractionlayers.wt \
	frontends.wt \
	lbfoha.wt \
	nativeapi.wt \
	platforms.wt \
	queryrouting.wt \
	security.wt \

OBJS = connectionpooling.html \
	backends.html \
	datatranslation.html \
	dbabstractionlayers.html \
	frontends.html \
	lbfoha.html \
	nativeapi.html \
	platforms.html \
	queryrouting.html \
	security.html

.wt.html:
	wt -i $< -o $@

all: $(OBJS)
	$(CHMOD) 755 *.html

clean:
	$(RM) $(OBJS)

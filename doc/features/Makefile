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
	notifications.wt \
	platforms.wt \
	routing.wt \
	security.wt \
	throttling.wt \
	triggers.wt

OBJS = connectionpooling.html \
	backends.html \
	datatranslation.html \
	dbabstractionlayers.html \
	frontends.html \
	lbfoha.html \
	nativeapi.html \
	notifications.html \
	platforms.html \
	routing.html \
	security.html \
	throttling.html \
	triggers.html

.wt.html:
	wt -i $< -o $@

all: $(OBJS)
	$(CHMOD) 755 *.html

clean:
	$(RM) $(OBJS)

top_builddir = ../..

include ../../config.mk

.SUFFIXES: .wt .html.in .html

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

INS = backends.html.in \
	connectionpooling.html.in \
	datatranslation.html.in \
	dbabstractionlayers.html.in \
	frontends.html.in \
	header.html.in \
	lbfoha.html.in \
	nativeapi.html.in \
	notifications.html.in \
	platforms.html.in \
	routing.html.in \
	security.html.in \
	throttling.html.in \
	triggers.html.in

OBJS = backends.html \
	connectionpooling.html \
	datatranslation.html \
	dbabstractionlayers.html \
	frontends.html \
	header.html \
	lbfoha.html \
	nativeapi.html \
	notifications.html \
	platforms.html \
	routing.html \
	security.html \
	throttling.html \
	triggers.html

# fragments the pages here include; editing one rebuilds all of them
FRAGMENTS = ../contentend.html \
	../contentstart.html \
	../title.html \
	../tracker.html \
	footer.html \
	header.html.in \
	projectheader.html

.PRECIOUS: $(INS)

.wt.html.in:
	wt -i $< -o $@

.html.in.html:
	$(SSI) $<

all: $(OBJS)
	$(CHMOD) 644 $(OBJS)

$(OBJS): $(FRAGMENTS)

clean:
	$(RM) $(OBJS)

top_builddir = ../..

include ../../config.mk

.SUFFIXES: .wt .html.in .html

SRCS = configguide.wt \
	configreference.wt \
	installing.wt \
	installingpkg.wt \
	oraclentier.wt \
	prerequisites.wt \
	running.wt \
	sqlrsh.wt \
	tuning.wt \
	tlscert.wt \
	tlsca.wt \
	createtlscert.wt

INS = configguide.html.in \
	configreference.html.in \
	createtlscert.html.in \
	header.html.in \
	installing.html.in \
	installingpkg.html.in \
	oraclentier.html.in \
	prerequisites.html.in \
	running.html.in \
	sqlrsh.html.in \
	tlsca.html.in \
	tlscert.html.in \
	tuning.html.in

# the subset of INS that wt generates from SRCS; the rest are checkout files
WTOBJS = configguide.html.in \
	configreference.html.in \
	createtlscert.html.in \
	installing.html.in \
	installingpkg.html.in \
	oraclentier.html.in \
	prerequisites.html.in \
	running.html.in \
	sqlrsh.html.in \
	tlsca.html.in \
	tlscert.html.in \
	tuning.html.in

OBJS = configguide.html \
	configreference.html \
	createtlscert.html \
	header.html \
	installing.html \
	installingpkg.html \
	oraclentier.html \
	prerequisites.html \
	running.html \
	sqlrsh.html \
	tlsca.html \
	tlscert.html \
	tuning.html

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
	$(RM) $(OBJS) $(WTOBJS)

top_builddir = ../../..

include ../../../config.mk

.SUFFIXES: .wt .html.in .html

SRCS = index.wt \
	quickstart.wt \
	basicconfiguration.wt \
	clientprotocoloptions.wt \
	sqlrclientprotocol.wt \
	mysqlprotocol.wt \
	postgresqlprotocol.wt \
	firebirdprotocol.wt \
	tdsprotocol.wt \
	oracleprotocol.wt \
	multipleprotocols.wt \
	highavailability.wt \
	security.wt \
	datatranslation.wt \
	querydirectives.wt \
	routing.wt \
	customqueries.wt \
	triggers.wt \
	logging.wt \
	notifications.wt \
	sessionqueries.wt \
	altconfigfile.wt \
	advanced.wt

INS = index.html.in \
	quickstart.html.in \
	basicconfiguration.html.in \
	clientprotocoloptions.html.in \
	sqlrclientprotocol.html.in \
	mysqlprotocol.html.in \
	postgresqlprotocol.html.in \
	firebirdprotocol.html.in \
	tdsprotocol.html.in \
	oracleprotocol.html.in \
	multipleprotocols.html.in \
	highavailability.html.in \
	security.html.in \
	datatranslation.html.in \
	querydirectives.html.in \
	routing.html.in \
	customqueries.html.in \
	triggers.html.in \
	logging.html.in \
	notifications.html.in \
	sessionqueries.html.in \
	altconfigfile.html.in \
	advanced.html.in \
	header.html.in

# the subset of INS that wt generates from SRCS; the rest are checkout files
WTOBJS = index.html.in \
	quickstart.html.in \
	basicconfiguration.html.in \
	clientprotocoloptions.html.in \
	sqlrclientprotocol.html.in \
	mysqlprotocol.html.in \
	postgresqlprotocol.html.in \
	firebirdprotocol.html.in \
	tdsprotocol.html.in \
	oracleprotocol.html.in \
	multipleprotocols.html.in \
	highavailability.html.in \
	security.html.in \
	datatranslation.html.in \
	querydirectives.html.in \
	routing.html.in \
	customqueries.html.in \
	triggers.html.in \
	logging.html.in \
	notifications.html.in \
	sessionqueries.html.in \
	altconfigfile.html.in \
	advanced.html.in

OBJS = index.html \
	quickstart.html \
	basicconfiguration.html \
	clientprotocoloptions.html \
	sqlrclientprotocol.html \
	mysqlprotocol.html \
	postgresqlprotocol.html \
	firebirdprotocol.html \
	tdsprotocol.html \
	oracleprotocol.html \
	multipleprotocols.html \
	highavailability.html \
	security.html \
	datatranslation.html \
	querydirectives.html \
	routing.html \
	customqueries.html \
	triggers.html \
	logging.html \
	notifications.html \
	sessionqueries.html \
	altconfigfile.html \
	advanced.html \
	header.html

# fragments the pages here include; editing one rebuilds all of them
FRAGMENTS = ../../contentend.html \
	../../contentstart.html \
	../../title.html \
	../../tracker.html \
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

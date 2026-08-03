top_builddir = ../..

include ../../config.mk

.SUFFIXES: .wt .html.in .html

SRCS = ado.net.wt \
	c.wt \
	c++.wt \
	cs.wt \
	binds.wt \
	erlang.wt \
	java.wt \
	jdbc.wt \
	nodejs.wt \
	odbc.wt \
	perldbi.wt \
	perl.wt \
	php.wt \
	phppdo.wt \
	precisionscale.wt \
	pythondb.wt \
	python.wt \
	ruby.wt \
	tcl.wt

INS = ado.net.html.in \
	binds.html.in \
	c++.html.in \
	c.html.in \
	cs.html.in \
	erlang.html.in \
	header.html.in \
	java.html.in \
	jdbc.html.in \
	nodejs.html.in \
	odbc.html.in \
	perldbi.html.in \
	perl.html.in \
	php.html.in \
	phppdo.html.in \
	precisionscale.html.in \
	pythondb.html.in \
	python.html.in \
	ruby.html.in \
	tcl.html.in

OBJS = ado.net.html \
	binds.html \
	c++.html \
	c.html \
	cs.html \
	erlang.html \
	header.html \
	java.html \
	jdbc.html \
	nodejs.html \
	odbc.html \
	perldbi.html \
	perl.html \
	php.html \
	phppdo.html \
	precisionscale.html \
	pythondb.html \
	python.html \
	ruby.html \
	tcl.html

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
	$(CHMOD) 755 *.html

$(OBJS): $(FRAGMENTS)

clean:
	$(RM) $(OBJS)

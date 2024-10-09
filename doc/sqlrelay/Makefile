top_builddir = ../..

include ../../config.mk

.SUFFIXES: .wt .html

SRCS = index.wt faq.wt
OBJS = index.html faq.html

.wt.html:
	wt -i $< -o $@ -xhtml xhtml.conf

all: $(OBJS)
	cd admin $(AND) $(MAKE)
	cd features $(AND) $(MAKE)
	cd programming $(AND) $(MAKE)

install:
	$(MKINSTALLDIRS) $(docdir)/sqlrelay
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/admin
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/ado.net/html
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/c/html
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/c++/html
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/cs/html
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/perl
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/php/media
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/php/sql_relay
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/python
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/ruby/doc/css
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/ruby/doc/fonts
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/ruby/doc/images
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/ruby/doc/js
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/java
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/java/com/firstworks/sqlrelay
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/java/jquery/external/jquery
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/java/jquery/images
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/java/jquery/jszip/dist
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/java/jquery/jszip-utils/dist
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/java/resources
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/tcl
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/erlang/html
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/api/nodejs/html
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/features
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/images
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/js
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/css
	$(MKINSTALLDIRS) $(docdir)/sqlrelay/programming
	$(CP) *.html $(docdir)/sqlrelay
	$(CHMOD) 644 $(docdir)/sqlrelay/*.html
	$(CP) admin/*.html $(docdir)/sqlrelay/admin
	$(CHMOD) 644 $(docdir)/sqlrelay/admin/*.html
	$(CP) css/*.css $(docdir)/sqlrelay/css
	$(CHMOD) 644 $(docdir)/sqlrelay/css/*.css
	$(CP) api/ado.net/html/*.html $(docdir)/sqlrelay/api/ado.net/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ado.net/html/*.html
	$(CP) api/ado.net/html/*.css $(docdir)/sqlrelay/api/ado.net/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ado.net/html/*.css
	$(CP) api/ado.net/html/*.png $(docdir)/sqlrelay/api/ado.net/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ado.net/html/*.png
	$(CP) api/ado.net/html/*.svg $(docdir)/sqlrelay/api/ado.net/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ado.net/html/*.svg
	$(CP) api/ado.net/html/*.js $(docdir)/sqlrelay/api/ado.net/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ado.net/html/*.js
	$(CP) api/c/html/*.html $(docdir)/sqlrelay/api/c/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c/html/*.html
	$(CP) api/c/html/*.css $(docdir)/sqlrelay/api/c/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c/html/*.css
	$(CP) api/c/html/*.png $(docdir)/sqlrelay/api/c/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c/html/*.png
	$(CP) api/c/html/*.svg $(docdir)/sqlrelay/api/c/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c/html/*.svg
	$(CP) api/c/html/*.js $(docdir)/sqlrelay/api/c/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c/html/*.js
	$(CP) api/c++/html/*.html $(docdir)/sqlrelay/api/c++/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c++/html/*.html
	$(CP) api/c++/html/*.css $(docdir)/sqlrelay/api/c++/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c++/html/*.css
	$(CP) api/c++/html/*.png $(docdir)/sqlrelay/api/c++/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c++/html/*.png
	$(CP) api/c++/html/*.svg $(docdir)/sqlrelay/api/c++/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c++/html/*.svg
	$(CP) api/c++/html/*.js $(docdir)/sqlrelay/api/c++/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/c++/html/*.js
	$(CP) api/cs/html/*.html $(docdir)/sqlrelay/api/cs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/cs/html/*.html
	$(CP) api/cs/html/*.css $(docdir)/sqlrelay/api/cs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/cs/html/*.css
	$(CP) api/cs/html/*.png $(docdir)/sqlrelay/api/cs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/cs/html/*.png
	$(CP) api/cs/html/*.svg $(docdir)/sqlrelay/api/cs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/cs/html/*.svg
	$(CP) api/cs/html/*.js $(docdir)/sqlrelay/api/cs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/cs/html/*.js
	$(CP) api/perl/perl.html $(docdir)/sqlrelay/api/perl
	$(CHMOD) 644 $(docdir)/sqlrelay/api/perl/perl.html
	$(CP) api/perl/Connection.html $(docdir)/sqlrelay/api/perl
	$(CHMOD) 644 $(docdir)/sqlrelay/api/perl/Connection.html
	$(CP) api/perl/Cursor.html $(docdir)/sqlrelay/api/perl
	$(CHMOD) 644 $(docdir)/sqlrelay/api/perl/Cursor.html
	$(CP) api/perl/SQLRelay.html $(docdir)/sqlrelay/api/perl
	$(CHMOD) 644 $(docdir)/sqlrelay/api/perl/SQLRelay.html
	$(CP) api/php/*.html $(docdir)/sqlrelay/api/php
	$(CHMOD) 644 $(docdir)/sqlrelay/api/php/*.html
	$(CP) api/php/media/*.css $(docdir)/sqlrelay/api/php/media
	$(CHMOD) 644 $(docdir)/sqlrelay/api/php/media/*.css
	$(CP) api/php/sql_relay/*.html $(docdir)/sqlrelay/api/php/sql_relay
	$(CHMOD) 644 $(docdir)/sqlrelay/api/php/sql_relay/*.html
	$(CP) api/python/*.html $(docdir)/sqlrelay/api/python
	$(CHMOD) 644 $(docdir)/sqlrelay/api/python/*.html
	$(CP) api/ruby/doc/*.html $(docdir)/sqlrelay/api/ruby/doc
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ruby/doc/*.html
	$(CP) api/ruby/doc/*.rid $(docdir)/sqlrelay/api/ruby/doc
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ruby/doc/*.rid
	$(CP) api/ruby/doc/css/*.css $(docdir)/sqlrelay/api/ruby/doc/css
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ruby/doc/css/*.css
	$(CP) api/ruby/doc/images/*.png $(docdir)/sqlrelay/api/ruby/doc/images
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ruby/doc/images/*.png
	$(CP) api/ruby/doc/images/*.gif $(docdir)/sqlrelay/api/ruby/doc/images
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ruby/doc/images/*.gif
	$(CP) api/ruby/doc/js/*.js $(docdir)/sqlrelay/api/ruby/doc/js
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ruby/doc/js/*.js
	$(CP) api/ruby/doc/js/*.js.gz $(docdir)/sqlrelay/api/ruby/doc/js
	$(CHMOD) 644 $(docdir)/sqlrelay/api/ruby/doc/js/*.js.gz
	$(CP) api/java/*.html $(docdir)/sqlrelay/api/java
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/*.html
	$(CP) api/java/*.js $(docdir)/sqlrelay/api/java
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/*.js
	$(CP) api/java/*.zip $(docdir)/sqlrelay/api/java
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/*.zip
	$(CP) api/java/*.js $(docdir)/sqlrelay/api/java
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/*.js
	$(CP) api/java/*.css $(docdir)/sqlrelay/api/java
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/*.css
	$(CP) api/java/element-list $(docdir)/sqlrelay/api/java
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/element-list
	$(CP) api/java/com/firstworks/sqlrelay/*.html $(docdir)/sqlrelay/api/java/com/firstworks/sqlrelay
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/com/firstworks/sqlrelay/*.html
	$(CP) api/java/resources/*.gif $(docdir)/sqlrelay/api/java/resources
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/resources/*.gif
	$(CP) api/java/resources/*.png $(docdir)/sqlrelay/api/java/resources
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/resources/*.png
	$(CP) api/java/jquery/*.js $(docdir)/sqlrelay/api/java/jquery
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/jquery/*.js
	$(CP) api/java/jquery/*.css $(docdir)/sqlrelay/api/java/jquery
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/jquery/*.css
	$(CP) api/java/jquery/external/jquery/*.js $(docdir)/sqlrelay/api/java/jquery/external/jquery
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/jquery/external/jquery/*.js
	$(CP) api/java/jquery/images/*.png $(docdir)/sqlrelay/api/java/jquery/images
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/jquery/images/*.png
	$(CP) api/java/jquery/jszip/dist/*.js $(docdir)/sqlrelay/api/java/jquery/jszip/dist
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/jquery/jszip/dist/*.js
	$(CP) api/java/jquery/jszip-utils/dist/*.js $(docdir)/sqlrelay/api/java/jquery/jszip-utils/dist
	$(CHMOD) 644 $(docdir)/sqlrelay/api/java/jquery/jszip-utils/dist/*.js
	$(CP) api/tcl/*.html $(docdir)/sqlrelay/api/tcl
	$(CHMOD) 644 $(docdir)/sqlrelay/api/tcl/*.html
	$(CP) api/tcl/*.txt $(docdir)/sqlrelay/api/tcl
	$(CHMOD) 644 $(docdir)/sqlrelay/api/tcl/*.txt
	$(CP) api/erlang/html/edoc-info $(docdir)/sqlrelay/api/erlang/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/erlang/html/edoc-info
	$(CP) api/erlang/html/*.png $(docdir)/sqlrelay/api/erlang/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/erlang/html/*.png
	$(CP) api/erlang/html/*.html $(docdir)/sqlrelay/api/erlang/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/erlang/html/*.html
	$(CP) api/erlang/html/*.css $(docdir)/sqlrelay/api/erlang/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/erlang/html/*.css
	$(CP) api/nodejs/html/*.html $(docdir)/sqlrelay/api/nodejs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/nodejs/html/*.html
	$(CP) api/nodejs/html/*.css $(docdir)/sqlrelay/api/nodejs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/nodejs/html/*.css
	$(CP) api/nodejs/html/*.png $(docdir)/sqlrelay/api/nodejs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/nodejs/html/*.png
	$(CP) api/nodejs/html/*.svg $(docdir)/sqlrelay/api/nodejs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/nodejs/html/*.svg
	$(CP) api/nodejs/html/*.js $(docdir)/sqlrelay/api/nodejs/html
	$(CHMOD) 644 $(docdir)/sqlrelay/api/nodejs/html/*.js
	$(CP) features/*.html $(docdir)/sqlrelay/features
	$(CHMOD) 644 $(docdir)/sqlrelay/features/*.html
	$(CP) images/*.png $(docdir)/sqlrelay/images
	$(CHMOD) 644 $(docdir)/sqlrelay/images/*.png
	$(CP) images/*.PNG $(docdir)/sqlrelay/images
	$(CHMOD) 644 $(docdir)/sqlrelay/images/*.PNG
	$(CP) js/*.js $(docdir)/sqlrelay/js
	$(CHMOD) 644 $(docdir)/sqlrelay/js/*.js
	$(CP) programming/*.html $(docdir)/sqlrelay/programming
	$(CHMOD) 644 $(docdir)/sqlrelay/programming/*.html

clean:
	$(RM) $(OBJS)
	cd admin $(AND) $(MAKE) clean
	cd features $(AND) $(MAKE) clean
	cd programming $(AND) $(MAKE) clean
	cd howtos $(AND) $(MAKE) clean

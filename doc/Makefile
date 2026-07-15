top_builddir = ..

include ../config.mk

.SUFFIXES: .wt .html

SRCS = documentation.wt faq.wt
OBJS = documentation.html faq.html

.wt.html:
	wt -i $< -o $@

all: $(OBJS)
	$(CHMOD) 755 $(OBJS)
	cd admin $(AND) $(MAKE)
	cd gettingstarted $(AND) $(MAKE)
	cd features $(AND) $(MAKE)
	cd programming $(AND) $(MAKE)

install:
	$(MKINSTALLDIRS) $(docdir)
	$(CP) .htaccess $(docdir)
	$(CHMOD) 755 $(docdir)/.htaccess
	$(CP) *.html $(docdir)
	$(CHMOD) 755 $(docdir)/*.html
	$(MKINSTALLDIRS) $(docdir)/css
	$(CP) css/* $(docdir)/css
	$(CHMOD) 644 $(docdir)/css/*
	$(MKINSTALLDIRS) $(docdir)/images
	$(CP) images/*.jpg $(docdir)/images
	$(CP) images/*.png $(docdir)/images
	$(CP) images/*.PNG $(docdir)/images
	$(CP) images/*.ico $(docdir)/images
	$(CHMOD) 644 $(docdir)/images/*
	$(MKINSTALLDIRS) $(docdir)/js
	$(CP) js/* $(docdir)/js
	$(CHMOD) 644 $(docdir)/js/*
	$(MKINSTALLDIRS) $(docdir)/licenses
	$(CP) licenses/* $(docdir)/licenses
	$(CHMOD) 644 $(docdir)/licenses/*
	$(MKINSTALLDIRS) $(docdir)/presentation
	$(CP) presentation/* $(docdir)/presentation
	$(CHMOD) 755 $(docdir)/presentation/*
	$(MKINSTALLDIRS) $(docdir)/slides
	$(CP) slides/* $(docdir)/slides
	$(CHMOD) 755 $(docdir)/slides/*
	$(MKINSTALLDIRS) $(docdir)/admin
	$(MKINSTALLDIRS) $(docdir)/gettingstarted
	$(MKINSTALLDIRS) $(docdir)/api
	$(MKINSTALLDIRS) $(docdir)/api/ado.net/html
	$(MKINSTALLDIRS) $(docdir)/api/c/html
	$(MKINSTALLDIRS) $(docdir)/api/c++/html
	$(MKINSTALLDIRS) $(docdir)/api/cs/html
	$(MKINSTALLDIRS) $(docdir)/api/perl
	$(MKINSTALLDIRS) $(docdir)/api/php/html
	$(MKINSTALLDIRS) $(docdir)/api/python
	$(MKINSTALLDIRS) $(docdir)/api/ruby/doc/css
	$(MKINSTALLDIRS) $(docdir)/api/ruby/doc/fonts
	$(MKINSTALLDIRS) $(docdir)/api/ruby/doc/images
	$(MKINSTALLDIRS) $(docdir)/api/ruby/doc/js
	$(MKINSTALLDIRS) $(docdir)/api/java
	$(MKINSTALLDIRS) $(docdir)/api/java/com/firstworks/sqlrelay
	$(MKINSTALLDIRS) $(docdir)/api/java/jquery/external/jquery
	$(MKINSTALLDIRS) $(docdir)/api/java/jquery/images
	$(MKINSTALLDIRS) $(docdir)/api/java/jquery/jszip/dist
	$(MKINSTALLDIRS) $(docdir)/api/java/jquery/jszip-utils/dist
	$(MKINSTALLDIRS) $(docdir)/api/java/resources
	$(MKINSTALLDIRS) $(docdir)/api/tcl
	$(MKINSTALLDIRS) $(docdir)/api/erlang/html
	$(MKINSTALLDIRS) $(docdir)/api/nodejs/html
	$(MKINSTALLDIRS) $(docdir)/features
	$(MKINSTALLDIRS) $(docdir)/images
	$(MKINSTALLDIRS) $(docdir)/js
	$(MKINSTALLDIRS) $(docdir)/css
	$(MKINSTALLDIRS) $(docdir)/programming
	$(CP) admin/*.html $(docdir)/admin
	$(CHMOD) 755 $(docdir)/admin/*.html
	$(CP) gettingstarted/*.html $(docdir)/gettingstarted
	$(CHMOD) 755 $(docdir)/gettingstarted/*.html
	$(CP) css/*.css $(docdir)/css
	$(CHMOD) 644 $(docdir)/css/*.css
	$(CP) api/ado.net/html/*.html $(docdir)/api/ado.net/html
	$(CHMOD) 644 $(docdir)/api/ado.net/html/*.html
	$(CP) api/ado.net/html/*.css $(docdir)/api/ado.net/html
	$(CHMOD) 644 $(docdir)/api/ado.net/html/*.css
	$(CP) api/ado.net/html/*.png $(docdir)/api/ado.net/html
	$(CHMOD) 644 $(docdir)/api/ado.net/html/*.png
	$(CP) api/ado.net/html/*.svg $(docdir)/api/ado.net/html
	$(CHMOD) 644 $(docdir)/api/ado.net/html/*.svg
	$(CP) api/ado.net/html/*.js $(docdir)/api/ado.net/html
	$(CHMOD) 644 $(docdir)/api/ado.net/html/*.js
	$(CP) api/c/html/*.html $(docdir)/api/c/html
	$(CHMOD) 644 $(docdir)/api/c/html/*.html
	$(CP) api/c/html/*.css $(docdir)/api/c/html
	$(CHMOD) 644 $(docdir)/api/c/html/*.css
	$(CP) api/c/html/*.png $(docdir)/api/c/html
	$(CHMOD) 644 $(docdir)/api/c/html/*.png
	$(CP) api/c/html/*.svg $(docdir)/api/c/html
	$(CHMOD) 644 $(docdir)/api/c/html/*.svg
	$(CP) api/c/html/*.js $(docdir)/api/c/html
	$(CHMOD) 644 $(docdir)/api/c/html/*.js
	$(CP) api/c++/html/*.html $(docdir)/api/c++/html
	$(CHMOD) 644 $(docdir)/api/c++/html/*.html
	$(CP) api/c++/html/*.css $(docdir)/api/c++/html
	$(CHMOD) 644 $(docdir)/api/c++/html/*.css
	$(CP) api/c++/html/*.png $(docdir)/api/c++/html
	$(CHMOD) 644 $(docdir)/api/c++/html/*.png
	$(CP) api/c++/html/*.svg $(docdir)/api/c++/html
	$(CHMOD) 644 $(docdir)/api/c++/html/*.svg
	$(CP) api/c++/html/*.js $(docdir)/api/c++/html
	$(CHMOD) 644 $(docdir)/api/c++/html/*.js
	$(CP) api/cs/html/*.html $(docdir)/api/cs/html
	$(CHMOD) 644 $(docdir)/api/cs/html/*.html
	$(CP) api/cs/html/*.css $(docdir)/api/cs/html
	$(CHMOD) 644 $(docdir)/api/cs/html/*.css
	$(CP) api/cs/html/*.png $(docdir)/api/cs/html
	$(CHMOD) 644 $(docdir)/api/cs/html/*.png
	$(CP) api/cs/html/*.svg $(docdir)/api/cs/html
	$(CHMOD) 644 $(docdir)/api/cs/html/*.svg
	$(CP) api/cs/html/*.js $(docdir)/api/cs/html
	$(CHMOD) 644 $(docdir)/api/cs/html/*.js
	$(CP) api/perl/perl.html $(docdir)/api/perl
	$(CHMOD) 644 $(docdir)/api/perl/perl.html
	$(CP) api/perl/Connection.html $(docdir)/api/perl
	$(CHMOD) 644 $(docdir)/api/perl/Connection.html
	$(CP) api/perl/Cursor.html $(docdir)/api/perl
	$(CHMOD) 644 $(docdir)/api/perl/Cursor.html
	$(CP) api/perl/SQLRelay.html $(docdir)/api/perl
	$(CHMOD) 644 $(docdir)/api/perl/SQLRelay.html
	$(CP) api/php/html/*.html $(docdir)/api/php/html
	$(CHMOD) 644 $(docdir)/api/php/html/*.html
	$(CP) api/php/html/*.css $(docdir)/api/php/html
	$(CHMOD) 644 $(docdir)/api/php/html/*.css
	$(CP) api/php/html/*.png $(docdir)/api/php/html
	$(CHMOD) 644 $(docdir)/api/php/html/*.png
	$(CP) api/php/html/*.svg $(docdir)/api/php/html
	$(CHMOD) 644 $(docdir)/api/php/html/*.svg
	$(CP) api/php/html/*.js $(docdir)/api/php/html
	$(CHMOD) 644 $(docdir)/api/php/html/*.js
	$(CP) api/python/*.html $(docdir)/api/python
	$(CHMOD) 644 $(docdir)/api/python/*.html
	$(CP) api/ruby/doc/*.html $(docdir)/api/ruby/doc
	$(CHMOD) 644 $(docdir)/api/ruby/doc/*.html
	$(CP) api/ruby/doc/*.rid $(docdir)/api/ruby/doc
	$(CHMOD) 644 $(docdir)/api/ruby/doc/*.rid
	$(CP) api/ruby/doc/css/*.css $(docdir)/api/ruby/doc/css
	$(CHMOD) 644 $(docdir)/api/ruby/doc/css/*.css
	$(CP) api/ruby/doc/images/*.png $(docdir)/api/ruby/doc/images
	$(CHMOD) 644 $(docdir)/api/ruby/doc/images/*.png
	$(CP) api/ruby/doc/images/*.gif $(docdir)/api/ruby/doc/images
	$(CHMOD) 644 $(docdir)/api/ruby/doc/images/*.gif
	$(CP) api/ruby/doc/js/*.js $(docdir)/api/ruby/doc/js
	$(CHMOD) 644 $(docdir)/api/ruby/doc/js/*.js
	$(CP) api/ruby/doc/js/*.js.gz $(docdir)/api/ruby/doc/js
	$(CHMOD) 644 $(docdir)/api/ruby/doc/js/*.js.gz
	$(CP) api/java/*.html $(docdir)/api/java
	$(CHMOD) 644 $(docdir)/api/java/*.html
	$(CP) api/java/*.js $(docdir)/api/java
	$(CHMOD) 644 $(docdir)/api/java/*.js
	$(CP) api/java/*.zip $(docdir)/api/java
	$(CHMOD) 644 $(docdir)/api/java/*.zip
	$(CP) api/java/*.js $(docdir)/api/java
	$(CHMOD) 644 $(docdir)/api/java/*.js
	$(CP) api/java/*.css $(docdir)/api/java
	$(CHMOD) 644 $(docdir)/api/java/*.css
	$(CP) api/java/element-list $(docdir)/api/java
	$(CHMOD) 644 $(docdir)/api/java/element-list
	$(CP) api/java/com/firstworks/sqlrelay/*.html $(docdir)/api/java/com/firstworks/sqlrelay
	$(CHMOD) 644 $(docdir)/api/java/com/firstworks/sqlrelay/*.html
	$(CP) api/java/resources/*.gif $(docdir)/api/java/resources
	$(CHMOD) 644 $(docdir)/api/java/resources/*.gif
	$(CP) api/java/resources/*.png $(docdir)/api/java/resources
	$(CHMOD) 644 $(docdir)/api/java/resources/*.png
	$(CP) api/java/jquery/*.js $(docdir)/api/java/jquery
	$(CHMOD) 644 $(docdir)/api/java/jquery/*.js
	$(CP) api/java/jquery/*.css $(docdir)/api/java/jquery
	$(CHMOD) 644 $(docdir)/api/java/jquery/*.css
	$(CP) api/java/jquery/external/jquery/*.js $(docdir)/api/java/jquery/external/jquery
	$(CHMOD) 644 $(docdir)/api/java/jquery/external/jquery/*.js
	$(CP) api/java/jquery/images/*.png $(docdir)/api/java/jquery/images
	$(CHMOD) 644 $(docdir)/api/java/jquery/images/*.png
	$(CP) api/java/jquery/jszip/dist/*.js $(docdir)/api/java/jquery/jszip/dist
	$(CHMOD) 644 $(docdir)/api/java/jquery/jszip/dist/*.js
	$(CP) api/java/jquery/jszip-utils/dist/*.js $(docdir)/api/java/jquery/jszip-utils/dist
	$(CHMOD) 644 $(docdir)/api/java/jquery/jszip-utils/dist/*.js
	$(CP) api/tcl/*.html $(docdir)/api/tcl
	$(CHMOD) 644 $(docdir)/api/tcl/*.html
	$(CP) api/tcl/*.txt $(docdir)/api/tcl
	$(CHMOD) 644 $(docdir)/api/tcl/*.txt
	$(CP) api/erlang/html/edoc-info $(docdir)/api/erlang/html
	$(CHMOD) 644 $(docdir)/api/erlang/html/edoc-info
	$(CP) api/erlang/html/*.png $(docdir)/api/erlang/html
	$(CHMOD) 644 $(docdir)/api/erlang/html/*.png
	$(CP) api/erlang/html/*.html $(docdir)/api/erlang/html
	$(CHMOD) 644 $(docdir)/api/erlang/html/*.html
	$(CP) api/erlang/html/*.css $(docdir)/api/erlang/html
	$(CHMOD) 644 $(docdir)/api/erlang/html/*.css
	$(CP) api/nodejs/html/*.html $(docdir)/api/nodejs/html
	$(CHMOD) 644 $(docdir)/api/nodejs/html/*.html
	$(CP) api/nodejs/html/*.css $(docdir)/api/nodejs/html
	$(CHMOD) 644 $(docdir)/api/nodejs/html/*.css
	$(CP) api/nodejs/html/*.png $(docdir)/api/nodejs/html
	$(CHMOD) 644 $(docdir)/api/nodejs/html/*.png
	$(CP) api/nodejs/html/*.svg $(docdir)/api/nodejs/html
	$(CHMOD) 644 $(docdir)/api/nodejs/html/*.svg
	$(CP) api/nodejs/html/*.js $(docdir)/api/nodejs/html
	$(CHMOD) 644 $(docdir)/api/nodejs/html/*.js
	$(CP) features/*.html $(docdir)/features
	$(CHMOD) 755 $(docdir)/features/*.html
	$(CP) images/*.png $(docdir)/images
	$(CHMOD) 644 $(docdir)/images/*.png
	$(CP) images/*.PNG $(docdir)/images
	$(CHMOD) 644 $(docdir)/images/*.PNG
	$(CP) js/*.js $(docdir)/js
	$(CHMOD) 644 $(docdir)/js/*.js
	$(CP) programming/*.html $(docdir)/programming
	$(CHMOD) 755 $(docdir)/programming/*.html
	$(MKINSTALLDIRS) $(docdir)/support
	$(CP) support/* $(docdir)/support
	$(CHMOD) 644 $(docdir)/support/*

uninstall:
	$(RMTREE) $(docdir)

clean:
	$(RM) $(OBJS)
	cd admin $(AND) $(MAKE) clean
	cd gettingstarted $(AND) $(MAKE) clean
	cd features $(AND) $(MAKE) clean
	cd programming $(AND) $(MAKE) clean
	cd howtos $(AND) $(MAKE) clean

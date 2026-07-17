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
	$(MKINSTALLDIRS) $(docdir)/api/perl/html
	$(MKINSTALLDIRS) $(docdir)/api/php/html
	$(MKINSTALLDIRS) $(docdir)/api/python/html
	$(MKINSTALLDIRS) $(docdir)/api/ruby/html/css
	$(MKINSTALLDIRS) $(docdir)/api/ruby/html/fonts
	$(MKINSTALLDIRS) $(docdir)/api/ruby/html/images
	$(MKINSTALLDIRS) $(docdir)/api/ruby/html/js
	$(MKINSTALLDIRS) $(docdir)/api/java/html
	$(MKINSTALLDIRS) $(docdir)/api/java/html/com/firstworks/sqlrelay
	$(MKINSTALLDIRS) $(docdir)/api/java/html/legal
	$(MKINSTALLDIRS) $(docdir)/api/java/html/resources
	$(MKINSTALLDIRS) $(docdir)/api/java/html/script-dir
	$(MKINSTALLDIRS) $(docdir)/api/tcl/html
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
	$(CP) api/perl/html/*.html $(docdir)/api/perl/html
	$(CHMOD) 644 $(docdir)/api/perl/html/*.html
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
	$(CP) api/python/html/*.html $(docdir)/api/python/html
	$(CHMOD) 644 $(docdir)/api/python/html/*.html
	$(CP) api/ruby/html/*.html $(docdir)/api/ruby/html
	$(CHMOD) 644 $(docdir)/api/ruby/html/*.html
	$(CP) api/ruby/html/*.rid $(docdir)/api/ruby/html
	$(CHMOD) 644 $(docdir)/api/ruby/html/*.rid
	$(CP) api/ruby/html/css/*.css $(docdir)/api/ruby/html/css
	$(CHMOD) 644 $(docdir)/api/ruby/html/css/*.css
	$(CP) api/ruby/html/fonts/*.ttf $(docdir)/api/ruby/html/fonts
	$(CHMOD) 644 $(docdir)/api/ruby/html/fonts/*.ttf
	$(CP) api/ruby/html/images/*.png $(docdir)/api/ruby/html/images
	$(CHMOD) 644 $(docdir)/api/ruby/html/images/*.png
	$(CP) api/ruby/html/images/*.gif $(docdir)/api/ruby/html/images
	$(CHMOD) 644 $(docdir)/api/ruby/html/images/*.gif
	$(CP) api/ruby/html/js/*.js $(docdir)/api/ruby/html/js
	$(CHMOD) 644 $(docdir)/api/ruby/html/js/*.js
	$(CP) api/ruby/html/js/*.js.gz $(docdir)/api/ruby/html/js
	$(CHMOD) 644 $(docdir)/api/ruby/html/js/*.js.gz
	$(CP) api/java/html/*.html $(docdir)/api/java/html
	$(CHMOD) 644 $(docdir)/api/java/html/*.html
	$(CP) api/java/html/*.js $(docdir)/api/java/html
	$(CHMOD) 644 $(docdir)/api/java/html/*.js
	$(CP) api/java/html/*.css $(docdir)/api/java/html
	$(CHMOD) 644 $(docdir)/api/java/html/*.css
	$(CP) api/java/html/*.svg $(docdir)/api/java/html
	$(CHMOD) 644 $(docdir)/api/java/html/*.svg
	$(CP) api/java/html/element-list $(docdir)/api/java/html
	$(CHMOD) 644 $(docdir)/api/java/html/element-list
	$(CP) api/java/html/com/firstworks/sqlrelay/*.html $(docdir)/api/java/html/com/firstworks/sqlrelay
	$(CHMOD) 644 $(docdir)/api/java/html/com/firstworks/sqlrelay/*.html
	$(CP) api/java/html/legal/* $(docdir)/api/java/html/legal
	$(CHMOD) 644 $(docdir)/api/java/html/legal/*
	$(CP) api/java/html/resources/*.png $(docdir)/api/java/html/resources
	$(CHMOD) 644 $(docdir)/api/java/html/resources/*.png
	$(CP) api/java/html/script-dir/*.js $(docdir)/api/java/html/script-dir
	$(CHMOD) 644 $(docdir)/api/java/html/script-dir/*.js
	$(CP) api/java/html/script-dir/*.css $(docdir)/api/java/html/script-dir
	$(CHMOD) 644 $(docdir)/api/java/html/script-dir/*.css
	$(CP) api/tcl/html/*.html $(docdir)/api/tcl/html
	$(CHMOD) 644 $(docdir)/api/tcl/html/*.html
	$(CP) api/tcl/html/*.txt $(docdir)/api/tcl/html
	$(CHMOD) 644 $(docdir)/api/tcl/html/*.txt
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

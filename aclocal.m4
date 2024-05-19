dnl prefixes to search for software packages
dnl	$1 - an specific prefix to also search in addition to standard prefixes
dnl		(may be left empty)
dnl		(should not contain spaces)
dnl	$2 - generic name of api or package, will be appended to various
dnl		partial prefixes in an attempt to search more exhaustively
dnl		eg. ssl, mysql, openssl, etc.
dnl		(may be left empty)
dnl		(should not contain spaces)
dnl	$3 - path to append to the prefix
dnl		eg. include, lib, bin, etc.
dnl		(may be left empty)
dnl		(should not contain spaces)
dnl	$4 - variable that will be set to the set of paths
AC_DEFUN([FW_SEARCH_PATHS],
[
EXTRAPATH="$1"
NAME="$2"
SUFFIX="$3"

SWPREFIXES="$EXTRAPATH / /usr"
if ( test -n "$NAME" )
then
	SWPREFIXES="$SWPREFIXES /usr/local/$NAME /opt/$NAME /usr/$NAME"
fi
SWPREFIXES="$SWPREFIXES /usr/local /usr/pkg"
if ( test -n "$NAME" )
then
	SWPREFIXES="$SWPREFIXES /usr/pkg/$NAME"
fi
SWPREFIXES="$SWPREFIXES /opt/sfw"
if ( test -n "$NAME" )
then
	SWPREFIXES="$SWPREFIXES /opt/sfw/$NAME"
fi
SWPREFIXES="$SWPREFIXES /usr/sfw"
if ( test -n "$NAME" )
then
	SWPREFIXES="$SWPREFIXES /usr/sfw/$NAME"
fi
SWPREFIXES="$SWPREFIXES /opt/csw /sw /usr/freeware /boot/common /resources/index"
SWPREFIXES="$SWPREFIXES /resources/firstworks"
if ( test -n "$NAME" )
then
	SWPREFIXES="$SWPREFIXES /Library/$NAME"
fi
SWPREFIXES="$SWPREFIXES /usr/local/firstworks"

if ( test -n "$SUFFIX" )
then
	SWPATHS=""
	for p in $SWPREFIXES
	do
		if ( test "$p" = "/" )
		then
			SWPATHS="$SWPATHS /$SUFFIX"
		else
			SWPATHS="$SWPATHS $p/$SUFFIX"
		fi
	done
	eval "$4=\"$SWPATHS\""
else
	eval "$4=\"$SWPREFIXES\""
fi
])




dnl sets UNAME to the uname of the machine
AC_DEFUN([FW_CHECK_UNAME],
[
UNAME=`uname -s`
AC_SUBST(UNAME)
])


dnl sets MAKE="make" if "make" is GNU make
dnl sets MAKE="gmake" if "gmake" is GNU make
dnl otherwise leaves MAKE unchanged
AC_DEFUN([FW_CHECK_GMAKE],
[
AC_MSG_CHECKING(for GNU Make)
if ( test -n "make -v | grep 'GNU Make'" )
then
	MAKE="make"
	AC_MSG_RESULT(yes)
else
	if ( test -n "gmake -v | grep 'GNU Make'" )
	then
		MAKE="gmake"
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
		AC_MSG_ERROR(GNU make not found.  SQL-Relay requires GNU make.)
	fi
fi
])


dnl verifies that a version number is greater than the specified version
dnl	$1 - generic name of api or package
dnl	$2 - version of the api or package
dnl	$3 - version to test against
AC_DEFUN([FW_CHECK_VERSION],
[

NAME="$1"
VERSION="$2"
TESTVERSION="$3"

AC_MSG_CHECKING(for $NAME >= $TESTVERSION)

V1=`echo $VERSION | cut -d. -f1`
V2=`echo $VERSION | cut -d. -f2`
V3=`echo $VERSION | cut -d. -f3`

TV1=`echo $TESTVERSION | cut -d. -f1`
TV2=`echo $TESTVERSION | cut -d. -f2`
TV3=`echo $TESTVERSION | cut -d. -f3`

if ( test "$V1" -gt "$TV1")
then
	AC_MSG_RESULT(yes - $VERSION found)
elif ( test "$V1" -eq "$TV1")
then
	if ( test "$V2" -gt "$TV2")
	then
		AC_MSG_RESULT(yes - $VERSION found)
	elif ( test "$V2" -eq "$TV2")
	then
		if ( test "$V3" -ge "$TV3")
		then
			AC_MSG_RESULT(yes - $VERSION found)
		else
			AC_MSG_ERROR(no - $VERSION found)
		fi
	else
		AC_MSG_ERROR(no - $VERSION found)
	fi
else
	AC_MSG_ERROR(no - $VERSION found)
fi
])


dnl displays a message indicating that the version of $1 is $2
AC_DEFUN([FW_VERSION],
[
if ( test -n "$2" )
then
	echo "$1 version... $2"
fi
])


dnl displays a message indicating that the include flags for $1 are $2
AC_DEFUN([FW_INCLUDES],
[
if ( test -n "$2" )
then
	echo "$1 includes... $2"
fi
])


dnl displays a message indicating that the lib flags for $1 are $2
AC_DEFUN([FW_LIBS],
[
if ( test -n "$2" )
then
	echo "$1 libs... $2"
fi
])


dnl if $1 is a readable file (full path name) then expression $2 is evaluated
AC_DEFUN([FW_CHECK_FILE],
[
dnl echo "check: $1"
if ( test -r "$1" )
then
	eval "$2"
fi
])


dnl attmepts to compile, but not link, a program
dnl	$1 - includes and other statements to go before main()
dnl	$2 - body of main()
dnl	$3 - CPPFLAGS
dnl	$4 - statement to evaluate if link succeeds
dnl	$5 - statement to evaluate if link fails
AC_DEFUN([FW_TRY_COMPILE],
[
SAVECPPFLAGS="$CPPFLAGS"
CPPFLAGS="$3"
AC_TRY_COMPILE([$1],[$2],[$4],[$5])
CPPFLAGS="$SAVECPPFLAGS"
])


dnl attmepts to compile and link a program
dnl	$1 - includes and other statements to go before main()
dnl	$2 - body of main()
dnl	$3 - CPPFLAGS
dnl	$4 - LIBS
dnl	$5 - LD_LIBRARY_PATH
dnl	$6 - statement to evaluate if link succeeds
dnl	$7 - statement to evaluate if link fails
AC_DEFUN([FW_TRY_LINK],
[
SAVECPPFLAGS="$CPPFLAGS"
SAVELIBS="$LIBS"
SAVE_LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
CPPFLAGS="$3"
LIBS="$4"
LD_LIBRARY_PATH="$5"
export LD_LIBRARY_PATH
AC_TRY_LINK([$1],[$2],[$6],[$7])
CPPFLAGS="$SAVECPPFLAGS"
LIBS="$SAVELIBS"
LD_LIBRARY_PATH="$SAVE_LD_LIBRARY_PATH"
export LD_LIBRARY_PATH
])


dnl checks for a library file
dnl	$1 - library file to check for (full path name)
dnl	$2 - statement to evaluate if library file is found
dnl	$3 - alternative library file to check for
dnl	$2 - statement to evaluate if alternative library file is found
AC_DEFUN([FW_CHECK_LIB],
[
FOUNDLIB=""
FW_CHECK_FILE($1, FOUNDLIB="yes")
if ( test -n "$FOUNDLIB" )
then
	eval "$2"
else
	if ( test -n "$3" )
	then
		FW_CHECK_FILE($3, FOUNDLIB="yes")
		if ( test -n "$FOUNDLIB" )
		then
			eval "$4"
		fi
	fi
fi
])


dnl checks for a header and library pair
dnl	$1 - header file to check for (full path name)
dnl	$2 - statement to evaluate if header and library are found
dnl	$3 - library file to check for (full path name)
dnl	$4 - statement to evaluate if header and library are found
dnl	$5 - alternative library file to check for (full path name)
dnl	$6 - statement to evaluate if header and alternative library are found
AC_DEFUN([FW_CHECK_HEADER_LIB],
[
FOUNDHEADER=""
FOUNDLIB=""
FW_CHECK_FILE([$3],[FOUNDLIB=\"yes\"])
if ( test -n "$FOUNDLIB" )
then
	FW_CHECK_FILE([$1],[FOUNDHEADER=\"yes\"])
	if ( test -n "$FOUNDHEADER" )
	then
		eval "$2"
		eval "$4"
	fi
elif ( test -n "$5" )
then
	FW_CHECK_FILE([$5],[FOUNDLIB=\"yes\"])
	if ( test -n "$FOUNDLIB" )
	then
		FW_CHECK_FILE([$1],[FOUNDHEADER=\"yes\"])
		if ( test -n "$FOUNDHEADER" )
		then
			eval "$2"
			eval "$6"
		fi
	fi
fi
])


dnl checks the specified search path and also various build-in paths for a
dnl header and library pair
dnl	$1 - search path
dnl	$2 - generic name of api or package, will be appended to various
dnl		partial paths in an attempt to search more exhaustively
dnl		eg. ssl, mysql, openssl, etc.
dnl	$3 - header file to check for (relative path name)
dnl		eg. api.h  not  /usr/include/api.h
dnl	$4 - library basename (excluding "lib" and suffix) to check for
dnl		eg. api  not  lbiapi, libapi.so, /usr/lib/libapi.so, etc.
dnl	$5 - flag to set $10 to if a static version of the library was found
dnl	$6 - ??? rpath related, but unused
dnl	$7 - variable that will be set to the include flags that were found
dnl		eg. -I/usr/local/include
dnl	$8 - variable that will be set to the lib flags that were found
dnl		eg. -L/usr/local/lib -lsomelib
dnl	$9 - variable that will be set to the path that the library was found in
dnl		eg. /usr/local/lib
dnl	$10 - variable that will be set to the value passed in $5 if a static
dnl		version of the library was found
dnl	$11 - variable that will be set to the base directory that the headers
dnl		and libs were found in
dnl		eg. /usr/local
dnl	$12 - whether or not to use the full library path when building the lib
dnl		flags - can be yes or no
dnl		eg. if yes, then $8 set to: -L/usr/local/lib -lsomelib
dnl		    if no, then $8 set to:  -Wl,/usr/local/lib/libsomelib.so
dnl
dnl If LIBARCHDIR is set, then it is appended, as appropriate, to the various
dnl paths when searching for libraries.  If it's not set then "lib" is appended.
AC_DEFUN([FW_CHECK_HEADERS_AND_LIBS],
[

SEARCHPATH=$1
NAME=$2
HEADER=$3
LIBNAME=$4
LINKSTATIC=$5
LINKRPATH=$6
USEFULLLIBPATH=$12
INCLUDESTRING=""
LIBSTRING=""
LIBPATH=""
STATIC=""
HEADERSANDLIBSPATH=""

eval "$7=\"\""
eval "$8=\"\""
eval "$9=\"\""
eval "$10=\"\""
if ( test -n "$11" )
then
	eval "$11=\"\""
fi

FW_SEARCH_PATHS([$SEARCHPATH],[$NAME],[],[SEARCHPATHS])
for path in $SEARCHPATHS
do

	dnl loop back if the path is empty or doesn't exist
	if ( test -z "$path" -o ! -d "$path" )
	then
		continue
	fi

	dnl if LIBARCHDIR is set then search $path/$LIBARCHDIR
	if ( test -n "$LIBARCHDIR" )
	then

		if ( test "$path" = "/" )
		then

			dnl when searching in / we need to search in
			dnl /usr/include, not in /include, and in
			dnl /$LIBARCHDIR, not //$LIBARCHDIR, so lets
			dnl handle / a little differently...

			if ( test "$USEFULLLIBPATH" = "yes" )
			then
				FW_CHECK_HEADER_LIB([/usr/include/$HEADER],[],[/$LIBARCHDIR/lib$LIBNAME.$SOSUFFIX],[LIBPATH=\"/$LIBARCHDIR\"; LIBSTRING=\"-Wl,/$LIBARCHDIR/lib$LIBNAME.$SOSUFFIX\"],[/$LIBARCHDIR/lib$LIBNAME.a],[LIBSTRING=\"/$LIBARCHDIR/lib$LIBNAME.a\"; STATIC=\"$LINKSTATIC\"])
			else
				FW_CHECK_HEADER_LIB([/usr/include/$HEADER],[],[/$LIBARCHDIR/lib$LIBNAME.$SOSUFFIX],[LIBPATH=\"/$LIBARCHDIR\"; LIBSTRING=\"-l$LIBNAME\"],[/$LIBARCHDIR/lib$LIBNAME.a],[LIBSTRING=\"-l$LIBNAME\"; STATIC=\"$LINKSTATIC\"])
			fi

		else
	
			dnl search various dirs under $path
			for libpath in \
				"$path/$LIBARCHDIR" \
				"$path/$LIBARCHDIR/$NAME" \
				"$path/$LIBARCHDIR/opt" \
				"$path/$LIBARCHDIR/$MULTIARCHDIR"
			do
				for includepath in "$path/include" "$path/include/$NAME"
				do
					if ( test "$USEFULLLIBPATH" = "yes" )
					then
						FW_CHECK_HEADER_LIB([$includepath/$HEADER],[INCLUDESTRING=\"-I$includepath\"],[$libpath/lib$LIBNAME.$SOSUFFIX],[LIBPATH=\"$libpath\"; LIBSTRING=\"-Wl,$libpath/lib$LIBNAME.$SOSUFFIX\"],[$libpath/lib$LIBNAME.a],[LIBSTRING=\"$libpath/lib$LIBNAME.a\"; STATIC=\"$LINKSTATIC\"])
					else
						FW_CHECK_HEADER_LIB([$includepath/$HEADER],[INCLUDESTRING=\"-I$includepath\"],[$libpath/lib$LIBNAME.$SOSUFFIX],[LIBPATH=\"$libpath\"; LIBSTRING=\"-L$libpath -l$LIBNAME\"],[$libpath/lib$LIBNAME.a],[LIBSTRING=\"-L$libpath -l$LIBNAME\"; STATIC=\"$LINKSTATIC\"])
					fi
					if ( test -n "$LIBSTRING" )
					then
						break
					fi
				done
				if ( test -n "$LIBSTRING" )
				then
					break
				fi
			done
		fi

		dnl break if we found the library and headers
		if ( test -n "$LIBSTRING" )
		then
			HEADERSANDLIBSPATH="$path"
			break
		fi
	fi

	dnl if that fails then we should search $path/lib...
	if ( test -z "$LIBSTRING" )
	then

		dnl however, if LIBARCHDIR is "lib" then we can skip this
		dnl step, as $path/lib will have been already been searched
		dnl above
		if ( test "$LIBARCHDIR" = "lib" )
		then
			continue
		fi

		dnl Also, if the path is / or /usr then we shouldn't search
		dnl in $path/lib.  We only want to search /$LIBARCHDIR and
		dnl /usr/$LIBARCHDIR.  If LIBARCHDIR happened to be "lib" then
		dnl we will already have searched /lib and /usr/lib above.
		if ( test "$path" = "/" -o "$path" = "/usr" )
		then
			continue
		fi

		dnl search various dirs under $path
		for libpath in \
			"$path/lib" \
			"$path/lib/$NAME" \
			"$path/lib/opt" \
			"$path/lib/$MULTIARCHDIR"
		do
			for includepath in "$path/include" "$path/include/$NAME"
			do
				if ( test "$USEFULLLIBPATH" = "yes" )
				then
					FW_CHECK_HEADER_LIB([$includepath/$HEADER],[INCLUDESTRING=\"-I$includepath\"],[$libpath/lib$LIBNAME.$SOSUFFIX],[LIBPATH=\"$libpath\"; LIBSTRING=\"-Wl,$libpath/lib$LIBNAME.$SOSUFFIX\"],[$libpath/lib$LIBNAME.a],[LIBSTRING=\"$libpath/lib$LIBNAME.a\"; STATIC=\"$LINKSTATIC\"])
				else
					FW_CHECK_HEADER_LIB([$includepath/$HEADER],[INCLUDESTRING=\"-I$includepath\"],[$libpath/lib$LIBNAME.$SOSUFFIX],[LIBPATH=\"$libpath\"; LIBSTRING=\"-L$libpath -l$LIBNAME\"],[$libpath/lib$LIBNAME.a],[LIBSTRING=\"-L$libpath -l$LIBNAME\"; STATIC=\"$LINKSTATIC\"])
				fi
				if ( test -n "$LIBSTRING" )
				then
					break
				fi
			done
			if ( test -n "$LIBSTRING" )
			then
				break
			fi
		done

		dnl break if we found the library and headers
		if ( test -n "$LIBSTRING" )
		then
			HEADERSANDLIBSPATH="$path"
			break
		fi
	fi

done

dnl remove -I/usr/include, -L/lib, -L/usr/lib, -L/lib64 and -L/usr/lib64
INCLUDESTRING=`echo $INCLUDESTRING | sed -e "s|-I/usr/include$||g" -e "s|-I/usr/include ||g"`
LIBSTRING=`echo $LIBSTRING | sed -e "s|-L/usr/lib$||g" -e "s|-L/lib$||g" -e "s|-L/usr/lib ||g" -e "s|-L/lib ||g"`
LIBSTRING=`echo $LIBSTRING | sed -e "s|-L/usr/lib64$||g" -e "s|-L/lib64$||g" -e "s|-L/usr/lib64 ||g" -e "s|-L/lib64 ||g"`

eval "$7=\"$INCLUDESTRING\""
eval "$8=\"$LIBSTRING\""
eval "$9=\"$LIBPATH\""
eval "$10=\"$STATIC\""
if ( test -n "$11" )
then
	eval "$11=\"$HEADERSANDLIBSPATH\""
fi
])


dnl removes -Wl,-Bsymbolic-functions from $2 and sets variable $1 to the result
AC_DEFUN([FW_STRIP_SYMBOLIC_FUNCTIONS],
[
dnl On some platforms (Ubuntu), the -Wl,-Bsymbolic-functions and flags end up
dnl coming through in ldflags, which can cause problems for apps (sqlrelay+db2
dnl on Ubuntu 18.04).  Filter those out.
STRIPPED=`echo "$2" | sed -e "s|-Wl,-Bsymbolic-functions||g"`
eval "$1=\"$STRIPPED\""
])


dnl removes -Wl,-z,relro from $2 and sets variable $1 to the result
AC_DEFUN([FW_STRIP_RELRO],
[
dnl On some platforms (Ubuntu), superfluous -Wl,-z,relro flags end up coming
dnl through in ldflags.  Filter those out.
STRIPPED=`echo "$2" | sed -e "s|-Wl,-z,relro||g"`
eval "$1=\"$STRIPPED\""
])


dnl if LDFLAGS contains -Wl,-z,relro the CPPFLAGS requires -fPIC
dnl libtool adds it at compile time but we need it for configure tests too
AC_DEFUN([FW_CHECK_RELRO_FPIC],
[
if ( test -n "`echo $LDFLAGS | grep '\-Wl,\-z,relro'`" )
then
	CPPFLAGS="-fPIC $CPPFLAGS"
fi
])


dnl if the variable USE_SYSTEM_LIBTOOL = "yes" then the variable LIBTOOL is
dnl overridden and set to "libtool" rather than its current value
dnl (likely "$(top_builddir)/libtool" so that the system-supplied libtool is
dnl used rather than the local version
AC_DEFUN([FW_CHECK_USE_SYSTEM_LIBTOOL],
[
if ( test "$USE_SYSTEM_LIBTOOL" = "yes" )
then
	LIBTOOL="libtool"
fi
])


dnl have to set ar here because libtool (currently) fails
dnl to use the correct one when cross-compiling
AC_DEFUN([FW_FIX_AR],
[
	AC_CHECK_PROG(AR,$host_alias-ar,$host_alias-ar,ar)
	AC_SUBST(AR)
])


dnl checks if the linker supports -rpath
dnl if it does, then it sets the enviroment variable RPATHFLAG="yes"
dnl if it does not, then it sets the enviroment variable RPATHFLAG=""
AC_DEFUN([FW_CHECK_LD_RPATH],
[
AC_MSG_CHECKING(whether ld -rpath works)
dnl FIXME: this can also return "missing argument" if it works
if ( test -n "`ld -rpath /usr/lib 2>&1 | grep 'no input files'`" )
then
	RPATHFLAG="yes"
	AC_MSG_RESULT(yes)
else
	RPATHFLAG=""
	AC_MSG_RESULT(no)
fi
])


dnl checks to see if the -pipe compiler option works or not
dnl if it does, then it sets the variable PIPE="-pipe"
dnl if it does not , then it sets the variable PIPE=""
AC_DEFUN([FW_CHECK_PIPE],
[
AC_MSG_CHECKING(for -pipe option)

dnl Solaris 8 and 9 support -pipe, but often fail to compile code that uses
dnl templates when -pipe is used, with errors like:
dnl
dnl "<stdin>", line 346800 : Internal: Out of symbol table
dnl 
dnl ... output pipe has been closed
dnl
dnl It's not clear why not using -pipe makes it work, but it does.
PIPECHECK="yes"
OS="`uname -s 2> /dev/null`"
VERSION="`uname -r 2> /dev/null`"
if ( test "$OS" = "SunOS" )
then
	if ( test "$VERSION" = "5.8" -o "$VERSION" = "5.9" )
	then
		PIPECHECK="no"
	fi
fi

if ( test "$PIPECHECK" = "yes" )
then
	FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-pipe],[],[],[PIPE="-pipe"],[PIPE=""])
	if ( test -n "$PIPE" )
	then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
else
	AC_MSG_RESULT(no - disabled on Solaris 8/9)
fi
AC_SUBST(PIPE)
])


dnl checks to see if -fPIC compiler option works or not
dnl if it does, then it sets the variable FPIC="-fPIC"
dnl if it does not , then it sets the variable FPIC=""
AC_DEFUN([FW_CHECK_FPIC],
[
AC_MSG_CHECKING(for -fPIC option)
FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-fPIC],[],[],[FPIC="-fPIC"],[FPIC=""])
if ( test -n "$FPIC" )
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(FPIC)
])


dnl checks to see if the -Werror compiler option works or not
dnl if it does, then WERROR="-Werror" is set
dnl if it does not, then WERROR="" is set
AC_DEFUN([FW_CHECK_WERROR],
[
WERROR=""
if ( test "$ENABLE_WERROR" = "yes" )
then
	AC_MSG_CHECKING(for -Werror)
	FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-Werror],[],[],[WERROR="-Werror"])

	dnl if -Werror appers to be supported...
	if ( test -n "$WERROR" )
	then

		dnl disable -Werror with gcc < 2.7 because
		dnl it misinterprets placement new
		CXX_VERSION=`$CXX --version 2> /dev/null | head -1`

		dnl Newer versions of gcc output the version differently
		dnl and the above results in "g+".  These all work correctly.
		if ( test -n "$CXX_VERSION" -a -z "`echo $CXX_VERSION | grep g++`" )
		then
			dnl older versions output something like 27, 28, 29, etc.
			if (  test "`echo $CXX_VERSION | tr -d '.' | cut -c1-2`" -lt "27" )
			then
				WERROR=""
			fi
		fi

	fi

	if ( test -n "$WERROR" )
	then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
fi

AC_SUBST(WERROR)
])


dnl enables -Werror (if supported) in CPPFLAGS/CXXFLAGS if it wasn't previously
dnl enabled and sets HADWERROR to yes if it was previously enabled or to no if
dnl it wasn't
AC_DEFUN([FW_ENABLE_WERROR],
[
HADWERROR="yes"
if ( test -z "`echo $CPPFLAGS | grep Werror`" )
then
	CPPFLAGS="$CPPFLAGS $WERROR"
	CXXFLAGS="$CXXFLAGS $WERROR"
	HADWERROR="no"
fi
])


dnl evaluates HADWERROR and removes instances of -Werror from CPPFLAGS and
dnl CXXFLAGS if it wasn't previously enabled (see FW_ENABLE_WERROR)
dnl FIXME: also removes Wall, though that isn't enabled by FW_ENABLE_WERROR
AC_DEFUN([FW_RESTORE_WERROR],
[
if ( test "$HADWERROR" = "no" )
then
	CPPFLAGS=`echo $CPPFLAGS | sed -e "s|-Werror[[^ ]]*||g" -e "s|-Wall||g"`
	CXXFLAGS=`echo $CXXFLAGS | sed -e "s|-Werror[[^ ]]*||g" -e "s|-Wall||g"`
fi
])


dnl checks to see if the -Wall compiler option works or not
dnl if it does, then WALL="-Wall" is set
dnl if it does not, then WALL="" is set
AC_DEFUN([FW_CHECK_WALL],
[
WALL=""
if ( test "$ENABLE_WALL" = "yes" )
then
	AC_MSG_CHECKING(for -Wall)
	FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-Wall],[],[],[WALL="-Wall"])
	if ( test -n "$WALL" )
	then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
fi
AC_SUBST(WALL)
])


dnl checks to see if -Wall includes -Wunused-variables
dnl if it does, then WALL="" is set
dnl if it does not, then WALL is unchanged
AC_DEFUN([FW_CHECK_WALL_WUNUSED],
[
if ( test -n "$WALL" )
then
	dnl Sometimes -Wall includes -Wunused-variables and
	dnl -Wunused-parameters which we don't care about.
	dnl Disable it if it does.
	OLDCPPFLAGS=$CPPFLAGS
	CPPFLAGS="$WALL $WERROR $CPPFLAGS"
	AC_MSG_CHECKING(whether -Wall includes -Wunused-*)
	AC_TRY_COMPILE([void f(int a) { return; }],[f(1);],AC_MSG_RESULT(no),WALL=""; AC_MSG_RESULT(yes))	
	CPPFLAGS=$OLDCPPFLAGS
fi
])


dnl checks to see if the -Wno-format compiler option is necessary to compile
dnl class methods named printf()
dnl if it is, then WNOFORMAT="-Wno-format" is set
dnl if it is not, then WNOFORMAT="" is set
AC_DEFUN([FW_CHECK_WNOFORMAT],
[

WNOFORMAT=""
if ( test -n "$WERROR" )
then
	OLDCPPFLAGS=$CPPFLAGS
	CPPFLAGS="$WALL $WERROR $CPPFLAGS"
	AC_MSG_CHECKING(for -Wno-format option)
	AC_TRY_COMPILE([#include <stdio.h>

class charstring {
	public:
		static void printf(char *buffer, const char *fmt);
};

void charstring::printf(char *buffer, const char *fmt) {
	vsprintf(buffer,fmt,NULL);
}],[char buf[10]; charstring::printf(buf,"hello");],AC_MSG_RESULT(no), AC_MSG_RESULT(yes); WNOFORMAT="-Wno-format")	
	CPPFLAGS=$OLDCPPFLAGS
fi

AC_SUBST(WNOFORMAT)
])


dnl checks to see if -Wall includes -Woverloaded-virtual
dnl if it does, then WNOOVERLOADEDVIRTUAL="-Wno-overloaded-virtual" is set
dnl if it does not, then WNOOVERLOADEDVIRTUAL="" is set
AC_DEFUN([FW_CHECK_WNOOVERLOADEDVIRTUAL],
[

WNOOVERLOADEDVIRTUAL=""
AC_MSG_CHECKING(for -Wno-overloaded-virtual option)

dnl clang's and gcc 13+'s -Wall includes -Woverloaded-virtual, which we don't
dnl want. It looks like all versions of clang and gcc are tolerant to passing
dnl in this parameter, whether they really support it or not, so we'll just
dnl always use it for clang and gcc
if ( test -n "`$CC --version 2> /dev/null | grep clang`" -o -n "`$CC --version 2> /dev/null | grep gcc`" )
then
	WNOOVERLOADEDVIRTUAL="-Wno-overloaded-virtual"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

AC_SUBST(WNOOVERLOADEDVIRTUAL)
])


dnl checks to see if -Wall includes -Wmismatched-tags
dnl if it is, then WNOMISMATCHEDTAGS="-Wno-mismatched-tags" is set
dnl if it is not, then WNOMISMATCHEDTAGS="" is set
AC_DEFUN([FW_CHECK_WNOMISMATCHEDTAGS],
[

WNOMISMATCHEDTAGS=""
AC_MSG_CHECKING(for -Wno-mismatched-tags option)

# clang's -Wall includes -Wmismatched-tags, which we don't want
if ( test -n "`$CC --version 2> /dev/null | grep clang`" )
then
	WNOMISMATCHEDTAGS="-Wno-mismatched-tags"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

AC_SUBST(WNOMISMATCHEDTAGS)
])


dnl checks to see if the -Wno-deprecated-declarations compiler option works or
dnl not
dnl if it does, then WNODEPRECATEDDECLARATIONS="-Wno-deprecated-declarations"
dnl is set
dnl if it does not, then WNODEPRECATEDDECLARATIONS="" is set
AC_DEFUN([FW_CHECK_WNODEPRECATEDDECLARATIONS],
[

WNODEPRECATEDDECLARATIONS=""
if ( test -n "$WERROR" )
then
	OLDCPPFLAGS=$CPPFLAGS
	CPPFLAGS="$CPPFLAGS -Wno-deprecated-declarations"
	AC_MSG_CHECKING(for -Wno-deprecated-declarations option)
	AC_TRY_COMPILE([#include <stdio.h>],[printf("hello");],AC_MSG_RESULT(yes); WNODEPRECATEDDECLARATIONS="-Wno-deprecated-declarations",AC_MSG_RESULT(yes))	
	CPPFLAGS=$OLDCPPFLAGS
fi

AC_SUBST(WNODEPRECATEDDECLARATIONS)
])


dnl checks to see if the C++ compiler allows undefined functions
dnl if it does, then WERROR is added to CPPFLAGS then sets WERROR to "" so it
dnl won't be put back in again later
dnl FIXME: I don't remember the exact purpose of this working this way.  I think
dnl -Werror needs to be added to CPPFLAGS so that the configure script uses it
dnl when testing for the presence of functions.  I'm not sure why it's set to
dnl "" though.
AC_DEFUN([FW_CHECK_UNDEFINED_FUNCTIONS],
[
AC_MSG_CHECKING(for whether undefined functions are allowed)
AC_TRY_COMPILE([],[printf("hello");],CPPFLAGS="$WERROR $CPPFLAGS"; WERROR=""; AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
])


dnl checks to see if the -g3 compiler option works or not
dnl adds it to CFLAGS and CXXFLAGS if it does
AC_DEFUN([FW_CHECK_DEBUG],
[
if ( test "$DEBUG" = "yes" )
then
	AC_MSG_CHECKING(for -g3)
	FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-g3],[],[],[DBG="-g3"],[DBG="-g"])
	if ( test "$DBG" = "-g3" )
	then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
	CFLAGS="$CLAGS $DBG"
	CXXFLAGS="$CXXFLAGS $DBG"
fi
])


dnl checks to see if the --disable-new-dtags linker option works or not
dnl if it does, then DISABLE_NEW_DTAGS="-Wl,--disable-new-dtags" is set
dnl if it does not, then DISABLE_NEW_DTAGS="" is set
AC_DEFUN([FW_CHECK_NEW_DTAGS],
[
	AC_MSG_CHECKING(for -disable-new-dtags)
	FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-Wl,--disable-new-dtags],[],[],[DISABLE_NEW_DTAGS="-Wl,--disable-new-dtags"],[DISABLE_NEW_DTAGS=""])
	if ( test "$DISABLE_NEW_DTAGS" = "-Wl,--disable-new-dtags" )
	then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
	AC_SUBST(DISABLE_NEW_DTAGS)
])


dnl Some environments throw warnings if stdlib is used because it redefines
dnl built-in functions abort() exit().  On those platforms we'll include the
dnl -fno-builtin flag in CPPFLAGS
AC_DEFUN([FW_CHECK_F_NO_BUILTIN],
[
AC_MSG_CHECKING(whether -fno-builtin needs to be used)

OLDCPPFLAGS="$CPPFLAGS"
CPPFLAGS="$WALL $WERROR $CPPFLAGS"
STDLIB_TEST="no"
AC_TRY_COMPILE([#include <stdlib.h>],[],STDLIB_TEST="yes")
CPPFLAGS="$OLDCPPFLAGS"

dnl If that failed, try again with -fno-builtin
if ( test "$STDLIB_TEST" = "no" )
then
	OLDCPPFLAGS="$CPPFLAGS"
	CPPFLAGS="-fno-builtin $WALL $WERROR $CPPFLAGS"
	AC_TRY_COMPILE([#include <stdlib.h>],[],STDLIB_TEST="yes")

	dnl if that also failed then restore CPPFLAGS,
	dnl the platform probably just doesn't have stdlib.h
	if ( test "$STDLIB_TEST" = "no" )
	then
		CPPFLAGS="$OLDCPPFLAGS"
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
	fi
 else
	AC_MSG_RESULT(no)
fi
])


dnl Determines what extension shared object files have
dnl sets SOSUFFIX, MODULESUFFIX, and JNISUFFIX accordingly
AC_DEFUN([FW_CHECK_SO_EXT],
[
AC_MSG_CHECKING(for dynamic library extensions)
SOSUFFIX="so"
MODULESUFFIX="so"
JNISUFFIX="so"
if ( test -n "$CYGWIN" )
then
	SOSUFFIX="dll.a"
fi
if ( test -n "$DARWIN" )
then
	SOSUFFIX="dylib"
	MODULESUFFIX="bundle"
	JNISUFFIX="jnilib"
fi
AC_MSG_RESULT(so=>$SOSUFFIX module=>$MODULESUFFIX jni=>$JNISUFFIX)
AC_SUBST(SOSUFFIX)
AC_SUBST(MODULESUFFIX)
AC_SUBST(JNISUFFIX)
])


dnl checks for multiarch platform and sets MULTIARCHDIR to the multiarch name
dnl
dnl also determines the arch-specific directory that libraries are usually
dnl found in, dnl eg. lib, lib64, or lib32 and sets LIBARCHDIR as appropriate
AC_DEFUN([FW_CHECK_MULTIARCH],
[
AC_MSG_CHECKING(for multiarch platform)
MULTIARCHDIR="`$CC $CPPFLAGS -print-multiarch 2> /dev/null`"
if ( test -n "$MULTIARCHDIR" )
then
	AC_MSG_RESULT($MULTIARCHDIR)
else
	AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for arch-specific library directory)
LIBARCHDIR="lib"
if ( test -z "$MULTIARCHDIR" )
then
	case $host_cpu in
		x86_64 )
			LIBARCHDIR="lib64"
			;;
		mips64 )
			LIBARCHDIR="lib64"
			;;
		mips )
			LIBARCHDIR="lib32"
			;;
	esac
fi
AC_MSG_RESULT($LIBARCHDIR)
])


dnl $CC -print-multiarch doesn't return anything on most platforms,
dnl but sometimes we need the multiarch dir anyway (eg. to find python on
dnl fedora 26) so, we'll attempt to finagle it...
AC_DEFUN([FW_CHECK_OVERRIDE_MULTIARCH],
[
if ( test -z "$MULTIARCHDIR" )
then
	AC_MSG_CHECKING(for whether to override multiarch)
	MAARCH=`uname -m 2> /dev/null`
	MAOS=`uname -o 2> /dev/null`
	if ( test "$MAOS" = "GNU/Linux" )
	then
		MAOS="linux-gnu"
	fi
	if ( test -n "$MAARCH" -a -n "$MAOS" )
	then
		MULTIARCHDIR="$MAARCH-$MAOS"
		AC_MSG_RESULT($MULTIARCHDIR)
	else
		AC_MSG_RESULT(no)
	fi
fi
])


dnl checks for microsoft platform.
dnl sets MINGW32, CYGWIN, UWIN, and MICROSOFT to "yes" as appropriate
AC_DEFUN([FW_CHECK_MICROSOFT],
[
AC_MSG_CHECKING(for microsoft platform)
CYGWIN=""
MINGW32=""
UWIN=""
MICROSOFT=""
EXE=""
case $host_os in
	*cygwin* )
		CYGWIN="yes"
		MICROSOFT="yes"
		EXE=".exe"
		AC_MSG_RESULT(cygwin)
		;;
	*mingw32* )
		MINGW32="yes"
		MICROSOFT="yes"
		EXE=".exe"
		cross_compiling="yes"
		AC_MSG_RESULT(mingw32)
		;;
	*uwin* )
		UWIN="yes"
		MICROSOFT="yes"
		EXE=".exe"
		AC_MSG_RESULT(uwin)
		;;
	* )
		AC_MSG_RESULT(no)
		;;
esac
AC_SUBST(CYGWIN)
AC_SUBST(MINGW32)
AC_SUBST(UWIN)
AC_SUBST(MICROSOFT)
AC_SUBST(EXE)
])


dnl checks for Mac OS X platform
dnl sets DARWIN to "yes" as appropriate
dnl sets SHELL="...path to bash shell..." if the bash shell is available
dnl sets BUNDLE_LOADER="-bundle_loader" if the ld flag is supported
AC_DEFUN([FW_CHECK_OSX],
[
DARWIN=""
BUNDLE_LOADER=""
AC_MSG_CHECKING(for OSX)
case $host_os in
	*darwin* )
		DARWIN="yes"
		AC_DEFINE(_DARWIN,1,Darwin)

		dnl get the actual mac os version
		PV1="`sw_vers | grep ProductVersion | cut -d':' -f2 | tr -d '\t' | cut -d'.' -f1`"
		PV2="`sw_vers | grep ProductVersion | cut -d':' -f2 | tr -d '\t' | cut -d'.' -f2`"
		PRODUCTVERSION="$PV1.$PV2"
		AC_MSG_RESULT($PRODUCTVERSION)

		FW_CHECK_WNOLONGDOUBLE

		dnl prefer bash to the default shell, which could be tcsh or
		dnl zsh on older versions, and which doesn't run libtool very
		dnl well
		BASH=`which bash`
		if ( test -n "$BASH" )
		then
			SHELL="$BASH"
		fi

		MACOSXDEPLOYMENTTARGET="MACOSX_DEPLOYMENT_TARGET=$PRODUCTVERSION"

		dnl set some version-specific stuff
		case "$PRODUCTVERSION" in
			10.0 )
				;;
			10.[[12]] )
				BUNDLE_LOADER="-bundle_loader"
				;;
			* )
				UNDEFINED_DYNAMIC_LOOKUP="-Wl,-undefined -Wl,dynamic_lookup"
				;;
		esac
		;;
	* )
		AC_MSG_RESULT(no)
		;;
esac

AC_SUBST(SHELL)
AC_SUBST(MACOSXDEPLOYMENTTARGET)
])



dnl checks for apache module api
AC_DEFUN([FW_CHECK_APACHE],
[
APACHEINCLUDES=""
APACHELINKFLAGS=""

if ( test "$INCLUDE_APACHE" = "1" )
then
	FW_CHECK_APXS
	FW_CHECK_APRCONFIG
	FW_CHECK_APUCONFIG

	if ( test -n "$APACHEINCLUDES" )
	then
		if ( test -n "$DARWIN" )
		then
			if ( test -n "$BUNDLE_LOADER" )
			then
				APACHEHTTPD=""
				for prog in httpd apache2 apache
				do
					AC_PATH_PROG(APACHEHTTPD,$prog,"",/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/opt/sfw/bin:/opt/sfw/sbin:/usr/pkg/bin:/usr/pkg/sbin:/sw/bin:/usr/freeware/bin:/usr/local/apache/bin:/usr/local/apache/sbin:/boot/common/bin:/resources/index/bin)
					if ( test -n "$APACHEHTTPD" )
					then
						break
					fi
				done
				APACHELINKFLAGS="-Wl,$BUNDLE_LOADER -Wl,$APACHEHTTPD"
			else
				APACHELINKFLAGS="$UNDEFINED_DYNAMIC_LOOKUP"
			fi
		fi
	else
		INCLUDE_APACHE="0"
	fi
else
	echo "disabled"
fi

AC_SUBST(APACHEINCLUDES)
AC_SUBST(APACHELINKFLAGS)
])



dnl checks for apache's apxs2
AC_DEFUN([FW_CHECK_APXS],
[

if ( test -n "$APXS" )
then
	AC_PATH_PROG(APXS,$APXS,"")
else
	if ( test "$cross_compiling" = "yes" )
	then
		echo "cross compiling"
	else
		AC_PATH_PROG(APXS2,apxs2,"",/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/opt/sfw/bin:/opt/sfw/sbin:/usr/pkg/bin:/usr/pkg/sbin:/sw/bin:/usr/freeware/bin:/usr/local/apache/bin:/usr/local/apache/sbin:/boot/common/bin:/resources/index/bin)
		if ( test -z "$APXS2" )
		then
			AC_PATH_PROG(APXS,apxs,"",/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/opt/sfw/bin:/opt/sfw/sbin:/usr/pkg/bin:/usr/pkg/sbin:/sw/bin:/usr/freeware/bin:/usr/local/apache/bin:/usr/local/apache/sbin:/boot/common/bin:/resources/index/bin)
		fi
		if ( test -n "$APXS2" )
		then
			APXS="$APXS2"
		fi
	fi
fi

if ( test -n "$APXS" )
then
	APACHEINCLUDES="-I`$APXS -q INCLUDEDIR`"
fi

])



dnl checks for apache2's apr-1-mt-config, apr-1-config or apr-config
AC_DEFUN([FW_CHECK_APRCONFIG],
[

if ( test -n "$APR_CONFIG" )
then
	AC_PATH_PROG(APR_CONFIG,$APR_CONFIG,"")
else
	if ( test "$cross_compiling" = "yes" )
	then
		echo "cross compiling"
	else
		dnl NOTE that we prefer /usr/bin and /usr/sbin to /bin and
		dnl /sbin for these.  On fedora37+ (and maybe other platforms)
		dnl these scripts are found in /usr/bin and in /bin but if you
		dnl run /bin/apr-1-config it thinks you're cross compiling
		dnl because you called it by "absolute path, but not installed
		dnl path".  It then prepends the APR_TARGET_DIR (which it
		dnl appears to miscalculate) to the includedir.  So, we'll
		dnl prefer /usr/bin and /usr/sbin to /bin and /sbin for these
		dnl scripts.  This doesn't appear to break older platforms.
		AC_PATH_PROG(APR_1_MT_CONFIG,apr-1-mt-config,"",/usr/bin:/usr/sbin:/bin:/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/apache/bin:/usr/local/apache/sbin:/usr/pkg/bin:/usr/pkg/sbin:/boot/common/bin)
		if ( test -z "$APR_1_MT_CONFIG" )
		then
			AC_PATH_PROG(APR_1_CONFIG,apr-1-config,"",/usr/bin:/usr/sbin:/bin:/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/apache/bin:/usr/local/apache/sbin:/usr/pkg/bin:/usr/pkg/sbin:/boot/common/bin:/resources/index/bin)
			if ( test -z "$APR_1_CONFIG" )
			then
				AC_PATH_PROG(APR_CONFIG,apr-config,"",/usr/bin:/usr/sbin:/bin:/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/apache/bin:/usr/local/apache/sbin:/usr/pkg/bin:/usr/pkg/sbin:/boot/common/bin:/resources/index/bin)
			fi
		fi
		if ( test -n "$APR_1_CONFIG" )
		then
			APR_CONFIG="$APR_1_CONFIG"
		fi
		if ( test -n "$APR_1_MT_CONFIG" )
		then
			APR_CONFIG="$APR_1_MT_CONFIG"
		fi
	fi
fi

if ( test -n "$APR_CONFIG" )
then
	APACHEINCLUDES="$APACHEINCLUDES `$APR_CONFIG --cppflags --includes | sed -e 's|-mt ||'` -DAPACHE_2"
fi
])



dnl checks for apache2's apr-1-mt-config, apr-1-config or apr-config
AC_DEFUN([FW_CHECK_APUCONFIG],
[

if ( test -n "$APU_CONFIG" )
then
	AC_PATH_PROG(APU_CONFIG,$APU_CONFIG,"")
else
	if ( test "$cross_compiling" = "yes" )
	then
		echo "cross compiling"
	else
		AC_PATH_PROG(APU_1_MT_CONFIG,apu-1-mt-config,"",/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/apache/bin:/usr/local/apache/sbin:/usr/pkg/bin:/usr/pkg/sbin:/boot/common/bin)
		if ( test -z "$APU_1_MT_CONFIG" )
		then
			AC_PATH_PROG(APU_1_CONFIG,apu-1-config,"",/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/apache/bin:/usr/local/apache/sbin:/usr/pkg/bin:/usr/pkg/sbin:/boot/common/bin:/resources/index/bin)
			if ( test -z "$APU_1_CONFIG" )
			then
				AC_PATH_PROG(APU_CONFIG,apu-config,"",/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/apache/bin:/usr/local/apache/sbin:/usr/pkg/bin:/usr/pkg/sbin:/boot/common/bin:/resources/index/bin)
			fi
		fi
		if ( test -n "$APU_1_CONFIG" )
		then
			APU_CONFIG="$APU_1_CONFIG"
		fi
		if ( test -n "$APU_1_MT_CONFIG" )
		then
			APU_CONFIG="$APU_1_MT_CONFIG"
		fi
	fi
fi

if ( test -n "$APU_CONFIG" )
then
	APACHEINCLUDES="$APACHEINCLUDES `$APU_CONFIG --includes | sed -e 's|-mt ||'`"
fi
])



dnl checks to see if the -Wno-long-double option compiler works or not
dnl if it does, then WNOLONGDOUBLE="-Wno-long-double" is set
dnl if it does not, then WNOLONGDOUBLE="" is set
AC_DEFUN([FW_CHECK_WNOLONGDOUBLE],
[
AC_MSG_CHECKING(for -Wno-long-double option)
FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-Wno-long-double],[],[],[WNOLONGDOUBLE="-Wno-long-double"],[WNOLONGDOUBLE=""])
if ( test -n "$WNOLONGDOUBLE" )
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(WNOLONGDOUBLE)
])


dnl checks to see if the -Wno-unknown-pragmas compiler option works or not
dnl (old versions of gcc with old versions of java might need this)
dnl if it does, then WNOUNKNOWNPRAGMAS="-Wno-unknown-pragmas" is set
dnl if it does not, then WNOUNKNOWNPRAGMAS="" is set
AC_DEFUN([FW_CHECK_WNOUNKNOWNPRAGMAS],
[
AC_MSG_CHECKING(for -Wno-unknown-pragmas option)
FW_TRY_LINK([#include <stdio.h>],[printf("hello");],[-Wno-unknown-pragmas],[],[],[WNOUNKNOWNPRAGMAS="-Wno-unknown-pragmas"],[WNOUNKNOWNPRAGMAS=""])
if ( test -n "$WNOUNKNOWNPRAGMAS" )
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(WNOUNKNOWNPRAGMAS)
])


dnl checks to see if the -Wno-error=date-time compiler option works or not
dnl if it does, then WNOERRORDATETIME="-Wno-error=date-time" is set
dnl if it does not, then WNOERRORDATETIME="" is set
AC_DEFUN([FW_CHECK_WNOERRORDATETIME],
[
AC_MSG_CHECKING(for -Wno-error=date-time option)
FW_TRY_LINK([#include <stdio.h>],[printf("%s %s\n",__DATE__,__TIME__);],[-Wall -Werror -Wno-error=date-time],[],[],[WNOERRORDATETIME="-Wno-error=date-time"],[WNOERRORDATETIME=""])
if ( test -n "$WNOERRORDATETIME" )
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(WNOERRORDATETIME)
])


dnl checks to see if the -Wno-string-plus-int compiler option works or not
dnl if it does, then WNOSTRINGPLUSINT="-Wno-string-plus-int" is set
dnl if it does not, then WNOSTRINGPLUSINT="" is set
AC_DEFUN([FW_CHECK_WNOSTRINGPLUSINT],
[
AC_MSG_CHECKING(for -Wno-string-plus-int option)
FW_TRY_LINK([#include <stdio.h>],[const char *a="12345"; unsigned short i=1; const char *b; b=a+i+1;],[-Wall -Werror -Wno-string-plus-int],[],[],[WNOSTRINGPLUSINT="-Wno-string-plus-int"],[WNOSTRINGPLUSINT=""])
if ( test -n "$WNOSTRINGPLUSINT" )
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(WNOSTRINGPLUSINT)
])


dnl checks to see if -fdeclspec compiler option works or not
dnl if it does, then it sets the variable FDECLSPEC="-fdeclspec"
dnl if it does not , then it sets the variable FDECLSPEC=""
AC_DEFUN([FW_CHECK_FDECLSPEC],
[
AC_MSG_CHECKING(for -fdeclspec option)
FW_TRY_LINK([],[int a; a=1;],[-fdeclspec],[],[],[FDECLSPEC="-fdeclspec"],[FDECLSPEC=""])
if ( test -n "$FDECLSPEC" )
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(FDECLSPEC)
])




dnl checks for minix platform and adds some defines to CPPFLAGS if it is
AC_DEFUN([FW_CHECK_MINIX],
[
AC_MSG_CHECKING(for minix)
MINIX=""
case $host_os in
	*minix* )
		MINIX="yes"
		CPPFLAGS="$CPPFLAGS -D_MINIX -D_POSIX_SOURCE -D_NETBSD_SOURCE -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED"
		AC_DEFINE(_MINIX,1,Minix)
		AC_MSG_RESULT(yes)
		;;
	* )
		AC_MSG_RESULT(no)
		;;
esac
AC_SUBST(MINIX)
])


dnl checks for haiku platform
dnl if it is, then prefix="/boot/common" is set unless it was already set,
dnl BELIB="-lbe", and GNULIB="-lgnu" are also set
AC_DEFUN([FW_CHECK_HAIKU],
[
AC_MSG_CHECKING(for haiku)
HAIKU=""
BELIB=""
GNULIB=""
case $host_os in
	*haiku* )
		HAIKU="yes"
		if ( test "$prefix" = "NONE" )
		then
			prefix="/boot/common"
		fi
		BELIB="-lbe"
		GNULIB="-lgnu"
		AC_DEFINE(_HAIKU,1,Haiku OS)
		AC_MSG_RESULT(yes)
		;;
	* )
		AC_MSG_RESULT(no)
		;;
esac
AC_SUBST(BELIB)
AC_SUBST(GNULIB)
AC_SUBST(HAIKU)
])


dnl checks for syllable platform
dnl if it is, then prefix="/resources/firstworks" is set unless it was already
dnl set, RUDIMENTS_DISABLE_FIONBIO=1 is defined, and _SYLLABLE=1 is defined
AC_DEFUN([FW_CHECK_SYLLABLE],
[
AC_MSG_CHECKING(for syllable)
SYLLABLE=""
case $host_os in
	*syllable* )
		SYLLABLE="yes"
		if ( test "$prefix" = "NONE" )
		then
			prefix="/resources/firstworks"
		fi
		AC_DEFINE(_SYLLABLE,1,Syllable OS)
		AC_MSG_RESULT(yes)
		;;
	* )
		AC_MSG_RESULT(no)
		;;
esac
AC_SUBST(SYLLABLE)
])


dnl checks for SCO platform
dnl if it is then:
dnl 	sets SCO="yes"
dnl 	sets SCO_OSR="yes"
dnl	sets ENABLE_RUDIMENTS_THREADS="no"
dnl	defines RUDIMENTS_HAVE_SCO_AVENRUN=1
dnl if it's 6.0.0 then:
dnl 	sets SCO_OSR6="yes"
dnl 	adds -D__STDC__=0 to CPPFLAGS
dnl 	sets CRTLIB="-lcrt"
dnl 	defines _SCO_OSR6
dnl if it's < 6.0.0 then:
dnl 	sets SCO_OSR5="yes"
dnl	defines RUDIMENTS_HAVE_BAD_SCO_MSGHDR=1
dnl	if it's 5.0.0 then:
dnl 		adds -D_SVID3 to CPPFLAGS
dnl 	defines _SCO_OSR5
dnl if it's UnixWare then:
dnl 	sets SCO_UW="yes"
dnl 	defines _SCO_UW
AC_DEFUN([FW_CHECK_SCO],
[
SCO_OSR5=""
SCO_OSR6=""
SCO_UW=""
CRTLIB=""

AC_MSG_CHECKING(for SCO)
if ( test "`uname -s 2> /dev/null`" = "SCO_SV" )
then
	dnl check for OSR6
	if ( test "`uname -v | tr -d '.'`" -eq "600" )
	then
		SCO_OSR6="yes"
		CPPFLAGS="$CPPFLAGS -D__STDC__=0"
		CRTLIB="-lcrt"
		AC_DEFINE(_SCO_OSR6,1,SCO OSR6 OS)
		AC_MSG_RESULT(OSR6)

	dnl check for OSR5
	dnl FIXME: detect versions older than OSR5
	else
		SCO_OSR5="yes"

		dnl OSR 5.0.0 needs -D_SVID3
		if ( test "`uname -v`" = "2" )
		then
			CPPFLAGS="$CPPFLAGS -D_SVID3"
		fi

		AC_DEFINE(_SCO_OSR5,1,SCO OSR5 OS)
		AC_MSG_RESULT(OSR5)
	fi

elif ( test "`uname -s 2> /dev/null`" = "UnixWare" -a "`uname os_provider 2> /dev/null`" = "SCO" )
then
	SCO_UW="yes"
	AC_DEFINE(_SCO_UW,1,SCO UnixWare OS)
	AC_MSG_RESULT(UnixWare)
else
	AC_MSG_RESULT(no)
fi

AC_SUBST(SCO_OSR5)
AC_SUBST(SCO_OSR6)
AC_SUBST(SCO_UW)
AC_SUBST(CRTLIB)
])


dnl checks for Irix platform
dnl if it is then:
dnl 	adds -D_XOPEN_SOURCE=500 to CPPFLAGS
dnl if also using the native compiler then:
dnl 	adds -D__SGICXX -diag_error 1035 -LANG:ansi-for-init-scope=on to
dnl 	CPPFLAGS
AC_DEFUN([FW_CHECK_IRIX],
[
IRIX=""
AC_MSG_CHECKING(for irix)
case $host_os in
	*irix* )
		IRIX="yes"
		AC_DEFINE(_IRIX,1,Irix OS)
		CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=500"
		if ( test "$CXX" = "CC" )
		then
			# define __SGICXX and interpret #error as an error
			CPPFLAGS="$CPPFLAGS -D__SGICXX -diag_error 1035 -LANG:ansi-for-init-scope=on"
		fi
		AC_MSG_RESULT(yes)
		;;
	* )
		AC_MSG_RESULT(no)
		;;
esac
AC_SUBST(IRIX)
])


dnl checks for illumos platform
dnl if it is, then _ILLUMOS=1 is defined
AC_DEFUN([FW_CHECK_ILLUMOS],
[
ILLUMOS=""
AC_MSG_CHECKING(for illumos)
if ( test -n "`uname -v 2> /dev/null | grep -i illumos`" )
then
	ILLUMOS="yes"
	AC_DEFINE(_ILLUMOS,1,IllumOS)
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(ILLUMOS)
])


dnl checks for HP-UX platform
dnl if it is, then it adds -D_XOPEN_SOURCE=500 to CPPFLAGS and disables -Werror
AC_DEFUN([FW_CHECK_HPUX],
[
HPUX=""
AC_MSG_CHECKING(for hp-ux)
case $host_os in
	*hpux* )
		HPUX="yes"
		CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=500"
		ENABLE_WERROR=""
		AC_MSG_RESULT(yes)
		AC_MSG_CHECKING([for -Wa,-w36])
		FW_TRY_COMPILE([#include <stdio.h>],[printf("hello");],[-Wa,-w36],[CXXFLAGS="$CXXFLAGS -Wa,-w36"; AC_MSG_RESULT(yes)],[ AC_MSG_RESULT(no)])
		AC_DEFINE(_HPUX,1,HP-UX OS)
		;;
	* )
		AC_MSG_RESULT(no)
		;;
esac
AC_SUBST(HPUX)
])


dnl check for x64 platform
dnl if it is, then X64="x64" is set
dnl if it is not, then X64="" is set
AC_DEFUN([FW_CHECK_X64],
[
X64=""
if ( test "$host_cpu" = "ia64" -o "$host_cpu" = "x86_64" -o "$host_cpu" = "amd64" )
then
	X64="x64"
fi
])



dnl checks for the pthreads library
dnl requires:  PTHREADPATH, RPATHFLAG, cross_compiling
dnl sets: HAVE_PTHREAD, PTHREADINCLUDES, PTHREADLIB
AC_DEFUN([FW_CHECK_PTHREAD],
[

AC_MSG_CHECKING(if -pthread works during compile phase)
if ( test -n "`$CXX -pthread 2>&1 | grep 'unrecognized option' | grep pthread`" )
then
	PTHREAD_COMPILE=""
else
	PTHREAD_COMPILE="-pthread"
fi
if ( test -n "$PTHREAD_COMPILE" )
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

HAVE_PTHREAD=""
PTHREADINCLUDES=""
PTHREADLIB=""

if ( test "$cross_compiling" = "yes" )
then

	dnl cross compiling
	echo "cross compiling"

	if ( test -z "$MINGW32" )
	then
		if ( test -n "$PTHREADPATH" )
		then
			PTHREADINCLUDES="$PTHREAD_COMPILE -I$PTHREADPATH/include"
			PTHREADLIB="-L$PTHREADPATH/lib -lpthread -pthread"
		else
			PTHREADINCLUDES="$PTHREAD_COMPILE"
			PTHREADLIB="-lpthread -pthread"
		fi
	fi
	HAVE_PTHREAD="yes"

else

	dnl check pthread.h and standard thread libraries
	for i in "pthread" "thread" "pthreads" "gthreads" ""
	do
		if ( test -n "$i" )
		then
			AC_MSG_CHECKING(for lib$i)
		else
			AC_MSG_CHECKING(for no library)
		fi

		INCLUDEDIR="pthread"
		if ( test "$i" = "gthreads" )
		then
			INCLUDEDIR="FSU"
		fi

		if ( test -n "$i" )
		then
			FW_CHECK_HEADERS_AND_LIBS([$PTHREADPATH],[$INCLUDEDIR],[pthread.h],[$i],[""],[""],[PTHREADINCLUDES],[PTHREADLIB],[PTHREADLIBPATH],[PTHREADSTATIC])
		fi

		if ( test -n "$PTHREADLIB" -o -z "$i" )
		then

			AC_MSG_RESULT(yes)

			dnl  If we found a set of headers and libs, try
			dnl  linking with them a bunch of different ways
			for try in 1 2 3 4 5 6 7 8
			do

				if ( test "$try" = "1" )
				then
					dnl for minix
					TESTINCLUDES="$PTHREADINCLUDES"
					TESTLIB="-lmthread $PTHREADLIB"
				elif ( test "$try" = "2" )
				then
					dnl for minix
					TESTINCLUDES="$PTHREAD_COMPILE $PTHREADINCLUDES"
					TESTLIB="-lmthread $PTHREADLIB"
				elif ( test "$try" = "3" )
				then
					TESTINCLUDES="$PTHREADINCLUDES"
					TESTLIB="$PTHREADLIB"
				elif ( test "$try" = "4" )
				then
					TESTINCLUDES="$PTHREADINCLUDES"
					TESTLIB="$PTHREADLIB -pthread"
				elif ( test "$try" = "5" )
				then
					TESTINCLUDES="$PTHREAD_COMPILE $PTHREADINCLUDES"
					TESTLIB="$PTHREADLIB"
				elif ( test "$try" = "6" )
				then
					TESTINCLUDES="$PTHREAD_COMPILE $PTHREADINCLUDES"
					TESTLIB="$PTHREADLIB -pthread"
				elif ( test "$try" = "7" )
				then
					TESTINCLUDES="$PTHREADINCLUDES"
					TESTLIB="-pthread"
				elif ( test "$try" = "8" )
				then
					TESTINCLUDES="$PTHREAD_COMPILE $PTHREADINCLUDES"
					TESTLIB="-pthread"
				fi

				HAVE_PTHREAD=""
				dnl try to link
				AC_MSG_CHECKING(whether $TESTINCLUDES ... $TESTLIB works)
				FW_TRY_LINK([#include <stddef.h>
#include <pthread.h>],[pthread_exit(NULL);],[$CPPFLAGS $TESTINCLUDES],[$TESTLIB],[],[AC_MSG_RESULT(yes); HAVE_PTHREAD="yes"],[AC_MSG_RESULT(no)])

				dnl  If the link succeeded then keep
				dnl  the flags.
				if ( test -n "$HAVE_PTHREAD" )
				then
					PTHREADINCLUDES="$TESTINCLUDES"
					PTHREADLIB="$TESTLIB"
					break
				fi

				dnl  If the link failed, reset the flags
				TESTLIB=""
				TESTINCLUDES=""
			done

			if ( test -n "$HAVE_PTHREAD" )
			then
				break
			fi
		else
			AC_MSG_RESULT(no)
		fi
	done
fi

FW_INCLUDES(pthreads,[$PTHREADINCLUDES])
FW_LIBS(pthreads,[$PTHREADLIB])

AC_SUBST(PTHREADINCLUDES)
AC_SUBST(PTHREADLIB)
])


dnl check to see what socket library combination is required
dnl sets: SOCKETLIBS
AC_DEFUN([FW_CHECK_SOCKET_LIBS],
[

	AC_MSG_CHECKING(for socket libraries)

	AC_LANG_SAVE
	AC_LANG(C)
	SOCKETLIBS=""
	DONE=""
	for i in "" "-lnsl" "-lsocket" "-lsocket -lnsl" "-lxnet" "-lwsock32 -lws2_32 -lnetapi32" "-lnetwork"
	do
		FW_TRY_LINK([
void connect(int,void *,int);
void listen(int,int);
void bind(int,void *,int);
void accept(int,void *,int);
void send(int,void *,int,int);
void sendto(int,void *,int,int,void *,int);
void gethostbyname(void *);
],[
connect(0,0,0);
listen(0,0);
bind(0,0,0);
accept(0,0,0);
send(0,0,0,0);
sendto(0,0,0,0,0,0);
gethostbyname(0);
],[$CPPFLAGS],[$i],[],[SOCKETLIBS="$i"; DONE="yes"],[])
		if ( test -n "$DONE" )
		then
			break
		fi
	done
	AC_LANG_RESTORE

	if ( test -z "$DONE" )
	then
		AC_MSG_ERROR(no combination of networking libraries was found.)
	fi

	if ( test -z "$SOCKETLIBS" )
	then
		AC_MSG_RESULT(none)
	else
		AC_MSG_RESULT($SOCKETLIBS)
	fi

	AC_SUBST(SOCKETLIBS)
])


dnl fixes various libtool issues
AC_DEFUN([FW_FIX_LIBTOOL],
[

dnl Some versions of NetBSD create a libtool with -lgcc_s -lgcc in it, but don't
dnl provide a shared libgcc.
dnl Also, some versions of NetBSD create a libtool with -lgcc_s_pic in it, but
dnl don't provide a shraed libgcc_s_pic.
dnl These cause all kinds of link failures.  Fix them
if ( test -r "libtool" )
then
	sed -e "s|-lgcc_s -lgcc |-lgcc_s |g" \
		-e "s|-lgcc_s -lgcc\"|-lgcc_s\"|g" \
		-e "s|-lgcc_s_pic|-lgcc_s|g" \
		libtool > libtool.new
	mv libtool.new libtool
fi

dnl On some versions of OS X, the libtool config erroneusly finds "Print" and
dnl tries to use it as "print -r --".  Replace this with "printf %s\\n".
if ( test -r "libtool" -a -n "$DARWIN" )
then
	sed -e "s|print -r --|printf %s\\\\\\\\n|g" libtool > libtool.new
	mv libtool.new libtool
fi

dnl On SCO UnixWare...
if ( test -r "libtool" -a -n "$SCO_UW" )
then
	dnl The linker doesn't support -R at all
	sed -e "s|hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=\"\"|g" libtool > libtool.new
	mv libtool.new libtool

	dnl And libtool wants to use the -Wl,-h option which causes all kinds
	dnl of problems.
	sed -e "s|\\\\\$wl-h,\\\\\$soname ||g" libtool > libtool.new
	mv libtool.new libtool
fi
])


dnl set project, soname, or other version
dnl	$1 - variable that will be set to the project version
dnl	$2 - project version
AC_DEFUN([FW_SET_VERSION],
[
	dnl intuitive versioning
	dnl Given a version number MAJOR.MINOR.PATCH, increment the:
	dnl 1. MAJOR when you make very significant changes
	dnl    (and set MINOR and PATCH to 0)
	dnl 2. MINOR when you make backwards-incompatible changes
	dnl    or add significant new features
	dnl    (and set PATCH to 0)
	dnl 3. PATCH when you make backwards-compatible changes

	dnl Libtool Versioning
	dnl For CURRENT:REVISION:AGE version info,
	dnl apply the following rules in order:
	dnl if library source changed at all,         c:r:a -> c:r+1:a
	dnl if interfaces added, removed, or changed, c:r:a -> c+1:0:a
	dnl if interfaces added,                      c:r:a -> c:r:a+1
	dnl if interfaces removed,                    c:r:a -> c:r:0

	eval "$1=\"$2\""
	AC_SUBST([$1])
	if ( test "$1" != "SONAME_VERSION_INFO" )
	then
		AC_DEFINE([$1],["$2"],Version)
	fi
])
dnl checks for the rudiments library
dnl requires:
dnl 	cross_compiling, RUDIMENTSPREFIX
dnl sets:
dnl	RUDIMENTSPREFIX, RUDIMENTSCFLAGS, RUDIMENTSLIBS, RUDIMENTSVERSION
AC_DEFUN([FW_CHECK_RUDIMENTS],
[

RUDIMENTSVERSION=""
RUDIMENTSLIBS=""
RUDIMENTSCFLAGS=""

if ( test "$cross_compiling" = "yes" )
then

	dnl cross compiling
	echo "cross compiling"
	if ( test -n "$RUDIMENTSPREFIX" )
	then
		RUDIMENTSCONFIG="$RUDIMENTSPREFIX/bin/rudiments-config"
		if ( test -r "$RUDIMENTSCONFIG" )
		then
			RUDIMENTSVERSION="`$RUDIMENTSCONFIG --version`"
			RUDIMENTSCFLAGS="`$RUDIMENTSCONFIG --cflags`"
			RUDIMENTSLIBS="`$RUDIMENTSCONFIG --libs`"
		else
			RUDIMENTSVERSION=""
			RUDIMENTSCFLAGS="-I$RUDIMENTSPREFIX/include"
			RUDIMENTSLIBS="-L$RUDIMENTSPREFIX/lib -lrudiments"
		fi
	fi

else

	FW_SEARCH_PATHS([$RUDIMENTSPREFIX],[rudiments],[bin],[SEARCHPATHS])
	for i in $SEARCHPATHS
	do
		RUDIMENTSCONFIG="$i/rudiments-config"
		if ( test -r "$RUDIMENTSCONFIG" )
		then
			RUDIMENTSVERSION="`$RUDIMENTSCONFIG --version`"
			RUDIMENTSCFLAGS="`$RUDIMENTSCONFIG --cflags`"
			RUDIMENTSLIBS="`$RUDIMENTSCONFIG --libs`"
		fi
		if ( test -n "$RUDIMENTSLIBS" )
		then
			break
		fi
	done
fi

if ( test -z "$RUDIMENTSLIBS" )
then
	AC_MSG_ERROR(Rudiments not found.  SQL-Relay requires this package.)
	exit
fi

FW_INCLUDES(rudiments,[$RUDIMENTSCFLAGS])
FW_LIBS(rudiments,[$RUDIMENTSLIBS])

AC_SUBST(RUDIMENTSPREFIX)
AC_SUBST(RUDIMENTSCFLAGS)
AC_SUBST(RUDIMENTSLIBS)
])

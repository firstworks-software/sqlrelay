' expands the server-side include directives in .html.in files, writing each
' one back out without its .in suffix
'
' usage: cscript /nologo ssi.vbs file.html.in [file.html.in ...]
'
' a directive looks like:
'	<!--#include virtual="path" -->
' and is replaced by the contents of the file it names.  the path is relative
' to the directory of the file holding the directive, the way apache's
' mod_include treats virtual.  when the named file has a .in counterpart, the
' .in file is expanded instead, so the order the files are generated in does
' not matter.
'
' this is the windows counterpart of ssi.sh, and must keep producing the same
' output that it does

' create file system object
set fso=CreateObject("Scripting.FileSystemObject")

' writes a message to stderr and gives up
sub die(message)
	fso.GetStandardStream(2).WriteLine("ssi.vbs: " & message)
	WScript.Quit 1
end sub

' returns the contents of a file, with its include directives expanded
' filename - the file to expand
' chain - the chain of files currently being expanded
'
' the arguments are byval because vbscript passes byref by default, and the
' recursion below would otherwise let a nested call rewrite its caller's copy
function expandfile(byval filename, byval chain)

	dim key
	dim infile
	dim content
	dim parent
	dim result
	dim pos
	dim start
	dim finish
	dim directive
	dim pathstart
	dim pathend
	dim target
	dim targetfile

	' catch an include cycle before it runs away
	key="|" & lcase(fso.GetAbsolutePathName(filename)) & "|"
	if instr(chain,key)>0 then
		die "include cycle at " & filename
	end if

	if not fso.FileExists(filename) then
		die "no such file: " & filename
	end if

	' read the file
	content=""
	set infile=fso.OpenTextFile(filename)
	if not infile.AtEndOfStream then
		content=infile.ReadAll()
	end if
	infile.Close

	chain=chain & key
	parent=fso.GetParentFolderName(fso.GetAbsolutePathName(filename))

	' copy the content out, swapping each directive for what it names
	result=""
	pos=1
	do
		start=instr(pos,content,"<!--#include")
		if start=0 then
			exit do
		end if

		finish=instr(start,content,"-->")
		if finish=0 then
			die "unterminated include directive in " & filename
		end if

		' pull the path out of the directive
		directive=mid(content,start,finish-start+3)
		pathstart=instr(directive,"virtual=""")
		if pathstart=0 then
			die "include directive with no path in " & filename
		end if
		pathstart=pathstart+9
		pathend=instr(pathstart,directive,"""")
		if pathend=0 then
			die "unterminated include path in " & filename
		end if
		target=mid(directive,pathstart,pathend-pathstart)

		' convert slashes to backslashes and resolve against the
		' directory of the file holding the directive
		targetfile=parent & "\" & replace(target,"/","\",1,-1,0)
		targetfile=fso.GetAbsolutePathName(targetfile)

		' prefer the .in file, when there is one
		if fso.FileExists(targetfile & ".in") then
			targetfile=targetfile & ".in"
		end if

		result=result & mid(content,pos,start-pos)
		result=result & expandfile(targetfile,chain)

		pos=finish+3
	loop
	result=result & mid(content,pos)

	expandfile=result
end function

' sanity check
if WScript.Arguments.Count<1 then
	fso.GetStandardStream(2).WriteLine("usage: ssi.vbs file.html.in [file.html.in ...]")
	WScript.Quit 1
end if

for argument=0 to WScript.Arguments.Count-1

	infilename=WScript.Arguments.Item(argument)

	' collapse backslashes and convert slashes to backslashes
	infilename=replace(infilename,"\\","\",1,-1,0)
	infilename=replace(infilename,"/","\",1,-1,0)

	if lcase(right(infilename,3))<>".in" then
		die "input file name does not end in .in: " & infilename
	end if

	outfilename=left(infilename,len(infilename)-3)

	' expand first, so a failure leaves no half finished file behind
	expanded=expandfile(infilename,"")

	set outfile=fso.CreateTextFile(outfilename,true)
	outfile.Write(expanded)
	outfile.Close
next

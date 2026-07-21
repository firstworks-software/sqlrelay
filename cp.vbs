' sanity check
if WScript.Arguments.Count < 2 then
	WScript.quit
end if

' create file system object
set fso=CreateObject("Scripting.FileSystemObject")

' get the destination
dest=WScript.Arguments.Item(WScript.Arguments.Count-1)

' redirect the destination under DESTDIR, if set, for a staged install
dest=applydestdir(dest)

' if it's a folder, append a backslash
if fso.FolderExists(dest) then
	dest=dest+"\"
end if

' collapse backslashes and convert slashes to backslashes
dest=replace(dest,"\\","\",1,-1,0)
dest=replace(dest,"/","\",1,-1,0)

' copy source files to destination...
for i=0 to WScript.Arguments.Count-2

	' get a source file
	source=WScript.Arguments.Item(i)

	' collapse backslashes and convert slashes to backslashes
	source=replace(source,"\\","\",1,-1,0)
	source=replace(source,"/","\",1,-1,0)

	' copy the file
	call fso.CopyFile(source,dest)
next

' redirect an absolute destination path under %DESTDIR% (for a staged install).
' the drive letter is kept as a directory, so "C:\a\b" becomes
' "%DESTDIR%\C\a\b".  with no DESTDIR set the path is returned unchanged, so
' normal installs are unaffected.
function applydestdir(p)
	dim dd
	dd=CreateObject("WScript.Shell").Environment("Process").Item("DESTDIR")
	if len(dd)=0 then
		applydestdir=p
		exit function
	end if
	p=replace(p,"/","\")
	if len(p)>=2 and mid(p,2,1)=":" then
		p=left(p,1)+mid(p,3)
	end if
	do while left(p,1)="\"
		p=mid(p,2)
	loop
	applydestdir=dd+"\"+p
end function

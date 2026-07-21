' sanity check
if WScript.Arguments.Count < 1 then
	WScript.quit
end if

' create file system object
set fso=CreateObject("Scripting.FileSystemObject")

' get the directory to create
fullpath=WScript.Arguments.Item(0)

' redirect under DESTDIR, if set, for a staged install
fullpath=applydestdir(fullpath)

' collapse backslashes and convert slashes to backslashes
fullpath=replace(fullpath,"\\","\",1,-1,0)
fullpath=replace(fullpath,"/","\",1,-1,0)

' split on backslashes
parts=split(fullpath,"\")

' create directories
path=""
for i=lbound(parts) to ubound(parts)

	' build path
	if strcomp(path,"")=0 then
		path=parts(i)
	else
		path=path+"\"+parts(i)
	end if

	' create the directory unless it already exists
	if fso.FolderExists(path)=false then
		call fso.CreateFolder(path)
	end if
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

' Removes a directory only when it is already empty, the way unix rmdir does.
' This is deliberately not rmtree.vbs.  The makefiles use RMDIR for tidy-up
' lines that must leave a non-empty directory alone; a recursive delete there
' wipes files the makefile never installed.  See #8682.

' sanity check
if WScript.Arguments.Count<1 then
	WScript.quit
end if

' create file system object
set fso=CreateObject("Scripting.FileSystemObject")

' remove folder
for i=0 to WScript.Arguments.Count-1

	' get the folder to remove
	folder=WScript.Arguments.Item(i)

	' collapse backslashes and convert slashes to backslashes
	folder=replace(folder,"\\","\",1,-1,0)
	folder=replace(folder,"/","\",1,-1,0)

	' remove the folder, if it's empty (ignoring errors)
	on error resume next
	err.clear
	set fldr=fso.GetFolder(folder)
	if err.number=0 then
		if fldr.Files.Count=0 and fldr.SubFolders.Count=0 then
			call fso.DeleteFolder(folder)
		end if
	end if
next

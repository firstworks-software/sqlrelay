prefix = Session.Property("CustomActionData")

Set wshShell = CreateObject("WScript.Shell")

Set wshSystemEnv = wshShell.Environment("SYSTEM")

' update the PATH environment variable, unless it's already there.  a repair,
' or any maintenance run, runs this action again, and an unguarded append
' leaves a duplicate entry behind each time.  the semicolons make it a
' whole-entry match, since one directory's name can be a prefix of another's.
bindir = prefix & "bin"
syspath = wshSystemEnv("PATH")
if InStr(1,";" & syspath & ";",";" & bindir & ";",1)=0 then
	wshSystemEnv("PATH") = syspath & ";" & bindir
end if

' update permissions on some folders
wshShell.run "cacls.exe """ & prefix & "var\sqlrelay\cache"" /E /G Everyone:C"
wshShell.run "cacls.exe """ & prefix & "var\sqlrelay\debug"" /E /G Everyone:C"
wshShell.run "cacls.exe """ & prefix & "var\sqlrelay\log"" /E /G Everyone:C"
wshShell.run "cacls.exe """ & prefix & "var\sqlrelay\tmp"" /E /G Everyone:C"
wshShell.run "cacls.exe """ & prefix & "var\sqlrelay\tmp\ipc"" /E /G Everyone:C"
wshShell.run "cacls.exe """ & prefix & "var\sqlrelay\tmp\pids"" /E /G Everyone:C"
wshShell.run "cacls.exe """ & prefix & "var\sqlrelay\tmp\sockets"" /E /G Everyone:C"

Set wshSystemEnv = Nothing
Set wshShell = Nothing

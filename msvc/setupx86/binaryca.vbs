' binaryca.vbs - give a built MSI its script custom actions, run from its own
' Binary table.
'
' usage:
'   cscript //nologo binaryca.vbs <msi>
'
' A .vdproj can only point a custom action at a file the MSI installs, so the
' installed copy had to survive for as long as the product did - and "nmake
' uninstall" deleted it, leaving the product impossible to uninstall (#8682).
' Storing the script in the Binary table instead makes the custom action
' self-contained, but no .vdproj setting can express that, so the actions are
' written here, after devenv builds the MSI.
'
' The setup project used to declare them and this script only repointed them,
' which meant the scripts had to stay in the install set purely to be named by
' the .vdproj.  They no longer are, so the CustomAction and
' InstallExecuteSequence rows are authored here too (#8690).
'
' The types and sequence numbers below are the ones the setup project used to
' generate.  The conditions are not: those were component-state conditions
' naming the component of the script itself, which no longer exists, so they
' are re-expressed in terms of REMOVE.

Option Explicit

Dim msiOpenDatabaseModeTransact, msiViewModifyUpdate
msiOpenDatabaseModeTransact = 1
msiViewModifyUpdate         = 4

' script, action identifier, type, sequence, condition, customactiondata.
' an entry with customactiondata also gets the .SetProperty companion action
' that carries it, one sequence number ahead, on the same condition.
Dim ACTIONS
ACTIONS = Array( _
	Array("pathuninstall.vbs", _
		"_C9E0E1C7_8042_4051_9180_D47EA7BA03E5", _
		3078, 1699, "REMOVE=""ALL""", ""), _
	Array("pathinstall.vbs", _
		"_A7572FA6_28FA_4791_BAF5_9EF849377208", _
		3590, 5999, "NOT REMOVE", "[TARGETDIR]"))

Dim fso : Set fso = CreateObject("Scripting.FileSystemObject")
Dim installer, db

' cscript exits 0 on an unhandled runtime error, which would let a broken edit
' pass for a good build, so the work is trapped and its failure made explicit
On Error Resume Next
Main
If Err.Number <> 0 Then
	WScript.StdErr.WriteLine "binaryca.vbs: " & Err.Description & _
					" (0x" & Hex(Err.Number) & ")"
	WScript.Quit 1
End If

' ============================ subroutines ============================

Sub Main

	Dim args : Set args = WScript.Arguments
	If args.Count < 1 Then
		WScript.StdErr.WriteLine "usage: binaryca.vbs <msi>"
		WScript.Quit 1
	End If

	Set installer = CreateObject("WindowsInstaller.Installer")
	Set db = installer.OpenDatabase(fso.GetAbsolutePathName(args(0)), _
						msiOpenDatabaseModeTransact)

	' a setup project with no binary custom action leaves the table out
	If Not TableExists("Binary") Then
		RunSql "CREATE TABLE `Binary` (`Name` CHAR(72) NOT NULL, " & _
				"`Data` OBJECT NOT NULL PRIMARY KEY `Name`)"
	End If

	Dim i
	For i = 0 To UBound(ACTIONS)
		AddAction ACTIONS(i)
	Next

	' nothing is written to the msi until here
	db.Commit
	Set db = Nothing
	Set installer = Nothing
End Sub

Sub AddAction(a)

	Dim script, action, catype, seq, cond, cadata
	script = a(0) : action = a(1) : catype = a(2)
	seq    = a(3) : cond   = a(4) : cadata = a(5)

	Dim pth : pth = fso.GetAbsolutePathName(script)
	If Not fso.FileExists(pth) Then
		WScript.StdErr.WriteLine "binaryca.vbs: no such script: " & script
		WScript.Quit 1
	End If

	' (re)store the script in the Binary table
	RunSql "DELETE FROM `Binary` WHERE `Name`='" & script & "'"
	Dim bv, br
	Set bv = db.OpenView("INSERT INTO `Binary` (`Name`,`Data`) VALUES (?,?)")
	Set br = installer.CreateRecord(2)
	br.StringData(1) = script
	br.SetStream 2, pth
	bv.Execute br
	bv.Close
	Set bv = Nothing

	' the action itself, then the .SetProperty that carries its data
	AddCustomAction action, catype, script, ""
	AddSequence action, seq, cond
	If cadata <> "" Then
		AddCustomAction action & ".SetProperty", 51, action, cadata
		AddSequence action & ".SetProperty", seq - 1, cond
	End If

	WScript.StdErr.WriteLine action & " runs " & script & _
					" from the Binary table at " & seq
End Sub

Sub AddCustomAction(action, catype, src, tgt)
	Dim v, r
	RunSql "DELETE FROM `CustomAction` WHERE `Action`='" & action & "'"
	Set v = db.OpenView("INSERT INTO `CustomAction` " & _
			"(`Action`,`Type`,`Source`,`Target`) VALUES (?,?,?,?)")
	Set r = installer.CreateRecord(4)
	r.StringData(1) = action
	r.IntegerData(2) = catype
	r.StringData(3) = src
	If tgt <> "" Then r.StringData(4) = tgt
	v.Execute r
	v.Close
	Set v = Nothing
End Sub

Sub AddSequence(action, seq, cond)
	Dim v, r
	RunSql "DELETE FROM `InstallExecuteSequence` WHERE `Action`='" & action & "'"
	Set v = db.OpenView("INSERT INTO `InstallExecuteSequence` " & _
			"(`Action`,`Condition`,`Sequence`) VALUES (?,?,?)")
	Set r = installer.CreateRecord(3)
	r.StringData(1) = action
	r.StringData(2) = cond
	r.IntegerData(3) = seq
	v.Execute r
	v.Close
	Set v = Nothing
End Sub

Function TableExists(tbl)
	Dim v, r
	Set v = db.OpenView("SELECT `Name` FROM `_Tables` WHERE `Name`='" & tbl & "'")
	v.Execute
	Set r = v.Fetch
	TableExists = Not (r Is Nothing)
	v.Close
	Set v = Nothing
End Function

Sub RunSql(sql)
	Dim v
	Set v = db.OpenView(sql)
	v.Execute
	v.Close
	Set v = Nothing
End Sub

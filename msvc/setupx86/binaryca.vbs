' binaryca.vbs - move a built MSI's script custom actions into its Binary table.
'
' usage:
'   cscript //nologo binaryca.vbs <msi> <script> [<script> ...]
'
' Each <script> is a file the MSI both installs and runs as a custom action.
' A .vdproj can only point a custom action at an installed file (msi source
' type 16), so the installed copy has to survive for as long as the product
' does - and "nmake uninstall" deletes it, leaving the product impossible to
' uninstall (#8682).  Storing the script in the Binary table instead (source
' type 0) makes the custom action self-contained, but no .vdproj setting can
' express that, so the MSI is edited here, after devenv builds it.
'
' Only the source of the custom action changes.  Its identifier, its condition,
' and its place in the install sequence are whatever the setup project built,
' as are the in-script and no-impersonate bits of its type.

Option Explicit

Dim msiOpenDatabaseModeTransact, msiViewModifyUpdate, msiSourceTypeMask
msiOpenDatabaseModeTransact = 1
msiViewModifyUpdate         = 4
msiSourceTypeMask           = 48

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
	If args.Count < 2 Then
		WScript.StdErr.WriteLine _
			"usage: binaryca.vbs <msi> <script> [<script> ...]"
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
	For i = 1 To args.Count - 1
		Binarize args(i)
	Next

	' nothing is written to the msi until here
	db.Commit
	Set db = Nothing
	Set installer = Nothing
End Sub

Sub Binarize(scriptPath)

	Dim nm, pth, fkey, action
	nm = fso.GetFileName(scriptPath)
	pth = fso.GetAbsolutePathName(scriptPath)

	' the custom action's source is the installed file's key, until this
	' script has run once, and the binary name afterward
	fkey = FileKey(nm)
	action = ActionWithSource(fkey)
	If action = "" Then action = ActionWithSource(nm)
	If action = "" Then
		WScript.StdErr.WriteLine "binaryca.vbs: no custom action runs " & nm
		WScript.Quit 1
	End If

	' (re)store the script in the Binary table
	RunSql "DELETE FROM `Binary` WHERE `Name`='" & nm & "'"
	Dim bv, br
	Set bv = db.OpenView("INSERT INTO `Binary` (`Name`,`Data`) VALUES (?,?)")
	Set br = installer.CreateRecord(2)
	br.StringData(1) = nm
	br.SetStream 2, pth
	bv.Execute br
	bv.Close
	Set bv = Nothing

	' point the custom action at it
	Dim v, r
	Set v = db.OpenView("SELECT `Action`,`Type`,`Source`,`Target` FROM " & _
				"`CustomAction` WHERE `Action`='" & action & "'")
	v.Execute
	Set r = v.Fetch
	r.IntegerData(2) = r.IntegerData(2) And Not msiSourceTypeMask
	r.StringData(3) = nm
	v.Modify msiViewModifyUpdate, r
	v.Close
	Set v = Nothing

	WScript.StdErr.WriteLine action & " now runs " & nm & " from the Binary table"
End Sub

' key of the installed file named nm; "" if the MSI installs no such file
Function FileKey(nm)
	Dim v, r, fn, bar
	Set v = db.OpenView("SELECT `File`,`FileName` FROM `File`")
	v.Execute
	FileKey = ""
	Do
		Set r = v.Fetch
		If r Is Nothing Then Exit Do
		' FileName is "shortname|longname", or just the name when short
		fn = r.StringData(2)
		bar = InStr(fn, "|")
		If bar > 0 Then fn = Mid(fn, bar + 1)
		If StrComp(fn, nm, 1) = 0 Then
			FileKey = r.StringData(1)
			Exit Do
		End If
	Loop
	v.Close
	Set v = Nothing
End Function

' identifier of the custom action whose source is src; "" if there is none
Function ActionWithSource(src)
	Dim v, r
	ActionWithSource = ""
	If src = "" Then Exit Function
	Set v = db.OpenView("SELECT `Action` FROM `CustomAction` WHERE " & _
				"`Source`='" & src & "'")
	v.Execute
	Set r = v.Fetch
	If Not r Is Nothing Then ActionWithSource = r.StringData(1)
	v.Close
	Set v = Nothing
End Function

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

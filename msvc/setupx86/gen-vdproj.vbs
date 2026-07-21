' gen-vdproj.vbs - generate a Visual Studio setup project (.vdproj) by walking a
' staged "make install" tree and filling a tokenized skeleton (.vdproj.skel).
'
' usage:
'   cscript //nologo gen-vdproj.vbs <skeleton> <dat> <stagedir> <outfile> <version>
'
' The skeleton carries every fixed section; this script emits only the three
' sections that change with the file set - Hierarchy, File, Folder - plus the
' @KEY_*@ coupling references and the version.  The .dat maps each staged
' top-level directory to an installer folder root; everything below a root is
' discovered automatically, so new files, modules and module types need no edit.

Option Explicit

Dim FILEGUID, TG_PRODUCT, TG_CUSTOM, TG_PF, TG_REGULAR, MODV
FILEGUID   = "{1FB2D0AE-D3B9-43D4-B9DD-F88EC61E35DE}"
TG_PRODUCT = "{3C67513D-01DD-4637-8A68-80971EB9504F}"
TG_CUSTOM  = "{994432C3-9487-495D-8656-3E829A8DBDDE}"
TG_PF      = "{1525181F-901A-416C-8A58-119130FE478E}"
TG_REGULAR = "{9EF0B969-E518-4E46-987F-47570745A589}"
MODV       = 2147483647

Dim fso : Set fso = CreateObject("Scripting.FileSystemObject")

Dim args : Set args = WScript.Arguments
If args.Count < 5 Then
	WScript.StdErr.WriteLine "usage: gen-vdproj.vbs <skeleton> <dat> <stagedir> <outfile> <version>"
	WScript.Quit 1
End If
Dim skelPath, datPath, stageDir, outPath, versionStr
skelPath   = args(0)
datPath    = args(1)
stageDir   = args(2)
outPath    = args(3)
versionStr = args(4)

' generated-section accumulators
Dim hierOut, filesOut, foldersOut
hierOut = "" : filesOut = "" : foldersOut = ""

' parsed .dat
Dim rootSub, rootType, rootName, rootDefloc, rootProp, rootInter
Dim krTok, krPath, versionToken
Dim nRoots, nKr
ReDim rootSub(63) : ReDim rootType(63) : ReDim rootName(63)
ReDim rootDefloc(63) : ReDim rootProp(63) : ReDim rootInter(63)
ReDim krTok(63) : ReDim krPath(63)
nRoots = 0 : nKr = 0 : versionToken = ""
ParseDat datPath

' walk each existing root and emit its folders/files/hierarchy
Dim i, rp
For i = 0 To nRoots - 1
	rp = stageDir & "\" & Replace(rootSub(i), "/", "\")
	If fso.FolderExists(rp) Then
		RenderFolder fso.GetFolder(rp), rootSub(i), rootSub(i), TypeGuid(rootType(i)), _
			rootName(i), rootDefloc(i), rootProp(i), 12, rootInter(i)
	End If
Next

' fill the skeleton (ASCII read/write preserves the BOM bytes and CRLF, matching
' how configure.vbs round-trips these files)
Dim ts, content
Set ts = fso.OpenTextFile(skelPath)
content = ts.ReadAll()
ts.Close()
content = Replace(content, "@GENERATED_HIERARCHY@", RTrimCrlf(hierOut))
content = Replace(content, "@GENERATED_FILES@",     RTrimCrlf(filesOut))
content = Replace(content, "@GENERATED_FOLDERS@",   RTrimCrlf(foldersOut))
If versionToken <> "" Then content = Replace(content, versionToken, versionStr)
For i = 0 To nKr - 1
	content = Replace(content, krTok(i), GenKey(krPath(i)))
Next

Dim outf
Set outf = fso.OpenTextFile(outPath, 2, True)
outf.Write content
outf.Close()
WScript.StdErr.WriteLine "generated " & outPath

' ============================ subroutines ============================

' identity = installer logical path (drives the component keys)
' srcrel   = real staged relative path (drives SourcePath); differs from identity
'            only under an intermediate folder.
' interName, when set, inserts one folder between this folder and the staged
' contents (e.g. ProgramFilesFolder -> nodejs -> ...).
Sub RenderFolder(folderObj, identity, srcrel, typeguidStr, nm, defloc, prop, indent, interName)
	Dim pad, key, propval
	Dim names, n, sf, j
	Dim fnames, fn, ff, k2
	pad = Space(indent)
	key = GenKey(identity)
	foldersOut = foldersOut & pad & """" & typeguidStr & ":" & key & """" & vbCrLf
	foldersOut = foldersOut & pad & "{" & vbCrLf
	If defloc <> "" Then
		foldersOut = foldersOut & pad & """DefaultLocation"" = ""8:" & Replace(defloc, "\", "\\") & """" & vbCrLf
	End If
	foldersOut = foldersOut & pad & """Name"" = ""8:" & nm & """" & vbCrLf
	foldersOut = foldersOut & pad & """AlwaysCreate"" = ""11:FALSE""" & vbCrLf
	foldersOut = foldersOut & pad & """Condition"" = ""8:""" & vbCrLf
	foldersOut = foldersOut & pad & """Transitive"" = ""11:FALSE""" & vbCrLf
	If prop <> "" Then propval = prop Else propval = GenKey(identity & "|prop")
	foldersOut = foldersOut & pad & """Property"" = ""8:" & propval & """" & vbCrLf
	foldersOut = foldersOut & pad & "    ""Folders""" & vbCrLf
	foldersOut = foldersOut & pad & "    {" & vbCrLf
	If interName <> "" Then
		' one intermediate child folder; its contents are folderObj (srcrel kept)
		RenderFolder folderObj, identity & "/" & interName, srcrel, TG_REGULAR, _
			interName, "", "", indent + 8, ""
	Else
		' subfolders, sorted for deterministic output
		n = 0
		ReDim names(folderObj.SubFolders.Count + 1)
		For Each sf In folderObj.SubFolders
			names(n) = sf.Name : n = n + 1
		Next
		SortArr names, n
		For j = 0 To n - 1
			RenderFolder fso.GetFolder(folderObj.Path & "\" & names(j)), _
				identity & "/" & names(j), srcrel & "/" & names(j), TG_REGULAR, _
				names(j), "", "", indent + 8, ""
		Next
	End If
	foldersOut = foldersOut & pad & "    }" & vbCrLf
	foldersOut = foldersOut & pad & "}" & vbCrLf
	' files, sorted (a folder carrying an intermediate has none of its own)
	If interName = "" Then
		fn = 0
		ReDim fnames(folderObj.Files.Count + 1)
		For Each ff In folderObj.Files
			fnames(fn) = ff.Name : fn = fn + 1
		Next
		SortArr fnames, fn
		For k2 = 0 To fn - 1
			EmitFile identity & "/" & fnames(k2), srcrel & "/" & fnames(k2), fnames(k2), key
		Next
	End If
End Sub

Sub EmitFile(fileIdentity, fileSrcRel, tgt, folderKey)
	Dim k, src
	k = GenKey(fileIdentity)
	' .vdproj string values escape backslashes, so emit them doubled
	src = "stage\" & Replace(fileSrcRel, "/", "\")
	src = Replace(src, "\", "\\")
	hierOut = hierOut & "        ""Entry""" & vbCrLf
	hierOut = hierOut & "        {" & vbCrLf
	hierOut = hierOut & "        ""MsmKey"" = ""8:" & k & """" & vbCrLf
	hierOut = hierOut & "        ""OwnerKey"" = ""8:_UNDEFINED""" & vbCrLf
	hierOut = hierOut & "        ""MsmSig"" = ""8:_UNDEFINED""" & vbCrLf
	hierOut = hierOut & "        }" & vbCrLf
	filesOut = filesOut & "            """ & FILEGUID & ":" & k & """" & vbCrLf
	filesOut = filesOut & "            {" & vbCrLf
	filesOut = filesOut & "            ""SourcePath"" = ""8:" & src & """" & vbCrLf
	filesOut = filesOut & "            ""TargetName"" = ""8:" & tgt & """" & vbCrLf
	filesOut = filesOut & "            ""Tag"" = ""8:""" & vbCrLf
	filesOut = filesOut & "            ""Folder"" = ""8:" & folderKey & """" & vbCrLf
	filesOut = filesOut & "            ""Condition"" = ""8:""" & vbCrLf
	filesOut = filesOut & "            ""Transitive"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""Vital"" = ""11:TRUE""" & vbCrLf
	filesOut = filesOut & "            ""ReadOnly"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""Hidden"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""System"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""Permanent"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""SharedLegacy"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""PackageAs"" = ""3:1""" & vbCrLf
	filesOut = filesOut & "            ""Register"" = ""3:1""" & vbCrLf
	filesOut = filesOut & "            ""Exclude"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""IsDependency"" = ""11:FALSE""" & vbCrLf
	filesOut = filesOut & "            ""IsolateTo"" = ""8:""" & vbCrLf
	filesOut = filesOut & "            }" & vbCrLf
End Sub

Sub ParseDat(path)
	Dim f, line, toks
	Set f = fso.OpenTextFile(path)
	Do Until f.AtEndOfStream
		line = Trim(f.ReadLine())
		If line <> "" And Left(line, 1) <> "#" Then
			toks = Split(line, "|")
			Select Case toks(0)
			Case "VERSIONTOKEN"
				versionToken = toks(1)
			Case "ROOT"
				rootSub(nRoots)  = toks(1)
				rootType(nRoots) = toks(2)
				rootName(nRoots) = toks(3)
				If toks(4) = "-" Then
					rootDefloc(nRoots) = ""
				Else
					rootDefloc(nRoots) = toks(4)
				End If
				rootProp(nRoots) = toks(5)
				If toks(6) = "-" Then
					rootInter(nRoots) = ""
				Else
					rootInter(nRoots) = toks(6)
				End If
				nRoots = nRoots + 1
			Case "KEYREF"
				krTok(nKr)  = toks(1)
				krPath(nKr) = toks(2)
				nKr = nKr + 1
			End Select
		End If
	Loop
	f.Close()
End Sub

Function TypeGuid(t)
	Select Case t
	Case "product" : TypeGuid = TG_PRODUCT
	Case "custom"  : TypeGuid = TG_CUSTOM
	Case "pf"      : TypeGuid = TG_PF
	Case Else      : TypeGuid = TG_REGULAR
	End Select
End Function

' deterministic 128-bit key as "_" + 32 hex, from four 31-bit polynomial hashes.
' all arithmetic stays below 2^53 so Double precision is exact.
Function GenKey(s)
	Dim muls, res, kk, ii, h, c
	muls = Array(131, 137, 139, 149)
	res = "_"
	For kk = 0 To 3
		h = 0
		For ii = 1 To Len(s)
			c = Asc(Mid(s, ii, 1))
			h = h * muls(kk) + c
			h = h - MODV * Int(h / MODV)
		Next
		res = res & Right("00000000" & Hex(h), 8)
	Next
	GenKey = res
End Function

Sub SortArr(a, n)
	Dim i, j, tmp
	For i = 1 To n - 1
		tmp = a(i)
		j = i - 1
		Do While j >= 0
			If StrComp(a(j), tmp, 0) > 0 Then
				a(j + 1) = a(j) : j = j - 1
			Else
				Exit Do
			End If
		Loop
		a(j + 1) = tmp
	Next
End Sub

Function RTrimCrlf(s)
	If Len(s) >= 2 And Right(s, 2) = vbCrLf Then
		RTrimCrlf = Left(s, Len(s) - 2)
	Else
		RTrimCrlf = s
	End If
End Function

Attribute VB_Name = "ExportVBA"
' Excel macro to export all VBA source code in this project to text files for proper source control versioning
' Requires enabling the Excel setting in Options/Trust Center/Trust Center Settings/Macro Settings/Trust access to the VBA project object model

Public Sub ExportVisualBasicCode()
    Const Module = 1
    Const ClassModule = 2
    Const Form = 3
    Const Document = 100
    Const Padding = 24
    
    Dim Sep As String
    Dim IsWin As Boolean
    Dim IsMac As Boolean
    Dim VBComponent As Object
    Dim Count As Integer
    Dim Path As String
    Dim parent_directory As String
    Dim directory As String
    Dim extension As String
    Dim FSO
    
    Sep = Application.PathSeparator

    If Sep = "\" Then
        IsWin = True
    ElseIf Sep = "/" Then
        IsMac = True
    End If
    
    Set FSO = CreateObject("Scripting.FileSystemObject")
    If IsWin Then
        parent_directory = ActiveWorkbook.Path & Sep
    ElseIf IsMac Then
        parent_directory = ActiveWorkbook.Path & Sep
    End If

    directory = parent_directory & "VisualBasic"
    Count = 0
    
    If Not FSO.FolderExists(directory) Then
        Call FSO.CreateFolder(directory)
    End If
    Set FSO = Nothing
    
    For Each VBComponent In ActiveWorkbook.VBProject.VBComponents
        Select Case VBComponent.Type
            Case ClassModule, Document
                extension = ".cls"
            Case Form
                extension = ".frm"
            Case Module
                extension = ".bas"
            Case Else
                extension = ".txt"
        End Select
            
                
        On Error Resume Next
        err.Clear
        
        Path = directory & Sep & VBComponent.name & extension
        Call VBComponent.Export(Path)
        
        If err.Number <> 0 Then
            Call MsgBox("Failed to export " & VBComponent.name & " to " & Path, vbCritical)
        Else
            Count = Count + 1
            Debug.Print "Exported " & Left$(VBComponent.name & ":" & Space(Padding), Padding) & Path
        End If

        On Error GoTo 0
    Next
    
    Application.StatusBar = "Successfully exported " & CStr(Count) & " VBA files to " & directory
    'Application.OnTime Now + TimeSerial(0, 0, 10), "ClearStatusBar"
End Sub

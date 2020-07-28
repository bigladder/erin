VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddSourceForm 
   Caption         =   "Add Source"
   ClientHeight    =   5055
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5820
   OleObjectBlob   =   "AddSourceForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddSourceForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub ModesAddButton_Click()
    Dim i As Integer

    For i = 0 To ModesListBox.ListCount - 1
        If ModesListBox.Selected(i) = True Then
            ModesIncludeListBox.AddItem ModesListBox.List(i)
        End If
    Next i

    For i = ModesListBox.ListCount - 1 To 0 Step -1
        If ModesListBox.Selected(i) = True Then
            ModesListBox.RemoveItem i
        End If
    Next i
    
    Me.ModesListBox.MultiSelect = 0
    Me.ModesListBox.Value = ""
    Me.ModesListBox.MultiSelect = 2

    ModesAddCheckBox.Value = False
End Sub

Private Sub ModesAddCheckBox_Click()
    Dim i As Integer

    If ModesAddCheckBox.Value = True Then
        For i = 0 To ModesListBox.ListCount - 1
            ModesListBox.Selected(i) = True
        Next i
    End If
    
    If ModesAddCheckBox.Value = False Then
        For i = 0 To ModesListBox.ListCount - 1
            ModesListBox.Selected(i) = False
        Next i
    End If

End Sub

Private Sub ModesRemoveButton_Click()
    Dim counter As Integer
    Dim i As Integer
    
    counter = 0
        
    For i = 0 To Me.ModesIncludeListBox.ListCount - 1
        If Me.ModesIncludeListBox.Selected(i) = True Then
            Me.ModesListBox.AddItem Me.ModesIncludeListBox.List(i)
        End If
    Next i

    For i = Me.ModesIncludeListBox.ListCount - 1 To 0 Step -1
        If Me.ModesIncludeListBox.Selected(i) = True Then
            Me.ModesIncludeListBox.RemoveItem i
        End If
    Next i
    
    ModesRemoveCheckBox.Value = False
End Sub

Private Sub ModesRemoveCheckBox_Click()
    Dim i As Integer

    If ModesRemoveCheckBox.Value = True Then
        For i = 0 To ModesIncludeListBox.ListCount - 1
            ModesIncludeListBox.Selected(i) = True
        Next i
    End If
    
    If ModesRemoveCheckBox.Value = False Then
        For i = 0 To ModesIncludeListBox.ListCount - 1
            ModesIncludeListBox.Selected(i) = False
        Next i
    End If
End Sub

Private Sub CurvesAddButton_Click()
    Dim i As Integer

    For i = 0 To CurvesListBox.ListCount - 1
        If CurvesListBox.Selected(i) = True Then
            CurvesIncludeListBox.AddItem CurvesListBox.List(i)
        End If
    Next i

    For i = CurvesListBox.ListCount - 1 To 0 Step -1
        If CurvesListBox.Selected(i) = True Then
            CurvesListBox.RemoveItem i
        End If
    Next i
    
    Me.CurvesListBox.MultiSelect = 0
    Me.CurvesListBox.Value = ""
    Me.CurvesListBox.MultiSelect = 2

    CurvesAddCheckBox.Value = False
End Sub

Private Sub CurvesAddCheckBox_Click()
    Dim i As Integer

    If CurvesAddCheckBox.Value = True Then
        For i = 0 To CurvesListBox.ListCount - 1
            CurvesListBox.Selected(i) = True
        Next i
    End If
    
    If CurvesAddCheckBox.Value = False Then
        For i = 0 To CurvesListBox.ListCount - 1
            CurvesListBox.Selected(i) = False
        Next i
    End If

End Sub

Private Sub CurvesRemoveButton_Click()
    Dim counter As Integer
    Dim i As Integer
    
    counter = 0
        
    For i = 0 To Me.CurvesIncludeListBox.ListCount - 1
        If Me.CurvesIncludeListBox.Selected(i) = True Then
            Me.CurvesListBox.AddItem Me.CurvesIncludeListBox.List(i)
        End If
    Next i

    For i = Me.CurvesIncludeListBox.ListCount - 1 To 0 Step -1
        If Me.CurvesIncludeListBox.Selected(i) = True Then
            Me.CurvesIncludeListBox.RemoveItem i
        End If
    Next i
    
    CurvesRemoveCheckBox.Value = False
End Sub

Private Sub CurvesRemoveCheckBox_Click()
    Dim i As Integer

    If CurvesRemoveCheckBox.Value = True Then
        For i = 0 To CurvesIncludeListBox.ListCount - 1
            CurvesIncludeListBox.Selected(i) = True
        Next i
    End If
    
    If CurvesRemoveCheckBox.Value = False Then
        For i = 0 To CurvesIncludeListBox.ListCount - 1
            CurvesIncludeListBox.Selected(i) = False
        Next i
    End If
End Sub

Private Sub UserForm_Initialize()
    Dim oDictionary As Object
    Dim lRow As Long
    Dim rngItems As Range
    Dim cLoc As Range
    Dim ws As Worksheet
    
    Set oDictionary = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("Components")
    ws.Activate
    lRow = Cells(Rows.Count, 3).End(xlUp).Row
    Set rngItems = Range("C4:C" & lRow)
    For Each cLoc In rngItems
        With Me.LocationIDInput
            If oDictionary.exists(cLoc.Value) Then
                'Do Nothing
            Else
                oDictionary.Add cLoc.Value, 0
                .AddItem cLoc.Value
            End If
        End With
    Next cLoc

    Set ws = Worksheets("menus")
    ws.Activate
    For Each cLoc In ws.Range(Cells(20, 1), Cells(24, 1))
        With Me.OutflowInput
            .AddItem cLoc.Value
        End With
    Next cLoc

    With Me.LimitedSourceInput
        .AddItem "TRUE"
        .AddItem "FALSE"
    End With
        
    Set ws = Worksheets("failure-mode")
    ws.Activate
    lRow = Cells(Rows.Count, 1).End(xlUp).Row
    Set rngItems = Range("A2:A" & lRow)
    For Each cLoc In rngItems
        With Me.ModesListBox
            .AddItem cLoc.Value
        End With
    Next cLoc

    ModesListBox.MultiSelect = 2
    ModesIncludeListBox.MultiSelect = 2

    Set ws = Worksheets("fragility-curve")
    ws.Activate
    lRow = Cells(Rows.Count, 1).End(xlUp).Row
    Set rngItems = Range("A2:A" & lRow)
    For Each cLoc In rngItems
        With Me.CurvesListBox
            .AddItem cLoc.Value
        End With
    Next cLoc

    CurvesListBox.MultiSelect = 2
    CurvesIncludeListBox.MultiSelect = 2
        
End Sub

Private Sub CancelButton_Click()

    Unload Me

End Sub

Private Sub SaveButton_Click()
    Dim ParentSheet As Worksheet
    Dim Lr As Long
    Dim i As Integer
    
    Set ParentSheet = Sheets("Components")
    ParentSheet.Activate
    Lr = Cells(Rows.Count, 2).End(xlUp).Row
    ParentSheet.Range("B" & (Lr + 1)).Value = IDInput.text
    ParentSheet.Range("C" & (Lr + 1)).Value = LocationIDInput.text
    MyLeft = Cells(Lr + 1, "A").Left
    MyTop = Cells(Lr + 1, "A").Top
    MyHeight = Cells(Lr + 1, "A").Height
    MyWidth = MyHeight = Cells(Lr + 1, "A").Width
    ParentSheet.CheckBoxes.Add(MyLeft, MyTop, MyHeight, MyWidth).Select
    With Selection
       .Caption = ""
       .Value = xlOff
       .Display3DShading = False
       .OnAction = "Module1.Mixed_State"
    End With
    
    Set ParentSheet = Sheets("source-component")
    Lr = LastRow(ParentSheet)
    ParentSheet.Range("A" & (Lr + 1)).Value = IDInput.text
    ParentSheet.Range("B" & (Lr + 1)).Value = LocationIDInput.text
    ParentSheet.Range("C" & (Lr + 1)).Value = OutflowInput.text
    ParentSheet.Range("D" & (Lr + 1)).Value = LimitedSourceInput.text
    ParentSheet.Range("E" & (Lr + 1)).Value = MaxOutflowInput.text

    Set ParentSheet = Sheets("component-failure-mode")
    ParentSheet.Activate
    Lr = LastRow(ParentSheet)
    For i = 0 To ModesIncludeListBox.ListCount - 1
        ParentSheet.Range("A" & (Lr + i + 1)).Value = IDInput.text
        ParentSheet.Range("B" & (Lr + i + 1)).Value = ModesIncludeListBox.List(i)
    Next i
    
    Set ParentSheet = Sheets("component-fragility")
    ParentSheet.Activate
    Lr = LastRow(ParentSheet)
    For i = 0 To CurvesIncludeListBox.ListCount - 1
        ParentSheet.Range("A" & (Lr + i + 1)).Value = IDInput.text
        ParentSheet.Range("B" & (Lr + i + 1)).Value = CurvesIncludeListBox.List(i)
    Next i
    
    Unload Me
    Sheets("Components").Activate
    
End Sub



VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddLineForm 
   Caption         =   "Add Line"
   ClientHeight    =   4035
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5790
   OleObjectBlob   =   "AddLineForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddLineForm"
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
    Dim lRow As Long
    Dim cLoc As Range
    Dim ws As Worksheet
    
    'initializes list of network links
    Set ws = Worksheets("network-link")
    ws.Activate
    lRow = ws.Cells(Rows.Count, 1).End(xlUp).Row
    For Each cLoc In ws.Range(Cells(2, 1), Cells(lRow, 1))
        With Me.NetworkLinkIDInput
            .AddItem cLoc.Value
        End With
    Next cLoc

    'initializes list of flows
    Set ws = Worksheets("menus")
    ws.Activate
    For Each cLoc In ws.Range("flows")
        With Me.FlowInput
            .AddItem cLoc.Value
        End With
    Next cLoc

    'initializes list of failure modes
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

    'initializes list of fragility curves
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
    Sheets("Components").Activate

End Sub

Private Sub SaveButton_Click()
    Dim idName As String
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim rowCntr As Long
    Dim componentRow As Long
    
    idName = IDInput.Text
    IsExit = False
    
    Set ParentSheet = Sheets("Components")
    componentRow = getComponentRow(ParentSheet, idName)
    If IsExit Then Exit Sub
    
    Set ParentSheet = Sheets("Components")
    ParentSheet.Activate
    ParentSheet.Range("B" & (componentRow)).Value = IDInput.Text
    
    Set ParentSheet = Sheets("pass-through-component")
    ParentSheet.Activate
    lRow = LastRow(ParentSheet)
    For rowCntr = lRow To 1 Step -1
        If ParentSheet.Cells(rowCntr, 1) = idName Then
            componentRow = rowCntr
            Exit For
        Else
            componentRow = (lRow + 1)
        End If
    Next rowCntr
    
    ParentSheet.Range("A" & (componentRow)).Value = IDInput.Text
    ParentSheet.Range("B" & (componentRow)).Value = NetworkLinkIDInput.Text
    ParentSheet.Range("C" & (componentRow)).Value = FlowInput.Text

    AddComponentSubs AddLineForm, idName
    
    Unload Me
    Sheets("Components").Activate
    Module1.Mixed_State
    
End Sub


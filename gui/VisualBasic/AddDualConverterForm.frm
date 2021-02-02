VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddDualConverterForm 
   Caption         =   "Add Dual Converter"
   ClientHeight    =   7395
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5760
   OleObjectBlob   =   "AddDualConverterForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddDualConverterForm"
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
    For Each cLoc In ws.Range("flows")
        With Me.InflowInput
            .AddItem cLoc.Value
        End With
    Next cLoc
    For Each cLoc In ws.Range("flows")
        With Me.PrimaryOutflowInput
            .AddItem cLoc.Value
        End With
    Next cLoc
    For Each cLoc In ws.Range("flows")
        With Me.SecondaryOutflowInput
            .AddItem cLoc.Value
        End With
    Next cLoc
    For Each cLoc In ws.Range("flows")
        With Me.LossflowInput
            .AddItem cLoc.Value
        End With
    Next cLoc
    
    If AddComponentForm.AddComponentInput.Text = "chp - electric primary (dual converter)" Then
        Me.Caption = "Add CHP - Electric Primary"
        PrimaryOutflowInput.Text = "electricity"
        SecondaryOutflowInput.Text = "heating"
        PrimaryOutflowInput.Enabled = False
        SecondaryOutflowInput.Enabled = False
    ElseIf AddComponentForm.AddComponentInput.Text = "chp - heating primary (dual converter)" Then
        Me.Caption = "Add CHP - Heating Primary"
        PrimaryOutflowInput.Text = "heating"
        SecondaryOutflowInput.Text = "electricity"
        PrimaryOutflowInput.Enabled = False
        SecondaryOutflowInput.Enabled = False
    Else
        Me.Caption = "Add Generic Dual Converter"
    End If

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
    Sheets("Components").Activate

End Sub

Private Sub SaveButton_Click()
    Dim idName As String
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim rowCntr As Long
    Dim componentRow As Long
    
    'If PrimaryEfficiencyInput.text <= 0 Or PrimaryEfficiencyInput.text > 1 Then
    '    MsgBox "Primary Efficiency must be greater than 0 and less than or equal to 1.0."
    '    Exit Sub
    'End If
    'If SecondaryEfficiencyInput.text <= 0 Or SecondaryEfficiencyInput.text > 1 Then
    '    MsgBox "Secondary Efficiency must be greater than 0 and less than or equal to 1.0."
    '    Exit Sub
    'End If
        
    idName = IDInput.Text
    IsExit = False
    
    Set ParentSheet = Sheets("Components")
    componentRow = getComponentRow(ParentSheet, idName)
    If IsExit Then Exit Sub
    
    Set ParentSheet = Sheets("Components")
    ParentSheet.Activate
    ParentSheet.Range("B" & (componentRow)).Value = IDInput.Text
    ParentSheet.Range("C" & (componentRow)).Value = LocationIDInput.Text
    
    Set ParentSheet = Sheets("dual-outflow-converter-comp")
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
    ParentSheet.Range("B" & (componentRow)).Value = LocationIDInput.Text
    ParentSheet.Range("C" & (componentRow)).Value = InflowInput.Text
    ParentSheet.Range("D" & (componentRow)).Value = PrimaryOutflowInput.Text
    ParentSheet.Range("E" & (componentRow)).Value = SecondaryOutflowInput.Text
    ParentSheet.Range("F" & (componentRow)).Value = LossflowInput.Text
    ParentSheet.Range("G" & (componentRow)).Value = PrimaryEfficiencyInput.Text
    ParentSheet.Range("H" & (componentRow)).Value = SecondaryEfficiencyInput.Text

    AddComponentSubs AddDualConverterForm, idName
    
    Unload Me
    Sheets("Components").Activate
    Module1.Mixed_State

End Sub



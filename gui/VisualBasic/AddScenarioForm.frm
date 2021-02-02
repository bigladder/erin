VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddScenarioForm 
   Caption         =   "Add Scenario"
   ClientHeight    =   4965
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5745
   OleObjectBlob   =   "AddScenarioForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddScenarioForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub AddLoadProfileScenario_Click()
    AddLoadProfileForm.Show
End Sub

Private Sub DeleteLoadProfileScenario_Click()
    Dim i As Integer
    
    For i = Me.LoadProfilesList.ListCount - 1 To 0 Step -1
        If Me.LoadProfilesList.Selected(i) = True Then
            DeleteRows (Me.LoadProfilesList.List(i))
            Me.LoadProfilesList.RemoveItem i
        End If
    Next i
End Sub

Private Sub AddDamageMetricScenario_Click()
    AddDamageMetricForm.Show
End Sub

Private Sub DeleteDamageMetricScenario_Click()
    Dim i As Integer
    
    For i = Me.DamageMetricsList.ListCount - 1 To 0 Step -1
        If Me.DamageMetricsList.Selected(i) = True Then
            DeleteRows (Me.DamageMetricsList.List(i))
            Me.DamageMetricsList.RemoveItem i
        End If
    Next i
End Sub

Private Sub UserForm_Initialize()
    Dim oDictionary As Object
    Dim lRow As Long
    Dim rngItems As Range
    Dim cLoc As Range
    Dim ws As Worksheet
    
    Set oDictionary = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("dist-type")
    ws.Activate
    lRow = Cells(Rows.Count, 1).End(xlUp).Row
    Set rngItems = Range("A2:A" & lRow)
    For Each cLoc In rngItems
        With Me.OccDistributionInput
            If oDictionary.exists(cLoc.Value) Then
                'Do Nothing
            Else
                oDictionary.Add cLoc.Value, 0
                .AddItem cLoc.Value
            End If
        End With
    Next cLoc

    With Me.CalcReliabilityInput
        .AddItem "TRUE"
        .AddItem "FALSE"
    End With
    
    LoadProfilesList.MultiSelect = 2
    DamageMetricsList.MultiSelect = 2
            
End Sub

Private Sub CancelButton_Click()

    Unload Me
    Worksheets("Scenarios").Activate

End Sub

Private Sub SaveButton_Click()
    Dim idName As String
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim rowCntr As Long
    Dim componentRow As Long
    
    idName = IDInput.Text
    IsExit = False
    
    Set ParentSheet = Sheets("Scenarios")
    componentRow = getComponentRow(ParentSheet, idName)
    If IsExit Then Exit Sub
    
    Set ParentSheet = Sheets("Scenarios")
    ParentSheet.Activate
    ParentSheet.Range("B" & (componentRow)).Value = IDInput.Text
    
    Set ParentSheet = Sheets("scenario")
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
    ParentSheet.Range("B" & (componentRow)).Value = DurationInput.Text
    ParentSheet.Range("C" & (componentRow)).Value = OccDistributionInput.Text
    ParentSheet.Range("D" & (componentRow)).Value = CalcReliabilityInput.Text
    ParentSheet.Range("E" & (componentRow)).Value = MaxOccurancesInput.Text

    Unload Me
    Sheets("Scenarios").Activate
    Module1.Mixed_State
    
End Sub

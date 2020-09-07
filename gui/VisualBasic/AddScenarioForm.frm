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
    Dim lRowow As Long
    Dim rngItems As Range
    Dim cLoc As Range
    Dim ws As Worksheet
    
    Set oDictionary = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("fixed-cdf")
    ws.Activate
    lRowow = Cells(Rows.Count, 1).End(xlUp).Row
    Set rngItems = Range("A2:A" & lRowow)
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

    Worksheets("Scenarios").Activate
    Unload Me

End Sub

Private Sub SaveButton_Click()
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim i As Integer
    
    Set ParentSheet = Sheets("Scenarios")
    ParentSheet.Activate
    lRow = Cells(Rows.Count, 2).End(xlUp).Row
    ParentSheet.Range("B" & (lRow + 1)).Value = IDInput.text
    MyLeft = Cells(lRow + 1, "A").Left
    MyTop = Cells(lRow + 1, "A").Top
    MyHeight = Cells(lRow + 1, "A").Height
    MyWidth = MyHeight = Cells(lRow + 1, "A").Width
    ParentSheet.CheckBoxes.Add(MyLeft, MyTop, MyHeight, MyWidth).Select
    With Selection
       .Caption = ""
       .Value = xlOff
       .Display3DShading = False
       .OnAction = "Module1.Mixed_State"
    End With
    
    Set ParentSheet = Sheets("scenario")
    lRow = LastRow(ParentSheet)
    ParentSheet.Range("A" & (lRow + 1)).Value = IDInput.text
    ParentSheet.Range("B" & (lRow + 1)).Value = DurationInput.text
    ParentSheet.Range("C" & (lRow + 1)).Value = OccDistributionInput.text
    ParentSheet.Range("D" & (lRow + 1)).Value = CalcReliabilityInput.text
    ParentSheet.Range("E" & (lRow + 1)).Value = MaxOccurancesInput.text

    Unload Me
    Sheets("Scenarios").Activate
    Module1.Mixed_State
    
End Sub

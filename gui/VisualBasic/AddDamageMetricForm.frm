VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddDamageMetricForm 
   Caption         =   "Add Damage Metric"
   ClientHeight    =   2190
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5760
   OleObjectBlob   =   "AddDamageMetricForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddDamageMetricForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub UserForm_Initialize()
        
End Sub

Private Sub CancelButton_Click()

    Unload Me

End Sub

Private Sub SaveButton_Click()
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim i As Integer
    
    Set ParentSheet = Sheets("damage-intensity")
    lRow = LastRow(ParentSheet)
    ParentSheet.Range("A" & (lRow + 1)).Value = AddScenarioForm.IDInput.text
    ParentSheet.Range("B" & (lRow + 1)).Value = NameInput.text
    ParentSheet.Range("C" & (lRow + 1)).Value = ValueInput.text
    
    With AddScenarioForm.DamageMetricsList
        .AddItem NameInput.text
    End With
        
    Unload Me
    
End Sub

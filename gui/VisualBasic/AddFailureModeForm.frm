VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddFailureModeForm 
   Caption         =   "Add Failure Mode"
   ClientHeight    =   3585
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5745
   OleObjectBlob   =   "AddFailureModeForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddFailureModeForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub CancelButton_Click()

    Unload Me

End Sub

Private Sub SaveButton_Click()
    Dim ParentSheet As Worksheet
    Dim Lr As Long
    
    Set ParentSheet = Sheets("failure-mode")
    Lr = LastRow(ParentSheet)
    ParentSheet.Range("A" & (Lr + 1)).Value = IDInput.text
    ParentSheet.Range("B" & (Lr + 1)).Value = FailureCDFInput.text
    ParentSheet.Range("C" & (Lr + 1)).Value = RepairCDFInput.text
    Unload Me

End Sub



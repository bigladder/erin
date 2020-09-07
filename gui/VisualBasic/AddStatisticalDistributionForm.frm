VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddStatisticalDistributionForm 
   Caption         =   "Add Statistical Distribution"
   ClientHeight    =   3030
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5400
   OleObjectBlob   =   "AddStatisticalDistributionForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddStatisticalDistributionForm"
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
    Dim lRow As Long
    
    Set ParentSheet = Sheets("fixed-cdf")
    lRow = LastRow(ParentSheet)
    ParentSheet.Range("A" & (lRow + 1)).Value = IDInput.text
    ParentSheet.Range("B" & (lRow + 1)).Value = ValueInput.text
    Unload Me

End Sub

VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddNormalDistribution 
   Caption         =   "UserForm1"
   ClientHeight    =   3015
   ClientLeft      =   120
   ClientTop       =   465
   ClientWidth     =   4560
   OleObjectBlob   =   "AddNormalDistribution.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddNormalDistribution"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub CancelButton_Click()
    Unload Me
    Worksheets("Settings").Activate
End Sub


Public Sub SetDistID(Name As String)
    Me.DistIDLabel.Caption = Name
End Sub


Private Sub SaveButton_Click()

    Dim ParentSheet As Worksheet
    Dim theRow As Long
    
    Set ParentSheet = Sheets("normal-dist")
    theRow = LastRow(ParentSheet)
    ParentSheet.Range("A" & (theRow + 1)).Value = DistIDLabel.Caption
    ParentSheet.Range("B" & (theRow + 1)).Value = MeanValueInput.Text
    ParentSheet.Range("C" & (theRow + 1)).Value = StdevValueInput.Text
    Unload Me
    Worksheets("Settings").Activate
    
End Sub



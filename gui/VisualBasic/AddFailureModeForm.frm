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
        With Me.FailureCDFInput
            If oDictionary.exists(cLoc.Value) Then
                'Do Nothing
            Else
                oDictionary.Add cLoc.Value, 0
                .AddItem cLoc.Value
            End If
        End With
    Next cLoc
    Set oDictionary = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("fixed-cdf")
    ws.Activate
    lRowow = Cells(Rows.Count, 1).End(xlUp).Row
    Set rngItems = Range("A2:A" & lRowow)
    For Each cLoc In rngItems
        With Me.RepairCDFInput
            If oDictionary.exists(cLoc.Value) Then
                'Do Nothing
            Else
                oDictionary.Add cLoc.Value, 0
                .AddItem cLoc.Value
            End If
        End With
    Next cLoc
End Sub

Private Sub CancelButton_Click()

    Unload Me
    Sheets("Settings").Activate

End Sub

Private Sub SaveButton_Click()
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    
    Set ParentSheet = Sheets("failure-mode")
    lRow = LastRow(ParentSheet)
    ParentSheet.Range("A" & (lRow + 1)).Value = IDInput.text
    ParentSheet.Range("B" & (lRow + 1)).Value = FailureCDFInput.text
    ParentSheet.Range("C" & (lRow + 1)).Value = RepairCDFInput.text
    Unload Me
    Sheets("Settings").Activate

End Sub



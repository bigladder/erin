VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddLoadProfileForm 
   Caption         =   "Add Load Profile"
   ClientHeight    =   4050
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5715
   OleObjectBlob   =   "AddLoadProfileForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddLoadProfileForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub UserForm_Initialize()
    Dim oDictionary As Object
    Dim lRow As Long
    Dim rngItems As Range
    Dim cLoc As Range
    Dim ws As Worksheet
    
    Set oDictionary = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("load-component")
    ws.Activate
    lRow = Cells(Rows.Count, 3).End(xlUp).Row
    Set rngItems = Range("A2:A" & lRow)
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
    For Each cLoc In ws.Range(Cells(22, 1), Cells(25, 1))
        With Me.FlowInput
            .AddItem cLoc.Value
        End With
    Next cLoc
        
End Sub

Private Sub CancelButton_Click()

    Unload Me

End Sub

Private Sub SaveButton_Click()
    Dim ParentSheet As Worksheet
    Dim Lr As Long
    Dim i As Integer
    
    Set ParentSheet = Sheets("load-profile")
    Lr = LastRow(ParentSheet)
    ParentSheet.Range("A" & (Lr + 1)).Value = AddScenarioForm.IDInput.text
    ParentSheet.Range("B" & (Lr + 1)).Value = LocationIDInput.text
    ParentSheet.Range("C" & (Lr + 1)).Value = FlowInput.text
    ParentSheet.Range("D" & (Lr + 1)).Value = FileNameInput.text
    
    With AddScenarioForm.LoadProfilesList
        .AddItem NameInput.text
    End With
        
    Unload Me
    
End Sub

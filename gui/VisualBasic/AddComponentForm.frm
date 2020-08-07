VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddComponentForm 
   Caption         =   "Add Component"
   ClientHeight    =   1635
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5610
   OleObjectBlob   =   "AddComponentForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddComponentForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub UserForm_Initialize()
    Dim lRow As Long
    Dim cLoc As Range
    Dim ws As Worksheet
    Set ws = Worksheets("menus")
    
    ws.Activate
    For Each cLoc In ws.Range(Cells(6, 1), Cells(19, 1))
        With Me.AddComponentInput
            .AddItem cLoc.Value
        End With
    Next cLoc
    
End Sub

Private Sub CancelButton_Click()

    Worksheets("Components").Activate
    Unload Me

End Sub

Private Sub AddComponentButton_Click()
    
    If AddComponentInput.text = "batteries (storage)" Then
        AddStorageForm.Show
    ElseIf AddComponentInput.text = "boiler (converter)" Then
        AddConverterForm.Show
    ElseIf AddComponentInput.text = "chp - electric primary (dual converter)" Then
        AddDualConverterForm.Show
    ElseIf AddComponentInput.text = "chp - heating primary (dual converter)" Then
        AddDualConverterForm.Show
    ElseIf AddComponentInput.text = "electric generator (converter)" Then
        AddConverterForm.Show
    ElseIf AddComponentInput.text = "generic converter" Then
        AddConverterForm.Show
    ElseIf AddComponentInput.text = "generic storage" Then
        AddStorageForm.Show
    ElseIf AddComponentInput.text = "generic dual converter" Then
        AddDualConverterForm.Show
    ElseIf AddComponentInput.text = "load" Then
        AddLoadForm.Show
    ElseIf AddComponentInput.text = "line (pass-through)" Then
        AddLineForm.Show
    ElseIf AddComponentInput.text = "mover" Then
        AddMoverForm.Show
    ElseIf AddComponentInput.text = "source" Then
        AddSourceForm.Show
    ElseIf AddComponentInput.text = "thermal energy storage (storage)" Then
        AddStorageForm.Show
    ElseIf AddComponentInput.text = "uncontrolled source" Then
        AddUncontrolledSourceForm.Show
    Else
        MsgBox "Invalid Entry"
    End If
        
    Unload Me
    Sheets("Components").Activate
    Module1.Mixed_State
            
End Sub



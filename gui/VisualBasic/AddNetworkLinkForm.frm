VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AddNetworkLinkForm 
   Caption         =   "Add Network Link"
   ClientHeight    =   4020
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   5805
   OleObjectBlob   =   "AddNetworkLinkForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AddNetworkLinkForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Private Sub UserForm_Initialize()
    Dim oDictionary1 As Object
    Dim oDictionary2 As Object
    Dim oItem As Object
    Dim lRowow As Long
    Dim rngItems As Range
    Dim cLoc As Range
    Dim ws As Worksheet
    
    Set oDictionary1 = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("Components")
    ws.Activate
    lRowow = Cells(Rows.Count, 3).End(xlUp).Row
    Set rngItems = Range("C4:C" & lRowow)
    For Each cLoc In rngItems
        With Me.SourceLocationIDInput
            If oDictionary1.exists(cLoc.Value) Then
                'Do Nothing
            Else
                oDictionary1.Add cLoc.Value, 0
                .AddItem cLoc.Value
            End If
        End With
    Next cLoc
    Set oDictionary2 = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("Components")
    ws.Activate
    lRowow = Cells(Rows.Count, 3).End(xlUp).Row
    Set rngItems = Range("C4:C" & lRowow)
    For Each cLoc In rngItems
        With Me.DestinationLocationIDInput
            If oDictionary2.exists(cLoc.Value) Then
                'Do Nothing
            Else
                oDictionary2.Add cLoc.Value, 0
                .AddItem cLoc.Value
            End If
        End With
    Next cLoc

    Set ws = Worksheets("menus")
    ws.Activate
    For Each cLoc In ws.Range("flows")
        With Me.FlowInput
            .AddItem cLoc.Value
        End With
    Next cLoc
        
End Sub

Private Sub CancelButton_Click()

    Worksheets("Network").Activate
    Unload Me

End Sub

Private Sub SaveButton_Click()
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim i As Integer
    
    Set ParentSheet = Sheets("Network")
    ParentSheet.Activate
    lRow = Cells(Rows.Count, 2).End(xlUp).Row
    ParentSheet.Range("B" & (lRow + 1)).Value = IDInput.text
    ParentSheet.Range("C" & (lRow + 1)).Value = SourceLocationIDInput.text
    ParentSheet.Range("D" & (lRow + 1)).Value = DestinationLocationIDInput.text
    ParentSheet.Range("E" & (lRow + 1)).Value = FlowInput.text
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
    
    Set ParentSheet = Sheets("network-link")
    lRow = LastRow(ParentSheet)
    ParentSheet.Range("A" & (lRow + 1)).Value = IDInput.text
    ParentSheet.Range("B" & (lRow + 1)).Value = SourceLocationIDInput.text
    ParentSheet.Range("C" & (lRow + 1)).Value = DestinationLocationIDInput.text
    ParentSheet.Range("D" & (lRow + 1)).Value = FlowInput.text

    Unload Me
    Sheets("Network").Activate
    Module1.Mixed_State
    
End Sub


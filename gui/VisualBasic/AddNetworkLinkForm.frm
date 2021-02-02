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
    Dim lRow As Long
    Dim rngItems As Range
    Dim cLoc As Range
    Dim ws As Worksheet
    
    Set oDictionary1 = CreateObject("Scripting.Dictionary")
    Set ws = Worksheets("Components")
    ws.Activate
    lRow = Cells(Rows.Count, 3).End(xlUp).Row
    Set rngItems = Range("C4:C" & lRow)
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
    lRow = Cells(Rows.Count, 3).End(xlUp).Row
    Set rngItems = Range("C4:C" & lRow)
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

    Unload Me
    Worksheets("Network").Activate

End Sub

Private Sub SaveButton_Click()
    Dim idName As String
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim rowCntr As Long
    Dim componentRow As Long
    
    idName = IDInput.Text
    IsExit = False
    
    Set ParentSheet = Sheets("Network")
    componentRow = getComponentRow(ParentSheet, idName)
    If IsExit Then Exit Sub
    
    Set ParentSheet = Sheets("Network")
    ParentSheet.Activate
    ParentSheet.Range("B" & (componentRow)).Value = IDInput.Text
    ParentSheet.Range("C" & (componentRow)).Value = SourceLocationIDInput.Text
    ParentSheet.Range("D" & (componentRow)).Value = DestinationLocationIDInput.Text
    ParentSheet.Range("E" & (componentRow)).Value = FlowInput.Text
    
    Set ParentSheet = Sheets("network-link")
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
    ParentSheet.Range("B" & (componentRow)).Value = SourceLocationIDInput.Text
    ParentSheet.Range("C" & (componentRow)).Value = DestinationLocationIDInput.Text
    ParentSheet.Range("D" & (componentRow)).Value = FlowInput.Text

    Unload Me
    Sheets("Network").Activate
    Module1.Mixed_State
    
End Sub


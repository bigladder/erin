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

Private Sub ContinueButton_Click()
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    
    If IDInput.Text = "" Then
        MsgBox "Please input an ID for this Distribution"
        Exit Sub
    End If
    
    Set ParentSheet = Sheets("dist-type")
    lRow = LastRow(ParentSheet)
    ParentSheet.Range("A" & (lRow + 1)).Value = IDInput.Text
    ParentSheet.Range("B" & (lRow + 1)).Value = DistType.Text
    
    If DistType.Text = "fixed" Then
        AddFixedDistribution.SetDistID IDInput.Text
        Me.Hide
        AddFixedDistribution.Show
    ElseIf DistType.Text = "uniform" Then
        AddUniformDistribution.SetDistID IDInput.Text
        Me.Hide
        AddUniformDistribution.Show
    ElseIf DistType.Text = "normal" Then
        AddNormalDistribution.SetDistID IDInput.Text
        Me.Hide
        AddNormalDistribution.Show
    ElseIf DistType.Text = "weibull" Then
        AddWeibullDistribution.SetDistID IDInput.Text
        Me.Hide
        AddWeibullDistribution.Show
    ElseIf DistType.Text = "quantile" Then
        AddQuantileDistribution.SetDistID IDInput.Text
        Me.Hide
        AddQuantileDistribution.Show
    Else
        MsgBox "Invalid Entry"
    End If
    
    Unload Me

End Sub


Private Sub UserForm_Initialize()
    Dim lRow As Long
    Dim cLoc As Range
    Dim ws As Worksheet
    Set ws = Worksheets("menus")
    
    ws.Activate
    For Each cLoc In ws.Range(Cells(33, 1), Cells(37, 1))
        Me.DistType.AddItem (cLoc.Value)
    Next cLoc
    Me.DistType.Value = ws.Cells(33, 1).Value
End Sub

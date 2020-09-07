Attribute VB_Name = "Module1"
' Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
' See the LICENSE.txt file for additional terms and conditions.

Option Explicit

Global IsExit As Boolean

Public Type SubComponent
    ModesListBox As Variant
    ModesIncludeListBox As Variant
    CurvesListBox As Variant
    CurvesIncludeListBox As Variant
    ProfileListBox As Variant
    DamageListBox As Variant
End Type

Sub AddStatisticalDistribution()
    AddStatisticalDistributionForm.Show
End Sub

Sub AddFragilityCurve()
    AddFragilityCurveForm.Show
End Sub

Sub AddFailureMode()
    AddFailureModeForm.Show
End Sub

Sub AddComponent()
    AddComponentForm.Show
End Sub

Sub AddNetworkLink()
    AddNetworkLinkForm.Show
End Sub

Sub AddScenario()
    AddScenarioForm.Show
End Sub

Sub DoNothing()
    'Do nothing
End Sub

Sub AddComponentSubs(userFormName As UserForm, idName As String)
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim rowCntr As Long
    Dim i As Integer

    Set ParentSheet = Sheets("component-failure-mode")
    ParentSheet.Activate
    lRow = LastRow(ParentSheet)
    For rowCntr = lRow To 1 Step -1
        If ParentSheet.Cells(rowCntr, 1) = idName Then
            ParentSheet.Rows(rowCntr).Delete
        End If
    Next rowCntr
    lRow = LastRow(ParentSheet)
    For i = 0 To userFormName.ModesIncludeListBox.ListCount - 1
        ParentSheet.Range("A" & (lRow + i + 1)).Value = userFormName.IDInput.text
        ParentSheet.Range("B" & (lRow + i + 1)).Value = userFormName.ModesIncludeListBox.List(i)
    Next i
    
    Set ParentSheet = Sheets("component-fragility")
    ParentSheet.Activate
    lRow = LastRow(ParentSheet)
    For rowCntr = lRow To 1 Step -1
        If ParentSheet.Cells(rowCntr, 1) = idName Then
            ParentSheet.Rows(rowCntr).Delete
        End If
    Next rowCntr
    lRow = LastRow(ParentSheet)
    For i = 0 To userFormName.CurvesIncludeListBox.ListCount - 1
        ParentSheet.Range("A" & (lRow + i + 1)).Value = userFormName.IDInput.text
        ParentSheet.Range("B" & (lRow + i + 1)).Value = userFormName.CurvesIncludeListBox.List(i)
    Next i

End Sub

Sub EditItem()
    Dim CB As CheckBox
    Dim checkedRow As Integer
    Dim checkedID As String
    
    For Each CB In ActiveSheet.CheckBoxes
        If CB.TopLeftCell.Row = 3 Then
            CB.Value = 0
        Else
            If CB.Value = 1 Then
                checkedRow = CB.TopLeftCell.Row
                checkedID = Cells(checkedRow, 2).Value
                FindItem (checkedID)
            End If
        End If
    Next

End Sub

Sub SelectAll_Click()
    Dim CB As CheckBox
    For Each CB In ActiveSheet.CheckBoxes
      If CB.Name <> ActiveSheet.CheckBoxes("Select All").Name Then
        CB.Value = ActiveSheet.CheckBoxes("Select All").Value
        If CB.Value = xlOff Then
            Application.ScreenUpdating = False
            ActiveSheet.Shapes("remove_group").Visible = False
            ActiveSheet.Shapes("edit_group").Visible = False
            'Me.Shapes("run_group").Visible = False
            Application.ScreenUpdating = True
        Else
            ActiveSheet.Shapes("remove_group").Visible = True
            ActiveSheet.Shapes("edit_group").Visible = True
            'Me.Shapes("run_group").Visible = True
        End If
      End If
    Next CB
End Sub

Sub Mixed_State()
    Dim CB As CheckBox
    For Each CB In ActiveSheet.CheckBoxes
      If CB.Name <> ActiveSheet.CheckBoxes("Select All").Name And CB.Value <> ActiveSheet.CheckBoxes("Select All").Value And ActiveSheet.CheckBoxes("Select All").Value <> 2 Then
        ActiveSheet.CheckBoxes("Select All").Value = 2
    Exit For
       Else
         ActiveSheet.CheckBoxes("Select All").Value = CB.Value
      End If
    Next CB
    If ActiveSheet.CheckBoxes("Select All").Value = xlOff Then
       Application.ScreenUpdating = False
       ActiveSheet.Shapes("remove_group").Visible = False
       ActiveSheet.Shapes("edit_group").Visible = False
       'Me.Shapes("run_group").Visible = False
       Application.ScreenUpdating = True
    Else
       ActiveSheet.Shapes("remove_group").Visible = True
       ActiveSheet.Shapes("edit_group").Visible = True
       'Me.Shapes("run_group").Visible = True
    End If
End Sub

Sub DeleteCheckboxandRow()
    Dim CB As CheckBox
    Dim checkedRow As Integer
    Dim checkedID As String
    
    If MsgBox("Are you sure you want to remove the selected rows?", vbYesNo) = vbNo Then
        Exit Sub
    End If
    
    For Each CB In ActiveSheet.CheckBoxes
        If CB.TopLeftCell.Row = 3 Then
            CB.Value = 0
        Else
            If CB.Value = 1 Then
                checkedRow = CB.TopLeftCell.Row
                checkedID = Cells(checkedRow, 2).Value
                DeleteRows (checkedID)
                CB.TopLeftCell.EntireRow.Delete
                CB.Delete
            End If
        End If
    Next
    Application.ScreenUpdating = False
    ActiveSheet.Shapes("remove_group").Visible = False
    ActiveSheet.Shapes("edit_group").Visible = False
    'Me.Shapes("run_group").Visible = False
    Application.ScreenUpdating = True

End Sub

Sub DeleteRows(idName As String)
    Dim MyArray As Variant
    Dim vName As Variant
    Dim ws As Worksheet
    Dim lRowow As Long
    Dim rowCntr As Long
    
    MyArray = Array("component-failure-mode", "component-fragility", "converter-component", "damage-intensity", "dual-outflow-converter-comp", "failure-mode", "fixed-cdf", "fragility-curve", "load-component", "load-profile", "mover-component", "muxer-component", "network-link", "pass-through-component", "scenario", "source-component", "storage-component", "uncontrolled-src")
    
    For Each vName In MyArray
        Set ws = ThisWorkbook.Sheets(vName)
        lRowow = LastRow(ws)
        For rowCntr = lRowow To 1 Step -1
            If ws.Cells(rowCntr, 1) = idName Or ws.Cells(rowCntr, 2) = idName Then
                ws.Rows(rowCntr).Delete
            End If
        Next rowCntr
    Next vName
End Sub

Sub FindItem(idName As String)
    Dim WorksheetArray As Variant
    Dim vName As Variant
    Dim ws As Worksheet
    Dim lRowow As Long
    Dim rowCntr As Long
    Dim TabName As String
    Dim ComponentSubs As SubComponent
    
    WorksheetArray = Array("converter-component", "dual-outflow-converter-comp", "load-component", "load-profile", "mover-component", "muxer-component", "network-link", "pass-through-component", "scenario", "source-component", "storage-component", "uncontrolled-src")
    For Each vName In WorksheetArray
        Set ws = ThisWorkbook.Sheets(vName)
        lRowow = LastRow(ws)
        For rowCntr = lRowow To 1 Step -1
            If ws.Cells(rowCntr, 1) = idName Then
                If ws.Name = "converter-component" Then
                    With AddConverterForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .LocationIDInput = ws.Range("B" & rowCntr).Value
                        .InflowInput = ws.Range("C" & rowCntr).Value
                        .OutflowInput = ws.Range("D" & rowCntr).Value
                        .LossflowInput = ws.Range("E" & rowCntr).Value
                        .ConstantEfficiencyInput = ws.Range("F" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
                If ws.Name = "dual-outflow-converter-comp" Then
                    With AddDualConverterForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .LocationIDInput = ws.Range("B" & rowCntr).Value
                        .InflowInput = ws.Range("C" & rowCntr).Value
                        .PrimaryOutflowInput = ws.Range("D" & rowCntr).Value
                        .SecondaryOutflowInput = ws.Range("E" & rowCntr).Value
                        .LossflowInput = ws.Range("F" & rowCntr).Value
                        .PrimaryEfficiencyInput = ws.Range("G" & rowCntr).Value
                        .SecondaryEfficiencyInput = ws.Range("H" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
                If ws.Name = "pass-through-component" Then
                    With AddLineForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .NetworkLinkIDInput = ws.Range("B" & rowCntr).Value
                        .FlowInput = ws.Range("C" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
                If ws.Name = "load-component" Then
                    With AddLoadForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .LocationIDInput = ws.Range("B" & rowCntr).Value
                        .InflowInput = ws.Range("C" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
                If ws.Name = "mover-component" Then
                    With AddMoverForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .LocationIDInput = ws.Range("B" & rowCntr).Value
                        .MainFlowInput = ws.Range("C" & rowCntr).Value
                        .SupportFlowInput = ws.Range("D" & rowCntr).Value
                        .COPInput = ws.Range("E" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
                If ws.Name = "network-link" Then
                    With AddNetworkLinkForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .SourceLocationIDInput = ws.Range("B" & rowCntr).Value
                        .DestinationLocationIDInput = ws.Range("C" & rowCntr).Value
                        .FlowInput = ws.Range("D" & rowCntr).Value
                        .Show
                    End With
                End If
                If ws.Name = "scenario" Then
                    With AddScenarioForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .DurationInput = ws.Range("B" & rowCntr).Value
                        .OccDistributionInput = ws.Range("C" & rowCntr).Value
                        .CalcReliabilityInput = ws.Range("D" & rowCntr).Value
                        .MaxOccurancesInput = ws.Range("E" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .LoadProfilesList.List = ComponentSubs.ProfileListBox
                        .DamageMetricsList.List = ComponentSubs.DamageListBox
                        .Show
                    End With
                End If
                If ws.Name = "source-component" Then
                    With AddSourceForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .LocationIDInput = ws.Range("B" & rowCntr).Value
                        .OutflowInput = ws.Range("C" & rowCntr).Value
                        .LimitedSourceInput = ws.Range("D" & rowCntr).Value
                        .MaxOutflowInput = ws.Range("E" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
                If ws.Name = "storage-component" Then
                    With AddStorageForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .LocationIDInput = ws.Range("B" & rowCntr).Value
                        .StorageFlowInput = ws.Range("C" & rowCntr).Value
                        .StorageCapacityInput = ws.Range("D" & rowCntr).Value
                        .MaxStorageInflowInput = ws.Range("E" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
                If ws.Name = "uncontrolled-src" Then
                    With AddUncontrolledSourceForm
                        .IDInput = ws.Range("A" & rowCntr).Value
                        .LocationIDInput = ws.Range("B" & rowCntr).Value
                        .OutflowInput = ws.Range("C" & rowCntr).Value
                        ComponentSubs = getComponentSubs(idName)
                        .ModesListBox.List = ComponentSubs.ModesListBox
                        .ModesIncludeListBox.List = ComponentSubs.ModesIncludeListBox
                        .CurvesListBox.List = ComponentSubs.CurvesListBox
                        .CurvesIncludeListBox.List = ComponentSubs.CurvesIncludeListBox
                        .Show
                    End With
                End If
            End If
        Next rowCntr
    Next vName
End Sub

Function getComponentRow(idName As String)
    Dim ParentSheet As Worksheet
    Dim lRow As Long
    Dim rowCntr As Long
    Dim componentRow As Long
    Dim NewComponent As Boolean
    Dim MyLeft As Double
    Dim MyTop As Double
    Dim MyHeight As Double
    Dim MyWidth As Double
    
    Set ParentSheet = Sheets("Components")
    ParentSheet.Activate
    lRow = Cells(Rows.Count, 2).End(xlUp).Row
    NewComponent = True
    For rowCntr = lRow To 1 Step -1
        If ParentSheet.Cells(rowCntr, 2) = idName Then
            NewComponent = False
            componentRow = rowCntr
            Exit For
        Else
            componentRow = (lRow + 1)
        End If
    Next rowCntr
    
    If NewComponent Then
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
    Else
        If MsgBox("Update current " & idName & " component?", vbYesNo) = vbNo Then
            IsExit = True
            Exit Function
        End If
    End If
    
    getComponentRow = componentRow
    
End Function

Function getComponentSubs(idName As String) As SubComponent
    Dim ComponentSubArray As Variant
    Dim vSubName As Variant
    Dim i As Long
    Dim j As Long
    Dim k As Long
    Dim ws As Worksheet
    Dim lRowow As Long
    Dim iSubCntr As Long
    Dim ComponentSubs As SubComponent
    Dim ModesListBox() As Variant
    Dim ModesListBox_orig() As Variant
    Dim ModesIncludeListBox() As Variant
    Dim CurvesListBox() As Variant
    Dim CurvesListBox_orig() As Variant
    Dim CurvesIncludeListBox() As Variant
    Dim ProfileListBox() As Variant
    Dim DamageListBox() As Variant
    
    ReDim Preserve ModesListBox(0)
    ReDim Preserve ModesListBox_orig(0)
    ReDim Preserve ModesIncludeListBox(0)
    ReDim Preserve CurvesListBox(0)
    ReDim Preserve CurvesListBox_orig(0)
    ReDim Preserve CurvesIncludeListBox(0)
    ReDim Preserve ProfileListBox(0)
    ReDim Preserve DamageListBox(0)
    ModesListBox(0) = ""
    ModesListBox_orig(0) = ""
    ModesIncludeListBox(0) = ""
    CurvesListBox(0) = ""
    CurvesListBox_orig(0) = ""
    CurvesIncludeListBox(0) = ""
    ProfileListBox(0) = ""
    DamageListBox(0) = ""
        
    ComponentSubArray = Array("component-failure-mode", "component-fragility", "failure-mode", "fragility-curve", "load-profile", "damage-intensity")
    For Each vSubName In ComponentSubArray
        Set ws = ThisWorkbook.Sheets(vSubName)
        lRowow = LastRow(ws)
        i = 0
        If ws.Name = "component-failure-mode" Then
            For iSubCntr = 2 To lRowow Step 1
                If ws.Cells(iSubCntr, 1) = idName Then
                    ReDim Preserve ModesIncludeListBox(i)
                    ModesIncludeListBox(i) = ws.Range("B" & iSubCntr).Value
                    i = i + 1
                End If
            Next iSubCntr
        ElseIf ws.Name = "component-fragility" Then
            For iSubCntr = 2 To lRowow Step 1
                If ws.Cells(iSubCntr, 1) = idName Then
                    ReDim Preserve CurvesIncludeListBox(i)
                    CurvesIncludeListBox(i) = ws.Range("B" & iSubCntr).Value
                    i = i + 1
                End If
            Next iSubCntr
        ElseIf ws.Name = "failure-mode" Then
            For iSubCntr = 2 To lRowow Step 1
                ReDim Preserve ModesListBox_orig(i)
                ModesListBox_orig(i) = ws.Range("A" & iSubCntr).Value
                i = i + 1
            Next iSubCntr
        ElseIf ws.Name = "fragility-curve" Then
            For iSubCntr = 2 To lRowow Step 1
                ReDim Preserve CurvesListBox_orig(i)
                CurvesListBox_orig(i) = ws.Range("A" & iSubCntr).Value
                i = i + 1
            Next iSubCntr
        ElseIf ws.Name = "load-profile" Then
            For iSubCntr = 2 To lRowow Step 1
                If ws.Cells(iSubCntr, 1) = idName Then
                    ReDim Preserve ProfileListBox(i)
                    ProfileListBox(i) = ws.Range("B" & iSubCntr).Value
                    i = i + 1
                End If
            Next iSubCntr
        ElseIf ws.Name = "damage-intensity" Then
            For iSubCntr = 2 To lRowow Step 1
                If ws.Cells(iSubCntr, 1) = idName Then
                    ReDim Preserve DamageListBox(i)
                    DamageListBox(i) = ws.Range("B" & iSubCntr).Value
                    i = i + 1
                End If
            Next iSubCntr
        End If
    Next vSubName
    
    Dim dup As Boolean: k = 0
    For i = LBound(ModesListBox_orig) To UBound(ModesListBox_orig) '~~> specify dimension
        dup = False
        For j = LBound(ModesIncludeListBox, 1) To UBound(ModesIncludeListBox) '~~> specify dimension
            If ModesListBox_orig(i) = ModesIncludeListBox(j) Then
                dup = True: Exit For
            End If
        Next j
        If Not dup Then '~~> transfer if not duplicate
            ReDim Preserve ModesListBox(k)
            ModesListBox(k) = ModesListBox_orig(i)
            k = k + 1
        End If
    Next i

    k = 0
    For i = LBound(CurvesListBox_orig) To UBound(CurvesListBox_orig) '~~> specify dimension
        dup = False
        For j = LBound(CurvesIncludeListBox, 1) To UBound(CurvesIncludeListBox) '~~> specify dimension
            If CurvesListBox_orig(i) = CurvesIncludeListBox(j) Then
                dup = True: Exit For
            End If
        Next j
        If Not dup Then '~~> transfer if not duplicate
            ReDim Preserve CurvesListBox(k)
            CurvesListBox(k) = CurvesListBox_orig(i)
            k = k + 1
        End If
    Next i
    
    ComponentSubs.ModesListBox = ModesListBox
    ComponentSubs.ModesIncludeListBox = ModesIncludeListBox
    ComponentSubs.CurvesListBox = CurvesListBox
    ComponentSubs.CurvesIncludeListBox = CurvesIncludeListBox
    ComponentSubs.ProfileListBox = ProfileListBox
    ComponentSubs.DamageListBox = DamageListBox
    getComponentSubs = ComponentSubs
End Function

Sub ExportAsCSV(ParentSheet As Worksheet)

    Dim Sep As String
    Dim MyFileName As String
    Dim CurrentWB As Workbook, TempWB As Workbook

    Sep = Application.PathSeparator
    Set CurrentWB = ActiveWorkbook
    ParentSheet.UsedRange.Copy

    Set TempWB = Application.Workbooks.Add(1)
    With TempWB.Sheets(1).Range("A1")
        .PasteSpecial xlPasteValues
        .PasteSpecial xlPasteFormats
    End With

    MyFileName = ThisWorkbook.Path & Sep & ParentSheet.Name & ".csv"

    Application.DisplayAlerts = False
    TempWB.SaveAs Filename:=MyFileName, FileFormat:=xlCSV, CreateBackup:=False, Local:=True
    TempWB.Close SaveChanges:=False
    Application.DisplayAlerts = True
End Sub

Public Sub ExportWorksheetAndSaveAsCSV(ParentSheet As Worksheet)

    Dim Sep As String
    Dim wbkExport As Workbook
    Dim shtToExport As Worksheet
    
    Sep = Application.PathSeparator
    Set shtToExport = ParentSheet     'Sheet to export as CSV
    Set wbkExport = Application.Workbooks.Add
    shtToExport.Copy Before:=wbkExport.Worksheets(wbkExport.Worksheets.Count)
    Application.DisplayAlerts = False                       'Possibly overwrite without asking
    wbkExport.SaveAs Filename:=ThisWorkbook.Path & Sep & ParentSheet.Name & ".csv", FileFormat:=xlCSV
    Application.DisplayAlerts = True
    wbkExport.Close SaveChanges:=False

End Sub

Function LastRow(Optional ParentSheet As Worksheet) As Long
' Extract the Last Row from the Excel Spreadsheet
    LastRow = ParentSheet.Cells(Rows.Count, 1).End(xlUp).Row
End Function

Function GetParameters(ParentSheet As Worksheet, i As Long) As String
    Dim j As Long
    Dim Arr As Variant
    Dim arr_str As String
    Dim last_row As Long
    Dim Value As Variant
    
    last_row = LastRow(ParentSheet)
    Arr = ParentSheet.Cells(2, i).Resize(last_row, i).Value
    arr_str = "["
    For j = 1 To last_row
        If WorksheetFunction.IsNumber(Arr(j, 1)) Then
            arr_str = arr_str & Arr(j, 1) & ", "
        Else
            arr_str = arr_str & """" & Arr(j, 1) & """, "
        End If
    Next j
    arr_str = Left(arr_str, Len(arr_str) - 6)
    arr_str = arr_str & "],"
    GetParameters = arr_str
    
End Function

Public Function ShellRowun(sCmd As String) As String
    'Run a shell command, returning the output as a string

    Dim oShell As Object
    Set oShell = CreateObject("WScript.Shell")
    Dim Sep As String
    Dim strFileExists As String
    
    Sep = Application.PathSeparator
    'run command
    Dim errorCode As Long
    Debug.Print sCmd
    errorCode = oShell.Run(sCmd, 1, True)
    strFileExists = Dir(ThisWorkbook.Path & Sep & "in.toml")
    If errorCode <> 0 Or strFileExists = "" Then
        MsgBox sCmd & " failed"
    Else
        MsgBox sCmd & " successful"
    End If

End Function

Public Function ShellExec(sCmd As String) As String
    'Run a shell command, returning the output as a string

    Dim oShell As Object
    Set oShell = CreateObject("WScript.Shell")
    
    'run command
    Dim oExec As Object
    Dim oOutput As Object
    Debug.Print sCmd
    Set oExec = oShell.Exec(sCmd)
    Set oOutput = oExec.stdout

    'handle the results as they are written to and read from the StdOut object
    Dim s As String
    Dim sLine As String
    While Not oOutput.AtEndOfStream
        sLine = oOutput.ReadLine
        If sLine <> "" Then s = s & sLine & vbCrLf
    Wend

    ShellExec = s
End Function

Sub WriteCaseFile(FilePath As String)
    Dim load_profile_parameters As Variant
    Dim scenario_parameters As Variant
    Dim building_level_parameters As Variant
    Dim arr_str As String
    Dim names As Variant
    Dim i As Long
    Dim names_length As Long
    
    load_profile_parameters = Array("load_profile_building_id", "load_profile_scenario_id", "load_profile_enduse", "load_profile_file")
    scenario_parameters = Array("scenario_id", "scenario_duration_in_hours", "scenario_max_occurrence", "scenario_fixed_frequency_in_years")
    building_level_parameters = Array("building_level_building_id", "building_level_egen_flag", "building_level_egen_eff_pct", "building_level_egen_peak_pwr_kW", "building_level_heat_storage_flag", "building_level_heat_storage_cap_kWh", "building_level_gas_boiler_flag", "building_level_gas_boiler_eff_pct", "building_level_gas_boiler_peak_heat_gen_kW", "building_level_echiller_flag", "building_level_echiller_peak_cooling_kW")
    
    Open (FilePath) For Output As #1
    'Print #1, "<% insert 'template.toml',"
    Print #1, "# General"
    Print #1, ":simulation_duration_in_years => " & Sheets("general").Range("simulation_dur_years").Value & ","
    Print #1, ":random_setting => " & """" & Sheets("general").Range("random_setting").Value & """" & ","
    Print #1, ":random_seed => " & Sheets("general").Range("random_seed").Value & ","
    Print #1, "# Load Profile"
    
    names_length = UBound(load_profile_parameters) - LBound(load_profile_parameters) + 1
    For i = 0 To names_length - 1
        arr_str = GetParameters(Sheets("load-profile"), i + 1)
        Print #1, ":" & load_profile_parameters(i) & " => " & arr_str
    Next i
    Print #1, "# Scenario"
    
    names_length = UBound(scenario_parameters) - LBound(scenario_parameters) + 1
    For i = 0 To names_length - 1
        arr_str = GetParameters(Sheets("scenario"), i + 1)
        Print #1, ":" & scenario_parameters(i) & " => " & arr_str
    Next i
    Print #1, "# Building-to-Cluster Connectivity"
    Print #1, "# Cluster-to-Community Connectivity"
    Print #1, "# Community-to-Utility Connectivity"
    Print #1, "# Building Level Configuration"
    
    names_length = UBound(building_level_parameters) - LBound(building_level_parameters) + 1
    For i = 0 To names_length - 1
        arr_str = GetParameters(Sheets("building-level"), i + 1)
        Print #1, ":" & building_level_parameters(i) & " => " & arr_str
    Next i
    
    Close #1
End Sub

Sub Run()
    Dim Sep As String
    Dim IsWin As Boolean
    Dim IsMac As Boolean
    Dim Path As String
    Dim folder As Object
    Dim pxt_path As String
    Dim template_path As String
    Dim output_path As String
    Dim modelkit_path As String
    Dim array_str As String
    Dim exe_path As String
    Dim input_file_path As String
    Dim output_file_path As String
    Dim stats_file_path As String
    Dim Cmd As String
    Dim FilePath As String
    Dim stdout As String
    Dim strLine As String
    Dim Total As String
    Dim i As Integer
    Dim strFileExists As String
    
    Sep = Application.PathSeparator

    If Sep = "\" Then
        IsWin = True
    ElseIf Sep = "/" Then
        IsMac = True
    End If
    
    ExportAsCSV Sheets("component-failure-mode")
    ExportAsCSV Sheets("component-fragility")
    ExportAsCSV Sheets("converter-component")
    ExportAsCSV Sheets("damage-intensity")
    ExportAsCSV Sheets("dual-outflow-converter-comp")
    ExportAsCSV Sheets("failure-mode")
    ExportAsCSV Sheets("fixed-cdf")
    ExportAsCSV Sheets("fragility-curve")
    ExportAsCSV Sheets("general")
    ExportAsCSV Sheets("load-component")
    ExportAsCSV Sheets("load-profile")
    ExportAsCSV Sheets("mover-component")
    ExportAsCSV Sheets("muxer-component")
    ExportAsCSV Sheets("network-link")
    ExportAsCSV Sheets("pass-through-component")
    ExportAsCSV Sheets("scenario")
    ExportAsCSV Sheets("source-component")
    ExportAsCSV Sheets("storage-component")
    ExportAsCSV Sheets("uncontrolled-src")
    
    Path = ThisWorkbook.Path & Sep & "in.toml"
    Set folder = CreateObject("scripting.filesystemobject")

    If folder.FileExists(Path) Then
        folder.DeleteFile Path, True
    End If
    
    'pxt_path = ThisWorkbook.Path & Sep & "in.pxt"
    template_path = ThisWorkbook.Path & Sep & "template.toml"
    output_path = ThisWorkbook.Path & Sep & "in.toml"
    'modelkit_path = "C:\Program Files (x86)\Modelkit Catalyst\bin\modelkit.bat"
    'WriteCaseFile (pxt_path)
    
    Cmd = "modelkit template-compose --output=""" & output_path & """ " & """" & template_path & """"
    ShellRowun (Cmd)
    
    If IsWin Then
        On Error Resume Next
        MkDir ThisWorkbook.Path & Sep & "output"
        On Error GoTo 0
        exe_path = Sheets("Settings").Range("exe_path").Value
        input_file_path = ThisWorkbook.Path & Sep & "in.toml"
        output_file_path = ThisWorkbook.Path & Sep & "output" & Sep & "out.csv"
        stats_file_path = ThisWorkbook.Path & Sep & "output" & Sep & "stats.csv"
        Cmd = """" & exe_path & """ " & """" & input_file_path & """ " & """" & output_file_path & """ " & """" & stats_file_path & """"
        stdout = ShellExec(Cmd)
        MsgBox (stdout)
        FilePath = ThisWorkbook.Path & Sep & "output" & Sep & "stdout"
    ElseIf IsMac Then
        'RunPython ("import e2rin_gui; e2rin_gui.run()")
        'FilePath = "/Users/matthew-larson/Documents/My Projects/e2rin/gui/stdout"
    End If
    
    Open ThisWorkbook.Path & Sep & "output" & Sep & "stdout" For Output As #1
    Print #1, stdout
    Close #1
    
    strFileExists = Dir(output_file_path)
    If Not strFileExists = "" Then
        CSVOutput (output_file_path)
        CSVOutput (stats_file_path)
    End If
    Sheets("Scenarios").Activate
    ActiveWorkbook.Save
    
End Sub

Sub CSVOutput(file_path As String)
 
 Dim otherWB As Workbook, ThisWB As Workbook
 Dim testWS As Worksheet
 
 Application.ScreenUpdating = False
 
 Set ThisWB = ActiveWorkbook
 Set otherWB = Workbooks.Open(file_path)
 Set testWS = ThisWB.Sheets(ActiveSheet.Name)
 
 If Not testWS Is Nothing Then
 Application.DisplayAlerts = False
 testWS.Delete
 Application.DisplayAlerts = True
 End If
 
 ActiveSheet.Copy after:=ThisWB.Sheets(ThisWB.Sheets.Count)
 
 otherWB.Close False
 ThisWB.Activate
 
 Set ThisWB = Nothing
 Set otherWB = Nothing
 Set testWS = Nothing
 
 Application.ScreenUpdating = True
 
End Sub

Sub ClearResults()
    Dim folder As Object
    Dim Path As String
    Dim Sep As String
    
    Sep = Application.PathSeparator
    Path = ThisWorkbook.Path & Sep & "output"
    Set folder = CreateObject("scripting.filesystemobject")

    If MsgBox("This will erase all your results! Are you sure?", vbYesNo) = vbNo Then
        Exit Sub
    End If
    
    Sheets("out").Cells.Clear
    Sheets("stats").Cells.Clear

    If folder.FolderExists(Path) Then
        folder.DeleteFolder Path, True
    End If
    
    MsgBox "The 'out' and 'stats' tabs have been cleared and the 'output' folder and it's contents have been deleted."

End Sub

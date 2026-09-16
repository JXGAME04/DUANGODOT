; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CAutoupdateDlg
LastTemplate=CProgressCtrl
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "autoupdate.h"
LastPage=0

ClassCount=7
Class1=CAutoupdateApp
Class2=CAboutDlg
Class3=CAutoupdateDlg
Class4=CStartDlg

ResourceCount=3
Resource1=IDD_AUTOUPDATE_DIALOG
Resource2=IDD_ABOUTBOX
Class5=CCQImageButton
Class6=CCQStatic
Class7=BitMapProgress
Resource3=IDD_START

[CLS:CAutoupdateApp]
Type=0
BaseClass=CWinApp
HeaderFile=Autoupdate.h
ImplementationFile=Autoupdate.cpp
LastObject=CAutoupdateApp

[CLS:CAboutDlg]
Type=0
BaseClass=CDialog
HeaderFile=AutoupdateDlg.cpp
ImplementationFile=AutoupdateDlg.cpp
Filter=D
LastObject=CAboutDlg

[CLS:CAutoupdateDlg]
Type=0
BaseClass=CDialog
HeaderFile=AutoupdateDlg.h
ImplementationFile=AutoupdateDlg.cpp
LastObject=CAutoupdateDlg
Filter=D
VirtualFilter=dWC

[CLS:CStartDlg]
Type=0
BaseClass=CDialog
HeaderFile=StartDlg.h
ImplementationFile=StartDlg.cpp
LastObject=IDC_STATIC_PROMPT

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_AUTOUPDATE_DIALOG]
Type=1
Class=CAutoupdateDlg
ControlCount=10
Control1=IDOK,button,1476395008
Control2=IDCANCEL,button,1342177280
Control3=IDC_PROGRESSCURRENT,msctls_progress32,1342177280
Control4=IDC_STATIC_VERSON,static,1073872897
Control5=IDC_STATIC_PROGRESSCURRENT,static,1073872897
Control6=IDC_STATIC_PROGRESS,static,1073872908
Control7=IDC_PROGRESS,msctls_progress32,1342177280
Control8=IDC_EXPLORER1,{8856F961-340A-11D0-A96B-00C04FD705A2},1342177280
Control9=IDC_REGISTER,button,1342177280
Control10=IDC_INVEST,button,1342177280

[DLG:IDD_START]
Type=1
Class=CStartDlg
ControlCount=1
Control1=IDC_STATIC_PROMPT,static,1342308352

[CLS:CCQImageButton]
Type=0
HeaderFile=CQImageButton.h
ImplementationFile=CQImageButton.cpp
BaseClass=CButton
Filter=W
LastObject=CCQImageButton
VirtualFilter=BWC

[CLS:CCQStatic]
Type=0
HeaderFile=CQStatic.h
ImplementationFile=CQStatic.cpp
BaseClass=CStatic
Filter=W
LastObject=CCQStatic
VirtualFilter=WC

[CLS:BitMapProgress]
Type=0
HeaderFile=BitMapProgress.h
ImplementationFile=BitMapProgress.cpp
BaseClass=CProgressCtrl
Filter=W
VirtualFilter=NWC
LastObject=BitMapProgress


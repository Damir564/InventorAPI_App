
// WSCADDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "WSCAD.h"
#include "WSCADDlg.h"
#include "afxdialogex.h"

#define PI 4*atan(1)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma once
//#include <atlbase.h>
#import <C:\\Program Files\\Autodesk\\Inventor 2022\\Bin\\rxinventor.tlb> \
rename_namespace("InventorNative") \
named_guids raw_dispinterfaces \
high_method_prefix("Method") \
rename("DeleteFile", "APIDeleteFile") \
rename("CopyFile", "APICopyFile") \
rename("MoveFile", "APIMoveFile") \
rename("SetEnvironmentVariable", "APISetEnvironmentVariable") \
rename("GetObject", "APIGetObject") \
exclude("_FILETIME", "IStream", "ISequentialStream", \
"_LARGE_INTEGER", "_ULARGE_INTEGER", "tagSTATSTG", \
"IEnumUnknown", "IPersistFile", "IPersist", "IPictureDisp")

// Для того, чтобы подсвечивалось, нажал Project -> Properties -> C/C++ -> General -> 
// Additional include Directories: C:\Projects\VS\WSCAD\WSCAD\x64\Debug (там, где есть rxinventor.tlb)
using namespace InventorNative;

CComPtr<Application> pInvApp; //приложение

PartDocumentPtr pPartDoc;  //документ
PartComponentDefinition* pPartComDef;//компоненты детали
TransientGeometry* pTransGeom; //геометрия детали
Transaction* pTrans; //операции

TransactionManagerPtr pTransManager; //менеджер операций


PlanarSketches* sketches; // эскизы
PartFeatures* ft; //элементы детали 

WorkPlanes* wp; //рабочие плоскости
WorkAxes* wax;//рабочие оси
WorkPoints* wpt;//рабочие точки


CWSCADDlg::CWSCADDlg(CWnd* pParent)
	: CDialogEx(IDD_WSCAD_DIALOG, pParent)
	, m_otvX(8.5f)
	, m_otvY(13.5f)
	, m_otvR(1.5f)
	, m_baseH(6.0f)
	, m_headH(11.0f)
	, m_lengthL1(12.0f)
	, m_lengthL2(18.0f)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWSCADDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_OTV_X, m_otvX);
	DDV_MinMaxFloat(pDX, m_otvX, 1, 9);
	DDX_Text(pDX, IDC_EDIT_OTV_Y, m_otvY);
	DDV_MinMaxFloat(pDX, m_otvY, 5, 16);
	DDX_Text(pDX, IDC_EDIT_OTV_R, m_otvR);
	DDV_MinMaxFloat(pDX, m_otvR, 0.1f, 2.4f);
	DDX_Text(pDX, IDC_EDIT_BASE_H, m_baseH);
	DDV_MinMaxFloat(pDX, m_baseH, 2.0f, 10000.0f);
	DDX_Text(pDX, IDC_EDIT_HEAD_H, m_headH);
	DDV_MinMaxFloat(pDX, m_headH, 5.0, 10000.0f);
	DDX_Text(pDX, IDC_EDIT_BASE_L1, m_lengthL1);
	DDV_MinMaxFloat(pDX, m_lengthL1, 8.0, 35.0f);
	DDX_Text(pDX, IDC_EDIT_BASE_L2, m_lengthL2);
	DDV_MinMaxFloat(pDX, m_lengthL2, m_lengthL1, 40.0f);
}

BEGIN_MESSAGE_MAP(CWSCADDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CWSCADDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CWSCADDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDOK, &CWSCADDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CWSCADDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWSCADDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWSCADDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool CWSCADDlg::CheckData()
{
	if (!UpdateData())
		return false;

	m_otvX /= 10;
	m_otvY /= 10;
	m_otvR /= 10;
	m_baseH /= 10;
	m_headH /= 10;
	m_lengthL1 /= 10;
	m_lengthL2 /= 10;

	return true;
}

void CWSCADDlg::OnBnClickedButton1()
{
	// TODO: добавьте свой код обработчика уведомлений
	BeginWaitCursor();

	if (!CheckData())
		return;

	if (!pInvApp)
	{
		// Get hold of the program id of Inventor.
		CLSID InvAppClsid;
		HRESULT hRes = CLSIDFromProgID(L"Inventor.Application", &InvAppClsid);
		if (FAILED(hRes)) {
			pInvApp = nullptr;
			return;// ReturnAndShowCOMError(hRes, L"ConnectToInventor,CLSIDFromProgID failed");
		}

		// See if Inventor is already running...
		CComPtr<IUnknown> pInvAppUnk = nullptr;
		hRes = ::GetActiveObject(InvAppClsid, NULL, &pInvAppUnk);
		if (FAILED(hRes)) {
			// Inventor is not already running, so try to start it...
			TRACE(L"Could not get hold of an active Inventor, will start a new session\n");
			hRes = CoCreateInstance(InvAppClsid, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IUnknown), (void**)&pInvAppUnk);
			if (FAILED(hRes)) {
				pInvApp = nullptr;
				return;
			}
		}

		// Get the pointer to the Inventor application...
		hRes = pInvAppUnk->QueryInterface(__uuidof(Application), (void**)&pInvApp);
		if (FAILED(hRes)) {
			return;
		}
	}
	pInvApp->put_Visible(TRUE);

	pPartDoc = pInvApp->Documents->MethodAdd(kPartDocumentObject, pInvApp->FileManager->MethodGetTemplateFile(kPartDocumentObject, kMetricSystemOfMeasure, kGOST_DraftingStandard), true);
	pPartDoc->DisplayName = _T("Конфигуратор");
	pPartDoc->get_ComponentDefinition(&pPartComDef);
	pInvApp->get_TransientGeometry(&pTransGeom);
	pTransManager = pInvApp->GetTransactionManager();
	Document* Doc = CComQIPtr <Document>(pPartDoc);
	pTransManager->raw_StartTransaction(Doc, _T("Конфигуратор"), &pTrans);
	pPartComDef->get_Sketches(&sketches);

	pPartComDef->get_WorkPlanes(&wp);
	pPartComDef->get_Features(&ft);
	pPartComDef->get_WorkAxes(&wax);
	pPartComDef->get_WorkPoints(&wpt);
	ExtrudeFeaturesPtr ftExtrude;
	FilletFeaturesPtr pFilletFt;
	ft->get_FilletFeatures(&pFilletFt);
	PlanarSketchPtr pSketch;
	sketches->raw_Add(wp->GetItem(2), false, &pSketch);
	SketchPointsPtr skPoints;
	SketchLinesPtr skLines;
	Profiles* skProfiles;
	pSketch->get_SketchPoints(&skPoints);
	pSketch->get_SketchLines(&skLines);
	pSketch->get_Profiles(&skProfiles);
	SketchPointPtr point[6];
	SketchLinesPtr line[6];
	point[0] = skPoints->MethodAdd(pTransGeom->MethodCreatePoint2d(2.10f, 0.0f), false);
	point[1] = skPoints->MethodAdd(pTransGeom->MethodCreatePoint2d(1.05f, -1.82f), false);
	point[2] = skPoints->MethodAdd(pTransGeom->MethodCreatePoint2d(-1.05f, -1.82f), false);
	point[3] = skPoints->MethodAdd(pTransGeom->MethodCreatePoint2d(-2.10f, 0.0f), false);
	point[4] = skPoints->MethodAdd(pTransGeom->MethodCreatePoint2d(-1.05f, 1.82f), false);
	point[5] = skPoints->MethodAdd(pTransGeom->MethodCreatePoint2d(1.05f, 1.82f), false);
	for (int i = 0; i != 6; ++i)
	{
		line[i] = skLines->MethodAddByTwoPoints(point[i], point[(i + 1) % 6]);
	}
	ProfilePtr pProfile;
	try
	{
		pProfile = skProfiles->MethodAddForSolid(true);
	}catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}

	ft->get_ExtrudeFeatures(&ftExtrude);
	ExtrudeFeaturePtr extrude = ftExtrude->MethodAddByDistanceExtent(pProfile, m_baseH, kNegativeExtentDirection, kJoinOperation);

	PlanarSketchPtr pSketch1;
	SketchPointsPtr skPoints1;
	SketchLinesPtr skLines1;
	SketchCirclesPtr skCircles1;
	Profiles* skProfiles1;

	WorkPlaneDefinitionEnum workPlaneDef();
	
	sketches->raw_Add(wp->GetItem(2), false, &pSketch1);
	pSketch1->get_SketchPoints(&skPoints1);
	pSketch1->get_SketchLines(&skLines1);
	pSketch1->get_SketchCircles(&skCircles1);
	pSketch1->get_Profiles(&skProfiles1);
	SketchPointPtr point1[4];
	SketchCirclePtr circle1[4];
	point1[0] = skPoints1->MethodAdd(pTransGeom->MethodCreatePoint2d(m_otvX, m_otvY), false);
	point1[1] = skPoints1->MethodAdd(pTransGeom->MethodCreatePoint2d(m_otvX, -m_otvY), false);
	point1[2] = skPoints1->MethodAdd(pTransGeom->MethodCreatePoint2d(-m_otvX, -m_otvY), false);
	point1[3] = skPoints1->MethodAdd(pTransGeom->MethodCreatePoint2d(-m_otvX, m_otvY), false);
	for (int i = 0; i != 4; ++i)
	{
		circle1[i] = skCircles1->MethodAddByCenterRadius(point1[i], m_otvR);
	}


	ProfilePtr pProfile1;
	try
	{
		pProfile1 = skProfiles1->MethodAddForSolid(true);
	}catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}
	ExtrudeFeaturePtr extrude1 = ftExtrude->MethodAddByDistanceExtent(pProfile1, m_baseH, kNegativeExtentDirection, kCutOperation);


	PlanarSketchPtr pSketch2;
	SketchPointsPtr skPoints2;
	SketchLinesPtr skLines2;
	SketchCirclesPtr skCircles2;
	Profiles* skProfiles2;
	sketches->raw_Add(wp->GetItem(2), false, &pSketch2);
	pSketch2->get_SketchPoints(&skPoints2);
	pSketch2->get_SketchLines(&skLines2);
	pSketch2->get_Profiles(&skProfiles2);
	SketchPointPtr point2[10];
	SketchLinePtr line2[8];
	point2[0] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(2.2f, 1.0f), false);
	point2[1] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(2.2f, -1.0f), false);
	point2[2] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(0.4f, -1.0f), false);
	point2[3] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(0.4f, 1.0f), false);

	point2[4] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(-2.20f, -1.0f), false);
	point2[5] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(-2.20f, 1.0f), false);
	point2[6] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(-0.4f, 1.0f), false);
	point2[7] = skPoints2->MethodAdd(pTransGeom->MethodCreatePoint2d(-0.4f, -1.0f), false);
	for (int i = 0; i != 4; ++i)
	{
		line2[i] = skLines2->MethodAddByTwoPoints(point2[i], point2[(i + 1) % 4]);
	}
	for (int i = 4; i != 8; ++i)
	{
		line2[i] = skLines2->MethodAddByTwoPoints(point2[i], point2[(i + 1) % 8 != 0 ? (i + 1) : 4]);
	}
	ProfilePtr pProfile2;
	try
	{
		pProfile2 = skProfiles2->MethodAddForSolid(true);
	}
	catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}

	ExtrudeFeaturePtr extrude2 = ftExtrude->MethodAddByDistanceExtent(pProfile2, m_headH, kPositiveExtentDirection, kJoinOperation);

	PlanarSketchPtr pSketch3;
	wp->MethodAddByPlaneAndOffset(wp->GetItem(2), -0.6, true);
	wp->GetItem(4)->PutVisible(false);
	sketches->raw_Add(wp->GetItem(4), false, &pSketch3);
	SketchPointsPtr skPoints3;
	SketchLinesPtr skLines3;
	Profiles* skProfiles3;
	pSketch3->get_SketchPoints(&skPoints3);
	pSketch3->get_SketchLines(&skLines3);
	pSketch3->get_Profiles(&skProfiles3);
	SketchPointPtr point3[6];
	SketchLinesPtr line3[6];
	point3[0] = skPoints3->MethodAdd(pTransGeom->MethodCreatePoint2d(2.1f, 0.0f), false);
	point3[1] = skPoints3->MethodAdd(pTransGeom->MethodCreatePoint2d(1.05f, -1.82f), false);
	point3[2] = skPoints3->MethodAdd(pTransGeom->MethodCreatePoint2d(-1.05f, -1.82f), false);
	point3[3] = skPoints3->MethodAdd(pTransGeom->MethodCreatePoint2d(-2.10f, 0.0f), false);
	point3[4] = skPoints3->MethodAdd(pTransGeom->MethodCreatePoint2d(-1.05f, 1.82f), false);
	point3[5] = skPoints3->MethodAdd(pTransGeom->MethodCreatePoint2d(1.05f, 1.82f), false);
	for (int i = 0; i != 6; ++i)
		line3[i] = skLines3->MethodAddByTwoPoints(point3[i], point3[(i + 1) % 6]);
	ProfilePtr pProfile3;
	try
	{
		pProfile3 = skProfiles3->MethodAddForSolid(true);
	}catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}
	ExtrudeFeaturePtr extrude3 = ftExtrude->MethodAddByDistanceExtent(pProfile3, m_headH + m_baseH, kPositiveExtentDirection, kIntersectOperation);

	PlanarSketchPtr pSketch4;
	sketches->raw_Add(wp->GetItem(1), false, &pSketch4);
	SketchPointsPtr skPoints4;
	SketchCirclesPtr skCircles4;
	Profiles* skProfiles4;
	pSketch4->get_SketchPoints(&skPoints4);
	pSketch4->get_SketchCircles(&skCircles4);
	pSketch4->get_Profiles(&skProfiles4);
	SketchPointPtr point4;
	SketchCirclePtr circle4;
	point4 = skPoints4->MethodAdd(pTransGeom->MethodCreatePoint2d(m_headH, 0.0f), false);
	circle4 = skCircles4->MethodAddByCenterRadius(point4, 0.8f);
	ProfilePtr pProfile4;
	try
	{
		pProfile4 = skProfiles4->MethodAddForSolid(true);
	}catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}
	ExtrudeFeaturePtr extrude4 = ftExtrude->MethodAddByDistanceExtent(pProfile4, m_lengthL1, kSymmetricExtentDirection, kCutOperation);

	PlanarSketchPtr pSketch5;
	sketches->raw_Add(wp->GetItem(1), false, &pSketch5);
	SketchPointsPtr skPoints5;
	SketchLinesPtr skLines5;
	Profiles* skProfiles5;
	pSketch5->get_SketchPoints(&skPoints5);
	pSketch5->get_SketchLines(&skLines5);
	pSketch5->get_Profiles(&skProfiles5);
	SketchPointPtr point5[6];
	SketchLinesPtr line5[6];
	point5[0] = skPoints5->MethodAdd(pTransGeom->MethodCreatePoint2d(m_headH - 0.65f, 0.0f), false);
	point5[1] = skPoints5->MethodAdd(pTransGeom->MethodCreatePoint2d(m_headH - 0.65f + 0.325f, 0.5629f), false);
	point5[2] = skPoints5->MethodAdd(pTransGeom->MethodCreatePoint2d(m_headH - 0.65f + 0.95f, 0.5629f), false);
	point5[3] = skPoints5->MethodAdd(pTransGeom->MethodCreatePoint2d(m_headH - 0.65f + 1.3f, 0.0f), false);
	point5[4] = skPoints5->MethodAdd(pTransGeom->MethodCreatePoint2d(m_headH - 0.65f + 0.95f, -0.5629f), false);
	point5[5] = skPoints5->MethodAdd(pTransGeom->MethodCreatePoint2d(m_headH - 0.65f + 0.325f, -0.5629f), false);
	for (int i = 0; i != 6; ++i)
		line5[i] = skLines5->MethodAddByTwoPoints(point5[i], point5[(i + 1) % 6]);
	ProfilePtr pProfile5;
	try
	{
		pProfile5 = skProfiles5->MethodAddForSolid(true);
	}catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}
	ExtrudeFeaturePtr extrude5 = ftExtrude->MethodAddByDistanceExtent(pProfile5, m_lengthL2, kSymmetricExtentDirection, kCutOperation);
	
	PlanarSketchPtr pSketch6;
	sketches->raw_Add(wp->GetItem(1), false, &pSketch6);
	SketchPointsPtr skPoints6;
	SketchCirclesPtr skCircles6;
	Profiles* skProfiles6;
	pSketch6->get_SketchPoints(&skPoints6);
	pSketch6->get_SketchCircles(&skCircles6);
	pSketch6->get_Profiles(&skProfiles6);
	SketchPointPtr point6;
	SketchCirclePtr circle6;
	point6 = skPoints6->MethodAdd(pTransGeom->MethodCreatePoint2d(1.1f, 0.0f), false);
	circle6 = skCircles6->MethodAddByCenterRadius(point6, 0.39f);
	ProfilePtr pProfile6;
	try
	{
		pProfile6 = skProfiles6->MethodAddForSolid(true);
	}catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}
	ExtrudeFeaturePtr extrude6 = ftExtrude->MethodAddByDistanceExtent(pProfile6, 4.2f, kSymmetricExtentDirection, kCutOperation);


	PlanarSketchPtr pSketch7;
	sketches->raw_Add(wp->GetItem(3), false, &pSketch7);
	SketchPointsPtr skPoints7;
	SketchLinesPtr skLines7;
	Profiles* skProfiles7;
	pSketch7->get_SketchPoints(&skPoints7);
	pSketch7->get_SketchLines(&skLines7);
	pSketch7->get_Profiles(&skProfiles7);
	SketchPointPtr point7[8];
	SketchLinesPtr line7[8];
	point7[0] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(0.675f, 0.15f), false);
	point7[1] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(0.675f, 0.35f), false);
	point7[2] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(1.025f, 0.35f), false);
	point7[3] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(1.025f, 0.15f), false);
	point7[4] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(-0.675f, 0.15f), false);
	point7[5] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(-0.675f, 0.35f), false);
	point7[6] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(-1.025f, 0.35f), false);
	point7[7] = skPoints7->MethodAdd(pTransGeom->MethodCreatePoint2d(-1.025f, 0.15f), false);
	for (int i = 0; i != 4; ++i)
		line7[i] = skLines7->MethodAddByTwoPoints(point7[i], point7[(i + 1) % 4]);
	for (int i = 4; i != 8; ++i)
		line7[i] = skLines7->MethodAddByTwoPoints(point7[i], point7[((i + 1) % 8) == 0 ? 4 : (i + 1)]);
	ProfilePtr pProfile7;
	try
	{
		pProfile7 = skProfiles7->MethodAddForSolid(true);
	}catch (...)
	{
		AfxMessageBox(L"Ошибочный контур!");
		return;
	}
	ExtrudeFeaturePtr extrude7 = ftExtrude->MethodAddByDistanceExtent(pProfile7, 2.0f, kSymmetricExtentDirection, kCutOperation);

	EdgeCollectionPtr edgeColl;
	pInvApp->TransientObjects->raw_CreateEdgeCollection(vtMissing, &edgeColl);
	SurfaceBodyPtr SurfBody;
	SurfaceBodiesPtr SurfBodies;
	pPartComDef->get_SurfaceBodies(&SurfBodies);
	SurfBodies->get_Item(1, &SurfBody);
	EdgesPtr edges;
	SurfBody->get_Edges(&edges);
	EdgePtr ed;
	for (int i = 112; i != 120; i += 2)
	{
		edges->get_Item(i, &ed);
		edgeColl->MethodAdd(ed);
	}
	FilletFeaturePtr fillFeat = pFilletFt->MethodAddSimple(edgeColl, 0.15f, false, false, false, false, false, false);
	pTrans->MethodEnd();
}

void CWSCADDlg::OnBnClickedButton2()
{
	SelectSet* pSelect;
	pPartDoc->get_SelectSet(&pSelect);
	if (pSelect->GetCount() > 0)
	{
		EdgePtr Seekedge = pSelect->GetItem(1);
		int seeknumber = -1;
		for (int i = 1; i <= pPartComDef->SurfaceBodies->GetCount(); i++)
		{
			SurfaceBody* SurfBody;
			SurfaceBodies* SurfBodies;
			pPartComDef->get_SurfaceBodies(&SurfBodies);
			SurfBodies->get_Item(i, &SurfBody);
			Edge* edge;
			Edges* edges;
			SurfBody->get_Edges(&edges);
			int N = SurfBody->Edges->GetCount();
			for (int j = 1; j <= SurfBody->Edges->GetCount(); j++)
			{
				edges->get_Item(j, &edge);
				if (Seekedge == edge)
				{
					seeknumber = j;
					CString str;
					str.Format(L"%i", j);
					MessageBox(str);

				}
			}
		}
	}
}

void CWSCADDlg::OnBnClickedOk()
{
	if (pInvApp)
		pInvApp->MethodQuit();

	CDialog::OnOK();
}

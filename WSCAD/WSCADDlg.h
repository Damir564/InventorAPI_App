
// WSCADDlg.h : header file
//

#pragma once


// CWSCADDlg dialog
class CWSCADDlg : public CDialogEx
{
// Construction
public:
	CWSCADDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WSCAD_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	// afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:

	afx_msg void OnBnClickedButton1();
	bool CheckData();

	int m_NOtv;
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedOk();
	float m_otvX;
	float m_otvY;
	float m_otvR;
	float m_baseH;
	float m_headH;
	float m_lengthL1;
	float m_lengthL2;
};

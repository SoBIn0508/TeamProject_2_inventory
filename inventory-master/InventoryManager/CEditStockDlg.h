#pragma once
#include "afxdialogex.h"


// CEditStockDlg 대화 상자

class CEditStockDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CEditStockDlg)

public:
	CEditStockDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CEditStockDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_EDIT_STOCK1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	// ========================================
	// 멤버 변수 (입력값)
	// ========================================
	int m_nOptionID;              // 옵션 ID (PK)
	CString m_strOptionCode;      // 품번
	CString m_strProductName;     // 상품명
	int m_nCurrentStock;          // 현재 재고

	// ========================================
	// 멤버 변수 (출력값)
	// ========================================
	int m_nNewStock;              // 새 재고 수량 (사용자 입력)

	// ========================================
	// UI 컨트롤 변수
	// ========================================
	CStatic m_staticOptionCode;
	CStatic m_staticProductName;
	CStatic m_staticCurrentStock;
	CEdit m_editNewStock;

	// ========================================
	// 메시지 핸들러
	// ========================================
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};

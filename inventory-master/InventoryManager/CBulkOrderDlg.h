#pragma once
#include "afxdialogex.h"


// CBulkOrderDlg 대화 상자

class CBulkOrderDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBulkOrderDlg)

public:
	CBulkOrderDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CBulkOrderDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BULK_ORDER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	// ===== 멤버 변수 =====
	int m_nSelectedItemCount; // [입력] 외부에서 전달받을 선택된 품목 개수
	int m_nOrderQuantity;     // [출력] 사용자가 입력할 발주 수량

	// ===== 메시지 핸들러 =====
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};

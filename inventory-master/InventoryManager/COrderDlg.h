#pragma once

// COrderDlg 대화 상자

class COrderDlg : public CDialogEx
{
	DECLARE_DYNAMIC(COrderDlg)

public:
	COrderDlg(CWnd* pParent = nullptr);
	virtual ~COrderDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ORDER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

public:
	// ========================================
	// 입력 데이터 (외부에서 설정)
	// ========================================
	int m_nOptionID;             // 옵션 ID
	CString m_strOptionCode;     // 품번
	CString m_strProductName;    // 상품명
	int m_nCurrentStock;         // 현재 재고
	int m_nWarningThreshold;     // '주의' 재고 기준
	int m_nDangerThreshold;      // '위험' 재고 기준

	// ========================================
	// 출력 데이터
	// ========================================
	int m_nOrderQuantity;        // 발주 수량

	// ========================================
	// 멤버 함수
	// ========================================
	void UpdateExpectedStock();
	virtual BOOL OnInitDialog();

	// ========================================
	// 이벤트 핸들러
	// ========================================
	afx_msg void OnBnClickedButtonMinus10();
	afx_msg void OnBnClickedButtonMinus5();
	afx_msg void OnBnClickedButtonPlus5();
	afx_msg void OnBnClickedButtonPlus10();
	afx_msg void OnEnChangeEditOrderQuantity();
	afx_msg void OnBnClickedOk();
};
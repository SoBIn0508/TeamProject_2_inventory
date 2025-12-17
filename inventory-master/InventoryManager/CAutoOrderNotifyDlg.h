#pragma once
#include "afxdialogex.h"
#include "DBManager.h"
#include <vector>

// 자동 발주 대상 품목 정보 구조체
struct AUTO_ORDER_ITEM
{
	int nOptionID;           // 옵션 ID
	CString strOptionCode;   // 품번
	CString strProductName;  // 상품명
	int nCurrentStock;       // 현재 재고
	int nOrderQuantity;      // 발주 예정 수량
	int nExpectedStock;      // 발주 후 예상 재고
};

class CAutoOrderNotifyDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAutoOrderNotifyDlg)

public:
	CAutoOrderNotifyDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CAutoOrderNotifyDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AUTO_ORDER_NOTIFY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog(); 
	DECLARE_MESSAGE_MAP()
public:
	CStatic m_staticSummary;
	CListCtrl m_listItems;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClickedButtonIgnore();

	// 외부에서 전달받을 데이터
	std::vector<AUTO_ORDER_ITEM> m_vecOrderItems;

	// 멤버 함수
	void InitList();        // 리스트 컨트롤 초기화
	void UpdateSummary();   // 요약 정보 업데이트
	void LoadItems();       // 품목 목록 로드
};

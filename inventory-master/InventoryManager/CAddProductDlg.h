
#pragma once
#include "afxdialogex.h"

// CAddProductDlg 대화 상자
class CAddProductDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAddProductDlg)

public:
	CAddProductDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CAddProductDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADD_PRODUCT };
#endif

public:
	// DDX/DDV를 위한 변수들
	CString m_strProductName;
	int m_nStock;//재고변수
	int m_nPrice; // ✅ 가격 변수 추가
	CString m_strDescription; //설명입력
	CComboBox m_comboBrand;
	CComboBox m_comboColor;
	CComboBox m_comboSize;
	CComboBox m_comboWare;     // 사용자님이 사용하는 변수명 그대로 유지
	CEdit m_editPreviewNum;      // 'IDC_PRODUCT_NUM' 컨트롤과 연결될 변수
	CString m_strImageUrl;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:
	// 품번 미리보기를 위한 공통 함수
	void UpdatePreviewCode();

public:
	// 이벤트 핸들러 함수들
	afx_msg void OnCbnSelchangeComboBrand();
	afx_msg void OnCbnSelchangeComboColor();
	afx_msg void OnCbnSelchangeComboSize();
	afx_msg void OnCbnSelchangeComboWere(); // 사용자님이 사용하는 함수명 그대로 유지
};

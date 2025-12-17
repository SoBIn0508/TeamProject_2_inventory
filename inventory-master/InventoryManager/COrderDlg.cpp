#include "pch.h"
#include "InventoryManager.h"
#include "COrderDlg.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(COrderDlg, CDialogEx)

COrderDlg::COrderDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ORDER, pParent)
	, m_nOptionID(0)
	, m_nCurrentStock(0)
	, m_nOrderQuantity(50)
	, m_strOptionCode(_T(""))
	, m_strProductName(_T(""))
	, m_nWarningThreshold(30)   // 기본값 설정 (추후 덮어씀)
	, m_nDangerThreshold(10)    // 기본값 설정 (추후 덮어씀)
{
}

COrderDlg::~COrderDlg()
{
}

void COrderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

}

BEGIN_MESSAGE_MAP(COrderDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_MINUS_10, &COrderDlg::OnBnClickedButtonMinus10)
	ON_BN_CLICKED(IDC_BUTTON_MINUS_5, &COrderDlg::OnBnClickedButtonMinus5)
	ON_BN_CLICKED(IDC_BUTTON_PLUS_5, &COrderDlg::OnBnClickedButtonPlus5)
	ON_BN_CLICKED(IDC_BUTTON_PLUS_10, &COrderDlg::OnBnClickedButtonPlus10)
	ON_EN_CHANGE(IDC_EDIT_ORDER_QUANTITY, &COrderDlg::OnEnChangeEditOrderQuantity)
	ON_BN_CLICKED(IDOK, &COrderDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL COrderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString strInfo;

	strInfo.Format(_T("품번: %s"), m_strOptionCode);
	SetDlgItemText(IDC_STATIC_OPTION_CODE, strInfo);

	strInfo.Format(_T("상품명: %s"), m_strProductName);
	SetDlgItemText(IDC_STATIC_PRODUCT_NAME, strInfo);

	CString strStatus;
	if (m_nCurrentStock == 0)
		strStatus = _T("(품절)");
	else if (m_nCurrentStock < m_nDangerThreshold)  // 하드코딩된 10 대신 변수 사용
		strStatus = _T("(위험)");
	else if (m_nCurrentStock < m_nWarningThreshold) // 하드코딩된 30 대신 변수 사용
		strStatus = _T("(주의)");
	else
		strStatus = _T("(정상)");

	strInfo.Format(_T("현재 재고: %d개 %s"), m_nCurrentStock, strStatus);
	SetDlgItemText(IDC_STATIC_CURRENT_STOCK, strInfo);

	SetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, m_nOrderQuantity, FALSE);

	UpdateExpectedStock();

	return TRUE;
}

void COrderDlg::UpdateExpectedStock()
{
	m_nOrderQuantity = GetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nullptr, FALSE);

	if (m_nOrderQuantity < 1)
	{
		m_nOrderQuantity = 1;
		SetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, m_nOrderQuantity, FALSE);
	}

	int nExpected = m_nCurrentStock + m_nOrderQuantity;

	CString strStatus;
	if (nExpected == 0)
		strStatus = _T("(품절)");
	else if (nExpected < m_nDangerThreshold)  // 하드코딩된 10 대신 변수 사용
		strStatus = _T("(위험)");
	else if (nExpected < m_nWarningThreshold) // 하드코딩된 30 대신 변수 사용
		strStatus = _T("(주의)");
	else
		strStatus = _T("(정상)");

	CString strInfo;
	strInfo.Format(_T("발주 후 예상: %d개 %s"), nExpected, strStatus);
	SetDlgItemText(IDC_STATIC_EXPECTED_STOCK, strInfo);
}

void COrderDlg::OnBnClickedButtonMinus10()
{
	int nQuantity = GetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nullptr, FALSE);
	nQuantity -= 10;
	if (nQuantity < 1) nQuantity = 1;
	SetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nQuantity, FALSE);
	UpdateExpectedStock();
}

void COrderDlg::OnBnClickedButtonMinus5()
{
	int nQuantity = GetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nullptr, FALSE);
	nQuantity -= 5;
	if (nQuantity < 1) nQuantity = 1;
	SetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nQuantity, FALSE);
	UpdateExpectedStock();
}

void COrderDlg::OnBnClickedButtonPlus5()
{
	int nQuantity = GetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nullptr, FALSE);
	nQuantity += 5;
	SetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nQuantity, FALSE);
	UpdateExpectedStock();
}

void COrderDlg::OnBnClickedButtonPlus10()
{
	int nQuantity = GetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nullptr, FALSE);
	nQuantity += 10;
	SetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nQuantity, FALSE);
	UpdateExpectedStock();
}

void COrderDlg::OnEnChangeEditOrderQuantity()
{
	UpdateExpectedStock();
}

void COrderDlg::OnBnClickedOk()
{
	m_nOrderQuantity = GetDlgItemInt(IDC_EDIT_ORDER_QUANTITY, nullptr, FALSE);

	if (m_nOrderQuantity < 1)
	{
		AfxMessageBox(_T("발주 수량은 1개 이상이어야 합니다."));
		return;
	}

	if (m_nOrderQuantity > 10000)
	{
		AfxMessageBox(_T("발주 수량은 10,000개 이하여야 합니다."));
		return;
	}

	CDialogEx::OnOK();
}
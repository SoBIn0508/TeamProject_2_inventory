// CEditStockDlg.cpp: 구현 파일
//

#include "pch.h"
#include "InventoryManager.h"
#include "CEditStockDlg.h"
#include "afxdialogex.h"


// CEditStockDlg 대화 상자

IMPLEMENT_DYNAMIC(CEditStockDlg, CDialogEx)

// ========================================
// 생성자
// ========================================
CEditStockDlg::CEditStockDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_EDIT_STOCK, pParent)
	, m_nOptionID(0)
	, m_nCurrentStock(0)
	, m_nNewStock(0)
{
}

// ========================================
// 소멸자
// ========================================
CEditStockDlg::~CEditStockDlg()
{
}

// ========================================
// DDX (Data Exchange)
// ========================================
void CEditStockDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_OPTION_CODE_EDIT, m_staticOptionCode);
	DDX_Control(pDX, IDC_STATIC_PRODUCT_NAME_EDIT, m_staticProductName);
	DDX_Control(pDX, IDC_STATIC_CURRENT_STOCK_EDIT, m_staticCurrentStock);
	DDX_Control(pDX, IDC_EDIT_NEW_STOCK, m_editNewStock);
}

// ========================================
// 메시지 맵
// ========================================
BEGIN_MESSAGE_MAP(CEditStockDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CEditStockDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// ========================================
// CEditStockDlg 메시지 처리기
// ========================================

// ========================================
// 대화상자 초기화
// ========================================
BOOL CEditStockDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ========================================
	// 1. 품번 표시
	// ========================================
	CString strText;
	strText.Format(_T("품번: %s"), m_strOptionCode);
	m_staticOptionCode.SetWindowText(strText);

	// ========================================
	// 2. 상품명 표시
	// ========================================
	strText.Format(_T("상품명: %s"), m_strProductName);
	m_staticProductName.SetWindowText(strText);

	// ========================================
	// 3. 현재 재고 표시
	// ========================================
	strText.Format(_T("현재 재고: %d개"), m_nCurrentStock);
	m_staticCurrentStock.SetWindowText(strText);

	// ========================================
	// 4. 새 재고 입력란에 현재 재고 미리 채우기
	// ========================================
	strText.Format(_T("%d"), m_nCurrentStock);
	m_editNewStock.SetWindowText(strText);

	// ========================================
	// 5. 입력란 포커스 및 전체 선택
	// ========================================
	m_editNewStock.SetFocus();
	m_editNewStock.SetSel(0, -1);  // 전체 선택

	return FALSE;  // 포커스를 직접 설정했으므로 FALSE 반환
}

// ========================================
// [저장] 버튼 클릭
// ========================================
void CEditStockDlg::OnBnClickedOk()
{
	// ========================================
	// 1. 입력값 가져오기
	// ========================================
	CString strNewStock;
	m_editNewStock.GetWindowText(strNewStock);

	// ========================================
	// 2. 빈 값 체크
	// ========================================
	if (strNewStock.IsEmpty())
	{
		AfxMessageBox(_T("재고 수량을 입력해주세요."));
		m_editNewStock.SetFocus();
		return;
	}

	// ========================================
	// 3. 숫자 변환
	// ========================================
	m_nNewStock = _ttoi(strNewStock);

	// ========================================
	// 4. 유효성 검사
	// ========================================
	if (m_nNewStock < 0)
	{
		AfxMessageBox(_T("재고는 0 이상이어야 합니다."));
		m_editNewStock.SetFocus();
		m_editNewStock.SetSel(0, -1);
		return;
	}

	// ========================================
	// 5. 변경 사항 확인
	// ========================================
	if (m_nNewStock == m_nCurrentStock)
	{
		int nResult = AfxMessageBox(
			_T("재고가 변경되지 않았습니다.\n그래도 저장하시겠습니까?"),
			MB_YESNO | MB_ICONQUESTION);

		if (nResult != IDYES)
		{
			return;
		}
	}

	// ========================================
	// 6. 확인 메시지
	// ========================================
	CString strConfirm;
	strConfirm.Format(
		_T("재고를 수정하시겠습니까?\n\n")
		_T("품번: %s\n")
		_T("현재 재고: %d개\n")
		_T("새 재고: %d개\n")
		_T("변경: %+d개"),
		m_strOptionCode,
		m_nCurrentStock,
		m_nNewStock,
		m_nNewStock - m_nCurrentStock);

	int nResult = AfxMessageBox(strConfirm, MB_YESNO | MB_ICONQUESTION);

	if (nResult != IDYES)
	{
		return;
	}

	// ========================================
	// 7. 대화상자 닫기 (IDOK 반환)
	// ========================================
	CDialogEx::OnOK();
}
// CAutoOrderNotifyDlg.cpp: 구현 파일
//

#include "pch.h"
#include "InventoryManager.h"
#include "afxdialogex.h"
#include "CAutoOrderNotifyDlg.h"


// CAutoOrderNotifyDlg 대화 상자

IMPLEMENT_DYNAMIC(CAutoOrderNotifyDlg, CDialogEx)

CAutoOrderNotifyDlg::CAutoOrderNotifyDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AUTO_ORDER_NOTIFY, pParent)
{

}

CAutoOrderNotifyDlg::~CAutoOrderNotifyDlg()
{
}

void CAutoOrderNotifyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_NOTIFY_SUMMARY, m_staticSummary);
	DDX_Control(pDX, IDC_LIST_AUTO_ORDER_ITEMS, m_listItems);
}


BEGIN_MESSAGE_MAP(CAutoOrderNotifyDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAutoOrderNotifyDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CAutoOrderNotifyDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_IGNORE, &CAutoOrderNotifyDlg::OnClickedButtonIgnore)
END_MESSAGE_MAP()

// ========================================
// 다이얼로그 초기화
// ========================================
BOOL CAutoOrderNotifyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitList();        // 리스트 컬럼 설정
	LoadItems();       // 데이터 로드
	UpdateSummary();   // 요약 정보 표시

	return TRUE;
}

// ========================================
// 리스트 컨트롤 초기화
// ========================================
void CAutoOrderNotifyDlg::InitList()
{
	// 리스트 스타일 설정
	DWORD dwStyle = m_listItems.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	m_listItems.SetExtendedStyle(dwStyle);

	// 컬럼 생성
	m_listItems.InsertColumn(0, _T("품번"), LVCFMT_LEFT, 120);
	m_listItems.InsertColumn(1, _T("상품명"), LVCFMT_LEFT, 150);
	m_listItems.InsertColumn(2, _T("현재 재고"), LVCFMT_RIGHT, 80);
	m_listItems.InsertColumn(3, _T("발주 수량"), LVCFMT_RIGHT, 80);
	m_listItems.InsertColumn(4, _T("예상 재고"), LVCFMT_RIGHT, 80);
}

// ========================================
// 품목 목록 로드
// ========================================
void CAutoOrderNotifyDlg::LoadItems()
{
    m_listItems.DeleteAllItems();

    for (size_t i = 0; i < m_vecOrderItems.size(); i++)
    {
        const AUTO_ORDER_ITEM& item = m_vecOrderItems[i];

        // 첫 번째 컬럼: 품번
        int nIndex = m_listItems.InsertItem(i, item.strOptionCode);

        // 두 번째 컬럼: 상품명
        m_listItems.SetItemText(nIndex, 1, item.strProductName);

        // 세 번째 컬럼: 현재 재고
        CString strStock;
        strStock.Format(_T("%d"), item.nCurrentStock);
        m_listItems.SetItemText(nIndex, 2, strStock);

        // 네 번째 컬럼: 발주 수량
        CString strOrder;
        strOrder.Format(_T("%d"), item.nOrderQuantity);
        m_listItems.SetItemText(nIndex, 3, strOrder);

        // 다섯 번째 컬럼: 예상 재고
        CString strExpected;
        strExpected.Format(_T("%d"), item.nExpectedStock);
        m_listItems.SetItemText(nIndex, 4, strExpected);

        // ItemData에 OptionID 저장 (나중에 발주할 때 사용)
        m_listItems.SetItemData(nIndex, (DWORD_PTR)item.nOptionID);
    }
}

// ========================================
// 요약 정보 업데이트
// ========================================
void CAutoOrderNotifyDlg::UpdateSummary()
{
	CString strSummary;
	strSummary.Format(_T("총 %d개 품목이 발주 기준 이하입니다."),
		(int)m_vecOrderItems.size());
	m_staticSummary.SetWindowText(strSummary);
}



// CAutoOrderNotifyDlg 메시지 처리기

// ========================================
// [모두 발주 확정] 버튼 클릭
// ========================================
void CAutoOrderNotifyDlg::OnBnClickedOk()
{
    CString strMsg;
    strMsg.Format(_T("총 %d개 품목을 발주하시겠습니까?"),
        (int)m_vecOrderItems.size());

    if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        // IDOK로 다이얼로그 종료 (발주 확정)
        CDialogEx::OnOK();
    }
}

// ========================================
// [나중에 확인] 버튼 클릭
// ========================================
void CAutoOrderNotifyDlg::OnBnClickedCancel()
{
    // IDCANCEL로 다이얼로그 종료 (나중에 확인)
    CDialogEx::OnCancel();
}

void CAutoOrderNotifyDlg::OnClickedButtonIgnore()
{
    if (AfxMessageBox(_T("이번 발주 알림을 무시하시겠습니까?"),
        MB_YESNO | MB_ICONWARNING) == IDYES)
    {
        // 사용자 정의 반환값으로 종료
        EndDialog(IDIGNORE);
    }
}

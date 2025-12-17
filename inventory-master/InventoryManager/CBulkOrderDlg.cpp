// CBulkOrderDlg.cpp: 구현 파일
//

#include "pch.h"
#include "InventoryManager.h"
#include "afxdialogex.h"
#include "CBulkOrderDlg.h"


// CBulkOrderDlg 대화 상자

IMPLEMENT_DYNAMIC(CBulkOrderDlg, CDialogEx)

CBulkOrderDlg::CBulkOrderDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_BULK_ORDER_DIALOG, pParent)
	, m_nSelectedItemCount(0)
	, m_nOrderQuantity(10) // 발주 수량 기본값을 10으로 설정
{

}

CBulkOrderDlg::~CBulkOrderDlg()
{
}

void CBulkOrderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// IDC_EDIT_BULK_QUANTITY 컨트롤과 m_nOrderQuantity 변수를 연결합니다.
	DDX_Text(pDX, IDC_EDIT_BULK_QUANTITY, m_nOrderQuantity);
}

// CBulkOrderDlg 메시지 처리기

BOOL CBulkOrderDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 선택된 품목 개수를 표시하는 텍스트를 설정합니다.
    CString strCount;
    strCount.Format(_T("%d개의 품목에 대해 일괄 발주합니다."), m_nSelectedItemCount);
    SetDlgItemText(IDC_STATIC_ITEM_COUNT, strCount);

    // 사용자가 바로 입력할 수 있도록 에디트 컨트롤에 포커스를 맞춥니다.
    GetDlgItem(IDC_EDIT_BULK_QUANTITY)->SetFocus();

    // 입력란의 텍스트를 전체 선택 상태로 만듭니다.
    ((CEdit*)GetDlgItem(IDC_EDIT_BULK_QUANTITY))->SetSel(0, -1);

    return FALSE;  // 포커스를 직접 설정했으므로 FALSE를 반환합니다.
}


void CBulkOrderDlg::OnBnClickedOk()
{
    UpdateData(TRUE); // 컨트롤에 입력된 값을 변수로 가져옵니다.

    if (m_nOrderQuantity <= 0)
    {
        AfxMessageBox(_T("발주 수량은 1 이상이어야 합니다."));
        return; // 함수 종료
    }

    // 사용자에게 최종 확인 메시지를 띄웁니다.
    CString strConfirm;
    strConfirm.Format(_T("총 %d개 품목에 대해 각각 %d개씩 발주하시겠습니까?"), m_nSelectedItemCount, m_nOrderQuantity);

    // '예'를 눌렀을 때만 대화상자를 닫습니다.
    if (AfxMessageBox(strConfirm, MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        CDialogEx::OnOK();
    }
}


BEGIN_MESSAGE_MAP(CBulkOrderDlg, CDialogEx)
END_MESSAGE_MAP()


// CBulkOrderDlg 메시지 처리기

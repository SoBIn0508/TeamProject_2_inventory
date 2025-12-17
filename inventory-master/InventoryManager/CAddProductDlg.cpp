// CAddProductDlg.cpp: 구현 파일
//

#include "pch.h"
#include "InventoryManager.h"
#include "afxdialogex.h"
#include "CAddProductDlg.h"
#include "DBManager.h"

// CAddProductDlg 대화 상자

IMPLEMENT_DYNAMIC(CAddProductDlg, CDialogEx)

CAddProductDlg::CAddProductDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_ADD_PRODUCT, pParent)
{

}           

CAddProductDlg::~CAddProductDlg()
{
}

void CAddProductDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_PRODUCTNAME, m_strProductName);
    DDX_Text(pDX, IDC_EDIT_BASE_STOCK, m_nStock);
    DDX_Control(pDX, IDC_COMBO_BRAND, m_comboBrand);
    DDX_Control(pDX, IDC_COMBO_COLOR, m_comboColor);
    DDX_Control(pDX, IDC_COMBO_SIZE, m_comboSize);
    DDX_Control(pDX, IDC_PRODUCT_NUM, m_editPreviewNum);
    DDX_Text(pDX, IDC_EDIT_PRICE, m_nPrice);
    DDX_Control(pDX, IDC_COMBO_WERE, m_comboWare);
    DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
    DDX_Text(pDX, IDC_EDIT_IMAGE_URL, m_strImageUrl);
}

BOOL CAddProductDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CDBManager* pDBManager = CDBManager::GetInstance();
    std::vector<CString> vecItems;

    // 브랜드 목록 채우기
    if (pDBManager->GetBrandList(vecItems)) {
        for (const auto& item : vecItems) m_comboBrand.AddString(item);
        m_comboBrand.SetCurSel(0);
    }

    // 색상 목록 채우기
    if (pDBManager->GetColorList(vecItems)) {
        for (const auto& item : vecItems) m_comboColor.AddString(item);
        m_comboColor.SetCurSel(0);
    }

    // 사이즈 목록 채우기
    if (pDBManager->GetSizeList(vecItems)) {
        for (const auto& item : vecItems) m_comboSize.AddString(item);
        m_comboSize.SetCurSel(0);
    }

    // ▼▼▼ '종류' 목록을 채우는 이 부분이 누락되었을 가능성이 높습니다! ▼▼▼
    // 종류 목록 채우기
    if (pDBManager->GetCategoryList(vecItems)) {
        for (const auto& item : vecItems) m_comboWare.AddString(item);
        m_comboWare.SetCurSel(0);
    }
    // ▲▲▲ 여기까지 ▲▲▲

    // 콤보박스 드롭다운 목록에 최소 10개의 항목이 보이도록 설정합니다.
    m_comboBrand.SetMinVisibleItems(10);
    m_comboColor.SetMinVisibleItems(10);
    m_comboSize.SetMinVisibleItems(10);
    m_comboWare.SetMinVisibleItems(10);
    //콤보박스////////////////

    UpdatePreviewCode();

    return TRUE;
}

void CAddProductDlg::OnOK()
{
    UpdateData(TRUE);

    if (m_strProductName.IsEmpty() || m_strDescription.IsEmpty() || m_strImageUrl.IsEmpty() || m_nPrice <= 0 || m_nStock < 0)
    {
        AfxMessageBox(_T("모든 항목을 올바르게 입력해주세요."));
        return;
    }

    CString strBrand, strColor, strSize, strCategory;
    int nIndex;

    // 모든 콤보박스에서 현재 선택된 텍스트를 정확하게 가져옵니다.
    nIndex = m_comboBrand.GetCurSel();
    if (nIndex == CB_ERR) { AfxMessageBox(_T("브랜드를 선택해주세요.")); return; }
    m_comboBrand.GetLBText(nIndex, strBrand);

    nIndex = m_comboColor.GetCurSel();
    if (nIndex == CB_ERR) { AfxMessageBox(_T("색상을 선택해주세요.")); return; }
    m_comboColor.GetLBText(nIndex, strColor);

    nIndex = m_comboSize.GetCurSel();
    if (nIndex == CB_ERR) { AfxMessageBox(_T("사이즈를 선택해주세요.")); return; }
    m_comboSize.GetLBText(nIndex, strSize);

    nIndex = m_comboWare.GetCurSel();
    if (nIndex == CB_ERR) { AfxMessageBox(_T("종류를 선택해주세요.")); return; }
    m_comboWare.GetLBText(nIndex, strCategory);

    // 사용자가 입력한 URL로 JSON 배열 문자열 생성
    CString strImagesJson;
    strImagesJson.Format(_T("[\"%s\"]"), m_strImageUrl);

    // 옵션 JSON 생성
    CString strOptionsJson;
    strOptionsJson.Format(
        _T("[{\"color\":\"%s\", \"size\":\"%s\", \"stock\":%d}]"),
        strColor, strSize, m_nStock);

    // DBManager의 AddProduct 함수 호출
    BOOL bResult = CDBManager::GetInstance()->AddProduct(
        strBrand,
        strCategory,
        m_strProductName,
        m_nPrice,
        m_strDescription,
        strImagesJson,
        strOptionsJson
    );

    if (bResult) {
        AfxMessageBox(_T("상품이 성공적으로 추가되었습니다."));
        CDialogEx::OnOK();
    }
    else {
        AfxMessageBox(_T("상품 추가 실패:\n") + CDBManager::GetInstance()->GetLastError());
    }
}

void CAddProductDlg::UpdatePreviewCode()
{
    CString strBrand, strColor, strSize, strCategory;
    int nIndex;

    // GetWindowText 대신 GetCurSel과 GetLBText를 사용하여 즉시 값을 가져옵니다.
    nIndex = m_comboBrand.GetCurSel();
    if (nIndex != CB_ERR) m_comboBrand.GetLBText(nIndex, strBrand);

    nIndex = m_comboColor.GetCurSel();
    if (nIndex != CB_ERR) m_comboColor.GetLBText(nIndex, strColor);

    nIndex = m_comboSize.GetCurSel();
    if (nIndex != CB_ERR) m_comboSize.GetLBText(nIndex, strSize);

    nIndex = m_comboWare.GetCurSel();
    if (nIndex != CB_ERR) m_comboWare.GetLBText(nIndex, strCategory);

    // 이름(Name)을 코드(Code)로 변환합니다.
    CDBManager* pDBManager = CDBManager::GetInstance();
    CString strBrandCode = pDBManager->GetBrandCodeFromName(strBrand);
    CString strCategoryCode = pDBManager->GetCategoryCodeFromName(strCategory);
    CString strColorCode = pDBManager->GetColorCodeFromName(strColor);
    CString strSizeCode = strSize;

    // 최종 품번 미리보기 문자열 생성
    CString strPreview;
    strPreview.Format(_T("%s-%s-???-%s-%s"),
        strBrandCode, strCategoryCode, strColorCode, strSizeCode);

    m_editPreviewNum.SetWindowText(strPreview);
}

BEGIN_MESSAGE_MAP(CAddProductDlg, CDialogEx)
    ON_CBN_SELCHANGE(IDC_COMBO_BRAND, &CAddProductDlg::OnCbnSelchangeComboBrand)
    ON_CBN_SELCHANGE(IDC_COMBO_COLOR, &CAddProductDlg::OnCbnSelchangeComboColor)
    ON_CBN_SELCHANGE(IDC_COMBO_SIZE, &CAddProductDlg::OnCbnSelchangeComboSize)
    ON_CBN_SELCHANGE(IDC_COMBO_WERE, &CAddProductDlg::OnCbnSelchangeComboWere)
END_MESSAGE_MAP()


// CAddProductDlg 메시지 처리기

void CAddProductDlg::OnCbnSelchangeComboBrand()
{
    UpdatePreviewCode();
}

void CAddProductDlg::OnCbnSelchangeComboColor()
{
    UpdatePreviewCode();
}

void CAddProductDlg::OnCbnSelchangeComboSize()
{
    UpdatePreviewCode();
}

void CAddProductDlg::OnCbnSelchangeComboWere()
{
    UpdatePreviewCode();
}

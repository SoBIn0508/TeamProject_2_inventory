// === CStatsDlg.cpp (FULL) ===
#include "pch.h"
#include "CStatsDlg.h"
#include "DBManager.h"
#include "resource.h"
#include <afxcmn.h> 

int CalcColWidthByContent(CListCtrl& list, int col, int pad /*=2*/)
{
    CHeaderCtrl* hdr = list.GetHeaderCtrl();
    CClientDC dc(&list);
    CFont* pOld = dc.SelectObject(list.GetFont());

    int w = 0;
    // (필요하면 헤더 너비 반영)
    // if (hdr) { ... }

    const int n = list.GetItemCount();
    for (int i = 0; i < n; ++i) {
        CString t = list.GetItemText(i, col);
        CSize s = dc.GetTextExtent(t);
        if (s.cx > w) w = s.cx;
    }

    dc.SelectObject(pOld);
    return w + pad;
}

IMPLEMENT_DYNAMIC(CStatsDlg, CDialogEx)

CStatsDlg::CStatsDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_STATS_DIALOG, pParent)
{
}

CStatsDlg::~CStatsDlg() {}

void CStatsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_STATS_ORDERS, m_listOrders);
    DDX_Control(pDX, IDC_LIST_STATS_BRAND, m_listBrand);
    DDX_Control(pDX, IDC_LIST_STATS_DAILY, m_listDaily);
    DDX_Control(pDX, IDC_DATETIMEPICKER_START, m_dtpStart);
    DDX_Control(pDX, IDC_DATETIMEPICKER_END, m_dtpEnd);
}

BEGIN_MESSAGE_MAP(CStatsDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BUTTON_SEARCH_PERIOD, &CStatsDlg::OnBnClickedButtonSearchPeriod)
END_MESSAGE_MAP()

BOOL CStatsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    InitLists();
    LoadStats();

    COleDateTime dtEnd = COleDateTime::GetCurrentTime();  // 오늘
    COleDateTime dtStart = dtEnd - COleDateTimeSpan(30, 0, 0, 0);  // 30일 전

    m_dtpStart.SetTime(dtStart);
    m_dtpEnd.SetTime(dtEnd);

    return TRUE;
}

// [EDIT] CStatsDlg::InitLists() 전체 교체
void CStatsDlg::InitLists()
{
    // [ADD] Report 강제 + OwnerData 끄기
    m_listOrders.ModifyStyle(0, LVS_REPORT);
    m_listBrand.ModifyStyle(0, LVS_REPORT);
    m_listDaily.ModifyStyle(0, LVS_REPORT);
    m_listOrders.ModifyStyle(LVS_OWNERDATA, 0);
    m_listBrand.ModifyStyle(LVS_OWNERDATA, 0);
    m_listDaily.ModifyStyle(LVS_OWNERDATA, 0);

    DWORD ex = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
    m_listOrders.SetExtendedStyle(ex);
    m_listBrand.SetExtendedStyle(ex);
    m_listDaily.SetExtendedStyle(ex);

    // 주문기록 컬럼
    m_listOrders.DeleteAllItems();
    if (auto* h = m_listOrders.GetHeaderCtrl())
        for (int i = h->GetItemCount() - 1; i >= 0; --i) m_listOrders.DeleteColumn(i);
    m_listOrders.InsertColumn(0, _T("주문번호"), LVCFMT_LEFT, 80);
    m_listOrders.InsertColumn(1, _T("일시"), LVCFMT_LEFT, 120);
    m_listOrders.InsertColumn(2, _T("브랜드"), LVCFMT_LEFT, 100);
    m_listOrders.InsertColumn(3, _T("상품명"), LVCFMT_LEFT, 180);
    m_listOrders.InsertColumn(4, _T("옵션코드"), LVCFMT_LEFT, 110);
    m_listOrders.InsertColumn(5, _T("색상"), LVCFMT_LEFT, 60);
    m_listOrders.InsertColumn(6, _T("사이즈"), LVCFMT_LEFT, 60);
    m_listOrders.InsertColumn(7, _T("수량"), LVCFMT_RIGHT, 60);
    m_listOrders.InsertColumn(8, _T("단가"), LVCFMT_RIGHT, 80);
    m_listOrders.InsertColumn(9, _T("금액"), LVCFMT_LEFT, 90);

    //// 브랜드별
    //m_listBrand.DeleteAllItems();
    //if (auto* h2 = m_listBrand.GetHeaderCtrl())
    //    for (int i = h2->GetItemCount() - 1; i >= 0; --i) m_listBrand.DeleteColumn(i);
    //m_listBrand.InsertColumn(0, _T("브랜드"), LVCFMT_LEFT, 150);
    //m_listBrand.InsertColumn(1, _T("판매수량"), LVCFMT_RIGHT, 80);
    //m_listBrand.InsertColumn(2, _T("매출"), LVCFMT_LEFT, 100);

    //// [ADD] 날짜별 총매출
    //m_listDaily.DeleteAllItems();
    //if (auto* h3 = m_listDaily.GetHeaderCtrl())
    //    for (int i = h3->GetItemCount() - 1; i >= 0; --i) m_listDaily.DeleteColumn(i);
    //m_listDaily.InsertColumn(0, _T("날짜"), LVCFMT_LEFT, 100);
    //m_listDaily.InsertColumn(1, _T("매출"), LVCFMT_LEFT, 110);

    // 브랜드별
    m_listBrand.DeleteAllItems();
    if (auto* h2 = m_listBrand.GetHeaderCtrl())
        for (int i = h2->GetItemCount() - 1; i >= 0; --i) m_listBrand.DeleteColumn(i);
    m_listBrand.InsertColumn(0, _T("브랜드"), LVCFMT_LEFT, 60);
    m_listBrand.InsertColumn(1, _T("주문수"), LVCFMT_RIGHT, 50);      // ✨ 추가
    m_listBrand.InsertColumn(2, _T("판매량"), LVCFMT_RIGHT, 50);      // ✨ 수정
    m_listBrand.InsertColumn(3, _T("매출액"), LVCFMT_RIGHT, 100);     // ✨ 추가

    // [ADD] 날짜별 총매출
    m_listDaily.DeleteAllItems();
    if (auto* h3 = m_listDaily.GetHeaderCtrl())
        for (int i = h3->GetItemCount() - 1; i >= 0; --i) m_listDaily.DeleteColumn(i);
    m_listDaily.InsertColumn(0, _T("날짜"), LVCFMT_LEFT, 70);
    m_listDaily.InsertColumn(1, _T("주문수"), LVCFMT_RIGHT, 50);      // ✨ 추가
    m_listDaily.InsertColumn(2, _T("판매량"), LVCFMT_RIGHT, 50);      // ✨ 추가
    m_listDaily.InsertColumn(3, _T("매출액"), LVCFMT_RIGHT, 90);      // ✨ 수정
}

void CStatsDlg::LoadStats()
{
    if (!m_bDBConnected || !m_pDB) {
        m_listOrders.DeleteAllItems();
        m_listBrand.DeleteAllItems();
        m_listDaily.DeleteAllItems();
        return;
    }

    std::vector<std::vector<CString>> rows;

    // 주문기록 (최신 200)
    {
        CString sql =
            _T("SELECT o.order_id, o.order_date, b.brand_name, p.product_name, po.option_code, ")
            _T("       c.color_name, s.size_name, od.quantity, od.price_per_item, ")
            _T("       (od.quantity * od.price_per_item) AS line_total ")
            _T("FROM order_details od ")
            _T("JOIN orders o           ON o.order_id = od.order_id ")
            _T("JOIN product_options po ON po.option_id = od.option_id ")
            _T("JOIN products p         ON p.product_id = po.product_id ")
            _T("JOIN brands b           ON b.brand_id = p.brand_id ")
            _T("JOIN colors c           ON c.color_id = po.color_id ")
            _T("JOIN sizes  s           ON s.size_id  = po.size_id ")
            _T("ORDER BY o.order_date DESC, o.order_id DESC LIMIT 200");

        rows.clear();
        if (m_pDB->SelectToRows(sql, rows)) {
            m_listOrders.SetRedraw(FALSE);
            m_listOrders.DeleteAllItems();
            for (size_t i = 0; i < rows.size(); ++i) {
                const auto& r = rows[i];
                int idx = m_listOrders.InsertItem((int)i, r.size() > 0 ? r[0] : _T(""));
                for (int col = 1; col <= 9; ++col)
                    m_listOrders.SetItemText(idx, col, (r.size() > col ? r[col] : _T("")));
            }
            m_listOrders.SetRedraw(TRUE);
            AutoSizeColumns(m_listOrders); 
            //AutoSizeColumns(m_listBrand);
            //AutoSizeColumns(m_listDaily);
        }
    }

    // 브랜드별 판매 TOP 20
    {
        CString sql =
            _T("SELECT b.brand_name, ")
            _T("       COUNT(DISTINCT o.order_id) AS order_count, ")  // ✨ 추가
            _T("       SUM(od.quantity) AS qty_sold, ")
            _T("       SUM(od.quantity * od.price_per_item) AS revenue ")
            _T("FROM order_details od ")
            _T("JOIN orders o           ON o.order_id = od.order_id ")
            _T("JOIN product_options po ON po.option_id = od.option_id ")
            _T("JOIN products p         ON p.product_id = po.product_id ")
            _T("JOIN brands b           ON b.brand_id = p.brand_id ")
            _T("GROUP BY b.brand_id, b.brand_name ")
            _T("ORDER BY qty_sold DESC, revenue DESC LIMIT 20");

        rows.clear();
        if (m_pDB->SelectToRows(sql, rows)) {
            m_listBrand.SetRedraw(FALSE);
            m_listBrand.DeleteAllItems();
            for (size_t i = 0; i < rows.size(); ++i) {
                const auto& r = rows[i];
                int idx = m_listBrand.InsertItem((int)i, r.size() > 0 ? r[0] : _T(""));
                m_listBrand.SetItemText(idx, 1, r.size() > 1 ? r[1] : _T(""));  // ✨ 주문수
                m_listBrand.SetItemText(idx, 2, r.size() > 2 ? r[2] : _T(""));  // 판매량
                m_listBrand.SetItemText(idx, 3, r.size() > 3 ? r[3] : _T(""));  // ✨ 매출액
            }
            m_listBrand.SetRedraw(TRUE);
            // AutoSizeColumns(m_listBrand);  // ✨ 삭제
        }
    }

    // [ADD] 날짜별 총매출 (최근 60일)
    {
        CString sql =
            _T("SELECT DATE(o.order_date) AS ymd, ")
            _T("       COUNT(DISTINCT o.order_id) AS order_count, ")  // ✨ 추가
            _T("       SUM(od.quantity) AS total_qty, ")              // ✨ 추가
            _T("       COALESCE(SUM(od.quantity * od.price_per_item), 0) AS revenue ")
            _T("FROM orders o ")
            _T("JOIN order_details od ON od.order_id = o.order_id ")
            _T("GROUP BY DATE(o.order_date) ")
            _T("ORDER BY ymd DESC LIMIT 60");

        rows.clear();
        if (m_pDB->SelectToRows(sql, rows)) {
            m_listDaily.SetRedraw(FALSE);
            m_listDaily.DeleteAllItems();
            for (size_t i = 0; i < rows.size(); ++i) {
                const auto& r = rows[i];
                int idx = m_listDaily.InsertItem((int)i, r.size() > 0 ? r[0] : _T(""));
                m_listDaily.SetItemText(idx, 1, r.size() > 1 ? r[1] : _T(""));  // ✨ 주문수
                m_listDaily.SetItemText(idx, 2, r.size() > 2 ? r[2] : _T(""));  // ✨ 판매량
                m_listDaily.SetItemText(idx, 3, r.size() > 3 ? r[3] : _T(""));  // 매출액
            }
            m_listDaily.SetRedraw(TRUE);
            // AutoSizeColumns(m_listDaily);  // ✨ 삭제
        }
    }
}

// 모달리스 종료 처리(선택)
void CStatsDlg::OnClose() { DestroyWindow(); }
void CStatsDlg::OnCancel() { DestroyWindow(); }
BOOL CStatsDlg::DestroyWindow()
{
    BOOL b = CDialogEx::DestroyWindow();
    delete this; // 자기 삭제(간단 패턴)
    return b;
}


void CStatsDlg::AutoSizeColumns(CListCtrl& list)
{
    CHeaderCtrl* hdr = list.GetHeaderCtrl();
    if (!hdr) return;
    const int cols = hdr->GetItemCount();
    for (int c = 0; c < cols; ++c) {
        list.SetColumnWidth(c, LVSCW_AUTOSIZE);
        int wc = list.GetColumnWidth(c);
        list.SetColumnWidth(c, LVSCW_AUTOSIZE_USEHEADER);
        int wh = list.GetColumnWidth(c);
        list.SetColumnWidth(c, max(wc, wh) + 6);
    }
}

// [ADD]
void CStatsDlg::Reload()
{
    InitLists();
    LoadStats();
}

void CStatsDlg::OnBnClickedButtonSearchPeriod()
{
    // 1. DateTimePicker에서 날짜 가져오기
    COleDateTime dtStart, dtEnd;
    m_dtpStart.GetTime(dtStart);
    m_dtpEnd.GetTime(dtEnd);

    // 2. 날짜 유효성 검사
    if (dtStart > dtEnd)
    {
        AfxMessageBox(_T("시작일이 종료일보다 늦을 수 없습니다."), MB_ICONWARNING);
        return;
    }

    // 3. 날짜를 SQL 형식 문자열로 변환
    CString strStartDate = dtStart.Format(_T("%Y-%m-%d"));
    CString strEndDate = dtEnd.Format(_T("%Y-%m-%d"));

    // 4. 기간별 통계 데이터 로드
    LoadStatsByPeriod(strStartDate, strEndDate);
}

void CStatsDlg::LoadStatsByPeriod(const CString& strStartDate, const CString& strEndDate)
{
    // DB 연결 확인
    if (!m_bDBConnected || !m_pDB)
    {
        AfxMessageBox(_T("데이터베이스에 연결되지 않았습니다."), MB_ICONERROR);
        return;
    }

    // ===== 1. 브랜드별 통계 로드 =====
    LoadBrandStatsByPeriod(strStartDate, strEndDate);

    // ===== 2. 일별 통계 로드 =====
    LoadDailyStatsByPeriod(strStartDate, strEndDate);
}

void CStatsDlg::LoadBrandStatsByPeriod(const CString& strStartDate, const CString& strEndDate)
{
    m_listBrand.DeleteAllItems();

    // ✅ SQL 쿼리 수정 (orders 테이블 JOIN 추가)
    CString strQuery;
    strQuery.Format(
        _T("SELECT b.brand_name, ")
        _T("COUNT(DISTINCT o.order_id) as order_count, ")
        _T("SUM(od.quantity) as total_quantity, ")
        _T("SUM(od.quantity * od.price_per_item) as total_sales ")
        _T("FROM order_details od ")
        _T("JOIN orders o ON od.order_id = o.order_id ")  // ✨ orders 테이블 조인
        _T("JOIN product_options po ON od.option_id = po.option_id ")
        _T("JOIN products p ON po.product_id = p.product_id ")
        _T("JOIN brands b ON p.brand_id = b.brand_id ")
        _T("WHERE DATE(o.order_date) BETWEEN '%s' AND '%s' ")  // ✨ order_date 사용
        _T("GROUP BY b.brand_name ")
        _T("ORDER BY total_sales DESC"),
        strStartDate, strEndDate
    );

    // DB 실행
    std::vector<std::vector<CString>> rows;
    if (!m_pDB->SelectToRows(strQuery, rows))
    {
        AfxMessageBox(_T("브랜드별 통계 조회 실패"), MB_ICONERROR);
        return;
    }

    // 리스트에 데이터 추가
    for (size_t i = 0; i < rows.size(); i++)
    {
        if (rows[i].size() >= 4)
        {
            int nIndex = m_listBrand.InsertItem((int)i, rows[i][0]); // 브랜드명
            m_listBrand.SetItemText(nIndex, 1, rows[i][1]);          // 주문수
            m_listBrand.SetItemText(nIndex, 2, rows[i][2]);          // 판매량
            m_listBrand.SetItemText(nIndex, 3, rows[i][3]);          // 매출액
        }
    }

}

void CStatsDlg::LoadDailyStatsByPeriod(const CString& strStartDate, const CString& strEndDate)
{
    // 리스트 초기화
    m_listDaily.DeleteAllItems();

    // SQL 쿼리 작성
    CString strQuery;
    strQuery.Format(
        _T("SELECT DATE(o.order_date) as order_date, ")
        _T("COUNT(DISTINCT o.order_id) as order_count, ")
        _T("SUM(od.quantity) as total_quantity, ")
        _T("SUM(od.quantity * od.price_per_item) as daily_sales ")
        _T("FROM orders o ")
        _T("JOIN order_details od ON o.order_id = od.order_id ")
        _T("WHERE DATE(o.order_date) BETWEEN '%s' AND '%s' ")
        _T("GROUP BY DATE(o.order_date) ")
        _T("ORDER BY order_date DESC"),
        strStartDate, strEndDate
    );

    // DB 실행
    std::vector<std::vector<CString>> rows;
    if (!m_pDB->SelectToRows(strQuery, rows))
    {
        AfxMessageBox(_T("일별 통계 조회 실패"), MB_ICONERROR);
        return;
    }

    // 리스트에 데이터 추가
    for (size_t i = 0; i < rows.size(); i++)
    {
        if (rows[i].size() >= 4)
        {
            int nIndex = m_listDaily.InsertItem((int)i, rows[i][0]); // 날짜
            m_listDaily.SetItemText(nIndex, 1, rows[i][1]);          // 주문수
            m_listDaily.SetItemText(nIndex, 2, rows[i][2]);          // 판매량
            m_listDaily.SetItemText(nIndex, 3, rows[i][3]);          // 매출액
        }
    }

}



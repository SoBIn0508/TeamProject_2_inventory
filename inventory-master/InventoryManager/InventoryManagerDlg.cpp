// InventoryManagerDlg.cpp: 구현 파일
// 이 파일은 재고 관리 프로그램의 메인 대화 상자(UI)에 대한 모든 로직을 담고 있습니다.

#include "pch.h" // 미리 컴파일된 헤더 (컴파일 속도 향상)
#include "framework.h" // MFC 기본 프레임워크
#include "InventoryManager.h" // 메인 애플리케이션 클래스
#include "InventoryManagerDlg.h" // 현재 클래스의 헤더 파일
#include "afxdialogex.h" // 대화 상자 확장을 위한 클래스
#include "CAddProductDlg.h" // '상품 추가' 대화 상자
#include "COrderDlg.h" // '발주' 대화 상자
#include "CStatsDlg.h" // '통계' 대화 상자
#include "CSettingsDlg.h" // '설정' 대화 상자
#include <algorithm> // 정렬(std::sort) 기능을 사용하기 위해 포함
#include <set> // 중복된 상품 삭제를 방지하기 위해 사용
#include "CBulkOrderDlg.h" // '대량 발주' 대화 상자
#include <fstream> // 파일 입출력 스트림

#ifdef _DEBUG
#define new DEBUG_NEW // 디버그 모드에서 메모리 누수 탐지를 위해 사용
#endif

// === [헬퍼 함수] ==================================
// CString 객체를 소문자로 변환하여 반환하는 간단한 도우미 함수입니다.
// 검색 기능에서 대소문자 구분 없이 비교할 때 유용하게 사용됩니다.
static CString ToLower(const CString& s) {
	CString t = s;
	t.MakeLower();
	return t;
}
// =========================================================


// CInventoryManagerDlg 대화 상자 클래스 정의 시작

/**
 * @brief CInventoryManagerDlg 클래스의 생성자입니다.
 * @details 대화 상자가 생성될 때 가장 먼저 호출되며, 멤버 변수들을 초기값으로 설정합니다.
 * @param pParent 부모 윈도우를 가리키는 포인터입니다. 기본값은 nullptr입니다.
 */
CInventoryManagerDlg::CInventoryManagerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_INVENTORYMANAGER_DIALOG, pParent)
	, m_nCurrentTab(0) // 현재 선택된 탭 인덱스 (0: 재고현황)
	, m_nDangerThreshold(10)    // '위험' 재고 기준 수량 (기본값 10개 미만)
	, m_nWarningThreshold(30)   // '주의' 재고 기준 수량 (기본값 30개 미만)
	, m_nTimerID(0)             // 타이머 ID (0은 비활성 상태)
	, m_nAutoOrderTimerID(0)	// 자동발주 타이머 ID (0은 비활성 상태)
	, m_tSnoozeEndTime(0)		// 스누즈 종료 시간 (기본값 0)
	, m_nRefreshInterval(30)    // 자동 새로고침 간격 (기본 30초)
	, m_bAutoRefresh(TRUE)      // 자동 새로고침 기능 활성화 여부
	, m_pDBManager(nullptr)     // 데이터베이스 관리자 포인터
	, m_bDBConnected(FALSE)     // 데이터베이스 연결 상태
	, m_pSettingsDlg(nullptr)   // 설정 대화 상자 포인터
	, m_nSortColumn(7)          // 기본 정렬 컬럼 인덱스 (7번 '재고' 컬럼)
	, m_bSortAscending(true)    // 기본 정렬 방식 (true: 오름차순, false: 내림차순)
	, m_bAutoOrderEnabled(FALSE)
	, m_nAutoOrderThreshold(50)
	, m_nAutoOrderQuantity(100)
	, m_nAutoOrderInterval(30)
{
	// 프로그램 아이콘을 로드합니다.
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

/**
 * @brief CInventoryManagerDlg 클래스의 소멸자입니다.
 * @details 대화 상자가 파괴될 때 호출되며, 할당된 리소스들을 해제합니다.
 * (예: 타이머 종료, 데이터베이스 연결 해제, 동적 할당된 메모리 해제)
 */
CInventoryManagerDlg::~CInventoryManagerDlg()
{
	DisconnectDatabase(); // 데이터베이스 연결 해제
	CDBManager::DestroyInstance(); // DB 관리자 싱글톤 인스턴스 파괴
	m_pDBManager = nullptr;

	// 설정 대화 상자 객체가 동적으로 할당되었으므로 메모리를 해제합니다.
	if (m_pSettingsDlg)
	{
		delete m_pSettingsDlg;
		m_pSettingsDlg = nullptr;
	}
}

/**
 * @brief DoDataExchange 함수는 대화 상자의 컨트롤과 멤버 변수 간의 데이터 교환을 담당합니다.
 * @details DDX (Dialog Data Exchange) 매커니즘을 통해 UI 컨트롤과 변수를 연결합니다.
 * @param pDX 데이터 교환을 위한 CDataExchange 객체 포인터입니다.
 */
void CInventoryManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// IDC_TAB_MAIN 이라는 ID를 가진 탭 컨트롤을 m_tabMain 변수와 연결합니다.
	DDX_Control(pDX, IDC_TAB_MAIN, m_tabMain);
	DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
	DDX_Control(pDX, IDC_LIST_INVENTORY, m_listInventory);
	DDX_Control(pDX, IDC_COMBO_FILTER_BRAND, m_comboFilterBrand);
	DDX_Control(pDX, IDC_COMBO_FILTER_CATEGORY, m_comboFilterCategory);
}

// BEGIN_MESSAGE_MAP ~ END_MESSAGE_MAP 구간은 이벤트와 핸들러 함수를 연결하는 '메시지 맵'입니다.
// 예를 들어, 특정 버튼이 클릭되면 어떤 함수를 호출할지 정의합니다.
BEGIN_MESSAGE_MAP(CInventoryManagerDlg, CDialogEx)
	ON_WM_PAINT() // 윈도우를 다시 그려야 할 때 (예: 화면에 처음 나타날 때) OnPaint 함수를 호출
	ON_WM_TIMER() // 타이머 이벤트가 발생할 때 OnTimer 함수를 호출
	ON_WM_QUERYDRAGICON() // 최소화된 창을 드래그할 때 아이콘을 얻기 위해 OnQueryDragIcon 함수를 호출
	ON_WM_CLOSE() // 창의 닫기 버튼을 누를 때 OnClose 함수를 호출
	ON_WM_DESTROY()
	// IDC_BUTTON_CLEAR_LOG ID를 가진 버튼이 클릭되면 OnBnClickedButtonClearLog 함수를 호출
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_LOG, &CInventoryManagerDlg::OnBnClickedButtonClearLog)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, &CInventoryManagerDlg::OnBnClickedButtonRefresh)
	// IDC_TAB_MAIN ID를 가진 탭 컨트롤의 선택이 변경되면 OnSelchangeTabMain 함수를 호출
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &CInventoryManagerDlg::OnSelchangeTabMain)
	ON_BN_CLICKED(IDC_BUTTON_ORDER, &CInventoryManagerDlg::OnBnClickedButtonOrder) // '발주' 버튼
	// IDC_LIST_INVENTORY ID를 가진 리스트 컨트롤에서 항목을 더블 클릭하면 OnDblclkListInventory 함수를 호출
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_INVENTORY, &CInventoryManagerDlg::OnDblclkListInventory)
	ON_BN_CLICKED(IDC_BUTTON2, &CInventoryManagerDlg::OnBnClickedButton2) // '삭제' 버튼
	ON_BN_CLICKED(IDC_BUTTON3, &CInventoryManagerDlg::OnBnClickedButton3) // '상품 추가' 버튼
	ON_BN_CLICKED(IDC_BTN_SEARCH, &CInventoryManagerDlg::OnBnClickedBtnSearch) // '검색' 버튼
	// IDC_COMBO_FILTER_BRAND ID를 가진 콤보 박스의 선택이 변경되면 OnSelchangeComboFilter 함수를 호출
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER_BRAND, &CInventoryManagerDlg::OnSelchangeComboFilter)
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER_CATEGORY, &CInventoryManagerDlg::OnSelchangeComboFilter)
	// IDC_LIST_INVENTORY ID를 가진 리스트 컨트롤의 컬럼 헤더를 클릭하면 OnColumnclickListInventory 함수를 호출 (정렬 기능)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_INVENTORY, &CInventoryManagerDlg::OnColumnclickListInventory)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT_INV, &CInventoryManagerDlg::OnBnClickedButtonExportInv)
END_MESSAGE_MAP()


// CInventoryManagerDlg 메시지 처리기 (핸들러 함수) 구현부

/**
 * @brief 대화 상자가 처음 생성될 때 호출되는 초기화 함수입니다.
 * @details 이 곳에서 UI 컨트롤의 초기 상태 설정, 데이터베이스 연결, 초기 데이터 로드 등
 * 프로그램 시작에 필요한 대부분의 작업을 수행합니다.
 * @return BOOL 성공 시 TRUE, 실패 시 FALSE를 반환합니다.
 */
BOOL CInventoryManagerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 창의 제목 표시줄과 작업 표시줄에 아이콘을 설정합니다.
	SetIcon(m_hIcon, TRUE);		// 큰 아이콘
	SetIcon(m_hIcon, FALSE);	// 작은 아이콘

	UpdateData(FALSE);

	// 코드에서 UI 컨트롤을 직접 제어하기 위해 변수와 연결합니다.
	m_editSearch.SubclassDlgItem(IDC_EDIT_SEARCH, this);
	m_btnSearch.SubclassDlgItem(IDC_BTN_SEARCH, this);

	// 탭 컨트롤과 재고 목록 리스트 컨트롤을 초기화합니다.
	InitTabs();
	InitInventoryList();

	// 다중 선택을 허용하기 위해 리스트 컨트롤의 기본 스타일(LVS_SINGLESEL)을 제거합니다.
	DWORD dwStyle = GetWindowLong(m_listInventory.m_hWnd, GWL_STYLE);
	SetWindowLong(m_listInventory.m_hWnd, GWL_STYLE, dwStyle & ~LVS_SINGLESEL);

	// 프로그램 시작을 알리는 로그를 남깁니다.
	AddLog(_T("✅ 프로그램 시작"));
	AddLog(_T("💡 The Most 재고 관리 시스템"));

	// 사용 중인 MySQL 클라이언트 라이브러리 버전을 로그에 표시합니다.
	CString strMySQLVersion;
	strMySQLVersion.Format(_T("📚 MySQL Client 버전: %S"), mysql_get_client_info());
	AddLog(strMySQLVersion);

	// 데이터베이스에 연결을 시도합니다.
	AddLog(_T("🔌 데이터베이스 연결 시도 중..."));
	LoadDbConfig();
	LoadAutoOrderConfig();
	ConnectDatabase();

	// 데이터베이스 연결에 성공한 경우에만 데이터 관련 작업을 수행합니다.
	if (m_bDBConnected)
	{
		m_comboFilterBrand.SetRedraw(FALSE); // 콤보 박스 항목 추가 중 화면 갱신을 잠시 멈춥니다 (성능 향상).
		m_comboFilterCategory.SetRedraw(FALSE);

		// 필터 콤보 박스에 기본값 "전체" 항목을 추가하고 선택합니다.
		m_comboFilterBrand.InsertString(0, _T("전체 브랜드"));
		m_comboFilterBrand.SetCurSel(0);
		m_comboFilterCategory.InsertString(0, _T("전체 카테고리"));
		m_comboFilterCategory.SetCurSel(0);

		// DB에서 브랜드 목록을 가져와 콤보 박스에 채웁니다.
		std::vector<CString> vecBrands;
		if (m_pDBManager->GetBrandList(vecBrands))
		{
			for (const auto& brand : vecBrands)
			{
				m_comboFilterBrand.AddString(brand);
			}
		}

		// DB에서 카테고리 목록을 가져와 콤보 박스에 채웁니다.
		std::vector<CString> vecCategories;
		if (m_pDBManager->GetCategoryList(vecCategories))
		{
			for (const auto& category : vecCategories)
			{
				m_comboFilterCategory.AddString(category);
			}
		}

		m_comboFilterBrand.SetRedraw(TRUE); // 콤보 박스 화면 갱신을 다시 활성화합니다.
		m_comboFilterCategory.SetRedraw(TRUE);

		// DB에서 재고 데이터를 로드하고, 리스트에 표시한 후, 검색/정렬을 위해 캐시에 저장합니다.
		LoadInventoryData();
		UpdateInventoryList();
		SnapshotDisplayToCache();
	}
	else
	{
		AddLog(_T("❌ DB 연결 실패. 오프라인 모드로 동작합니다."));
	}

	// '통계' 탭에 표시될 CStatsDlg 대화 상자를 생성하고 초기화합니다.
	if (!m_pStatsDlg) {
		m_pStatsDlg = new CStatsDlg();
		m_pStatsDlg->Create(IDD_STATS_DIALOG, this); // 메인 대화 상자의 자식 윈도우로 생성
		m_pStatsDlg->InitDB(m_pDBManager, m_bDBConnected); // DB 관리자 포인터 전달

		// 메인 다이얼로그의 특정 위치(IDC_PLACE_STATS)에 통계 다이얼로그를 배치합니다.
		CRect rc;
		GetDlgItem(IDC_PLACE_STATS)->GetWindowRect(&rc);
		ScreenToClient(&rc);
		m_pStatsDlg->SetWindowPos(nullptr, rc.left, rc.top, rc.Width(), rc.Height(),
			SWP_NOZORDER | SWP_NOACTIVATE);

		m_pStatsDlg->ShowWindow(SW_HIDE);  // 처음에는 숨겨둡니다.
	}
	// 통계 다이얼로그가 위치할 영역(Static 컨트롤)을 숨깁니다.
	if (CWnd* pHost = GetDlgItem(IDC_PLACE_STATS)) {
		pHost->ShowWindow(SW_HIDE);
		pHost->EnableWindow(FALSE);
	}

	// '설정' 탭에 표시될 CSettingsDlg 대화 상자를 생성하고 초기화합니다.
	if (!m_pSettingsDlg) {
		m_pSettingsDlg = new CSettingsDlg();
		m_pSettingsDlg->Create(IDD_SETTINGS_DIALOG, this);
		m_pSettingsDlg->SetParentDlg(this); // 설정 변경 사항을 메인 다이얼로그에 전달하기 위해 포인터 설정

		// 메인 다이얼로그의 특정 위치(IDC_PLACE_STATS2)에 설정 다이얼로그를 배치합니다.
		CRect rc;
		GetDlgItem(IDC_PLACE_STATS2)->GetWindowRect(&rc);
		ScreenToClient(&rc);
		m_pSettingsDlg->SetWindowPos(nullptr, rc.left, rc.top, rc.Width(), rc.Height(),
			SWP_NOZORDER | SWP_NOACTIVATE);

		m_pSettingsDlg->ShowWindow(SW_HIDE);  // 처음에는 숨겨둡니다.
	}
	// 설정 다이얼로그가 위치할 영역(Static 컨트롤)을 숨깁니다.
	if (CWnd* pHost = GetDlgItem(IDC_PLACE_STATS2)) {
		pHost->ShowWindow(SW_HIDE);
		pHost->EnableWindow(FALSE);
	}

	// 현재 선택된 탭(기본값: 재고현황)에 맞는 UI만 표시합니다.
	ShowTabPage(m_tabMain.GetCurSel());

	// 자동발주 타이머 시작
	if (m_bAutoOrderEnabled)
	{
		StartAutoOrderTimer();
	}

	return TRUE;
}

/**
 * @brief WM_PAINT 메시지 핸들러. 창을 다시 그려야 할 때 호출됩니다.
 * @details 창이 최소화되었을 때 아이콘을 그리고, 그 외에는 기본 그리기 동작을 수행합니다.
 */
void CInventoryManagerDlg::OnPaint()
{
	if (IsIconic()) // 창이 최소화 상태인지 확인
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect; GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

/**
 * @brief WM_QUERYDRAGICON 메시지 핸들러. 최소화된 창을 드래그할 때 표시할 커서(아이콘)를 반환합니다.
 */
HCURSOR CInventoryManagerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/**
 * @brief 로그 메시지를 로그창(에디트 컨트롤)에 추가하는 함수입니다.
 * @param strLog 추가할 로그 문자열입니다.
 */
void CInventoryManagerDlg::AddLog(CString strLog)
{
	CTime time = CTime::GetCurrentTime();
	CString strTime = time.Format(_T("[%H:%M:%S]")); // 현재 시간을 [HH:MM:SS] 형식으로 만듭니다.

	CString strExistingLog; m_editLog.GetWindowText(strExistingLog);

	// 기존 로그에 새 로그를 추가합니다.
	CString strNewLog;
	if (strExistingLog.IsEmpty())
		strNewLog.Format(_T("%s %s"), strTime, strLog);
	else
		strNewLog.Format(_T("%s\r\n%s %s"), strExistingLog, strTime, strLog);

	m_editLog.SetWindowText(strNewLog);

	// 스크롤을 항상 맨 아래로 이동시켜 최신 로그가 보이게 합니다.
	int nLen = m_editLog.GetWindowTextLength();
	m_editLog.SetSel(nLen, nLen);
	m_editLog.SendMessage(EM_SCROLLCARET, 0, 0);
}

/**
 * @brief 로그창의 모든 내용을 지웁니다.
 */
void CInventoryManagerDlg::ClearLog()
{
	m_editLog.SetWindowText(_T(""));
	AddLog(_T("로그를 지웠습니다."));
}

/**
 * @brief 메인 탭 컨트롤을 초기화하고 탭 아이템들을 추가합니다.
 */
void CInventoryManagerDlg::InitTabs()
{
	m_tabMain.DeleteAllItems(); // 기존 탭 모두 삭제
	m_tabMain.InsertItem(0, _T("재고현황"));
	m_tabMain.InsertItem(1, _T("통계"));
	m_tabMain.InsertItem(2, _T("설정"));
	m_tabMain.SetCurSel(0); // 첫 번째 탭을 기본으로 선택
	m_nCurrentTab = 0;
}

/**
 * @brief '로그 지우기' 버튼 클릭 이벤트 핸들러입니다.
 */
void CInventoryManagerDlg::OnBnClickedButtonClearLog()
{
	ClearLog();
}

/**
 * @brief '새로고침' 버튼 클릭 이벤트 핸들러입니다.
 */
void CInventoryManagerDlg::OnBnClickedButtonRefresh()
{
	AddLog(_T("🔄 수동 새로고침 실행"));

	if (TestConnection()) // DB 연결이 유효한지 먼저 확인
	{
		RefreshInventoryData();  // 재고 데이터를 다시 불러옵니다.
		AddLog(_T("✅ 새로고침 완료"));
	}
	else // 연결이 끊겼다면 재연결 시도
	{
		AddLog(_T("🔌 재연결 시도 중..."));
		ConnectDatabase();
		if (m_bDBConnected) { // 재연결 성공 시 데이터 로드
			LoadInventoryData();
			UpdateInventoryList();
			SnapshotDisplayToCache();
		}
	}
}

/**
 * @brief 탭 선택 변경 이벤트 핸들러입니다.
 * @details 사용자가 다른 탭을 클릭했을 때 호출됩니다.
 */
void CInventoryManagerDlg::OnSelchangeTabMain(NMHDR*, LRESULT* pResult)
{
	m_nCurrentTab = m_tabMain.GetCurSel(); // 선택된 탭의 인덱스를 가져옵니다.

	CString strTabName;
	TCITEM item = { 0 };
	item.mask = TCIF_TEXT;
	item.pszText = strTabName.GetBuffer(50);
	item.cchTextMax = 50;
	m_tabMain.GetItem(m_nCurrentTab, &item);
	strTabName.ReleaseBuffer();
	AddLog(_T("📄 탭 변경: ") + strTabName);

	ShowTabPage(m_nCurrentTab); // 해당 탭에 맞는 화면을 보여줍니다.
	*pResult = 0;
}

/**
 * @brief 데이터베이스 연결을 설정하는 함수입니다.
 */
void CInventoryManagerDlg::ConnectDatabase()
{
	// CDBManager는 싱글톤 패턴으로 구현되어, 프로그램 전체에서 단 하나의 인스턴스만 존재합니다.
	m_pDBManager = CDBManager::GetInstance();
	if (m_pDBManager == nullptr)
	{
		AddLog(_T("❌ DB 관리자 초기화 실패"));
		m_bDBConnected = FALSE;
		return;
	}

	//// DB 연결 정보를 설정합니다.
	//DB_CONFIG config;
	//config.strHost = _T("192.168.0.92");
	//config.nPort = 3306;
	//config.strDatabase = _T("themost_db");
	//config.strUser = _T("mfcuser");
	//config.strPassword = _T("Moble1234");

	// 설정된 정보로 DB 연결을 시도합니다.
	//[수정] m_dbConfig 변수를 사용하도록 수정
	BOOL bResult = m_pDBManager->Connect(m_dbConfig);

	//[수정] 로그을 출력할 때도 m_dbConfig 변수를 사용하도록 수정
	if (bResult)
	{
		AddLog(_T("✅ 데이터베이스 연결 성공!"));
		CString strInfo;
		strInfo.Format(_T("📊 DB: %s@%s:%d"), m_dbConfig.strDatabase, m_dbConfig.strHost, m_dbConfig.nPort);
		AddLog(strInfo);
		m_bDBConnected = TRUE;
	}
	else
	{
		CString strError;
		strError.Format(_T("❌ DB 연결 실패: %s"), m_pDBManager->GetLastError());
		AddLog(strError);
		m_bDBConnected = FALSE;
		MessageBox(strError, _T("데이터베이스 연결 오류"), MB_OK | MB_ICONERROR);
	}
}

/**
 * @brief 데이터베이스 연결을 해제하는 함수입니다.
 */
void CInventoryManagerDlg::DisconnectDatabase()
{
	if (m_pDBManager != nullptr && m_bDBConnected)
	{
		m_pDBManager->Disconnect();
		AddLog(_T("🔌 데이터베이스 연결 해제"));
		m_bDBConnected = FALSE;
	}
}

/**
 * @brief 데이터베이스 연결이 활성 상태인지 테스트하는 함수입니다.
 * @return BOOL 연결이 유효하면 TRUE, 아니면 FALSE를 반환합니다.
 */
BOOL CInventoryManagerDlg::TestConnection()
{
	if (!m_bDBConnected || m_pDBManager == nullptr)
	{
		AddLog(_T("⚠️ 데이터베이스에 연결되지 않았습니다."));
		return FALSE;
	}

	// 가장 간단한 쿼리를 실행하여 DB가 응답하는지 확인합니다.
	CString strQuery = _T("SELECT 1");
	BOOL bResult = m_pDBManager->ExecuteSelect(strQuery);
	if (bResult)
	{
		AddLog(_T("✅ 연결 테스트 성공"));
		m_pDBManager->FreeResult(); // 쿼리 결과 리소스 해제
		return TRUE;
	}
	else
	{
		CString strError;
		strError.Format(_T("❌ 연결 테스트 실패: %s"), m_pDBManager->GetLastError());
		AddLog(strError);
		return FALSE;
	}
}

/**
 * @brief 데이터베이스에서 모든 재고 데이터를 가져와 m_vecInventory 벡터에 저장합니다.
 */
void CInventoryManagerDlg::LoadInventoryData()
{
	if (!m_bDBConnected || m_pDBManager == nullptr)
	{
		AddLog(_T("⚠️ 데이터베이스에 연결되지 않았습니다."));
		return;
	}

	AddLog(_T("📊 재고 데이터 로드 중..."));

	// DB 관리자를 통해 재고 목록을 가져옵니다.
	BOOL bResult = m_pDBManager->GetInventoryList(m_vecInventory);

	// 디버깅용 로그
	CString strDebug;
	strDebug.Format(_T("🔍 [DEBUG] GetInventoryList 결과: %s"), bResult ? _T("TRUE") : _T("FALSE"));
	AddLog(strDebug);
	strDebug.Format(_T("🔍 [DEBUG] 벡터 크기: %d"), (int)m_vecInventory.size());
	AddLog(strDebug);

	if (bResult)
	{
		CString strLog;
		strLog.Format(_T("✅ 재고 데이터 로드 완료 (%d건)"), (int)m_vecInventory.size());
		AddLog(strLog);

		// 로드된 데이터를 바탕으로 재고 상태(정상/주의/위험)별 개수를 셉니다.
		int nDanger = 0, nWarning = 0, nNormal = 0;
		for (size_t i = 0; i < m_vecInventory.size(); i++)
		{
			if (m_vecInventory[i].nStock == 0)            nDanger++;
			else if (m_vecInventory[i].nStock < m_nDangerThreshold)  nDanger++;
			else if (m_vecInventory[i].nStock < m_nWarningThreshold) nWarning++;
			else nNormal++;
		}

		// 통계 정보를 로그에 남깁니다.
		CString strStats;
		strStats.Format(_T("📈 정상: %d건 / 주의: %d건 / 위험: %d건"), nNormal, nWarning, nDanger);
		AddLog(strStats);

		if (nDanger > 0)
		{
			CString strAlert; strAlert.Format(_T("⚠️ 긴급 발주 필요: %d건!"), nDanger);
			AddLog(strAlert);
		}
	}
	else
	{
		CString strError;
		strError.Format(_T("❌ 재고 데이터 로드 실패: %s"), m_pDBManager->GetLastError());
		AddLog(strError);
	}
}

/**
 * @brief 재고 데이터를 새로고침합니다. (기존 데이터 삭제 -> 새로 로드 -> 화면 갱신 -> 캐시 갱신)
 */
void CInventoryManagerDlg::RefreshInventoryData()
{
	m_vecInventory.clear(); // 기존 메모리 내 재고 데이터 삭제
	LoadInventoryData();    // DB에서 새로 로드
	UpdateInventoryList();  // 화면 리스트 컨트롤 갱신
	SnapshotDisplayToCache();   // 검색/정렬을 위한 캐시 갱신
}

/**
 * @brief 재고 목록 리스트 컨트롤(CListCtrl)을 초기화합니다.
 * @details 컬럼 헤더(제목)를 생성하고, 리스트의 스타일을 설정합니다.
 */
void CInventoryManagerDlg::InitInventoryList()
{
	AddLog(_T("🔍 InitInventoryList 시작"));

	if (m_listInventory.GetSafeHwnd() == nullptr)
	{
		AddLog(_T("❌ 리스트 컨트롤 핸들이 없습니다!"));
		return;
	}

	m_listInventory.DeleteAllItems(); // 모든 아이템 삭제

	// 모든 컬럼 삭제
	int nColCount = m_listInventory.GetHeaderCtrl()->GetItemCount();
	for (int i = nColCount - 1; i >= 0; i--) m_listInventory.DeleteColumn(i);

	// 리스트 스타일 설정: 행 전체 선택, 그리드 라인 표시
	DWORD dwStyle = m_listInventory.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	m_listInventory.SetExtendedStyle(dwStyle);

	// 8개의 컬럼을 순서대로 생성합니다. (너비, 정렬 방식 지정)
	m_listInventory.InsertColumn(0, _T("상태"), LVCFMT_CENTER, 60);
	m_listInventory.InsertColumn(1, _T("품번"), LVCFMT_LEFT, 150);
	m_listInventory.InsertColumn(2, _T("상품명"), LVCFMT_LEFT, 150);
	m_listInventory.InsertColumn(3, _T("브랜드"), LVCFMT_LEFT, 100);
	m_listInventory.InsertColumn(4, _T("카테고리"), LVCFMT_LEFT, 100);
	m_listInventory.InsertColumn(5, _T("색상"), LVCFMT_CENTER, 80);
	m_listInventory.InsertColumn(6, _T("사이즈"), LVCFMT_CENTER, 80);
	m_listInventory.InsertColumn(7, _T("재고"), LVCFMT_RIGHT, 80);

	// 초기화 후 최종 컬럼 개수 확인 (디버깅 목적)
	int nFinalColCount = m_listInventory.GetHeaderCtrl()->GetItemCount();
	CString strDebug; strDebug.Format(_T("📋 최종 컬럼 개수: %d"), nFinalColCount);
	AddLog(strDebug);

	if (nFinalColCount == 8) AddLog(_T("✅ 재고 리스트 초기화 완료"));
	else                     AddLog(_T("❌ 재고 리스트 초기화 실패 (컬럼 개수 불일치)"));
}

/**
 * @brief m_vecInventory 벡터에 저장된 데이터로 리스트 컨트롤 화면을 채웁니다.
 */
void CInventoryManagerDlg::UpdateInventoryList()
{
	AddLog(_T("🔍 UpdateInventoryList 시작"));

	if (m_listInventory.GetSafeHwnd() == nullptr) {
		AddLog(_T("❌ 리스트 컨트롤 핸들이 없습니다!"));
		return;
	}

	// 컬럼이 제대로 생성되었는지 확인, 문제가 있다면 재초기화
	int nColCount = m_listInventory.GetHeaderCtrl()->GetItemCount();
	if (nColCount != 8)
	{
		CString strError; strError.Format(_T("❌ 컬럼 개수 오류 (현재: %d, 필요: 8)"), nColCount);
		AddLog(strError);
		AddLog(_T("🔄 리스트 재초기화 시도..."));
		InitInventoryList();
	}

	CString strDebug; strDebug.Format(_T("🔍 업데이트할 데이터: %d건"), (int)m_vecInventory.size());
	AddLog(strDebug);
	if (m_vecInventory.empty()) {
		AddLog(_T("⚠️ 표시할 재고 데이터가 없습니다."));
		m_listInventory.DeleteAllItems();
		return;
	}

	m_listInventory.SetRedraw(FALSE); // 많은 데이터를 추가하기 전 화면 갱신을 멈춰 성능 향상
	m_listInventory.DeleteAllItems(); // 기존 아이템 모두 삭제

	int nAddedCount = 0;
	// m_vecInventory의 모든 아이템에 대해 반복
	for (size_t i = 0; i < m_vecInventory.size(); i++)
	{
		const INVENTORY_ITEM& item = m_vecInventory[i];

		// 재고 수량에 따라 '상태' 문자열 결정
		CString strStatus;
		if (item.nStock == 0)                       strStatus = _T("품절");
		else if (item.nStock < m_nDangerThreshold)  strStatus = _T("위험");
		else if (item.nStock < m_nWarningThreshold) strStatus = _T("주의");
		else                                        strStatus = _T("정상");

		// 새 행(아이템)을 추가하고 각 컬럼에 맞는 데이터를 설정
		int nIndex = m_listInventory.InsertItem((int)i, strStatus);
		if (nIndex == -1) {
			CString strError; strError.Format(_T("❌ 항목 추가 실패 (인덱스: %d)"), (int)i);
			AddLog(strError);
			continue;
		}

		m_listInventory.SetItemText(nIndex, 1, item.strOptionCode);
		m_listInventory.SetItemText(nIndex, 2, item.strProductName);
		m_listInventory.SetItemText(nIndex, 3, item.strBrandName);
		m_listInventory.SetItemText(nIndex, 4, item.strCategoryName);
		m_listInventory.SetItemText(nIndex, 5, item.strColorName);
		m_listInventory.SetItemText(nIndex, 6, item.strSizeName);
		CString strStock; strStock.Format(_T("%d"), item.nStock);
		m_listInventory.SetItemText(nIndex, 7, strStock);

		// 각 행에 고유 ID(nOptionID)를 데이터로 저장해둡니다. 화면에는 보이지 않지만,
		// 나중에 특정 행을 식별할 때 매우 유용하게 사용됩니다.
		m_listInventory.SetItemData(nIndex, (DWORD_PTR)item.nOptionID);
		nAddedCount++;
	}

	m_listInventory.SetRedraw(TRUE); // 화면 갱신을 다시 활성화
	m_listInventory.Invalidate(TRUE); // 컨트롤을 강제로 다시 그리도록 요청
	m_listInventory.UpdateWindow();

	CString strLog; strLog.Format(_T("✅ 리스트 업데이트 완료 (%d건)"), nAddedCount);
	AddLog(strLog);
}

/**
 * @brief '발주' 버튼 클릭 이벤트 핸들러입니다. (단일/일괄 발주 지원)
 */
void CInventoryManagerDlg::OnBnClickedButtonOrder()
{
	int nSelectedCount = m_listInventory.GetSelectedCount();
	if (nSelectedCount == 0) {
		AfxMessageBox(_T("발주할 품목을 선택해주세요."));
		AddLog(_T("⚠️ 발주: 품목 선택 안 됨"));
		return;
	}

	// =============================================================
	// 분기 1: 한 개의 품목만 선택된 경우 (기존 방식)
	// =============================================================
	if (nSelectedCount == 1)
	{
		POSITION pos = m_listInventory.GetFirstSelectedItemPosition();
		int nItem = m_listInventory.GetNextSelectedItem(pos);
		int nOptionID = (int)m_listInventory.GetItemData(nItem);

		// 기존의 단일 발주 대화상자를 사용합니다.
		COrderDlg dlg;
		dlg.m_nOptionID = nOptionID;
		dlg.m_strOptionCode = m_listInventory.GetItemText(nItem, 1);
		dlg.m_strProductName = m_listInventory.GetItemText(nItem, 2);
		dlg.m_nCurrentStock = _ttoi(m_listInventory.GetItemText(nItem, 7)); // 리스트에서 직접 재고 읽기
		dlg.m_nWarningThreshold = this->m_nWarningThreshold; // 메인 설정값 전달
		dlg.m_nDangerThreshold = this->m_nDangerThreshold;   // 메인 설정값 전달

		if (dlg.DoModal() == IDOK)
		{
			CString strLog;
			strLog.Format(_T("📦 발주 시도: %s (수량: %d)"), dlg.m_strOptionCode, dlg.m_nOrderQuantity);
			AddLog(strLog);

			if (!m_bDBConnected || m_pDBManager == nullptr) {
				AfxMessageBox(_T("데이터베이스에 연결되지 않았습니다."));
				AddLog(_T("❌ 발주 실패: DB 연결 없음"));
				return;
			}

			if (m_pDBManager->AddStock(nOptionID, dlg.m_nOrderQuantity))
			{
				strLog.Format(_T("✅ 발주 성공: %s (%d개 추가)"), dlg.m_strOptionCode, dlg.m_nOrderQuantity);
				AddLog(strLog);
				AfxMessageBox(_T("발주가 완료되었습니다."));
				RefreshInventoryData();
			}
			else
			{
				CString strError; strError.Format(_T("❌ 발주 실패: %s"), m_pDBManager->GetLastError());
				AddLog(strError);
				AfxMessageBox(strError);
			}
		}
		else {
			AddLog(_T("🚫 발주 취소됨"));
		}
	}
	// =============================================================
	// 분기 2: 여러 개의 품목이 선택된 경우 (새로운 방식)
	// =============================================================
	else
	{
		// 선택된 모든 품목의 ID를 벡터에 저장합니다.
		std::vector<int> vecOptionIDs;
		POSITION pos = m_listInventory.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_listInventory.GetNextSelectedItem(pos);
			vecOptionIDs.push_back((int)m_listInventory.GetItemData(nItem));
		}

		// 새로운 일괄 발주 대화상자를 생성합니다.
		CBulkOrderDlg dlg;
		dlg.m_nSelectedItemCount = vecOptionIDs.size(); // 선택된 개수를 전달

		// 사용자가 '확인'을 누르면
		if (dlg.DoModal() == IDOK)
		{
			int nQuantity = dlg.m_nOrderQuantity; // 사용자가 입력한 발주 수량
			CString strLog;
			strLog.Format(_T("📦 %d개 품목에 대해 %d개씩 일괄 발주 시도..."), (int)vecOptionIDs.size(), nQuantity);
			AddLog(strLog);

			if (!m_bDBConnected || m_pDBManager == nullptr) {
				AfxMessageBox(_T("데이터베이스에 연결되지 않았습니다."));
				AddLog(_T("❌ 발주 실패: DB 연결 없음"));
				return;
			}

			// DBManager의 일괄 발주 함수를 호출합니다.
			if (m_pDBManager->AddStockToItems(vecOptionIDs, nQuantity))
			{
				AddLog(_T("✅ 일괄 발주 성공!"));
				AfxMessageBox(_T("발주가 완료되었습니다."));
				RefreshInventoryData(); // 성공 시 목록 새로고침
			}
			else
			{
				CString strError; strError.Format(_T("❌ 일괄 발주 실패: %s"), m_pDBManager->GetLastError());
				AddLog(strError);
				AfxMessageBox(strError);
			}
		}
		else
		{
			AddLog(_T("🚫 일괄 발주 취소됨"));
		}
	}
}

/**
 * @brief WM_CLOSE 메시지 핸들러. 창이 닫힐 때 호출됩니다.
 * @details 안전한 종료를 위해 DB 연결 해제 등의 마무리 작업을 수행합니다.
 */
void CInventoryManagerDlg::OnClose()
{
	AddLog(_T("🚪 프로그램 종료 요청"));

	// 다른 팝업 대화 상자가 열려있으면 먼저 닫도록 처리
	CWnd* pWnd = GetWindow(GW_ENABLEDPOPUP);
	if (pWnd != nullptr && pWnd != this)
	{
		AddLog(_T("⚠️ 열린 대화상자를 먼저 닫습니다..."));
		pWnd->SendMessage(WM_CLOSE);

		MSG msg; int nCount = 0;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) && nCount < 100) {
			TranslateMessage(&msg); DispatchMessage(&msg); nCount++;
		}
		AddLog(_T("✅ 대화상자 닫기 완료"));
	}

	DisconnectDatabase();
	AddLog(_T("👋 프로그램 종료 중..."));
	CDialogEx::OnClose(); // 기본 닫기 동작 수행
}

/**
 * @brief 리스트 항목 더블 클릭 이벤트 핸들러입니다. (재고 수량 직접 수정 기능)
 */
void CInventoryManagerDlg::OnDblclkListInventory(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 여러 항목이 선택된 경우, 재고 수정 기능을 실행하지 않고 즉시 종료합니다.
	if (m_listInventory.GetSelectedCount() > 1)
	{
		*pResult = 0; // 메시지 처리를 완료했다고 알림
		return;       // 함수 종료
	}

	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nItem = pNMItemActivate->iItem; // 더블 클릭된 아이템의 인덱스

	if (nItem == -1) { AddLog(_T("⚠️ 재고 수정: 항목이 선택되지 않음")); *pResult = 0; return; }

	// 더블클릭된 항목의 고유 ID(OptionID)를 가져옵니다.
	int nOptionID = (int)m_listInventory.GetItemData(nItem);

	INVENTORY_ITEM* pItem = nullptr;
	for (size_t i = 0; i < m_vecInventory.size(); i++) {
		if (m_vecInventory[i].nOptionID == nOptionID) { pItem = &m_vecInventory[i]; break; }
	}
	if (!pItem) {
		AfxMessageBox(_T("품목 정보를 찾을 수 없습니다."));
		AddLog(_T("❌ 재고 수정: 품목 정보 없음"));
		*pResult = 0; return;
	}

	// CEditStockDlg는 헤더 파일에 선언되지 않았으므로, 실제로는 다른 이름의 대화 상자일 수 있습니다.
	// 여기서는 가상의 '재고 수정 대화 상자'로 간주하고 설명합니다.
	CEditStockDlg dlg; // CEditStockDlg는 이 프로젝트에 없는 가상의 이름입니다. COrderDlg와 유사할 것으로 추정됩니다.
	dlg.m_nOptionID = pItem->nOptionID;
	dlg.m_strOptionCode = pItem->strOptionCode;
	dlg.m_strProductName = pItem->strProductName;
	dlg.m_nCurrentStock = pItem->nStock;

	if (dlg.DoModal() == IDOK)
	{
		CString strLog;
		strLog.Format(_T("✏️ 재고 수정 시도: %s (%d → %d)"),
			pItem->strOptionCode, pItem->nStock, dlg.m_nNewStock);
		AddLog(strLog);

		if (!m_bDBConnected || m_pDBManager == nullptr) {
			AfxMessageBox(_T("데이터베이스에 연결되지 않았습니다."));
			AddLog(_T("❌ 재고 수정 실패: DB 연결 없음"));
			*pResult = 0; return;
		}

		// DB에 재고 수량 업데이트를 요청합니다.
		BOOL bResult = m_pDBManager->UpdateStock(nOptionID, dlg.m_nNewStock);
		if (bResult)
		{
			strLog.Format(_T("✅ 재고 수정 성공: %s (%d → %d, 변경: %+d)"),
				pItem->strOptionCode, pItem->nStock, dlg.m_nNewStock, dlg.m_nNewStock - pItem->nStock);
			AddLog(strLog);
			AfxMessageBox(_T("재고가 수정되었습니다."));
			RefreshInventoryData(); // 화면 새로고침
		}
		else
		{
			CString strError; strError.Format(_T("❌ 재고 수정 실패: %s"), m_pDBManager->GetLastError());
			AddLog(strError);
			AfxMessageBox(strError);
		}
	}
	else {
		AddLog(_T("🚫 재고 수정 취소됨"));
	}
	*pResult = 0;
}

/**
 * @brief [전면 수정] '삭제' 버튼(IDC_BUTTON2) 클릭 이벤트 핸들러입니다.
 * 요구사항에 맞춰 옵션 삭제 및 마지막 옵션일 경우 상품까지 삭제하도록 변경되었습니다.
 */
void CInventoryManagerDlg::OnBnClickedButton2()
{
	// 리스트에서 선택된 모든 항목의 위치(POSITION)를 가져옵니다.
	POSITION pos = m_listInventory.GetFirstSelectedItemPosition();
	if (pos == nullptr) {
		AfxMessageBox(_T("삭제할 품목(옵션)을 먼저 선택해주세요."));
		AddLog(_T("⚠️ 삭제: 품목이 선택되지 않음"));
		return;
	}

	std::vector<int> vecOptionIDs;
	CString strProductList; // 확인 메시지에 보여줄 품번 목록
	int nItemCount = 0;

	// 루프를 돌며 선택된 모든 항목의 정보를 수집합니다.
	while (pos)
	{
		int nItem = m_listInventory.GetNextSelectedItem(pos);
		vecOptionIDs.push_back((int)m_listInventory.GetItemData(nItem));

		if (nItemCount < 5) // 확인 창에는 최대 5개 품번만 표시
		{
			strProductList += _T("- ") + m_listInventory.GetItemText(nItem, 1) + _T("\n");
		}
		nItemCount++;
	}

	if (nItemCount > 5)
	{
		strProductList.AppendFormat(_T("... 외 %d개"), nItemCount - 5);
	}

	// 사용자에게 최종 확인을 받습니다.
	CString strConfirmMsg;
	strConfirmMsg.Format(
		_T("총 %d개의 품목(옵션)을 정말로 삭제하시겠습니까?\n\n")
		_T("%s\n")
		_T("이 작업은 되돌릴 수 없습니다.\n")
		_T("(만약 이 삭제로 인해 상품에 남는 옵션이 없게 되면, 상품 정보 전체가 삭제됩니다.)"),
		nItemCount, strProductList);

	if (AfxMessageBox(strConfirmMsg, MB_YESNO | MB_ICONWARNING) != IDYES) {
		AddLog(_T("🚫 삭제가 사용자에 의해 취소됨"));
		return;
	}

	if (!m_bDBConnected || m_pDBManager == nullptr) {
		AfxMessageBox(_T("데이터베이스에 연결되지 않았습니다."));
		AddLog(_T("❌ 삭제 실패: DB 연결 없음"));
		return;
	}

	CString strLog;
	strLog.Format(_T("🗑️ %d개 품목(옵션) 일괄 삭제 시도..."), nItemCount);
	AddLog(strLog);

	// DB에 품목(옵션) 일괄 삭제 및 상품 정리 요청
	if (m_pDBManager->DeleteOptionsAndCleanup(vecOptionIDs)) {
		AfxMessageBox(_T("선택한 품목(옵션)이 성공적으로 삭제되었습니다."));
		AddLog(_T("✅ 삭제 성공!"));
		RefreshInventoryData(); // 화면 새로고침
	}
	else {
		CString strError; strError.Format(_T("❌ 삭제 실패: %s"), m_pDBManager->GetLastError());
		AddLog(strError);
		AfxMessageBox(strError);
	}
}


/**
 * @brief '상품 추가' 버튼(IDC_BUTTON3) 클릭 이벤트 핸들러입니다.
 */
void CInventoryManagerDlg::OnBnClickedButton3()
{
	AddLog(_T("'상품 추가' 대화상자를 엽니다."));
	CAddProductDlg dlg;
	// '상품 추가' 대화상자를 띄우고, '확인'을 누르면 목록을 새로고침합니다.
	if (dlg.DoModal() == IDOK) {
		AddLog(_T("새 상품이 추가되었습니다. 목록을 새로고침합니다."));
		RefreshInventoryData();
	}
	else
	{
		AddLog(_T("🚫 '상품 추가'가 취소되었습니다."));
	}
}

/**
 * @brief '검색' 버튼 클릭 이벤트 핸들러입니다.
 */
void CInventoryManagerDlg::OnBnClickedBtnSearch()
{
	ApplyFiltersAndSearch();
}

/**
 * @brief 키보드 입력을 가로채는 함수입니다.
 * @details 검색창(IDC_EDIT_SEARCH)에서 엔터 키를 눌렀을 때 검색을 실행하기 위해 사용됩니다.
 */
BOOL CInventoryManagerDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		// 현재 포커스가 검색 에디트 컨트롤에 있는지 확인
		if (GetFocus() && GetFocus()->GetDlgCtrlID() == IDC_EDIT_SEARCH)
		{
			ApplyFiltersAndSearch();
			return TRUE; // 메시지 처리를 여기서 완료했음을 알림
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

/**
 * @brief 필터 콤보 박스 선택 변경 이벤트 핸들러입니다.
 */
void CInventoryManagerDlg::OnSelchangeComboFilter()
{
	ApplyFiltersAndSearch();
}

/**
 * @brief 현재 리스트 컨트롤에 표시된 모든 데이터를 m_allRowsDisplay 벡터에 복사(캐시)합니다.
 * @details DB를 다시 읽지 않고, 메모리에 있는 캐시 데이터로 검색/정렬을 빠르게 수행하기 위함입니다.
 */
void CInventoryManagerDlg::SnapshotDisplayToCache()
{
	m_allRowsDisplay.clear(); // 기존 캐시 삭제
	const int rowCount = m_listInventory.GetItemCount();
	if (rowCount == 0) return;

	m_allRowsDisplay.reserve(rowCount); // 메모리 재할당 방지를 위해 미리 공간 확보
	for (int i = 0; i < rowCount; ++i)
	{
		DisplayRow r;
		// 화면에 보이지 않는 고유 ID(OptionID)도 함께 캐시에 저장합니다.
		r.nOptionID = (int)m_listInventory.GetItemData(i);

		// 화면에 보이는 모든 컬럼의 텍스트 데이터를 캐시 구조체에 복사합니다.
		r.col0 = m_listInventory.GetItemText(i, 0);
		r.col1 = m_listInventory.GetItemText(i, 1);
		r.col2 = m_listInventory.GetItemText(i, 2);
		r.col3 = m_listInventory.GetItemText(i, 3);
		r.col4 = m_listInventory.GetItemText(i, 4);
		r.col5 = m_listInventory.GetItemText(i, 5);
		r.col6 = m_listInventory.GetItemText(i, 6);
		r.col7 = m_listInventory.GetItemText(i, 7);
		m_allRowsDisplay.push_back(r);
	}
}

/**
 * @brief 캐시된 데이터(DisplayRow 벡터)를 리스트 컨트롤 화면에 표시합니다.
 * @details 검색 또는 정렬된 결과를 화면에 보여줄 때 사용됩니다.
 * @param rows 화면에 표시할 행 데이터의 벡터입니다.
 */
void CInventoryManagerDlg::ShowRowsFromCache(const std::vector<DisplayRow>& rows)
{
	m_listInventory.SetRedraw(FALSE); // 화면 갱신 중지
	m_listInventory.DeleteAllItems(); // 기존 항목 모두 삭제

	for (const auto& r : rows)
	{
		int i = m_listInventory.InsertItem(m_listInventory.GetItemCount(), r.col0);
		m_listInventory.SetItemText(i, 1, r.col1);
		m_listInventory.SetItemText(i, 2, r.col2);
		m_listInventory.SetItemText(i, 3, r.col3);
		m_listInventory.SetItemText(i, 4, r.col4);
		m_listInventory.SetItemText(i, 5, r.col5);
		m_listInventory.SetItemText(i, 6, r.col6);
		m_listInventory.SetItemText(i, 7, r.col7);
		m_listInventory.SetItemData(i, (DWORD_PTR)r.nOptionID); // 고유 ID도 다시 설정
	}

	m_listInventory.SetRedraw(TRUE); // 화면 갱신 재개
	m_listInventory.Invalidate(); // 컨트롤 다시 그리기
}


/**
 * @brief 선택된 탭에 따라 관련된 UI 컨트롤들을 보여주거나 숨깁니다.
 * @param idx 보여줄 탭의 인덱스입니다. (0: 재고, 1: 통계, 2: 설정)
 */
void CInventoryManagerDlg::ShowTabPage(int idx)
{
	const bool showInventory = (idx == 0);
	const bool showStats = (idx == 1);
	const bool showSettings = (idx == 2);

	// '재고 현황' 탭 관련 컨트롤들의 표시 여부를 설정합니다.
	m_listInventory.ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_EDIT_SEARCH)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_BTN_SEARCH)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_BUTTON_ORDER)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_BUTTON2)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_BUTTON3)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_COMBO_FILTER_BRAND)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_COMBO_FILTER_CATEGORY)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_BUTTON_EXPORT_INV)->ShowWindow(showInventory ? SW_SHOW : SW_HIDE);

	// '통계' 탭을 선택한 경우, 통계 다이얼로그를 보여줍니다.
	if (m_pStatsDlg) {
		if (showStats) {
			m_pStatsDlg->Reload(); // 통계 데이터를 최신으로 다시 로드
			m_pStatsDlg->ShowWindow(SW_SHOW);
		}
		else {
			m_pStatsDlg->ShowWindow(SW_HIDE);
		}
	}

	// '설정' 탭을 선택한 경우, 설정 다이얼로그를 보여줍니다.
	if (m_pSettingsDlg) {
		if (showSettings) {
			// 기존 임계값 설정 로드
			m_pSettingsDlg->LoadSettings(m_nWarningThreshold, m_nDangerThreshold);

			// [추가된 핵심 코드] 현재 DB 정보를 설정 탭의 입력란에 로드합니다.
			m_pSettingsDlg->LoadDbSettings(m_dbConfig);
			m_pSettingsDlg->LoadAutoOrderConfig();
			m_pSettingsDlg->ShowWindow(SW_SHOW);
		}
		else {
			m_pSettingsDlg->ShowWindow(SW_HIDE);
		}
	}
}

/**
 * @brief 필터와 검색어를 적용하여 리스트를 갱신하는 핵심 함수입니다.
 */
void CInventoryManagerDlg::ApplyFiltersAndSearch()
{
	// 1. 현재 필터 콤보 박스에서 선택된 값을 가져옵니다.
	CString strBrandFilter;
	int nBrandIndex = m_comboFilterBrand.GetCurSel();
	if (nBrandIndex > 0) // "전체 브랜드"가 아닌 특정 브랜드를 선택한 경우
	{
		m_comboFilterBrand.GetLBText(nBrandIndex, strBrandFilter);
	}

	CString strCategoryFilter;
	int nCategoryIndex = m_comboFilterCategory.GetCurSel();
	if (nCategoryIndex > 0) // "전체 카테고리"가 아닌 특정 카테고리를 선택한 경우
	{
		m_comboFilterCategory.GetLBText(nCategoryIndex, strCategoryFilter);
	}

	// 2. 검색창에 입력된 검색어를 가져옵니다.
	CString strSearchKeyword;
	m_editSearch.GetWindowText(strSearchKeyword);
	strSearchKeyword.Trim(); // 앞뒤 공백 제거
	const CString lowerKeyword = ToLower(strSearchKeyword); // 대소문자 구분 없는 비교를 위해 소문자로 변환

	CString strLog;
	strLog.Format(_T("🔍 검색/필터 적용: 브랜드=[%s], 카테고리=[%s], 키워드=[%s]"),
		strBrandFilter.IsEmpty() ? _T("전체") : strBrandFilter,
		strCategoryFilter.IsEmpty() ? _T("전체") : strCategoryFilter,
		strSearchKeyword.IsEmpty() ? _T("없음") : strSearchKeyword);
	AddLog(strLog);

	// 3. 필터링된 결과를 담을 새로운 벡터를 준비합니다.
	std::vector<DisplayRow> filteredRows;

	// 4. 전체 데이터가 담긴 캐시(m_allRowsDisplay)를 하나씩 확인하며 필터링합니다.
	for (const auto& row : m_allRowsDisplay)
	{
		// [조건 1] 브랜드 필터 확인
		CString brandFromRow = row.col3;
		bool bBrandMatch = strBrandFilter.IsEmpty() || (brandFromRow.Trim() == strBrandFilter);
		if (!bBrandMatch) // 브랜드가 일치하지 않으면 이 행은 건너뜁니다.
		{
			continue;
		}

		// [조건 2] 카테고리 필터 확인
		CString categoryFromRow = row.col4;
		bool bCategoryMatch = strCategoryFilter.IsEmpty() || (categoryFromRow.Trim() == strCategoryFilter);
		if (!bCategoryMatch) // 카테고리가 일치하지 않으면 이 행은 건너뜁니다.
		{
			continue;
		}

		// [조건 3] 검색어 필터 확인 (검색어가 입력된 경우에만)
		if (!lowerKeyword.IsEmpty())
		{
			// 모든 컬럼 내용에 검색어가 포함되어 있는지 확인합니다.
			bool bKeywordMatch =
				(ToLower(row.col0).Find(lowerKeyword) >= 0) || // 상태
				(ToLower(row.col1).Find(lowerKeyword) >= 0) || // 품번
				(ToLower(row.col2).Find(lowerKeyword) >= 0) || // 상품명
				(ToLower(row.col3).Find(lowerKeyword) >= 0) || // 브랜드
				(ToLower(row.col4).Find(lowerKeyword) >= 0) || // 카테고리
				(ToLower(row.col5).Find(lowerKeyword) >= 0) || // 색상
				(ToLower(row.col6).Find(lowerKeyword) >= 0);   // 사이즈 (col5와 col6가 색상/사이즈로 추정)

			if (!bKeywordMatch) // 어느 컬럼에도 검색어가 없으면 이 행은 건너뜁니다.
			{
				continue;
			}
		}

		// 모든 필터 조건을 통과한 행만 결과 벡터에 추가합니다.
		filteredRows.push_back(row);
	}

	// 5. 필터링된 최종 결과를 화면에 표시합니다.
	ShowRowsFromCache(filteredRows);
}

/**
 * @brief 리스트 컬럼 헤더 클릭 이벤트 핸들러 (정렬 기능)
 */
void CInventoryManagerDlg::OnColumnclickListInventory(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int nColumn = pNMLV->iSubItem; // 사용자가 클릭한 컬럼의 인덱스

	// 같은 컬럼을 다시 클릭하면 정렬 순서를 반대로 변경 (오름차순 <-> 내림차순)
	if (nColumn == m_nSortColumn) {
		m_bSortAscending = !m_bSortAscending;
	}
	// 다른 컬럼을 클릭하면 해당 컬럼을 기준으로 오름차순 정렬
	else {
		m_nSortColumn = nColumn;
		m_bSortAscending = true;
	}

	TCHAR szText[256] = { 0 };
	HDITEM hdi = { 0 };
	hdi.mask = HDI_TEXT;
	hdi.pszText = szText;
	hdi.cchTextMax = 256;
	m_listInventory.GetHeaderCtrl()->GetItem(nColumn, &hdi);
	CString strLog;
	strLog.Format(_T("🔃 목록 정렬: '%s' 컬럼, %s"), CString(hdi.pszText), m_bSortAscending ? _T("오름차순") : _T("내림차순"));
	AddLog(strLog);

	// std::sort에 사용할 비교 함수(람다)를 정의합니다.
	auto sortLambda = [&](const DisplayRow& a, const DisplayRow& b) -> bool {
		CString strA, strB;

		// 클릭된 컬럼(m_nSortColumn)에 따라 비교할 데이터를 선택합니다.
		switch (m_nSortColumn) {
		case 0: strA = a.col0; strB = b.col0; break; // 상태
		case 1: strA = a.col1; strB = b.col1; break; // 품번
		case 2: strA = a.col2; strB = b.col2; break; // 상품명
		case 3: strA = a.col3; strB = b.col3; break; // 브랜드
		case 4: strA = a.col4; strB = b.col4; break; // 카테고리
		case 5: strA = a.col5; strB = b.col5; break; // 색상
		case 6: strA = a.col6; strB = b.col6; break; // 사이즈
		case 7: { // '재고' 컬럼은 문자열이 아닌 숫자로 비교해야 합니다.
			int valA = _ttoi(a.col7); // 문자열을 정수로 변환
			int valB = _ttoi(b.col7);
			if (m_bSortAscending) return valA < valB; // 오름차순
			else return valA > valB; // 내림차순
		}
		default: return false; // 예외 처리
		}

		// 문자열 컬럼 비교
		if (m_bSortAscending) {
			return strA.Compare(strB) < 0; // 오름차순
		}
		else {
			return strA.Compare(strB) > 0; // 내림차순
		}
		};

	// 정의된 람다 함수를 이용해 캐시 데이터(m_allRowsDisplay)를 정렬합니다.
	std::sort(m_allRowsDisplay.begin(), m_allRowsDisplay.end(), sortLambda);

	// 정렬된 캐시 데이터로 리스트 화면을 다시 그립니다.
	ShowRowsFromCache(m_allRowsDisplay);

	*pResult = 0;
}

/**
 * @brief '설정' 탭에서 변경된 재고 기준치를 메인 다이얼로그에 적용하는 함수입니다.
 * @details CSettingsDlg에서 이 함수를 호출하여 메인 다이얼로그의 상태를 변경합니다.
 * @param nWarning 새로운 '주의' 기준값
 * @param nDanger 새로운 '위험' 기준값
 */
void CInventoryManagerDlg::UpdateThresholds(int nWarning, int nDanger)
{
	// 메인 다이얼로그의 멤버 변수를 업데이트합니다.
	m_nWarningThreshold = nWarning;
	m_nDangerThreshold = nDanger;

	//변경된 값을 config.ini 파일에 즉시 저장합니다.
	SaveThresholdsToConfig();

	CString strLog;
	strLog.Format(_T("⚙️ 설정 변경: 주의 기준=%d, 위험 기준=%d"), nWarning, nDanger);
	AddLog(strLog);

	// 변경된 기준(주의/위험)을 리스트의 '상태' 컬럼에 즉시 반영하기 위해 새로고침합니다.
	AddLog(_T("🔄 설정 적용을 위해 목록을 새로고침합니다."));
	RefreshInventoryData();
}

// ✅ [추가] config.ini 파일의 전체 경로를 만들어주는 함수
CString GetConfigFilePath()
{
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH); // 실행 파일의 전체 경로를 얻어옴
	CString strPath(szPath);
	int nPos = strPath.ReverseFind('\\');
	if (nPos != -1)
	{
		strPath = strPath.Left(nPos); // 경로에서 파일 이름(InventoryManager.exe)만 제거
	}
	return strPath + _T("\\config.ini"); // 실행 파일 경로에 config.ini를 덧붙여 반환
}

// ✅ [추가] config.ini 파일에서 DB 정보를 읽어와 m_dbConfig 변수에 저장하는 함수
void CInventoryManagerDlg::LoadDbConfig()
{
	CString strConfigFile = GetConfigFilePath();

	// ini 파일에서 값을 읽어옵니다. 만약 키가 없으면 지정된 기본값을 사용합니다.
	// GetPrivateProfileString(섹션, 키, 기본값, 저장할버퍼, 버퍼크기, 파일경로)
	TCHAR szHost[256], szDatabase[256], szUser[256], szPassword[256];

	GetPrivateProfileString(_T("Database"), _T("Host"), _T("192.168.0.92"), szHost, 256, strConfigFile);
	m_dbConfig.strHost = szHost;

	m_dbConfig.nPort = GetPrivateProfileInt(_T("Database"), _T("Port"), 3306, strConfigFile);

	GetPrivateProfileString(_T("Database"), _T("DatabaseName"), _T("themost_db"), szDatabase, 256, strConfigFile);
	m_dbConfig.strDatabase = szDatabase;

	GetPrivateProfileString(_T("Database"), _T("User"), _T("mfcuser"), szUser, 256, strConfigFile);
	m_dbConfig.strUser = szUser;

	GetPrivateProfileString(_T("Database"), _T("Password"), _T("Moble1234"), szPassword, 256, strConfigFile);
	m_dbConfig.strPassword = szPassword;

	// [Settings] 섹션에서 WarningThreshold 값을 읽어옵니다. 없으면 기본값 30을 사용합니다.
	m_nWarningThreshold = GetPrivateProfileInt(_T("Settings"), _T("WarningThreshold"), 30, strConfigFile);
	// [Settings] 섹션에서 DangerThreshold 값을 읽어옵니다. 없으면 기본값 10을 사용합니다.
	m_nDangerThreshold = GetPrivateProfileInt(_T("Settings"), _T("DangerThreshold"), 10, strConfigFile);

	// 만약 config.ini 파일이 존재하지 않았다면, 방금 읽어온 기본값으로 파일을 새로 저장합니다.
	// 이 로직 덕분에 프로그램을 처음 실행하면 자동으로 기존 정보와 동일한 config.ini가 생성됩니다.
	if (GetFileAttributes(strConfigFile) == INVALID_FILE_ATTRIBUTES)
	{
		SaveDbConfig();
	}
}

// ✅ [추가] 현재 m_dbConfig 변수에 저장된 DB 정보를 config.ini 파일에 기록하는 함수
void CInventoryManagerDlg::SaveDbConfig()
{
	CString strConfigFile = GetConfigFilePath();
	CString strPort;
	strPort.Format(_T("%d"), m_dbConfig.nPort);

	WritePrivateProfileString(_T("Database"), _T("Host"), m_dbConfig.strHost, strConfigFile);
	WritePrivateProfileString(_T("Database"), _T("Port"), strPort, strConfigFile);
	WritePrivateProfileString(_T("Database"), _T("DatabaseName"), m_dbConfig.strDatabase, strConfigFile);
	WritePrivateProfileString(_T("Database"), _T("User"), m_dbConfig.strUser, strConfigFile);
	WritePrivateProfileString(_T("Database"), _T("Password"), m_dbConfig.strPassword, strConfigFile);
}

// ✅ [추가] CSettingsDlg로부터 새로운 DB 설정을 받아 재연결을 수행하는 함수
void CInventoryManagerDlg::UpdateDbConfigAndReconnect(const DB_CONFIG& newConfig)
{
	AddLog(_T("⚙️ DB 설정 변경 시도..."));

	// 1. 멤버 변수를 새로운 설정으로 업데이트합니다.
	m_dbConfig = newConfig;

	// 2. 새로운 설정을 config.ini 파일에 저장합니다.
	SaveDbConfig();
	AddLog(_T("💾 새로운 DB 설정을 파일에 저장했습니다."));

	// 3. 기존 DB 연결을 끊습니다.
	DisconnectDatabase();

	// 4. 새로운 정보로 다시 연결을 시도합니다.
	AddLog(_T("🔌 새로운 정보로 재연결 시도 중..."));
	ConnectDatabase();

	// 5. 재연결 결과에 따라 후속 조치를 취합니다.
	if (m_bDBConnected)
	{
		AfxMessageBox(_T("✅ 데이터베이스에 성공적으로 다시 연결되었습니다."));
		AddLog(_T("✅ 재연결 성공! 목록을 새로고침합니다."));

		// 재고 목록, 필터 등을 모두 새로고침합니다.
		RefreshInventoryData();
	}
	else
	{
		// 실패 메시지는 ConnectDatabase 함수 내부에서 이미 처리하므로 추가적인 MessageBox는 필요 없습니다.
		AddLog(_T("❌ 재연결 실패. 이전 설정으로 되돌리거나 설정을 다시 확인해주세요."));

		// 연결 실패 시, 목록을 비워서 오래된 정보가 보이는 것을 방지합니다.
		m_vecInventory.clear();
		UpdateInventoryList();
	}
}

void CInventoryManagerDlg::SaveThresholdsToConfig()
{
	CString strConfigFile = GetConfigFilePath();
	CString strValue;

	// m_nWarningThreshold 값을 문자열로 변환하여 [Settings] 섹션에 저장
	strValue.Format(_T("%d"), m_nWarningThreshold);
	WritePrivateProfileString(_T("Settings"), _T("WarningThreshold"), strValue, strConfigFile);

	// m_nDangerThreshold 값을 문자열로 변환하여 [Settings] 섹션에 저장
	strValue.Format(_T("%d"), m_nDangerThreshold);
	WritePrivateProfileString(_T("Settings"), _T("DangerThreshold"), strValue, strConfigFile);
}

void CInventoryManagerDlg::OnBnClickedButtonExportInv()
{
	// 1. 파일 저장 대화상자 열기
	CFileDialog dlg(FALSE, _T("csv"), _T("inventory_list.csv"),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("CSV 파일 (*.csv)|*.csv|모든 파일 (*.*)|*.*||"));

	if (dlg.DoModal() != IDOK)
	{
		AddLog(_T("🚫 내보내기가 취소되었습니다."));
		return;
	}

	CString strFilePath = dlg.GetPathName();

	// CFile을 사용하여 바이너리 모드로 파일을 엽니다.
	CFile file;
	if (!file.Open(strFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
	{
		AfxMessageBox(_T("파일을 생성하는 데 실패했습니다. 파일이 다른 프로그램에서 열려있는지 확인해주세요."));
		return;
	}

	AddLog(_T("🔄 재고 목록을 CSV 파일로 내보내는 중... 경로: ") + strFilePath);

	TRY
	{
		// ---[핵심 수정] CString(유니코드)을 UTF-8로 변환하여 파일에 쓰는 람다 함수---
		auto WriteLineAsUtf8 = [&](const CString& line) {
			// CString(UTF-16) -> UTF-8로 변환
			int nLen = WideCharToMultiByte(CP_UTF8, 0, line, -1, NULL, 0, NULL, NULL);
			char* buf = new char[nLen];
			WideCharToMultiByte(CP_UTF8, 0, line, -1, buf, nLen, NULL, NULL);

			// 변환된 UTF-8 문자열을 파일에 쓴다 (NULL 종료 문자는 제외)
			file.Write(buf, nLen - 1);
			delete[] buf;
		};
	// -------------------------------------------------------------------

	// 2. UTF-8 BOM(Byte Order Mark) 쓰기: Excel에서 한글 깨짐을 방지하는 신호
	unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	file.Write(bom, 3);

	// 3. 컬럼 헤더(제목) 쓰기
	CString strHeader;
	CHeaderCtrl* pHeader = m_listInventory.GetHeaderCtrl();
	int nColumnCount = pHeader->GetItemCount();
	for (int i = 0; i < nColumnCount; ++i)
	{
		TCHAR szText[256] = { 0 };
		HDITEM hdi = { 0 };
		hdi.mask = HDI_TEXT;
		hdi.pszText = szText;
		hdi.cchTextMax = 256;
		pHeader->GetItem(i, &hdi);

		CString strCol(hdi.pszText);
		// 콤마나 큰따옴표가 포함된 경우 처리 (CSV 표준)
		strCol.Replace(_T("\""), _T("\"\""));
		strHeader += _T("\"") + strCol + _T("\"");

		if (i < nColumnCount - 1)
			strHeader += _T(",");
	}
	WriteLineAsUtf8(strHeader + _T("\n"));

	// 4. 리스트의 모든 데이터 행 쓰기
	int nRowCount = m_listInventory.GetItemCount();
	for (int i = 0; i < nRowCount; ++i)
	{
		CString strLine;
		for (int j = 0; j < nColumnCount; ++j)
		{
			CString strItem = m_listInventory.GetItemText(i, j);
			// 데이터에 콤마가 포함된 경우 큰따옴표로 감싸기
			if (strItem.Find(_T(',')) != -1 || strItem.Find(_T('\"')) != -1)
			{
				strItem.Replace(_T("\""), _T("\"\"")); // 큰따옴표는 2개로 치환
				strItem = _T("\"") + strItem + _T("\"");
			}
			strLine += strItem;

			if (j < nColumnCount - 1)
				strLine += _T(",");
		}
		WriteLineAsUtf8(strLine + _T("\n"));
	}

	file.Close();
	AddLog(_T("✅ 내보내기 완료!"));
	AfxMessageBox(_T("재고 목록을 성공적으로 내보냈습니다."));
	}
		CATCH(CFileException, e)
	{
		TCHAR szError[1024];
		e->GetErrorMessage(szError, 1024);
		AfxMessageBox(szError);
		AddLog(_T("❌ 내보내기 실패: ") + CString(szError));
	}
	END_CATCH
}

// ========================================
// 설정 다이얼로그에서 자동발주 설정 업데이트
// ========================================
void CInventoryManagerDlg::UpdateAutoOrderSettings(BOOL bEnabled, int nThreshold, int nQuantity, int nInterval)
{
	m_bAutoOrderEnabled = bEnabled;
	m_nAutoOrderThreshold = nThreshold;
	m_nAutoOrderQuantity = nQuantity;
	m_nAutoOrderInterval = nInterval;

	// config.ini에 저장
	SaveAutoOrderConfig();

	CString strLog;
	strLog.Format(_T("⚙️ 자동발주 설정 변경: %s, 기준=%d개, 수량=%d개, 주기=%d초"),
		bEnabled ? _T("활성화") : _T("비활성화"),
		nThreshold, nQuantity, nInterval);
	AddLog(strLog);

	// 타이머 재시작
	if (bEnabled)
	{
		StartAutoOrderTimer();  // 활성화 → 타이머 시작
	}
	else
	{
		StopAutoOrderTimer();   // 비활성화 → 타이머 중지
	}
}

// ========================================
// config.ini에서 자동발주 설정 로드
// ========================================
void CInventoryManagerDlg::LoadAutoOrderConfig()
{
	CString strConfigFile = GetConfigFilePath();

	// 활성화 여부
	int nEnabled = GetPrivateProfileInt(_T("AutoOrder"), _T("Enabled"), 0, strConfigFile);
	m_bAutoOrderEnabled = (nEnabled == 1);

	// 발주 기준 재고
	m_nAutoOrderThreshold = GetPrivateProfileInt(_T("AutoOrder"), _T("Threshold"), 50, strConfigFile);

	// 발주 수량
	m_nAutoOrderQuantity = GetPrivateProfileInt(_T("AutoOrder"), _T("Quantity"), 100, strConfigFile);

	// 체크 주기
	m_nAutoOrderInterval = GetPrivateProfileInt(_T("AutoOrder"), _T("Interval"), 30, strConfigFile);
}

// ========================================
// config.ini에 자동발주 설정 저장
// ========================================
void CInventoryManagerDlg::SaveAutoOrderConfig()
{
	CString strConfigFile = GetConfigFilePath();
	CString strValue;

	// 활성화 여부
	strValue.Format(_T("%d"), m_bAutoOrderEnabled ? 1 : 0);
	WritePrivateProfileString(_T("AutoOrder"), _T("Enabled"), strValue, strConfigFile);

	// 발주 기준
	strValue.Format(_T("%d"), m_nAutoOrderThreshold);
	WritePrivateProfileString(_T("AutoOrder"), _T("Threshold"), strValue, strConfigFile);

	// 발주 수량
	strValue.Format(_T("%d"), m_nAutoOrderQuantity);
	WritePrivateProfileString(_T("AutoOrder"), _T("Quantity"), strValue, strConfigFile);

	// 체크 주기
	strValue.Format(_T("%d"), m_nAutoOrderInterval);
	WritePrivateProfileString(_T("AutoOrder"), _T("Interval"), strValue, strConfigFile);
}

// ========================================
// 자동발주 체크
// ========================================
void CInventoryManagerDlg::CheckAutoOrder()
{
	// 0. [추가] 알림 끄기(Snooze) 상태인지 확인
	if (CTime::GetCurrentTime() < m_tSnoozeEndTime)
	{
		// 아직 알림 끄기 시간이 끝나지 않았으므로, 아무것도 하지 않고 함수를 종료합니다.
		return;
	}

	// 1. 자동발주가 비활성화되어 있으면 종료
	if (!m_bAutoOrderEnabled)
	{
		return;
	}

	// 2. DB 연결 확인
	if (!m_bDBConnected || m_pDBManager == nullptr)
	{
		return;
	}

	// 3. 발주 기준 이하인 품목 찾기
	std::vector<AUTO_ORDER_ITEM> vecAutoOrderItems;

	for (size_t i = 0; i < m_vecInventory.size(); i++)
	{
		const INVENTORY_ITEM& invItem = m_vecInventory[i];

		// 재고가 기준 이하인지 확인
		if (invItem.nStock <= m_nAutoOrderThreshold)
		{
			AUTO_ORDER_ITEM autoItem;
			autoItem.nOptionID = invItem.nOptionID;
			autoItem.strOptionCode = invItem.strOptionCode;
			autoItem.strProductName = invItem.strProductName;
			autoItem.nCurrentStock = invItem.nStock;
			autoItem.nOrderQuantity = m_nAutoOrderQuantity;
			autoItem.nExpectedStock = invItem.nStock + m_nAutoOrderQuantity;

			vecAutoOrderItems.push_back(autoItem);
		}
	}

	// 4. 발주할 품목이 없으면 종료
	if (vecAutoOrderItems.empty())
	{
		return;
	}

	// 5. 알림 다이얼로그 표시
	CAutoOrderNotifyDlg dlg;
	dlg.m_vecOrderItems = vecAutoOrderItems;

	INT_PTR nResult = dlg.DoModal();

	// 6. 사용자 선택에 따라 처리
	if (nResult == IDOK)
	{
		// "모두 발주 확정" 선택
		ExecuteAutoOrder(vecAutoOrderItems);
	}
	else if (nResult == IDCANCEL)
	{
		// "나중에 확인" 선택
		AddLog(_T("⏰ 자동발주를 나중에 확인하기로 했습니다."));
	}
	else if (nResult == IDIGNORE)
	{
		// "무시" 선택 -> 1시간 동안 알림 끄기
		m_tSnoozeEndTime = CTime::GetCurrentTime() + CTimeSpan(0, 1, 0, 0); // 1시간 후
		CString strLog;
		strLog.Format(_T("🚫 자동발주 알림을 1시간 동안 끄기 설정했습니다. (종료: %s)"),
			m_tSnoozeEndTime.Format(_T("%H:%M:%S")));
		AddLog(strLog);
	}
}

// ========================================
// 자동발주 실행
// ========================================
void CInventoryManagerDlg::ExecuteAutoOrder(const std::vector<AUTO_ORDER_ITEM>& vecItems)
{
	if (vecItems.empty())
	{
		return;
	}

	CString strLog;
	strLog.Format(_T("📦 자동발주 실행 중... (총 %d개 품목)"), (int)vecItems.size());
	AddLog(strLog);

	int nSuccessCount = 0;
	int nFailCount = 0;

	// 각 품목에 대해 발주 실행
	for (size_t i = 0; i < vecItems.size(); i++)
	{
		const AUTO_ORDER_ITEM& item = vecItems[i];

		// DBManager를 통해 재고 추가
		BOOL bResult = m_pDBManager->AddStock(item.nOptionID, item.nOrderQuantity);

		if (bResult)
		{
			nSuccessCount++;
			strLog.Format(_T("  ✅ %s: %d개 발주 성공"),
				item.strOptionCode, item.nOrderQuantity);
			AddLog(strLog);
		}
		else
		{
			nFailCount++;
			strLog.Format(_T("  ❌ %s: 발주 실패 - %s"),
				item.strOptionCode, m_pDBManager->GetLastError());
			AddLog(strLog);
		}
	}

	// 결과 요약
	strLog.Format(_T("📊 자동발주 완료: 성공 %d건, 실패 %d건"), nSuccessCount, nFailCount);
	AddLog(strLog);

	// 성공한 건이 있으면 화면 새로고침
	if (nSuccessCount > 0)
	{
		RefreshInventoryData();
		AddLog(_T("🔄 재고 목록을 새로고침했습니다."));
	}

	// 완료 메시지
	CString strMsg;
	strMsg.Format(_T("자동발주가 완료되었습니다.\n\n성공: %d건\n실패: %d건"),
		nSuccessCount, nFailCount);
	AfxMessageBox(strMsg, MB_ICONINFORMATION);
}

// ========================================
// 자동발주 타이머 시작
// ========================================
void CInventoryManagerDlg::StartAutoOrderTimer()
{
	// 기존 타이머가 있으면 먼저 중지
	StopAutoOrderTimer();

	// 타이머 ID: 2번 사용 (1번은 기존 새로고침용)
	// 주기: m_nAutoOrderInterval초 (밀리초로 변환)
	m_nAutoOrderTimerID = SetTimer(2, m_nAutoOrderInterval * 1000, nullptr);

	if (m_nAutoOrderTimerID != 0)
	{
		CString strLog;
		strLog.Format(_T("⏰ 자동발주 타이머 시작 (주기: %d초)"), m_nAutoOrderInterval);
		AddLog(strLog);
	}
	else
	{
		AddLog(_T("❌ 자동발주 타이머 시작 실패"));
	}
}

// ========================================
// 자동발주 타이머 중지
// ========================================
void CInventoryManagerDlg::StopAutoOrderTimer()
{
	if (m_nAutoOrderTimerID != 0)
	{
		KillTimer(m_nAutoOrderTimerID);
		m_nAutoOrderTimerID = 0;
		AddLog(_T("⏹️ 자동발주 타이머 중지"));
	}
}

// ========================================
// 타이머 이벤트 처리
// ========================================
void CInventoryManagerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 2)  // 자동발주 타이머 (ID=2)
	{
		// 자동발주 체크 실행
		CheckAutoOrder();
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CInventoryManagerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_nTimerID != 0) { KillTimer(m_nTimerID); m_nTimerID = 0; } // 타이머가 실행 중이면 종료
	if (m_nAutoOrderTimerID != 0) { KillTimer(m_nAutoOrderTimerID); m_nAutoOrderTimerID = 0; } // 자동발주 타이머 종료
}


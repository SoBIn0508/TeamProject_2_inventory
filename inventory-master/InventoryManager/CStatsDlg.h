// === CStatsDlg.h (PATCH) ===
#pragma once
#include <vector>
#include "resource.h"
#include <afxcmn.h>

class CDBManager;

class CStatsDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CStatsDlg)

public:
    CStatsDlg(CWnd* pParent = nullptr);
    virtual ~CStatsDlg();

    void InitDB(CDBManager* pDB, BOOL bConnected) { m_pDB = pDB; m_bDBConnected = bConnected; }
    void Reload();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_STATS_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()

    // ===== UI =====
    CListCtrl m_listOrders;
    CListCtrl m_listBrand;
    CListCtrl m_listDaily;          // [ADD] 날짜별 총매출 리스트
    CDateTimeCtrl m_dtpStart;       // 시작일 DateTimePicker
    CDateTimeCtrl m_dtpEnd;         // 종료일 DateTimePicker

    // ===== 헬퍼 =====
    void AutoSizeColumns(CListCtrl& list);

    // ===== DB =====
    CDBManager* m_pDB = nullptr;
    BOOL        m_bDBConnected = FALSE;

    // ===== 내부 =====
    void InitLists();
    void LoadStats();

    void LoadStatsByPeriod(const CString& strStartDate, const CString& strEndDate);
    void LoadBrandStatsByPeriod(const CString& strStartDate, const CString& strEndDate);
    void LoadDailyStatsByPeriod(const CString& strStartDate, const CString& strEndDate);

    afx_msg void OnClose();
    virtual void OnCancel();
    virtual BOOL DestroyWindow();

    afx_msg void OnBnClickedButtonSearchPeriod();
};

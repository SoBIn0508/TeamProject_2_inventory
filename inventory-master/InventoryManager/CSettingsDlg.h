// CSettingsDlg.h : 헤더 파일
//

#pragma once
#include "afxdialogex.h"
#include "DBManager.h" // ✅ [추가] DB_CONFIG 구조체를 사용하기 위해 include

// CInventoryManagerDlg 전방 선언
class CInventoryManagerDlg;

// CSettingsDlg 대화 상자
class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CSettingsDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
private:
	int m_nWarningThreshold;
	int m_nDangerThreshold;
	CInventoryManagerDlg* m_pParentDlg;
public:
	void SetParentDlg(CInventoryManagerDlg* pDlg);
	afx_msg void OnBnClickedButtonApply();
	void LoadSettings(int nWarning, int nDanger);

	// ✅ [추가] DB 설정 UI와 연결될 변수들
	CString m_strDbHost;
	UINT    m_nDbPort;
	CString m_strDbName;
	CString m_strDbUser;
	CString m_strDbPass;

	// ✅ [추가] 메인 다이얼로그의 DB 설정을 불러오는 함수 선언
	void LoadDbSettings(const DB_CONFIG& dbConfig);
	// ✅ [추가] DB 설정 저장 버튼 핸들러 선언
	afx_msg void OnBnClickedButtonSaveDb();
	BOOL m_bAutoOrderEnable;
	int m_nAutoOrderThreshold;
	int m_nAutoOrderQuantity;
	int m_nAutoOrderInterval;
	afx_msg void OnBnClickedButtonAutoOrderApply();

	void SaveAutoOrderConfig();       // config.ini에 자동발주 설정 저장
	void LoadAutoOrderConfig();       // config.ini에서 자동발주 설정 로드
};
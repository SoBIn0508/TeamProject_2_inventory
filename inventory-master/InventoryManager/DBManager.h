#pragma once

#include <mysql.h>
#include <vector>

// ========================================
// 데이터베이스 연결 정보 구조체
// ========================================
struct DB_CONFIG
{
	CString strHost;        // 호스트 (예: localhost, 192.168.1.100)
	int nPort;              // 포트 (기본: 3306)
	CString strDatabase;    // 데이터베이스 이름
	CString strUser;        // 사용자 이름
	CString strPassword;    // 비밀번호

	// 기본값 설정
	DB_CONFIG()
	{
		strHost = _T("192.168.0.92");
		nPort = 3306;
		strDatabase = _T("themost_db");
		strUser = _T("mfcuser");
		strPassword = _T("Moble1234");
	}
};

// ========================================
// 재고 데이터 구조체
// ========================================
struct INVENTORY_ITEM
{
	CString strOptionCode;      // 옵션 코드 (예: SP-TS-001-BK-L)
	CString strProductName;     // 상품명
	CString strBrandName;       // 브랜드명
	CString strCategoryName;	// 카테고리명
	CString strColorName;       // 색상명
	CString strSizeName;        // 사이즈명
	int nStock;                 // 재고 수량
	int nOptionID;              // 옵션 ID (PK)
	int nProductID;             // 상품 ID

	INVENTORY_ITEM()
	{
		nStock = 0;
		nOptionID = 0;
		nProductID = 0;
	}
};

// ========================================
// CDBManager 클래스 (싱글톤)
// ========================================
class CDBManager
{
private:
	// ========================================
	// 싱글톤 관련
	// ========================================
	static CDBManager* m_pInstance;     // 싱글톤 인스턴스
	CDBManager();                       // private 생성자
	~CDBManager();                      // private 소멸자

	// ========================================
	// MySQL 연결 객체
	// ========================================
	MYSQL* m_pConnection;               // MySQL 연결 핸들
	MYSQL_RES* m_pResult;               // 쿼리 결과

	// ========================================
	// 연결 정보
	// ========================================
	DB_CONFIG m_config;                 // DB 설정
	BOOL m_bConnected;                  // 연결 상태

	// ========================================
	// 에러 정보
	// ========================================
	CString m_strLastError;             // 마지막 에러 메시지
	int m_nLastErrorCode;               // 마지막 에러 코드

public:
	// ========================================
	// 싱글톤 인스턴스 접근
	// ========================================
	static CDBManager* GetInstance();
	static void DestroyInstance();
	BOOL SelectToRows(const CString& sql, std::vector<std::vector<CString>>& rows);
	// ========================================
	// 연결 관리
	// ========================================
	BOOL Connect(const DB_CONFIG& config);  // DB 연결
	void Disconnect();                      // DB 연결 해제
	BOOL IsConnected() const;               // 연결 상태 확인
	BOOL Reconnect();                       // 재연결

	// ========================================
	// 쿼리 실행
	// ========================================
	BOOL ExecuteQuery(const CString& strQuery);  // 일반 쿼리 (INSERT, UPDATE, DELETE)
	BOOL ExecuteSelect(const CString& strQuery); // SELECT 쿼리

	// ========================================
	// 결과 가져오기
	// ========================================
	MYSQL_ROW FetchRow();                   // 한 행 가져오기
	int GetRowCount();                      // 결과 행 수
	void FreeResult();                      // 결과 해제

	// ========================================
	// 재고 데이터 조회
	// ========================================
	BOOL GetInventoryList(std::vector<INVENTORY_ITEM>& vecItems);  // 전체 재고 조회
	BOOL UpdateStock(int nOptionID, int nNewStock);                // 재고 수정
	BOOL AddStock(int nOptionID, int nQuantity);                   // 재고 추가 (발주)
	BOOL AddStockToItems(const std::vector<int>& vecOptionIDs, int nQuantity); // 일괄 발주

	// ========================================
	// [변경] 상품 옵션 삭제 및 상품 정리
	// ========================================
	BOOL DeleteOptionsAndCleanup(const std::vector<int>& vecOptionIDs);

	// ========================================
	// 에러 처리
	// ========================================
	CString GetLastError() const;           // 마지막 에러 메시지
	int GetLastErrorCode() const;           // 마지막 에러 코드

	// ========================================
	// 유틸리티
	// ========================================
	CString EscapeString(const CString& strInput);  // SQL Injection 방지

	//======================================
	//품목 추가
	//======================================
	CString GetBrandCodeFromName(const CString& strName);
	CString GetCategoryCodeFromName(const CString& strName); // '종류'를 위한 함수
	CString GetColorCodeFromName(const CString& strName);
	BOOL GetBrandList(std::vector<CString>& vecBrands);
	BOOL GetColorList(std::vector<CString>& vecColors);
	BOOL GetSizeList(std::vector<CString>& vecSizes);

	BOOL AddProduct(
		const CString& strBrandName,
		const CString& strCategoryName,
		const CString& strProductName,
		int nPrice,
		const CString& strDescription,
		const CString& strImagesJson,
		const CString& strOptionsJson
	);

	BOOL GetCategoryList(std::vector<CString>& vecCategories);

private:
	// ========================================
	// 내부 함수
	// ========================================
	void SetError(const CString& strError, int nCode = 0);  // 에러 설정
	CString ConvertToUTF8(const CString& strInput);         // UTF-8 변환
	CString ConvertFromUTF8(const char* szInput);           // UTF-8에서 변환


};

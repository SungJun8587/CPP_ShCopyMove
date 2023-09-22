
//***************************************************************************
// ShCopyMove.cpp : Defines the entry point for the console application.
//
//***************************************************************************

#include "pch.h"

//***************************************************************************
//
BOOL SHDirectoryRecursive(const TCHAR* ptszSourceFolder, const TCHAR* ptszDestFolder, SH_APPLY_FILEINFO& ShApplyFileInfo, CBaseDoublyLinkedList<SH_FILESYSTEM_INFO*>& MemBufferFrom, CBaseDoublyLinkedList<SH_FILESYSTEM_INFO*>& MemBufferTo)
{
	BOOL		bResult = true;
	TCHAR		tszActiveFolder[DIRECTORY_STRLEN + 16];
	TCHAR		tszSourceFullPath[FULLPATH_STRLEN];
	TCHAR		tszSourceFolder[DIRECTORY_STRLEN];
	TCHAR		tszDestFullPath[FULLPATH_STRLEN];
	TCHAR		tszDestFolder[DIRECTORY_STRLEN];

	WIN32_FIND_DATA		FindData;
	HANDLE				hFindFile;

	SH_FILESYSTEM_INFO* pFileFrom = NULL;
	SH_FILESYSTEM_INFO* pFileTo = NULL;

	if( !ptszSourceFolder || !ptszDestFolder ) return false;
	if( _tcslen(ptszSourceFolder) < 1 || _tcslen(ptszDestFolder) < 1 ) return false;

	if( ptszSourceFolder[_tcslen(ptszSourceFolder) - 1] != '/' && ptszSourceFolder[_tcslen(ptszSourceFolder) - 1] != '\\' )
	{
		StringCchPrintf(tszSourceFolder, _countof(tszSourceFolder), _T("%s\\"), ptszSourceFolder);
		StringCchPrintf(tszActiveFolder, _countof(tszActiveFolder), _T("%s\\*.*"), ptszSourceFolder);
	}
	else
	{
		StringCchPrintf(tszSourceFolder, _countof(tszSourceFolder), _T("%s"), ptszSourceFolder);
		StringCchPrintf(tszActiveFolder, _countof(tszActiveFolder), _T("%s*.*"), ptszSourceFolder);
	}

	if( ptszDestFolder[_tcslen(ptszDestFolder) - 1] != '/' && ptszDestFolder[_tcslen(ptszDestFolder) - 1] != '\\' )
		StringCchPrintf(tszDestFolder, _countof(tszDestFolder), _T("%s\\"), ptszDestFolder);
	else
		StringCchPrintf(tszDestFolder, _countof(tszDestFolder), _T("%s"), ptszDestFolder);

	hFindFile = FindFirstFile(tszActiveFolder, &FindData);

	// Check if sub folders exists.
	if( INVALID_HANDLE_VALUE != hFindFile )
	{	// There are sub-folders.
		while( bResult )
		{
			if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if( _tcscmp(FindData.cFileName, _T(".")) != 0 && _tcscmp(FindData.cFileName, _T("..")) != 0 )
				{
					StringCchPrintf(tszSourceFullPath, _countof(tszSourceFullPath), _T("%s%s"), tszSourceFolder, FindData.cFileName);
					StringCchPrintf(tszDestFullPath, _countof(tszDestFullPath), _T("%s%s"), tszDestFolder, FindData.cFileName);

					SHDirectoryRecursive(tszSourceFullPath, tszDestFullPath, ShApplyFileInfo, MemBufferFrom, MemBufferTo);
				}
			}
			else
			{
				StringCchPrintf(tszSourceFullPath, _countof(tszSourceFullPath), _T("%s%s"), tszSourceFolder, FindData.cFileName);
				StringCchPrintf(tszDestFullPath, _countof(tszDestFullPath), _T("%s%s"), tszDestFolder, FindData.cFileName);

				if( IsAbleFile(tszSourceFullPath, ShApplyFileInfo) )
				{
					pFileFrom = new SH_FILESYSTEM_INFO;

					StringCchCopy(pFileFrom->m_tszFullPath, _countof(pFileFrom->m_tszFullPath), tszSourceFullPath);
					StringCchCopy(pFileFrom->m_tszFolder, _countof(pFileFrom->m_tszFolder), tszSourceFolder);
					StringCchCopy(pFileFrom->m_tszFileNameExt, _countof(pFileFrom->m_tszFileNameExt), FindData.cFileName);

					MemBufferFrom.Add(pFileFrom);

					pFileTo = new SH_FILESYSTEM_INFO;

					StringCchCopy(pFileTo->m_tszFullPath, _countof(pFileTo->m_tszFullPath), tszDestFullPath);
					StringCchCopy(pFileTo->m_tszFolder, _countof(pFileTo->m_tszFolder), tszDestFolder);
					StringCchCopy(pFileTo->m_tszFileNameExt, _countof(pFileTo->m_tszFileNameExt), FindData.cFileName);

					MemBufferTo.Add(pFileTo);
				}
			}

			bResult = FindNextFile(hFindFile, &FindData);
		}
	}

	FindClose(hFindFile);
	bResult = true;

	return bResult;
}

//***************************************************************************
//
int main(int argc, TCHAR* argv[])
{
	BOOL	bFlag = false;
	int		nFunc = 0;
	TCHAR	tszProgressTitle[32];
	TCHAR	tszSrcFullPath[FULLPATH_STRLEN];
	TCHAR	tszDestFullPath[FULLPATH_STRLEN];

	TCHAR* pszFrom = NULL;
	TCHAR* pszTo = NULL;

	SH_FILESYSTEM_INFO* pFileFrom = NULL;
	SH_FILESYSTEM_INFO* pFileTo = NULL;

	MEMORY_CHAR_BUFFER	CharBufferFrom;
	MEMORY_CHAR_BUFFER	CharBufferTo;

	SH_APPLY_FILEINFO	ShApplyFileInfo;

	CMemBuffer<TCHAR>	TSrcFullPath;
	CMemBuffer<TCHAR>	TDestFullPath;

	SHFILEOPSTRUCT		ShFile;

	CBaseDoublyLinkedList<SH_FILESYSTEM_INFO*>	MemBufferFrom;
	CBaseDoublyLinkedList<SH_FILESYSTEM_INFO*>	MemBufferTo;

	memset(&ShApplyFileInfo, 0, sizeof(ShApplyFileInfo));

	if( argc < 5 )
	{
		printf("알림 : 잘못된 요청입니다.");
		return 1;
	}

	if( argc > 1 )
	{
		if( argv[1][0] == 'M' )
		{
			StringCchCopy(tszProgressTitle, _countof(tszProgressTitle), _T("파일 이동"));
			nFunc = FO_MOVE;
		}
		else
		{
			StringCchCopy(tszProgressTitle, _countof(tszProgressTitle), _T("파일 복사"));
			nFunc = FO_COPY;
		}
	}
	if( argc > 2 ) StringCchCopy(tszSrcFullPath, _countof(tszSrcFullPath), argv[2]);		// 대상(원본) 폴더
	if( argc > 3 ) StringCchCopy(tszDestFullPath, _countof(tszDestFullPath), argv[3]);		// 복사/이동할 목적지 폴더

	// m_bIsApply = true이면 m_tszApplyExt에 포함된 확장자만 적용
	// m_bIsApply = false이면 m_tszApplyExt에 포함된 확장자을 제외
	if( argc > 4 )
	{
		if( argv[4][0] == '1' )
			ShApplyFileInfo.m_bIsApply = true;
		else ShApplyFileInfo.m_bIsApply = false;
	}
	if( argc > 5 ) StringCchCopy(ShApplyFileInfo.m_tszApplyExt, _countof(ShApplyFileInfo.m_tszApplyExt), argv[5]);				// 적용할 확장자(Ex. ".asp;.htm;.html")

	// 수정 일시가 시작 일시(m_tszModifyStDate), 종료 일시(m_tszModifyEdDate) 사이에 있는 것만 적용
	if( argc > 6 ) StringCchCopy(ShApplyFileInfo.m_tszModifyStDate, _countof(ShApplyFileInfo.m_tszModifyStDate), argv[6]);		// 수정 일시을 기준으로 시작 일시 
	if( argc > 7 ) StringCchCopy(ShApplyFileInfo.m_tszModifyEdDate, _countof(ShApplyFileInfo.m_tszModifyEdDate), argv[7]);		// 수정 일시을 기준으로 종료 일시

	StrReplace(TSrcFullPath, tszSrcFullPath, _T(";32;"), _T(" "));
	StrReplace(TDestFullPath, tszDestFullPath, _T(";32;"), _T(" "));

	SHDirectoryRecursive(TSrcFullPath.GetBuffer(), TDestFullPath.GetBuffer(), ShApplyFileInfo, MemBufferFrom, MemBufferTo);

	MemBufferCreate(&CharBufferFrom);
	for( int i = 0; i < MemBufferFrom.GetCount(); i++ )
	{
		pFileFrom = MemBufferFrom.At(i);
		if( pFileFrom ) MemBufferAddBuffer(&CharBufferFrom, pFileFrom->m_tszFullPath, _tcslen(pFileFrom->m_tszFullPath) + 1);

		if( pFileFrom )
		{
			delete pFileFrom;
			pFileFrom = NULL;
		}
	}
	pszFrom = CharBufferFrom.m_ptszBuffer;
	*(CharBufferFrom.m_ptszPosition) = 0;

	MemBufferCreate(&CharBufferTo);
	for( int i = 0; i < MemBufferTo.GetCount(); i++ )
	{
		pFileTo = MemBufferTo.At(i);
		if( pFileTo ) MemBufferAddBuffer(&CharBufferTo, pFileTo->m_tszFullPath, _tcslen(pFileTo->m_tszFullPath) + 1);

		if( pFileTo )
		{
			delete pFileTo;
			pFileTo = NULL;
		}
	}
	pszTo = CharBufferTo.m_ptszBuffer;
	*(CharBufferTo.m_ptszPosition) = 0;

	bFlag = true;
	if( nFunc == FO_COPY )
	{
		if( !pszFrom || _tcslen(pszFrom) < 1 )
		{
			bFlag = false;
			printf("\r\n알림 : 복사할 파일이 존재하지 않습니다.\r\n\r\n\r\n");
		}
	}
	else
	{
		if( !pszFrom || _tcslen(pszFrom) < 1 )
		{
			bFlag = false;
			printf("\r\n알림 : 이동할 파일이 존재하지 않습니다.\r\n\r\n\r\n");
		}
	}

	if( bFlag )
	{
		ShFile.hwnd = NULL;
		ShFile.pFrom = pszFrom;
		ShFile.pTo = pszTo;
		ShFile.wFunc = nFunc;
		ShFile.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION;
		ShFile.lpszProgressTitle = tszProgressTitle;

		SHFileOperation(&ShFile);
	}

	if( pszFrom )
	{
		delete pszFrom;
		pszFrom = NULL;
	}

	if( pszTo )
	{
		delete pszTo;
		pszTo = NULL;
	}

	system("pause");

	return 0;
}
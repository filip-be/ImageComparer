#include "pHashWrapper.h"
#include <Shlwapi.h>

/// <summary>Pobranie pe³nej œcie¿ki katalogu</summary>
/// <param name="_dirPath">katalog</param>
/// <param name="_bMask">maska katalogu</param>
/// <param name="IsInputDirectory">blokowanie nazw katalogów wejœciowych</param>
CStringW GetDirectory(CStringW _dirPath, CStringW *_bMask, bool IsInputDirectory)
{
	wchar_t path[_MAX_PATH];
	wchar_t drive[_MAX_DRIVE];
	wchar_t dir[_MAX_DIR];
	wchar_t fname[_MAX_FNAME];
	wchar_t ext[_MAX_EXT];
	CStringW strDir;

	_wsplitpath_s(_dirPath, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

	// Czy podano maske
	bool bMask = false;
	CStringW strMask = fname;
	strMask += ext;
	if (strMask.Find(L".") != -1)
	{
		bMask = true;
		// Sprawdzamy czy to katalog
		_wfinddata_t fileDesc;
		const int handle = _wfindfirst(strMask, &fileDesc);
		if (handle != -1)
		{
			if (fileDesc.attrib&_A_SUBDIR)
				bMask = false;
			_findclose(handle);
		}
	}
	// Czy podano pelna sciezke?
	strDir = drive;
	if (strDir.IsEmpty())
	{
		::GetCurrentDirectoryW(_MAX_PATH, path);
		strDir = path;
		if (strDir[strDir.GetLength() - 1] != '\\')
			strDir += _T("\\");
		strDir += dir;
	}
	else
	{ // Podano sciezke wzgledna
		strDir += dir;
	}
	if (!bMask)
	{
		if (strDir[strDir.GetLength() - 1] != '\\')
			strDir += _T("\\");
		strDir += strMask;
	}
	// Usuniecie ostatniego slasha
	if (strDir[strDir.GetLength() - 1] == '\\')
		strDir = strDir.Left(strDir.GetLength() - 1);
	// Wyluskanie ostatniego katalogu
	CStringW strToken;
	CStringW strLastToken;
	int pos = 0;
	strToken = strDir.Tokenize(L"\\", pos);
	strLastToken = strToken;
	while (strToken != L"")
	{
		strLastToken = strToken;
		strToken = strDir.Tokenize(L"\\", pos);
	};
	if (IsInputDirectory &&
		((strLastToken.CompareNoCase(L"OK") == 0) ||
		(strLastToken.CompareNoCase(L"BAD") == 0) ||
		(strLastToken.CompareNoCase(L"MAN") == 0) || (strLastToken.CompareNoCase(L"BAD_70") == 0) ||
		(strLastToken.CompareNoCase(L"BAD_95") == 0)))
	{
		wprintf(L"Not allowed directory name !!!\n\n");
		system("PAUSE");
		return "";
	}
	_wsplitpath_s(strDir, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

	strDir = _T("\\\\?\\");
	strDir += drive;
	strDir += dir;
	strDir += fname;
	strDir += ext;
	if (strDir[strDir.GetLength() - 1] != '\\')
		strDir += _T("\\");

	if (!bMask)
		strMask = _T("*.*");

	if (_bMask != NULL)
		*_bMask = strMask;

	return strDir;
}

/// <summary>Rekurencyjne wyszukanie plikow</summary>
/// <param name="_fv"> wektor, do którego zapisywane s¹ œcie¿ki znalezionych plików</param>
/// <param name="strDir"> przeszukiwany katalog</param>
/// <param name="strMask"> maska przeszukiwania</param>
/// <param name="strDirIn"> przedrostek nazw katalogów, który jest pomijany podczas zapisu do mapy</param>
/// <param name="_DirMap"> opcjonalny wskaŸnik na mapê struktur w której zapisywane s¹ daty katalogów</param>
/// <param name="strExt"> opcjonalna lista rozszerzeñ, które maj¹ byæ sprawdzane</param>
void SearchFiles(FileVector &_fv, CStringW strDir, const CStringW &strMask,
	const CStringW &strDirIn, const CStringW &strExt/* = ""*/)
{
	// Find matching files
	_wfinddata_t fileDesc;
	const int handle = _wfindfirst(strDir + strMask, &fileDesc);
	if (handle == -1)
		return;
	do
	{
		bool bDotSubdir = false;
		if (fileDesc.name[0] == '.')
		{
			if (wcslen(fileDesc.name) == 1)
				bDotSubdir = true;
			if ((fileDesc.name[1] == '.') && (wcslen(fileDesc.name) == 2))
				bDotSubdir = true;
		}
		if (!bDotSubdir)
		{
			if (_A_SUBDIR&fileDesc.attrib)
			{
				// Rekurencyjne wyszukiwanie plików i katalogów
				SearchFiles(_fv, strDir + fileDesc.name + "\\", strMask, strDirIn, strExt);
			}
			else
			{
				if (!strExt.IsEmpty())
				{
					CStringW strFileExt = PathFindExtensionW(fileDesc.name);
					if (!strFileExt.IsEmpty())
					{
						strFileExt += ";";
						if (strExt.Find(strFileExt.MakeLower()) != -1)
							_fv.push_back(strDir + fileDesc.name);
					}
				}
				else
					_fv.push_back(strDir + fileDesc.name);
			}
		}
	} while (_wfindnext(handle, &fileDesc) == 0);
	_findclose(handle);
}

/// <summary>Przeniesienie pliku</summary>
bool FileMove(CString strSrc, CString strDst)
{
	TRY
	{
		CString pom;
		CString strNewName = strDst.Right(strDst.GetLength() - 7);
		CString strDirectory = strDst.Left(6);
		// Utworzenie wszystkich podkatalogow
		pom.Empty();
		for (int i = 0; i<strNewName.GetLength(); i++)
		{
			if (strNewName[i] != '\\')
				pom += strNewName[i];
			else
			{
				strDirectory = strDirectory + "\\" + pom;
				if (::CreateDirectory(strDirectory, NULL) == FALSE)
				{
					if (::GetLastError() != ERROR_ALREADY_EXISTS)
						return false;
				}
				pom.Empty();
			}
		}
		CFile::Rename(strSrc, strDst);
	}
		CATCH(CFileException, e)
	{
		return false;
	}
	CATCH_ALL(e)
	{
		e->Delete();
		return false;
	}
	END_CATCH_ALL
		return true;
}

/// <summary>Analiza katalogu</summary>
bool AnalyzeDirectory(CStringW strDir, MVPTree *mTree)
{
	return false;
}
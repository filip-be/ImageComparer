// Tst.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <afx.h>
#include <Windows.h>
#include <Shlwapi.h>

#include <pHash.h>

typedef	vector< CString >	FileVector;
FileVector	m_fv;
/////////////////////////////////////////////////////////////////////////////
// Pobranie œcie¿ki katalogu
/////////////////////////////////////////////////////////////////////////////
CString GetDirectory(CString _dirPath, CString *_bMask, bool IsInputDirectory)
{
	_TCHAR path[_MAX_PATH];
	_TCHAR drive[_MAX_DRIVE];
	_TCHAR dir[_MAX_DIR];
	_TCHAR fname[_MAX_FNAME];
	_TCHAR ext[_MAX_EXT];
	CString strDir;

	_wsplitpath_s(_dirPath, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

	// Czy podano maske
	bool bMask = false;
	CString strMask = fname;
	strMask += ext;
	if (strMask.Find(_T(".")) != -1)
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
		::GetCurrentDirectory(_MAX_PATH, path);
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
	CString strToken;
	CString strLastToken;
	int pos = 0;
	strToken = strDir.Tokenize(_T("\\"), pos);
	strLastToken = strToken;
	while (strToken != _T(""))
	{
		strLastToken = strToken;
		strToken = strDir.Tokenize(_T("\\"), pos);
	};
	if (IsInputDirectory &&
		((strLastToken.CompareNoCase(_T("OK")) == 0) ||
		(strLastToken.CompareNoCase(_T("BAD")) == 0) ||
		(strLastToken.CompareNoCase(_T("MAN")) == 0) || (strLastToken.CompareNoCase(_T("BAD_70")) == 0) ||
		(strLastToken.CompareNoCase(_T("BAD_95")) == 0)))
	{
		wprintf(_T("Not allowed directory name !!!\n\n"));
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
/////////////////////////////////////////////////////////////////////////////////////////
// Rekurencyjne wyszukanie plikow
//
// _fv		- wektor, do którego zapisywane s¹ œcie¿ki znalezionych plików
// strDir	- przeszukiwany katalog
// strMask	- maska przeszukiwania
// strDirIn	- przedrostek nazw katalogów, który jest pomijany podczas zapisu do mapy
// _DirMap	- opcjonalny wskaŸnik na mapê struktur w której zapisywane s¹ daty katalogów
// strExt	- opcjonalna lista rozszerzeñ, które maj¹ byæ sprawdzane
/////////////////////////////////////////////////////////////////////////////////////////
void SearchFiles(FileVector &_fv, CString strDir, const CString &strMask,
	const CString &strDirIn, const CString &strExt = "")
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
				SearchFiles(_fv, strDir + fileDesc.name + _T("\\"), strMask, strDirIn, strExt);
			}
			else
			{
				if (!strExt.IsEmpty())
				{
					CString strFileExt = PathFindExtension(fileDesc.name);
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
/////////////////////////////////////////////////////////////////////////////////////////
//Przeniesienie pliku
/////////////////////////////////////////////////////////////////////////////////////////
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


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 3)
	{
		wchar_t n1[10000];
		wchar_t n2[10000];
		char n1A[10000];
		char n2A[10000];
		memset(n1, 0x00, 10000);
		memset(n2, 0x00, 10000);

		if (GetShortPathName(argv[1], n1, 10000) != 0 && GetShortPathName(argv[2], n2, 10000) != 0)
		{
			wcstombs_s(NULL, n1A, n1, 10000);
			wcstombs_s(NULL, n2A, n2, 10000);
			ulong64 h1, h2;
			
			int r2 = ph_dct_imagehash(n1A, h1);
			r2 = ph_dct_imagehash(n2A, h2);
			int hamm = ph_hamming_distance(h1, h2);
			
			Digest d1, d2;
			int res = ph_image_digest(n1A, 1.0, 1.0, d1, 180);
			res = ph_image_digest(n2A, 1.0, 1.0, d2, 180);

			double pcc, pcc2;
			res = ph_crosscorr(d1, d2, pcc);

			res = ph_compare_images(n1A, n2A, pcc2);
			int x = 0;
			x = x + 1;

		}
	}
	return 0;
}


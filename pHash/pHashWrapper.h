
#ifndef _PHASH_WRAPPER_H
#define _PHASH_WRAPPER_H

#include "pHash.h"
#include <vector>
#include <list>
//#include "mvptree.h"

#define _MAX_BUFFER 20000000

class __declspec(dllexport) CImageFile
{
public:
	CStringW			fName;
	LARGE_INTEGER		fSize;
	unsigned long int	fCRC;
	FILETIME			fDate;
	int iWidth;
	int iHeight;
	ulong64 iHash[4];

	static bool ImageCanBeRotated;

	CImageFile();
	CImageFile(CStringW file);
	bool Initialize(CStringW file);
	bool IsSimiliar(const CImageFile &obj, const double &eQuality);
};

__declspec(dllexport) bool GetShortPathNameANSI(wchar_t *unicodestr, int lenW, char **ansistr);

typedef void(*ProgressUpdateCallback)(const __int64&, const __int64&);


__declspec(dllexport) typedef	vector< CStringW >FileVector;

/// <summary>Pobranie pe³nej œcie¿ki katalogu</summary>
/// <param name="_dirPath">katalog</param>
/// <param name="_bMask">maska katalogu</param>
/// <param name="IsInputDirectory">blokowanie nazw katalogów wejœciowych</param>
__declspec(dllexport) CStringW GetDirectory(CStringW _dirPath, bool IsInputDirectory);

/// <summary>Rekurencyjne wyszukanie plikow</summary>
/// <param name="_fv"> wektor, do którego zapisywane s¹ œcie¿ki znalezionych plików</param>
/// <param name="strDir"> przeszukiwany katalog</param>
/// <param name="strMask"> maska przeszukiwania</param>
/// <param name="strDirIn"> przedrostek nazw katalogów, który jest pomijany podczas zapisu do mapy</param>
/// <param name="strExt"> opcjonalna lista rozszerzeñ, które maj¹ byæ sprawdzane</param>
__declspec(dllexport) void SearchFiles(FileVector &_fv, CStringW strDir, const CStringW &strMask,
	const CStringW &strDirIn, const CStringW &strExt = "");

/// <summary>Przeniesienie pliku</summary>
__declspec(dllexport) bool FileMove(CStringW strSrc, CStringW strDst);

/// <summary>Analiza katalogu</summary>
__declspec(dllexport) bool AnalyzeDirectory(CStringW strDir, CStringW strMask, const CStringW &strExt,
	std::list<CImageFile> &imageList, ProgressUpdateCallback ProgressUpdate);

#endif
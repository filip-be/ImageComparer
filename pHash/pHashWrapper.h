
#ifndef _PHASH_WRAPPER_H
#define _PHASH_WRAPPER_H

#include "pHash.h"
#include <vector>
#include <list>
#include <boost/container/map.hpp> // for boost::containter::map
//#include "mvptree.h"

#define _MAX_BUFFER 20000000

class __declspec(dllexport) CImageFile
{
public:
	CStringW			fName;
	LARGE_INTEGER		fSize;
	unsigned int		fCRC;
	__time64_t			fDate;
	int					iWidth;
	int					iHeight;
	ulong64				iHash[4];

	bool				hasExif;
	long				eWidth;
	long				eLength;
	CStringW			eMakeModel;
	__time64_t			eDateTimeOriginal;

	bool				ImageCanBeRotated;

	CImageFile();
	CImageFile(CStringW file, bool _ImageCanBeRotated, bool _CalculateHash);
	bool ReadExif();
	bool CalculateHash();
	bool CalculateCRC();
	bool Initialize(CStringW file, bool _ImageCanBeRotated, bool _CalculateHash);
	bool IsSimiliar(const CImageFile &obj, const double &eQuality);
};

__time64_t filetime_to_timet(FILETIME const& ft);

__declspec(dllexport) bool GetShortPathNameANSI(wchar_t *unicodestr, int lenW, char **ansistr, bool removeUnicodeAddon=false);

typedef void(*ProgressUpdateCallback)(const __int64&, const __int64&);

__declspec(dllexport) typedef	vector< CStringW >FileVector;

/// <summary>Pobranie pe�nej �cie�ki katalogu</summary>
/// <param name="_dirPath">katalog</param>
/// <param name="_bMask">maska katalogu</param>
/// <param name="IsInputDirectory">blokowanie nazw katalog�w wej�ciowych</param>
__declspec(dllexport) CStringW GetDirectory(CStringW _dirPath, bool IsInputDirectory);

/// <summary>Rekurencyjne wyszukanie plikow</summary>
/// <param name="_fv"> wektor, do kt�rego zapisywane s� �cie�ki znalezionych plik�w</param>
/// <param name="strDir"> przeszukiwany katalog</param>
/// <param name="strMask"> maska przeszukiwania</param>
/// <param name="strDirIn"> przedrostek nazw katalog�w, kt�ry jest pomijany podczas zapisu do mapy</param>
/// <param name="strExt"> opcjonalna lista rozszerze�, kt�re maj� by� sprawdzane</param>
__declspec(dllexport) void SearchFiles(FileVector &_fv, CStringW strDir, const CStringW &strMask,
	const CStringW &strDirIn, const CStringW &strExt = "");

/// <summary>Przeniesienie pliku</summary>
__declspec(dllexport) bool FileMove(CStringW strSrc, CStringW strDst);

/// <summary>Analiza katalogu</summary>
__declspec(dllexport) bool AnalyzeDirectory(CStringW strDir, CStringW strMask, const CStringW &strExt,
	std::list<CImageFile> &imageList, bool _ImageCanBeRotated,
	ProgressUpdateCallback ProgressUpdate, FILE *logFile,
	bool UseList);

/// <summary>Wczytanie listy</summary>
__declspec(dllexport) bool ReadList(boost::container::map<CStringW, CImageFile> &imageList, CStringW strDir, CStringW fileName);

/// <summary>Zapisanie list do pliku</summary>
__declspec(dllexport) bool SaveList(std::list<CImageFile> &imageList, CStringW strDir, CStringW fileName);

/// <summary>Pobranie nazwy pliku/katalogu ze �cie�ki</summary>
__declspec(dllexport) CStringW GetFileNameFromPath(CStringW path);

#endif
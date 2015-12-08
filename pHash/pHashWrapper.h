
#ifndef _PHASH_WRAPPER_H
#define _PHASH_WRAPPER_H

#include "pHash.h"
#include "mvptree.h"

__declspec(dllexport) typedef	vector< CStringW >FileVector;

/// <summary>Pobranie pe�nej �cie�ki katalogu</summary>
/// <param name="_dirPath">katalog</param>
/// <param name="_bMask">maska katalogu</param>
/// <param name="IsInputDirectory">blokowanie nazw katalog�w wej�ciowych</param>
__declspec(dllexport) CStringW GetDirectory(CStringW _dirPath, CStringW *_bMask, bool IsInputDirectory);

/// <summary>Rekurencyjne wyszukanie plikow</summary>
/// <param name="_fv"> wektor, do kt�rego zapisywane s� �cie�ki znalezionych plik�w</param>
/// <param name="strDir"> przeszukiwany katalog</param>
/// <param name="strMask"> maska przeszukiwania</param>
/// <param name="strDirIn"> przedrostek nazw katalog�w, kt�ry jest pomijany podczas zapisu do mapy</param>
/// <param name="_DirMap"> opcjonalny wska�nik na map� struktur w kt�rej zapisywane s� daty katalog�w</param>
/// <param name="strExt"> opcjonalna lista rozszerze�, kt�re maj� by� sprawdzane</param>
__declspec(dllexport) void SearchFiles(FileVector &_fv, CStringW strDir, const CStringW &strMask,
	const CStringW &strDirIn, const CStringW &strExt = "");

/// <summary>Przeniesienie pliku</summary>
__declspec(dllexport) bool FileMove(CStringW strSrc, CStringW strDst);

/// <summary>Analiza katalogu</summary>
__declspec(dllexport) bool AnalyzeDirectory(CStringW strDir, MVPTree *mTree);

#endif
// Tst.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <pHashWrapper.h>
#include "wingetopt.h"

bool batch;
CString strDir, strDir2, strDirOK, strDirDUP;

typedef std::list<CImageFile> IFList;

__int64 cDUP, cERROR;

IFList::iterator findNext(const CImageFile &obj, const double &eQuality, IFList::iterator start, IFList::iterator end)
{
	while (start != end)
	{
		if (start->IsSimiliar(obj, eQuality))
			return start;
		else
			start++;
	};

	return end;
}

void WriteLogErrorMove(FILE *logFile, CString orgLoc, CString newLoc)
{
	if (logFile != NULL)
	{
		fwprintf_s(logFile, L"ERROR while moving file: %s -> %s\r\n", orgLoc.GetBuffer(), newLoc.GetBuffer());
	}
}

void WriteLogMoved(FILE *logFile, CString fOK, CString fDUP)
{
	if (logFile != NULL)
	{
		fwprintf_s(logFile, L"OK:  %s\r\nDUP: %s\r\n\r\n", fOK.GetBuffer(), fDUP.GetBuffer());
	}
}

void AnalyzeSingleList(IFList &list, const double &eQuality, FILE *logFile, bool MoveSmaller)
{
	IFList::iterator iFile = list.begin();
	while (iFile != list.end())
	{
		IFList::iterator iFind = iFile;
		iFind = findNext(*iFile, eQuality, ++iFind, list.end());
		if (iFind != list.end())
		{
			CString pom;
			if ((iFile->iHeight < iFind->iHeight || iFile->iWidth < iFind->iWidth) // iFile has smaller resolution
				|| (iFile->iHeight == iFind->iHeight && iFile->iWidth == iFind->iWidth	// files has same width
					&& iFile->fSize.QuadPart < iFind->fSize.QuadPart					// and iFile is smaller
					&& MoveSmaller)														// and smaller files will be treated as duplicates
				)
			{	// iFile is worse
				pom = iFile->fName;
				pom.Replace(strDir, strDirDUP);
				if (FileMove(iFile->fName, pom))
				{
					cDUP++;
					WriteLogMoved(logFile, iFind->fName, pom);
				}
				else
				{
					cERROR++;
					WriteLogErrorMove(logFile, iFile->fName, pom);
				}
				iFile = list.erase(iFile);
			}
			else
			{	// iFind is worse
				pom = iFind->fName;
				pom.Replace(strDir, strDirDUP);
				if (FileMove(iFind->fName, pom))
				{
					cDUP++;
					WriteLogMoved(logFile, iFile->fName, pom);
				}
				else
				{
					cERROR++;
					WriteLogErrorMove(logFile, iFind->fName, pom);
				}
				list.erase(iFind);
			}
		}
		else
			iFile++;
	}
}

void AnalizeListWithTemplate(IFList &lTemplate, IFList &lBad, const double &eQuality, FILE *logFile)
{
	for each(CImageFile badFile in lBad)
	{	// Dla ka¿dego elementu na liœcie BAD wyszukaj podobne obiekty na liœcie OK
		IFList::iterator iFind = findNext(badFile, eQuality, lTemplate.begin(), lTemplate.end());
		if (iFind != lTemplate.end())
		{	// Znaleziono podobny plik
			CString pom = badFile.fName;
			pom.Replace(strDir2, strDirDUP);
			// Przenieœ plik
			if (FileMove(badFile.fName, pom))
			{
				cDUP++;
				WriteLogMoved(logFile, pom, iFind->fName);
			}
			else
			{
				cERROR++;
				WriteLogErrorMove(logFile, badFile.fName, pom);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// U¿ycie programu
/////////////////////////////////////////////////////////////////////////////////////////
void usageInfo()
{
	std::printf("Usage: <this> <dir> [<dir2>] [-brlsq:e:]\n\n");
	std::printf("Arguments:\n");
	std::printf("-b\t- run in batch mode\n");
	std::printf("-r\t- allow image rotation\n");
	std::printf("-l\t- use file lists\n");
	std::printf("-s\t- files with smaller size will be treated as duplicates\n");
	std::printf("-q\t- eQuality parameter. DEFAULT: 5.00\n");
	std::printf("-e LIST\t- extension (one or more) of files to validate\n");
	std::printf(" DEFAULT: -e .jpg;.jpeg;.png;.crw;.cr2;.raf;.mrw;.raw;.nef;.orf;.pef;.x3f;.arw;.sr2;.srf;.rw2;\n\n");
	std::printf("DEFAULT USAGE: ImageComparerCmd <dir> [<dir2>] -rl \n\n");
	if (!batch)
		system("PAUSE");
}

void ProgressAnalyzeUpdate(const __int64&counter, const __int64&count)
{
	std::printf("%10I64d/%010I64d", counter, count);
	std::printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("ImageComparer v.1.2 by HDLAB 2015-2016 (Unicode & Long Path (partially?) supported)\n\n");

	// Sprawdzenie parametrow
	if (argc<2)
	{
		usageInfo();
		return -1;
	}
	batch = false;
	cDUP = 0;
	cERROR = 0;

	bool ImageCanBeRotated = false;
	bool MoveSmaller = false;
	bool UseLists = false;
	double eQuality = 5;
	CString strMaskExt = L".jpg;.jpeg;.png;.crw;.cr2;.raf;.mrw;.raw;.nef;.orf;.pef;.x3f;.arw;.sr2;.srf;.rw2;";

	int c;
	opterr = 0;
	strDir = GetDirectory(argv[1], true);
	if (strDir.GetLength() == 0)
	{
		if (!batch)
			system("PAUSE");
		return -1;
	}

	optind = 2;

	if (argc > 2 && argv[2][0] != '-')
	{
		strDir2 = GetDirectory(argv[2], true);
		if (strDir2.GetLength() == 0)
		{
			if (!batch)
				system("PAUSE");
			return -1;
		}

		optind = 3;
	}

	CString tempString = L"";
	while ((c = getopt(argc, argv, _T("sblrq:e:"))) != -1)
	switch (c)
	{
		case 's':
			MoveSmaller = true;
			std::printf("Files with smaller size will be treated as dups...\n");
			break;
		case 'b':
			batch = true;
			printf("Batch mode activated...\n");
			break;
		case 'l':
			UseLists = true;
			printf("Will use file lists...\n");
			break;
		case 'r':
			ImageCanBeRotated = true;
			std::printf("Image can be rotated (90, 180, 270 degrees)...\n");
			break;
		case 'q':
			eQuality = _wtof(optarg);
			std::printf("Setting eQuality parameter: %lf...\n", eQuality);
			break;
		case 'e':
			tempString = optarg;
			strMaskExt = tempString;
			if (strMaskExt.ReverseFind(';') != (strMaskExt.GetLength() - 1))
				strMaskExt += ';';
			std::printf("Using file mask: %S\n", strMaskExt);
			break;
		case '?':
			if (optopt == 'e')
				std::fprintf(stderr, "Option -%c requires an argument.\n\n", optopt);
			else if (isprint(optopt))
				std::fprintf(stderr, "Unknown option `-%c'.\n\n", optopt);
			else
				std::fprintf(stderr,
				"Unknown option character `\\x%x'.\n\n",
				optopt);
			usageInfo();
			return -1;
		default:
			usageInfo();
			return -1;
	}
	if (optind<argc)
	{
		for (int index = optind; index < argc; index++)
			std::printf("Non-option argument %S\n\n", argv[index]);
		usageInfo();
		return -1;
	}

	strDirOK = GetDirectory("OK", false);
	if (strDirOK.GetLength() == 0)
	{
		if (!batch)
			system("PAUSE");
		return -1;
	}

	strDirDUP = GetDirectory("DUP", false);
	if (strDirOK.GetLength() == 0)
	{
		if (!batch)
			system("PAUSE");
		return -1;
	}

	FILE *logFile = NULL;
	int logNum = 0;
	CString pom = "ImageComparerCmdLog.txt";
	
	while (!_wfopen_s(&logFile, pom.GetBuffer(), L"rb"))
	{	// Ustalenie nowej nazwy pliku logu
		fclose(logFile);
		pom.Format(L"ImageComparerCmdLog_%3d.txt", ++logNum);

	};

	if (_wfopen_s(&logFile, pom.GetBuffer(), L"wb"))
	{	// Nie uda³o siê otworzyæ pliku
		logFile = NULL;
		std::printf("ERROR WHILE OPENING LOG FILE %S!!!\n\n", pom.GetBuffer());
	}

	if (UseLists)
	{
		GetFileNameFromPath(strDir);
		

		if (strDir2.GetLength() > 0)
		{
			GetFileNameFromPath(strDir2);
		}
		
	}

	std::printf("Analyzing %S...\n", strDir.GetBuffer());
	IFList lFiles;
	time_t lFilesSize = 0;
	if (AnalyzeDirectory(strDir, "*.*", strMaskExt, lFiles, ImageCanBeRotated, *ProgressAnalyzeUpdate, logFile, UseLists))
	{
		std::printf("\nDone!\n\n");
	}
	
	if (strDir2.GetLength() > 0)
	{
		std::printf("Analyzing %S...\n", strDir2.GetBuffer());
		IFList lFiles2;
		if (AnalyzeDirectory(strDir2, "*.*", strMaskExt, lFiles2, ImageCanBeRotated, *ProgressAnalyzeUpdate, logFile, UseLists))
		{
			std::printf("\nDone!\n\n");
		}
		lFilesSize = lFiles.size();
		AnalizeListWithTemplate(lFiles, lFiles2, eQuality, logFile);

		lFiles.clear();
		lFiles2.clear();
	}
	else
	{
		lFilesSize = lFiles.size();
		AnalyzeSingleList(lFiles, eQuality, logFile, MoveSmaller);
		lFiles.clear();
	}

	std::printf("\n\nFiles validating complete !!!\n\n");
	
	std::printf("\nSummary:\n");
	std::printf(" - all files:\t\t\t%10d\t\t%6.2f %%\n", lFilesSize, lFilesSize>0 ? double(lFilesSize) * 100 / double(lFilesSize) : 0);
	std::printf(" - dup files:\t\t\t%10I64d\t\t%6.2f %%\n", cDUP, lFilesSize>0 ? double(cDUP) * 100 / double(lFilesSize) : 0);
	std::printf(" - error files:\t\t\t%10I64d\t\t%6.2f %%\n", cERROR, lFilesSize>0 ? double(cERROR) * 100 / double(lFilesSize) : 0);
	//printf("Data Recovery Efficiency:%6.2f %%\n\n\n", (cok + cdup)>0 ? double(cok) * 100 / double(cok + cdup) : 0);
	
	if (logFile != NULL)
		fclose(logFile);
	
	if (!batch)
		system("PAUSE");
	return 0;
}


// Tst.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <pHashWrapper.h>
#include "wingetopt.h"

bool batch;
CString strDir, strDir2, strDirOK, strDirDUP;

typedef std::list<CImageFile> IFList;
bool CImageFile::ImageCanBeRotated = false;

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

void AnalyzeSingleList(IFList &list, const double &eQuality)
{
	IFList::iterator iFile = list.begin();
	while (iFile != list.end())
	{
		IFList::iterator iFind = iFile;
		iFind = findNext(*iFile, eQuality, ++iFind, list.end());
		if (iFind != list.end())
		{
			CString pom;
			if (iFile->iHeight < iFind->iHeight || iFile->iWidth < iFind->iWidth)
			{	// iFile is worse
				pom = iFile->fName;
				pom.Replace(strDir, strDirDUP);
				if (FileMove(iFile->fName, pom))
					cDUP++;
				else
					cERROR++;
				iFile = list.erase(iFile);
			}
			else
			{	// iFind is worse
				pom = iFind->fName;
				pom.Replace(strDir, strDirDUP);
				if (FileMove(iFind->fName, pom))
					cDUP++;
				else
					cERROR++;
				list.erase(iFind);
			}
		}
		else
			iFile++;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// U¿ycie programu
/////////////////////////////////////////////////////////////////////////////////////////
void usageInfo()
{
	std::printf("Usage: <this> <dir> [<dir2>] [-brq:e:]\n\n");
	std::printf("Arguments:\n");
	std::printf("-b\t- run in batch mode\n");
	std::printf("-r\t- allow image rotation\n");
	std::printf("-q\t- eQuality parameter. DEFAULT: 5.00\n");
	std::printf("-e LIST\t- extension (one or more) of files to validate\n");
	std::printf(" DEFAULT: -e .jpg;.jpeg;.png;.crw;.cr2;.raf;.mrw;.raw;.nef;.orf;.pef;.x3f;.arw;.sr2;.srf;.rw2;\n\n");
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
	printf("ImageComparer v.1.01 by HDLab 2015 (Unicode & Long Path supported)\n\n");

	// Sprawdzenie parametrow
	if (argc<2)
	{
		usageInfo();
		return -1;
	}
	batch = false;
	cDUP = 0;
	cERROR = 0;
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
	while ((c = getopt(argc, argv, _T("brq:e:"))) != -1)
		switch (c)
	{
		case 'b':
			batch = true;
			printf("Batch mode activated...\n");
			break;
		case 'r':
			CImageFile::ImageCanBeRotated = true;
			printf("Image can be rotated (90, 180, 270 degrees)...\n");
			break;
		case 'q':
			eQuality = _wtof(optarg);
			printf("Setting eQuality parameter: %lf...\n", eQuality);
			break;
		case 'e':
			tempString = optarg;
			strMaskExt = tempString;
			if (strMaskExt.ReverseFind(';') != (strMaskExt.GetLength() - 1))
				strMaskExt += ';';
			printf("Using file mask: %S\n", strMaskExt);
			break;
		case '?':
			if (optopt == 'e')
				fprintf(stderr, "Option -%c requires an argument.\n\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n\n", optopt);
			else
				fprintf(stderr,
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
			printf("Non-option argument %S\n\n", argv[index]);
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

	std::printf("Analyzing %S...\n", strDir.GetBuffer());
	IFList lFiles;
	if (AnalyzeDirectory(strDir, "*.*", strMaskExt, lFiles, *ProgressAnalyzeUpdate))
	{
		std::printf("\nDone!\n\n");
	}
	
	if (strDir2.GetLength() > 0)
	{
		std::printf("Analyzing %S...\n", strDir2.GetBuffer());
		IFList lFiles2;
		if (AnalyzeDirectory(strDir2, "*.*", strMaskExt, lFiles2, *ProgressAnalyzeUpdate))
		{
			std::printf("\nDone!\n\n");
		}



		lFiles.clear();
		lFiles2.clear();
	}
	else
	{
		AnalyzeSingleList(lFiles, eQuality);
		lFiles.clear();
	}
	

	std::printf("\n\nFiles validating complete !!!\n\n");
	
	std::printf("\nSummary:\n");
	std::printf(" - all files:\t\t\t%10d\t\t%6.2f %%\n", lFiles.size(), lFiles.size()>0 ? double(lFiles.size()) * 100 / double(lFiles.size()) : 0);
	std::printf(" - dup files:\t\t\t%10I64d\t\t%6.2f %%\n", cDUP, lFiles.size()>0 ? double(cDUP) * 100 / double(lFiles.size()) : 0);
	std::printf(" - error files:\t\t\t%10I64d\t\t%6.2f %%\n", cERROR, lFiles.size()>0 ? double(cERROR) * 100 / double(lFiles.size()) : 0);
	//printf("Data Recovery Efficiency:%6.2f %%\n\n\n", (cok + cdup)>0 ? double(cok) * 100 / double(cok + cdup) : 0);
	
	
	if (!batch)
		system("PAUSE");
	return 0;
}


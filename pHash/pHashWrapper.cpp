#include "pHashWrapper.h"
#include <Shlwapi.h>
#include <boost/crc.hpp>  // for boost::crc_32_type
#include <exiv2/exiv2.hpp>

CImageFile::CImageFile()
	: fName(""),
	fCRC(0),
	iWidth(0),
	iHeight(0),
	ImageCanBeRotated(false),
	hasExif(false),
	eWidth(-1),
	eLength(-1),
	eMakeModel(),
	eDateTimeOriginal(0)
{
	fSize.QuadPart = 0;
	fDate = 0;
	iHash[0] = 0;
	iHash[1] = 0;
	iHash[2] = 0;
	iHash[3] = 0;
}

__time64_t filetime_to_timet(FILETIME const& ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

CImageFile::CImageFile(CStringW file, bool _ImageCanBeRotated, bool _CalculateHash) : CImageFile()
{
	Initialize(file, _ImageCanBeRotated, _CalculateHash);
}

bool CImageFile::Initialize(CStringW file, bool _ImageCanBeRotated, bool _CalculateHash)
{
	bool res = true;
	this->ImageCanBeRotated = _ImageCanBeRotated;
	this->fName = file;

	HANDLE hFile = CreateFileW(fName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// Read file size
		BOOL res = GetFileSizeEx(hFile, &fSize);

		FILETIME date;
		date.dwHighDateTime = 0;
		date.dwLowDateTime = 0;
		// Read file time
		res = GetFileTime(hFile, NULL, NULL, &date);
		if (res)
			fDate = filetime_to_timet(date);

		CloseHandle(hFile);

		// Read exif
		ReadExif();

		// ReadHash
		if (_CalculateHash)
		{
			CalculateHash();
		}
	}
	else
		res = false;

	return res;
}

bool CImageFile::ReadExif()
{
	bool res = true;

	char *ansiname = NULL;
	if (GetShortPathNameANSI(fName.GetBuffer(), fName.GetLength(), &ansiname, true))
	{
		try
		{
			Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(ansiname);
			if (image.get() != 0)
			{
				image->readMetadata();
				Exiv2::ExifData &exifData = image->exifData();
				hasExif = !exifData.empty();

				if (hasExif)
				{
					if (iWidth == 0)
						iWidth = image->pixelWidth();
					if (iHeight == 0)
						iHeight = image->pixelHeight();

					Exiv2::ExifData::const_iterator end = exifData.end();
					for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i)
					{
						CString typeName = i->typeName();
						CString key = i->key().c_str();
						
						if (key.CompareNoCase("Exif.Image.ImageWidth") == 0)
						{
							if (typeName.CompareNoCase("Short") == 0)
								eWidth = i->value().toLong();
							else
								eWidth = -1;
						}
						else if (key.CompareNoCase("Exif.Image.ImageLength") == 0)
						{
							if (typeName.CompareNoCase("Short") == 0)
								eLength = i->value().toLong();
							else
								eLength = -1;
						}
						else if (key.CompareNoCase("Exif.Image.Make") == 0)
						{
							if (typeName.CompareNoCase("Ascii") == 0)
							{
								CString temp = eMakeModel;
								eMakeModel = i->value().toString().c_str();
								eMakeModel += temp;
							}
						}
						else if (key.CompareNoCase("Exif.Image.Model") == 0)
						{
							if (typeName.CompareNoCase("Ascii") == 0)
							{
								eMakeModel += " ";
								eMakeModel += i->value().toString().c_str();
							}
						}
						else if (key.CompareNoCase("Exif.Photo.DateTimeOriginal") == 0)
						{
							if (typeName.CompareNoCase("Ascii") == 0 && i->count() == 20)
							{
								struct std::tm tm;

								std::istringstream ss(i->value().toString().c_str());
								std::string time_formats[] = { "%Y-%m-%d%t%H:%M:%S",
															  "%Y:%m:%d%t%H:%M:%S" };
								for each (std::string time_format in time_formats)
								{
									ss >> std::get_time(&tm, time_format.c_str());
									if (!ss.fail())
									{
										eDateTimeOriginal = mktime(&tm);
										break;
									}
								}
							}
						}
					}
				}
			}
			else
				res = false;
		}
		catch (Exiv2::Error) {
			//std::cout << "Caught Exiv2 exception '" << e.what() << "'\n";
			res = false;;
		}
		catch (...)
		{
			res = false;
		}
	}
	else
		res = false;
	
	return res;
}

bool CImageFile::CalculateHash()
{
	bool res = true;

	char *ansiname = NULL;
	if (GetShortPathNameANSI(fName.GetBuffer(), fName.GetLength(), &ansiname, true))
	{
		try
		{
			CImg<uint8_t> *src = new CImg<uint8_t>(ansiname);
			
			if (src)
			{
				if (iWidth == 0)
					iWidth = src->width();
				if (iHeight == 0)
					iHeight = src->height();

				if (_ph_dct_imagehash(src, iHash[0]) < 0)
					res = false;
				else if (ImageCanBeRotated)
				{
					if (_ph_dct_imagehash(src, iHash[1], 90) < 0)
						res = false;
					if (_ph_dct_imagehash(src, iHash[2], 180) < 0)
						res = false;
					if (_ph_dct_imagehash(src, iHash[3], 270) < 0)
						res = false;
				}
				delete src;
			}
			else
				res = false;
		}
		catch (...)
		{
			res = false;
		}
		delete[] ansiname;

	}
	else
		res = false;

	return res;
}

bool CImageFile::CalculateCRC()
{
	bool res = true;
	if (fName.IsEmpty())
		return false;

	HANDLE hFile = CreateFileW(fName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// Read CRC
		char *buffer = new char[_MAX_BUFFER];
		LARGE_INTEGER readBytes = fSize;
		boost::crc_32_type  result;
		DWORD read;
		do{
			if (readBytes.QuadPart > _MAX_BUFFER)
			{
				if (!ReadFile(hFile, buffer, _MAX_BUFFER, &read, NULL))
				{
					res = false;
					break;
				}
				else
				{
					result.process_bytes(buffer, _MAX_BUFFER);
					readBytes.QuadPart -= _MAX_BUFFER;
				}
			}
			else{
				if (!ReadFile(hFile, buffer, (DWORD)readBytes.QuadPart, &read, NULL))
				{
					res = false;
					break;
				}
				else
				{
					result.process_bytes(buffer, (size_t)readBytes.QuadPart);
					readBytes.QuadPart = 0;
				}
			}
		} while (readBytes.QuadPart > 0);

		fCRC = result.checksum();
		CloseHandle(hFile);
		delete[]buffer;
	}
	else
		return false;

	return res;
}

bool CImageFile::IsSimiliar(const CImageFile &obj, const double &eQuality)
{
	if (ImageCanBeRotated)
	{
		for each(ulong64 thisHash in this->iHash)
		{
			for each(ulong64 otherHash in obj.iHash)
			{
				if (ph_hamming_distance(thisHash, otherHash) <= eQuality)
					return true;
			}
		}
	}
	else if (ph_hamming_distance(this->iHash[0], obj.iHash[0]) <= eQuality)
		return true;

	return false;
}

bool GetShortPathNameANSI(wchar_t *unicodestr, int lenW, char **ansistr, bool removeUnicodeAddon/*=false*/)
{
	bool res = true;
	int stringOffset = 0;
	if (removeUnicodeAddon && unicodestr[0] == L'\\' && unicodestr[1] == L'\\' && unicodestr[2] == L'?' && unicodestr[0] == L'\\')
	{
		stringOffset = 4;
	}
	lenW = GetShortPathNameW(unicodestr + stringOffset, NULL, 0);
	if (lenW > 0)
	{
		//lenW = ::SysStringLen(unicodestr);
		wchar_t *wName = new wchar_t[lenW + 1];
		GetShortPathNameW(unicodestr + stringOffset, wName, lenW);
		wName[lenW] = 0x00;

		int lenA = ::WideCharToMultiByte(CP_ACP, 0, wName, lenW, 0, 0, NULL, NULL);
		if (lenA > 0)
		{
			*ansistr = new char[lenA + 1]; // allocate a final null terminator as well
			::WideCharToMultiByte(CP_ACP, 0, wName, lenW, *ansistr, lenA, NULL, NULL);
			(*ansistr)[lenA] = 0; // Set the null terminator yourself
		}
		else
		{
			res = false;
		}
		delete wName;
	}
	else
		res = false;

	return res;
}

/// <summary>Pobranie pe³nej œcie¿ki katalogu</summary>
/// <param name="_dirPath">katalog</param>
/// <param name="_bMask">maska katalogu</param>
/// <param name="IsInputDirectory">blokowanie nazw katalogów wejœciowych</param>
CStringW GetDirectory(CStringW _dirPath, bool IsInputDirectory)
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
		if (memcmp(dir, L"\\\\?\\", 8) != 0)
		{
			::GetCurrentDirectoryW(_MAX_PATH, path);
			strDir = path;
			if (strDir[strDir.GetLength() - 1] != '\\')
				strDir += _T("\\");
		}
		
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
		//system("PAUSE");
		return "";
	}
	_wsplitpath_s(strDir, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

	if (memcmp(dir, L"\\\\?\\", 8) != 0)
		strDir = _T("\\\\?\\");
	else
		strDir = "";

	strDir += drive;
	strDir += dir;
	strDir += fname;
	strDir += ext;
	if (strDir[strDir.GetLength() - 1] != '\\')
		strDir += _T("\\");

	return strDir;
}

/// <summary>Rekurencyjne wyszukanie plikow</summary>
/// <param name="_fv"> wektor, do którego zapisywane s¹ œcie¿ki znalezionych plików</param>
/// <param name="strDir"> przeszukiwany katalog</param>
/// <param name="strMask"> maska przeszukiwania</param>
/// <param name="strDirIn"> przedrostek nazw katalogów, który jest pomijany podczas zapisu do mapy</param>
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
bool FileMove(CStringW strSrc, CStringW strDst)
{
	TRY
	{
		CStringW pom;
		CStringW strNewName = strDst.Right(strDst.GetLength() - 7);
		CStringW strDirectory = strDst.Left(6);
		// Utworzenie wszystkich podkatalogow
		pom.Empty();
		for (int i = 0; i<strNewName.GetLength(); i++)
		{
			if (strNewName[i] != '\\')
				pom += strNewName[i];
			else
			{
				strDirectory = strDirectory + "\\" + pom;
				if (::CreateDirectoryW(strDirectory, NULL) == FALSE)
				{
					if (::GetLastError() != ERROR_ALREADY_EXISTS)
						return false;
				}
				pom.Empty();
			}
		}
		return MoveFileW(strSrc, strDst) != 0;
		//CFile::Rename(strSrc, strDst);
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

/// <summary>Pobranie nazwy pliku/katalogu ze œcie¿ki</summary>
__declspec(dllexport) CStringW GetFileNameFromPath(CStringW path)
{
	CStringW fileName;
	// Trim last slash
	if (path.GetBuffer()[path.GetLength() - 1] == '\\')
		path = path.Left(path.GetLength() - 1);

	fileName = path;
	int slashPos = path.ReverseFind('\\');
	if (slashPos != -1)
		fileName = path.Mid(slashPos + 1);

	return fileName;
}

/// <summary>Analiza katalogu</summary>
bool AnalyzeDirectory(CStringW strDir, CStringW strMask, const CStringW &strExt,
	std::list<CImageFile> &imageList, bool _ImageCanBeRotated, 
	ProgressUpdateCallback ProgressUpdate, FILE *logFile,
	bool UseList)
{
	// Gathering files
	strDir = GetDirectory(strDir, false);

	FileVector m_fv;
	SearchFiles(m_fv, strDir, strMask, strDir, strExt);
	__int64 counter = 0;
	__int64 count = m_fv.size();

	boost::container::map<CStringW, CImageFile> imageListInFile;
	boost::container::map<CStringW, CImageFile>::iterator imageListInFileIt = imageListInFile.end();
	if (UseList)
	{
		ReadList(imageListInFile, strDir, GetFileNameFromPath(strDir) + ".imgList");
	}

	while (!m_fv.empty())
	{
		ProgressUpdate(counter, count);
		
		try
		{
			CImageFile imageFile(m_fv.back(), _ImageCanBeRotated, false);
			bool foundEqual = false;
			if (UseList)
			{
				imageListInFileIt = imageListInFile.find(m_fv.back());
				if (imageListInFileIt != imageListInFile.end())
				{	// Nazwa pliku sie zgadza
					if (imageFile.fSize.QuadPart == imageListInFile[m_fv.back()].fSize.QuadPart
						&& imageFile.fDate == imageListInFile[m_fv.back()].fDate
						&& (imageFile.iWidth == 0 || (imageFile.iWidth == imageListInFile[m_fv.back()].iWidth))
						&& (imageFile.iHeight == 0 || (imageFile.iHeight == imageListInFile[m_fv.back()].iHeight)))
					{	// Rozmiar, data, wysokoœæ i szerokoœæ obrazka równie¿ siê zgadazaj¹
						imageFile = imageListInFile[m_fv.back()];
						foundEqual = true;
						imageList.push_back(imageFile);
					}
					
					imageListInFileIt = imageListInFile.erase(imageListInFileIt);
				}
			}
			
			if (!foundEqual)
			{
				imageFile.CalculateHash();
				imageList.push_back(imageFile);
			}
		}
		catch (exception)
		{
			if (logFile != NULL)
			{
				fwprintf_s(logFile, L"ERROR PARSING FILE: %s\r\n", m_fv.back());
			}
		}
		m_fv.pop_back();

		counter++;
	}

	if (UseList)
	{
		SaveList(imageList, strDir, GetFileNameFromPath(strDir) + ".imgList");
	}


	return true;
}

#define BUFFER_SIZE 50000

/// <summary>Wczytanie listy</summary>
__declspec(dllexport) bool ReadList(boost::container::map<CStringW, CImageFile> &imageList, CStringW strDir, CStringW fileName)
{
	FILE *fList = NULL;
	errno_t err = _wfopen_s(&fList, fileName, L"rb");
	if (err)
	{
		return false;
	}
	else
	{
		BYTE *BUFFER[BUFFER_SIZE];
		memset(BUFFER, 0x00, BUFFER_SIZE);
		const char *fileHeader = "FILE";
		bool error = false;

		// "FILE"
		while (fread_s(BUFFER, BUFFER_SIZE, 1, 4, fList) == 4 && memcmp(BUFFER, fileHeader, 4) == 0)
		{
			/*
			"FILE"			4
			NAME_SIZE		4
			fName			NAME_SIZE
			fSize			8
			fDate			8
			iWidth			4
			iHeight			4
			iHash[0]		8
			iHash[1]		8
			iHash[2]		8
			iHash[3]		8

			Offset      0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F

			00000000   46 49 4C 45 17 00 00 00  5C 00 5C 00 3F 00 5C 00   FILE....\.\.?.\.
			00000010   7A 00 3A 00 5C 00 61 00  6C 00 6C 00 5C 00 5F 00   z.:.\.a.l.l.\._.
			00000020   44 00 53 00 43 00 30 00  37 00 31 00 37 00 2E 00   D.S.C.0.7.1.7...
			00000030   4E 00 45 00 46 00 A2 44  A2 00 00 00 00 00 D4 D9   N.E.F.¢D¢.....ÔÙ
			00000040   34 56 00 00 00 00 C0 10  00 00 20 0B 00 00 13 CD   4V....À... ....Í
			00000050   A4 BA D9 53 66 89 F6 3C  03 C7 2C 19 26 EB 5C 69   ¤ºÙSf‰ö<.Ç,.&ë\i
			00000060   B9 92 C6 0C 97 BA B8 D8  86 25 32 C6 EC DE         ¹’Æ.—º¸Ø†%2ÆìÞ

			*/
			CImageFile imageFile;

			// Rozmiar nazwy
			int nameLength;
			if (fread(&nameLength, 1, sizeof(int), fList) != sizeof(int))
			{
				error = true;
				break;
			}

			memset(BUFFER, 0x00, BUFFER_SIZE);
			// Nazwa pliku - œcie¿ka wzglêdna
			if (fread(&BUFFER, 1, nameLength * sizeof(wchar_t), fList) != nameLength * sizeof(wchar_t))
			{
				error = true;
				break;
			}
			imageFile.fName = strDir + (wchar_t*)BUFFER;

			// Image file size
			if (fread(&imageFile.fSize.QuadPart, 1, sizeof(__int64), fList) != sizeof(__int64))
			{
				error = true;
				break;
			}

			// Date
			if (fread(&imageFile.fDate, 1, sizeof(__int64), fList) != sizeof(__int64))
			{
				error = true;
				break;
			}

			// Width
			if (fread(&imageFile.iWidth, 1, sizeof(int), fList) != sizeof(int))
			{
				error = true;
				break;
			}

			// Height
			if (fread(&imageFile.iHeight, 1, sizeof(int), fList) != sizeof(int))
			{
				error = true;
				break;
			}

			// Hash[0]
			if (fread(&imageFile.iHash[0], 1, sizeof(ulong64), fList) != sizeof(ulong64))
			{
				error = true;
				break;
			}

			// Hash[1]
			if (fread(&imageFile.iHash[1], 1, sizeof(ulong64), fList) != sizeof(ulong64))
			{
				error = true;
				break;
			}

			// Hash[2]
			if (fread(&imageFile.iHash[2], 1, sizeof(ulong64), fList) != sizeof(ulong64))
			{
				error = true;
				break;
			}

			// Hash[3]
			if (fread(&imageFile.iHash[3], 1, sizeof(ulong64), fList) != sizeof(ulong64))
			{
				error = true;
				break;
			}

			if (imageFile.iHash[1] != 0 || imageFile.iHash[2] != 0 || imageFile.iHash[3] != 0)
				imageFile.ImageCanBeRotated = true;
			imageList[imageFile.fName] = imageFile;
		}

		fclose(fList);

		if (error)
			return false;
		else
			return true;
	}
}

/// <summary>Zapisanie list do pliku</summary>
bool SaveList(std::list<CImageFile> &imageList, CStringW strDir, CStringW fileName)
{
	FILE *fOut = NULL;
	errno_t err = _wfopen_s(&fOut, fileName, L"wb");
	if (err)
	{
		return false;
	}
	else
	{
		for each (CImageFile imageFile in imageList)
		{
			/*
				"FILE"			4
				NAME_SIZE		4
				fName			NAME_SIZE
				fSize			8
				fDate			8
				iWidth			4
				iHeight			4
				iHash[0]		8
				iHash[1]		8
				iHash[2]		8
				iHash[3]		8

				Offset      0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F

				00000000   46 49 4C 45 17 00 00 00  5C 00 5C 00 3F 00 5C 00   FILE....\.\.?.\.
				00000010   7A 00 3A 00 5C 00 61 00  6C 00 6C 00 5C 00 5F 00   z.:.\.a.l.l.\._.
				00000020   44 00 53 00 43 00 30 00  37 00 31 00 37 00 2E 00   D.S.C.0.7.1.7...
				00000030   4E 00 45 00 46 00 A2 44  A2 00 00 00 00 00 D4 D9   N.E.F.¢D¢.....ÔÙ
				00000040   34 56 00 00 00 00 C0 10  00 00 20 0B 00 00 13 CD   4V....À... ....Í
				00000050   A4 BA D9 53 66 89 F6 3C  03 C7 2C 19 26 EB 5C 69   ¤ºÙSf‰ö<.Ç,.&ë\i
				00000060   B9 92 C6 0C 97 BA B8 D8  86 25 32 C6 EC DE         ¹’Æ.—º¸Ø†%2ÆìÞ

			*/

			// "FILE"
			int written = fwrite("FILE", 1, 4, fOut);
			if (written != 4)
			{
				return false;
			}

			// Œcie¿ka wzglêdna!
			CStringW imageName = imageFile.fName;
			imageName.Replace(strDir, L"");

			int nameLength = imageName.GetLength();
			written = fwrite(&nameLength, 1, sizeof(int), fOut);
			if (written != sizeof(int))
			{
				return false;
			}

			written = fwrite(imageName.GetBuffer(), 1, nameLength * sizeof(wchar_t), fOut);
			if (written != nameLength * sizeof(wchar_t))
			{
				return false;
			}

			written = fwrite(&imageFile.fSize.QuadPart, 1, sizeof(__int64), fOut);
			if (written != sizeof(__int64))
			{
				return false;
			}

			written = fwrite(&imageFile.fDate, 1, sizeof(__int64), fOut);
			if (written != sizeof(__int64))
			{
				return false;
			}

			written = fwrite(&imageFile.iWidth, 1, sizeof(int), fOut);
			if (written != sizeof(int))
			{
				return false;
			}

			written = fwrite(&imageFile.iHeight, 1, sizeof(int), fOut);
			if (written != sizeof(int))
			{
				return false;
			}

			written = fwrite(&imageFile.iHash[0], 1, sizeof(ulong64), fOut);
			if (written != sizeof(ulong64))
			{
				return false;
			}

			written = fwrite(&imageFile.iHash[1], 1, sizeof(ulong64), fOut);
			if (written != sizeof(ulong64))
			{
				return false;
			}

			written = fwrite(&imageFile.iHash[2], 1, sizeof(ulong64), fOut);
			if (written != sizeof(ulong64))
			{
				return false;
			}

			written = fwrite(&imageFile.iHash[3], 1, sizeof(ulong64), fOut);
			if (written != sizeof(ulong64))
			{
				return false;
			}
		}

		fclose(fOut);
		return true;
	}

}
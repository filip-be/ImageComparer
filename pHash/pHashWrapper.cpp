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
	fDate.dwHighDateTime = 0;
	fDate.dwLowDateTime = 0;
}

CImageFile::CImageFile(CStringW file, bool _ImageCanBeRotated) : CImageFile()
{
	Initialize(file, _ImageCanBeRotated);
}

bool CImageFile::Initialize(CStringW file, bool _ImageCanBeRotated)
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

		// Read file time
		res = GetFileTime(hFile, NULL, NULL, &fDate);
		CloseHandle(hFile);

		// Read exif
		ReadExif();

		// ReadHash
		CalculateHash();
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
				iWidth = src->width();
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

/// <summary>Analiza katalogu</summary>
bool AnalyzeDirectory(CStringW strDir, CStringW strMask, const CStringW &strExt,
	std::list<CImageFile> &imageList, bool _ImageCanBeRotated, 
	ProgressUpdateCallback ProgressUpdate, FILE *logFile)
{
	// Gathering files
	strDir = GetDirectory(strDir, false);

	FileVector m_fv;
	SearchFiles(m_fv, strDir, strMask, strDir, strExt);
	__int64 counter = 0;
	__int64 count = m_fv.size();
	while (!m_fv.empty())
	{
		ProgressUpdate(counter, count);
		
		try
		{
			imageList.push_back(CImageFile(m_fv.back(), _ImageCanBeRotated));
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


	return true;
}
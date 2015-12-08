// Tst.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <pHashWrapper.h>

FileVector	m_fv;

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
			
			double pcc2;
			int res = ph_compare_images(n1A, n2A, pcc2, true);
			int x = 0;
			x = x + 1;

		}
	}
	return 0;
}


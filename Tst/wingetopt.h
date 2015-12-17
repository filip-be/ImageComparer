/*
POSIX getopt for Windows
AT&T Public License
Code given out at the 1985 UNIFORUM conference in Dallas.  

Dzia�anie:
	getopt() ko�czy swoje dzia�anie w momencie znalezienia niepoprawnego parametru
		(bez my�lnika poprzedzaj�cego go)
	getopt() jako trzeci parametr przyjmuje list� akceptowalnych parametr�w wej�ciowych
		nast�pnik ":" oznacza, �e dodatkowy argument dla parametru jest wymagany
		nast�pnik "::" oznacza, �e dodatkowy argument dla parametru jest opcjonalny

Przyk�ad:

// Katalog wej�ciowy
CString strDirIn(argv[1]);
// Nr parametru od kt�rego rozpoczyna si� sprawdzanie listy argument�w wej�ciowych
optind=2;
int c;
opterr = 0;

//[-ba] [-e LIST]
CString tempString="";
while ((c = getopt(argc, argv, _T("blfsrc:e:"))) != -1)
	switch (c)
	{
	case 'a':
		all=true;
		break;
	case 'b':
		batch=true;
		printf("Batch mode activated...\n");
		break;
	case 'e':
		strMaskExt+=optarg;
		if(strMaskExt.ReverseFind(';')!=(strMaskExt.GetLength()-1))
			strMaskExt+=';';
		printf("Using file mask: %S\n",strMaskExt);
		break;
	case '?':
		if (optopt == 'e')
			fprintf(stderr, "Option -%c requires an argument.\n\n", optopt);
		else if (isprint (optopt))
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

// Na li�cie argument�w pozosta�o co� jeszcze!
if(optind<argc)
{
	for (int index = optind; index < argc; index++)
		printf ("Non-option argument %S\n\n", argv[index]);
	usageInfo();
	return -1;
}

*/
#ifdef __GNUC__
	#include <getopt.h>
#endif
#ifndef __GNUC__
	#ifndef _WINGETOPT_H_
		#define _WINGETOPT_H_

		#ifdef __cplusplus
			extern "C" {
		#endif

		extern int opterr;
		extern int optind;
		extern int optopt;
		extern TCHAR *optarg;
		extern int getopt(int argc, TCHAR **argv, TCHAR *opts);

		#ifdef __cplusplus
			}
		#endif

	#endif  /* _GETOPT_H_ */
#endif  /* __GNUC__ */
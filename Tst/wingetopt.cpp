/*
POSIX getopt for Windows

AT&T Public License

Code given out at the 1985 UNIFORUM conference in Dallas.  
*/
#include "stdafx.h"
#include "wingetopt.h"
#include <stdio.h>

#ifndef __GNUC__
	#ifdef _UNICODE
		#define _strcmp wcscmp
		#define _strchr wcschr
		#define _fputs fputws
		#define _fputc fputwc
	#else
		#define _strcmp strcmp
		#define _strchr strchr
		#define _fputs fputs
		#define _fputc fputc
	#endif

	#define NULL	0
	#define EOF	(-1)
	#define ERR(s, c)	if(opterr){\
		TCHAR errbuf[2];\
		errbuf[0] = c; errbuf[1] = '\n';\
		_fputs(argv[0], stderr);\
		_fputs(s, stderr);\
		_fputc(c, stderr);}
		//(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
		//(void) write(2, s, (unsigned)strlen(s));\
		//(void) write(2, errbuf, 2);}

	int		opterr = 1;
	int		optind = 1;
	int		optopt;
	TCHAR	*optarg;

	

	int getopt(int argc, TCHAR **argv, TCHAR *opts)
	{
		static int sp = 1;
		register int c;
		register TCHAR *cp;

		if(sp == 1)
			if(optind >= argc ||
			   argv[optind][0] != '-' || argv[optind][1] == '\0')
				return(EOF);
			else if(_strcmp(argv[optind], _T("--")) == NULL) {
				optind++;
				return(EOF);
			}
		optopt = c = argv[optind][sp];
		if(c == ':' || (cp=_strchr(opts, c)) == NULL) {
			ERR(_T(": illegal option -"), c);
			if(argv[optind][++sp] == '\0') {
				optind++;
				sp = 1;
			}
			return('?');
		}
		if(*++cp == ':') {
			/*if(argv[optind][sp+1] != '\0')
				optarg = &argv[optind++][sp+1];
			else if(++optind >= argc ) {*/
		// CUSTOM - requies optarg as another argument
			if(++optind >= argc || argv[optind][0]=='-') {
				ERR(_T(": option requires an argument -- "), c);
				sp = 1;
				return('?');
			} else
				optarg = argv[optind++];
			sp = 1;
		} else {
			if(argv[optind][++sp] == '\0') {
				sp = 1;
				optind++;
			}
			optarg = NULL;
		}
		return(c);
	}

#endif  /* __GNUC__ */
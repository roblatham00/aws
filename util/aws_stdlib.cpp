#ifdef _WIN32
#include <Windows.h>
#else 
#include <sys/statvfs.h>
#include <sys/types.h>
#endif
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <cmath>
using namespace std;

#include "aws_stdlib.h"

float getDrvUse(const char * path)
{
#ifdef _WIN32
	//ULARGE_INTEGER FBA, TNB, TNFB;
	unsigned __int64 FBA, TNB, TNFB;
#ifdef UNICODE
	wchar_t wpath[2048];
	mbstowcs(wpath, path, 2048);
	int n;
	n = GetDiskFreeSpaceEx(wpath, (PULARGE_INTEGER)&FBA, (PULARGE_INTEGER)&TNB, (PULARGE_INTEGER)&TNFB);
	if(n == 0){
		wcerr << L"Error GetDiskFreeSpaceEx() " << path << endl;
	}

#else
	n = GetDiskFreeSpaceEx(path, (PULARGE_INTEGER)&FBA, (PULARGE_INTEGER)&TNB, (PULARGE_INTEGER)&TNFB);
	if(n == 0){
		cerr << "Error GetDiskFreeSpaceEx() " << path << endl;
	}
#endif
	if(n == 0){
		cerr << "Error GetDiskFreeSpaceEx() " << path << endl;
	}
	return (float)((double) FBA  / (double) TNB);
#else
	struct statvfs buf;
	int rc =	statvfs(path, &buf);

	if(rc < 0){
		cerr << "Error statvfs() " << strerror(errorno) << " " << buf << endl;
		return 1.;
	}
	return (float)((double)buf.f_blocks / (double)buf.f_bavail);
#endif
}
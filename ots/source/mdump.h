// http://www.codeproject.com/debug/postmortemdebug_standalone1.asp

#ifndef MDUMP_H
#define MDUMP_H
#include <windows.h>
#include <dbghelp.h>
#include <string>

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

class MiniDumper
{
private:
	static std::string appName;
	static LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo);
public:
	MiniDumper(const std::string& name);
};

#endif //MDUMP_H

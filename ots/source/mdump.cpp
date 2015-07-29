#ifdef MSVC_EXCEPTION_TRACER

#include "mdump.h"
#include <cassert>
#include <time.h>

std::string MiniDumper::appName;

MiniDumper::MiniDumper(const std::string& name)
{
	assert(appName.empty());	// two instances are not allowed
	appName = name.empty()? "Application" : name;
	::SetUnhandledExceptionFilter(TopLevelFilter);
}

LONG MiniDumper::TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
	HMODULE hDll = NULL;
	char buf[_MAX_PATH];
	std::string appPath, dbgHelpPath, dumpPath;

		// look for dbghelp.dll in program dir
	if (GetModuleFileName(NULL, buf, _MAX_PATH))	
	{
		appPath = buf;
		appPath = appPath.substr(0, appPath.rfind('\\')+1);
		dbgHelpPath = appPath + "DBGHELP.DLL";
		hDll = ::LoadLibrary(dbgHelpPath.c_str());
	}

		// load any version we can find
	if (hDll == NULL)
		hDll = ::LoadLibrary("DBGHELP.DLL");	

		// could not find dll anywhere
	if (hDll == NULL)
	{
		::MessageBox(NULL, "DBGHELP.DLL not found", appName.c_str(), MB_OK);
		return EXCEPTION_CONTINUE_SEARCH;
	}

		// get address of dumping procedure from dll
	MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
	if (pDump == NULL)
	{
		::MessageBox(NULL, "DBGHELP.DLL too old", appName.c_str(), MB_OK);
		return EXCEPTION_CONTINUE_SEARCH;
	}

		// naming file after current time will make it 99% unique
	time_t ticks = time(0);
#ifdef USING_VISUAL_2005
	tm now;
	localtime_s(&now, &ticks);
	strftime(buf, sizeof(buf), "%y%m%d%H%M%S.dmp", &now);
#else
	strftime(buf, sizeof(buf), "%y%m%d%H%M%S.dmp", localtime(&ticks));
#endif //USING_VISUAL_2005
	dumpPath = appPath + buf;

		// create the file
	HANDLE hFile = ::CreateFile(dumpPath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL, NULL);

		// failed to create file
	if (hFile == INVALID_HANDLE_VALUE)
	{
		::MessageBox(NULL, "Failed to create dump file", appName.c_str(), MB_OK);
		return EXCEPTION_CONTINUE_SEARCH;
	}

		// fill exception info struct
	_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	ExInfo.ThreadId = ::GetCurrentThreadId();
	ExInfo.ExceptionPointers = pExceptionInfo;
	ExInfo.ClientPointers = NULL;

		// save the dump to file
	if (!pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL))
	{
		::MessageBoxA(NULL, "Failed to save dump file", appName.c_str(), MB_OK);
		return EXCEPTION_CONTINUE_SEARCH;
	}

		// close file and exit
	::CloseHandle(hFile);
	return EXCEPTION_EXECUTE_HANDLER;
}

#endif //MSVC_EXCEPTION_TRACER

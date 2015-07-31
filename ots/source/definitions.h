#ifndef __definitions_h
#define __definitions_h

#ifdef XML_GCC_FREE
#define xmlFreeOTSERV(s)	free(s)
#else
#define xmlFreeOTSERV(s)	xmlFree(s)
#endif

#ifdef __LINUX__
#define _timeb  timeb
#define _ftime  ftime
#define _atoi64 atoll
#define _int64 int64_t
#define __int64 int64_t
#define OTSYS_THREAD_RETURN void*
#define SOCKET_ERROR -1
#define ERROR_EINTR EINTR
//#define int64_t int64_t

#else //windows

#define OTSYS_THREAD_RETURN  void

#define EWOULDBLOCK WSAEWOULDBLOCK
#ifndef __GNUC__
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned char uint8_t;
typedef signed char int8_t;
#endif

#pragma warning(disable:4786) // msvc too int32_t debug names in stl
#pragma warning(disable:4100) // jak to wkurwia...

#ifdef MIN
#undef MIN
#endif

#ifdef MAX
#undef MAX
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif

#endif // __definitions_h


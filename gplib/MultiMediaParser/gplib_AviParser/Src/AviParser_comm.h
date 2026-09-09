#ifndef __AVIPARSER_COMM_H__
#define __AVIPARSER_COMM_H__

#include "AviIndex.h"

/////////////////////////////////////////////////////////////////////
// program build option
// #define DYNAMIC_MEMORY_CHECK		// check dynamic memory

/////////////////////////////////////////////////////////////////////
// platform dependent
#ifdef WIN32
	#include "stdio.h"
	#include "io.h"
	#include "stdlib.h"
	#include "memory.h"

	#define READ(fid, buf, cnt)	if(fread((buf), 1, (cnt), (fid))!=(unsigned)(cnt)) RETURN(MEDIAPARSER_FILE_READ_ERR);
	#define DBG_PRINT			printf
	#define SEEK				fseek
	#define FTELL				ftell
	#define FILELENGTH(fid)		filelength(fileno(fid))

	#define MEMSET				memset
	#define NEW_LINE			"\n"
	#define ENABLE_DEBUG_INFO
	#define INVALID_FILE_ID		0
#else
	//__CC_ARM
	#ifdef __cplusplus
	extern "C" {
	#endif
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	#include "application.h"
#else
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
#ifndef GP_FREERTOS
	#include <syscall.h>
#endif
	#include <sys/time.h>
	#include <errno.h>
	#include <sched.h>
	#include <signal.h>
#ifndef GP_FREERTOS
	#include <unistd.h>
#endif
#ifdef GP_FREERTOS
    #include "gplib.h"

    extern void print_string(CHAR *fmt, ...);
#endif
#endif
	#ifdef __cplusplus
	}
	#endif

#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	// extern varaible
	extern int (*pRead)(short fd, unsigned int buf, unsigned int size);
#ifndef SEEK64_EN
	extern int (*pSeek)(short fd, int offset0, short fromwhere);
#else
	extern long long (*pSeek)(short fd, long long offset0, short fromwhere);
#endif

	#define READ(fid, buf, len) \
	{ \
		if(pRead((fid), (unsigned long)(buf), (len))!=(len)) \
			RETURN(MEDIAPARSER_FILE_READ_ERR); \
	}


	#define SEEK(fid, offset, org)	\
	if(org==SEEK_SET) \
	{ \
		if(pSeek((fid), (offset), (org))!=(offset)) RETURN(MEDIAPARSER_FILE_SEEK_ERR); \
	} \
	else \
	{ \
		pSeek((fid), (offset), (org)); \
	}

	__inline long FILELENGTH(int fid)
	{
	#ifdef SEEK64_EN
		long pos = pSeek(fid, 0, SEEK_CUR);
		long ret = pSeek(fid, 0, SEEK_END);
		pSeek(fid, pos, SEEK_SET);
		return ret;
	#else
		long long pos = pSeek(fid, 0, SEEK_CUR);
		long long ret = pSeek(fid, 0, SEEK_END);
		pSeek(fid, pos, SEEK_SET);
		return (long)ret;
	#endif //SEEK64_EN
	}

	#define FTELL(fid)				pSeek((fid), 0, SEEK_CUR)
	#define MEMSET(ptr, c, len)		gp_memset((INT8S*)(ptr), (c), (len))
	#define NEW_LINE				"\r\n"

#elif defined(GP_FREERTOS)
	#define READ(fid, buf, len) \
	{ \
		if(read((fid), (INT32U)(buf), (len))!=(INT32S)(len)) \
			RETURN(MEDIAPARSER_FILE_READ_ERR); \
	}

#ifdef SEEK64_EN
	#define SEEK(fid, offset, org)	\
	if(org==SEEK_SET) \
	{ \
		if(lseek64((fid), (offset), (org))!=(offset)) \
			RETURN(MEDIAPARSER_FILE_SEEK_ERR); \
	} \
	else \
	{ \
		lseek64((fid), (offset), (org)); \
	}

	__inline long long FILELENGTH(int fid)
	{
		long long pos = lseek64(fid, 0, SEEK_CUR);
		long long ret = lseek64(fid, 0, SEEK_END);
		lseek64(fid, pos, SEEK_SET);
		return ret;
	}

	#define FTELL(fid)				lseek64((fid), 0, SEEK_CUR)
#else
	#define SEEK(fid, offset, org)	\
	if(org==SEEK_SET) \
	{ \
		if(lseek((fid), (offset), (org))!=(INT32S)(offset)) \
			RETURN(MEDIAPARSER_FILE_SEEK_ERR); \
	} \
	else \
	{ \
		lseek((fid), (offset), (org)); \
	}

	__inline long FILELENGTH(int fid)
	{
		long pos = lseek(fid, 0, SEEK_CUR);
		long ret = lseek(fid, 0, SEEK_END);
		lseek(fid, pos, SEEK_SET);
		return ret;
	}

	#define FTELL(fid)				lseek((fid), 0, SEEK_CUR)
#endif
	#define MEMSET(ptr, c, len)		memset((void *)(ptr), (c), (len))
	#define MEMCPY(dst, src, len)	memcpy((void*)(dst), (void *)(src), (len))
	#define MALLOC(size)			pvPortMalloc(size)
	#define FREE(addr)				vPortFree((void *)addr);
	#define NEW_LINE				"\r\n"
#else
	#define READ(fid, buf, len) \
	{ \
		if(read((fid), (unsigned char *)(buf), (len))!=(len)) \
			RETURN(MEDIAPARSER_FILE_READ_ERR); \
	}


	#define SEEK(fid, offset, org)	\
	if(org==SEEK_SET) \
	{ \
		if(lseek((fid), (offset), (org))!=(offset)) \
			RETURN(MEDIAPARSER_FILE_SEEK_ERR); \
	} \
	else \
	{ \
		lseek((fid), (offset), (org)); \
	}

	__inline long FILELENGTH(int fid)
	{
		long pos = lseek(fid, 0, SEEK_CUR);
		long ret = lseek(fid, 0, SEEK_END);
		lseek(fid, pos, SEEK_SET);
		return ret;
	}

	#define FTELL(fid)				lseek((fid), 0, SEEK_CUR)
	#define MEMSET(ptr, c, len)		memset((char *)(ptr), (c), (len))
	#define NEW_LINE				"\n"
#endif	//LINUX_SYSTEM

	#if ((defined __OPTIMISE_LEVEL) && (__OPTIMISE_LEVEL<2))
		#define ENABLE_DEBUG_INFO
	#endif

	#define INVALID_FILE_ID		(-1)
	#define __int64 long long
#endif //WIN32

#ifdef ENABLE_DEBUG_INFO
	#define DEBUG_ONLY(x)	{x;}
#else
	#define DEBUG_ONLY(x)	{}
#endif

#ifdef LINUX_SYSTEM
#if 1
	#define DBG_PRINT		printf
#else
	#define DBG_PRINT(...)
#endif
#endif

/////////////////////////////////////////////////////////////////////
// configuration

#define DEFAULT_AUD_SIZE		4096
#define SECTOR_SHIFT			9
#define SECTOR_SIZE				(1<<SECTOR_SHIFT)

#define MAX_INDEX_SECTION		256

/* flags for use in <dwFlags> in AVIFileHdr */
#define AVIF_HASINDEX			0x00000010	// Indicates the AVI file has an index.
#define AVIF_MUSTUSEINDEX		0x00000020	// Indicates that application should use the index,
#define AVIF_ISINTERLEAVED		0x00000100	// Indicates the AVI file is interleaved.
#define AVIF_TRUSTCKTYPE		0x00000800	// Use CKType to find key frames?
#define AVIF_WASCAPTUREFILE		0x00010000	// Indicates the AVI file is a specially allocated file used for capturing real-time video.
#define AVIF_COPYRIGHTED		0x00020000	// Indicates the AVI file contains copyrighted data and software.

/////////////////////////////////////////////////////////////////////
// macro
// extern "C" void Err(int x);
// #define RETURN(x) {ret = x; Err(ret); goto Return;}
#define RETURN(x) {ret = x; goto Return;}

/////////////////////////////////////////////////////////////////////
// debug functions
#ifdef ENABLE_DEBUG_INFO
	extern const char *indent(int i);
	extern const char *fourcc_to_str(long fourcc);
#endif

/////////////////////////////////////////////////////////////////////
// macro
#define MAKEFOURCC(ch0, ch1, ch2, ch3)				\
	((unsigned long)(unsigned char)(ch0)		|	\
	((unsigned long)(unsigned char)(ch1) << 8)   |	\
	((unsigned long)(unsigned char)(ch2) << 16)  |	\
	((unsigned long)(unsigned char)(ch3) << 24))

/////////////////////////////////////////////////////////////////////
// structures

typedef union
{
	long v;
	char s[4];
} FOURCC;

typedef struct
{
	unsigned short	wFormatTag;
	unsigned short	nChannels;
	unsigned long	nSamplesPerSec;
	unsigned long	nAvgBytesPerSec;
	unsigned short	nBlockAlign;
	unsigned short	wBitsPerSample;
} GP_PCMWAVEFORMAT;

typedef struct
{
	unsigned short	wFormatTag;
	unsigned short	nChannels;
	unsigned long	nSamplesPerSec;
	unsigned long	nAvgBytesPerSec;
	unsigned short	nBlockAlign;
	unsigned short	wBitsPerSample;
	unsigned short	cbSize;
} GP_WAVEFORMATEX;


// #include "Vfw.h"

typedef struct
{
	unsigned long biSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short  biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
} GP_BITMAPINFOHEADER;

typedef struct
{
	unsigned long	dwMicroSecPerFrame;		// frame display rate (or 0L)
	unsigned long	dwMaxBytesPerSec;		// max. transfer rate
	unsigned long	dwPaddingGranularity;	// pad to multiples of this
											// size; normally 2K.
	unsigned long	dwFlags;				// the ever-present flags
	unsigned long	dwTotalFrames;			// # frames in file
	unsigned long	dwInitialFrames;
	unsigned long	dwStreams;
	unsigned long	dwSuggestedBufferSize;

	unsigned long	dwWidth;
	unsigned long	dwHeight;

	unsigned long	dwReserved[4];
} GP_MainAVIHeader;

typedef struct {
    FOURCC			fccType;
    FOURCC			fccHandler;
    unsigned long	dwFlags;	/* Contains AVITF_* flags */
    unsigned short	wPriority;
    unsigned short	wLanguage;
    unsigned long	dwInitialFrames;
    unsigned long	dwScale;
    unsigned long	dwRate;	/* dwRate / dwScale == samples/second */
    unsigned long	dwStart;
    unsigned long	dwLength; /* In units above... */
    unsigned long	dwSuggestedBufferSize;
    unsigned long	dwQuality;
    unsigned long	dwSampleSize;
    short			left;
    short			top;
    short			right;
    short			bottom;
} GP_AVIStreamHeader;

typedef struct
{
	FOURCC			fourcc;
	unsigned long   len;
} CHUNKHDR;


typedef struct
{
	GP_AVIStreamHeader	Info;
	const char			*ExData;
	int					ExDataLen;
	CAviIndex			Idx;
	long				flag;
	int					ID;
	int					nBlockAlign;
} AVI_STREAM_INFO;


typedef struct
{
	FILEHANDLE				fid;

	long					flag;

	int						lev;

	short					maxStream;
	short					nStream;

	AVI_STREAM_INFO			Stream[2];

	GP_MainAVIHeader		main_hdr;

	AVI_STREAM_INFO			*aStream;
	AVI_STREAM_INFO			*vStream;

#ifdef SEEK64_EN
	long long				BaseOffset;
	long long 				FileSize;
	long long				movi_pos;
#else
	long					BaseOffset;
	long 					FileSize;
	long					movi_pos;
#endif

	char *ExData;
	int ExDataLen;
} AVIPARSER;

#endif //__AVIPARSER_COMM_H__

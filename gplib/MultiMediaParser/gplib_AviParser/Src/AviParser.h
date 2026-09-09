#ifndef __AVI_PARSER_H__
#define __AVI_PARSER_H__

#ifndef LINUX_SYSTEM
#include "MediaParser.h"
#else
#include "./../libMultiMediaParser/MediaParser.h"
#endif

#ifdef WIN32
	#include "stdio.h"
	#ifndef FILEHANDLE
		#define FILEHANDLE (FILE *)
	#endif
#else
	//__CC_ARM
	#ifndef FILEHANDLE
		#define FILEHANDLE 	int
	#endif
#endif

#ifndef __int64
	#define __int64		long long
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define AVIPARSER_WORKMEM_SIZE		12672

extern const MEDIA_PARSER_FCN_TAB *AviParser_GetFcnTab(void);
extern int AviParser_GetWorkMemSize(void);
extern const char *AviParser_GetVersion(void);

// Avi
#ifdef SEEK64_EN
extern int AviParser_Init(void *hAvi, char *ExData, int ExDataLen, FILEHANDLE fid, __int64 FileSize);
#else
extern int AviParser_Init(void *hAvi, char *ExData, int ExDataLen, FILEHANDLE fid, long FileSize);
#endif
extern void *AviParser_GetAudHandle(void *hAvi);
extern void *AviParser_GetVidHandle(void *hAvi);

// Track level function
extern long Stream_GetTotalSamples(const void *hStream);
extern long Stream_GetTickRate(const void *hStream);
extern long Stream_GetTotalSec(const void *hStream);		// return second (Q8)
extern const char *Stream_GetExData(const void *hStream, long *Len);
extern long Stream_Tick2Sample(void *hStream, __int64 Tick64);	// time to sample
																// To compatible with AVI format, Tick must be 64-bit
extern long Stream_GetFormatFourCC(const void *hStream);
extern int Stream_GetFrameRate(const void *hStream);

// Track stream control
extern int Stream_GetSampleInfo(void *hStream, long tarFrame, MEDIA_PARSER_SAMPLE_INFO *Info);
extern int Stream_GetKeyFrame(void *hStream, long tarFrame);

#ifdef __cplusplus
}
#endif


#endif // __AVI_PARSER_H__

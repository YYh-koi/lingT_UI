#ifndef __QTFF_PARSER_H__
#define __QTFF_PARSER_H__

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
#else //__CC_ARM
	#ifndef FILEHANDLE
		#define FILEHANDLE int
	#endif
#endif

#ifndef __int64
	#define __int64		long long
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define QTFFPARSER_WORKMEM_SIZE		5252

extern const MEDIA_PARSER_FCN_TAB *QtffParser_GetFcnTab(void);
extern int QtffParser_GetWorkMemSize(void);
extern const char *QtffParser_GetVersion(void);

// Qtff initial
#ifndef SEEK64_EN
extern int QtffParser_Init(void *hQtff, char *ExData, int ExDataLen, FILEHANDLE fid, long FileSize);
#else
extern int QtffParser_Init(void *hQtff, char *ExData, int ExDataLen, FILEHANDLE fid, __int64 FileSize);
#endif
extern void *QtffParser_GetAudHandle(void *hQtff);
extern void *QtffParser_GetVidHandle(void *hQtff);
extern int QtffParser_GetAudTrkCnt(void *hQtff);
extern int QtffParser_GetVidTrkCnt(void *hQtff);
extern void *QtffParser_SelectAudTrack(void *hQtff, int trk_no);
extern void *QtffParser_SelectVidTrack(void *hQtff, int trk_no);

// Track level function
extern long Track_GetTotalSamples(const void *hTrk);
extern long Track_GetTickRate(const void *hTrk);
extern long Track_GetTotalSec(const void *hTrk);		// return second (Q8)
extern const char *Track_GetExData(const void *hTrk, long *Len);
extern long Track_Tick2Sample(void *hTrk, __int64 Tick64);	// time to sample
															// To compatible with AVI format, Tick must be 64-bit
extern long Track_GetFormatFourCC(const void *hTrk);
extern int Track_GetFrameRate(const void *hTrck);

// Track stream control
extern int Track_GetSampleInfo(void *hTrk, long tarSample, MEDIA_PARSER_SAMPLE_INFO *Info);
extern int Track_GetKeyFrame(void *hTrk, long tarSample);

// other
extern char *QtffParser_GetavcCData(void *hQtff, long *Len);

#ifdef __cplusplus
}
#endif


#endif // __QTFF_PARSER_H__
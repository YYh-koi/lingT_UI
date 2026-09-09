#include "AviParser.h"
#include "AviParser_comm.h"

/////////////////////////////////////////////////////////////////////
//define
#define C_CHECK_SIZE_EN		0

/////////////////////////////////////////////////////////////////////
//debug
#ifdef ENABLE_DEBUG_INFO
const char *indent(int i)
{
	const char *ind = "                                ";
	return ind + 32 - (i<<1);
}

static char buf[5] = {0,0,0,0,0};
const char *fourcc_to_str(long fourcc)
{
	*(long*)buf = fourcc;
	return buf;
}
#endif

#ifdef ENABLE_DEBUG_INFO
AVIPARSER *g_avi = 0;
void *g_index_aud = 0;
void *g_index_vid = 0;
#endif


/////////////////////////////////////////////////////////////////////
// flags
#define FLAG_MAIN_HEADER_READ	0x00000001
#define	FLAG_AUD_INDEX20_READ	0x00000010
#define	FLAG_VID_INDEX20_READ	0x00000020
#define	FLAG_AUD_IDX1_READ		0x00000040
#define	FLAG_VID_IDX1_READ		0x00000080


static const MEDIA_PARSER_FCN_TAB AviParserFcnTab =
{
	13,
	AviParser_Init,
	AviParser_GetWorkMemSize,
	NULL,
	NULL,
	AviParser_GetAudHandle,
	AviParser_GetVidHandle,
	NULL,
	NULL,

	// Track level function
	Stream_GetTotalSamples,
	Stream_GetTickRate,
	Stream_GetTotalSec,
	Stream_GetExData,
	Stream_Tick2Sample,
	Stream_GetFormatFourCC,
	Stream_GetFrameRate,

	// Track stream control
	Stream_GetSampleInfo,
	Stream_GetKeyFrame,

	// other
	NULL,
};

const MEDIA_PARSER_FCN_TAB *AviParser_GetFcnTab()
{
	return &AviParserFcnTab;
}

extern "C"
unsigned long AviParser_udiv32by32(unsigned long d, unsigned long n)
{
	int i;
	unsigned ret = 0;
	unsigned long high = 0;

	i = 32;
	do
	{
		if(high >= d)
		{
			high -= d;
			ret |= 1;
		}

		high <<= 1;
		high |= n >> 31;
		n <<= 1;

		ret <<= 1;
	} while(--i);
	if(high >= d) ret |= 1;

	return ret;
}

static unsigned __int64 AviParser_udiv64by32(unsigned long d, unsigned __int64 n64)
{
	int i;
	/*register*/ unsigned long high = 0;

	/*register*/ unsigned long mid = (unsigned long)(n64 >> 32);
	/*register*/ unsigned long low = (unsigned long)n64;

	/*register*/ unsigned long retHigh = 0;
	/*register*/ unsigned long retLow = 0;

	i = 64;
	do
	{
		if(high >= d)
		{
			high -= d;
			retLow |= 1;
		}

		high <<= 1;
		high |= mid >> 31;
		mid <<= 1;
		mid |= low >> 31;
		low <<= 1;

		retHigh <<= 1;
		retHigh |= retLow >> 31;
		retLow <<= 1;
	} while(--i);
	if(high >= d) retLow |= 1;

	return (unsigned __int64)retHigh << 32 | retLow;
}

// Avi initial
// MEDIAPARSER_FILE_READ_ERR
// MEDIAPARSER_FORMAT_ERR
// MEDIAPARSER_MEM_ERR
// static char buf[5] = {0,0,0,0,0};
#ifndef SEEK64_EN
static int RiffParser(AVIPARSER *avi, unsigned long ParentRest, unsigned long Parent)
#else
static long long RiffParser(AVIPARSER *avi, unsigned long ParentRest, unsigned long Parent)
#endif
{
	unsigned long type;
	CHUNKHDR hdr;
	unsigned long cklen, pos;
#ifndef SEEK64_EN
	int ret = 0;
#else
	long long ret = 0;
#endif
	unsigned long orglen, t;
	AVI_STREAM_INFO *pStream;

#ifdef ENABLE_DEBUG_INFO
	int new_line = 0;
#endif

	if(ParentRest<sizeof(CHUNKHDR))
		RETURN(ParentRest);

	READ(avi->fid, &hdr, sizeof(CHUNKHDR));
	ParentRest -= 8;

	if(hdr.len>ParentRest)
	{
	#if C_CHECK_SIZE_EN == 1
		DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %d, %XH)", indent(avi->lev), fourcc_to_str(hdr.fourcc.v), hdr.len, hdr.len));
		RETURN(MEDIAPARSER_FORMAT_ERR);
	#else
		hdr.len = ParentRest;
		DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %d, %XH)", indent(avi->lev), fourcc_to_str(hdr.fourcc.v), hdr.len, hdr.len));
	#endif
	}
	else
	{
		DEBUG_ONLY(DBG_PRINT((char*)"%s%s(%d, %XH)", indent(avi->lev), fourcc_to_str(hdr.fourcc.v), hdr.len, hdr.len));
	}

	orglen = hdr.len;
	if(hdr.len & 1) hdr.len ++;
	cklen = 0;


	switch(hdr.fourcc.v)
	{
	case MAKEFOURCC('R','I','F','F'):
	case MAKEFOURCC('L','I','S','T'):
		READ(avi->fid, &type, sizeof(unsigned long));

		DEBUG_ONLY(DBG_PRINT((char*)"-%s", fourcc_to_str(type)));

		cklen += 4;
		switch(type)
		{
		case MAKEFOURCC('s','t','r','l'):
			if(Parent!=MAKEFOURCC('h','d','r','l')) goto SkipSubChunk;
			avi->maxStream++;
			goto ParseSubChunk;

		case MAKEFOURCC('A','V','I',' '):
			if(Parent!=0) goto SkipSubChunk;
			goto ParseSubChunk;

		case MAKEFOURCC('h','d','r','l'):
			if(Parent!=MAKEFOURCC('A','V','I',' ')) goto SkipSubChunk;
			goto ParseSubChunk;

		case MAKEFOURCC('m','o','v','i'):
			if(Parent!=MAKEFOURCC('A','V','I',' ')) goto SkipSubChunk;
			avi->movi_pos = FTELL(avi->fid) - 4;
			goto SkipSubChunk;

		default:
			goto SkipSubChunk;
		}

		ParseSubChunk:
			DEBUG_ONLY(DBG_PRINT((char*)NEW_LINE));
			DEBUG_ONLY(new_line = 1);
			// parse sub-chunk
			avi->lev++;
			while(cklen<hdr.len)
			{
				ret = RiffParser(avi, hdr.len - cklen, type);
				if(ret<=0) RETURN(ret);
				cklen += ret;
			}
			avi->lev--;

		SkipSubChunk:
		SEEK(avi->fid, hdr.len - cklen, SEEK_CUR);
		cklen = hdr.len;
		break;

	case MAKEFOURCC('a','v','i','h'):
		if(Parent!=MAKEFOURCC('h','d','r','l')) goto SkipSubChunk;
		if(!(avi->flag & FLAG_MAIN_HEADER_READ))
		{
			// READ_STRUCT(&avi->main_hdr, GP_MainAVIHeader);
			if(hdr.len<sizeof(GP_MainAVIHeader)) RETURN(MEDIAPARSER_FORMAT_ERR);
			READ(avi->fid, &avi->main_hdr, sizeof(GP_MainAVIHeader));

			cklen += sizeof(GP_MainAVIHeader);

			avi->flag |= FLAG_MAIN_HEADER_READ;

		#if C_CHECK_SIZE_EN == 1
			//check index exist
			if((avi->main_hdr.dwFlags & AVIF_HASINDEX) == 0) {
				RETURN(MEDIAPARSER_NO_INDEX);
			}
		#endif
		}
		SEEK(avi->fid, hdr.len - cklen, SEEK_CUR);
		cklen = hdr.len;
		break;

	case MAKEFOURCC('s','t','r','h'):
		if(Parent!=MAKEFOURCC('s','t','r','l')) goto SkipSubChunk;
		if(avi->nStream<2)
		{
			// if(hdr.len<sizeof(GP_AVIStreamHeader))
			//	RETURN(MEDIAPARSER_FORMAT_ERR);
			// READ(avi->fid, &avi->Stream[avi->nStream].Info, sizeof(GP_AVIStreamHeader));
			if(hdr.len<sizeof(GP_AVIStreamHeader))
			{
				READ(avi->fid, &avi->Stream[avi->nStream].Info, hdr.len);
				cklen = hdr.len;
			}
			else
			{
				READ(avi->fid, &avi->Stream[avi->nStream].Info, sizeof(GP_AVIStreamHeader));
				cklen = sizeof(GP_AVIStreamHeader);
			}
		}
		SEEK(avi->fid, hdr.len - cklen, SEEK_CUR);
		cklen = hdr.len;
		break;

	case MAKEFOURCC('s','t','r','f'):
		if(Parent!=MAKEFOURCC('s','t','r','l')) goto SkipSubChunk;
		if(avi->Stream[avi->nStream].Info.fccType.v==MAKEFOURCC('a','u','d','s') && avi->aStream==0)
		{
			GP_PCMWAVEFORMAT *fmt;
			GP_AVIStreamHeader *Info = &avi->Stream[avi->nStream].Info;

			avi->aStream = avi->Stream + avi->nStream;
			avi->aStream->ID = avi->maxStream - 1;

			// READ_STRUCT(&avi->aFmt, PCMWAVEFORMAT);
			if(hdr.len<sizeof(GP_PCMWAVEFORMAT)) RETURN(MEDIAPARSER_FORMAT_ERR);
			if(orglen>(unsigned long)(avi->ExDataLen)) RETURN(MEDIAPARSER_EXDATA_OVERFLOW);

			avi->aStream->ExData = avi->ExData;
			fmt = (GP_PCMWAVEFORMAT*)avi->ExData;
			avi->aStream->ExDataLen = orglen;

			READ(avi->fid, avi->ExData, orglen);

			t = (orglen + 3) >> 2 << 2;
			avi->ExData += t;
			avi->ExDataLen -= t;

			// to avoid file format error
			if(fmt->wFormatTag==0x0055) // MP3 only
			{
				if(fmt->nBlockAlign!=1 || Info->dwScale != 1 || Info->dwRate!=fmt->nAvgBytesPerSec)
				{
					long dwScale, dwRate, dwLength;
					fmt->nBlockAlign = 1;	// modify nBlockAlign to correct value
					dwScale = Info->dwScale;
					dwRate = Info->dwRate;
					dwLength = Info->dwLength;

					Info->dwScale = 1;
					Info->dwRate = fmt->nAvgBytesPerSec;
					// Info->dwLength = (unsigned long)(((__int64)dwLength * dwScale  * ptr->nAvgBytesPerSec) / dwRate);
					Info->dwLength = (unsigned long) AviParser_udiv64by32(dwRate, (__int64)dwLength * dwScale  * fmt->nAvgBytesPerSec);
				}
			}

			avi->aStream->nBlockAlign = fmt->nBlockAlign;
			avi->nStream++;
			cklen += hdr.len;
		}
		else if(avi->Stream[avi->nStream].Info.fccType.v==MAKEFOURCC('v','i','d','s') && avi->vStream==0)
		{
			avi->vStream = avi->Stream + avi->nStream;
			avi->vStream->ID = avi->maxStream - 1;
			// READ_STRUCT(&avi->vFmt, BITMAPINFOHEADER);
			if(hdr.len<sizeof(GP_BITMAPINFOHEADER)) RETURN(MEDIAPARSER_FORMAT_ERR);
			if(orglen>(unsigned long)(avi->ExDataLen)) RETURN(MEDIAPARSER_EXDATA_OVERFLOW);

			avi->vStream->ExData = avi->ExData;
			avi->vStream->ExDataLen = orglen;

			READ(avi->fid, avi->ExData, orglen);

			t = (orglen + 3) >> 2 << 2;
			avi->ExData += t;
			avi->ExDataLen -= t;

			avi->vStream->nBlockAlign = 0;
			avi->nStream++;
			cklen += hdr.len;
		}

		SEEK(avi->fid, hdr.len - cklen, SEEK_CUR);
		cklen = hdr.len;
		break;
	case MAKEFOURCC('i','n','d','x'):
		if(Parent!=MAKEFOURCC('s','t','r','l')) goto SkipSubChunk;

		pos = FTELL(avi->fid);
		pStream = avi->Stream + avi->nStream - 1;

		AVIINDEXARGUMENT arg20;
		arg20.fid = avi->fid;
		arg20.BaseOffset = avi->BaseOffset;
		arg20.ChunkSize = hdr.len;
		arg20.nBlockAlign = pStream->nBlockAlign;

		if(pStream->Info.fccType.v == MAKEFOURCC('v','i','d','s') && !(avi->flag & FLAG_VID_INDEX20_READ))
		{
			ret = pStream->Idx.Init(2, &arg20);
			if(ret>=0) avi->flag |= FLAG_VID_INDEX20_READ;
			DEBUG_ONLY(g_index_aud = &pStream->Idx);
		}
		else if(pStream->Info.fccType.v == MAKEFOURCC('a','u','d','s') && !(avi->flag & FLAG_AUD_INDEX20_READ))
		{
			ret = pStream->Idx.Init(2, &arg20);
			if(ret>=0) avi->flag |= FLAG_AUD_INDEX20_READ;
			DEBUG_ONLY(g_index_vid = &pStream->Idx);
		}
		SEEK(avi->fid, pos + hdr.len, SEEK_SET);
		cklen = hdr.len;
		break;

	case MAKEFOURCC('i','d','x','1'):
		if(Parent!=MAKEFOURCC('A','V','I',' ') && Parent!=0) goto SkipSubChunk;
		pos = FTELL(avi->fid);

		// Read first index to decide index10 offset type
		long t_idx[4];
		READ(avi->fid, t_idx, sizeof(t_idx));
		SEEK(avi->fid, pos, SEEK_SET);

		AVIINDEXARGUMENT arg10;
		arg10.fid = avi->fid;
		if(t_idx[2] != 4 && avi->movi_pos - avi->BaseOffset + 4 == t_idx[2])
		{
			// base on file head
			arg10.BaseOffset = avi->BaseOffset + 8;
		}
		else
		{
			// based on movi position
			arg10.BaseOffset = avi->movi_pos + 8;
		}
		arg10.ChunkSize = hdr.len;

		if(avi->aStream && (avi->flag & FLAG_AUD_INDEX20_READ) == 0)
		{
			pStream = avi->aStream;

			arg10.nBlockAlign = pStream->nBlockAlign;
			arg10.ID = pStream->ID;
			arg10.IsVideo = 0;
			arg10.SectionSize = (pStream->Info.dwLength + MAX_INDEX_SECTION - 1)/ MAX_INDEX_SECTION;
			if(arg10.nBlockAlign) arg10.SectionSize *= arg10.nBlockAlign;

			ret = pStream->Idx.Init(1, &arg10);
			if(ret<0) RETURN(ret);
			DEBUG_ONLY(g_index_aud = &pStream->Idx);
			SEEK(avi->fid, pos, SEEK_SET);
			avi->flag |= FLAG_AUD_IDX1_READ;
		}

		if(avi->vStream && (avi->flag & FLAG_VID_INDEX20_READ) == 0)
		{
			pStream = avi->vStream;

			arg10.nBlockAlign = pStream->nBlockAlign;
			arg10.ID = pStream->ID;
			arg10.IsVideo = 1;
			arg10.SectionSize = (pStream->Info.dwLength + MAX_INDEX_SECTION - 1)/ MAX_INDEX_SECTION;

			ret = pStream->Idx.Init(1, &arg10);
			if(ret<0) RETURN(ret);
			DEBUG_ONLY(g_index_vid = &pStream->Idx);
			avi->flag |= FLAG_VID_IDX1_READ;
		}

		SEEK(avi->fid, pos + hdr.len, SEEK_SET);
		cklen = hdr.len;
		break;

	case 0:
		RETURN(MEDIAPARSER_FORMAT_ERR);
		break;

	default:
		SEEK(avi->fid, hdr.len - cklen, SEEK_CUR);
		cklen = hdr.len;
	}
	DEBUG_ONLY(if(!new_line) DBG_PRINT((char*)NEW_LINE););

	ret = 0;

Return:
	if(ret<0) return ret;
	else return hdr.len + 8;
}

#ifdef SEEK64_EN
int AviParser_Init(void *hAvi, char *ExData, int ExDataLen, FILEHANDLE fid, __int64 FileSize)
#else
int AviParser_Init(void *hAvi, char *ExData, int ExDataLen, FILEHANDLE fid, long FileSize)
#endif
{
	int ret;
#ifdef SEEK64_EN
	long long t, parser_ret;
#else
	int t, parser_ret;
#endif
	// long FileRestLength;
	AVIPARSER *avi = (AVIPARSER *)hAvi;

	// when FileSize = 0, into resume play init
	if(FileSize == 0 ) {
		unsigned int i, temp;
		AVIINDEXARGUMENT arg;
		AVI_STREAM_INFO *pStream;

		DEBUG_ONLY(DBG_PRINT((char*)"AviParser_InitResumePlay" NEW_LINE));
		DEBUG_ONLY(g_avi = avi;);

		for(i=0; i<2; i++) {
			pStream = avi->Stream + i;
			switch(pStream->Info.fccType.v)
			{
			case MAKEFOURCC('a','u','d','s'):
				DEBUG_ONLY(DBG_PRINT((char*)"auds" NEW_LINE));
				avi->aStream = avi->Stream + i;
				break;

			case MAKEFOURCC('v','i','d','s'):
				DEBUG_ONLY(DBG_PRINT((char*)"vids" NEW_LINE));
				avi->vStream = avi->Stream + i;
				break;

			default:
				RETURN(MEDIAPARSER_PARAM_ERR);
			}
		}

		temp = avi->vStream->ExDataLen;
		if(temp & 0x03) {
			temp = temp >> 2 << 2;
			temp += 0x04;
		}

		temp += avi->aStream->ExDataLen;
		if(temp & 0x03) {
			temp = temp >> 2 << 2;
			temp += 0x04;
		}

		avi->fid = fid;
		avi->ExData = ExData + temp;

		if(avi->aStream && avi->vStream) {
			if(avi->aStream->ExData < avi->vStream->ExData) {
				temp = (long)avi->vStream->ExData - (long)avi->aStream->ExData;
				avi->aStream->ExData = ExData;
				avi->vStream->ExData = ExData + temp;
			} else {
				temp = (long)avi->aStream->ExData - (long)avi->vStream->ExData;
				avi->vStream->ExData = ExData;
				avi->aStream->ExData = ExData + temp;
			}
		} else if(avi->aStream) {
			avi->aStream->ExData = ExData;
		} else if(avi->vStream) {
			avi->vStream->ExData = ExData;
		}

		arg.fid = fid;
		if(avi->aStream) {
			ret = avi->aStream->Idx.InitResumePlay(&arg);
			if(ret < 0) {
				RETURN(ret);
			}
		}

		if(avi->vStream) {
			ret = avi->vStream->Idx.InitResumePlay(&arg);
			if(ret < 0) {
				RETURN(ret);
			}
		}

		RETURN(ret);
	}

	DEBUG_ONLY(DBG_PRINT((char*)"AviParser_Init" NEW_LINE));
	DEBUG_ONLY(g_avi = avi;);

	MEMSET(avi, 0, sizeof(AVIPARSER));

	avi->fid = fid;

	avi->ExData = ExData;
	avi->ExDataLen = ExDataLen;

	avi->BaseOffset = FTELL(fid);
	if(FileSize<=0)
	{
		FileSize = FILELENGTH(fid) - avi->BaseOffset;
	}
	avi->FileSize = FileSize;
	FileSize = FileSize >> 1 << 1;

	t = 0;
	while(t<FileSize)
	{
		parser_ret = RiffParser(avi, FileSize - t, 0);
		if(parser_ret<=0) RETURN(parser_ret);
		t += parser_ret;
	}

	if(!(avi->flag & (FLAG_AUD_IDX1_READ | FLAG_AUD_INDEX20_READ)))
		avi->aStream = NULL;

	if(!(avi->flag & (FLAG_VID_IDX1_READ | FLAG_VID_INDEX20_READ)))
		avi->vStream = NULL;

	// check AVIPARSER format
	if(!(avi->flag & FLAG_MAIN_HEADER_READ))
		RETURN(MEDIAPARSER_FORMAT_ERR);
	if(avi->movi_pos==0)
		RETURN(MEDIAPARSER_FORMAT_ERR);
	if(avi->aStream && avi->aStream->ExData==0)
		avi->aStream = NULL;
	if(avi->vStream && avi->vStream->ExData==0)
		avi->vStream = NULL;
	if(avi->aStream==0 && avi->vStream==0) RETURN(MEDIAPARSER_NO_STREAM_EXIST);

	ret = 0;
Return:
	return ret;
}


int AviParser_GetWorkMemSize(void)
{
	return sizeof(AVIPARSER);
}

void *AviParser_GetAudHandle(void *hAvi)
{
	AVIPARSER *avi = (AVIPARSER *)hAvi;
	return avi->aStream;
}

void *AviParser_GetVidHandle(void *hAvi)
{
	AVIPARSER *avi = (AVIPARSER *)hAvi;
	return avi->vStream;
}

// Track level function
long Stream_GetTotalSamples(const void *hStream)
{
	const AVI_STREAM_INFO *Stream = (const AVI_STREAM_INFO*)hStream;
	return Stream->Info.dwLength;
}

long Stream_GetTickRate(const void *hStream)
{
	const AVI_STREAM_INFO *Stream = (const AVI_STREAM_INFO*)hStream;
	return Stream->Info.dwRate;
}

int Stream_GetFrameRate(const void *hStream)
{
	int rate = 0;
	const AVI_STREAM_INFO *Stream = (const AVI_STREAM_INFO*)hStream;
	rate = Stream->Info.dwRate/Stream->Info.dwScale;
	return rate;
}

long Stream_GetTotalSec(const void *hStream)
{
	const AVI_STREAM_INFO *Stream = (const AVI_STREAM_INFO*)hStream;
	// return second (Q8)
	// return (long)((__int64)(Stream->Info.dwLength << 8) * Stream->Info.dwScale / Stream->Info.dwRate);
	return (long)AviParser_udiv64by32(Stream->Info.dwRate, (__int64)(Stream->Info.dwLength << 8) * Stream->Info.dwScale);
}

const char *Stream_GetExData(const void *hStream, long *Len)
{
	const AVI_STREAM_INFO *Stream = (const AVI_STREAM_INFO*)hStream;
	*Len = Stream->ExDataLen;
	return Stream->ExData;
}

// To compatible with AVI format, Tick must be 64-bit
long Stream_Tick2Sample(void *hStream, __int64 Tick64)
{
	const AVI_STREAM_INFO *Stream = (const AVI_STREAM_INFO*)hStream;
	if(Tick64<0)
		return 0;
	else
	{
		// return (long)(Tick64 / Stream->Info.dwScale);
		return (long)AviParser_udiv64by32(Stream->Info.dwScale, Tick64);
	}
}	// time to sample

long Stream_GetFormatFourCC(const void *hStream)
{
	const AVI_STREAM_INFO *Stream = (const AVI_STREAM_INFO*)hStream;
	return Stream->Info.fccHandler.v;
}

// Track stream control

int Stream_GetSampleInfo(void *hStream, long tarFrame, MEDIA_PARSER_SAMPLE_INFO *Info)
{
	int ret;
	AVI_STREAM_INFO *Stream = (AVI_STREAM_INFO*)hStream;
	CAviIndex *Idx = &Stream->Idx;

	if(tarFrame<0) RETURN(0);

	ret = Idx->Get(tarFrame, &Info->Offset, &Info->Size, &Info->IsKey);
	if(ret<0) RETURN(ret);
	if(ret)
	{
		if(Stream->nBlockAlign == 0) Info->Count = 1;
		else Info->Count = AviParser_udiv32by32(Stream->nBlockAlign, Info->Size);
		Info->Tick64 = (__int64)tarFrame * Stream->Info.dwScale;
		Info->dTick = Info->Count * Stream->Info.dwScale;

		RETURN(1);
	}
	else
	{
		Info->Count = 0;
		Info->Tick64 = (__int64)Idx->GetNumberOfFrames() * Stream->Info.dwScale;
		Info->dTick = 0;

		RETURN(0);
	}


Return:
	return ret;
}

int Stream_GetKeyFrame(void *hStream, long tarFrame)
{
	int ret;
	AVI_STREAM_INFO *Stream = (AVI_STREAM_INFO*)hStream;
	CAviIndex *Idx = &Stream->Idx;

	if(Stream->Info.fccType.v == MAKEFOURCC('v','i','d','s'))
	{
		if(tarFrame<=0)
		{
			tarFrame = 0;
		}
		else
		{
		#ifdef SEEK64_EN
			long long Offset;
		#else
			long Offset;
		#endif
			long Size, IsKey;
			while(1)
			{
				ret = Idx->Get(tarFrame, &Offset, &Size, &IsKey);
				if(ret<0) RETURN(ret);
				if(ret==0 || IsKey)
					break;
				tarFrame++;
			}
		}
	}

	ret = tarFrame;
Return:
	return ret;
}


const char *AviParser_GetVersion(void)
{
	return "GP$308 Generalplus AVI Parser 2014.04.15";
}

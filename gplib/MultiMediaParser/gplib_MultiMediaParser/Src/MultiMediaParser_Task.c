
#include "gplib.h"
//#include "MediaParser.h"
#include "MultiMediaParser_def.h"
#include "MultiMediaParser.h"
#include "MultiMediaParser_Version.h"

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
#include <typedef.h>

#ifdef SPECIAL_MEM_MALLOC
#include <chunkmem.h>
#include <mach/gp_chunkmem.h>
#endif
#endif

/////////////////////////////////////////////////////////////////////
// program build option
// #define DYNAMIC_MEMORY_CHECK		// check dynamic memory

/////////////////////////////////////////////////////////////////////
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
// read and seek
int (*pRead)(short fd, unsigned int buf, unsigned int size);
#ifndef SEEK64_EN
int (*pSeek)(short fd, int offset0, short fromwhere);
#else
long long (*pSeek)(short fd, long long offset0, short fromwhere);
#endif

// resume play
int (*pfuncStore)(unsigned int buffer, unsigned int size);
int (*pfuncLoad)(unsigned int buffer, unsigned int size);

#define READ(fid, buf, len) \
{ \
	if(pRead((fid), (unsigned long)(buf), (len))!=(len)) \
		RETURN(MEDIAPARSER_FILE_READ_ERR); \
}

#define MAKEFOURCC(ch0, ch1, ch2, ch3)				\
	((unsigned long)(unsigned char)(ch0)		|	\
	((unsigned long)(unsigned char)(ch1) << 8)   |	\
	((unsigned long)(unsigned char)(ch2) << 16)  |	\
	((unsigned long)(unsigned char)(ch3) << 24))

#define SEEK(fid, offset, org)	pSeek((fid), (offset), (org))
#define FTELL(fid)				pSeek((fid), 0, SEEK_CUR)
#define MEMSET(ptr, c, len)		gp_memset((INT8S*)(ptr), (c), (len))
#define MEMCPY(ptr, src, len)	gp_memcpy((INT8S*)(ptr), (INT8S*)(src), (len))

#ifdef DYNAMIC_MEMORY_CHECK
	extern void FREE(void *ptr_);
	extern void *MALLOC(int Len);
	extern void MemoryCheck(void);
#else
	#define MALLOC(len)		gp_malloc_align((len), 16)
	#define FREE			gp_free
	#define MemoryCheck()
#endif

#elif defined(GP_FREERTOS)
// resume play
int (*pfuncStore)(unsigned int buffer, unsigned int size);
int (*pfuncLoad)(unsigned int buffer, unsigned int size);

extern INT64S lseek64(INT16S handle, INT64S offset0,INT16S fromwhere);
extern INT32S lseek(INT16S handle, INT32S offset0,INT16S fromwhere);

#define READ(fid, buf, len) \
{ \
	if(read((fid), (INT32U)(buf), (len))!=(len)) \
		RETURN(MEDIAPARSER_FILE_READ_ERR); \
}

#define MAKEFOURCC(ch0, ch1, ch2, ch3)				\
	((unsigned long)(unsigned char)(ch0)		|	\
	((unsigned long)(unsigned char)(ch1) << 8)   |	\
	((unsigned long)(unsigned char)(ch2) << 16)  |	\
	((unsigned long)(unsigned char)(ch3) << 24))

#ifdef SEEK64_EN
#define SEEK(fid, offset, org)	lseek64((fid), (offset), (org))
#define FTELL(fid)				lseek64((fid), 0, SEEK_CUR)
#else
#define SEEK(fid, offset, org)	lseek((fid), (offset), (org))
#define FTELL(fid)				lseek((fid), 0, SEEK_CUR)
#endif
#define MEMSET(ptr, c, len)		memset((INT8S*)(ptr), (c), (len))
#define MEMCPY(ptr, src, len)	memcpy((INT8S*)(ptr), (INT8S*)(src), (len))

#ifdef SPECIAL_MEM_MALLOC
	#define osMalloc(x)	gpChunkMemAlloc(x)
	#define osFree(x)	gpChunkMemFree((void *)(x))
#else
	#define osMalloc(x)	pvPortMalloc(x)
	#define osFree(x)	vPortFree((void *)(x))
#endif

#ifdef DYNAMIC_MEMORY_CHECK
	extern void FREE(void *ptr_);
	extern void *MALLOC(int Len);
	extern void MemoryCheck(void);
#else
	#define MALLOC(len)		osMalloc(len)
	#define FREE			osFree
	#define MemoryCheck()
#endif

#else

// resume play
int (*pfuncStore)(unsigned int buffer, unsigned int size);
int (*pfuncLoad)(unsigned int buffer, unsigned int size);

#define READ(fid, buf, len) \
{ \
	if(read((fid), (unsigned long *)(buf), (len))!=(len)) \
		RETURN(MEDIAPARSER_FILE_READ_ERR); \
}

#define MAKEFOURCC(ch0, ch1, ch2, ch3)				\
	((unsigned long)(unsigned char)(ch0)		|	\
	((unsigned long)(unsigned char)(ch1) << 8)   |	\
	((unsigned long)(unsigned char)(ch2) << 16)  |	\
	((unsigned long)(unsigned char)(ch3) << 24))

#define SEEK(fid, offset, org)	lseek((fid), (offset), (org))
#define FTELL(fid)				lseek((fid), 0, SEEK_CUR)
#define MEMSET(ptr, c, len)		memset((INT8S*)(ptr), (c), (len))
#define MEMCPY(ptr, src, len)	memcpy((INT8S*)(ptr), (INT8S*)(src), (len))

#ifdef SPECIAL_MEM_MALLOC
	#define osMalloc(x)	gpChunkMemAlloc(x)
	#define osFree(x)	gpChunkMemFree((void *)(x))
#else
	#define osMalloc(x)	malloc(x)
	#define osFree(x)	free((void *)(x))
#endif

#ifdef DYNAMIC_MEMORY_CHECK
	extern void FREE(void *ptr_);
	extern void *MALLOC(int Len);
	extern void MemoryCheck(void);
#else
	#define MALLOC(len)		osMalloc(len)
	#define FREE			osFree
	#define MemoryCheck()
#endif

#define INT32U	UINT32
#define INT8U	UINT8
#endif

#define RETURN(x) {ret = x; goto Return;}

#if ((defined __OPTIMISE_LEVEL) && (__OPTIMISE_LEVEL<2))
	#define ENABLE_DEBUG_INFO
#endif

#define INVALID_FILE_ID		(-1)
#define __int64 long long

#ifdef ENABLE_DEBUG_INFO
	#define DEBUG_ONLY(x)	{x;}
#else
	#define DEBUG_ONLY(x)	{}
#endif

#ifdef LINUX_SYSTEM
#if 1
	#define DBG_PRINT	printf
#else
	#define DBG_PRINT(...)
#endif
#endif

/////////////////////////////////////////////////////////////////////
// Messages
#define MULTIMEDIAPARSER_ACK_OK		1

/////////////////////////////////////////////////////////////////////
// Flags
#define MMP_FLAG_FOLLOW_MODE		0x0001
#define MMP_FLAG_REVERSE_PLAY		0x0002
#define MMP_FLAG_EOA				0x0004
#define MMP_FLAG_SEEK_TO_KEY		0x0008
#define MMP_FLAG_AUD_READ_END		0x0010
#define MMP_FLAG_LACK_OF_MEM		0x0100
#define MMP_FLAG_AVC1_READ			0x1000
#define MMP_FLAG_ERROR				0x80000000

/////////////////////////////////////////////////////////////////////
// configuration
#define SECTOR_SHIFT			12


void SwapByte32(unsigned char *ptr0, int len)
{
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	register unsigned long t;
	register unsigned long high_mask = 0xFF00FF00;
	register unsigned long low_mask = 0x00FF00FF;
	register unsigned long *src, *des, *ptr_end;

	len = (len + 31) >> 5 << 5;
	if(len<=0) return;
	des = src = (unsigned long*)ptr0;
	ptr_end = (unsigned long*)(ptr0 + len);
	do
	{
	__asm {
		LDMIA	src!, {r1-r8}
		AND		t, low_mask, r1, LSR #8
		AND		r1, high_mask, r1, LSL #8
		ORR		r1, r1, t

		AND		t, low_mask, r2, LSR #8
		AND		r2, high_mask, r2, LSL #8
		ORR		r2, r2, t

		AND		t, low_mask, r3, LSR #8
		AND		r3, high_mask, r3, LSL #8
		ORR		r3, r3, t

		AND		t, low_mask, r4, LSR #8
		AND		r4, high_mask, r4, LSL #8
		ORR		r4, r4, t

		AND		t, low_mask, r5, LSR #8
		AND		r5, high_mask, r5, LSL #8
		ORR		r5, r5, t

		AND		t, low_mask, r6, LSR #8
		AND		r6, high_mask, r6, LSL #8
		ORR		r6, r6, t

		AND		t, low_mask, r7, LSR #8
		AND		r7, high_mask, r7, LSL #8
		ORR		r7, r7, t

		AND		t, low_mask, r8, LSR #8
		AND		r8, high_mask, r8, LSL #8
		ORR		r8, r8, t

		STMIA	des!, {r1-r8}
		}
	} while(des!=ptr_end);
#else
	unsigned long temp, value, i;
	unsigned long *pData = (unsigned long *)ptr0;

	//4 align
	len >>= 2;
	for(i=0; i<len; i++) {
		temp = *(pData + i);
		value = (temp >> 8) & 0x00FF00FF;
		value |= (temp << 8) & 0xFF00FF00;
		*(pData + i) = value;
	}
#endif
}


static unsigned __int64 MMP_udiv64by32(unsigned long d, unsigned __int64 n64)
{
	int i;
	register unsigned long high = 0;

	register unsigned long mid = (unsigned long)(n64 >> 32);
	register unsigned long low = (unsigned long)n64;

	register unsigned long retHigh = 0;
	register unsigned long retLow = 0;

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

// seek audio stream to specific time position (mmpapi->SeekSecQ8)
// this function used by
// 1) ReadVideo if Following mode
// 2) Seek
static int SeekAud(PARSERAPI *mmpapi)
{
	int ret;
	long Time;
	void *aTrk = mmpapi->Parser->GetAudHandle(mmpapi->kernel);

	if(aTrk)
	{
		Time = (long)((__int64)mmpapi->SeekSecQ8 * mmpapi->Parser->GetTickRate(aTrk) >> 8);
		ret = mmpapi->Parser->Tick2Sample(aTrk, Time);
		if(ret<0) RETURN(ret);
		mmpapi->aCnt = ret;
		ret = mmpapi->Parser->GetSampleInfo(aTrk, ret, &mmpapi->aSample);
		if(ret<0) RETURN(ret);
		if(ret) mmpapi->flag &= ~MMP_FLAG_EOA;
		else mmpapi->flag |= MMP_FLAG_EOA;
	}
	ret = 0;
Return:
	return ret;
}

// Read video data from stream
// mmpapi->SeekSecQ8 will be increase by this function
// mode 0: normal read, 1: half buffer size read
long for_fast_Duration,g_pre_offset;
extern INT32U Speed_reverse_flag;
static int ReadVideo(PARSERAPI *mmpapi, INT32U mode)
{
	int ret;
	int WI = mmpapi->VidBufWI;
	const int RI = mmpapi->VidBufRI;
	int FreeBuffers;
	void *vTrk;
	char *ptr;
	long MaxSize, vTimeScale;
	int vCnt, Drop, /*i,*/ IsKey = 0;
	unsigned int i;

	if (mmpapi->NumVidBuf <= 10) {
		FreeBuffers = RI - WI - 1;
		if(FreeBuffers<0) {
			FreeBuffers += mmpapi->NumVidBuf;
		}
	}
	else {
		if(mode == 1) {
			FreeBuffers = mmpapi->NumVidBuf / 2;
		} else {
			FreeBuffers = RI - WI - 1;
			if(FreeBuffers<0) FreeBuffers += mmpapi->NumVidBuf;
		}
	}

	vTrk = mmpapi->Parser->GetVidHandle(mmpapi->kernel);
	vTimeScale = mmpapi->Parser->GetTickRate(vTrk);
	vCnt = mmpapi->vCnt;

	Drop = 0;
	while(FreeBuffers)
	{
		int align, pad;
	#ifdef GP_FREERTOS
		int align2, ptr_end;
	#endif
	#ifndef SEEK64_EN
		long Size, Offset;
	#else
		long long Size, Offset;
	#endif
		long Dur;


		MaxSize = mmpapi->VidBufMax[WI];
		ptr = mmpapi->VidBuf[WI];

		if(mmpapi->flag & MMP_FLAG_REVERSE_PLAY)
		{
			int SkipCnt, SeekToKey;

			// EOS Check
			if(vCnt<0) break;

			mmpapi->VidTime[WI] = mmpapi->vSample.Tick64;
			vCnt--;
			ret = mmpapi->Parser->GetSampleInfo(vTrk, vCnt, &mmpapi->vSample);
			if(ret<0) RETURN(ret);
			Size = mmpapi->vSample.Size;
			if(Size<=0) mmpapi->TotalNullFrame++;

			Dur = 0;
			SkipCnt = mmpapi->FrameDropLevel;
			SeekToKey = mmpapi->flag & MMP_FLAG_SEEK_TO_KEY;
			mmpapi->flag ^= SeekToKey;
			while(vCnt>0 && (!mmpapi->vSample.IsKey || Size<=0 || SkipCnt || SeekToKey))
			{
				if(SkipCnt) SkipCnt--;
				Drop++;
				Dur += mmpapi->vSample.dTick;
				vCnt--;
				ret = mmpapi->Parser->GetSampleInfo(vTrk, vCnt, &mmpapi->vSample);
				if(ret<0) RETURN(ret);
				Size = mmpapi->vSample.Size;
				if(Size<=0) mmpapi->TotalNullFrame++;
			}
			// The time for Audio Follow Mode
		#if 0
			// mmpapi->SeekSecQ8 = (mmpapi->vSample.Tick64 << 8) / vTimeScale;
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vSample.Tick64 << 8);
		#else
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vUserTime << 8);
		#endif
			//DEBUG_ONLY(DBG_PRINT("RdRvdVid=%dSec\r\n", mmpapi->SeekSecQ8 >> 8));
			Offset = mmpapi->vSample.Offset;
			Dur += mmpapi->vSample.dTick;
		}
		else
		{
			int SkipCnt, SeekToKey;
			int FrameDropLevel;

			// EOS Check
			if(vCnt>=mmpapi->VidMaxSample) break;

			FrameDropLevel = mmpapi->FrameDropLevel;
			SkipCnt = FrameDropLevel + 1;

			Size = mmpapi->vSample.Size;
			mmpapi->VidTime[WI] = mmpapi->vSample.Tick64;
			// The time for Audio Follow Mode
		#if 0
			// mmpapi->SeekSecQ8 = (mmpapi->vSample.Tick64 << 8) / vTimeScale;
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vSample.Tick64 << 8);
		#else
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vUserTime << 8);
		#endif
			//DEBUG_ONLY(DBG_PRINT("RdNorVid=%d\r\n", mmpapi->SeekSecQ8 >> 8));
			Offset = mmpapi->vSample.Offset;
			IsKey = mmpapi->vSample.IsKey;
			Dur = 0;

			SeekToKey = mmpapi->flag & MMP_FLAG_SEEK_TO_KEY;
			mmpapi->flag ^= SeekToKey;
			do
			{
				Dur += mmpapi->vSample.dTick;
				Drop++;
				vCnt++;
				ret = mmpapi->Parser->GetSampleInfo(vTrk, vCnt, &mmpapi->vSample);
				if(ret<0) RETURN(ret);
				// check EOV
				if(ret==0)
				{
					// update VidMaxSample if necessary
					if(vCnt<mmpapi->VidMaxSample)
						mmpapi->VidMaxSample = vCnt;
					break;
				}

				if(mmpapi->vSample.Size<=0) mmpapi->TotalNullFrame++;
			/*
			    if offset of previous frame same to current frame , that means
			    current frame is dummy frame (for mjpg + mov insert dummy frame use )
			*/
				if(mmpapi->vSample.Offset==g_pre_offset)  mmpapi->TotalNullFrame++;
				 g_pre_offset = mmpapi->vSample.Offset;

				if(SkipCnt) SkipCnt--;
			} while(SkipCnt>0 || (!mmpapi->vSample.IsKey && (FrameDropLevel || SeekToKey)) || mmpapi->vSample.Size<=0);
			Drop--;
		}

		// calculate alignment and padding bytes
		/*if ((mmpapi->flag & MMP_FLAG_AVC1_READ) ||*/
		if ((mmpapi->Parser->GetFormatFourCC(vTrk) == MAKEFOURCC('a','v','c','1'))) {
			//align = 0;
			align = Offset & 3;
			pad = 0;
		} else {
			align = Offset & 3;
			pad = (4 - (Offset + Size)) & 3;
		}

		if(Offset + Size + pad > mmpapi->FileLength)
		{
			pad = mmpapi->FileLength - Offset - Size;
		}

		// re-allocate buffer if Size greater than MaxSize
		#ifdef GP_FREERTOS
		align2 = 0;
		if ((INT32U)ptr & 0x3F)
			align2 = (0x40 - ((INT32U)ptr & 0x3F));

		ptr_end = (INT32U)ptr + Size + align + pad + align2;
		if ((INT32U)ptr_end & 0x3F)
			align2 += (0x40 - ((INT32U)ptr_end & 0x3F));

		if(MaxSize < Size + align + pad + align2)
		#else
		if(MaxSize < Size + align + pad)
		#endif
		{
			int new_size;
			void *new_ptr;
			new_size = Size + align + pad;
			new_size += new_size >> 3; // x1.25 to reduce re-allocate
#ifdef GP_FREERTOS
			new_size = (new_size + 0x3F) >> 6 << 6;
			new_ptr = (char*)MALLOC(new_size); //64 byte alignmemt for H.264
#else
			new_size = (new_size + 3) >> 2 << 2; // WORD alignment
			new_ptr = (char*)MALLOC(new_size);
#endif
			if(new_ptr)
			{
				FREE(ptr);
				mmpapi->VidBufMax[WI] = new_size;
				MaxSize = new_size;
				ptr = new_ptr;
				mmpapi->VidBuf[WI] = ptr;
			}
			else
			{
				mmpapi->flag |= MMP_FLAG_LACK_OF_MEM | MMP_FLAG_SEEK_TO_KEY;
			}
		}

		if(MaxSize >= Size + align + pad)
		{
			// check avc1
			if(mmpapi->Parser->GetFormatFourCC(vTrk) == MAKEFOURCC('a','v','c','1')) {
				INT8U *pData;
				char  *dest = ptr;
				INT32U len = 0;
				INT32U tmp1 = 0;
				static INT32U gblengthSizeMinusOne = 3;

				#ifdef GP_FREERTOS
				ptr = (char*)(((INT32U)ptr + 0x3F) >> 6 << 6); // 64 byte alignment for H.264
				dest = ptr;
				#endif

				if(mmpapi->flag & MMP_FLAG_AVC1_READ) {
					pData = (INT8U *)mmpapi->Parser->GetExtHeaderData(mmpapi->kernel, (long *)&len);
					if (align) {
						for(i=0; i<align; i++) {
							*(ptr + i) = 0;
						}
					}
					else{
						//---不處於reverse status , 才做消除MMP_FLAG_AVC1_READ----
                        //---這是為了確保每個VID BUF 都會去加header-----
					       if(Speed_reverse_flag==0)
					      {
					          mmpapi->flag &= ~MMP_FLAG_AVC1_READ;
					      }

						MEMCPY(ptr, pData, len);
					}
					ptr += len;
					gblengthSizeMinusOne = *(pData + 1024 + 4);
					//IsKey = 1;
				}

				// modify bitstream
				if(gblengthSizeMinusOne != 3) {
					pData = (INT8U *)mmpapi->Parser->GetExtHeaderData(mmpapi->kernel, (long *)&len);
					switch(gblengthSizeMinusOne)
					{
					case 0:
						for (i = 0; i < Size;)
						{
							tmp1 = 0;
							tmp1 |= (*pData);

							*ptr = 0x00;
							*(ptr + 1) = 0x00;
							*(ptr + 2) = 0x00;
							*(ptr + 3) = 0x01;

							memcpy(ptr + 4, pData + 1, tmp1);

							ptr += tmp1 + 4;
							pData += tmp1 + 1;
							i += tmp1 + 1;
						}
						break;
					case 1:
						for (i = 0; i < Size;)
						{
							tmp1 = 0;
							tmp1 |= (*pData) << 8;
							tmp1 |= *(pData + 1);

							*ptr = 0x00;
							*(ptr + 1) = 0x00;
							*(ptr + 2) = 0x00;
							*(ptr + 3) = 0x01;

							memcpy(ptr + 4, pData + 2, tmp1);

							ptr += tmp1 + 4;
							pData += tmp1 + 2;
							i += tmp1 + 2;
						}
						break;

					case 2:
						for (i = 0; i < Size;)
						{
							tmp1 = 0;
							tmp1 |= (*pData) << 16;
							tmp1 |= (*(pData + 1)) << 8;
							tmp1 |= *(pData + 2);

							*ptr = 0x00;
							*(ptr + 1) = 0x00;
							*(ptr + 2) = 0x00;
							*(ptr + 3) = 0x01;

							memcpy(ptr + 4, pData + 3, tmp1);

							ptr += tmp1 + 4;
							pData += tmp1 + 3;
							i += tmp1 + 3;
						}
						break;

					default:
						ret = -1;
						goto Return;
					}
				} else {
					SEEK(mmpapi->fid, Offset - align, SEEK_SET);
					READ(mmpapi->fid, ptr, Size + align + pad);

					if(mmpapi->flag & MMP_FLAG_AVC1_READ){
						//---不處於reverse status , 才做消除MMP_FLAG_AVC1_READ----
                        //---這是為了確保每個VID BUF 都會去加header-----
                        if(Speed_reverse_flag==0)
						    mmpapi->flag &= ~MMP_FLAG_AVC1_READ;
						if (align) {
							pData = (INT8U *)mmpapi->Parser->GetExtHeaderData(mmpapi->kernel, (long *)&len);
							MEMCPY(ptr-(len-align), pData, len);
							ptr += align;
						}
					}
					else
					// invalid header set zero
					if (align) {
						for(i=0; i<align; i++) {
							*(ptr + i) = 0;
						}
						ptr += align;
					}

					for(i = 0; i < Size;)
					{
						tmp1 = 0;
						tmp1 |= (*ptr) << 24;
						tmp1 |= (*(ptr + 1)) << 16;
						tmp1 |= (*(ptr + 2)) << 8;
						tmp1 |= (*(ptr + 3));

						*ptr = 0x00;
						*(ptr + 1) = 0x00;
						*(ptr + 2) = 0x00;
						*(ptr + 3) = 0x01;

						ptr += tmp1 + 4;
						i += tmp1 + 4;
					}
				}

				Size += len;
				if((INT32U)ptr > ((INT32U)dest + Size + align)) {
					len = (INT32U)ptr - (INT32U)dest - Size - align;
				} else {
					len = 0;
				}

				ptr = dest;
				Size += len;
			} else {
				SEEK(mmpapi->fid, Offset - align, SEEK_SET);
				READ(mmpapi->fid, ptr, Size + align + pad);

				// invalid header set zero
				for(i=0; i<align; i++) {
					*(ptr + i) = 0;
				}
			}

			mmpapi->VidBufLen[WI] = Size;
			mmpapi->VidData[WI] = ptr + align;
		}
		else
		{
			mmpapi->VidBufLen[WI] = 0;
			mmpapi->VidData[WI] = 0;
		}
		//	為了快進快退做的,因為Vidbuf 會先預讀Duration,因此當從快進切回normal
        //  會發生沒消化完的Vudbuf(Duration 仍是長的),導致慢動作
		if(mmpapi->FrameDropLevel!=0)
		{
		  for_fast_Duration = Dur;
		}
		else
		{
		  mmpapi->VidDur[WI] = Dur;
		}

        //	為了快進快退做的,因為Vidbuf 會先預讀Duration,因此當從快進切回normal
        //  會發生沒消化完的Vudbuf(Duration 仍是長的),導致慢動作
		mmpapi->VidIsKey[WI] = IsKey;
		WI++;
		if(WI>=mmpapi->NumVidBuf) WI -= mmpapi->NumVidBuf;
		mmpapi->VidBufWI = WI;
		FreeBuffers--;
	}

	mmpapi->TotalSkipFrame += Drop;
	mmpapi->vCnt = vCnt;

	//speed up read audio
	if(mmpapi->NumVidBuf <= 10 && (mmpapi->flag & MMP_FLAG_FOLLOW_MODE || mmpapi->flag & MMP_FLAG_REVERSE_PLAY))
	{
		ret = SeekAud(mmpapi);
		if(ret<0) RETURN(ret);
	}

	ret = 0;
Return:
	mmpapi->VidBufWI = WI;
	return ret;
}

// Read Audio Data
static int ReadAudio(PARSERAPI *mmpapi)
{
	int ret;

	int WI = mmpapi->AudRingWI;
	int AudRingWI = mmpapi->AudRingWI;
	const int RI = mmpapi->AudRingRI;
	const int Len = mmpapi->AudRingLen;
	int Rest = mmpapi->AudRest;
#ifndef SEEK64_EN
	long FileOffset = mmpapi->AudFileOffset;
#else
	long long FileOffset = mmpapi->AudFileOffset;
#endif
	int FreeSize;
	void *aTrk = mmpapi->Parser->GetAudHandle(mmpapi->kernel);
	FILEHANDLE fid = mmpapi->fid;
	int t;
	int DoSwapByte;
	int IsAAC;

	if(aTrk==0) RETURN(0);

	FreeSize = RI - WI - 1;
	if(FreeSize<0) FreeSize += mmpapi->AudRingLen;
	FreeSize = FreeSize >> SECTOR_SHIFT << SECTOR_SHIFT;

	if(mmpapi->Parser->GetFormatFourCC(aTrk)==MAKEFOURCC('t','w','o','s'))
		DoSwapByte = 1;
	else
		DoSwapByte = 0;

	if(mmpapi->Parser->GetFormatFourCC(aTrk)==MAKEFOURCC('m','p','4','a'))
		IsAAC = 1;
	else
		IsAAC = 0;


	if(Rest) SEEK(fid, FileOffset, SEEK_SET);
	while(FreeSize >= 7) // Reserve 7 bytes for AAC ADTS Header
	{
		if(Rest==0)
		{
			Rest = mmpapi->aSample.Size;
			if(mmpapi->aSample.Size==0)
				break;

			if(IsAAC)
			{
				// ADTS Header;
				char *Ring = mmpapi->AudRing;
				const char *Header = mmpapi->ADTSHeader;
				int AACLen = Rest + 7;
				Ring[WI++] = *Header++;									if(WI>=Len) WI -= Len;
				Ring[WI++] = *Header++;									if(WI>=Len) WI -= Len;
				Ring[WI++] = *Header++;									if(WI>=Len) WI -= Len;
				Ring[WI++] = (char)((*Header++) | ((AACLen >> 11) & 0x3));	if(WI>=Len) WI -= Len;
				Ring[WI++] = (char)((AACLen >> 3) & 0xFF);				if(WI>=Len) WI -= Len;
				Ring[WI++] = (char)(((AACLen << 5) & 0xE0) | 0x1F);		if(WI>=Len) WI -= Len;
				Ring[WI++] = (char)(0xFC);								if(WI>=Len) WI -= Len;
				FreeSize -= 7;
			}


			FileOffset = mmpapi->aSample.Offset;
			mmpapi->aCnt += mmpapi->aSample.Count;
			ret = mmpapi->Parser->GetSampleInfo(aTrk, mmpapi->aCnt, &mmpapi->aSample);
			if(ret<0) RETURN(ret);

			if(!IsAAC)
			{
				while(FileOffset + Rest == mmpapi->aSample.Offset && ret)
				{
					Rest += mmpapi->aSample.Size;
					mmpapi->aCnt += mmpapi->aSample.Count;
					ret = mmpapi->Parser->GetSampleInfo(aTrk, mmpapi->aCnt, &mmpapi->aSample);
					if(ret<0) RETURN(ret);
				}
			}

			//check audio end, when read audio data end
			if(mmpapi->aSample.Size==0) {
				mmpapi->flag |= MMP_FLAG_AUD_READ_END;
			} else {
				mmpapi->flag &= ~MMP_FLAG_AUD_READ_END;
			}

			// DEBUG_ONLY(DBG_PRINT("ReadAudio\t%d\t%d\r\n", Offset, Rest));
			SEEK(fid, FileOffset, SEEK_SET);
		}
		if(FreeSize < Rest) t = FreeSize;
		else t = Rest;

		if(WI + t >= Len)
		{
			int t2 = Len - WI;
			READ(fid, mmpapi->AudRing + WI, t2);
			FileOffset += t2;
			WI = 0;
			t -= t2;
			FreeSize -= t2;
			Rest -= t2;

			if(DoSwapByte)
				SwapByte32((unsigned char*)(mmpapi->AudRing + AudRingWI), Len - AudRingWI);
			AudRingWI = 0;
		}
		if(t)
		{
			READ(fid, mmpapi->AudRing + WI, t);
			FileOffset += t;
			WI += t;
			FreeSize -= t;
			Rest -= t;

			if(DoSwapByte)
				SwapByte32((unsigned char*)(mmpapi->AudRing + AudRingWI), (WI >> 5 << 5) - AudRingWI);
			// Do not update WI too much, because SwapByte32 not processed yet!
			AudRingWI = WI >> 5 << 5;
		}
	}
	if(DoSwapByte)
		SwapByte32((unsigned char*)(mmpapi->AudRing + AudRingWI), WI - AudRingWI);
	mmpapi->AudRingWI = WI;

	ret = 0;
Return:
	mmpapi->AudRest = Rest;
	mmpapi->AudFileOffset = FileOffset;

	//check audio end, when audio read data end and audio decode data end
	// DEBUG_ONLY(DBG_PRINT("AudWIRI=%d,%d,%d\r\n", mmpapi->AudRingWI, mmpapi->AudRingRI, Rest));
	if((mmpapi->AudRingWI == mmpapi->AudRingRI) && (mmpapi->flag & MMP_FLAG_AUD_READ_END))
		mmpapi->flag |= MMP_FLAG_EOA;

	return ret;
}

// Seek to mmpapi->SeekSecQ8
// mmpapi->SeekSecQ8 will be increased to where key frame exists
// then audio will seek to the new mmpapi->SeekSecQ8
// when seek time > 5 minute, it will into step by step seek,
static int Seek(PARSERAPI *mmpapi)
{
	int ret;
	long vTimeScale;
	long curtime, seektime, seekdistance;
	void *vTrk = mmpapi->Parser->GetVidHandle(mmpapi->kernel);
	void *aTrk = mmpapi->Parser->GetAudHandle(mmpapi->kernel);
	MEDIA_PARSER_SAMPLE_INFO info;

	// seek range is over 5 minute, use seek step by step
	if(vTrk) {
		vTimeScale = mmpapi->Parser->GetTickRate(vTrk);
		curtime = (long)MMP_udiv64by32(vTimeScale, mmpapi->vUserTime << 8);
		if (mmpapi->SeekSample) {
			ret = mmpapi->Parser->GetSampleInfo(vTrk, mmpapi->SeekSample, &info);
			if(ret<0) RETURN(ret);
			seektime = (long)MMP_udiv64by32(vTimeScale, info.Tick64 << 8);
		}
		else
			seektime = mmpapi->SeekSecQ8;
		seekdistance = seektime - curtime;
		if(seekdistance < 0) {
			seekdistance = curtime - seektime;
		}

		if(seekdistance >= (300 << 8)) {
			goto __SeekSetpByStep;
		}
	}

	if(vTrk)
	{
		long Time;
		long tarSample;
		long vTimeScale;

		vTimeScale = mmpapi->Parser->GetTickRate(vTrk);

		if (mmpapi->SeekSample) {
			tarSample = mmpapi->SeekSample;
			mmpapi->SeekSample = 0;
		}
		else {
			Time = (long)((__int64)mmpapi->SeekSecQ8 * vTimeScale >> 8);
			tarSample = mmpapi->Parser->Tick2Sample(vTrk, Time);
			if(tarSample<0) RETURN(tarSample);
			tarSample++;
		}

		// Seek to key frame
		tarSample = mmpapi->Parser->GetKeyFrame(vTrk, tarSample);
		mmpapi->vCnt = tarSample;
		ret = mmpapi->Parser->GetSampleInfo(vTrk, tarSample, &mmpapi->vSample);
		if(ret<0) RETURN(ret);
		if(ret==0)
		{
			// Seek out of range
			if(tarSample<mmpapi->VidMaxSample)
				mmpapi->VidMaxSample = tarSample;
		}

		mmpapi->vUserTime = mmpapi->vSample.Tick64;
		// mmpapi->SeekSecQ8 = (long)((mmpapi->vSample.Tick64 << 8) / vTimeScale);
		mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vSample.Tick64 << 8);
		// end of seek to key frame
	}

	ret = SeekAud(mmpapi);
	if(ret<0) RETURN(ret);

	ret = 0;
Return:
	return ret;

__SeekSetpByStep:
	if(vTrk) {
		long Time, SeekTime, AudTime;
		long tarSample;
		long vTimeScale;

		// seek video
		vTimeScale = mmpapi->Parser->GetTickRate(vTrk);

		if (mmpapi->SeekSample) {
			SeekTime = info.Tick64;
			mmpapi->SeekSample = 0;
		}
		else
			SeekTime = (long)((__int64)mmpapi->SeekSecQ8 * vTimeScale >> 8);

		Time = curtime;

		while(1) {
			if(Time == SeekTime) {
				break;
			} else if(Time < SeekTime) {
				Time += 1000;
				if(Time >= SeekTime) {
					Time = SeekTime;
				}
			} else if(Time > SeekTime) {
				Time -= 1000;
				if(Time <= SeekTime) {
					Time = SeekTime;
				}
			}

			tarSample = mmpapi->Parser->Tick2Sample(vTrk, Time);
			if(tarSample<0) RETURN(tarSample);
			tarSample++;

			// Seek to key frame
			tarSample = mmpapi->Parser->GetKeyFrame(vTrk, tarSample);
			mmpapi->vCnt = tarSample;
			ret = mmpapi->Parser->GetSampleInfo(vTrk, tarSample, &mmpapi->vSample);
			if(ret<0) RETURN(ret);
			if(ret==0) {
				// Seek out of range
				if(tarSample<mmpapi->VidMaxSample)
					mmpapi->VidMaxSample = tarSample;
			}

			mmpapi->vUserTime = mmpapi->vSample.Tick64;
			// mmpapi->SeekSecQ8 = (long)((mmpapi->vSample.Tick64 << 8) / vTimeScale);
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vSample.Tick64 << 8);
			// end of seek to key frame

			if(aTrk) {
				// seek audio
				AudTime = (long)((__int64)mmpapi->SeekSecQ8 * mmpapi->Parser->GetTickRate(aTrk) >> 8);
				ret = mmpapi->Parser->Tick2Sample(aTrk, AudTime);
				if(ret<0) {
					RETURN(ret);
				}

				mmpapi->aCnt = ret;
				ret = mmpapi->Parser->GetSampleInfo(aTrk, ret, &mmpapi->aSample);
				if(ret<0) {
					RETURN(ret);
				}

				if(ret) {
					mmpapi->flag &= ~MMP_FLAG_EOA;
				} else {
					mmpapi->flag |= MMP_FLAG_EOA;
				}
			}
		}
	} else {
		ret = SeekAud(mmpapi);
		if(ret<0) RETURN(ret);
	}

	RETURN(0);
}

#ifdef GP_FREERTOS
static void MultiMediaParser_Task(void const *Param)
#elif LINUX_SYSTEM
static void *MultiMediaParser_Task(void *Param)
#else
static void MultiMediaParser_Task(void *Param)
#endif
{
	PARSERAPI *mmpapi = (PARSERAPI*)Param;
	/*const*/ void *vTrk = mmpapi->Parser->GetVidHandle((void *)mmpapi->kernel);
	long vTimeScale = 0;
	int msg;
	//INT8U err;
	int ret;
	int ErrAction;

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_Task\r\n"));
	if(vTrk) {
		vTimeScale = mmpapi->Parser->GetTickRate(vTrk);
	}

	while(1)
	{
#ifdef GP_FREERTOS
        uint32_t mm;
        osEvent event;
        event = osMessageGet(mmpapi->Msg, 50);
        if (event.status == osEventMessage)
            msg = event.value.v;
        else {
#else
		msg = (int)OSQPend(mmpapi->Msg, 50, &err);
		if(err == OS_TIMEOUT){
#endif
			//DEBUG_ONLY(DBG_PRINT("%s:%d get msg timeout!\r\n", __FUNCTION__, __LINE__));
			continue;
		}
		MemoryCheck();

		switch(msg)
		{
		case MMP_MSG_REFILL_AUD:
			mmpapi->AudReqRx++;
			// DEBUG_ONLY(DBG_PRINT("MMP_MSG_REFILL_AUD\r\n"));
			if(!(mmpapi->flag & (MMP_FLAG_REVERSE_PLAY | MMP_FLAG_FOLLOW_MODE)))
			{
				ret = ReadAudio(mmpapi);
				if(ret<0) {
					DEBUG_ONLY(DBG_PRINT("MMP_MSG_REFILL_AID error ret = %d\r\n", ret));
				}
				if(ret<0 && mmpapi->ErrHandler)
				{
					ErrAction = mmpapi->ErrHandler(ret);
					DEBUG_ONLY(DBG_PRINT("MMP_MSG_REFILL_AID refill error(%d)!!!!\r\n", ErrAction));
					if(ErrAction==1) goto ErrMode;
				}
			}
			break;

		case MMP_MSG_REFILL_VID:
			mmpapi->VidReqRx++;
			// DEBUG_ONLY(DBG_PRINT("MMP_MSG_REFILL_VID\r\n"));
			ret = ReadVideo(mmpapi, 0);
			if(ret<0) {
				DEBUG_ONLY(DBG_PRINT("MMP_MSG_REFILL_VID error ret = %d\r\n", ret));
			}
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				DEBUG_ONLY(DBG_PRINT("MMP_MSG_REFILL_VID refill error(%d)!!!!\r\n", ErrAction));
				if(ErrAction==1) goto ErrMode;
			}
			break;

		case MMP_MSG_SELECT_TRACK:
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_SELECT_TRACK\r\n"));
			ret = 0;
			if (mmpapi->SelectVTrk >= 0 || mmpapi->SelectATrk >= 0)
			{
				void* retTrk;
				int seekVideo = 0, seekAudio = 0;

				if (mmpapi->SelectVTrk >= 0)
				{
					if (mmpapi->Parser->SelectVidTrack)
						retTrk = mmpapi->Parser->SelectVidTrack((void*)mmpapi->kernel, mmpapi->SelectVTrk);
					else
						retTrk = NULL;
					mmpapi->SelectVTrk = -1;
					if (retTrk && retTrk != vTrk)
					{
						vTrk = retTrk;
						vTimeScale = mmpapi->Parser->GetTickRate(vTrk);
						seekVideo = 1;
					}
				}
				if (mmpapi->SelectATrk >= 0)
				{
					void* aTrk = mmpapi->Parser->GetAudHandle((void*)mmpapi->kernel);
					if (mmpapi->Parser->SelectAudTrack)
						retTrk = mmpapi->Parser->SelectAudTrack((void*)mmpapi->kernel, mmpapi->SelectATrk);
					else
						retTrk = NULL;
					mmpapi->SelectATrk = -1;
					if (retTrk && retTrk != aTrk);
						seekAudio = 1;
				}
				if (seekVideo || seekAudio)
				{
					mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vUserTime << 8); //continue to SEEK
				}
				else
					ret = -1;
			}
			else
				ret = -1;
			if (ret < 0)
			{
			#ifdef GP_FREERTOS
				mm = MULTIMEDIAPARSER_ACK_OK;
				osMessagePut(mmpapi->Ack, (INT32U)&mm, 0);
			#else
				OSMboxPost(mmpapi->Ack, (void*)MULTIMEDIAPARSER_ACK_OK);
			#endif
				break;
			}
		case MMP_MSG_SEEK:
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_SEEK\r\n"));
			ret = Seek(mmpapi);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}

			if(mmpapi->Parser->GetFormatFourCC(vTrk) == MAKEFOURCC('a','v','c','1')) {
				mmpapi->flag |= MMP_FLAG_AVC1_READ;
			}

			mmpapi->TotalSkipFrame = mmpapi->TotalNullFrame = 0;

			mmpapi->VidBufRI = mmpapi->VidBufWI = 0;
			ret = ReadVideo(mmpapi, 0);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}

			if(!(mmpapi->flag & (MMP_FLAG_REVERSE_PLAY | MMP_FLAG_FOLLOW_MODE)))
			{
				mmpapi->AudRingRI = mmpapi->AudRingWI = mmpapi->AudRest = 0;
				ret = ReadAudio(mmpapi);
				if(ret<0 && mmpapi->ErrHandler)
				{
					ErrAction = mmpapi->ErrHandler(ret);
					if(ErrAction==1) goto ErrMode;
				}
			}

#ifdef GP_FREERTOS
            mm = MULTIMEDIAPARSER_ACK_OK;
            osMessagePut(mmpapi->Ack, (INT32U)&mm, 0);
#else
			OSMboxPost(mmpapi->Ack, (void*)MULTIMEDIAPARSER_ACK_OK);
#endif
			break;

		case MMP_MSG_STOP:
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_STOP\r\n"));
			goto EndTask;

		case MMP_MSG_ENABLE_FOLLOW_MODE:
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_ENABLE_FOLLOW_MODE\r\n"));
			mmpapi->flag |= MMP_FLAG_FOLLOW_MODE;
			mmpapi->flag |= MMP_FLAG_AVC1_READ;  //在快進快退處於reverse時,強制加header
			break;

		case MMP_MSG_DISABLE_FOLLOW_MODE:
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_DISABLE_FOLLOW_MODE\r\n"));
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vUserTime << 8);
			DEBUG_ONLY(DBG_PRINT("DISABLE_FOLLOW_MODE=%d\r\n", mmpapi->SeekSecQ8 >> 8));

			ret = Seek(mmpapi);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}

			mmpapi->TotalSkipFrame = mmpapi->TotalNullFrame = 0;
			mmpapi->VidBufWI = mmpapi->VidBufRI = 0;

			ret = ReadVideo(mmpapi, 0);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}

			mmpapi->flag &= ~MMP_FLAG_FOLLOW_MODE;
			mmpapi->flag |= MMP_FLAG_AVC1_READ;  //在快進快退處於reverse時,強制加heade
			if((mmpapi->flag & MMP_FLAG_REVERSE_PLAY)==0)
			{
				mmpapi->AudRingRI = mmpapi->AudRingWI = mmpapi->AudRest = 0;
				ret = ReadAudio(mmpapi);
				if(ret<0 && mmpapi->ErrHandler)
				{
					ErrAction = mmpapi->ErrHandler(ret);
					if(ErrAction==1) goto ErrMode;
				}
			}
#ifdef GP_FREERTOS
            mm = MULTIMEDIAPARSER_ACK_OK;
            osMessagePut(mmpapi->Ack, (INT32U)&mm, 0);
#else
			OSMboxPost(mmpapi->Ack, (void*)MULTIMEDIAPARSER_ACK_OK);
#endif
			break;

		case MMP_MSG_ENABLE_REVERSE_PLAY:
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_ENABLE_REVERSE_PLAY\r\n"));
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vUserTime << 8);
			DEBUG_ONLY(DBG_PRINT("ENABLE_REVERSE_PLAY=%dSec\r\n", mmpapi->SeekSecQ8 >> 8));

			ret = Seek(mmpapi);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}

			if(mmpapi->Parser->GetFormatFourCC(vTrk) == MAKEFOURCC('a','v','c','1')) {
				mmpapi->flag |= MMP_FLAG_AVC1_READ;
			}

			mmpapi->flag |= MMP_FLAG_REVERSE_PLAY;
			mmpapi->TotalSkipFrame = mmpapi->TotalNullFrame = 0;

			mmpapi->VidBufRI = mmpapi->VidBufWI = 0;
			ret = ReadVideo(mmpapi, 0);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}
#ifdef GP_FREERTOS
            mm = MULTIMEDIAPARSER_ACK_OK;
            osMessagePut(mmpapi->Ack, (INT32U)&mm, 0);
#else
			OSMboxPost(mmpapi->Ack, (void*)MULTIMEDIAPARSER_ACK_OK);
#endif
			break;

		case MMP_MSG_DISABLE_REVERSE_PLAY:
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_DISABLE_REVERSE_PLAY\r\n"));
			mmpapi->SeekSecQ8 = (long)MMP_udiv64by32(vTimeScale, mmpapi->vUserTime << 8);
			DEBUG_ONLY(DBG_PRINT("DISABLE_REVERSE_PLAY=%dSec\r\n", mmpapi->SeekSecQ8 >> 8));

			ret = Seek(mmpapi);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}

			if(mmpapi->Parser->GetFormatFourCC(vTrk) == MAKEFOURCC('a','v','c','1')) {
				mmpapi->flag |= MMP_FLAG_AVC1_READ;
			}

			mmpapi->flag &= ~MMP_FLAG_REVERSE_PLAY;
			mmpapi->TotalSkipFrame = mmpapi->TotalNullFrame = 0;

			mmpapi->VidBufRI = mmpapi->VidBufWI = 0;
			ret = ReadVideo(mmpapi, 0);
			if(ret<0 && mmpapi->ErrHandler)
			{
				ErrAction = mmpapi->ErrHandler(ret);
				if(ErrAction==1) goto ErrMode;
			}

			if((mmpapi->flag & MMP_FLAG_FOLLOW_MODE)==0)
			{
				mmpapi->AudRingRI = mmpapi->AudRingWI = mmpapi->AudRest = 0;
				ret = ReadAudio(mmpapi);
				if(ret<0 && mmpapi->ErrHandler)
				{
					ErrAction = mmpapi->ErrHandler(ret);
					if(ErrAction==1) goto ErrMode;
				}
			}

#ifdef GP_FREERTOS
            mm = MULTIMEDIAPARSER_ACK_OK;
            osMessagePut(mmpapi->Ack, (INT32U)&mm, 0);
#else
			OSMboxPost(mmpapi->Ack, (void*)MULTIMEDIAPARSER_ACK_OK);
#endif
			break;
		}
	}
EndTask:
#ifdef GP_FREERTOS
    msg = MULTIMEDIAPARSER_ACK_OK;
    osMessagePut(mmpapi->Ack, (INT32U)&msg, 0);
#else
	OSMboxPost(mmpapi->Ack, (void*)MULTIMEDIAPARSER_ACK_OK);
#endif
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	OSTaskDel(OS_PRIO_SELF);
#elif defined(GP_FREERTOS)
    osThreadTerminate(NULL);
#else
	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_Task exit ok!!!\r\n"));
	return NULL;
#endif

ErrMode:
	mmpapi->flag |= MMP_FLAG_ERROR;
	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_Task enter error mode\r\n"));
	while(1)
	{
#ifdef GP_FREERTOS
        osEvent event;
		event = osMessageGet(mmpapi->Msg, 50);
		msg = event.value.v;
#else
        msg = (int)OSQPend(mmpapi->Msg, 0, &err);
#endif
		MemoryCheck();
		if(msg==MMP_MSG_STOP)
		{
			DEBUG_ONLY(DBG_PRINT("MMP_MSG_STOP\r\n"));
			goto EndTask;
		}
	}
}

static void MultiMediaParser_Release(PARSERAPI *mmpapi)
{
	int i;

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_Release\r\n"));
	MemoryCheck();

	if(mmpapi)
	{
	#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
		INT8U err;

		// release OS events handler
		if(mmpapi->Ack) OSMboxDel(mmpapi->Ack, 0, &err);
		if(mmpapi->Msg) OSQDel(mmpapi->Msg, 0, &err);
    #elif defined(GP_FREERTOS)
        if(mmpapi->Ack) vQueueDelete(mmpapi->Ack);
		if(mmpapi->Msg) vQueueDelete(mmpapi->Msg);
	#else
		//UINT8 err;

		// release OS events handler
		if(mmpapi->Ack) mcp_mbox_free(mmpapi->Ack);
		if(mmpapi->Msg) mcp_mbox_free(mmpapi->Msg);
	#endif

		// release video buffers

		for(i=0;i<mmpapi->NumVidBuf; i++)
		{
			if(mmpapi->VidBuf[i]) FREE(mmpapi->VidBuf[i]);
		}

		if (mmpapi->VidBuf) FREE(mmpapi->VidBuf);
		if (mmpapi->VidData) FREE(mmpapi->VidData);
		if (mmpapi->VidDur) FREE(mmpapi->VidDur);
		if (mmpapi->VidTime) FREE(mmpapi->VidTime);
		if (mmpapi->VidBufMax) FREE(mmpapi->VidBufMax);
		if (mmpapi->VidBufLen) FREE(mmpapi->VidBufLen);
		if (mmpapi->VidIsKey) FREE(mmpapi->VidIsKey);

	#ifdef LINUX_SYSTEM
		if(mmpapi->thread) {
			mcp_thread_join(mmpapi->thread);
		}
	#endif
		FREE(mmpapi);
	}
}

#if __OPTIMISE_LEVEL<2
PARSERAPI *g_MMPAPI = 0;
#endif

static void MakeADTSHeader(const GP_WAVEFORMATEX *aFmt, char *ptr)
{
	int fs_ind;
	if(aFmt->nSamplesPerSec>=96000) fs_ind = 0;
	else if (aFmt->nSamplesPerSec>=88200) fs_ind = 1;
	else if (aFmt->nSamplesPerSec>=64000) fs_ind = 2;
	else if (aFmt->nSamplesPerSec>=48000) fs_ind = 3;
	else if (aFmt->nSamplesPerSec>=44100) fs_ind = 4;
	else if (aFmt->nSamplesPerSec>=32000) fs_ind = 5;
	else if (aFmt->nSamplesPerSec>=24000) fs_ind = 6;
	else if (aFmt->nSamplesPerSec>=22000) fs_ind = 7;
	else if (aFmt->nSamplesPerSec>=16000) fs_ind = 8;
	else if (aFmt->nSamplesPerSec>=12000) fs_ind = 9;
	else if (aFmt->nSamplesPerSec>=11025) fs_ind = 10;
	else if (aFmt->nSamplesPerSec>=8000)  fs_ind = 11;
	else fs_ind = 15; // UNKNOWN


	ptr[0] = 0xFF;
	ptr[1] = 0xF1;
	ptr[2] = 0x40 | (fs_ind << 2) | (aFmt->nChannels >> 2);
	ptr[3] = aFmt->nChannels << 6;
}

//////////////////////////////////////////////////////////////////////////////////
//
// API Functions
//
//////////////////////////////////////////////////////////////////////////////////
void *MultiMediaParser_Create(
	int fid,
#ifdef SEEK64_EN
	long long FileSize,
#else
	long FileSize,
#endif
	int AudRingLen,
	int VidBufSize,
	int VidBufNum,
	int prio,
	const void *FcnTab,
	int (*ErrHandler)(int ErrID),
	int *ErrID)
{
	PARSERAPI *mmpapi = NULL;
	int ret = 0, i;
	int t;

	// char *AudRing = 0;
	void *vTrk, *aTrk;

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_Create\r\n"));
	MemoryCheck();

	if(fid==INVALID_FILE_ID) RETURN(MEDIAPARSER_PARAM_ERR);
	if(AudRingLen<=0) AudRingLen = DEFAULT_AUD_BUF_SIZE;
	if(VidBufSize<=0) VidBufSize = DEFAULT_VID_BUF_SIZE;

#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	if(pRead == 0) pRead = read;
	if(pSeek == 0) {
	#ifndef SEEK64_EN
		pSeek = lseek;
	#else
		pSeek = lseek64;
	#endif
	}
#endif

	// allocate PARSERAPI memory /////////////////////////////////
	t = (sizeof(PARSERAPI) + 3) >> 2 << 2;
	t += (AudRingLen + 3) >> 2 << 2;
	t += (((const MEDIA_PARSER_FCN_TAB*)FcnTab)->GetWorkMemSize() + 3) >> 2 << 2;
	mmpapi = (PARSERAPI*)MALLOC(t);
	if(mmpapi==NULL) RETURN(MEDIAPARSER_MEM_ERR);
	MEMSET(mmpapi, 0, t);

	if (VidBufNum <= 0)
		VidBufNum = NUM_VID_BUF;

	mmpapi->VidBuf = (char **)MALLOC(VidBufNum*sizeof(char*));
	MEMSET(mmpapi->VidBuf, 0, VidBufNum*sizeof(char*));
	mmpapi->VidData = (char **)MALLOC(VidBufNum*sizeof(char*));
	mmpapi->VidDur = (long *)MALLOC(VidBufNum*sizeof(long));
	mmpapi->VidTime = (long *)MALLOC(VidBufNum*sizeof(long));
	mmpapi->VidBufMax = (int *)MALLOC(VidBufNum*sizeof(int));
	mmpapi->VidBufLen = (int *)MALLOC(VidBufNum*sizeof(int));
	mmpapi->VidIsKey = (int *)MALLOC(VidBufNum*sizeof(int));

	if (!mmpapi->VidBuf || !mmpapi->VidData ||
		!mmpapi->VidDur || !mmpapi->VidTime ||
		!mmpapi->VidBufMax || !mmpapi->VidBufLen || !mmpapi->VidIsKey)
	{
		if (mmpapi->VidBuf) FREE(mmpapi->VidBuf);
		if (mmpapi->VidData) FREE(mmpapi->VidData);
		if (mmpapi->VidDur) FREE(mmpapi->VidDur);
		if (mmpapi->VidTime) FREE(mmpapi->VidTime);
		if (mmpapi->VidBufMax) FREE(mmpapi->VidBufMax);
		if (mmpapi->VidBufLen) FREE(mmpapi->VidBufLen);
		if (mmpapi->VidIsKey) FREE(mmpapi->VidIsKey);

		FREE(mmpapi);
		RETURN(MEDIAPARSER_MEM_ERR);
	}

	mmpapi->NumVidBuf = VidBufNum;

#if __OPTIMISE_LEVEL<2
	DEBUG_ONLY(g_MMPAPI = mmpapi);
#endif
	// set work_memsize
	mmpapi->workmem_size = t;
	// initial PARSERAPI /////////////////////////////////
	mmpapi->Parser = FcnTab;
	// set AudRing
	t = (sizeof(PARSERAPI) + 3) >> 2 << 2;
	mmpapi->AudRing = (char*)mmpapi + t;
	// set kernel working memory
	t += (AudRingLen + 3) >> 2 << 2;
	mmpapi->kernel = (char*)mmpapi + t;

	mmpapi->AudRingLen = AudRingLen;
	mmpapi->ErrHandler = ErrHandler;
	mmpapi->fid = fid;

	t = FTELL(fid);
	mmpapi->BaseOffset = t;
	mmpapi->FileLength = SEEK(fid, 0, SEEK_END);
	SEEK(fid, t, SEEK_SET);

	ret = mmpapi->Parser->Init(
		mmpapi->kernel,
		mmpapi->ExData,
		EXDATA_LEN,
		fid,
		FileSize);
	if(ret<0) RETURN(ret);

	// allocate video buffer /////////////////////////////////
	for(i=0;i<VidBufNum;i++)
	{
		mmpapi->VidBuf[i] = (char*)MALLOC(VidBufSize);
		if(mmpapi->VidBuf[i]==NULL) RETURN(MEDIAPARSER_MEM_ERR);
		mmpapi->VidBufMax[i] = VidBufSize;
		DEBUG_ONLY(DBG_PRINT("VidBuf[%d]=0x%x\r\n", i, mmpapi->VidBuf[i]));
	}

	vTrk = mmpapi->Parser->GetVidHandle(mmpapi->kernel);
	if(vTrk)
	{
		mmpapi->VidMaxSample = 0x7FFFFFFF;
		ret = mmpapi->Parser->GetSampleInfo(vTrk, mmpapi->vCnt, &mmpapi->vSample);
		if(ret<0) RETURN(ret);

		if(mmpapi->Parser->GetFormatFourCC(vTrk)==MAKEFOURCC('a','v','c','1')) {
			mmpapi->flag |= MMP_FLAG_AVC1_READ;
		}

		ret = ReadVideo(mmpapi, 1);
		if(ret<0) RETURN(ret);
	}

	aTrk = mmpapi->Parser->GetAudHandle(mmpapi->kernel);
	if(aTrk)
	{
		if(mmpapi->Parser->GetFormatFourCC(aTrk)==MAKEFOURCC('m','p','4','a'))
		{
			long Len;
			// Force typecast from const GP_WAVEFORMATEX* to GP_WAVEFORMATEX*
			// to modify wFormatTag as WAVE_FORMAT_MPEG_ADTS_AAC.
			// Actually I don't think this is a current way, but it is easy.
			GP_WAVEFORMATEX *aFmt = (GP_WAVEFORMATEX*)mmpapi->Parser->GetExData(aTrk, &Len);
			aFmt->wFormatTag = WAVE_FORMAT_MPEG_ADTS_AAC;
			MakeADTSHeader(aFmt, mmpapi->ADTSHeader);
		}

		// mmpapi->AudMaxSample = 0x7FFFFFFF;
		ret = mmpapi->Parser->GetSampleInfo(aTrk, mmpapi->aCnt, &mmpapi->aSample);
		if(ret<0) RETURN(ret);
		if(ret) mmpapi->flag &= ~MMP_FLAG_EOA;
		else  mmpapi->flag |= MMP_FLAG_EOA;
		ret = ReadAudio(mmpapi);
		if(ret<0) RETURN(ret);
	}

	// config OS
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	mmpapi->Msg = OSQCreate(mmpapi->MsgQ, MSGQ_SIZE);	if(mmpapi->Msg==NULL) RETURN(MEDIAPARSER_OS_ERR);
	mmpapi->Ack = OSMboxCreate(0);						if(mmpapi->Ack==NULL) RETURN(MEDIAPARSER_OS_ERR);

	mmpapi->GAP0 = 0x12345678;
	MEMSET(mmpapi->stack, 0, sizeof(mmpapi->stack));
	mmpapi->GAP1 = 0x87654321;
	if(OSTaskCreate(
		MultiMediaParser_Task,
		mmpapi,
		mmpapi->stack + sizeof(mmpapi->stack)/sizeof(OS_STK)-1,
		prio) != OS_NO_ERR)
	{
		RETURN(MEDIAPARSER_OS_ERR);
	}
#elif defined(GP_FREERTOS)
    {
        osMessageQDef_t def = {MSGQ_SIZE, sizeof(void*), /*mmpapi->MsgQ, */0};
        osMessageQDef_t ack_def = {1, sizeof(void*), NULL};
        mmpapi->Msg = osMessageCreate(&def, NULL );	if(mmpapi->Msg==NULL) RETURN(MEDIAPARSER_OS_ERR);
        mmpapi->Ack = osMessageCreate(&ack_def, NULL);	if(mmpapi->Ack==NULL) RETURN(MEDIAPARSER_OS_ERR);
    }

	mmpapi->GAP0 = 0x12345678;
	//MEMSET(mmpapi->stack, 0, sizeof(mmpapi->stack));
	mmpapi->GAP1 = 0x87654321;
	{
        osThreadDef(MultiMediaParser_Task, prio, 0, MMP_STACK_SIZE*4);
        //osThreadDef_t thread_def = {"MultiMediaParser_Task", MultiMediaParser_Task, osPriorityNormal, 0, MMP_STACK_SIZE*4};
        if (!osThreadCreate(&os_thread_def_MultiMediaParser_Task, mmpapi))
            RETURN(MEDIAPARSER_OS_ERR);
	}
#else
	mmpapi->Msg = &mmpapi->msg_queue;
	mmpapi->Ack = &mmpapi->ack_queue;
	if(mcp_mbox_init(mmpapi->Msg) == 0) {
		RETURN(MEDIAPARSER_OS_ERR);
	}

	if(mcp_mbox_init(mmpapi->Ack) == 0) {
		RETURN(MEDIAPARSER_OS_ERR);
	}

	mmpapi->thread = mcp_thread_create(MultiMediaParser_Task, (void *)mmpapi, prio);
	if(mmpapi->thread == 0) {
		RETURN(MEDIAPARSER_OS_ERR);
	}
#endif
	ret = 0;

Return:
	*ErrID = ret;
	DEBUG_ONLY(DBG_PRINT("%s:%d error ret=%x mmpapi=0x%x\r\n", __FUNCTION__, __LINE__, ret, mmpapi));
	if(ret<0 && mmpapi)
	{
		MultiMediaParser_Release(mmpapi);
		mmpapi = NULL;
	}
	return mmpapi;
}


void MultiMediaParser_Delete(void *hWorkMem)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	//INT8U err;
#ifdef GP_FREERTOS
    uint32_t msg;
#endif

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_Delete\r\n"));
	MemoryCheck();

	if(mmpapi==NULL) return;

#ifdef GP_FREERTOS
    msg = MMP_MSG_STOP;
	if(mmpapi->Msg && osMessagePut(mmpapi->Msg, (INT32U)&msg, 0)== osOK)
	{
		if(mmpapi->Ack)
		{
            osMessageGet(mmpapi->Ack, 10000);
			DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));
		}
	}
#else
	if(mmpapi->Msg && OSQPost(mmpapi->Msg, (void*)MMP_MSG_STOP)==OS_NO_ERR)
	{
		if(mmpapi->Ack)
		{
			OSMboxPend(mmpapi->Ack, 10000, &err);
			DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));
		}
	}
#endif

	MultiMediaParser_Release(mmpapi);
}

int MultiMediaParser_GetTotalVideoTrackNum(void *hWorkMem)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;

	if (mmpapi->Parser->GetVidTrkCnt)
		return mmpapi->Parser->GetVidTrkCnt(mmpapi->kernel);
	else
		return -1;
}

int MultiMediaParser_GetTotalAudioTrackNum(void *hWorkMem)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;

	if (mmpapi->Parser->GetAudTrkCnt)
		return mmpapi->Parser->GetAudTrkCnt(mmpapi->kernel);
	else
		return -1;
}

int MultiMediaParser_SelectVideoTrack(void *hWorkMem, int trk_no)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	INT8U err;
	int ret = 0;
	int Ack;
#ifdef GP_FREERTOS
    osEvent event;
    uint32_t msg;
#endif

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_SelectVideoTrack\r\n"));
	MemoryCheck();

	if (!mmpapi->Parser->GetVidTrkCnt)
		return -1;

	//mmpapi->SeekSecQ8 = Sec;
	if (trk_no >= 0 && trk_no < mmpapi->Parser->GetVidTrkCnt(mmpapi->kernel))
		mmpapi->SelectVTrk = trk_no;
	else
		return -1;

#ifdef GP_FREERTOS
    msg = MMP_MSG_SELECT_TRACK;
    err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
    if(err!=osOK)   RETURN(MEDIAPARSER_OS_ERR);
    event = osMessageGet(mmpapi->Ack, osWaitForever);
    if (event.status != osEventMessage) RETURN(MEDIAPARSER_OS_ERR);
    Ack = (int)event.value.v;
#else
	err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_SEEK);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);

	Ack = (int)OSMboxPend(mmpapi->Ack, 0, &err);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
	if(Ack!=MULTIMEDIAPARSER_ACK_OK) RETURN(MEDIAPARSER_KERNEL_ERR);
	DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));

	//ret = mmpapi->SeekSecQ8;
Return:
	return ret;
}

int MultiMediaParser_SelectAudioTrack(void *hWorkMem, int trk_no)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	INT8U err;
	int ret = 0;
	int Ack;
#ifdef GP_FREERTOS
    osEvent event;
    uint32_t msg;
#endif

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_SelectAudioTrack\r\n"));
	MemoryCheck();

	if (!mmpapi->Parser->GetAudTrkCnt)
		return -1;

	if (trk_no >= 0 && trk_no < mmpapi->Parser->GetAudTrkCnt(mmpapi->kernel))
		mmpapi->SelectATrk = trk_no;
	else
		return -1;

#ifdef GP_FREERTOS
    msg = MMP_MSG_SELECT_TRACK;
    err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
    if(err!=osOK)   RETURN(MEDIAPARSER_OS_ERR);
    event = osMessageGet(mmpapi->Ack, osWaitForever);
    if (event.status != osEventMessage) RETURN(MEDIAPARSER_OS_ERR);
    Ack = (int)event.value.v;
#else
	err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_SEEK);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);

	Ack = (int)OSMboxPend(mmpapi->Ack, 0, &err);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
	if(Ack!=MULTIMEDIAPARSER_ACK_OK) RETURN(MEDIAPARSER_KERNEL_ERR);
	DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));

	//ret = mmpapi->SeekSecQ8;
Return:
	return ret;
}
// Sec is in Q23.8
int MultiMediaParser_SeekTo(void *hWorkMem, long Sec)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	INT8U err;
	int ret;
	int Ack;
#ifdef GP_FREERTOS
    osEvent event;
    uint32_t msg;
#endif

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_SeekTo\r\n"));
	MemoryCheck();

	mmpapi->SeekSecQ8 = Sec;

#ifdef GP_FREERTOS
    msg = MMP_MSG_SEEK;
    err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
    if(err!=osOK)   RETURN(MEDIAPARSER_OS_ERR);
    event = osMessageGet(mmpapi->Ack, osWaitForever);
    if (event.status != osEventMessage) RETURN(MEDIAPARSER_OS_ERR);
    Ack = (int)event.value.v;
#else
	err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_SEEK);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);

	Ack = (int)OSMboxPend(mmpapi->Ack, 0, &err);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
	if(Ack!=MULTIMEDIAPARSER_ACK_OK) RETURN(MEDIAPARSER_KERNEL_ERR);
	DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));

	ret = mmpapi->SeekSecQ8;
Return:
	return ret;
}

int MultiMediaParser_SeekToFrame(void *hWorkMem, long FrameNo)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	INT8U err;
	int ret;
	int Ack;
#ifdef GP_FREERTOS
    osEvent event;
    uint32_t msg;
#endif
	const void *vTrk = mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel);

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_SeekToFrame\r\n"));
	MemoryCheck();

	if (!vTrk)
		return 0;

	if (FrameNo >= mmpapi->Parser->GetTotalSamples(vTrk))
		return 0;

	mmpapi->SeekSample = FrameNo;

#ifdef GP_FREERTOS
    msg = MMP_MSG_SEEK;
    err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
    if(err!=osOK)   RETURN(MEDIAPARSER_OS_ERR);
    event = osMessageGet(mmpapi->Ack, osWaitForever);
    if (event.status != osEventMessage) RETURN(MEDIAPARSER_OS_ERR);
    Ack = (int)event.value.v;
#else
	err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_SEEK);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);

	Ack = (int)OSMboxPend(mmpapi->Ack, 0, &err);
	if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
	if(Ack!=MULTIMEDIAPARSER_ACK_OK) RETURN(MEDIAPARSER_KERNEL_ERR);
	DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));

	ret = FrameNo;
Return:
	return ret;
}

void MultiMediaParser_SetFrameDropLevel(void *hWorkMem, int n)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	if(n>256) n = 256;
	else if(n<0) n = 0;
	mmpapi->FrameDropLevel = n;
}

int MultiMediaParser_EnableAudStreaming(void *hWorkMem, int flag)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	INT8U err;
	int ret;
	int Ack;
#ifdef GP_FREERTOS
	osEvent event;
	uint32_t msg;
#endif

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_EnableAudStreaming\r\n"));
	MemoryCheck();

	if(mmpapi->Parser->GetAudHandle(mmpapi->kernel))
	{
		if(flag)
		{
			if(mmpapi->flag & MMP_FLAG_FOLLOW_MODE)
			{
#ifdef GP_FREERTOS
                msg = MMP_MSG_DISABLE_FOLLOW_MODE;
                err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
                if(err!=osOK) RETURN(MEDIAPARSER_OS_ERR);
                event = osMessageGet(mmpapi->Ack, osWaitForever);
                if (event.status != osEventMessage) RETURN(MEDIAPARSER_OS_ERR);
                Ack = (int)event.value.v;
#else
				err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_DISABLE_FOLLOW_MODE);
				if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);

				Ack = (int)OSMboxPend(mmpapi->Ack, 0, &err);
				if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
				if(Ack!=MULTIMEDIAPARSER_ACK_OK) RETURN(MEDIAPARSER_KERNEL_ERR);
				DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));
			}
		}
		else
		{
			if(!(mmpapi->flag & MMP_FLAG_FOLLOW_MODE))
			{
#ifdef GP_FREERTOS
                msg = MMP_MSG_ENABLE_FOLLOW_MODE;
                err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
                if(err!=osOK) RETURN(MEDIAPARSER_OS_ERR);
#else
				err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_ENABLE_FOLLOW_MODE);
				if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
			}
		}
	}

	ret = 0;
Return:
	return ret;
}

int MultiMediaParser_SetReversePlay(void *hWorkMem, int flag)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	INT8U err;
	int ret;
	int Ack;
#ifdef GP_FREERTOS
    osEvent event;
    uint32_t msg;
#endif
	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_EnableReversePlay\r\n"));
	MemoryCheck();

	if(flag)
	{
		if(!(mmpapi->flag & MMP_FLAG_REVERSE_PLAY))
		{
#ifdef GP_FREERTOS
            msg = MMP_MSG_ENABLE_REVERSE_PLAY;
            err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
            if(err!=osOK) RETURN(MEDIAPARSER_OS_ERR);
            event = osMessageGet(mmpapi->Ack, osWaitForever);
            if (event.status != osEventMessage) RETURN(MEDIAPARSER_OS_ERR);
            Ack = (int)event.value.v;
#else
			err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_ENABLE_REVERSE_PLAY);
			if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
			Ack = (int)OSMboxPend(mmpapi->Ack, 0, &err);
			if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
			if(Ack!=MULTIMEDIAPARSER_ACK_OK) RETURN(MEDIAPARSER_KERNEL_ERR);
			DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));
		}
	}
	else
	{
		if(mmpapi->flag & MMP_FLAG_REVERSE_PLAY)
		{
#ifdef GP_FREERTOS
            msg = MMP_MSG_DISABLE_REVERSE_PLAY;
            err = osMessagePut(mmpapi->Msg, (INT32U)&msg, 0);
            if(err!=osOK) RETURN(MEDIAPARSER_OS_ERR);
            event = osMessageGet(mmpapi->Ack, osWaitForever);
            if (event.status != osEventMessage) RETURN(MEDIAPARSER_OS_ERR);
            Ack = (int)event.value.v;
#else
			err = OSQPost(mmpapi->Msg, (void*)MMP_MSG_DISABLE_REVERSE_PLAY);
			if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
			Ack = (int)OSMboxPend(mmpapi->Ack, 0, &err);
			if(err!=OS_NO_ERR) RETURN(MEDIAPARSER_OS_ERR);
#endif
			if(Ack!=MULTIMEDIAPARSER_ACK_OK) RETURN(MEDIAPARSER_KERNEL_ERR);
			DEBUG_ONLY(DBG_PRINT("MULTIMEDIAPARSER_ACK_OK\r\n"));
		}
	}

	ret = 0;
Return:
	return ret;
}


//////////////////////////////////////////////////////////////////////////////////
//
// Audio Functions
//
//////////////////////////////////////////////////////////////////////////////////

int MultiMediaParser_IsEOA(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *aTrk = mmpapi->Parser->GetAudHandle((void*)mmpapi->kernel);

	if(aTrk==0) return 1;

	if(mmpapi->AudRest) {
		return 0;
	}

	// audio data decode end
	if(mmpapi->flag & MMP_FLAG_EOA) {
		return 1;
	}

	// audio data read end
	if(mmpapi->flag & MMP_FLAG_AUD_READ_END) {
		return 2;
	}

	return 0;
}

long MultiMediaParser_GetAudTrackDur(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *aTrk = mmpapi->Parser->GetAudHandle((void*)mmpapi->kernel);
	if(aTrk) return mmpapi->Parser->GetTotalSec(aTrk);
	else return 0;
}

const GP_WAVEFORMATEX *MultiMediaParser_GetAudInfo(const void *hWorkMem, long *Len)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *aTrk = mmpapi->Parser->GetAudHandle((void*)mmpapi->kernel);
	if(aTrk)
	{
		return (const GP_WAVEFORMATEX*)mmpapi->Parser->GetExData(aTrk, Len);
	}
	else
		return 0;
}

const char *MultiMediaParser_GetAudRing(void *hWorkMem)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_GetAudRing\r\n"));
	MemoryCheck();

	if(!mmpapi->Parser->GetAudHandle(mmpapi->kernel)) return 0;
	return mmpapi->AudRing;
}

int MultiMediaParser_GetAudRingLen(void *hWorkMem)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;

	DEBUG_ONLY(DBG_PRINT("MultiMediaParser_GetAudRingLen\r\n"));
	MemoryCheck();

	if(!mmpapi->Parser->GetAudHandle(mmpapi->kernel)) return 0;
	return mmpapi->AudRingLen;
}

int MultiMediaParser_GetAudRingWI(void *hWorkMem)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;

	// DEBUG_ONLY(DBG_PRINT("MultiMediaParser_GetAudRingWI\r\n"));
	MemoryCheck();

	if(!mmpapi->Parser->GetAudHandle(mmpapi->kernel)) return 0;
	return mmpapi->AudRingWI;
}

int MultiMediaParser_RefillAudBuf(void *hWorkMem, int RI)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	int Len;
	int req;

	if(mmpapi->flag & MMP_FLAG_ERROR) {
		DEBUG_ONLY(DBG_PRINT("MultiMediaParser_RefillAudBuf flag error: 0x%x\r\n", mmpapi->flag));
		return -1;
	}
	// DEBUG_ONLY(DBG_PRINT("MultiMediaParser_RefillAudBuf\r\n"));
	MemoryCheck();

	if(!mmpapi->Parser->GetAudHandle(mmpapi->kernel)) return 0;

	Len = mmpapi->AudRingLen;
	if(RI >= Len) return MEDIAPARSER_PARAM_ERR;
	mmpapi->AudRingRI = RI;

	req = mmpapi->AudReqTx - mmpapi->AudReqRx;
	if(req < 0) {
		req =mmpapi->AudReqRx -  mmpapi->AudReqTx;
	}

	if(req < 2)
	{
#ifdef GP_FREERTOS
        uint32_t msg;
#endif
		mmpapi->AudReqTx++;
#ifdef GP_FREERTOS
        msg = MMP_MSG_REFILL_AUD;
		if (osMessagePut(mmpapi->Msg, (INT32U)&msg, 0) != osOK)
#else
		if(OSQPost(mmpapi->Msg, (void*)MMP_MSG_REFILL_AUD)!=OS_NO_ERR)
#endif
			return MEDIAPARSER_OS_ERR;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////
//
// Video Functions
//
//////////////////////////////////////////////////////////////////////////////////
int MultiMediaParser_IsEOV(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel);
	if(vTrk==0) return 1;

	if(mmpapi->flag & MMP_FLAG_REVERSE_PLAY)
	{
		if(mmpapi->vCnt<=0) return 1;
	}
	else
	{
		if(mmpapi->vCnt>=mmpapi->VidMaxSample) return 1;
	}
	return 0;
}

int MultiMediaParser_GetNumberOfFrameInVidBuf(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI*)hWorkMem;
	int ret;

	// DEBUG_ONLY(DBG_PRINT("MultiMediaParser_GetNumberOfFrameInVidBuf\r\n"));
	MemoryCheck();

	ret = mmpapi->VidBufWI - mmpapi->VidBufRI;
	if(ret<0) ret += mmpapi->NumVidBuf;

	return ret;
}

long MultiMediaParser_GetVidCurTime(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI*)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void *)mmpapi->kernel);
	if(vTrk)
		return (long)MMP_udiv64by32(mmpapi->Parser->GetTickRate(vTrk), mmpapi->vUserTime << 8);
	else
		return 0;
}

long MultiMediaParser_GetVidCurNumberOfVideo(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI*)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void *)mmpapi->kernel);
	if(vTrk)
		return mmpapi->Parser->Tick2Sample((void*)vTrk, mmpapi->vUserTime);
	else
		return 0;
}

long MultiMediaParser_GetVidTrackDur(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel);
	if(vTrk) return mmpapi->Parser->GetTotalSec(vTrk);
	else return 0;
}

const GP_BITMAPINFOHEADER *MultiMediaParser_GetVidInfo(const void *hWorkMem, long *Len)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel);
	if(vTrk)
		return (const GP_BITMAPINFOHEADER*)mmpapi->Parser->GetExData(vTrk, Len);
	else
		return 0;
}

const char *MultiMediaParser_GetVidBuf(void *hWorkMem, long *Len, long *Duration, int *ErrID)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;
	// const char *buf = NULL;
	const char *ptr = 0;
	int ret;

	if(mmpapi->flag & MMP_FLAG_ERROR) {
		DEBUG_ONLY(DBG_PRINT("MultiMediaParser_GetVidBuf flag error: 0x%x\r\n", mmpapi->flag));
		RETURN(-1);
	}
	// DEBUG_ONLY(DBG_PRINT("MultiMediaParser_GetVidBuf\r\n"));
	MemoryCheck();

	*Len = 0;
	*Duration = 0;
	if(mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel)==0) RETURN(0);

	if(mmpapi->VidBufRI!=mmpapi->VidBufWI)
	{
		int req;
		int RI = mmpapi->VidBufRI;
		ptr = mmpapi->VidData[RI];
        //	為了快進快退做的,因為Vidbuf 會先預讀Duration,因此當從快進切回normal
        //  會發生沒消化完的Vudbuf(Duration 仍是長的),導致慢動作
		if(mmpapi->FrameDropLevel!=0)
		{
		  *Duration = for_fast_Duration;
		}
		else
		{
		  *Duration = mmpapi->VidDur[RI];
		}
        //	為了快進快退做的,因為Vidbuf 會先預讀Duration,因此當從快進切回normal
        //  會發生沒消化完的Vudbuf(Duration 仍是長的),導致慢動作
		mmpapi->vUserTime = mmpapi->VidTime[RI];
		*Len = mmpapi->VidBufLen[RI];
		RI++;
		if(RI>=mmpapi->NumVidBuf) RI -= mmpapi->NumVidBuf;
		mmpapi->VidBufRI = RI;

		req = mmpapi->VidReqTx - mmpapi->VidReqRx;
		if(req < 0) {
			req = mmpapi->VidReqRx - mmpapi->VidReqTx;
		}

		mmpapi->ReadVidCnt++;
		if ((req < 2) && ((mmpapi->ReadVidCnt >= 2) || mmpapi->NumVidBuf <= 10))
		{
#ifdef GP_FREERTOS
            uint32_t msg;
#endif
			mmpapi->VidReqTx++;
#ifdef GP_FREERTOS
            msg = MMP_MSG_REFILL_VID;
            if (osMessagePut(mmpapi->Msg, (INT32U)&msg, 0) != osOK)
#else
			if(OSQPost(mmpapi->Msg, (void*)MMP_MSG_REFILL_VID)!=OS_NO_ERR)
#endif
				RETURN(MEDIAPARSER_OS_ERR);
		}
	}
	ret = 0;

Return:
	*ErrID = ret;
	if(ret<0)
	{
		ptr = 0;
	}
	return ptr;
}

long MultiMediaParser_GetVidTickRate(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel);
	if(vTrk)
		return mmpapi->Parser->GetTickRate(vTrk);
	else
		return 1;
}

long MultiMediaParser_GetVidTotalSamples(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel);
	if(vTrk) {
		return mmpapi->Parser->GetTotalSamples(vTrk);
	}
	else {
		return 0;
	}
}

int MultiMediaParser_GetVidFrameRate(const void *hWorkMem)
{
	const PARSERAPI *mmpapi = (const PARSERAPI *)hWorkMem;
	const void *vTrk = mmpapi->Parser->GetVidHandle((void*)mmpapi->kernel);
	if(vTrk) {
		return mmpapi->Parser->GetFrameRate(vTrk);
	}
	else {
		return 0;
	}
}

int MultiMediaParser_RegisterReadFunc(int ReadFunc(short, unsigned int, unsigned int))
{
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	if(ReadFunc) {
		pRead = ReadFunc;
		return 0;
	}
#endif
	return -1;
}

#ifndef SEEK64_EN
int MultiMediaParser_RegisterSeekFunc(int SeekFunc(short, int, short))
#else
int MultiMediaParser_RegisterSeekFunc(long long SeekFunc(short, long long, short))
#endif
{
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	if(SeekFunc) {
		pSeek = SeekFunc;
		return 0;
	}
#endif
	return -1;
}

void *MultiMediaParser_ReCreate(
	int fid,
	long FileSize,
	int AudRingLen,
	int VidBufSize,
	int VidBufNum,
	int prio,
	const void *FcnTab,
	int (*ErrHandler)(int ErrID),
	int *ErrID)
{
	PARSERAPI *mmpapi = NULL;
	int ret = 0, i;
	int t;
	void *vTrk, *aTrk;

#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	if(pRead == 0) {
		pRead = read;
	}

	if(pSeek == 0) {
	#ifndef SEEK64_EN
		pSeek = lseek;
	#else
		pSeek = lseek64;
	#endif
	}
#endif

	// allocate PARSERAPI memory /////////////////////////////////
	t = (sizeof(PARSERAPI) + 3) >> 2 << 2;
	t += (AudRingLen + 3) >> 2 << 2;
	t += (((const MEDIA_PARSER_FCN_TAB*)FcnTab)->GetWorkMemSize() + 3) >> 2 << 2;
	mmpapi = (PARSERAPI*)MALLOC(t);
	if(mmpapi==NULL) RETURN(MEDIAPARSER_MEM_ERR);
	MEMSET(mmpapi, 0, t);

	if (VidBufNum <= 0)
		VidBufNum = NUM_VID_BUF;

	mmpapi->VidBuf = (char **)MALLOC(VidBufNum*sizeof(char*));
	mmpapi->VidData = (char **)MALLOC(VidBufNum*sizeof(char*));
	mmpapi->VidDur = (long *)MALLOC(VidBufNum*sizeof(long));
	mmpapi->VidTime = (long *)MALLOC(VidBufNum*sizeof(long));
	mmpapi->VidBufMax = (int *)MALLOC(VidBufNum*sizeof(int));
	mmpapi->VidBufLen = (int *)MALLOC(VidBufNum*sizeof(int));
	mmpapi->VidIsKey = (int *)MALLOC(VidBufNum*sizeof(int));

	if (!mmpapi->VidBuf || !mmpapi->VidData ||
		!mmpapi->VidDur || !mmpapi->VidTime ||
		!mmpapi->VidBufMax || !mmpapi->VidBufLen || !mmpapi->VidIsKey)
	{
		if (mmpapi->VidBuf) FREE(mmpapi->VidBuf);
		if (mmpapi->VidData) FREE(mmpapi->VidData);
		if (mmpapi->VidDur) FREE(mmpapi->VidDur);
		if (mmpapi->VidTime) FREE(mmpapi->VidTime);
		if (mmpapi->VidBufMax) FREE(mmpapi->VidBufMax);
		if (mmpapi->VidBufLen) FREE(mmpapi->VidBufLen);
		if (mmpapi->VidIsKey) FREE(mmpapi->VidIsKey);

		FREE(mmpapi);
		RETURN(MEDIAPARSER_MEM_ERR);
	}

	mmpapi->NumVidBuf = VidBufNum;
#if __OPTIMISE_LEVEL<2
	DEBUG_ONLY(g_MMPAPI = mmpapi);
#endif

	// read back work memory //////////////////////////////
	if(pfuncLoad == 0) {
		RETURN(MEDIAPARSER_PARAM_ERR);
	}

	ret = pfuncLoad((unsigned int)mmpapi, t);
	if(mmpapi->workmem_size != t) {
		RETURN(MEDIAPARSER_PARAM_ERR);
	}

	// init varaible /////////////////////////////////////
	mmpapi->ReadVidCnt = 0;
	mmpapi->Msg = 0;
	mmpapi->Ack = 0;
	for(i=0;i<mmpapi->NumVidBuf;i++) {
		mmpapi->VidBuf[i] = 0;
	}

	// initial PARSERAPI /////////////////////////////////
	mmpapi->Parser = FcnTab;
	// set AudRing
	t = (sizeof(PARSERAPI) + 3) >> 2 << 2;
	mmpapi->AudRing = (char*)mmpapi + t;
	// set kernel working memory
	t += (AudRingLen + 3) >> 2 << 2;
	mmpapi->kernel = (char*)mmpapi + t;

	mmpapi->AudRingLen = AudRingLen;
	mmpapi->ErrHandler = ErrHandler;
	mmpapi->fid = fid;

	// check is the same file ///////////////////////////////
	t = FTELL(fid);
	if(t != mmpapi->BaseOffset) {
		RETURN(MEDIAPARSER_PARAM_ERR);
	}

	ret = SEEK(fid, 0, SEEK_END);
	SEEK(fid, t, SEEK_SET);
	if(ret != mmpapi->FileLength) {
		RETURN(MEDIAPARSER_PARAM_ERR);
	}

	// set flag /////////////////////////////////////////////
	mmpapi->flag &= ~(MMP_FLAG_REVERSE_PLAY | MMP_FLAG_FOLLOW_MODE);

	// resume play by filesize = 0 //////////////////////////
	ret = mmpapi->Parser->Init(
		mmpapi->kernel,
		mmpapi->ExData,
		EXDATA_LEN,
		fid,
		0);
	if(ret<0) RETURN(ret);

	// allocate video buffer /////////////////////////////////
	for(i=0;i<mmpapi->NumVidBuf;i++) {
		mmpapi->VidBuf[i] = (char*)MALLOC(mmpapi->VidBufMax[i]);
		if(mmpapi->VidBuf[i]==NULL) RETURN(MEDIAPARSER_MEM_ERR);
		DEBUG_ONLY(DBG_PRINT("VidBuf[%d]=0x%x\r\n", i, mmpapi->VidBuf[i]));
	}

	vTrk = mmpapi->Parser->GetVidHandle(mmpapi->kernel);
	if(vTrk)
	{
		mmpapi->SeekSecQ8 = MMP_udiv64by32(mmpapi->Parser->GetTickRate(vTrk), mmpapi->vUserTime << 8);
		DEBUG_ONLY(DBG_PRINT("MultiMediaParser_ReCreate=%dSec\r\n", mmpapi->SeekSecQ8 >> 8));

		ret = Seek(mmpapi);
		if(ret<0) RETURN(ret);

		mmpapi->TotalSkipFrame = mmpapi->TotalNullFrame = 0;
		mmpapi->VidBufWI = mmpapi->VidBufRI = 0;
		ret = ReadVideo(mmpapi, 1);
		if(ret<0) RETURN(ret);
	}

	aTrk = mmpapi->Parser->GetAudHandle(mmpapi->kernel);
	if(aTrk)
	{
		mmpapi->AudRingRI = mmpapi->AudRingWI = mmpapi->AudRest = 0;
		ret = ReadAudio(mmpapi);
		if(ret<0) RETURN(ret);
	}

	// config OS
#if !defined(LUNUX_SYSTEM) && !defined(GP_FREERTOS)
	mmpapi->Msg = OSQCreate(mmpapi->MsgQ, MSGQ_SIZE);	if(mmpapi->Msg==NULL) RETURN(MEDIAPARSER_OS_ERR);
	mmpapi->Ack = OSMboxCreate(0);						if(mmpapi->Ack==NULL) RETURN(MEDIAPARSER_OS_ERR);

	mmpapi->GAP0 = 0x12345678;
	MEMSET(mmpapi->stack, 0, sizeof(mmpapi->stack));
	mmpapi->GAP1 = 0x87654321;
	if(OSTaskCreate(
		MultiMediaParser_Task,
		mmpapi,
		mmpapi->stack + sizeof(mmpapi->stack)/sizeof(OS_STK)-1,
		prio) != OS_NO_ERR)
	{
		RETURN(MEDIAPARSER_OS_ERR);
	}
#elif defined(GP_FREERTOS)
    {
        osMessageQDef_t def = {MSGQ_SIZE, sizeof(void*), NULL};
        osMessageQDef_t ack_def = {1, sizeof(void*), NULL};
        mmpapi->Msg = osMessageCreate(&def, NULL );	if(mmpapi->Msg==NULL) RETURN(MEDIAPARSER_OS_ERR);
        mmpapi->Ack = osMessageCreate(&ack_def, NULL);	if(mmpapi->Ack==NULL) RETURN(MEDIAPARSER_OS_ERR);
    }

	mmpapi->GAP0 = 0x12345678;
	//MEMSET(mmpapi->stack, 0, sizeof(mmpapi->stack));
	mmpapi->GAP1 = 0x87654321;
	{
        osThreadDef_t thread_def = {"MultiMediaParser_Task", MultiMediaParser_Task, prio, 0, MMP_STACK_SIZE*4};
        if (!osThreadCreate(&thread_def, NULL))
            RETURN(MEDIAPARSER_OS_ERR);
	}
#else
	mmpapi->Msg = &mmpapi->msg_queue;
	mmpapi->Ack = &mmpapi->ack_queue;
	if(mcp_mbox_init(mmpapi->Msg) == 0) {
		RETURN(MEDIAPARSER_OS_ERR);
	}

	if(mcp_mbox_init(mmpapi->Ack) == 0) {
		RETURN(MEDIAPARSER_OS_ERR);
	}

	mmpapi->thread = mcp_thread_create(MultiMediaParser_Task, (void *)mmpapi, prio);
	if(mmpapi->thread == 0) {
		RETURN(MEDIAPARSER_OS_ERR);
	}
#endif
	ret = 0;

Return:
	*ErrID = ret;
	if(ret<0 && mmpapi)
	{
		MultiMediaParser_Release(mmpapi);
		mmpapi = NULL;
	}
	return mmpapi;
}

int MultiMediaParser_StoreState(void *hWorkMem)
{
	PARSERAPI *mmpapi = (PARSERAPI*)hWorkMem;

	if(pfuncStore == 0) {
		return -1;
	}

	return pfuncStore((unsigned int)mmpapi, (unsigned int)mmpapi->workmem_size);
}

int MultiMediaParser_RegisterStoreFunc(int StoreFunc(unsigned int, unsigned int))
{
	if(StoreFunc) {
		pfuncStore = StoreFunc;
		return 0;
	}

	return -1;
}

int MultiMediaParser_RegisterLoadFunc(int LoadFunc(unsigned int, unsigned int))
{
	if(LoadFunc) {
		pfuncLoad = LoadFunc;
		return 0;
	}

	return -1;
}

const char *MultiMediaParser_GetVersion(void)
{
	DBG_PRINT("MULTIMEDIA_PARSER_SVN_VERSION = %s \r\n",MULTIMEDIA_PARSER_SVN_VERSION);
	DBG_PRINT("MULTIMEDIA_PARSER_LIB_BUILD_TIME = %s \r\n",MULTIMEDIA_PARSER_LIB_BUILD_TIME);
	return "GP$221 Generalplus Multi-Media Parser 2016.09.07";
}

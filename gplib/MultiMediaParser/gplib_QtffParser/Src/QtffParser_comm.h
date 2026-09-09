#ifndef __QTFFPARSER_COMM_H__
#define __QTFFPARSER_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

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

	#define MEMCPY				memcpy
	#define MEMSET				memset
	#define MALLOC				malloc
	#define FREE				free
	#define NEW_LINE			"\n"
	#define ENABLE_DEBUG_INFO
	#define INVALID_FILE_ID		0
#else
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	#include "application.h"
#else
	//__CC_ARM
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
#ifdef GP_FREERTOS
    #include "gplib.h"

    extern void print_string(CHAR *fmt, ...);
#endif
#endif //LINUX_SYSTEM

#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	// extern varaible
	extern int (*pRead)(short fd, unsigned int buf, unsigned int size);
#ifndef SEEK64_EN
	extern int (*pSeek)(short fd, int offset0, short fromwhere);
#else
	extern long long (*pSeek)(short fd, long long offset0, short fromwhere);
#endif //SEEK64_EN

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
	#ifndef SEEK64_EN
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
	#define MEMCPY(dst, src, len)	gp_memcpy((INT8S*)(dst), (INT8S *)(src), (len))
	#define MALLOC(size)			gp_malloc_align(size, 4)
	#define FREE(addr)				gp_free((void *)addr);
	#define NEW_LINE				"\r\n"

#elif defined(GP_FREERTOS)

#ifndef SEEK64_EN
	#define __seek	lseek
#else
	#define __seek	lseek64
#endif

	#define READ(fid, buf, len) \
	{ \
		if(read((fid), (INT32U)(buf), (len))!=(INT32S)(len)) \
			RETURN(MEDIAPARSER_FILE_READ_ERR); \
	}

#ifndef SEEK64_EN
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
    #else
	#define SEEK(fid, offset, org)	\
	if(org==SEEK_SET) \
	{ \
		if(lseek64((fid), (offset), (org))!=(INT32S)(offset)) \
			RETURN(MEDIAPARSER_FILE_SEEK_ERR); \
	} \
	else \
	{ \
		lseek64((fid), (offset), (org)); \
	}
    #endif
#ifndef SEEK64_EN
	__inline long FILELENGTH(int fid)
#else
	__inline long long FILELENGTH(int fid)
#endif
	{
#ifndef SEEK64_EN
		long pos = lseek(fid, 0, SEEK_CUR);
		long ret = lseek(fid, 0, SEEK_END);
		lseek(fid, pos, SEEK_SET);
#else
		long long pos = lseek64(fid, 0, SEEK_CUR);
		long long ret = lseek64(fid, 0, SEEK_END);
		lseek64(fid, pos, SEEK_SET);
#endif
		return ret;
	}

	#define FTELL(fid)				__seek((fid), 0, SEEK_CUR)
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
	#define MEMSET(ptr, c, len)		memset((void *)(ptr), (c), (len))
	#define MEMCPY(dst, src, len)	memcpy((void*)(dst), (void *)(src), (len))
	#define MALLOC(size)			malloc(size)
	#define FREE(addr)				free((void *)addr);
	#define NEW_LINE				"\n"

	#define INT8U					UINT8
	#define INT8S					SINT8
	#define INT32U					UINT32
	#define INT32S					SINT32
#endif //LINUX_SYSTEM

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
#define MAX_TRACK_NUM			3
#define MAX_VID_TRK_NUM			2
#define MAX_AUD_TRK_NUM			1
#define DEFAULT_AUD_SIZE		4096
#define SECTOR_SHIFT			9
#define SECTOR_SIZE				(1<<SECTOR_SHIFT)


#define VID_BUF_OVERHEAD		6		// (sizeof(WORD) - 1)*2

#define SHIFT_STTS				6					// Time to Sample
#define SHIFT_STSS				7					// Sync Sample
#define SHIFT_STCO				7
#define SHIFT_STSZ				7
#define SHIFT_STSC				5

#define MAX_TIME_TO_SAMPLE_BUF	(1<<SHIFT_STTS)		//  64 *  8-byte
#define MAX_SYNC_SAMPLE_BUF		(1<<SHIFT_STSS)		// 128 *  4-byte
#define MAX_CHUNK_OFFSET_BUF	(1<<SHIFT_STCO)		// 128 *  4-byte
#define MAX_SAMPLE_SIZE_BUF		(1<<SHIFT_STSZ)		// 128 *  4-byte
#define MAX_SAMPLE_TO_CHUNK_BUF	(1<<SHIFT_STSC)		//  32 * 12-byte


/////////////////////////////////////////////////////////////////////
// flags
// #define TRACK_FLAG_EOIDX		0x00000001
#define FLAG_MDAT_ATOM_EXIST	0x00000001
#define FLAG_MOOV_ATOM_EXIST	0x00000002

/////////////////////////////////////////////////////////////////////
// macro
// extern "C" void QtErr(int x);
// #define RETURN(x) {ret = x; QtErr(ret); goto Return;}
#define RETURN(x) {ret = x; goto Return;}

__inline unsigned short ByteSwap16(unsigned short t)
{
	return (t >> 8) | (t << 8);
}

#define BYTE_REVERSE4(a) \
	((((unsigned long)(a) & 0xFF000000) >> 24) |\
	(((unsigned long)(a) & 0x00FF0000) >>  8) |\
	(((unsigned long)(a) & 0x0000FF00) <<  8) |\
	(((unsigned long)(a) & 0x000000FF) << 24))

#define BYTE_REVERSE2(a) \
	((((unsigned short)(a) & 0xFF00) >> 8) |\
	(((unsigned short)(a) & 0x00FF) <<  8))

#define MAKEFOURCC(ch0, ch1, ch2, ch3)				\
	((unsigned long)(unsigned char)(ch0)		|	\
	((unsigned long)(unsigned char)(ch1) << 8)   |	\
	((unsigned long)(unsigned char)(ch2) << 16)  |	\
	((unsigned long)(unsigned char)(ch3) << 24))

#define READ_STRUCT(ptr, STRUCT, fid) \
{\
	READ(fid, ptr, sizeof(STRUCT)); \
	sub_len -= sizeof(STRUCT); \
	QtffParser_Struct_ByteReverse((void*)ptr, sizeof(STRUCT)>>2, STRUCT##_FMT);\
}

/////////////////////////////////////////////////////////////////////
// Qtff atom format
#define FTYP_FMT			"14"
#define MVHD_FMT			"4" "44444211" "444444444" "4444444"
#define TKHD_FMT			"4" "44444" "11" "22" "444444444" "44"
#define MDHD_FMT			"4" "44442"
#define MDHDV1_FMT			"4" "88482"
#define	HDLR_FMT			"4" "11144"
#define	VMHD_FMT			"4" "22"
#define	SMHD_FMT			"4" "2"
#define	STSD_FMT			"4" "4"
#define	ESDS_FMT			"4"
#define	STTS_FMT			"4" "4"
#define STSS_FMT			"4" "4"
#define	STSC_FMT			"4" "4"
#define	STSZ_FMT			"4" "4" "4"
#define STCO_FMT			"4" "4"
// #define TIME_TO_SAMPLE_FMT	"44"
#define STSD_GENERAL_FMT	"22"
#define STSD_AUD_V0_FMT		"21224"
#define STSD_AUD_V1_FMT		"4444"
#define STSD_VID_FMT		"24442444"

/////////////////////////////////////////////////////////////////////
// fourcc
#define _moov MAKEFOURCC('m','o','o','v')
#define _mdat MAKEFOURCC('m','d','a','t')
#define _mvhd MAKEFOURCC('m','v','h','d')
#define _trak MAKEFOURCC('t','r','a','k')
#define _iods MAKEFOURCC('i','o','d','s')
#define _tkhd MAKEFOURCC('t','k','h','d')
#define _tref MAKEFOURCC('t','r','e','f')
#define _rtng MAKEFOURCC('r','t','n','g')
#define _uuid MAKEFOURCC('u','u','i','d')
#define _mdia MAKEFOURCC('m','d','i','a')
#define _mdhd MAKEFOURCC('m','d','h','d')
#define _hdlr MAKEFOURCC('h','d','l','r')
#define _minf MAKEFOURCC('m','i','n','f')
#define _smhd MAKEFOURCC('s','m','h','d')
#define _vmhd MAKEFOURCC('v','m','h','d')
#define _hmhd MAKEFOURCC('h','m','h','d')
#define _nmhd MAKEFOURCC('n','m','h','d')
#define _dinf MAKEFOURCC('d','i','n','f')
#define _dref MAKEFOURCC('d','r','e','f')
#define _stbl MAKEFOURCC('s','t','b','l')
#define _stsd MAKEFOURCC('s','t','s','d')
#define _stts MAKEFOURCC('s','t','t','s')
#define _stss MAKEFOURCC('s','t','s','s')
#define _stsc MAKEFOURCC('s','t','s','c')
#define _stsz MAKEFOURCC('s','t','s','z')
#define _meta MAKEFOURCC('m','e','t','a')
#define _udta MAKEFOURCC('u','d','t','a')
#define _hnti MAKEFOURCC('h','n','t','i')
#define _sdp  MAKEFOURCC('s','d','p',' ')
#define _rtp  MAKEFOURCC('r','t','p',' ')
#define _stco MAKEFOURCC('s','t','c','o')
#define _ftyp MAKEFOURCC('f','t','y','p')
#define _thum MAKEFOURCC('t','h','u','m')

#define _vide MAKEFOURCC('v','i','d','e')
#define _soun MAKEFOURCC('s','o','u','n')

#define _NONE	MAKEFOURCC('N','O','N','E')
#define _raw	MAKEFOURCC('r','a','w',' ')
#define _twos	MAKEFOURCC('t','w','o','s')
#define _sowt	MAKEFOURCC('s','o','w','t')
#define _MAC3	MAKEFOURCC('M','A','C','3')
#define _MAC6	MAKEFOURCC('M','A','C','6')
#define _ima4	MAKEFOURCC('i','m','a','4')
#define _fl32	MAKEFOURCC('f','l','3','2')
#define _fl64	MAKEFOURCC('f','l','6','4')
#define _in24	MAKEFOURCC('i','n','2','4')
#define _ulaw	MAKEFOURCC('u','l','a','w')
#define _alaw	MAKEFOURCC('a','l','a','w')
#define _dvca	MAKEFOURCC('d','v','c','a')
#define _QMDC	MAKEFOURCC('Q','M','D','C')
#define _Qclp	MAKEFOURCC('Q','c','l','p')
#define _mp3	MAKEFOURCC('.','m','p','3')
#define _mp4a	MAKEFOURCC('m','p','4','a')

#define _cvid	MAKEFOURCC('c','v','i','d')
#define _jpeg	MAKEFOURCC('j','p','e','g')
#define _smc 	MAKEFOURCC('s','m','c',' ')
#define _rle 	MAKEFOURCC('r','l','e',' ')
#define _rpza	MAKEFOURCC('r','p','z','a')
#define _kpcd	MAKEFOURCC('k','p','c','d')
#define _png 	MAKEFOURCC('p','n','g',' ')
#define _mjpa	MAKEFOURCC('m','j','p','a')
#define _mjpb	MAKEFOURCC('m','j','p','b')
#define _SVQ1	MAKEFOURCC('S','V','Q','1')
#define _SVQ3	MAKEFOURCC('S','V','Q','3')
#define _mp4v	MAKEFOURCC('m','p','4','v')
#define _dvc 	MAKEFOURCC('d','v','c',' ')
#define _dvcp	MAKEFOURCC('d','v','c','p')
#define _gif 	MAKEFOURCC('g','i','f',' ')
#define _h263	MAKEFOURCC('h','2','6','3')
#define _s263	MAKEFOURCC('s','2','6','3')
#define _d263	MAKEFOURCC('d','2','6','3')
#define _tiff	MAKEFOURCC('t','i','f','f')
#define _raw    MAKEFOURCC('r','a','w',' ')
#define _2vuy   MAKEFOURCC('2','v','u','y')
#define _yuv2   MAKEFOURCC('y','u','v','2')
#define _v308   MAKEFOURCC('v','3','0','8')
#define _v408   MAKEFOURCC('v','4','0','8')
#define _v216   MAKEFOURCC('v','2','1','6')
#define _v410   MAKEFOURCC('v','4','1','0')
#define _v210   MAKEFOURCC('v','2','1','0')
#define _avc1	MAKEFOURCC('a','v','c','1')

#define _wave	MAKEFOURCC('w','a','v','e')
#define _frma	MAKEFOURCC('f','r','m','a')
#define _esds	MAKEFOURCC('e','s','d','s')

#define _avcC	MAKEFOURCC('a','v','c','C')

#define _edts	MAKEFOURCC('e','d','t','s')
#define _free	MAKEFOURCC('f','r','e','e')
#define _skip	MAKEFOURCC('s','k','i','p')
#define _wide	MAKEFOURCC('w','i','d','e')
#define _pnot	MAKEFOURCC('p','n','o','t')

#define _tapt	MAKEFOURCC('t','a','p','t')
#define _iods	MAKEFOURCC('i','o','d','s')
#define _PICT	MAKEFOURCC('P','I','C','T')
#define _ctts	MAKEFOURCC('c','t','t','s')

#define _btrt	MAKEFOURCC('b','t','r','t')


/////////////////////////////////////////////////////////////////////
// structures
typedef union
{
	long v;
	char s[4];
} FOURCC;

typedef struct
{
	unsigned long len;
	FOURCC type;
} ATOM_HDR;

typedef struct
{
	FOURCC  Major_Brand;
	FOURCC  Minor_Version;
} FTYP;

typedef struct
{
	long  ver_flag;			// Version // Flags
	long  Creation_time;		// Creation time
	long  Modification_time;	// Modification time
	long  Time_scale;			// Time scale
	long  Duration;			// Duration
	long  Preferred_rate;		// Preferred rate
	short Preferred_volume;	// Preferred volume
	char  Reserved[10];		// Reserved
	long  Matrix_structure[9];	// Matrix structure
	long  Preview_time;		// Preview time
	long  Preview_duration;	// Preview duration
	long  Poster_time;			// Poster time
	long  Selection_time;		// Selection time
	long  Selection_duration;	// Selection duration
	long  Current_time;		// Current time
	long  Next_track_ID;		// Next track ID
} MVHD;

typedef struct
{
	long  ver_flag;           // Version // Flags
	long  Creation_time;      // Creation time
	long  Modification_time;  // Modification time
	long  Track_ID;           // Track ID
	long  Reserved;           // Reserved
	long  Duration;           // Duration
	long  Reserved2[2];       // Reserved
	short Layer;              // Layer
	short Alternate_group;    // Alternate group
	short Volume;             // Volume
	short Reserved3;          // Reserved
	long  Matrix_structure[9];// Matrix structure
	long  Track_width;        // Track width
	long  Track_height;       // Track height
} TKHD;

typedef struct
{
	long  ver_flag;
	long  Creation_time;
	long  Modification_time;
	long  Time_scale;
	long  Duration;
	short Language;
	short Quality;
} MDHD;

typedef struct
{
	long  		ver_flag;
	long long 	Creation_time;
	long long  	Modification_time;
	long  		Time_scale;
	long long 	Duration;
	short Language;
	short Quality;
} __attribute__((__packed__)) MDHDV1;

typedef struct
{
	long	ver_flag;
	FOURCC  Component_type;
	FOURCC  Component_subtype;
	long	Component_manufacturer;
	long	Component_flags;
	long	Component_flags_mask;
} HDLR;

typedef struct
{
	long  ver_flag;
	short Graphics_mode;
	short Opcolor[3];
} VMHD;

typedef struct
{
	long  ver_flag;
	short Balance;
	short Reserved;
} SMHD;

typedef struct
{
	long  ver_flag;
	long  Number_of_entries;
} STSD; // Sample Description Atoms

typedef struct
{
	long  ver_flag;
} ESDS;

typedef struct
{
	long  ver_flag;
	long  Number_of_entries;
} STTS;

typedef struct
{
	long  ver_flag;
	long  Number_of_entries;
} STSS;

typedef struct
{
	long  ver_flag;
	long  Number_of_entries;
} STSC;

typedef struct
{
	long  ver_flag;
	long  Sample_size;
	long  Number_of_entries;
} STSZ;

typedef struct
{
	long  ver_flag;
	long  Number_of_entries;
} STCO;

typedef struct
{
	long Sample_count;		// number of sample
	long Sample_duration;	// time-unit per sample
} TIME_TO_SAMPLE;

typedef struct
{
	long First_chunk;
	long Samples_per_chunk;
	long Sample_description_ID;
} CLUSTER;

typedef struct
{
	// long  Sample_description_size;
	// long  Data_format;
	char  Reserved[6];
	short Data_reference_index;
} STSD_GENERAL;

typedef struct
{
	short			Version;
	short			Revision_level;
	FOURCC			Vendor;
	short			Number_of_channels;
	short			Sample_size;	// A 16-bit integer that specifies the number of bits in each uncompressed sound sample. Set this
									// field to 8 for 8-bit sound, and to 16 for 16-bit sound.
	short			Compression_ID;
	short			Packet_size;		// A 16-bit integer that must be set to 0.
	unsigned long	Sample_rate;
} STSD_AUD_V0;

typedef struct
{
	unsigned long  Samples_per_packet;	// the number of uncompressed samples in a packet
	unsigned long  Bytes_per_packet;	// the resulting compressed number of bytes for one channel
	unsigned long  Bytes_per_frame;		// the resulting compressed number of bytes for all channels (channels * bytes per packet)
	unsigned long  Bytes_per_sample;	// the size of an uncompressed sample
} STSD_AUD_V1;

#ifdef WIN32
	#pragma pack(2)
	typedef struct
	{
		unsigned short	wFormatTag;
		unsigned short	nChannels;
		unsigned long	nSamplesPerSec;
		unsigned long	nAvgBytesPerSec;
		unsigned short	nBlockAlign;
		unsigned short	wBitsPerSample;
		unsigned short	cbSize;
	} GP_WAVEFORMATEX;	// strf
	#pragma pack()
#else
	//__CC_ARM
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
	typedef __packed struct
	{
		unsigned short	wFormatTag;
		unsigned short	nChannels;
		unsigned long	nSamplesPerSec;
		unsigned long	nAvgBytesPerSec;
		unsigned short	nBlockAlign;
		unsigned short	wBitsPerSample;
		unsigned short	cbSize;
	} GP_WAVEFORMATEX;	// strf
#else
	typedef struct
	{
		unsigned short	wFormatTag;
		unsigned short	nChannels;
		unsigned long	nSamplesPerSec;
		unsigned long	nAvgBytesPerSec;
		unsigned short	nBlockAlign;
		unsigned short	wBitsPerSample;
		unsigned short	cbSize;
	} __attribute__((__packed__)) GP_WAVEFORMATEX;	// strf
#endif
#endif

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

#ifdef WIN32
#pragma pack(2)
typedef struct
{
	short			Version;
	short			Revision_level;
	long			Vendor;
	long			Temporal_quality;
	long			Spatial_quality;
	short			Width;
	short			Height;
	long			Horizontal_resolution;
	long			Vertical_resolution;
	long			Data_size;
	short			Frame_count;
	char			Compressor_name[32];
	short			Depth;
	short			Color_table_ID;
} STSD_VID;
#pragma pack()
#else
//__CC_ARM
#if !defined(LINUX_SYSTEM) && !defined(GP_FREERTOS)
typedef __packed struct
{
	short			Version;
	short			Revision_level;
	long			Vendor;
	long			Temporal_quality;
	long			Spatial_quality;
	short			Width;
	short			Height;
	long			Horizontal_resolution;
	long			Vertical_resolution;
	long			Data_size;
	short			Frame_count;
	char			Compressor_name[32];
	short			Depth;
	short			Color_table_ID;
} STSD_VID;
#else
typedef struct
{
	short			Version;
	short			Revision_level;
	long			Vendor;
	long			Temporal_quality;
	long			Spatial_quality;
	short			Width;
	short			Height;
	long			Horizontal_resolution;
	long			Vertical_resolution;
	long			Data_size;
	short			Frame_count;
	char			Compressor_name[32];
	short			Depth;
	short			Color_table_ID;
} __attribute__((__packed__)) STSD_VID;
#endif
#endif


class CTime2Sample
{
private:
	STTS			m_hdr;
#ifndef SEEK64_EN
	long			m_FileOffset;
#else
	long long		m_FileOffset;
#endif
	FILEHANDLE		m_fid;

	long			m_curSample;
	long			m_curTime;
	long			m_startSample;

	short			m_cntCache;
	short			m_maxCache;
	long			m_page;
	TIME_TO_SAMPLE	m_Cache[MAX_TIME_TO_SAMPLE_BUF];

	// functions
private:
	// int Read();
	int Read();
public:
	int Init(FILEHANDLE fid);
	int InitResumePlay(FILEHANDLE fid);
	long Sample2Time(long tarSample);
	long Time2Sample(long tarTime);


	__inline STTS *GetHdr() { return &m_hdr; };
#ifndef SEEK64_EN
	__inline void SetFileOffset(long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long GetFileOffset() const { return m_FileOffset; };
#else
	__inline void SetFileOffset(long long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long long GetFileOffset() const { return m_FileOffset; };
#endif
	__inline long GetCurTime() const { return m_curTime; };
	__inline long GetCurSample() const { return m_curSample; };
};

class CSyncSample
{
private:
	STSS		m_hdr;
#ifndef SEEK64_EN
	long 		m_FileOffset;
#else
	long long	m_FileOffset;
#endif
	FILEHANDLE	m_fid;

	long		m_maxSample;

	long		m_curSample;

	short		m_page;
	short		m_cnt;
	long		*m_Cache;

	// functions
private:
	int Read();
public:
	int Init(FILEHANDLE fid, long *SyncBuf, long maxSample);
	int InitResumePlay(FILEHANDLE fid, long *SyncBuf);
	int Update(long tarSample);
	int IsKeyFrame() const;
	long GetKeyFrame(long tarSample);

	__inline STSS *GetHdr() { return &m_hdr; };
#ifndef SEEK64_EN
	__inline void SetFileOffset(long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long GetFileOffset() const { return m_FileOffset; };
#else
	__inline void SetFileOffset(long long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long long GetFileOffset() const { return m_FileOffset; };
#endif

};

class CChunkOffset
{
private:
	STCO		m_hdr;
#ifndef SEEK64_EN
	long 		m_FileOffset, m_FileBaseOffset;
#else
	long long	m_FileOffset, m_FileBaseOffset;
#endif
	FILEHANDLE	m_fid;

	short		m_page;
	short		m_cnt;
	long		m_Cache[MAX_CHUNK_OFFSET_BUF];

	// functions
private:
	int Read();
public:
	int Init(FILEHANDLE fid, long FileBaseOffset);
	int InitResumePlay(FILEHANDLE fid);
	int Update(long tarChunk);

	__inline STCO *GetHdr() { return &m_hdr;};
#ifndef SEEK64_EN
	__inline void SetFileOffset(long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long GetFileOffset() const { return m_FileOffset; };
#else
	__inline void SetFileOffset(long long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long long GetFileOffset() const { return m_FileOffset; };
#endif
	__inline long GetChunkOffset() const { return m_Cache[m_cnt] + m_FileBaseOffset;};
};

class CSampleSize
{
private:
	STSZ		m_hdr;
#ifndef SEEK64_EN
	long 		m_FileOffset;
#else
	long long	m_FileOffset;
#endif
	FILEHANDLE	m_fid;

	long		m_curOffset;
	long		m_curBase;
	long		m_curRest;

	long		m_CBR;
	long		m_divFrameSize;
	long		m_mulPackageSize;
	short		m_page;
	short		m_cnt;
	long		m_Cache[MAX_SAMPLE_SIZE_BUF];

	// functions
private:
	int Read();
public:
	int Init(FILEHANDLE fid);
	int InitResumePlay(FILEHANDLE fid);
	int Update(long tarBase, long tarRest);
	long GetSize() const;
	long GetSampleOffset() const;

	__inline STSZ *GetHdr() { return &m_hdr; };
	__inline const STSZ *GetHdr() const { return &m_hdr; };
#ifndef SEEK64_EN
	__inline void SetFileOffset(long FileOffset) { m_FileOffset = FileOffset; };
	__inline long GetFileOffset() const { return m_FileOffset; };
#else
	__inline void SetFileOffset(long long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long long GetFileOffset() const { return m_FileOffset; };
#endif
	__inline void SetVBR() {m_CBR = 0;};
	__inline void SetCBR(long FrameSize, long PackageSize)
	{
		m_CBR = 1;
		m_divFrameSize = FrameSize;
		m_mulPackageSize = PackageSize;
	};
};

class CSample2Chunk
{
private:
	STSC		m_hdr;
#ifndef SEEK64_EN
	long 		m_FileOffset;
#else
	long long	m_FileOffset;
#endif
	FILEHANDLE	m_fid;

	long		m_maxChunk;

	long		m_startSample;
	long		m_curChunk;

	short		m_page;
	short		m_cnt;
	CLUSTER		m_Cache[MAX_SAMPLE_TO_CHUNK_BUF + 1];

	// functions
private:
	int Read();
public:
	int Init(FILEHANDLE fid, long maxChunk);
	int InitResumePlay(FILEHANDLE fid);
	int Update(long tarSample);

	__inline STSC *GetHdr() { return &m_hdr; };
#ifndef SEEK64_EN
	__inline void SetFileOffset(long FileOffset) { m_FileOffset = FileOffset; };
	__inline long GetFileOffset() const { return m_FileOffset; };
#else
	__inline void SetFileOffset(long long FileOffset) { m_FileOffset = FileOffset;	};
	__inline long long GetFileOffset() const { return m_FileOffset; };
#endif
	__inline long GetStartSample() const { return m_startSample;};
	__inline long GetCurChunk() const { return m_curChunk;};
	__inline long GetSamplesPerChunk() const {return m_Cache[m_cnt].Samples_per_chunk;};
};

typedef struct
{
	long flag;

	MDHDV1			mdhd;
	long			padding;
	HDLR			hdlr;
	STSD			stsd;
	STSD_GENERAL	stsd_general;
	FOURCC			fmt;
	ESDS			esds;
	char			*ExData;
	int				ExDataLen;

	CTime2Sample	stts;
	CSyncSample		stss;
	CChunkOffset	stco;
	CSampleSize		stsz;
	CSample2Chunk	stsc;

	long SyncBuf[MAX_SYNC_SAMPLE_BUF + 1];
	char vidHeaderData[1024];
	int vidHeaderSize;
	int lengthSizeMinusOne;
} TRACK_INFO;

typedef struct
{
	FILEHANDLE fid;
	long	flag;

	FTYP ftyp;
	MVHD mvhd;
	TKHD tkhd;

	TRACK_INFO Track[MAX_TRACK_NUM];
	TRACK_INFO *aTrk, *vTrk;
	TRACK_INFO *aTrkList[MAX_AUD_TRK_NUM], *vTrkList[MAX_VID_TRK_NUM];

	//long SyncBuf[MAX_SYNC_SAMPLE_BUF + 1];

	STSD_AUD_V0	AudFmtV0;
	STSD_AUD_V1	AudFmtV1;
	STSD_VID VidFmt;

	VMHD vmhd;
	SMHD smhd;

	int TrkCnt;
	int vTrkCnt;
	int aTrkCnt;

	int lev;
	char *ExData;
	char *ExData2;
	int ExDataLen;
	int ExDataLen2;
	//char vidHeaderData[1024];
	//int vidHeaderSize;
	//int lengthSizeMinusOne;
} QTFFPARSER;

// common functions
#ifdef ENABLE_DEBUG_INFO
	extern const char *indent(int i);
	extern const char *fourcc_to_str(long fourcc);
#endif

extern void QtffParser_Struct_ByteReverse(void *ptr, int m_cnt, const char *fmt);
extern void QtffParser_ByteSwap(void *_ptr, int m_cnt);

#ifdef __cplusplus
}
#endif


#endif //__QTFFPARSER_COMM_H__

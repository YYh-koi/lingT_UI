#ifndef __AVI_INDEX_DEF_H__
#define __AVI_INDEX_DEF_H__


#include "AviIndex.h"

#define NUM_IDX_BIT_SHIFT		8
#define IDX_CACHE_MAX_SIZE		(1<<NUM_IDX_BIT_SHIFT)

typedef struct
{
    FOURCC			ckid;
    unsigned long	dwFlags;
    unsigned long	dwChunkOffset;		// Position of chunk
    long			dwChunkLength;		// Length of chunk
} AVIINDEXENTRY;

class CAviIndex10 : public CAviBaseIndex
{
private:
	FILEHANDLE		m_fid;
#ifndef SEEK64_EN
	long 			m_idx1_pos;
#else
	long long 		m_idx1_pos; //benjamin
#endif
	long			m_NumberOfIdx;
#ifndef SEEK64_EN
	long			m_BaseOffset;
#else
	long long		m_BaseOffset;
#endif

	long			m_flag;

	AVIINDEXENTRY	m_cache[IDX_CACHE_MAX_SIZE];
	long			m_page;

	FOURCC			m_Fourcc1, m_Fourcc2;
	long			m_nBlockAlign;				// Bytes per frame; if m_nBlockAlign == 0 then each chunk is a frame
	long			m_CurSample;
	long			m_CurIdx;
	long			m_PassFrames;
	long			m_LastIdx; // all indexes between [0~m_vLastIdx] have been read and used by VidSTab


	// Section Table for speed up idx1 search
	long			m_STab[MAX_INDEX_SECTION];
	long			m_SOffset[MAX_INDEX_SECTION];
	long			m_nSTab;
	long			m_SectionSize;
	long			m_NextSSample;

private:
	void UpdateSTab(long refIdx, long Len);
	const AVIINDEXENTRY *GetEntry(long tarIdx, int *ErrID);

public:
	int Init(const AVIINDEXARGUMENT *arg);
	int InitResumePlay(const AVIINDEXARGUMENT *arg);
	__inline long GetNumberOfFrames() const {return m_PassFrames;};
#ifndef SEEK64_EN
	int Get(long tarFrame, long *Offset, long *Size, long *IsKey);
#else
	int Get(long tarFrame, long long *Offset, long *Size, long *IsKey);
#endif
	// __inline int IsEOS() const {return m_CurIdx+1>=m_NumberOfIdx;};
};


/////////////////////////////////////////////////////////////////////////////////
//
// OpenDML AVI File Format
//
/////////////////////////////////////////////////////////////////////////////////

//
#define AVI_INDEX_OF_INDEXES	0x00	// when each entry in aIndex
										// array points to an index chunk
#define AVI_INDEX_OF_CHUNKS		0x01	// when each entry in aIndex
										// array points to a chunk in the file
#define AVI_INDEX_IS_DATA		0x80	// when each entry is aIndex is
										// really the data
										// bIndexSubtype codes for INDEX_OF_CHUNKS
//
#define AVI_INDEX_2FIELD 0x01			// when fields within frames
										// are also indexed

struct _aviindex_chunk
{
	// FOURCC fcc;
	// long cb;
	short wLongsPerEntry;	// size of each entry in aIndex array
	char bIndexSubType;		// future use. must be 0
	char bIndexType;		// one of AVI_INDEX_* codes
	long nEntriesInUse;		// index of first unused member in aIndex array
	long dwChunkId;			// fcc of what is indexed
	long dwReserved[3];		// meaning differs for each index
							// type/subtype. 0 if unused
};

#ifdef __GNUC__
typedef struct __attribute__((__packed__)) _avistdindex_chunk
#else
typedef struct _avistdindex_chunk
#endif
{
	// FOURCC fcc;				// 'ix##'
	// long cb;
	short wLongsPerEntry;	// must be sizeof(aIndex[0])/sizeof(DWORD)
	char bIndexSubType;		// must be 0
	char bIndexType;		// must be AVI_INDEX_OF_CHUNKS
	long nEntriesInUse;		//
	long dwChunkId;			// '##dc' or '##db' or '##wb' etc..
	__int64 qwBaseOffset;	// all dwOffsets in aIndex array are
	// relative to this
	long dwReserved3;		// must be 0
} AVISTDINDEX, *PAVISTDINDEX;

typedef struct _avistdindex_entry
{
	long dwOffset;				// qwBaseOffset + this is absolute file offset
	unsigned long dwSize : 31;	// bit 31 is set if this is NOT a keyframe
	unsigned long NotKey :  1;
} AVISTDINDEXENTRY; // aIndex[];

#ifdef __GNUC__
typedef struct __attribute__((__packed__)) _avifieldindex_chunk
#else
typedef struct _avifieldindex_chunk
#endif
{
	// FOURCC fcc;				// 'ix##'
	// long cb;
	short wLongsPerEntry;	// must be 3 (size of each entry in
							// aIndex array)
	char bIndexSubType;		// AVI_INDEX_2FIELD
	char bIndexType;		// AVI_INDEX_OF_CHUNKS
	long nEntriesInUse;		//
	long dwChunkId;			// '##dc' or '##db'
	__int64 qwBaseOffset;	// offsets in aIndex array are relative to this
	long dwReserved3;		// must be 0
} AVIFIELDINDEX, * PAVIFIELDINDEX;

struct _avifieldindex_entry
{
	long dwOffset;
	long dwSize;			// size of all fields
							// (bit 31 set for NON-keyframes)
	long dwOffsetField2;	// offset to second field
}; // aIndex[ ];


typedef struct _avisuperindex_chunk {
	// FOURCC fcc;			// 'ix##'
	// long cb;				// size of this structure
	short wLongsPerEntry;	// must be 4 (size of each entry in aIndex array)
	char bIndexSubType;		// must be 0 or AVI_INDEX_2FIELD
	char bIndexType;		// must be AVI_INDEX_OF_INDEXES
	long nEntriesInUse;		// number of entries in aIndex array that
	// are used
	long dwChunkId;			// '##dc' or '##db' or '##wb', etc
	long dwReserved[3];		// must be 0
} AVISUPERINDEX, * PAVISUPERINDEX;

#ifdef __GNUC__
typedef struct __attribute__((__packed__)) _avispuerindex_entry
#else
typedef struct _avispuerindex_entry
#endif
{
	__int64 qwOffset;	// absolute file offset, offset 0 is
						// unused entry??
	long dwSize;		// size of index chunk at this offset
	long dwDuration;	// time span in stream ticks
}  AVISUPERINDEXENTRY; // aIndex[];



#define SUPERINDEX_CACHE_MAX_SIZE	32
#define STDINDEX_CACHE_MAX_SIZE		32

class CAviIndex20 : public CAviBaseIndex
{
private:
	FILEHANDLE m_fid;

#ifndef SEEK64_EN
	long				m_BaseOffset;
#else
	long long			m_BaseOffset; //benjamin
#endif
	long				m_nBlockAlign;
	long				m_TotalFrames;
	long				m_Current_Frame;

#ifndef SEEK64_EN
	long				m_SuperIndex_file_pos;
#else
	long long			m_SuperIndex_file_pos; //benjamin
#endif
	short				m_SuperIndex_page;
	short				m_SuperIndex_cur;
	long				m_SuperIndex_max;

#ifndef SEEK64_EN
	long				m_StdIndex_qwBaseOffset; // 64bit but store 32bit only
#else
	long long			m_StdIndex_qwBaseOffset;
#endif
	long				m_StdIndex_frame_base;
	long				m_StdIndex_frame_offset;
#ifndef SEEK64_EN
	long				m_StdIndex_file_pos;
#else
	long long			m_StdIndex_file_pos; //benjamin
#endif
	short				m_StdIndex_page;
	short				m_cur_frame;
	long				m_cur_frame_dur;
	long				m_StdIndex_max;

	AVISUPERINDEXENTRY	m_SuperIndex[SUPERINDEX_CACHE_MAX_SIZE];
	AVISTDINDEXENTRY	m_StdIndex[STDINDEX_CACHE_MAX_SIZE];

private:
	int LoadStdIndexChunk(int dir);

public:
	int Init(const AVIINDEXARGUMENT *arg);
	int InitResumePlay(const AVIINDEXARGUMENT *arg);
	__inline long GetNumberOfFrames() const {return m_TotalFrames;};
#ifndef SEEK64_EN
	int Get(long tarFrame, long *Offset, long *Size, long *IsKey);
#else
	int Get(long tarFrame, long long *Offset, long *Size, long *IsKey);
#endif
	// __inline int IsEOS() const {return m_TotalFrames <= m_Current_Frame;};
};



#endif // __AVI_INDEX_DEF_H__

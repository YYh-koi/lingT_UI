#include "AviParser.h"
#include "AviParser_comm.h"
#include "AviIndex.h"
#include "AviIndex_def.h"

extern "C" unsigned long AviParser_udiv32by32(unsigned long, unsigned long);


#define AVIIF_KEYFRAME			0x00000010 // this frame is a key frame.


int CAviIndex::Init(
	int type,
	const AVIINDEXARGUMENT *arg)
{
	/*int t = sizeof(CAviIndex);
	t = sizeof(CAviIndex10);
	t = sizeof(CAviIndex20);*/

#ifndef SEEK64_EN
	while(sizeof(CAviIndex)<sizeof(CAviIndex10));
	while(sizeof(CAviIndex)<sizeof(CAviIndex20));
#endif
	if(type==1)
	{
		ClassType = 1;
		return ((CAviIndex10*)this)->Init(arg);
	}
	if(type==2)
	{
		ClassType = 2;
		return ((CAviIndex20*)this)->Init(arg);
	}
	/*
	if(ChunkName==MAKEFOURCC('i','d','x','1'))
	{
		ClassType = 1;
		return ((CAviIndex10*)this)->Init(fid, ID, IsVideo, nBlockAlign, ChunkName, ChunkSize, BaseOffset, SectionSize);
	}

	if(ChunkName==MAKEFOURCC('i','n','d','x'))
	{
		ClassType = 2;
		return ((CAviIndex20*)this)->Init(fid, ID, IsVideo, nBlockAlign, ChunkName, ChunkSize, BaseOffset, SectionSize);
	}
	*/

	while(1);
}

int CAviIndex::InitResumePlay(const AVIINDEXARGUMENT *arg)
{
	if(ClassType==1) {
		return ((CAviIndex10*)this)->InitResumePlay(arg);
	}

	if(ClassType==2) {
		return ((CAviIndex20*)this)->InitResumePlay(arg);
	}

	while(1);
}

long CAviIndex::GetNumberOfFrames() const
{
	if(ClassType==1)
		return ((CAviIndex10*)this)->GetNumberOfFrames();
	if(ClassType==2)
		return ((CAviIndex20*)this)->GetNumberOfFrames();
	while(1);
}

#ifdef SEEK64_EN
int CAviIndex::Get(long tarFrame, long long *Offset, long *Size, long *IsKey) const
#else
int CAviIndex::Get(long tarFrame, long *Offset, long *Size, long *IsKey) const
#endif
{
	if(ClassType==1)
		return ((CAviIndex10*)this)->Get(tarFrame, Offset, Size, IsKey);
	if(ClassType==2)
		return ((CAviIndex20*)this)->Get(tarFrame, Offset, Size, IsKey);
	while(1);
}

/*
int CAviIndex::IsEOS() const
{
	if(ClassType==1)
		return ((CAviIndex10*)this)->IsEOS();
	if(ClassType==2)
		return ((CAviIndex20*)this)->IsEOS();
	while(1);
}*/


/////////////////////////////////////////////////////////////////////////////////
//
// AVI v1.0
//
/////////////////////////////////////////////////////////////////////////////////

int CAviIndex10::Init(const AVIINDEXARGUMENT *arg)
{
	int ret, offset;
	const AVIINDEXENTRY *ptr;
	int page, nRead, page_size;
	int RestIdx, refIdx;

	m_fid = arg->fid;
	m_idx1_pos = FTELL(arg->fid);
	m_BaseOffset = arg->BaseOffset;
	m_NumberOfIdx = arg->ChunkSize >> 4;
	m_SectionSize = arg->SectionSize;
	m_nBlockAlign = arg->nBlockAlign;

	if(arg->IsVideo)
	{
		m_Fourcc1.v = MAKEFOURCC('0','0'+arg->ID,'d','b');
		m_Fourcc2.v = MAKEFOURCC('0','0'+arg->ID,'d','c');
	}
	else
	{
		m_Fourcc1.v = MAKEFOURCC('0','0'+arg->ID,'w','b');
		m_Fourcc2.v = 0;
	}

	m_nSTab = 0;
	m_LastIdx = -1;
	m_PassFrames = 0;
	RestIdx = m_NumberOfIdx;
	refIdx = 0;
	m_NextSSample = 0;
	m_flag = 0;

	// find Chunk
	page = 0;
	nRead = 0;
	SEEK(m_fid, m_idx1_pos, SEEK_SET);
	while(RestIdx)
	{
		nRead = RestIdx;
		if(nRead>IDX_CACHE_MAX_SIZE)
			nRead = IDX_CACHE_MAX_SIZE;
		READ(m_fid, m_cache, nRead * sizeof(AVIINDEXENTRY));

		RestIdx -= nRead;
		ptr = m_cache;
		m_page = page++;
		if(nRead >= IDX_CACHE_MAX_SIZE)
		{
			page_size = IDX_CACHE_MAX_SIZE;
			nRead -= IDX_CACHE_MAX_SIZE;
		}
		else
		{
			page_size = nRead;
			nRead = 0;
		}
		for(offset=0;offset<page_size;offset++)
		{
			if((ptr->ckid.v==m_Fourcc1.v) || (m_Fourcc2.v && (ptr->ckid.v==m_Fourcc2.v)))
			{
				UpdateSTab(refIdx, ptr->dwChunkLength);
				m_CurSample = 0;
				m_CurIdx = m_STab[0];
				RETURN(0);
			}
			ptr++;
			refIdx++;
		}
	}
	RETURN(MEDIAPARSER_FORMAT_ERR);

Return:
	return ret;
}

int CAviIndex10::InitResumePlay(const AVIINDEXARGUMENT *arg)
{
	m_fid = arg->fid;
	return 0;
}

__inline static int ABS(int x)
{
	if(x<0) return -x;
	return x;
}

void CAviIndex10::UpdateSTab(long refIdx, long Len)
{
	long t;
	if(refIdx>m_LastIdx)
	{
		if(m_nBlockAlign) t = m_PassFrames + Len;
		else t = m_PassFrames + 1;

		m_LastIdx = refIdx;
		while(t>=m_NextSSample && m_nSTab<MAX_INDEX_SECTION)
		{
			m_STab[m_nSTab] = m_LastIdx;
			m_SOffset[m_nSTab] = m_NextSSample - m_PassFrames;
			m_NextSSample += m_SectionSize;
			m_nSTab++;
		}
		m_PassFrames = t;
	}
}

const AVIINDEXENTRY *CAviIndex10::GetEntry(long tarIdx, int *ErrID)
{
	int page, ret, rest, t;
	const AVIINDEXENTRY *ptr = 0;

	if(tarIdx<0) RETURN(0);
	if(tarIdx>=m_NumberOfIdx) RETURN(0);

	page = tarIdx >> NUM_IDX_BIT_SHIFT;
	rest = tarIdx - (page<<NUM_IDX_BIT_SHIFT);

	if(page!=m_page)
	{
		SEEK(m_fid, m_idx1_pos + page*IDX_CACHE_MAX_SIZE*sizeof(AVIINDEXENTRY), SEEK_SET);
		t = m_NumberOfIdx - (page<<NUM_IDX_BIT_SHIFT);
		if(t>IDX_CACHE_MAX_SIZE) t = IDX_CACHE_MAX_SIZE;
		m_page = page;
		READ(m_fid, m_cache, t*sizeof(AVIINDEXENTRY));
	}

	ptr = m_cache + rest;
	ret = 0;
Return:
	*ErrID = ret;
	if(ret<0) ptr = 0;
	return ptr;
}

#ifdef SEEK64_EN
int CAviIndex10::Get(long tarFrame, long long *Offset, long *Size, long *IsKey)
#else
int CAviIndex10::Get(long tarFrame, long *Offset, long *Size, long *IsKey)
#endif
{
	int ret;
	int t, Cnt;
	long refIdx, refSample;
	const AVIINDEXENTRY *ptr;
	//long m_NextSSample;

	if(m_Fourcc1.v==0)
	{
		ptr = 0;
		RETURN(0);
	}

	ptr = GetEntry(m_CurIdx, &ret);
	if(ret<0) RETURN(ret);
	if(ptr==0) RETURN(0); // out of range
	if(m_nBlockAlign) Cnt = ptr->dwChunkLength;
	else Cnt = 1;

	if(m_nBlockAlign) tarFrame *= m_nBlockAlign;

	if(m_CurSample<=tarFrame && tarFrame<=m_CurSample + Cnt)
	{
		// sequential read
		refSample = m_CurSample;
		refIdx = m_CurIdx;
	}
	else
	{
		// use table look up
		// t = (tarFrame + (m_aSectionSize >> 1)) / m_aSectionSize;
		t = AviParser_udiv32by32(m_SectionSize, tarFrame + (m_SectionSize >> 1));
		if(t>=m_nSTab) t = m_nSTab-1;

		refSample = t * m_SectionSize - m_SOffset[t];
		refIdx = m_STab[t];

		if(ABS(refSample - tarFrame) > ABS(m_CurSample - tarFrame))
		{
			refSample = m_CurSample;
			refIdx = m_CurIdx;
		}
		else
		{
			m_CurSample = refSample;
			m_CurIdx = refIdx;
			ptr = GetEntry(refIdx, &ret);
			if(ret<0) RETURN(ret);
			if(ptr==0) RETURN(0); // out of range
			if(m_nBlockAlign) Cnt = ptr->dwChunkLength;
			else Cnt = 1;
		}
	}

	//m_NextSSample = m_nSTab * m_SectionSize;

	while(tarFrame >= refSample + Cnt)
	{
		refSample += Cnt; // sample increase
		while(1) // search to next idx
		{
			refIdx++;
			ptr = GetEntry(refIdx, &ret);
			if(ret<0) RETURN(ret);
			if(ptr==0) RETURN(0); // out of range
			if(m_nBlockAlign) Cnt = ptr->dwChunkLength;
			else Cnt = 1;

			if((ptr->ckid.v==m_Fourcc1.v) || (m_Fourcc2.v && (ptr->ckid.v==m_Fourcc2.v)))
			{
				m_CurSample = refSample;
				m_CurIdx = refIdx;
				UpdateSTab(refIdx, ptr->dwChunkLength);
				break;
			}
		}
	}

	while(tarFrame < refSample)
	{
		while(1) // seek to previous audio idx
		{
			refIdx--;
			ptr = GetEntry(refIdx, &ret);
			if(ret<0) RETURN(ret);
			if(ptr==0) RETURN(0); // out of range
			if(m_nBlockAlign) Cnt = ptr->dwChunkLength;
			else Cnt = 1;

			if((ptr->ckid.v == m_Fourcc1.v) || (m_Fourcc2.v && (ptr->ckid.v == m_Fourcc2.v)))
			{
				refSample -= Cnt;
				m_CurSample = refSample;
				m_CurIdx = refIdx;
				break;
			}
		}
	}

	ret = 1;

Return:
	if(ret<0) ptr = 0;
	if(ptr)
	{
		t = tarFrame - refSample;
		*Offset = m_BaseOffset + ptr->dwChunkOffset + t;
		*Size = ptr->dwChunkLength - t;
		*IsKey = ptr->dwFlags & AVIIF_KEYFRAME ? 1 : 0;
	}
	else
	{
		*Offset = 0;
		*Size = 0;
		*IsKey = 0;
	}
	return ret;
}


/////////////////////////////////////////////////////////////////////////////////
//
// OpenDML AVI File Format
//
/////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_DEBUG_INFO
extern void *g_index_aud;
extern void *g_index_vid;

static CAviIndex10 *&g_index10_aud = (CAviIndex10*&)g_index_aud;
static CAviIndex10 *&g_index10_vid = (CAviIndex10*&)g_index_vid;
static CAviIndex20 *&g_index20_aud = (CAviIndex20*&)g_index_aud;
static CAviIndex20 *&g_index20_vid = (CAviIndex20*&)g_index_vid;
#endif


int CAviIndex20::Init(const AVIINDEXARGUMENT *arg)
{
	int ret, t;
	AVISUPERINDEX AviSuperIndex;

	m_fid = arg->fid;
	m_BaseOffset = arg->BaseOffset;
	m_nBlockAlign = arg->nBlockAlign;

	m_StdIndex_frame_base = 0;
	m_SuperIndex_page = 0;
	m_SuperIndex_cur = 0;
	m_TotalFrames = 0x7FFFFFFF;

	// read super index header //////////////////////////////////////
	READ(m_fid, &AviSuperIndex, sizeof(AVISUPERINDEX));

	if(AviSuperIndex.bIndexType!=AVI_INDEX_OF_INDEXES || AviSuperIndex.bIndexSubType!=0 || AviSuperIndex.wLongsPerEntry!=4)
		RETURN(MEDIAPARSER_FORMAT_ERR);
	m_SuperIndex_file_pos = FTELL(m_fid);

	m_SuperIndex_max = (arg->ChunkSize - sizeof(AVISUPERINDEX)) >> 4;
	if(m_SuperIndex_max > AviSuperIndex.nEntriesInUse)
		m_SuperIndex_max  = AviSuperIndex.nEntriesInUse;
	if(m_SuperIndex_max<=0) RETURN(MEDIAPARSER_FORMAT_ERR);


	// read super index entry
	t = m_SuperIndex_max;
	if(t>SUPERINDEX_CACHE_MAX_SIZE) t = SUPERINDEX_CACHE_MAX_SIZE;
	READ(m_fid, m_SuperIndex, t * sizeof(AVISUPERINDEXENTRY));

	ret = LoadStdIndexChunk(1);
	if(ret<0) RETURN(ret);

	ret = 0;
Return:
	return ret;
}

int CAviIndex20::InitResumePlay(const AVIINDEXARGUMENT *arg)
{
	m_fid = arg->fid;
	return 0;
}

int CAviIndex20::LoadStdIndexChunk(int dir)
{
	int ret, t;
	AVISTDINDEX AviStdIndex;
	CHUNKHDR hdr;

	// read std index header //////////////////////////////////////
	SEEK(m_fid, m_BaseOffset + m_SuperIndex[m_SuperIndex_cur].qwOffset, SEEK_SET);
	READ(m_fid, &hdr, sizeof(CHUNKHDR));
	READ(m_fid, &AviStdIndex, sizeof(AVISTDINDEX));

	m_StdIndex_qwBaseOffset = AviStdIndex.qwBaseOffset;
	if(AviStdIndex.bIndexType!=AVI_INDEX_OF_CHUNKS || AviStdIndex.bIndexSubType!=0 || AviStdIndex.wLongsPerEntry!=2)
		RETURN(MEDIAPARSER_FORMAT_ERR);
	m_StdIndex_file_pos = FTELL(m_fid);

	m_StdIndex_max = (hdr.len - sizeof(AVISTDINDEX)) >> 3;
	if(m_StdIndex_max > AviStdIndex.nEntriesInUse)
		m_StdIndex_max  = AviStdIndex.nEntriesInUse;
	if(m_StdIndex_max<=0) RETURN(MEDIAPARSER_FORMAT_ERR);

	// read std index entry
	if(dir) // forward
	{
		m_StdIndex_page = 0;
		m_cur_frame = 0;
		t = m_StdIndex_max;
		if(t>STDINDEX_CACHE_MAX_SIZE) t = STDINDEX_CACHE_MAX_SIZE;
		READ(m_fid, m_StdIndex, t * sizeof(AVISTDINDEXENTRY));

		if(m_nBlockAlign) m_cur_frame_dur = AviParser_udiv32by32(m_nBlockAlign, m_StdIndex[m_cur_frame].dwSize);
		else m_cur_frame_dur = 1;
		m_StdIndex_frame_offset = 0;
	}
	else // backward
	{
		m_StdIndex_page = m_StdIndex_max / STDINDEX_CACHE_MAX_SIZE;
		t = m_StdIndex_max - m_StdIndex_page * STDINDEX_CACHE_MAX_SIZE;
		m_cur_frame = t - 1;
		SEEK(m_fid, m_StdIndex_page*STDINDEX_CACHE_MAX_SIZE*sizeof(AVISTDINDEXENTRY),SEEK_CUR);
		READ(m_fid, m_StdIndex, t * sizeof(AVISTDINDEXENTRY));

		if(m_nBlockAlign) m_cur_frame_dur = AviParser_udiv32by32(m_nBlockAlign, m_StdIndex[m_cur_frame].dwSize);
		else m_cur_frame_dur = 1;
		m_StdIndex_frame_offset = m_SuperIndex[m_SuperIndex_cur].dwDuration - m_cur_frame_dur;
	}
	ret = 0;
Return:
	return ret;
}

#ifdef SEEK64_EN
int CAviIndex20::Get(long tarFrame, long long *Offset, long *Size, long *IsKey)
#else
int CAviIndex20::Get(long tarFrame, long *Offset, long *Size, long *IsKey)
#endif
{
	int ret;
	int SuperIndex_bk;
	int page_start, t;

	m_Current_Frame = tarFrame;

	if(tarFrame<0) RETURN(0);

	SuperIndex_bk = m_SuperIndex_cur;
	page_start = m_SuperIndex_page * SUPERINDEX_CACHE_MAX_SIZE;
	while(tarFrame < m_StdIndex_frame_base)
	{
		if(m_SuperIndex_page==0 && m_SuperIndex_cur==0) RETURN(0);
		if(m_SuperIndex_cur<=0) // previous super index page
		{
			m_SuperIndex_page--;
			page_start -= SUPERINDEX_CACHE_MAX_SIZE;
			// read super index page entry
			SEEK(m_fid, m_SuperIndex_file_pos + page_start * sizeof(AVISUPERINDEXENTRY), SEEK_SET);
			READ(m_fid, m_SuperIndex, SUPERINDEX_CACHE_MAX_SIZE * sizeof(AVISUPERINDEXENTRY));
			m_SuperIndex_cur = SUPERINDEX_CACHE_MAX_SIZE;
		}
		m_SuperIndex_cur--;
		m_StdIndex_frame_base -= m_SuperIndex[m_SuperIndex_cur].dwDuration;
	}

	while(tarFrame >= m_StdIndex_frame_base + m_SuperIndex[m_SuperIndex_cur].dwDuration)
	{
		if(page_start + m_SuperIndex_cur + 1 >= m_SuperIndex_max)
		{
			m_TotalFrames = m_StdIndex_frame_base + m_SuperIndex[m_SuperIndex_cur].dwDuration;
			RETURN(0);
		}
		m_StdIndex_frame_base += m_SuperIndex[m_SuperIndex_cur].dwDuration;
		m_SuperIndex_cur++;
		if(m_SuperIndex_cur>=SUPERINDEX_CACHE_MAX_SIZE) // next super index page
		{
			m_SuperIndex_page++;
			page_start += SUPERINDEX_CACHE_MAX_SIZE;
			t = m_SuperIndex_max - page_start;
			if(t>SUPERINDEX_CACHE_MAX_SIZE) t = SUPERINDEX_CACHE_MAX_SIZE;
			SEEK(m_fid, m_SuperIndex_file_pos + m_SuperIndex_page * SUPERINDEX_CACHE_MAX_SIZE * sizeof(AVISUPERINDEXENTRY), SEEK_SET);
			READ(m_fid, m_SuperIndex, t * sizeof(AVISUPERINDEXENTRY));
			m_SuperIndex_cur = 0;
		}
	}
	if(SuperIndex_bk<m_SuperIndex_cur)
	{
		// Load Std Index forward
		ret = LoadStdIndexChunk(1);
		if(ret<0) RETURN(ret);
	}
	else if(SuperIndex_bk>m_SuperIndex_cur)
	{
		// Load Std Index forward
		ret = LoadStdIndexChunk(0);
		if(ret<0) RETURN(ret);
	}

	page_start = m_StdIndex_page * STDINDEX_CACHE_MAX_SIZE;
	while(tarFrame < m_StdIndex_frame_base + m_StdIndex_frame_offset)
	{
		if(m_StdIndex_page==0 && m_cur_frame==0) RETURN(0); // never this condition
		if(m_cur_frame<=0) // previous std index page
		{
			m_StdIndex_page--;
			page_start -= STDINDEX_CACHE_MAX_SIZE;
			SEEK(m_fid, m_StdIndex_file_pos + page_start * sizeof(AVISTDINDEXENTRY), SEEK_SET);
			READ(m_fid, m_StdIndex, STDINDEX_CACHE_MAX_SIZE * sizeof(AVISTDINDEXENTRY));
			m_cur_frame = STDINDEX_CACHE_MAX_SIZE;
		}
		m_cur_frame--;
		m_StdIndex_frame_offset -= m_cur_frame_dur;

		if(m_nBlockAlign) m_cur_frame_dur = AviParser_udiv32by32(m_nBlockAlign, m_StdIndex[m_cur_frame].dwSize);
		else m_cur_frame_dur = 1;
	}

	while(tarFrame >= m_StdIndex_frame_base + m_StdIndex_frame_offset + m_cur_frame_dur)
	{
		if(page_start + m_cur_frame + 1 >= m_StdIndex_max)
		{
			m_TotalFrames = m_StdIndex_frame_base + m_StdIndex_frame_offset + m_cur_frame_dur;
			RETURN(0); // EOS
		}
		m_cur_frame++;
		m_StdIndex_frame_offset += m_cur_frame_dur;
		if(m_cur_frame>=STDINDEX_CACHE_MAX_SIZE) // next std index page
		{
			m_StdIndex_page++;
			page_start += STDINDEX_CACHE_MAX_SIZE;
			t = m_StdIndex_max - page_start;
			if(t>STDINDEX_CACHE_MAX_SIZE) t = STDINDEX_CACHE_MAX_SIZE;
			SEEK(m_fid, m_StdIndex_file_pos + page_start * sizeof(AVISTDINDEXENTRY), SEEK_SET);
			READ(m_fid, m_StdIndex, t * sizeof(AVISTDINDEXENTRY));
			m_cur_frame = 0;
		}

		if(m_nBlockAlign) m_cur_frame_dur = AviParser_udiv32by32(m_nBlockAlign, m_StdIndex[m_cur_frame].dwSize);
		else m_cur_frame_dur = 1;
	}

	*Offset =  m_StdIndex_qwBaseOffset + m_StdIndex[m_cur_frame].dwOffset;
	*Size   =  m_StdIndex[m_cur_frame].dwSize;
	*IsKey  = !m_StdIndex[m_cur_frame].NotKey;

	ret = 1;
Return:
	if(ret<=0)
	{
		*Offset = 0;
		*Size = 0;
		*IsKey = 0;
	}
	return ret;
}


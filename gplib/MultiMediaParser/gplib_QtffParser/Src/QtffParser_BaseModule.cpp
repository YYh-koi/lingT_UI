#include "QtffParser.h"
#include "QtffParser_comm.h"

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


extern "C" unsigned long QtffParser_udiv32by32(unsigned long, unsigned long);

int CTime2Sample::Read()
{
	int t;
	int ret;

	t = m_hdr.Number_of_entries - (m_page << SHIFT_STTS);
	if(t>MAX_TIME_TO_SAMPLE_BUF) t = MAX_TIME_TO_SAMPLE_BUF;
	else if(t<0) t = 0;

	m_maxCache = t;
	if(t)
	{
		SEEK(m_fid, m_FileOffset + m_page*(sizeof(TIME_TO_SAMPLE)<<SHIFT_STTS), SEEK_SET);
		READ(m_fid, m_Cache, t*sizeof(TIME_TO_SAMPLE));
		QtffParser_ByteSwap(m_Cache, t*sizeof(TIME_TO_SAMPLE));
	}

	ret = t;
Return:
	return ret;
}

int CTime2Sample::Init(FILEHANDLE fid)
{
	m_cntCache = 0;
	m_page = 0;
	m_curSample = 0;
	m_curTime = 0;
	m_fid = fid;
	return Read();
}

int CTime2Sample::InitResumePlay(FILEHANDLE fid)
{
	m_fid = fid;
	return 0;
}

long CTime2Sample::Sample2Time(long tarSample)
{
	long ret;
	int endSample = m_startSample + m_Cache[m_cntCache].Sample_count;
	int dur = m_Cache[m_cntCache].Sample_duration;

	if(tarSample < 0) tarSample = 0;
	while(tarSample>m_curSample)
	{
		if(m_maxCache==0) RETURN(0); // exceed the max sample of this stream
		if(tarSample>=endSample)
		{
			m_curTime += dur * (endSample - m_curSample);
			m_curSample = endSample;
			// next cache
			m_cntCache++;
			if(m_cntCache >= m_maxCache)
			{
				m_page++;
				ret = Read();
				if(ret<0) RETURN(ret);
				m_cntCache = 0;
			}
			m_startSample = endSample;
			endSample += m_Cache[m_cntCache].Sample_count;
			dur = m_Cache[m_cntCache].Sample_duration;
		}
		else
		{
			m_curTime += dur * (tarSample - m_curSample);
			m_curSample = tarSample;
		}
	}
	while(tarSample<m_curSample)
	{
		if(tarSample<m_startSample)
		{
			m_curTime -= dur * (m_curSample - m_startSample);
			m_curSample = m_startSample;
			// previous cache
			m_cntCache--;
			if(m_cntCache<0)
			{
				m_page--;
				ret = Read();
				if(ret<0) RETURN(ret);
				m_cntCache = m_maxCache - 1;
			}
			endSample = m_startSample;
			m_startSample -= m_Cache[m_cntCache].Sample_count;
			dur = m_Cache[m_cntCache].Sample_duration;
		}
		else
		{
			m_curTime -= dur * (m_curSample - tarSample);
			m_curSample = tarSample;
		}
	}

	ret = m_curTime;
Return:
	return ret;
}

long CTime2Sample::Time2Sample(long tarTime)
{
	int ret, t;

	int endSample = m_startSample + m_Cache[m_cntCache].Sample_count;
	int dur = m_Cache[m_cntCache].Sample_duration;
	long startTime = m_curTime - dur*(m_curSample - m_startSample);
	long endTime = m_curTime + dur*(endSample - m_curSample);

	if(tarTime < 0) tarTime = 0;
	while(tarTime>=m_curTime + dur)
	{
	#if 0
		if(m_maxCache==0) RETURN(0); // exceed the max time of this stream
	#else
		if(m_maxCache == 0) {
			// t = (long)((tarTime - m_curTime) / dur);
			t = QtffParser_udiv32by32(dur, tarTime - m_curTime);
			m_curSample += t;
			m_curTime += dur * t;
			break;
		}
	#endif
		if(tarTime>=endTime)
		{
			m_curTime = endTime;
			m_curSample = endSample;
			// next cache
			m_cntCache++;
			if(m_cntCache >= m_maxCache)
			{
				m_page++;
				ret = Read();
				if(ret<0) RETURN(ret);
				m_cntCache = 0;
			}
			m_startSample = endSample;
			startTime = endTime;
			endSample += m_Cache[m_cntCache].Sample_count;
			dur = m_Cache[m_cntCache].Sample_duration;
			endTime += dur * m_Cache[m_cntCache].Sample_count;
		}
		else
		{
			// t = (long)((tarTime - m_curTime) / dur);
			t = QtffParser_udiv32by32(dur, tarTime - m_curTime);
			m_curSample += t;
			m_curTime += dur * t;
		}
	}
	while(tarTime<m_curTime)
	{
		if(tarTime<startTime)
		{
			m_curTime = startTime;
			m_curSample = m_startSample;
			// previous cache
			m_cntCache--;
			if(m_cntCache<0)
			{
				m_page--;
				ret = Read();
				if(ret<0) RETURN(ret);
				m_cntCache = m_maxCache - 1;
			}
			endSample = m_startSample;
			endTime = startTime;
			m_startSample -= m_Cache[m_cntCache].Sample_count;
			dur = m_Cache[m_cntCache].Sample_duration;
			startTime -= dur * m_Cache[m_cntCache].Sample_count;
		}
		else
		{
			// t = (long)((m_curTime - tarTime + dur - 1) / dur);
			t = QtffParser_udiv32by32(dur, m_curTime - tarTime + dur - 1);
			m_curSample -= t;
			m_curTime -= dur * t;
		}
	}

	ret = m_curSample;
Return:
	return ret;
}

int CSyncSample::Read()
{
	int t, read_more;
	int ret;

	if(m_hdr.Number_of_entries==0) RETURN(0);

	t = m_hdr.Number_of_entries - (m_page<<SHIFT_STSS);
	if(t<0)
		RETURN(MEDIAPARSER_KERNEL_ERR);

	if(t>MAX_SYNC_SAMPLE_BUF) read_more = 1;
	else read_more = 0;

	if(t>MAX_SYNC_SAMPLE_BUF) t = MAX_SYNC_SAMPLE_BUF;
	// m_max = t;

	if(read_more) t++; // read one entry more that following current m_page

	if(t)
	{
		SEEK(m_fid, m_FileOffset + m_page*(sizeof(long)<<SHIFT_STSS), SEEK_SET);
		READ(m_fid, m_Cache, t*sizeof(long));
		// Byte Swap and then subtract one
		{
			int i;
			unsigned long *ptr = (unsigned long *)m_Cache;
			/*register*/ unsigned long u32;
			/*register*/ const unsigned long mask8H = 0xFF00FF00;
			/*register*/ const unsigned long mask8L = 0x00FF00FF;

			for(i=0;i<t;i++)
			{
				u32 = *ptr;
				u32 = ((u32 >> 8) & mask8L) | ((u32 << 8) & mask8H);
				u32 = (u32 >> 16) | (u32 << 16);
				*ptr++ = u32 - 1;
			}
		}
	}
	if(!read_more) m_Cache[t] = m_maxSample;

	ret = t;
Return:
	return ret;
}

int CSyncSample::Init(FILEHANDLE fid, long *SyncBuf, long maxSample)
{
	m_cnt = 0;
	m_page = 0;
	m_maxSample = maxSample;
	m_Cache = SyncBuf;
	m_fid = fid;
	if(SyncBuf)
		return Read();
	else
		return 0;
}

int CSyncSample::InitResumePlay(FILEHANDLE fid, long *SyncBuf)
{
	m_fid = fid;
	m_Cache = SyncBuf;
	return 0;
}

int CSyncSample::Update(long tarSample)
{
	int ret = 0;
	if(m_hdr.Number_of_entries==0) RETURN(1);
	if(m_Cache==0) RETURN(1);

	m_curSample = tarSample;

	// bound tarSample in [0, m_maxSample)
	if(tarSample<0) tarSample = 0;
	else if(tarSample>=m_maxSample) tarSample = m_maxSample - 1;

	while(tarSample>=m_Cache[m_cnt+1])
	{
		m_cnt++;
		if(m_cnt>=MAX_SYNC_SAMPLE_BUF)
		{
			m_page++;
			ret = Read();
			if(ret<0) RETURN(ret);
			m_cnt = 0;
		}
	}

	while(tarSample<m_Cache[m_cnt])
	{
		m_cnt--;
		if(m_cnt<0)
		{
			m_page--;
			Read();
			if(ret<0) RETURN(ret);
			m_cnt = MAX_SYNC_SAMPLE_BUF - 1;
		}
	}

	ret = 0;
Return:
	return ret;
}

long CSyncSample::GetKeyFrame(long tarSample)
{
	int ret;
	if(tarSample<0) tarSample = 0;
	if(m_hdr.Number_of_entries==0) RETURN(tarSample);
	if(m_Cache==0) RETURN(tarSample);

	ret = Update(tarSample);
	if(ret<0) RETURN(ret);

	if(m_Cache[m_cnt] >= tarSample) ret = m_Cache[m_cnt];
	else ret = m_Cache[m_cnt + 1];

Return:
	return ret;
}

int CSyncSample::IsKeyFrame() const
{
	if(m_hdr.Number_of_entries==0) return 1;
	if(m_Cache==0) return 1;
	return m_Cache[m_cnt] == m_curSample;
}

int CChunkOffset::Read()
{
	int t;
	int ret;

	t = m_hdr.Number_of_entries - (m_page << SHIFT_STCO);
	if(t>MAX_CHUNK_OFFSET_BUF) t = MAX_CHUNK_OFFSET_BUF;
	else if(t<0)
		RETURN(MEDIAPARSER_KERNEL_ERR);

	if(t)
	{
		SEEK(m_fid, m_FileOffset + m_page*(sizeof(long)<<SHIFT_STCO), SEEK_SET);
		READ(m_fid, m_Cache, t*sizeof(long));
		QtffParser_ByteSwap(m_Cache, t*sizeof(long));
	}

	ret = t;
Return:
	return ret;
}

int CChunkOffset::Init(FILEHANDLE fid, long FileBaseOffset)
{
	m_page = 0;
	m_fid = fid;
	m_FileBaseOffset = FileBaseOffset;
	return Read();
}

int CChunkOffset::InitResumePlay(FILEHANDLE fid)
{
	m_fid = fid;
	return 0;
}

int CChunkOffset::Update(long tarChunk)
{
	int ret, page;
	page = tarChunk >> SHIFT_STCO;
	if(m_page!=page)
	{
		m_page = page;
		ret = Read();
		if(ret<0) RETURN(ret);
	}
	m_cnt = tarChunk - (page<<SHIFT_STCO);

	ret = 0;
Return:
	return ret;
}

int CSampleSize::Read()
{
	int t;
	int ret;

	t = m_hdr.Number_of_entries - (m_page << SHIFT_STCO);
	if(t>MAX_CHUNK_OFFSET_BUF) t = MAX_CHUNK_OFFSET_BUF;
	else if(t<0) RETURN(MEDIAPARSER_KERNEL_ERR); // t = 0;

	if(t)
	{
		SEEK(m_fid, m_FileOffset + m_page*(sizeof(long)<<SHIFT_STCO), SEEK_SET);
		READ(m_fid, m_Cache, t*sizeof(long));
		QtffParser_ByteSwap(m_Cache, t*sizeof(long));
	}

	ret = t;
Return:
	return ret;
}

int CSampleSize::Init(FILEHANDLE fid)
{
	m_page = 0;
	m_curOffset = 0;
	m_curBase = 0;
	m_curRest = 0;
	m_fid = fid;

	if(m_hdr.Sample_size==0)
		return Read();
	else
		return 0;
}

int CSampleSize::InitResumePlay(FILEHANDLE fid)
{
	m_fid = fid;
	return 0;
}

int CSampleSize::Update(long tarBase, long tarRest)
{
	int ret;
	if(m_hdr.Sample_size)
	{
		if(m_CBR)
			m_curOffset = tarRest;
		else
			m_curOffset = m_hdr.Sample_size * tarRest;
	}
	else
	{
		int t;
		int cnt;
		long curSize;
		long curBase = m_curBase;
		long curRest = m_curRest;
		long curOffset = m_curOffset;

		if(tarBase!=curBase)
		{
			curBase = tarBase;
			curRest = 0;
			curOffset = 0;
		}

		cnt = curBase + curRest;
		t = cnt >> SHIFT_STSZ; // page
		cnt -= t << SHIFT_STSZ;

		if(t!=m_page)
		{
			m_page = t;
			ret = Read();
			if(ret<0) RETURN(ret);
		}
		if(tarRest>=curRest)
		{
			curSize = m_Cache[cnt];
			while(tarRest>curRest)
			{
				curOffset += curSize;
				curRest++;
				cnt++;
				if(cnt>=MAX_SAMPLE_SIZE_BUF)
				{
					m_page++;
					ret = Read();
					if(ret<0) RETURN(ret);
					cnt = 0;
				}
				curSize = m_Cache[cnt];
			}
		}
		else
		{
			while(tarRest<curRest)
			{
				curRest--;
				cnt--;
				if(cnt<0)
				{
					m_page--;
					ret = Read();
					if(ret<0) RETURN(ret);
					cnt = MAX_SAMPLE_SIZE_BUF-1;
				}
				curSize = m_Cache[cnt];
				curOffset -= curSize;
			}
		}

		m_cnt = cnt;
		m_curBase = curBase;
		m_curRest = curRest;
		m_curOffset = curOffset;
	}

	ret = 0;
Return:
	return ret;
}

long CSampleSize::GetSize() const
{
	if(m_hdr.Sample_size)
		return m_hdr.Sample_size;
	else
		return m_Cache[m_cnt];
}

long CSampleSize::GetSampleOffset() const
{
	long ret = m_curOffset;
	if(m_CBR)
	{
		if(m_divFrameSize>1)
		{
			// ret /= m_divFrameSize;
			ret = QtffParser_udiv32by32(m_divFrameSize, ret);
		}
		if(m_mulPackageSize>1)
			ret *= m_mulPackageSize;
	}
	return ret;
}

int CSample2Chunk::Read()
{
	int t;
	int ret, read_more;

	t = m_hdr.Number_of_entries - (m_page << SHIFT_STSC);

	if(t<0)
		RETURN(MEDIAPARSER_KERNEL_ERR);

	if(t>MAX_SAMPLE_TO_CHUNK_BUF) read_more = 1;
	else read_more = 0;

	if(t>MAX_SAMPLE_TO_CHUNK_BUF) t = MAX_SAMPLE_TO_CHUNK_BUF;
	if(read_more) t++;

	if(t)
	{
		SEEK(m_fid, m_FileOffset + m_page*(sizeof(CLUSTER)<<SHIFT_STSC), SEEK_SET);
		READ(m_fid, m_Cache, t*sizeof(CLUSTER));
		QtffParser_ByteSwap(m_Cache, t*sizeof(CLUSTER));
	}

	if(!read_more)
	{
		m_Cache[t].First_chunk = m_maxChunk;
		m_Cache[t].Samples_per_chunk = 1;
		m_Cache[t].Sample_description_ID = -1;
	}

	ret = t;
Return:
	return ret;
}

int CSample2Chunk::Init(FILEHANDLE fid, long maxChunk)
{
	int ret;

	m_page = 0;
	m_maxChunk = maxChunk + 1;
	m_fid = fid;

	ret = Read();
	if(ret<0) RETURN(ret);

	m_cnt = 0;
	m_curChunk = 0;
	m_startSample = 0;

	ret = 0;
Return:
	return ret;
}

int CSample2Chunk::InitResumePlay(FILEHANDLE fid)
{
	m_fid = fid;
	return 0;
}

int CSample2Chunk::Update(long tarSample)
{
	int ret;
	long endSample;
	long startChunk;
	long endChunk;

	endSample = m_startSample + m_Cache[m_cnt].Samples_per_chunk;
	startChunk = m_Cache[m_cnt].First_chunk - 1;
	endChunk = m_Cache[m_cnt + 1].First_chunk - 1;
	while(tarSample>=endSample)
	{
		m_curChunk++;
		if(m_curChunk >= endChunk)
		{
			m_cnt++;
			if(m_cnt>=MAX_SAMPLE_TO_CHUNK_BUF)
			{
				m_page++;
				ret = Read();
				if(ret<0) RETURN(ret);
				m_cnt = 0;
			}
			startChunk = endChunk;
			endChunk = m_Cache[m_cnt+1].First_chunk - 1;
		}
		m_startSample = endSample;
		endSample += m_Cache[m_cnt].Samples_per_chunk;
	}

	while(tarSample<m_startSample)
	{
		m_curChunk--;
		if(m_curChunk < startChunk)
		{
			m_cnt--;
			if(m_cnt<0)
			{
				m_page--;
				ret = Read();
				if(ret<0) RETURN(ret);
				m_cnt = MAX_SAMPLE_TO_CHUNK_BUF-1;
			}
			endChunk = startChunk;
			startChunk = m_Cache[m_cnt].First_chunk - 1;
		}
		endSample = m_startSample;
		m_startSample -= m_Cache[m_cnt].Samples_per_chunk;
	}

	if(m_curChunk==m_maxChunk)
		ret = 0;
	else
		ret = 1;
Return:
	return ret;
}
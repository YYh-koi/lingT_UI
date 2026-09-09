#ifndef __AVI_INDEX_H__
#define __AVI_INDEX_H__

class CAviBaseIndex
{
protected:
	int ClassType;
};

typedef struct
{
	FILEHANDLE fid;
#ifdef SEEK64_EN
	long long BaseOffset;
#else
	long BaseOffset;
#endif
	long ChunkSize;
	int nBlockAlign;
	int ID;
	int IsVideo;
	int SectionSize;
} AVIINDEXARGUMENT;

class CAviIndex : public CAviBaseIndex
{
public:
	int Init(int type, const AVIINDEXARGUMENT *arg);
	int InitResumePlay(const AVIINDEXARGUMENT *arg);
	long GetNumberOfFrames() const;
#ifdef SEEK64_EN
	int Get(long tarFrame, long long *Offset, long *Size, long *IsKey) const;
#else
	int Get(long tarFrame, long *Offset, long *Size, long *IsKey) const;
#endif
	// int IsEOS() const;

private:
#ifndef SEEK64_EN
	char WorkMem[6208];
#else
	char WorkMem[6220]; //dummy data make CAviIndex size equale to CAviIndex10 and larger than CAviIndex20
#endif
};


#endif // __AVI_INDEX_H__
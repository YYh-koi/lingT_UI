#include "QtffParser.h"
#include "QtffParser_comm.h"

#ifdef SEEK64_EN
extern long long Parse_AudParam(QTFFPARSER *qtff, long long Length, long Parent);
extern long long Parse_VidParam(QTFFPARSER *qtff, long long Length, long Parent);
#else
extern int Parse_AudParam(QTFFPARSER *qtff, int Length, long Parent);
extern int Parse_VidParam(QTFFPARSER *qtff, int Length, long Parent);
#endif

/*
extern "C" void QtErr(int x)
{
	if(x<0)
		x = x;
}
*/

static const MEDIA_PARSER_FCN_TAB QtffParserFcnTab =
{
	13,
	QtffParser_Init,
	QtffParser_GetWorkMemSize,
	QtffParser_SelectAudTrack,
	QtffParser_SelectVidTrack,
	QtffParser_GetAudHandle,
	QtffParser_GetVidHandle,
	QtffParser_GetAudTrkCnt,
	QtffParser_GetVidTrkCnt,

	// Track level function
	Track_GetTotalSamples,
	Track_GetTickRate,
	Track_GetTotalSec,
	Track_GetExData,
	Track_Tick2Sample,
	Track_GetFormatFourCC,
	Track_GetFrameRate,

	// Track stream control
	Track_GetSampleInfo,
	Track_GetKeyFrame,

	// other
	QtffParser_GetavcCData,
};

const MEDIA_PARSER_FCN_TAB * QtffParser_GetFcnTab()
{
	return &QtffParserFcnTab;
}

extern "C"
unsigned long QtffParser_udiv32by32(unsigned long d, unsigned long n)
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


static unsigned __int64 QtffParser_udiv64by32(unsigned long d, unsigned __int64 n64)
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

static INT32S vidH264VoVolParse(/*QTFFPARSER *qtff,*/ TRACK_INFO* Trk, INT8U *pbuf)
{
	#define VID_DATA_BUF_SIZE		1024

	INT8U  tmp1;
	INT8U  ucNum;
	INT8U  ucaHeader[4] = {0x00, 0x00, 0x00, 0x01};
	INT32U videoDSISize, N;
	INT32U i;

	pbuf += 82;
	Trk->vidHeaderSize = 0;
	N = 100;
	do {
		if(*(pbuf+0) == 0x61 && *(pbuf+1) == 0x76 && *(pbuf+2) == 0x63 && *(pbuf+3) == 0x43) {
			break;
		}

		pbuf++;
		N--;
	} while(N > 0);

	if(N == 0) {
		return -1;
	}

	pbuf += 8;
	tmp1 = *pbuf;  /* lengthSizeMinusOne */
	pbuf++;
	//qtff->lengthSizeMinusOne = (INT8U)(tmp1 & 0x03);
	Trk->lengthSizeMinusOne = (INT8U)(tmp1 & 0x03);
	/*
	if (qtff->lengthSizeMinusOne == 0 ||
		qtff->lengthSizeMinusOne == 1 ||
		qtff->lengthSizeMinusOne == 2)
	{
		if (pvidH264Data == NULL)
		{
			pvidH264Data = (INT8I *)Malloc(VID_DATA_BUF_SIZE);
			if (pvidH264Data == NULL)
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}*/

	tmp1 = *pbuf;  /* numOfSequenceParameterSets */
	pbuf++;
	ucNum = (INT8U)(tmp1 & 0x1F);
	for (i=0; i<ucNum; i++)
	{
		tmp1 = *pbuf;
		pbuf++;
		videoDSISize = tmp1 << 8;
		tmp1 = *pbuf;
		pbuf++;
		videoDSISize |= tmp1;

		MEMCPY((void *)(Trk->vidHeaderData + Trk->vidHeaderSize), ucaHeader, 4);
		Trk->vidHeaderSize += 4;

		MEMCPY((void *)(Trk->vidHeaderData + Trk->vidHeaderSize), pbuf, videoDSISize);
		Trk->vidHeaderSize += videoDSISize;
		pbuf += videoDSISize;
	}

	tmp1 = *pbuf;  /* numOfPictureParameterSets */
	pbuf++;
	ucNum = tmp1;
	for (i=0; i<ucNum; i++)
	{
		tmp1 = *pbuf;
		pbuf++;
		videoDSISize = tmp1 << 8;
		tmp1 = *pbuf;
		pbuf++;
		videoDSISize |= tmp1; /*pictureParameterSetLength*/

		MEMCPY((void *)(Trk->vidHeaderData + Trk->vidHeaderSize), ucaHeader, 4);
		Trk->vidHeaderSize += 4;

		MEMCPY((void *)(Trk->vidHeaderData + Trk->vidHeaderSize), pbuf, videoDSISize);
		Trk->vidHeaderSize += videoDSISize;
		pbuf += videoDSISize;
	}

	return 0;
}

#ifndef SEEK64_EN
static int QtffParser(QTFFPARSER *qtff, long Length, long Parent)
#else
static long long QtffParser(QTFFPARSER *qtff, long long Length, long Parent)
#endif
{
	ATOM_HDR hdr;
	int t;
#ifndef SEEK64_EN
	int ret;
	long main_len, sub_len;
#else
	long long main_len, sub_len, ret;
#endif
	int RestDataType;

	TRACK_INFO *Trk = &qtff->Track[qtff->TrkCnt];

	main_len = Length;
	while(main_len>0)
	{
		// Read header
		if(main_len < (long)sizeof(ATOM_HDR))
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s(<SizeErr> %u, %XH)", indent(qtff->lev), main_len, main_len));
			hdr.len = main_len;
			sub_len = main_len;
			RestDataType = 0;
			goto EndOfParseAtom;
		}

		READ(qtff->fid, &hdr, sizeof(ATOM_HDR));
		hdr.len = BYTE_REVERSE4(hdr.len);
		RestDataType = 1;

		if(hdr.len<(signed)sizeof(ATOM_HDR))
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %u, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
			hdr.len = main_len;
			sub_len = main_len;
			RestDataType = 0;
			goto EndOfParseAtom;
		}
		else if(hdr.len>(unsigned long)main_len)
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %u, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
			hdr.len = main_len;
		}
		else
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(%u, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
		}

		sub_len = hdr.len;
		sub_len -= 8;
		switch(hdr.type.v)
		{
		case _ftyp:
			if(Parent==0)
			{
				READ_STRUCT(&qtff->ftyp, FTYP, qtff->fid);
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _mdat:
			if(Parent!=0)
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			else
			{
				qtff->flag |= FLAG_MDAT_ATOM_EXIST;
			}
			RestDataType = 0;
			break;

		case _moov:
			if(Parent!=0)
			{
				RestDataType = 0;
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			else
			{
				qtff->flag |= FLAG_MOOV_ATOM_EXIST;
			}
			break;

		case _mvhd:
			if(Parent==_moov)
			{
				READ_STRUCT(&qtff->mvhd, MVHD, qtff->fid);
			}
			RestDataType = 0;
			break;

		case _trak:
			if(Parent!=_moov)
			{
				RestDataType = 0;
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			// New track condition 1
			if(qtff->TrkCnt>=MAX_TRACK_NUM) RestDataType = 0;
			break;

		case _tkhd:
			if(Parent==_trak)
			{
				READ_STRUCT(&qtff->tkhd, TKHD, qtff->fid);
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _tref:
			if(Parent!=_trak)
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _edts:
			if(Parent!=_trak)
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _mdia:
			if(Parent!=_trak)
			{
				RestDataType = 0;
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		case _mdhd:
			if(Parent==_mdia)
			{
				MDHDV1 tmp;
				MDHD* tmp0 = (MDHD*)&tmp;
				int ver;

				READ_STRUCT(tmp0, MDHD, qtff->fid);
				ver = tmp0->ver_flag >> 24;
				if (ver == 0) {
					Trk->mdhd.Creation_time = (long long)tmp0->Creation_time;
					Trk->mdhd.Modification_time = (long long)tmp0->Modification_time;
					Trk->mdhd.Time_scale = tmp0->Time_scale;
					Trk->mdhd.Duration = (long long)tmp0->Duration;
					Trk->mdhd.Language = tmp0->Language;
					Trk->mdhd.Quality = tmp0->Quality;
				}
				else if (ver == 1){
					sub_len += sizeof(MDHD);
					SEEK(qtff->fid, -sizeof(MDHD), SEEK_CUR);
					READ_STRUCT(&Trk->mdhd, MDHDV1, qtff->fid);
				}
				else {
					DEBUG_ONLY(DBG_PRINT((char*)"<MDHD Version Err>"));
				}
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _hdlr:
			if(Parent==_mdia)
			{
				READ_STRUCT(&Trk->hdlr, HDLR, qtff->fid);
				// skip second audio or video track
				//if(Trk->hdlr.Component_subtype.v==_vide && qtff->vTrk) RestDataType = 0;
				//else if(Trk->hdlr.Component_subtype.v==_soun && qtff->aTrk) RestDataType = 0;
			}
			else if(Parent==_minf)
			{
				// skip rest data
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _minf:
			if(Parent!=_mdia)
			{
				RestDataType = 0;
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		case _vmhd:
			if(Parent==_minf)
			{
				READ_STRUCT(&qtff->vmhd, VMHD, qtff->fid);
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _smhd:
			if(Parent==_minf)
			{
				READ_STRUCT(&qtff->smhd, SMHD, qtff->fid);
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _dinf:
			if(Parent!=_minf)
			{
				RestDataType = 0;
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		case _stbl:
			if(Parent!=_minf)
			{
				RestDataType = 0;
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		case _stsd:
			RestDataType = 0;
			if(Parent==_stbl)
			{
				INT32U head_size, codec, offset, size;
				TRACK_INFO *Trk = &qtff->Track[qtff->TrkCnt];
				READ_STRUCT(&Trk->stsd, STSD, qtff->fid);

				offset = FTELL(qtff->fid);
				READ(qtff->fid, (INT32U)&head_size, 4);
				READ(qtff->fid, (INT32U)&codec, 4);
				size = (head_size >> 24) & 0xFF;
				size |= (head_size >> 8) & 0xFF00;
				if(codec == 0x31637661 && size < 1024) { //avc1
					INT8U *temp = (INT8U *)MALLOC(size);
					if(temp == 0) {
						break;
					}

					READ(qtff->fid, (INT32U)temp, size);
					vidH264VoVolParse(/*qtff,*/ Trk, temp);
					FREE((void *)temp);
				}
				SEEK(qtff->fid, offset, SEEK_SET);

				if(Trk->stsd.Number_of_entries==1)
				{
					Trk->stsz.SetVBR();
					if(Trk->hdlr.Component_subtype.v==_soun || Trk->hdlr.Component_manufacturer==_soun) {
						if(Trk->hdlr.Component_subtype.v != _soun) {
							Trk->hdlr.Component_subtype.v = _soun;
						}
						RestDataType = 2;
					} else if(Trk->hdlr.Component_subtype.v==_vide || Trk->hdlr.Component_manufacturer==_vide) {
						if(Trk->hdlr.Component_subtype.v != _vide) {
							Trk->hdlr.Component_subtype.v = _vide;
						}
						RestDataType = 3;
					} else {
						RestDataType = 0;
					}
				}
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		case _stts:
			if(Parent==_stbl)
			{
				READ_STRUCT(Trk->stts.GetHdr(), STTS, qtff->fid);
				Trk->stts.SetFileOffset(FTELL(qtff->fid));
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _stsc:
			if(Parent==_stbl)
			{
				READ_STRUCT(&Trk->stsc, STSC, qtff->fid);
				Trk->stsc.SetFileOffset(FTELL(qtff->fid));
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;
		case _stsz:
			if(Parent==_stbl)
			{
				READ_STRUCT(&Trk->stsz, STSZ, qtff->fid);
				Trk->stsz.SetFileOffset(FTELL(qtff->fid));
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;
		case _stco:
			if(Parent==_stbl)
			{
				READ_STRUCT(Trk->stco.GetHdr(), STCO, qtff->fid);
				Trk->stco.SetFileOffset(FTELL(qtff->fid));
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;
		case _stss:
			if(Parent==_stbl)
			{
				READ_STRUCT(Trk->stss.GetHdr(), STSS, qtff->fid);
				Trk->stss.SetFileOffset(FTELL(qtff->fid));
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _uuid:
			READ(qtff->fid, &t, 4);
			DEBUG_ONLY(DBG_PRINT((char*)"-%s", fourcc_to_str(t)));
			sub_len -= 4;
			RestDataType = 0;
			break;

		case _dref:
			if(Parent!=_dinf)
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _udta:
			if(Parent!=_moov)
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			RestDataType = 0;
			break;

		case _nmhd: // ?
		case _tapt: // ?
		case _iods: // ?
		case _ctts: // ?
		case _PICT: // to indicate a QuickDraw picture.
		case _free: // Unused space available in file.
		case _skip: // Unused space available in file.
		case _wide:	// Reserved space¡Xcan be overwritten by an extended size field if the following atom exceeds
					// 2^32 bytes, without displacing the contents of the following atom.
		case _pnot: // Reference to movie preview data.
			RestDataType = 0;
			break;

		/*
		case _rtng:
		case _hmhd:
		case _nmhd:
		case _meta:
		case _hnti:
		case _sdp :
		case _rtp :
			break; */

		default: // unknown
			DEBUG_ONLY(DBG_PRINT((char*)"-<?>"));
			RestDataType = 0;
			break;
		}
EndOfParseAtom:
		DEBUG_ONLY(DBG_PRINT((char*)NEW_LINE));

		// read sub atom
		if(RestDataType)
		{
			if(sub_len)
			{
				qtff->lev++;
				if(RestDataType==1)
					ret = QtffParser(qtff, sub_len, hdr.type.v);
				else if(RestDataType==2)
				{
					// reserve data space for WAVEFORMATEX
					// all extension data will be put after the WAVEFORMATEX immediately
					// NOTE WAVEFORMATEX is not WORD-alignment
					GP_PCMWAVEFORMAT *aFmt;
					Trk->ExData = qtff->ExData;
					aFmt = (GP_PCMWAVEFORMAT*)(Trk->ExData);
					aFmt->wFormatTag = 0;
					((GP_WAVEFORMATEX*)aFmt)->cbSize = 0;
					ret = Parse_AudParam(qtff, sub_len, hdr.type.v);

					// WAVEFORMATEX
					qtff->ExData2 = qtff->ExData + sizeof(GP_PCMWAVEFORMAT) + 2;
					qtff->ExDataLen2 -= sizeof(GP_PCMWAVEFORMAT) + 2;

					// extension data
					qtff->ExData2 += ((GP_WAVEFORMATEX*)aFmt)->cbSize;
					qtff->ExDataLen2 -= ((GP_WAVEFORMATEX*)aFmt)->cbSize;

					// WORD-alignment
					t = qtff->ExDataLen2 & 3;
					qtff->ExData2 += t;
					qtff->ExDataLen2 -= t;
				}
				else
				{
					// reserve data space for BITMAPINFOHEADER
					// all extension data will be put after the BITMAPINFOHEADER immediately
					GP_BITMAPINFOHEADER *vFmt;
					Trk->ExData = qtff->ExData;
					Trk->ExDataLen = sizeof(GP_BITMAPINFOHEADER);
					vFmt = (GP_BITMAPINFOHEADER*)(Trk->ExData);
					vFmt->biSize = sizeof(GP_BITMAPINFOHEADER);
					//	long biWidth;	// set in Stream Description
					//	long biHeight;	// set in Stream Description
					vFmt->biPlanes = 1;
					vFmt->biBitCount = 24;
					vFmt->biCompression = 0;
					vFmt->biSizeImage = 0;
					vFmt->biXPelsPerMeter = 0;
					vFmt->biYPelsPerMeter = 0;
					vFmt->biClrUsed = 0;
					vFmt->biClrImportant = 0;
					ret = Parse_VidParam(qtff, sub_len, hdr.type.v);

					// BITMAPINFOHEADER
					qtff->ExData2 = qtff->ExData + Trk->ExDataLen;
					qtff->ExDataLen2 -= Trk->ExDataLen;

					// WORD-alignment
					t = qtff->ExDataLen2 & 3;
					qtff->ExData2 += t;
					qtff->ExDataLen2 -= t;
				}
				if(ret < 0) goto Return;
				qtff->lev--;
				sub_len -= ret;
			}
			// check if new track
			// 1. TrkCnt < MAX_TRACK_NUM.
			// 2. track information must be completed
			// 3. new track's stts MUST has only one element. (MUST be single rate track)
			// 4. new track's stsd MUST has only one element. (MUST be single compress type track)
			// 5. new track is the first video or audio track
			if(hdr.type.v==_trak && qtff->TrkCnt < MAX_TRACK_NUM) // condition 1
			{
				TRACK_INFO *Trk = &qtff->Track[qtff->TrkCnt];
				int new_track_flag = 0;

				// condition 2
				if(Trk->stco.GetFileOffset()==0) goto EndOf_CheckNewTrack;
				if(Trk->stco.GetHdr()->Number_of_entries==0) goto EndOf_CheckNewTrack;
				if(Trk->stsc.GetFileOffset()==0) goto EndOf_CheckNewTrack;
				if(Trk->stsc.GetHdr()->Number_of_entries==0) goto EndOf_CheckNewTrack;
				if(Trk->stsz.GetFileOffset()==0) goto EndOf_CheckNewTrack;

				// condition 3
				if(Trk->stts.GetFileOffset()==0) goto EndOf_CheckNewTrack;

				// condition 4
				if(Trk->stsd.Number_of_entries!=1) goto EndOf_CheckNewTrack;

				// condition 5
				/*if(Trk->hdlr.Component_subtype.v == _vide && qtff->vTrk==0)
				{
					GP_BITMAPINFOHEADER *vFmt;
					vFmt = (GP_BITMAPINFOHEADER*)(Trk->ExData);
					if(vFmt->biCompression==0) goto EndOf_CheckNewTrack;

					qtff->vTrk = Trk;
					new_track_flag = 1;
				}
				else if(Trk->hdlr.Component_subtype.v == _soun && qtff->aTrk==0)
				{
					// condition 6
					GP_PCMWAVEFORMAT *aFmt;
					aFmt = (GP_PCMWAVEFORMAT*)(Trk->ExData);
					if(aFmt->wFormatTag==0) goto EndOf_CheckNewTrack;
					qtff->aTrk = Trk;
					new_track_flag = 1;
				}*/

				if (Trk->hdlr.Component_subtype.v == _vide && qtff->vTrkCnt < MAX_VID_TRK_NUM)
				{
					GP_BITMAPINFOHEADER *vFmt;
					vFmt = (GP_BITMAPINFOHEADER*)(Trk->ExData);
					if(vFmt->biCompression==0) goto EndOf_CheckNewTrack;

					if (!qtff->vTrk)
						qtff->vTrk = Trk;
					qtff->vTrkList[qtff->vTrkCnt++] = Trk;
					new_track_flag = 1;
				}
				else if(Trk->hdlr.Component_subtype.v == _soun && qtff->aTrkCnt < MAX_AUD_TRK_NUM)
				{
					// condition 6
					GP_PCMWAVEFORMAT *aFmt;
					aFmt = (GP_PCMWAVEFORMAT*)(Trk->ExData);
					if(aFmt->wFormatTag==0) goto EndOf_CheckNewTrack;
					if (!qtff->aTrk)
						qtff->aTrk = Trk;
					qtff->aTrkList[qtff->aTrkCnt++] = Trk;
					new_track_flag = 1;
				}

EndOf_CheckNewTrack:
				if(new_track_flag)
				{
					qtff->TrkCnt++;
					t = qtff->ExData2 - qtff->ExData;
					qtff->ExData = qtff->ExData2;
					qtff->ExDataLen = qtff->ExDataLen2;
				}
				else
				{
					// clear track infor if not new_track_flag
					// condition 2
					Trk->stco.SetFileOffset(0);
					Trk->stsc.SetFileOffset(0);
					Trk->stsz.SetFileOffset(0);
					Trk->stss.SetFileOffset(0);
					// condition 3
					Trk->stts.SetFileOffset(0);
					// condition 4
					Trk->stsd.Number_of_entries = 0;
					// condition 5
					Trk->hdlr.Component_subtype.v = 0;

					// reset data
					qtff->ExData2 = qtff->ExData;
					qtff->ExDataLen2 = qtff->ExDataLen;
				}
			}
		}
		if(sub_len)
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s<Skip=%u>" NEW_LINE, indent(qtff->lev), sub_len));
			SEEK(qtff->fid, sub_len, SEEK_CUR);
			sub_len = 0;
		}

		main_len -= hdr.len;
	}

	ret = 0;
Return:
	if(ret<0)
		return ret;
	else return Length;
}

///////////////////////////////////////////////////////////////////////////////
//
// Track level functions
//
///////////////////////////////////////////////////////////////////////////////
int Track_Init(TRACK_INFO *Trk, FILEHANDLE fid, long FileBaseOffset, long *SyncBuf)
{
	int ret;

	Trk->flag = 0;

	// initial stts
	ret = Trk->stts.Init(fid);
	if(ret<0) RETURN(ret);
	// initial stsc
	ret = Trk->stsc.Init(fid, Trk->stco.GetHdr()->Number_of_entries);
	if(ret<0) RETURN(ret);
	// initial stco
	ret = Trk->stco.Init(fid, FileBaseOffset);
	if(ret<0) RETURN(ret);
	// initial stsz
	ret = Trk->stsz.Init(fid);
	if(ret<0) RETURN(ret);
	// initial stss
	ret = Trk->stss.Init(fid, SyncBuf, Trk->stsz.GetHdr()->Number_of_entries);
	if(ret<0) RETURN(ret);

	ret = 0;
Return:
	return ret;
}

int Track_InitResumePlay(TRACK_INFO *Trk, FILEHANDLE fid, long *SyncBuf)
{
	int ret;

	// initial stts
	ret = Trk->stts.InitResumePlay(fid);
	if(ret<0) RETURN(ret);
	// initial stsc
	ret = Trk->stsc.InitResumePlay(fid);
	if(ret<0) RETURN(ret);
	// initial stco
	ret = Trk->stco.InitResumePlay(fid);
	if(ret<0) RETURN(ret);
	// initial stsz
	ret = Trk->stsz.InitResumePlay(fid);
	if(ret<0) RETURN(ret);
	// initial stss
	ret = Trk->stss.InitResumePlay(fid, SyncBuf);
	if(ret<0) RETURN(ret);

	ret = 0;
Return:
	return ret;
}

int Track_GetSampleInfo(void *hTrk, long tarSample, MEDIA_PARSER_SAMPLE_INFO *pSample)
{
	int ret;
	TRACK_INFO *Trk = (TRACK_INFO *)hTrk;
	long t32, tarTime;

	// limit Sample Range
	if(tarSample<0)
	{
		pSample->IsKey = 0;
		pSample->Size = 0;
		pSample->Offset = 0;
		pSample->Count = 0;
		pSample->dTick = 0;
		pSample->Tick64 = 0;
		RETURN(0);
	}
	if(tarSample>=Trk->stsz.GetHdr()->Number_of_entries)
	{
		tarSample = Trk->stsz.GetHdr()->Number_of_entries;
		tarTime = Trk->stts.Sample2Time(tarSample);
		if(tarTime<0) RETURN(tarTime);

		pSample->IsKey = 0;
		pSample->Size = 0;
		pSample->Offset = 0;
		pSample->Count = 0;
		pSample->dTick = 0;
		pSample->Tick64 = tarTime;
		RETURN(0);
	}

	// Update stsc
	ret = Trk->stsc.Update(tarSample);
	if(ret<0) RETURN(ret);
	if(!ret)
	{
		tarSample = Trk->stsz.GetHdr()->Number_of_entries;
		tarTime = Trk->stts.Sample2Time(tarSample);
		if(tarTime<0) RETURN(tarTime);

		pSample->IsKey = 0;
		pSample->Size = 0;
		pSample->Offset = 0;
		pSample->Count = 0;
		pSample->dTick = 0;
		pSample->Tick64 = tarTime;
		RETURN(0);
	}

	// Update stco
	ret = Trk->stco.Update(Trk->stsc.GetCurChunk());
	if(ret<0) RETURN(ret);

	// Update stsz
	ret = Trk->stsz.Update(Trk->stsc.GetStartSample(), tarSample - Trk->stsc.GetStartSample());
	if(ret<0) RETURN(ret);

	// Update stss
	ret = Trk->stss.Update(tarSample);
	if(ret<0) RETURN(ret);

	pSample->IsKey = Trk->stss.IsKeyFrame() ? 1 : 0;

	// Sample offset and size
	pSample->Offset = Trk->stco.GetChunkOffset() + Trk->stsz.GetSampleOffset();
	if(Trk->hdlr.Component_subtype.v==_vide || Trk->fmt.v == MAKEFOURCC('m','p','4','a'))
	{
		pSample->Size = Trk->stsz.GetSize();
		pSample->Count = 1;
	}
	else
	{
		t32 = Trk->stsz.GetSampleOffset();
		pSample->Count = Trk->stsc.GetStartSample() + Trk->stsc.GetSamplesPerChunk() - tarSample;
		ret = Trk->stsz.Update(Trk->stsc.GetStartSample(), Trk->stsc.GetSamplesPerChunk());
		if(ret<0) RETURN(ret);
		pSample->Size = Trk->stsz.GetSampleOffset() - t32;

		//ytliao 2017.12 following code is disabled, offset align is modify in ParseAud.cpp Parse_AudParam()
		/*if (Trk->hdlr.Component_subtype.v==_soun) {
			// pcm format
			if ((Trk->fmt.v == MAKEFOURCC('t','w','o','s')) || (Trk->fmt.v == MAKEFOURCC('s','o','w','t'))) {
				//pSample->Size <<= 1;
				pSample->Size *= Trk->stsz.GetSize(); //ytliao 2016.4 fix stereo pcm playback
			}
		}*/
	}

	// Update stts
	if(tarSample==Trk->stts.GetCurSample())
	{
		tarTime = Trk->stts.GetCurTime();
		t32 = Trk->stts.Sample2Time(tarSample + pSample->Count);
		if(t32<0) RETURN(t32);
	}
	else if(tarSample+pSample->Count==Trk->stts.GetCurSample())
	{
		t32 = Trk->stts.GetCurTime();
		tarTime = Trk->stts.Sample2Time(tarSample);
		if(tarTime<0) RETURN(tarTime);
	}
	else
	{
		tarTime = Trk->stts.Sample2Time(tarSample);
		if(tarTime<0) RETURN(tarTime);
		t32 = Trk->stts.Sample2Time(tarSample + pSample->Count);
		if(t32<0) RETURN(t32);
	}
	pSample->Tick64 = tarTime;
	pSample->dTick = t32 - tarTime;
	//print_string("dTick=%d t32=%d tarTime=%d count=%d\r\n", pSample->dTick, t32, tarTime, tarSample + pSample->Count);

	ret = 1;
Return:
	return ret;
}

long Track_Tick2Sample(void *hTrk, __int64 Tick64)
{
	TRACK_INFO *Trk = (TRACK_INFO *)hTrk;
	return Trk->stts.Time2Sample((long)Tick64);
}

long Track_GetFormatFourCC(const void *hTrk)
{
	TRACK_INFO *Trk = (TRACK_INFO *)hTrk;
	return Trk->fmt.v;
}

int Track_GetKeyFrame(void *hTrk, long tarSample)
{
	TRACK_INFO *Trk = (TRACK_INFO *)hTrk;
	return Trk->stss.GetKeyFrame(tarSample);
}

// Track Information
long Track_GetTotalSamples(const void *hTrk)
{
	const TRACK_INFO *Trk = (const TRACK_INFO *)hTrk;
	return Trk->stsz.GetHdr()->Number_of_entries;
}

int Track_GetFrameRate(const void *hTrck)
{
	int rate = 0;
	const TRACK_INFO *Trk = (const TRACK_INFO *)hTrck;
	if(Trk->mdhd.Duration) {
		rate = (Track_GetTotalSamples(hTrck)*1000)/(Trk->mdhd.Duration);
	}
	else {
		rate = Track_GetTotalSamples(hTrck);
	}
	return rate;
}

long Track_GetTickRate(const void *hTrk)
{
	const TRACK_INFO *Trk = (const TRACK_INFO *)hTrk;
	return Trk->mdhd.Time_scale;
}

long Track_GetTotalSec(const void *hTrk)
{
	const TRACK_INFO *Trk = (const TRACK_INFO *)hTrk;
	return (long)QtffParser_udiv64by32(Trk->mdhd.Time_scale, ((unsigned __int64)Trk->mdhd.Duration) << 8);
}

const char *Track_GetExData(const void *hTrk, long *Len)
{
	const TRACK_INFO *Trk = (const TRACK_INFO *)hTrk;

	if(Len) *Len = Trk->ExDataLen;
	return Trk->ExData;
}

char *QtffParser_GetavcCData(void *hQtff, long *Len)
{
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;

	*Len = qtff->vTrk->vidHeaderSize;
	return qtff->vTrk->vidHeaderData;
}

#if __OPTIMISE_LEVEL<2
TRACK_INFO *g_aTrk = 0;
TRACK_INFO *g_vTrk = 0;
QTFFPARSER *g_qtff = 0;
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Qtff level functions
//
///////////////////////////////////////////////////////////////////////////////
#ifndef SEEK64_EN
int QtffParser_Init(void *hQtff, char *ExData, int ExDataLen, FILEHANDLE fid, long FileSize)
#else
int QtffParser_Init(void *hQtff, char *ExData, int ExDataLen, FILEHANDLE fid, long long FileSize)
#endif
{
#ifndef SEEK64_EN
	int ret;
#else
	long long ret;
#endif
	int FileBaseOffset;
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;

	DEBUG_ONLY(g_qtff = qtff;);

	// when FileSize = 0, into resume play init
	if(FileSize == 0 ) {
		unsigned int i, temp;
		TRACK_INFO *pTrack;

		for(i=0; i<2; i++) {
			pTrack = qtff->Track + i;
			switch(pTrack->fmt.v)
			{
			case MAKEFOURCC('m','p','4','a'):
				DEBUG_ONLY(DBG_PRINT((char*)"mp4a" NEW_LINE));
				qtff->aTrk = qtff->Track + i;
				break;

			case MAKEFOURCC('m','p','4','v'):
				DEBUG_ONLY(DBG_PRINT((char*)"mp4v" NEW_LINE));
				qtff->vTrk = qtff->Track + i;
				break;

			default:
				RETURN(MEDIAPARSER_PARAM_ERR);
			}
		}

		temp = qtff->vTrk->ExDataLen;
		if(temp & 0x03) {
			temp = temp >> 2 << 2;
			temp += 0x04;
		}

		temp += qtff->aTrk->ExDataLen;
		if(temp & 0x03) {
			temp = temp >> 2 << 2;
			temp += 0x04;
		}

		qtff->fid = fid;
		qtff->ExData2 = qtff->ExData = ExData + temp;

		if(qtff->aTrk && qtff->vTrk) {
			if(qtff->aTrk->ExData < qtff->vTrk->ExData) {
				temp = (long)qtff->vTrk->ExData - (long)qtff->aTrk->ExData;
				qtff->aTrk->ExData = ExData;
				qtff->vTrk->ExData = ExData + temp;
			} else {
				temp = (long)qtff->aTrk->ExData - (long)qtff->vTrk->ExData;
				qtff->vTrk->ExData = ExData;
				qtff->aTrk->ExData = ExData + temp;
			}
		} else if(qtff->aTrk) {
			qtff->aTrk->ExData = qtff->ExData;
		} else if(qtff->vTrk) {
			qtff->vTrk->ExData = qtff->ExData;
		}

		if(qtff->vTrk) {
			ret = Track_InitResumePlay(qtff->vTrk, qtff->fid, qtff->vTrk->SyncBuf);
			if(ret<0)
				RETURN(ret);
		}

		if(qtff->aTrk) {
			ret = Track_InitResumePlay(qtff->aTrk, qtff->fid, 0/*qtff->SyncBuf*/);
			if(ret<0)
				RETURN(ret);
		}
		RETURN(ret);
	}

	MEMSET(qtff, 0, sizeof(QTFFPARSER));

	qtff->fid = fid;

	qtff->ExData2 = qtff->ExData = ExData;
	qtff->ExDataLen2 = qtff->ExDataLen = ExDataLen;

	FileBaseOffset = FTELL(qtff->fid);
	if(FileSize<=0)
		FileSize = FILELENGTH(qtff->fid) - FileBaseOffset;

	ret = QtffParser(qtff, FileSize, 0);
	if(ret<0) RETURN(ret);

	DEBUG_ONLY(g_vTrk = qtff->vTrk;);

	if((qtff->flag & (FLAG_MDAT_ATOM_EXIST | FLAG_MOOV_ATOM_EXIST)) != (FLAG_MDAT_ATOM_EXIST | FLAG_MOOV_ATOM_EXIST))
		RETURN(MEDIAPARSER_FORMAT_ERR);

	if(qtff->vTrkCnt)
	{
		int i;
		for (i = 0; i < qtff->vTrkCnt; i++)
		{
			ret = Track_Init(qtff->vTrkList[i], qtff->fid, FileBaseOffset, qtff->vTrkList[i]->SyncBuf);
			if(ret<0)
				RETURN(ret);
		}
		/*ret = Track_Init(qtff->vTrk, qtff->fid, FileBaseOffset, qtff->SyncBuf);
		if(ret<0)
			RETURN(ret);*/
	}

	DEBUG_ONLY(g_aTrk = qtff->aTrk;);

	if(qtff->aTrk)
	{
		ret = Track_Init(qtff->aTrk, qtff->fid, FileBaseOffset, 0);
		if(ret<0)
			RETURN(ret);
	}

	ret = 0;
Return:
	return ret;
}

int QtffParser_GetWorkMemSize()
{
	return sizeof(QTFFPARSER);
}

void *QtffParser_SelectAudTrack(void *hQtff, int trk_no)
{
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;

	if (trk_no >= qtff->aTrkCnt || trk_no < 0)
		return NULL;

	qtff->aTrk = qtff->aTrkList[trk_no];

	return qtff->aTrk;
}

void *QtffParser_SelectVidTrack(void *hQtff, int trk_no)
{
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;

	if (trk_no >= qtff->vTrkCnt || trk_no < 0)
		return NULL;

	qtff->vTrk = qtff->vTrkList[trk_no];

	return qtff->vTrk;
}

int QtffParser_GetAudTrkCnt(void *hQtff)
{
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;

	return qtff->aTrkCnt;
}

int QtffParser_GetVidTrkCnt(void *hQtff)
{
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;

	return qtff->vTrkCnt;
}

void *QtffParser_GetAudHandle(void *hQtff)
{
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;
	return qtff->aTrk;
}

void *QtffParser_GetVidHandle(void *hQtff)
{
	QTFFPARSER *qtff = (QTFFPARSER *)hQtff;
	return qtff->vTrk;
}

const char *QtffParser_GetVersion(void)
{
	return "GP$214 Generalplus QTFF Parser 2014.04.15";
}

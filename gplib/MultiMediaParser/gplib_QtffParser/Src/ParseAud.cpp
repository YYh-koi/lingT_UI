#include "QtffParser.h"
#include "QtffParser_comm.h"

extern "C" unsigned long QtffParser_udiv32by32(unsigned long, unsigned long);

static int descr_len(const unsigned char **ptr0)
{
	int i;
	int L = 0;
	const unsigned char *ptr = *ptr0;
	unsigned char a;
	a = *ptr++;
	L = a & 0x7F;
	for(i=0;i<4 && (a&0x80); i++)
	{
		a = *ptr++;
		L = (L << 7) | (a & 0x7F);
	}
	*ptr0 = ptr;
	return L;
}

// defines if an object type can be decoded by this library or not
/* static uint8_t ObjectTypesTable[32] = {
	0, //  0 NULL
	0, //  1 AAC Main
	1, //  2 AAC LC
	0, //  3 AAC SSR
	0, //  4 AAC LTP
	0, //  5 SBR
	0, //  6 AAC Scalable
	0, //  7 TwinVQ
	0, //  8 CELP
	0, //  9 HVXC
	0, // 10 Reserved
	0, // 11 Reserved
	0, // 12 TTSI
	0, // 13 Main synthetic
	0, // 14 Wavetable synthesis
	0, // 15 General MIDI
	0, // 16 Algorithmic Synthesis and Audio FX
    // MPEG-4 Version 2
// No ER defined
    0, // 17 ER AAC LC
    0, // 18 (Reserved)
    0, // 19 ER AAC LTP
    0, // 20 ER AAC scalable
    0, // 21 ER TwinVQ
    0, // 22 ER BSAC
    0, // 23 ER AAC LD
    0, // 24 ER CELP
    0, // 25 ER HVXC
    0, // 26 ER HILN
    0, // 27 ER Parametric
    0, // 28 (Reserved)
    0, // 29 (Reserved)
    0, // 30 (Reserved)
    0  // 31 (Reserved)
};
*/

static const unsigned long AAC_sample_rates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};

static int Aud_ES_Descriptor(GP_PCMWAVEFORMAT *aFmt, const unsigned char *ptr)
{
	int ret;
	unsigned char tag, audioTypeIdicaton, objectTypeIndex, sample_rate_index;
	unsigned long /*maxBitRate, */avgBitRate, L;
	int channel;

	tag = *ptr++;
	if(tag==0x03)
	{
		L = descr_len(&ptr);
		ptr += 3;
		if(L< 5 + 15) RETURN(0);
	}
	else
		ptr += 2;

	tag = *ptr++;
	if(tag != 0x04) RETURN(0);
	L = descr_len(&ptr);
	if(L < 13) RETURN(0);

	audioTypeIdicaton = *ptr++;
	//	0x40 	Audio ISO/IEC 14496-3
	//	0x66 	Audio ISO/IEC 13818-7 Main Profile
	//	0x67 	Audio ISO/IEC 13818-7 LowComplexity Profile
	//	0x68 	Audio ISO/IEC 13818-7 Scaleable Sampling Rate Profile
	//	0x69 	Audio ISO/IEC 13818-3
	//	0x6B 	Audio ISO/IEC 11172-3
	//	0x6D - 0xBF 	reserved for ISO use
	//	0xC0 - 0xFE	user private

	ptr += 4;

	//maxBitRate = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
	ptr += 4;

	avgBitRate = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
	ptr += 4;

	aFmt->nAvgBytesPerSec = (avgBitRate +4) >> 3;


	tag = *ptr++;
	if(tag!=0x05) RETURN(0);

	L = descr_len(&ptr);

	// 5 bit
	objectTypeIndex = *ptr >> 3;

	if(audioTypeIdicaton!=0x40 || // AAC
		objectTypeIndex!=2) // LC
		aFmt->wFormatTag = 0;

	// 4 bit
	sample_rate_index = ((ptr[0] & 0x7) << 1) | (ptr[1] >> 7);
	ptr++;

	if(sample_rate_index==0x0F)
	{
		aFmt->nSamplesPerSec =
			(((unsigned long)ptr[0] & 0x7F) << 17) |
			 ((unsigned long)ptr[1]         <<  9) |
			 ((unsigned long)ptr[2]         <<  1) |
			 ((unsigned long)ptr[3]         >>  7);
		ptr += 3;
	}
	else if(sample_rate_index<12)
	{
		aFmt->nSamplesPerSec = AAC_sample_rates[sample_rate_index];
	}

	channel = (*ptr >> 3) & 0xF;
	if(channel<=7) aFmt->nChannels = channel;

	ret = 0;
Return:
	return ret;
}

#ifndef SEEK64_EN
static int Parse_siDecompressionParam(QTFFPARSER *qtff, int Length, long Parent)
#else
static long long Parse_siDecompressionParam(QTFFPARSER *qtff, long long Length, long Parent)
#endif
{
	ATOM_HDR hdr;
#ifndef SEEK64_EN
	int ret;
	long main_len, sub_len;
#else
	long long ret;
	long long main_len, sub_len;
#endif
	int RestDataType = 0;
	TRACK_INFO *Trk = &qtff->Track[qtff->TrkCnt];

	main_len = Length;
	while(main_len>0)
	{
		// Read header
		if(main_len < (long)sizeof(ATOM_HDR))
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s(<SizeErr> %d, %XH)", indent(qtff->lev), main_len, main_len));
			hdr.len = main_len;
			sub_len = main_len;
			RestDataType = 0;
			goto EndOfParseAtom;
		}

		READ(qtff->fid, &hdr, sizeof(ATOM_HDR));
		hdr.len = BYTE_REVERSE4(hdr.len);

		if(hdr.len<(signed)sizeof(ATOM_HDR))
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %d, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
			hdr.len = main_len;
			sub_len = main_len;
			RestDataType = 0;
			goto EndOfParseAtom;
		}
		else if(hdr.len>(unsigned long)main_len)
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %d, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
			hdr.len = main_len;
		}
		else
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(%d, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
		}

		sub_len = hdr.len;
		sub_len -= 8;
		switch(hdr.type.v)
		{
		case _wave:
			RestDataType = 1;
			break;
		case _frma:
			READ(qtff->fid, &qtff->Track[qtff->TrkCnt].fmt, sizeof(int));
			sub_len -= 4;
			break;

		case 0:
			// End of siDecompressionParam
			break;

		case _raw:
		case _NONE:
		case _twos:
		case _sowt:
		case _MAC3:
		case _MAC6:
		case _ima4:
		case _fl32:
		case _fl64:
		case _in24:
		case _ulaw:
		case _alaw:
		case 0x0200736D: // 0x6D730002
		case 0x1100736D: // 0x6D730011 'ms\x00\x11'
		case _dvca:
		case _QMDC:
		case _Qclp:
		case 0x5500736D: // ms\0x0055 MS MP3
		case _mp3:
		// case _mp4a: mp4a do not need PCMWAVEFORMAT
			if(Parent==_wave)
			{
				// Read WAVEFORMATEX to aFmt
				if(sub_len > qtff->ExDataLen) RETURN(MEDIAPARSER_EXDATA_OVERFLOW);
				Trk->fmt.v = hdr.type.v;
				READ(qtff->fid, Trk->ExData, sub_len);
				Trk->ExDataLen = sub_len;
				sub_len = 0;
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		case _esds: // used only for mp4a
			if((Parent==_mp4a) || (Parent==_wave && Trk->fmt.v==_mp4a))
			{
				GP_PCMWAVEFORMAT *aFmt = (GP_PCMWAVEFORMAT*)Trk->ExData;
				char *ptr = Trk->ExData + sizeof(GP_PCMWAVEFORMAT) + 2;
				READ_STRUCT(&Trk->esds, ESDS, qtff->fid);
				if(sub_len > qtff->ExDataLen - (signed)sizeof(GP_PCMWAVEFORMAT) - 2) RETURN(MEDIAPARSER_EXDATA_OVERFLOW);
				READ(qtff->fid, ptr, sub_len);
				((GP_WAVEFORMATEX *)aFmt)->cbSize = (short)sub_len;
				Aud_ES_Descriptor(aFmt, (const unsigned char*)ptr);

				Trk->ExDataLen = sizeof(GP_PCMWAVEFORMAT) + 2 + sub_len;
				sub_len = 0;
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		default:
			DEBUG_ONLY(DBG_PRINT((char*)"-<?>"));
			break;
		}
EndOfParseAtom:
		DEBUG_ONLY(DBG_PRINT((char*)NEW_LINE));

		if(RestDataType)
		{
			if(sub_len)
			{
				qtff->lev++;
				ret = Parse_siDecompressionParam(qtff, sub_len, hdr.type.v);
				if(ret<0) RETURN(ret);
				sub_len -= ret;
				qtff->lev--;
			}
		}
		if(sub_len)
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s<Skip=%d>" NEW_LINE, indent(qtff->lev), sub_len));
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

#ifndef SEEK64_EN
int Parse_AudParam(QTFFPARSER *qtff, int Length, long Parent)
#else
long long Parse_AudParam(QTFFPARSER *qtff, long long Length, long Parent)
#endif
{
	ATOM_HDR hdr;
#ifndef SEEK64_EN
	int ret;
	long main_len, sub_len;
#else
	long long ret;
	long long main_len, sub_len;
#endif
	int RestDataType;
	TRACK_INFO *Trk = &qtff->Track[qtff->TrkCnt];

	main_len = Length;
	while(main_len>0)
	{
		// Read header
		if(main_len < (long)sizeof(ATOM_HDR))
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s(<SizeErr> %d, %XH)", indent(qtff->lev), main_len, main_len));
			hdr.len = main_len;
			sub_len = main_len;
			RestDataType = 0;
			goto EndOfParseAtom;
		}

		READ(qtff->fid, &hdr, sizeof(ATOM_HDR));
		hdr.len = BYTE_REVERSE4(hdr.len);

		if(hdr.len<(signed)sizeof(ATOM_HDR))
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %d, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
			hdr.len = main_len;
			sub_len = main_len;
			RestDataType = 0;
			goto EndOfParseAtom;
		}
		else if(hdr.len>(unsigned long)main_len)
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(SizeErr %d, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
			hdr.len = main_len;
		}
		else
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s%s(%d, %XH)", indent(qtff->lev), fourcc_to_str(hdr.type.v), hdr.len, hdr.len));
		}

		sub_len = hdr.len;
		RestDataType = 0;
		sub_len -= 8;

		switch(hdr.type.v)
		{
		case 0:
		case _raw:
		case _NONE:
		case _twos:			// 16 bit big-endian
		case _sowt:			// 0x0001 WAVE_FORMAT_PCM (x86 used)
		// case _MAC3:
		// case _MAC6:
		case _ima4:
		// case _fl32:
		// case _fl64:
		// case _in24:
		case _ulaw:			// 0x0007 WAVE_FORMAT_MULAW
		case _alaw:			// 0x0006 WAVE_FORMAT_ALAW
		case 0x0200736D:	// 0x0002 WAVE_FORMAT_ADPCM
		case 0x1100736D:	// 0x0011 WAVE_FORMAT_DVI_ADPCM
		// case _dvca:
		// case _QMDC:
		// case _Qclp:
		case 0x5500736D:	// 0x0055 WAVE_FORMAT_MPEGLAYER3
		case _mp3:			// 0x0055 WAVE_FORMAT_MPEGLAYER3
		case _mp4a:			// 0xA106 WAVE_FORMAT_AAC         ISO/MPEG-4 AAC
			if(Parent==_stsd)
			{
				int FrameSize;
				GP_PCMWAVEFORMAT *aFmt = (GP_PCMWAVEFORMAT*)(qtff->ExData);
				Trk->fmt.v = hdr.type.v;
				READ_STRUCT(&Trk->stsd_general, STSD_GENERAL, qtff->fid);
				READ_STRUCT(&qtff->AudFmtV0, STSD_AUD_V0, qtff->fid);
				aFmt->nChannels			= qtff->AudFmtV0.Number_of_channels;
				aFmt->nSamplesPerSec	= (qtff->AudFmtV0.Sample_rate + 32767) >> 16;
				aFmt->nAvgBytesPerSec	= 0;
				aFmt->nBlockAlign		= 1;
				aFmt->wBitsPerSample	= qtff->AudFmtV0.Sample_size;
				FrameSize				= 1;
				switch(Trk->fmt.v) // only for uncompressed audio
				{
				case 0:
				case _raw:
				case _twos: // TODO do swap if twos
				case _sowt:
					aFmt->wFormatTag		= WAVE_FORMAT_PCM;
					aFmt->nBlockAlign		= aFmt->wBitsPerSample * aFmt->nChannels >> 3;
					aFmt->nAvgBytesPerSec	= aFmt->nSamplesPerSec * aFmt->nBlockAlign;
					break;
				case _ulaw:
					aFmt->wFormatTag		= WAVE_FORMAT_MULAW;
					aFmt->nBlockAlign		= aFmt->nChannels;
					aFmt->nAvgBytesPerSec	= aFmt->nSamplesPerSec * aFmt->nBlockAlign;
					break;
				case _alaw:
					aFmt->wFormatTag		= WAVE_FORMAT_ALAW;
					aFmt->nBlockAlign		= aFmt->nChannels;
					aFmt->nAvgBytesPerSec	= aFmt->nSamplesPerSec * aFmt->nBlockAlign;
					break;

				case 0x0200736D:
					aFmt->wFormatTag		= WAVE_FORMAT_ADPCM;
					break;
				case _ima4:
				case 0x1100736D:
					aFmt->wFormatTag		= WAVE_FORMAT_DVI_ADPCM;
					break;
				case 0x5500736D:
				case _mp3:
					aFmt->wFormatTag		= WAVE_FORMAT_MPEGLAYER3;
					break;
				case _mp4a:
					// 0xA106 WAVE_FORMAT_AAC       ISO/MPEG-4 AAC
					// 0x00FF WAVE_FORMAT_RAW_AAC1	Raw AAC.
					aFmt->wFormatTag		= WAVE_FORMAT_RAW_AAC1;
					break;
				}
				if(qtff->AudFmtV0.Version==1)
				{
					READ_STRUCT(&qtff->AudFmtV1, STSD_AUD_V1, qtff->fid);
					aFmt->nAvgBytesPerSec	= // (qtff->AudFmtV1.Bytes_per_frame * aFmt->nSamplesPerSec) / qtff->AudFmtV1.Samples_per_packet;
						QtffParser_udiv32by32(qtff->AudFmtV1.Samples_per_packet, qtff->AudFmtV1.Bytes_per_frame * aFmt->nSamplesPerSec);
					aFmt->nBlockAlign		= (short)qtff->AudFmtV1.Bytes_per_frame;
					FrameSize			= qtff->AudFmtV1.Samples_per_packet;
				}

				if(qtff->AudFmtV0.Compression_ID==-2) {
					Trk->stsz.SetVBR();
				} else {
					//mark by ytliao 2022.09
					/*if(Trk->stsc.GetHdr()->Number_of_entries<=1) {
						Trk->stsz.SetCBR(FrameSize, 1);
					} else {
						Trk->stsz.SetCBR(FrameSize, aFmt->nBlockAlign);
					}*/
					/* add by ytliao 2022.09, if stsz has sample size, means all samples has same size.
					   so call SetCBR(), make GetSampleOffset() return offset*sample_size.
					   if no sample size, call SetVBR to disable CBR,
					   and make GetSampleOffset() return offset(in bytes) as it is.
					   this fix pcm stereo and mono playback issue.*/
					if(Trk->stsz.GetHdr()->Sample_size) {
						Trk->stsz.SetCBR(FrameSize, Trk->stsz.GetHdr()->Sample_size);
					} else {
						Trk->stsz.SetVBR();
					}
				}
				RestDataType = 2;
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;

		default:
			DEBUG_ONLY(DBG_PRINT((char*)"-<?>" NEW_LINE));
			break;
		}
EndOfParseAtom:
		DEBUG_ONLY(DBG_PRINT((char*)NEW_LINE));

		if(RestDataType)
		{
			if(sub_len)
			{
				qtff->lev++;
				if(RestDataType==1)
					ret = Parse_AudParam(qtff, sub_len, hdr.type.v);
				else
					ret = Parse_siDecompressionParam(qtff, sub_len, hdr.type.v);
				if(ret<0) RETURN(ret);
				sub_len -= ret;
				qtff->lev--;
			}
		}
		if(sub_len)
		{
			DEBUG_ONLY(DBG_PRINT((char*)"%s<Skip=%d>" NEW_LINE, indent(qtff->lev), sub_len));
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

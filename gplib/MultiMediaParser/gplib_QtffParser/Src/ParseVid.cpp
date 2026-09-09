#include "QtffParser.h"
#include "QtffParser_comm.h"

#ifndef SEEK64_EN
int Parse_VidParam(QTFFPARSER *qtff, int Length, long Parent)
#else
long long Parse_VidParam(QTFFPARSER *qtff, long long Length, long Parent)
#endif
{
	ATOM_HDR hdr;
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
			DEBUG_ONLY(DBG_PRINT((char*)"%s(<SizeErr> %d, %XH)", indent(qtff->lev), main_len, main_len));
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
		// case _raw:
		// case _cvid:
		case _jpeg:
		// case _smc :
		// case _rle :
		// case _rpza:
		// case _kpcd:
		// case _png :
		case _mjpa:
		case _mjpb:
		// case _SVQ1:
		// case _SVQ3:
		case _mp4v:
		// case _dvc :
		// case _dvcp:
		// case _gif :
		case _h263:
		case _s263:
		// case _tiff:
		// case _2vuy:
		// case _yuv2:
		// case _v308:
		// case _v408:
		// case _v216:
		// case _v410:
		// case _v210:
		case _avc1:
			RestDataType = 0;
			if(Parent==_stsd)
			{
				GP_BITMAPINFOHEADER *vFmt = (GP_BITMAPINFOHEADER*)Trk->ExData;

				Trk->fmt.v = hdr.type.v;
				READ_STRUCT(&Trk->stsd_general, STSD_GENERAL, qtff->fid);

				READ_STRUCT(&qtff->VidFmt, STSD_VID, qtff->fid);
				qtff->VidFmt.Frame_count = ByteSwap16(qtff->VidFmt.Frame_count);
				qtff->VidFmt.Depth = ByteSwap16(qtff->VidFmt.Depth);
				qtff->VidFmt.Color_table_ID = ByteSwap16(qtff->VidFmt.Color_table_ID);

				Trk->ExDataLen = sizeof(GP_BITMAPINFOHEADER); // clear extension data size
				vFmt->biWidth = qtff->VidFmt.Width;
				vFmt->biHeight = qtff->VidFmt.Height;
				switch(hdr.type.v)
				{
				case _jpeg: vFmt->biCompression = MAKEFOURCC('M','J','P','G') ; break;
				case _mjpa: vFmt->biCompression = MAKEFOURCC('M','J','P','G') ; break;
				case _mjpb: vFmt->biCompression = MAKEFOURCC('M','J','P','G') ; break;
				case _mp4v: vFmt->biCompression = MAKEFOURCC('M','4','S','2') ; break;
				case _avc1: vFmt->biCompression = MAKEFOURCC('H','2','6','4') ; break;
				case _s263:
				case _h263: vFmt->biCompression = MAKEFOURCC('H','2','6','3') ; break;
				}

				if(qtff->VidFmt.Color_table_ID!=-1) RestDataType = 0;
				else RestDataType = 1;
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
			}
			break;
		case _esds: // used only for mp4v
			if(Parent==_mp4v)
			{
				READ_STRUCT(&Trk->esds, ESDS, qtff->fid);
				Trk->ExDataLen = sizeof(GP_BITMAPINFOHEADER) + sub_len;
				if(Trk->ExDataLen > qtff->ExDataLen2) RETURN(MEDIAPARSER_EXDATA_OVERFLOW);
				READ(qtff->fid, Trk->ExData + sizeof(GP_BITMAPINFOHEADER), sub_len);
				sub_len = 0;
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
				RestDataType = 0;
			}
			break;

		case _d263: // used only for h263
			if(Parent==_h263||Parent==_s263)
			{
				Trk->ExDataLen = sizeof(GP_BITMAPINFOHEADER) + sub_len;
				if(Trk->ExDataLen > qtff->ExDataLen2) RETURN(MEDIAPARSER_EXDATA_OVERFLOW);
				READ(qtff->fid, Trk->ExData + sizeof(GP_BITMAPINFOHEADER), sub_len);
				sub_len = 0;
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
				RestDataType = 0;
			}
			break;

		case _avcC: // used only for avc1
			if(Parent==_avc1)
			{
				// TODO: I guess that avcC has a header like esds, but not sure. Homer Lu 2009.10.12
				READ_STRUCT(&Trk->esds, ESDS, qtff->fid);
				Trk->ExDataLen = sizeof(GP_BITMAPINFOHEADER) + sub_len;
				if(Trk->ExDataLen > qtff->ExDataLen2) RETURN(MEDIAPARSER_EXDATA_OVERFLOW);
				READ(qtff->fid, Trk->ExData + sizeof(GP_BITMAPINFOHEADER), sub_len);
				sub_len = 0;
			}
			else
			{
				DEBUG_ONLY(DBG_PRINT((char*)"<ParrentErr>"));
				RestDataType = 0;
			}
			break;
		case _btrt: // ?
			RestDataType = 0;
			break;

		default:
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
				ret = Parse_VidParam(qtff, sub_len, hdr.type.v);
				if(ret < 0) goto Return;
				qtff->lev--;
				sub_len -= ret;
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

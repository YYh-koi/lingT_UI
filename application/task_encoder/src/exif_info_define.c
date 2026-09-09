/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "exif_info_define.h"

#if (defined C_PHOTO_JPEG_ADD_EXIF) && (C_PHOTO_JPEG_ADD_EXIF == 1)

#include "exif.h"

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
#define DBG_EXIF_LOG	0

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
ExifInfo
ifd_0_tags_table[] =
{
    {EXIF_TAG_MAKE, {"Generalplus"}},
    {EXIF_TAG_MODEL, {"GPCV2178_CVR"}},
    {EXIF_TAG_X_RESOLUTION, {"72", "1"}},
    {EXIF_TAG_Y_RESOLUTION, {"72", "1"}},
    {EXIF_TAG_RESOLUTION_UNIT, {"2"}}
};

ExifInfo
ifd_exif_tags_table[] =
{
	{EXIF_TAG_EXIF_VERSION,{"0x30323230"}},
	{EXIF_TAG_EXPOSURE_TIME,{"5","1000"}},
	{EXIF_TAG_FNUMBER,{"26","10"}},
    {EXIF_TAG_ISO_SPEED_RATINGS,{"100"}},
    {EXIF_TAG_SHUTTER_SPEED_VALUE,{"814","100"}},
    {EXIF_TAG_BRIGHTNESS_VALUE,{"540","100"}},
    {EXIF_TAG_EXPOSURE_BIAS_VALUE,{"0","1"}},
    {EXIF_TAG_METERING_MODE,{"1"}},
    {EXIF_TAG_LIGHT_SOURCE,{"1"}},
    {EXIF_TAG_FLASH,{"0"}},
    {EXIF_TAG_FOCAL_LENGTH,{"228","100"}},
    {EXIF_TAG_EXPOSURE_MODE,{"0"}},
    {EXIF_TAG_WHITE_BALANCE,{"0"}},
    {EXIF_TAG_DIGITAL_ZOOM_RATIO,{"0","0"}},
    {EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM,{"18"}},
    {EXIF_TAG_CONTRAST,{"0"}},
    {EXIF_TAG_SATURATION,{"0"}},
    {EXIF_TAG_SHARPNESS,{"0"}},
	{EXIF_TAG_DATE_TIME_ORIGINAL,{"2024:11:12 08:59:59"}},
	{EXIF_TAG_DATE_TIME_DIGITIZED,{"2024:11:12 08:59:59"}},
	{EXIF_TAG_COLOR_SPACE,{"1"}},
    {EXIF_TAG_MAX_APERTURE_VALUE,{"100","100"}},
    {EXIF_TAG_MAX_APERTURE_VALUE,{"200","100"}},
};

// For thumbnail image
ExifInfo
ifd_1_tags_table[] =
{
	{EXIF_TAG_COMPRESSION,{"6"}},// JPEG compression (thumbnails only)
	{EXIF_TAG_ORIENTATION,{"1"}},
    {EXIF_TAG_X_RESOLUTION, {"72", "1"}},
    {EXIF_TAG_Y_RESOLUTION, {"72", "1"}},
    {EXIF_TAG_RESOLUTION_UNIT, {"2"}} // inches

};

struct ExifGPSIfdTagInfo
exif_gps_ifd_tags[] =
{
    {EXIF_TAG_GPS_VERSION_ID, EXIF_FORMAT_BYTE, 4, 4, "\x02\x02\x00\x00"},
    {EXIF_TAG_GPS_LATITUDE_REF, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_LATITUDE, EXIF_FORMAT_RATIONAL, 3, 0, 0},
    {EXIF_TAG_GPS_LONGITUDE_REF, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_LONGITUDE, EXIF_FORMAT_RATIONAL, 3, 0, 0},
    {EXIF_TAG_GPS_ALTITUDE_REF, EXIF_FORMAT_BYTE, 1, 1, "\x00"},
    {EXIF_TAG_GPS_ALTITUDE, EXIF_FORMAT_RATIONAL, 1, 0, 0},
    {EXIF_TAG_GPS_TIME_STAMP, EXIF_FORMAT_RATIONAL, 3, 0, 0},
    {EXIF_TAG_GPS_SATELLITES, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_STATUS, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_MEASURE_MODE, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_DOP, EXIF_FORMAT_RATIONAL, 1, 0, 0},
    {EXIF_TAG_GPS_SPEED_REF, EXIF_FORMAT_ASCII, 2, 2, "K\x00"},
    {EXIF_TAG_GPS_SPEED, EXIF_FORMAT_RATIONAL, 1, 0, 0},
    {EXIF_TAG_GPS_TRACK_REF, EXIF_FORMAT_ASCII, 2, 2, "T\x00"},
    {EXIF_TAG_GPS_TRACK, EXIF_FORMAT_RATIONAL, 1, 0, 0},
    {EXIF_TAG_GPS_IMG_DIRECTION_REF, EXIF_FORMAT_ASCII, 2, 2, "T\x00"},
    {EXIF_TAG_GPS_IMG_DIRECTION, EXIF_FORMAT_RATIONAL, 1, 0, 0},
    {EXIF_TAG_GPS_MAP_DATUM, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_DEST_LATITUDE_REF, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_DEST_LATITUDE, EXIF_FORMAT_RATIONAL, 3, 0, 0},
    {EXIF_TAG_GPS_DEST_LONGITUDE_REF, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_DEST_LONGITUDE, EXIF_FORMAT_RATIONAL, 3, 0, 0},
    {EXIF_TAG_GPS_DEST_BEARING_REF, EXIF_FORMAT_ASCII, 2, 2, "T\x00"},
    {EXIF_TAG_GPS_DEST_BEARING, EXIF_FORMAT_RATIONAL, 1, 0, 0},
    {EXIF_TAG_GPS_DEST_DISTANCE_REF, EXIF_FORMAT_ASCII, 2, 2, "K\x00"},
    {EXIF_TAG_GPS_DEST_DISTANCE, EXIF_FORMAT_RATIONAL, 1, 0, 0},
    {EXIF_TAG_GPS_PROCESSING_METHOD, EXIF_FORMAT_UNDEFINED, 0, 0, 0},
    {EXIF_TAG_GPS_AREA_INFORMATION, EXIF_FORMAT_UNDEFINED, 0, 0, 0},
    {EXIF_TAG_GPS_DATE_STAMP, EXIF_FORMAT_ASCII, 0, 0, 0},
    {EXIF_TAG_GPS_DIFFERENTIAL, EXIF_FORMAT_SHORT, 1, 0, 0},
    {EXIF_TAG_GPS_H_POSITIONING_ERROR, EXIF_FORMAT_RATIONAL, 1, 0, 0},
};

// Front/Rear photo capture
static ExifData *exif_data_start[2] = {NULL,NULL};

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
#if DBG_EXIF_LOG
static const struct {
	ExifIfd ifd;
	const char *name;
} ExifIfdTable[] = {
	{EXIF_IFD_0, "0"},
	{EXIF_IFD_1, "1"},
	{EXIF_IFD_EXIF, "EXIF"},
	{EXIF_IFD_GPS, "GPS"},
	{EXIF_IFD_INTEROPERABILITY, "Interoperability"},
	{0, NULL}
};

void
exif_data_dump(
	INT32U src
)
{
	unsigned i, cnt;
	ExifContent *c;
	ExifRational r;
	ExifData * exif_data_handle = exif_data_start[src];

	if(exif_data_handle)
	{
		for (i = 0; i < EXIF_IFD_COUNT; i++)
		{
			DBG_PRINT("------\r\n");
			DBG_PRINT("IFD:%s\r\n",ExifIfdTable[i].name);
			c = exif_data_handle->ifd[i];
			if(c)
			{
				for (cnt = 0; cnt < c->count; cnt++)
				{
					if (c->entries[cnt])
					{
						DBG_PRINT("tag:0x%04X\r\n",c->entries[cnt]->tag);
						DBG_PRINT("format:%d\r\n",c->entries[cnt]->format);

						if(c->entries[cnt]->data == NULL)
						{
							DBG_PRINT("data:(NULL)\r\n");
						}
						else
						{
							switch(c->entries[cnt]->format)
							{
								case EXIF_FORMAT_ASCII:
									DBG_PRINT("data:%s\r\n",c->entries[cnt]->data);
								break;
								case EXIF_FORMAT_RATIONAL:
									r = exif_get_rational(c->entries[cnt]->data);
									DBG_PRINT("numerator:%d, denominator:%d\r\n",r.numerator, r.denominator);
								break;

							}
						}
						DBG_PRINT("\r\n");
					}
				}
				DBG_PRINT("------\r\n");
			}
			DBG_PRINT("\r\n");
		}
	}

}
#endif

INT32S
exif_info_init(
	INT32U src,
	INT32U thumbnail_img_en
)
{
	INT32S nRet = STATUS_OK;
 	ExifEntry *e;
	INT32U i;
	ExifData * exif_data_handle;

	exif_data_handle = exif_data_new();
	if (!exif_data_handle)
    {
    	ERR_PRINT("[%d] fail to new data\r\n",src);
   		RETURN(STATUS_FAIL);
	}

	// Init IFD-0
	for (i=0; i < sizeof(ifd_0_tags_table)/sizeof(ifd_0_tags_table[0]); ++i) {
 		e = exif_entry_new();
		if (!e) {
	    	ERR_PRINT("fail to new entry\r\n");
		}

		exif_content_add_entry(exif_data_handle->ifd[EXIF_IFD_0], e);
        exif_entry_initialize(e,&ifd_0_tags_table[i]);
    }

	// Init IFD-EXIF
	for (i=0; i < sizeof(ifd_exif_tags_table)/sizeof(ifd_exif_tags_table[0]); ++i) {
 		e = exif_entry_new();
		if (!e) {
	    	ERR_PRINT("fail to new entry\r\n");
		}

		exif_content_add_entry(exif_data_handle->ifd[EXIF_IFD_EXIF], e);
        exif_entry_initialize(e,&ifd_exif_tags_table[i]);
    }

	// Init IFD-GPS
	for (i=0; i < sizeof(exif_gps_ifd_tags)/sizeof(exif_gps_ifd_tags[0]); ++i) {
 		e = exif_entry_new();
		if (!e) {
	    	ERR_PRINT("fail to new entry\r\n");
		}

		exif_content_add_entry(exif_data_handle->ifd[EXIF_IFD_GPS], e);
        exif_entry_initialize_gps(e,&exif_gps_ifd_tags[i]);
    }

	if(thumbnail_img_en)
	{
		// Init IFD-1
		for (i=0; i < sizeof(ifd_1_tags_table)/sizeof(ifd_1_tags_table[0]); ++i) {
	 		e = exif_entry_new();
			if (!e) {
		    	DBG_PRINT("fail to new entry\r\n");
			}

			exif_content_add_entry(exif_data_handle->ifd[EXIF_IFD_1], e);
	        exif_entry_initialize(e,&ifd_1_tags_table[i]);
	    }
	}

	exif_data_start[src] = exif_data_handle;

Return:
	return nRet;
}

void
exif_tag_set(
	INT32U src,
	ExifIfd ifd_id,
	ExifTag tag,
	ExifArgs* arg
)
{
	ExifEntry *entry;
	ExifData * exif_data_handle = exif_data_start[src];

	entry = exif_content_get_entry(exif_data_handle->ifd[ifd_id], tag);
	switch(entry->format)
	{
		case EXIF_FORMAT_BYTE:
		{
		}
		break;
		case EXIF_FORMAT_LONG:
		{
		}
		break;
		case EXIF_FORMAT_ASCII:
        {
			if(arg->data0)
			{
           		INT32U newStingLen = strlen((char *)arg->data0)+1;
				if(newStingLen <= entry->size)
				{
					memset(entry->data,0,entry->size);
	                memcpy (entry->data, arg->data0, newStingLen);
					entry->components = newStingLen;
					entry->size = exif_format_get_size (EXIF_FORMAT_ASCII) * entry->components;
				}
				else
				{
					ERR_PRINT("The length of new staring over old string\r\n");
				}
			}
        }
        break;
		case EXIF_FORMAT_SHORT:
		{
			exif_set_short (entry->data, atoi((char *)arg->data0));
		}
		break;
		case EXIF_FORMAT_RATIONAL:
		{
			ExifRational r;
			r.numerator = atoi((char *)arg->data0);
			r.denominator = atoi((char *)arg->data1);
			exif_set_rational (entry->data, r);
		}
		break;
		case EXIF_FORMAT_SBYTE:
		{
		}
		break;
		case EXIF_FORMAT_UNDEFINED:
		{
		}
		break;
		case EXIF_FORMAT_SSHORT:
		{
		}
		break;
		case EXIF_FORMAT_SLONG:
		{
		}
		break;
		case EXIF_FORMAT_SRATIONAL:
		{
		}
		break;
	}
}

INT32S
exif_data_content_save(
	INT32U src,
	INT32U thumbnail_size,
	INT8U** ret_exif_data,
	INT32U* ret_exif_data_len
)
{
	INT16U app1_size = 0;
	ExifData * exif_data_handle = exif_data_start[src];

	if(exif_data_handle == NULL)
	{
		ERR_PRINT("[%d] exif_data_handle is null\r\n",src);
		return STATUS_FAIL;
	}

	exif_data_handle->size = thumbnail_size;

	/*
		Save Exif data
		ret_exif_data_len: return total bytes include 0xFF 0xFD ...
	*/
	exif_data_save_data(exif_data_handle,ret_exif_data,ret_exif_data_len);

	//DBG_PRINT("EXIF len:%d\r\n",*ret_exif_data_len);
	//DBG_PRINT("Thumbnail len:%d\r\n",thumbnail_size);

	app1_size = (thumbnail_size+*ret_exif_data_len);
	//DBG_PRINT("app1_size:%d\r\n",app1_size);

	exif_set_sshort(*ret_exif_data+4,(app1_size-4));//Reduce 0xFF 0xD8 0xFF 0xE1

	return STATUS_OK;
}

INT32S
exif_info_uninit(
	INT32U src
)
{
	ExifData * exif_data_handle = exif_data_start[src];

	if(exif_data_handle)
	{
		exif_data_free(exif_data_handle);
		exif_data_start[src] = NULL;
	}

	return STATUS_OK;
}
#endif //#if (defined C_PHOTO_JPEG_ADD_EXIF) && (C_PHOTO_JPEG_ADD_EXIF == 1)


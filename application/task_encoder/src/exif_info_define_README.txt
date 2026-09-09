/*===如何添加EXIF tag:===*/
1.開啟application\task_encoder\src\exif_info_define.c
2.在"G L O B A L    D A T A"區域,修改以下table
ifd_0_tags_table[]
ifd_exif_tags_table[]
ifd_1_tags_table[]
exif_gps_ifd_tags[]

3.格式為 ExifInfo 結構體,此結構體定義在 gplib\linexif\inc\exif-entry.h
typedef struct _ExifArgs        ExifArgs;
struct _ExifArgs {
	/*! Pointer to the raw EXIF data for this entry. */
        unsigned char *data0; // put to the first argument
        unsigned char *data1;
};

typedef struct _ExifInfo        ExifInfo;
struct _ExifInfo {
	/*! EXIF tag for this entry */
        ExifTag tag;

	/*! Pointer to the raw EXIF data for this entry. */
		ExifArgs arg;
};


範例:#1/#2 示範如何填 ExifInfo table
#1.
    tag             data0
{EXIF_TAG_MAKE, {"Generalplus"}}

#2.
    tag                 data0 data1 
{EXIF_TAG_X_RESOLUTION, {"72", "1"}}

4.如何將data0/data1帶入tag,需要修改 exif_entry_initialize() API, 此API定義在 gplib\linexif\src\exif-entry.c 
#1.以{EXIF_TAG_X_RESOLUTION, {"72", "1"}} 為例,需要添加 LINE47~51 將data0/data1 帶入numerator/denominator

	case EXIF_TAG_X_RESOLUTION:
	case EXIF_TAG_Y_RESOLUTION:
		e->components = 1;
		e->format = EXIF_FORMAT_RATIONAL;
		e->size = exif_format_get_size (e->format) * e->components;
		e->data = exif_entry_alloc (e, e->size);
		if (!e->data) { clear_entry(e); break; }
		
		if(info->arg.data0 && info->arg.data1)
		{
			r.numerator = atoi(info->arg.data0);
			r.denominator = atoi(info->arg.data1);
		}
		else
		{
			r.numerator = 72;
			r.denominator = 1;			
		}
		exif_set_rational (e->data, r);
		break;
 
5.如tag 需帶入2參數以上,請修改第3項的struct _ExifArgs擴充data2...及第4項的exif_entry_initialize()API將dataN...帶入tag
6.tag相關說明,請網路搜尋EXIF    
 
/*===如何添加程式中修改tag參數:===*/ 
1.使用exif_tag_set() API,此API定義在application\task_encoder\src\exif_info_define.c
2.以下範例示範動態修改tag 內容值   

    ExifArgs argMake = {"SampleDemo"};
    exif_tag_set(src,EXIF_IFD_0,EXIF_TAG_MAKE,&argMake);

    ExifArgs argExp = {"1","500"};
    exif_tag_set(src,EXIF_IFD_EXIF,EXIF_TAG_EXPOSURE_TIME,&argExp);

    ExifArgs argGPS = {"M"};
    exif_tag_set(src,EXIF_IFD_GPS,EXIF_TAG_GPS_SPEED_REF,&argGPS);			

3.tag format 並未完全實現,請修改第1項的exif_tag_set() API

 
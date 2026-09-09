#include "application.h"

t_GP_RESOURCE_HEADER *resource_header;
t_FONT_TABLE_STRUCT *number_font_cache;
t_FONT_TABLE_STRUCT *number_font_cache_1;
t_FONT_TABLE_STRUCT *number_font_cache_2;
static INT16U resource_handle;
INT8U	*ir_key_code;
INT32U	*ir_key_msg_id;
INT16S	time_stamp_position_x, time_stamp_position_y;

#define COLOR_MAP_ARRAY_MAX     32
t_COLOR_MAP color_map_array[COLOR_MAP_ARRAY_MAX];
INT16U color_map_array_size;

INT8U keycode[19] = {
	0xa, 0xe, 0x1c, 0x14, 0x54, 0x4d, 0x1a, 0x4, 0x40, 0x10, 0x03, 0x01, 0x06, 0x1d, 0x1f, 0x0d, 0x1b, 0x11, 0x15
};

INT32S resource_read(INT32U offset_byte, INT8U *pbuf, INT32U byte_count)
{
	if(resource_handle == 0xFFFF)
	{
        #if 0   //Eugene removed 20251223
		DBG_PRINT("[ERROR] resource handle is null! @%s:%d\r\n", __func__, __LINE__);
		#endif
		return STATUS_FAIL;
	}

	nv_lseek(resource_handle, offset_byte, SEEK_SET);
	if(nv_read(resource_handle, (INT32U) pbuf, byte_count))
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

void ap_state_resource_time_stamp_position_x_set(INT16S x)
{
	time_stamp_position_x = x;
}

void ap_state_resource_time_stamp_position_y_set(INT16S y)
{
	time_stamp_position_y = y;
}

INT16S ap_state_resource_time_stamp_position_x_get(void)
{
	return time_stamp_position_x;
}

INT16S ap_state_resource_time_stamp_position_y_get(void)
{
	return time_stamp_position_y;
}

INT32U ap_state_resource_string_load(INT16U language, INT16U index)
{
	INT32U			offset, buff_addr;
	INT32S			size;
	t_STRING_STRUCT string_header;

	offset = (resource_header->offset_string[language]) + (sizeof(t_STRING_STRUCT) * index);
	if(resource_read(offset, (INT8U *) &string_header, sizeof(t_STRING_STRUCT)))
	{		//read string header
		DBG_PRINT("Failed to read string header in resource_string_load()\r\n");
		return 0;
	}

	size = (string_header.length) * (resource_header->str_ch_width[language]);
	if(size <= 0)
	{
		return 0;
	}

	buff_addr = (INT32U) gp_malloc(size);
	if(!buff_addr)
	{
		DBG_PRINT("fail to allocate buffer size %d in resource_string_load\r\n", size);
		return 0;
	}

	if(resource_read(string_header.raw_data_offset, (INT8U *) buff_addr, size))
	{		//read string
		gp_free((void *) buff_addr);
		DBG_PRINT("Failed to read string data in resource_string_load()\r\n");
		return 0;
	}

	//repair resource_read() error
	if(resource_header->str_ch_width[language] == 1)
	{
        *((INT8U *)buff_addr + string_header.length - 1) = 0;
	}
	else
	{
        *((INT16U *)buff_addr + string_header.length - 1) = 0;
	}

	return buff_addr;
}

INT32S ap_state_resource_char_resolution_get(INT16U target_char, STRING_INFO *str_info, t_STR_SIZE_STRUCT *str_res)
{
	INT32U				offset;
	t_FONT_STRUCT		font_header;
	t_FONT_TABLE_STRUCT font;

	if(str_info->language == LCD_EN)
	{
		target_char -= 0x0020;
	}

	if(str_info->language == LCD_EN && (target_char > 0xE && target_char < 0x1B) && number_font_cache)
	{
        t_FONT_TABLE_STRUCT *ptr;

        if(str_info->font_type == 1)
        {
            ptr = number_font_cache_1;
        }
        else if(str_info->font_type == 2)
        {
            ptr = number_font_cache_2;
        }
        else
        {
            ptr = number_font_cache;
        }

        str_res->string_width += (ptr + (target_char - 0xF))->font_width;
        str_res->string_height = (ptr + (target_char - 0xF))->font_height;
	}
	else
	{
		offset = (resource_header->offset_font[str_info->language]) + (sizeof(t_FONT_STRUCT) * str_info->font_type);
		if(resource_read(offset, (INT8U *) &font_header, sizeof(t_FONT_STRUCT)))
		{	//read font header
            #if 0   //Eugene removed 20251223
			DBG_PRINT("Failed to read font header in resource_font_load()\r\n");
			#endif
			return STATUS_FAIL;
		}

		//get target charactor table offset
		offset = font_header.raw_data_offset + (target_char * sizeof(t_FONT_TABLE_STRUCT));
		if(resource_read(offset, (INT8U *) &font, sizeof(t_FONT_TABLE_STRUCT)))
		{	//read font table
			DBG_PRINT("Failed to read font table in resource_font_load()\r\n");
			return STATUS_FAIL;
		}

		str_res->string_width += font.font_width;
		if(str_res->string_height < font.font_height)
		{
			str_res->string_height = font.font_height;
		}
	}

	return STATUS_OK;
}

INT32S ap_state_resource_string_resolution_get(STRING_INFO *str_info, t_STR_SIZE_STRUCT *str_res)
{
	INT32U	str_addr, iStrLoc;
	INT16U	character = 0;

	str_addr = ap_state_resource_string_load(str_info->language, str_info->str_idx);
	if(!str_addr)
	{
		DBG_PRINT("Failed load string\r\n");
		return STATUS_FAIL;
	}

	str_res->string_width = 0;
	str_res->string_height = 0;
	iStrLoc = 0;
	while(1)
	{
		if(str_info->language == LCD_EN)
		{
			character = (INT16U) (((char *) str_addr)[iStrLoc]);
			if(character == 0)
			{
				break;
			}

			if((character < 0x20) || character > 0x7f)
			{
				character = '_';
			}

			iStrLoc++;
		}
		else
		{
			character = ((INT16U *) str_addr)[iStrLoc];
			if(character == 0)
			{
				break;
			}

			iStrLoc++;
		}

		if(ap_state_resource_char_resolution_get(character, str_info, str_res))
		{
			gp_free((void *) str_addr);
			DBG_PRINT("Failed to draw char font\r\n");
			return STATUS_FAIL;
		}
	}

	gp_free((void *) str_addr);
	return STATUS_OK;
}

INT32S ap_state_resource_ascii_string_resolution_get(STRING_ASCII_INFO *ascii_str_info, t_STR_SIZE_STRUCT *str_res)
{
	INT16U	iStrLoc;
	INT8U	character = 0;
	STRING_INFO str_info = {0};

	str_info.font_color = ascii_str_info->font_color;
	str_info.font_type = ascii_str_info->font_type;
	str_info.language = LCD_EN;

	str_res->string_width = 0;
	str_res->string_height = 0;
	iStrLoc = 0;
	while(1)
	{
        character = ascii_str_info->str_ptr[iStrLoc];
        if(character == 0)
        {
            break;
        }

        if((character < 0x20) || character > 0x7f)
        {
            character = '_';
        }

        iStrLoc++;
		if(ap_state_resource_char_resolution_get(character, &str_info, str_res))
		{
            #if 0   //Eugene removed 20251223
			DBG_PRINT("%s failed\r\n", __FUNCTION__);
			#endif
			return STATUS_FAIL;
		}
	}
	return STATUS_OK;
}


INT32S ap_state_resource_number_font_cache_init(void)
{
	INT32U				offset, offset1, offset2, len, len1, len2, i;
	INT8U				*input_buffer, *input_buffer1, *input_buffer2;
	INT32U				input_buffer_len_temp, input_buffer_len_temp1, input_buffer_len_temp2, temp;
	t_FONT_STRUCT		font_header, font_header1, font_header2;
	t_FONT_TABLE_STRUCT *font, *font1, *font2;

	input_buffer_len_temp = 0;
	offset = resource_header->offset_font[LCD_EN];                                  //font_type = 0
	if(resource_read(offset, (INT8U *) &font_header, sizeof(t_FONT_STRUCT)))
	{		//read font header
		DBG_PRINT("Failed to read font header in ap_state_resource_number_font_cache_init()\r\n");
		return -1;
	}

	for(i = 0; i < 12; i++)
	{
		font = number_font_cache + i;

		//get target charactor table offset
		offset = font_header.raw_data_offset + ((i + 0xF) * sizeof(t_FONT_TABLE_STRUCT));
		if(resource_read(offset, (INT8U *) font, sizeof(t_FONT_TABLE_STRUCT)))
		{	//read font table
			DBG_PRINT("Failed to read font table in ap_state_resource_number_font_cache_init()\r\n");
			return -1;
		}

		len = font->font_height * font->bytes_per_line;
		input_buffer_len_temp += len;
	}

	input_buffer_len_temp1 = 0;
	offset1 = resource_header->offset_font[LCD_EN] + sizeof(t_FONT_STRUCT);         //font_type = 1
	if(resource_read(offset1, (INT8U *) &font_header1, sizeof(t_FONT_STRUCT)))
	{		//read font header
		DBG_PRINT("Failed to read font header in ap_state_resource_number_font_cache_init()\r\n");
		return -1;
	}

	for(i = 0; i < 12; i++)
	{
		font1 = number_font_cache_1 + i;

		//get target charactor table offset
		offset1 = font_header1.raw_data_offset + ((i + 0xF) * sizeof(t_FONT_TABLE_STRUCT));
		if(resource_read(offset1, (INT8U *) font1, sizeof(t_FONT_TABLE_STRUCT)))
		{	//read font table
			DBG_PRINT("Failed to read font table in ap_state_resource_number_font_cache_init()\r\n");
			return -1;
		}

		len1 = font1->font_height * font1->bytes_per_line;
		input_buffer_len_temp1 += len1;
	}

	input_buffer_len_temp2 = 0;
	offset2 = resource_header->offset_font[LCD_EN] + sizeof(t_FONT_STRUCT) * 2;     //font_type = 2
	if(resource_read(offset2, (INT8U *) &font_header2, sizeof(t_FONT_STRUCT)))
	{		//read font header
		DBG_PRINT("Failed to read font header in ap_state_resource_number_font_cache_init()\r\n");
		return -1;
	}

	for(i = 0; i < 12; i++)
	{
		font2 = number_font_cache_2 + i;

		//get target charactor table offset
		offset2 = font_header2.raw_data_offset + ((i + 0xF) * sizeof(t_FONT_TABLE_STRUCT));
		if(resource_read(offset2, (INT8U *) font2, sizeof(t_FONT_TABLE_STRUCT)))
		{	//read font table
			DBG_PRINT("Failed to read font table in ap_state_resource_number_font_cache_init()\r\n");
			return -1;
		}

		len2 = font2->font_height * font2->bytes_per_line;
		input_buffer_len_temp2 += len2;
	}

	temp = (INT32U) gp_malloc(input_buffer_len_temp + input_buffer_len_temp1 + input_buffer_len_temp2);
	if(!temp)
	{
		DBG_PRINT("Failed to allocate buffer in ap_state_resource_number_font_cache_init()\r\n");
		return -1;
	}

	input_buffer_len_temp2 = temp + input_buffer_len_temp1 + input_buffer_len_temp;
	input_buffer_len_temp1 = temp + input_buffer_len_temp;
	input_buffer_len_temp = temp;

	for(i = 0; i < 12; i++)
	{
		font = number_font_cache + i;
		len = font->font_height * font->bytes_per_line;
		input_buffer = (INT8U *) input_buffer_len_temp;
		input_buffer_len_temp += len;

		if(resource_read(font_header.raw_data_offset + font->font_content, input_buffer, len))
		{	//read font content
			DBG_PRINT("Failed to read font data in ap_state_resource_number_font_cache_init()\r\n");
			gp_free((void *) temp);
			return -1;
		}

		font->font_content = (INT32U) input_buffer;
	}

	for(i = 0; i < 12; i++)
	{
		font1 = number_font_cache_1 + i;
		len1 = font1->font_height * font1->bytes_per_line;
		input_buffer1 = (INT8U *) input_buffer_len_temp1;
		input_buffer_len_temp1 += len1;
		if(resource_read(font_header1.raw_data_offset + font1->font_content, input_buffer1, len1))
		{	//read font content
			DBG_PRINT("Failed to read font data in ap_state_resource_number_font_cache_init()\r\n");
			gp_free((void *) temp);
			return -1;
		}

		font1->font_content = (INT32U) input_buffer1;
	}

	for(i = 0; i < 12; i++)
	{
		font2 = number_font_cache_2 + i;
		len2 = font2->font_height * font2->bytes_per_line;
		input_buffer2 = (INT8U *) input_buffer_len_temp2;
		input_buffer_len_temp2 += len2;

		if(resource_read(font_header2.raw_data_offset + font2->font_content, input_buffer2, len2))
		{	//read font content
			DBG_PRINT("Failed to read font data in ap_state_resource_number_font_cache_init()\r\n");
			gp_free((void *) temp);
			return -1;
		}

		font2->font_content = (INT32U) input_buffer2;
	}

	DBG_PRINT("Resource cache initial OK!\r\n");
	return 0;
}

INT16U ap_state_resource_language_num_get(void)
{
	if(!resource_header)
	{
		return 0;
	}

	return resource_header->language_num;
}

/*
INT32S ap_state_resource_user_option_load(SYSTEM_USER_OPTION *user_option)
{
	if(resource_read(resource_header->offset_factor_default_option, (INT8U *) user_option, sizeof(SYSTEM_USER_OPTION)))
	{
		DBG_PRINT("Failed to read user config in resource_user_items_load()\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
} */



void ap_RGB_Transfer_GP420(INT16U RGB, INT8U *Y, INT8U *U, INT8U *V)
{
    INT8U   R, G, B;
    INT32S  temp_y, temp_u, temp_v;

    R = ((RGB >> 11) & 0x1f) << 3;
    if(R >= (7 << 1))    R |= 0x07;
    G = ((RGB >> 5) & 0x3f) << 2;
    if(G >= (3 << 1))   G |= 0x03;
    B = (RGB & 0x1f) << 3;
    if(B >= (7 << 1))   B |= 0x07;

    temp_y = 77 * R + 151 * G + 28 * B;
    temp_u = -43 * R - 85 * G + 128 * B + 128 * 256;
    temp_v = 128 * R - 108 * G - 20 * B + 128 * 256;

    temp_y = temp_y >> 8;
    temp_u = temp_u >> 8;
    temp_v = temp_v >> 8;

    if(temp_y > 255)    temp_y = 255;
    else if(temp_y < 0) temp_y = 0;

    if(temp_u > 255)    temp_u = 255;
    else if(temp_u < 0) temp_u = 0;

    if(temp_v > 255)    temp_v = 255;
    else if(temp_v < 0) temp_v = 0;

    *Y = temp_y;
    *U = temp_u;
    *V = temp_v;
}


void ap_state_resource_color_map_init(void)
{
    t_COLOR_MAP *ptr;
    INT8U   Y, U, V;

    gp_memset((INT8S *)&color_map_array[0], 0, sizeof(color_map_array));

    ap_RGB_Transfer_GP420(WHITE_COLOR, &Y, &U, &V);
    ptr = &color_map_array[0];
    ptr->rgb_color = WHITE_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    ap_RGB_Transfer_GP420(RED_COLOR, &Y, &U, &V);
    ptr += 1;
    ptr->rgb_color = RED_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    ap_RGB_Transfer_GP420(GREEN_COLOR, &Y, &U, &V);
    ptr += 1;
    ptr->rgb_color = GREEN_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    ap_RGB_Transfer_GP420(BLUE_COLOR, &Y, &U, &V);
    ptr += 1;
    ptr->rgb_color = BLUE_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    ap_RGB_Transfer_GP420(YELLOW_COLOR, &Y, &U, &V);
    ptr += 1;
    ptr->rgb_color = YELLOW_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    ap_RGB_Transfer_GP420(BLACK_COLOR, &Y, &U, &V);
    ptr += 1;
    ptr->rgb_color = BLACK_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    ap_RGB_Transfer_GP420(BLUE1_COLOR, &Y, &U, &V);
    ptr += 1;
    ptr->rgb_color = BLUE1_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    ap_RGB_Transfer_GP420(PURPLE_COLOR, &Y, &U, &V);
    ptr += 1;
    ptr->rgb_color = PURPLE_COLOR;
    ptr->Y = Y;
    ptr->U = U;
    ptr->V = V;

    color_map_array_size = 8;
}

void rgb_to_yuv(INT16U rgb, INT8U *Y, INT8U *U, INT8U *V)
{
    INT16U  i;

    for(i = 0; i < color_map_array_size; i++)
    {
        if(color_map_array[i].rgb_color == rgb)
        {
            *Y = color_map_array[i].Y;
            *U = color_map_array[i].U;
            *V = color_map_array[i].V;
            return;
        }
    }

    ap_RGB_Transfer_GP420(rgb, Y, U, V);

    if(color_map_array_size < COLOR_MAP_ARRAY_MAX)
    {
        color_map_array[color_map_array_size].rgb_color = rgb;
        color_map_array[color_map_array_size].Y = *Y;
        color_map_array[color_map_array_size].U = *U;
        color_map_array[color_map_array_size].V = *V;
        color_map_array_size += 1;
    }
}


INT32S ap_state_resource_init(void)
{
	INT32S	font_cache_sts;
	INT32U	size;
	INT32U	buffer_ptr;

	ap_state_resource_exit();

	resource_header = (t_GP_RESOURCE_HEADER *) gp_malloc(sizeof(t_GP_RESOURCE_HEADER));
	if(!resource_header)
	{
		DBG_PRINT("Failed to allocate resource_header in umi_resource_init\r\n");
		return STATUS_FAIL;
	}

	resource_handle = nv_open((INT8U *) "GPRS.PAK");
	if(resource_handle == 0xFFFF)
	{
		DBG_PRINT("Failed to open resource from NVRAM \r\n");
		gp_free((void *) resource_header);
		resource_header = NULL;
		return STATUS_FAIL;
	}

	if(resource_read(0, (INT8U *) resource_header, sizeof(t_GP_RESOURCE_HEADER)))
	{
		DBG_PRINT("Failed to read resource_header in umi_resource_init\r\n");
		gp_free((void *) resource_header);
		resource_header = NULL;
		return STATUS_FAIL;
	}

	resource_header->irkey_num = sizeof(keycode);

	size = sizeof(t_GP_RESOURCE_HEADER) + (36 * sizeof(t_FONT_TABLE_STRUCT)) + ((resource_header->irkey_num + 3) &~0x3) + (resource_header->irkey_num * 4);

	buffer_ptr = (INT32U) gp_malloc(size);
	if(!buffer_ptr)
	{
		DBG_PRINT("Failed to allocate buffer_ptr in umi_resource_init\r\n");
		gp_free((void *) resource_header);
		resource_header = NULL;
		return STATUS_FAIL;
	}

	gp_memcpy((INT8S *) buffer_ptr, (INT8S *) resource_header, sizeof(t_GP_RESOURCE_HEADER));
	gp_free((void *) resource_header);
	resource_header = (t_GP_RESOURCE_HEADER *) buffer_ptr;
	number_font_cache = (t_FONT_TABLE_STRUCT *) (resource_header + 1);
	number_font_cache_1 = (t_FONT_TABLE_STRUCT *) (number_font_cache + 12);
	number_font_cache_2 = (t_FONT_TABLE_STRUCT *) (number_font_cache_1 + 12);

	font_cache_sts = ap_state_resource_number_font_cache_init();
	if(font_cache_sts < 0)
	{
		if(number_font_cache->font_content)
		{
			gp_free((void *) number_font_cache->font_content);
		}

		number_font_cache = number_font_cache_1 = number_font_cache_2 = 0;
	}

	ir_key_code = (INT8U *) (number_font_cache_2 + 12);
	ir_key_msg_id = (INT32U *) (ir_key_code + ((resource_header->irkey_num + 3) &~0x3));

	//set the time stamp position here
	ap_state_resource_time_stamp_position_x_set(64);    //for video record & photo capture
	ap_state_resource_time_stamp_position_y_set(-64);   //for photo capture

	//init color_map
	ap_state_resource_color_map_init();
	return STATUS_OK;
}

void ap_state_resource_exit(void)
{
	if(resource_header)
	{
		if(number_font_cache && number_font_cache->font_content)
		{
			gp_free((void *) number_font_cache->font_content);
		}

		gp_free((void *) resource_header);
		resource_header = NULL;
		number_font_cache = number_font_cache_1 = number_font_cache_2 = 0;
	}
}

INT32S ap_state_resource_char_draw(INT16U target_char, INT16U *frame_buff, STRING_INFO *str_info, INT8U draw_type, INT8U draw_black_edge, INT8U draw_uv)
{
	INT32U  offset, offset_pixel, font_data, offset_tmp, byte_tmp, len, width, height;
	INT16S  pos_x, pos_y;
	INT32S  i, j, buf_h_idx;
	INT8U   *input_buffer, Y, U, V;
	INT8U   *ptr_byte_buf = (INT8U *)frame_buff;
	t_FONT_STRUCT		font_header;
	t_FONT_TABLE_STRUCT font;
	t_FONT_TABLE_STRUCT *pfont;

	if(str_info->language == LCD_EN)
	{
		target_char -= 0x0020;
	}

	if(str_info->language == LCD_EN && (target_char > 0xE && target_char < 0x1B) && number_font_cache)
	{
		if(str_info->font_type == 0)
		{
			pfont = number_font_cache + (target_char - 0xF);
		}
		else
		if(str_info->font_type == 1)
		{
			pfont = number_font_cache_1 + (target_char - 0xF);
		}
		else //font_type = 2
		{
			pfont = number_font_cache_2 + (target_char - 0xF);
		}
		input_buffer = (INT8U *)(pfont->font_content);
		len = 0;
	}
	else
	{
		offset = (resource_header->offset_font[str_info->language]) + (sizeof(t_FONT_STRUCT) * str_info->font_type);
		if(resource_read(offset, (INT8U *) &font_header, sizeof(t_FONT_STRUCT)))
		{	//read font header
            #if 0   //Eugene removed 20251223
			DBG_PRINT("Failed to read font header in resource_font_load()\r\n");
			#endif
			return STATUS_FAIL;
		}

		//get target charactor table offset
		offset = font_header.raw_data_offset + (target_char * sizeof(t_FONT_TABLE_STRUCT));
		if(resource_read(offset, (INT8U *) &font, sizeof(t_FONT_TABLE_STRUCT)))
		{	//read font table
			DBG_PRINT("Failed to read font table in resource_font_load()\r\n");
			return STATUS_FAIL;
		}

		len = font.font_height * font.bytes_per_line;
		input_buffer = (INT8U *) gp_malloc(len);
		if(!input_buffer)
		{
			DBG_PRINT("Failed to allocate input_buffer in resource_font_load()\r\n");
			return STATUS_FAIL;
		}

		if(resource_read(font_header.raw_data_offset + font.font_content, input_buffer, len))
		{	//read font content
			gp_free(input_buffer);
			DBG_PRINT("Failed to read font data in resource_font_load()\r\n");
			return STATUS_FAIL;
		}
        pfont = &font;
	}

	width = str_info->buff_w;
	height = str_info->buff_h;
	pos_x = str_info->pos_x;
	pos_y = str_info->pos_y;

	if(pos_x < 0)   pos_x += width;
	if(pos_y < 0)   pos_y += height;

	if((pos_x + pfont->font_width) > width)  goto    Return;

    font_data = 0;

    //draw black frame edge
    if(draw_black_edge == DRAW_BLACK_EDGE_ON)
    {
        for(i = 0; i < pfont->font_height; i++)
        {
            if((pos_y + i) >= (height - 1))		break;

            if(draw_type == YUV420_DRAW)
            {
                /*
                    0-> YYYYYYYY
                        UVUVUVUV
                    1->	YYYYYYYY

                    2-> YYYYYYYY
                        UVUVUVUV
                    3-> YYYYYYYY

                 odd line: locate to previous line Y and add pos_x, then add 2*width bytes
                 even line: directly locate to 1.5*Y*width bytes and add pos_x
                */
                if((pos_y + i) & 0x1)
                {
                    offset_tmp = ((pos_y + i - 1) * width * 3 >> 1) + (width << 1) + pos_x;
                }
                else
                {
                    offset_tmp = ((pos_y + i) * width * 3 >> 1) + pos_x;
                }
            }
            else
            if(draw_type == YUYV_DRAW)
            {
                offset_tmp = (pos_y + i) * width + pos_x;
            }
            else //RGB565_DRAW
            {
                offset_tmp = (pos_y + i) * width + pos_x;
            }

            byte_tmp = i * pfont->bytes_per_line;
            for(j = 0; j < pfont->font_width; j++)
            {
                if(!(j & 7))
                {
                    font_data = *(input_buffer + (j >> 3) + byte_tmp);
                }

                if(font_data & (1 << (7 - (j & 7))))
                {
                    offset_pixel = offset_tmp + j;
                    if(draw_type == YUV420_DRAW)
                    {
                        if((pos_y + i) & 0x1) //odd
                        {
                            if(offset_pixel >= 1)
                                *(ptr_byte_buf + offset_pixel - 1) = 0;
                            *(ptr_byte_buf + offset_pixel + 1) = 0;
                            if(offset_pixel >= (width << 1))
                                *(ptr_byte_buf + offset_pixel - (width << 1)) = 0;
                            *(ptr_byte_buf + offset_pixel + width) = 0;
                        }
                        else  // even
                        {
                            if(offset_pixel >= 1)
                                *(ptr_byte_buf + offset_pixel - 1) = 0;
                            *(ptr_byte_buf + offset_pixel + 1) = 0;
                            if(offset_pixel >= width)
                                *(ptr_byte_buf + offset_pixel - width) = 0;
                            *(ptr_byte_buf + offset_pixel + (width << 1)) = 0;
                        }
                    }
                    else
                    if(draw_type == YUYV_DRAW)
                    {
                        if(offset_pixel >= 1)
                            *(frame_buff + offset_pixel - 1) = 0x0080;
                        *(frame_buff + offset_pixel + 1) = 0x0080;
                        if(offset_pixel >= width)
                            *(frame_buff + offset_pixel - width ) = 0x0080;
                        *(frame_buff + offset_pixel + width ) = 0x0080;
                    }
                    else //RGB565_DRAW
                    {
                        if(offset_pixel >= 1)
                            *(frame_buff + offset_pixel - 1) = 0;
                        *(frame_buff + offset_pixel + 1) = 0;
                        if(offset_pixel >= width)
                            *(frame_buff + offset_pixel - width) = 0;
                        *(frame_buff + offset_pixel + width) = 0;
                    }
                }
            }
        }
    }

    if(draw_type != RGB565_DRAW)
    {
        rgb_to_yuv(str_info->font_color, &Y, &U, &V);
    }

    //draw
    if(((str_info->font_color == WHITE_COLOR) || (str_info->font_color == BLACK_COLOR)) && (draw_uv == DRAW_UV_OFF))
    { //draw Y only
        for(i = str_info->font_off_h, buf_h_idx = 0; i < pfont->font_height; i++, buf_h_idx++)
        {
            if((pos_y + buf_h_idx) >= height)	break;

            if(draw_type == YUV420_DRAW)
            {
                /*
                    0-> YYYYYYYY
                        UVUVUVUV
                    1->	YYYYYYYY

                    2-> YYYYYYYY
                        UVUVUVUV
                    3-> YYYYYYYY

                 odd line: locate to previous line Y and add pos_x, then add 2*width bytes
                 even line: directly locate to 1.5*Y*width bytes and add pos_x
                */
                if((pos_y + buf_h_idx) & 0x1)
                {
                    offset_tmp = ((pos_y + buf_h_idx - 1) * width * 3 >> 1) + (width << 1) + pos_x;
                }
                else
                {
                    offset_tmp = ((pos_y + buf_h_idx) * width * 3 >> 1) + pos_x;
                }
            }
            else
            {
                offset_tmp = (pos_y + buf_h_idx) * width + pos_x;
            }

            byte_tmp = i * pfont->bytes_per_line;
            for(j = 0; j < pfont->font_width; j++)
            {
                if(!(j & 7))
                {
                    font_data = *(input_buffer + (j >> 3) + byte_tmp);
                    if(font_data == 0)
                    {
                        j += 7;
                        continue;
                    }
                }

                //if(font_data & (1 << (7 - (j&7))))
                if(font_data & 0xF0)
                {
                    if(font_data & 0x80)
                    {
                        offset_pixel = offset_tmp + j;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }

                    if(font_data & 0x40)
                    {
                        offset_pixel = offset_tmp + j + 1;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }

                    if(font_data & 0x20)
                    {
                        offset_pixel = offset_tmp + j + 2;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }

                    if(font_data & 0x10)
                    {
                        offset_pixel = offset_tmp + j + 3;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }
                }

                if(font_data & 0x0F)
                {
                    if(font_data & 0x08)
                    {
                        offset_pixel = offset_tmp + j + 4;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }

                    if(font_data & 0x04)
                    {
                        offset_pixel = offset_tmp + j + 5;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }

                    if(font_data & 0x02)
                    {
                        offset_pixel = offset_tmp + j + 6;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }

                    if(font_data & 0x01)
                    {
                        offset_pixel = offset_tmp + j + 7;
                        if(draw_type == YUV420_DRAW)
                        {
                            *(ptr_byte_buf + offset_pixel) = Y;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                        }
                    }
                }
                j += 7;
            }
        }
    }
    else
    {
        for(i = str_info->font_off_h, buf_h_idx = 0; i < pfont->font_height; i++, buf_h_idx++)
        {
            if((pos_y + buf_h_idx) >= height)	break;

            if(draw_type == YUV420_DRAW)
            {
                /*
                    0-> YYYYYYYY
                        UVUVUVUV
                    1->	YYYYYYYY

                    2-> YYYYYYYY
                        UVUVUVUV
                    3-> YYYYYYYY

                 odd line: locate to previous line Y and add pos_x, then add 2*width bytes
                 even line: directly locate to 1.5*Y*width bytes and add pos_x
                */
                if((pos_y + buf_h_idx) & 0x1)
                {
                    offset_tmp = ((pos_y + buf_h_idx - 1) * width * 3 >> 1) + (width << 1) + pos_x;
                }
                else
                {
                    offset_tmp = ((pos_y + buf_h_idx) * width * 3 >> 1) + pos_x;
                }
            }
            else
            if(draw_type == YUYV_DRAW)
            {
                offset_tmp = (pos_y + buf_h_idx) * width + pos_x;
            }
            else //RGB565_DRAW
            {
                offset_tmp = (pos_y + buf_h_idx) * width + pos_x;
            }

            byte_tmp = i * pfont->bytes_per_line;
            for(j = 0; j < pfont->font_width; j++)
            {
                if(!(j & 7))
                {
                    font_data = *(input_buffer + (j >> 3) + byte_tmp);
                    if(font_data == 0)
                    {
                        j += 7;
                        continue;
                    }
                }

                if(font_data & (1 << (7 - (j&7))))
                {
                    offset_pixel = offset_tmp + j;
                    if(draw_type == YUV420_DRAW)
                    {
                        *(ptr_byte_buf + offset_pixel) = Y;
                        if(((pos_y + buf_h_idx) & 0x1) == 0) //even
                        {
                            if(((pos_x + j) & 0x01) == 0)	// even
                            {
                                *(ptr_byte_buf + offset_pixel + width) = U;
                                *(ptr_byte_buf + offset_pixel + width + 1) = V;
                            }
                        }
                    }
                    else if(draw_type == YUYV_DRAW)
                    {
                        if((pos_x + j) & 0x01)	// odd
                        {
                            *((INT8U *)(frame_buff + offset_pixel)) = U;
                        }
                        else
                        {
                            *((INT8U *)(frame_buff + offset_pixel)) = V;
                        }

                        *((INT8U *)(frame_buff + offset_pixel) + 1) = Y;
                    }
                    else
                    {
                        *(frame_buff + offset_pixel) = str_info->font_color;
                    }
                }
            }
        }
    }

	pos_x += pfont->font_width;
	str_info->pos_x = pos_x;
Return:
	if(len)
	{
		gp_free((void *) input_buffer);
	}
	return STATUS_OK;
}


static INT32S ap_string_draw(INT32U str_addr, INT16U *frame_buff, STRING_INFO *str_info, INT8U draw_type, INT8U draw_black_edge, INT8U draw_uv)
{
    INT32U iStrLoc = 0;
	INT16U character = 0;
	INT32S st = STATUS_OK;

	while(1)
	{
		if(str_info->language == LCD_EN)
		{
			character = ((char *)str_addr)[iStrLoc];
			if(character == 0)
			{
				break;
			}

			if((character < 0x20) || character > 0x7f)
			{
				character = '_';
			}

			iStrLoc++;
		}
		else
		{
			character = ((INT16U *) str_addr)[iStrLoc];
			if(character == 0)
			{
				break;
			}

			iStrLoc++;
		}

		if(draw_type == 0)
		{
            if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
            {
                if(ap_state_resource_char_draw(character, frame_buff, str_info, RGB565_DRAW, draw_black_edge, draw_uv))
                {
                    st = STATUS_FAIL;
                    break;
                }
            }
            else if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
            {
                if(ap_state_resource_char_draw(character, frame_buff, str_info, YUV420_DRAW, draw_black_edge, draw_uv))
                {
                    st = STATUS_FAIL;
                    break;
                }
            }
            else if(drv_l2_display_tft_fmt_get() == DISP_FMT_YUYV)
            {
                if(ap_state_resource_char_draw(character, frame_buff, str_info, YUYV_DRAW, draw_black_edge, draw_uv))
                {
                    st = STATUS_FAIL;
                    break;
                }
            }
            else
            {
                st = STATUS_FAIL;
                break;
            }
        }
        else
        {
            if(ap_state_resource_char_draw(character, frame_buff, str_info, draw_type, draw_black_edge, draw_uv))
            {
                st = STATUS_FAIL;
                break;
            }
        }
	}
    return st;
}


INT32S ap_state_resource_string_draw(INT16U *frame_buff, STRING_INFO *str_info, INT8U draw_type, INT8U draw_black_edge, INT8U draw_uv)
{
	INT32U str_addr;
	INT32S st = STATUS_OK;

	str_addr = ap_state_resource_string_load(str_info->language, str_info->str_idx);
	if(!str_addr)
	{
		DBG_PRINT("Failed load string\r\n");
		return STATUS_FAIL;
	}

    st = ap_string_draw(str_addr, frame_buff, str_info, draw_type, draw_black_edge, draw_uv);
	if(st == STATUS_FAIL)
	{
		DBG_PRINT("Failed to draw char font\r\n");
	}

	gp_free((void *) str_addr);
	return st;
}


INT32S ap_state_resource_string_ascii_draw(INT16U *frame_buff, STRING_ASCII_INFO *str_ascii_info, INT8U draw_type, INT8U draw_black_edge, INT8U draw_uv)
{
	STRING_INFO str_info = { 0 };
	INT32S st = STATUS_OK;

	str_info.font_color = str_ascii_info->font_color;
	str_info.font_type = str_ascii_info->font_type;
	str_info.pos_x = str_ascii_info->pos_x;
	str_info.pos_y = str_ascii_info->pos_y;
	str_info.buff_w = str_ascii_info->buff_w;
	str_info.buff_h = str_ascii_info->buff_h;
	str_info.font_off_h = str_ascii_info->font_off_h;
	str_info.language = LCD_EN;

    ap_string_draw((INT32U)str_ascii_info->str_ptr, frame_buff, &str_info, draw_type, draw_black_edge, draw_uv);
	if(st == STATUS_FAIL)
	{
		DBG_PRINT("Failed to draw char font\r\n");
	}

    str_ascii_info->pos_x = str_info.pos_x;      //increment
	return st;
}



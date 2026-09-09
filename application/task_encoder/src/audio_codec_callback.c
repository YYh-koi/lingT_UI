#include "audio_codec_callback.h"
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
#define fs_open         open
#define fs_close        close
#define fs_read         read
#define fs_write        write

extern INT32U Audio_Decoder_Status_0;
extern INT32U Audio_Decoder_Status_1;
extern INT32U Audio_Decoder_Status_2;
extern INT32U Audio_I2S_Decoder_Status[4];

void audio_decode_end(INT32U audio_decoder_num);
void audio_i2s_decode_end(INT32U audio_decoder_num, INT32U i2s_ch);
INT32U g_user_read_data_addr=0;
INT32U global_user_read_data_addr=0;//for speech repeat 090701
INT16S g_user_read_data_fd = -1;
INT8U g_user_read_data_mode = 0;
INT32U g_audio_data_length = 0;
//m-ALIGN8 INT8U G_user_read_buffer[512*2];//081115
#if 1
INT8U G_user_read_buffer[512*2];//081115
#endif
// following are call out only when MEDIA_SOURCE.type = 0 (user-defined)
INT32S audio_encoded_data_read(INT32U buf_addr, INT32U buf_size, INT8U *data_start_addr, INT32U data_offset);
INT16S G_Snd_UserFd;
INT32S (*audio_data_write_callback)(INT8U bHeader, INT32U buffer_addr, INT32U cbLen);

void user_err(void)
{
	INT32U err_temp=1;
	while(err_temp)
		;
}

extern INT32S  g_audio_data_offset;
INT32S audio_encoded_data_read(INT32U buf_addr, INT32U buf_size, INT8U *data_start_addr, INT32U buf_offset)
{

	INT8U* offset;

	offset = data_start_addr + buf_offset;
	gp_memcpy((INT8S*)buf_addr, (INT8S*)offset, buf_size);

	return buf_size;
}
//=============================================================================
INT32S Snd_GetData(INT32U buf_adr,INT32U buf_size)
{
	//USER should fill this buf
	//Just for demo use
	return fs_read(G_Snd_UserFd, buf_adr, buf_size);
}

INT32U Snd_GetLen(void)	//read file length
{
	struct stat_t state_buffer;

 	fstat(G_Snd_UserFd, &state_buffer);
	return state_buffer.st_size;
}

//=============================================================================
//========================================================
//Function Name:	Snd_Lseek
//Syntax:		INT32S Snd_Lseek(INT32S offset,INT16S fromwhere)
//Purpose:		change data point of file
//Note:			use lseek(fd, 0, SEEK_CUR) can get current offset of file.
//Parameters:   fd, offset, fromwhere
//Return:		data point
//=======================================================
INT32S Snd_Lseek(INT32S offset,INT16S fromwhere)
{
    lseek(G_Snd_UserFd, offset, SEEK_CUR);
	lseek(G_Snd_UserFd, 0, SEEK_CUR);
	return 0;
}

//=======================================================================
//  SPU midi decode use memory manage
//=======================================================================
void* user_memory_malloc(INT32U size)
{
	return (void*)gp_malloc_align(size, 4);
}

void user_memory_free(void* buffer_addr)
{
	gp_free(buffer_addr);
}

void audio_data_write_callback_register(INT32S (*callback_function)(INT8U bHeader, INT32U buffer_addr, INT32U cbLen))
{
	audio_data_write_callback = callback_function;
	return;
}

/*******************************************************************************
This function is for audio record to write data when use user-define mode.
INT32U audio_encode_data_write(INT8U bHeader, INT32U buffer_addr, INT32U cbLen)
parameter: 	bHeader -> is file header data or not.
			buffer_addr -> buffer address
			cbLen -> buffer size by byte
return:	return the success write data size
********************************************************************************/
INT32U audio_encode_data_write(INT8U bHeader, INT32U buffer_addr, INT32U cbLen)
{
	static INT8U  status = 0;
	static INT16S fd;
	INT32U nRet;

	if(audio_data_write_callback!=NULL)
	{
		audio_data_write_callback(bHeader, buffer_addr, cbLen);
		return cbLen;
	}
	else
	{
		if(status == 0)
		{
			fd = fs_open("a:\\UserDef.wav", O_CREAT|O_WRONLY);
			status = 1;
		}

		if(bHeader == 0)
		{
			nRet = fs_write(fd, (INT32U) buffer_addr, cbLen);
		}
		else
		{
			lseek(fd, 0, SEEK_SET);
			nRet = fs_write(fd, (INT32U) buffer_addr, cbLen);
			fs_close(fd);
		}
		return nRet;
	}
}
#endif

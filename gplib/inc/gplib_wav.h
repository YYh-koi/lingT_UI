#ifndef __GPLIB_WAV_H__
#define __GPLIB_WAV_H__

#define WAVE_OUT_INTERLEAVED

#define WAV_DEC_FRAMESIZE				2048
#define WAV_DEC_BITSTREAM_BUFFER_SIZE	8192
#define WAV_DEC_MEMORY_SIZE				128//100    //for 64Byte alignment

//=======wave format tag====================
#define WAVE_FORMAT_PCM			(0x0001)
#define WAVE_FORMAT_ADPCM		(0x0002)
#define WAVE_FORMAT_ALAW		(0x0006)
#define WAVE_FORMAT_MULAW		(0x0007)
#define WAVE_FORMAT_IMA_ADPCM	(0x0011)

//========wave decoder error NO.====================
#define WAV_DEC_NO_ERROR				0
#define WAV_DEC_INBUFFER_NOT_ENOUGH		0x80000001
#define WAV_DEC_RIFF_HEADER_NOT_FOUND	0x80000002
#define WAV_DEC_HEADER_NOT_FOUND		0x80000003
#define WAV_DEC_CHUNK_ALIGN_ERROR		0x80000004
#define WAV_DEC_DATA_CHUNK_NOT_FOUND	0x80000005
#define WAV_DEC_FMT_CHUNK_TOO_SHORT		0x80000006
#define WAV_DEC_CHANNEL_ERROR			0x80000007
#define WAV_DEC_BIT_DEPTH_ERROR			0x80000008
#define WAV_DEC_CHUNK_EXTEND_ERROR		0x80000009
#define WAV_DEC_MSADPCM_COEF_ERROR		0x8000000A
#define WAV_DEC_UNKNOWN_FORMAT			0x8000000B

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   wav decoder initial
 * @param   p_workmem:    pointer to working memory
 * @param   p_bsbuf:      pointer to bit-stream buffer
 * @return 	0 success
 */
int wav_dec_init(void *p_workmem, const unsigned char *p_bsbuf);

/**
 * @brief   wav decoder parsing
 * @param   p_workmem:    	pointer to working memory
 * @param   wi:      		write index of bitstream ring buffer
 * @return 	0 success
 */
int wav_dec_parsing(void *p_workmem, int wi);

/**
 * @brief   wav decoder set parameters
 * @param   p_workmem:    	pointer to working memory
 * @param   p_WaveFormatEx: pointer to waveformat structure
 * @return 	0 success
 * @note	this function is only used for AVI
 */
int wav_dec_set_param(void *p_workmem, const unsigned char *p_WaveFormatEx);

/**
 * @brief   wav decoder set ring buffer size
 * @param   p_workmem:    	pointer to working memory
 * @param   size: 			ring buffer size
 * @return 	none
 */
void wav_dec_set_ring_buf_size(void *p_workmem, int size);

/**
 * @brief   wav decoder run
 * @param   p_workmem:    	pointer to working memory
 * @param   pcmbuf:    		pointer to PCM buffer
 * @return 	samples of PCM, 0: not enough bitstream data -1:error
 */
int wav_dec_run(void *p_workmem, short *p_pcmbuf, int wi);

/**
 * @brief   wav decoder set read index
 * @param   wave_dec_workmem:	pointer to working memory
 * @param   Readindex: 			read index
 * @return 	none
 */
void wav_dec_set_ri(void *wave_dec_workmem, int Readindex);

/**
 * @brief   wav decoder get read index
 * @param   p_workmem:		pointer to working memory
 * @return	read index
 */
int wav_dec_get_ri(void *p_workmem);

/**
 * @brief   wav decoder get working memory size
 * @param   none
 * @return	Wave Decoder Working memory size
 */
int wav_dec_get_mem_block_size(void);

/**
 * @brief   wav decoder get wave format tag
 * @param   p_workmem:		pointer to working memory
 * @return	wave format tag
 */
int wav_dec_get_wFormatTag(void *p_workmem);

/**
 * @brief   wav decoder get number of channels
 * @param   p_workmem:		pointer to working memory
 * @return	number of channels
 */
int wav_dec_get_nChannels(void *p_workmem);

/**
 * @brief   wav decoder get sample rate
 * @param   p_workmem:		pointer to working memory
 * @return	sample rate
 */
int wav_dec_get_SampleRate(void *p_workmem);

/**
 * @brief   wav decoder get average bytes per second
 * @param   p_workmem:		pointer to working memory
 * @return	average bytes per second
 */
int wav_dec_get_nAvgBytesPerSec(void *p_workmem);

/**
 * @brief   wav decoder get block align
 * @param   p_workmem:		pointer to working memory
 * @return	block align
 */
int wav_dec_get_nBlockAlign(void *p_workmem);

/**
 * @brief   wav decoder bits per sample
 * @param   p_workmem:		pointer to working memory
 * @return	bits per sample
 */
int wav_dec_get_wBitsPerSample(void *p_workmem);

#endif // __GPLIB_WAV_H__

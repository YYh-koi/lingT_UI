#ifndef __GPLIB_MM_SP_H__
#define __GPLIB_MM_SP_H__


///////////////////  User   Define   Area  //////////////////////
#define FUNCTION_NAME_LEN		32			// divided by 4 bytes

#define MM_PRINT_METHOD			0
#if (MM_PRINT_METHOD==0)
#define mm_printf DBG_PRINT
#define mm_print(argc, ...) do { mm_printf(__VA_ARGS__);} while (0)
#else
extern int mm_print(int argc, char *fmt, ...);
#endif

////////////////////////////////////////////////////////////
#define MM_STATUS_OK (0)
#define MM_STATUS_FAIL (-1)


typedef struct _mm_struct
{
    struct _mm_struct *next;			// Point to next management entry of linking list
    char name[FUNCTION_NAME_LEN];			// malloc function name
    unsigned int *end;							// Point to the last word(4 bytes) of allocated memory
} MM_STRUCT;


typedef struct __mm_type
{
	unsigned int mm_fix_64_num;
    unsigned char *mm_fix_64_flag;
    unsigned int *mm_fix_64_addr;
	unsigned int mm_fix_256_num;
    unsigned char *mm_fix_256_flag;
    unsigned int *mm_fix_256_addr;
	unsigned int mm_fix_1024_num;
    unsigned char *mm_fix_1024_flag;
    unsigned int *mm_fix_1024_addr;
	unsigned int mm_fix_2048_num;
    unsigned char *mm_fix_2048_flag;
    unsigned int *mm_fix_2048_addr;
	unsigned int mm_fix_4096_num;
    unsigned char *mm_fix_4096_flag;
    unsigned int *mm_fix_4096_addr;

	unsigned int mm_fix_total_num;
    unsigned int fix_64_alloc_count;
    unsigned int fix_256_alloc_count;
    unsigned int fix_1024_alloc_count;
    unsigned int fix_2048_alloc_count;
    unsigned int fix_4096_alloc_count;
    unsigned int fix_64_alloc_count_max;
    unsigned int fix_256_alloc_count_max;
    unsigned int fix_1024_alloc_count_max;
    unsigned int fix_2048_alloc_count_max;
    unsigned int fix_4096_alloc_count_max;

    unsigned int sdram_alloc_count;
    unsigned int sdram_alloc_count_max;

    MM_STRUCT *gp_mm_sdram_head;
    unsigned int free_sdram_start;
    unsigned int free_sdram_end;
} mm_type;


#define MEM_INFO ( sizeof(MM_STRUCT)/sizeof(struct _mm_struct *) )

extern mm_type CHUNK_MEM;
extern void sp_mm_init(mm_type *p, unsigned int free_memory_start, unsigned int free_memory_end);
extern void * sp_malloc_align(mm_type *p, unsigned int size, unsigned int align, const char *name);		// SDRAM allocation. Align value must be power of 2, eg: 2, 4, 8, 16, 32...
extern void * sp_malloc(mm_type *p, unsigned int size, const char *name);							// SDRAM allocation
extern int sp_free(mm_type *p, void *ptr);									// Both SDRAM and IRAM can be freed by this function
extern int sp_fix_MM_BYTE_NUM_set(mm_type *p, int mem_type, int num, unsigned char *flag, unsigned int *addr);
extern void sp_mm_dump(mm_type *p);


#endif 		// __GPLIB_MM_SP_H__

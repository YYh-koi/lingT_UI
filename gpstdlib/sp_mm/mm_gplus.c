/*
* Description: A simple memory management module
*
* Author: Tristan Yang
*
* Date: 2008/02/12
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
*/
#include "stddef.h"
#include "stdio.h"
#include "string.h"
#include "project.h"
#include "mm_gplus.h"


#if _OPERATING_SYSTEM != _OS_NONE	// Soft Protect for critical section
#if _OPERATING_SYSTEM == _OS_UCOS2
static OS_CPU_SR cpu_sr;
#elif _OPERATING_SYSTEM == _OS_FREERTOS
static unsigned int cpu_isr=0;
#endif
#endif

extern void print_string(CHAR *fmt, ...);
#if (WIFI_VENDOR == WIFI_VENDOR_RTK) || (WIFI_VENDOR == WIFI_VENDOR_ATBM)
unsigned int OS_EnterCritical(void)
{
    vPortEnterCritical();
    return 0;
}

void OS_ExitCritical(unsigned int val)
{
   vPortExitCritical();
   return;
}
#endif
static int SW_MM_LOCK(void)
{
#if _OPERATING_SYSTEM != _OS_NONE	// Soft Protect for critical section
#if _OPERATING_SYSTEM == _OS_UCOS2
    OS_ENTER_CRITICAL();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
    cpu_isr=OS_EnterCritical();
#endif
#endif
    return 0;
}

static void SW_MM_UNLOCK(void)
{
#if _OPERATING_SYSTEM != _OS_NONE	// Soft Protect for critical section
#if _OPERATING_SYSTEM == _OS_UCOS2
    OS_EXIT_CRITICAL();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
    OS_ExitCritical(cpu_isr);
#endif
#endif
}


static void * sp_fixed_size_malloc(mm_type *p, unsigned int size)
{
    unsigned short i, j;
    unsigned char *pFlag;
    unsigned int  *pAddr;
    unsigned int  *pAlloc_count;
    unsigned int  *pAlloc_count_max;
    unsigned int  num;

#if _OPERATING_SYSTEM != _OS_NONE
    if(SW_MM_LOCK() != MM_STATUS_OK)
    {
        return NULL;
    }
#endif

    do
    {
    	// find right segment
        if (size <= 64)
        {
            pFlag = p->mm_fix_64_flag;
            pAddr = p->mm_fix_64_addr;
            pAlloc_count = &(p->fix_64_alloc_count);
            pAlloc_count_max = &(p->fix_64_alloc_count_max);
            num = p->mm_fix_64_num;
        }
        else if (size <= 256)
        {
            pFlag = p->mm_fix_256_flag;
            pAddr = p->mm_fix_256_addr;
            pAlloc_count = &(p->fix_256_alloc_count);
            pAlloc_count_max = &(p->fix_256_alloc_count_max);
            num = p->mm_fix_256_num;
        }
        else if (size <= 1024)
        {
            pFlag = p->mm_fix_1024_flag;
            pAddr = p->mm_fix_1024_addr;
            pAlloc_count = &(p->fix_1024_alloc_count);
            pAlloc_count_max = &(p->fix_1024_alloc_count_max);
            num = p->mm_fix_1024_num;
        }
        else if (size <= 2048)
        {
            pFlag = p->mm_fix_2048_flag;
            pAddr = p->mm_fix_2048_addr;
            pAlloc_count = &(p->fix_2048_alloc_count);
            pAlloc_count_max = &(p->fix_2048_alloc_count_max);
            num = p->mm_fix_2048_num;
        }
        else if (size <= 4096)
        {
            pFlag = p->mm_fix_4096_flag;
            pAddr = p->mm_fix_4096_addr;
            pAlloc_count = &(p->fix_4096_alloc_count);
            pAlloc_count_max = &(p->fix_4096_alloc_count_max);
            num = p->mm_fix_4096_num;
        }

        // search....
        for (i=0; i<num/8; i++)
        {
            if (pFlag[i] != 0xFF)
            {
                for (j=0; j<8; j++)
                {
                    if (!(pFlag[i]&(1<<j)))
                    {
                        pFlag[i] |= (1<<j);

                        *pAlloc_count = (*pAlloc_count)+1;
                        if ( (*pAlloc_count)>(*pAlloc_count_max))
                        {
                            (*pAlloc_count_max) = (*pAlloc_count);
                        }

#if _OPERATING_SYSTEM != _OS_NONE
                        SW_MM_UNLOCK();
#endif

                        return (void *) pAddr[(i<<3) + j];
                    }
                }
            }
        }

        // find next segment
        if (size<=64)
            size = 256;
        else if (size<=256)
            size = 1024;
        else if (size<=1024)
            size = 2048;
        else if (size<=2048)
            size = 4096;
        else
            size = 4096+1;
    }
    while (size<=4096);



#if _OPERATING_SYSTEM != _OS_NONE
    SW_MM_UNLOCK();
#endif

    return NULL;

}


static int sp_fixed_size_free(mm_type *p, void *ptr)
{
    unsigned short i;
    unsigned int   idx;
    unsigned char *pFlag;
    unsigned int  *pAddr;
    unsigned int  *pAlloc_count;
    unsigned int  num;


    for (idx=0; idx<5; ++idx)
    {
        switch (idx)
        {
            case 0:
                pFlag = p->mm_fix_64_flag;
                pAddr = p->mm_fix_64_addr;
                pAlloc_count = &(p->fix_64_alloc_count);
                num = p->mm_fix_64_num;
                break;
            case 1:
                pFlag = p->mm_fix_256_flag;
                pAddr = p->mm_fix_256_addr;
                pAlloc_count = &(p->fix_256_alloc_count);
                num = p->mm_fix_256_num;
                break;
            case 2:
                pFlag = p->mm_fix_1024_flag;
                pAddr = p->mm_fix_1024_addr;
                pAlloc_count = &(p->fix_1024_alloc_count);
                num = p->mm_fix_1024_num;
                break;
            case 3:
                pFlag = p->mm_fix_2048_flag;
                pAddr = p->mm_fix_2048_addr;
                pAlloc_count = &(p->fix_2048_alloc_count);
                num = p->mm_fix_2048_num;
                break;
            case 4:
                pFlag = p->mm_fix_4096_flag;
                pAddr = p->mm_fix_4096_addr;
                pAlloc_count = &(p->fix_4096_alloc_count);
                num = p->mm_fix_4096_num;
                break;
        }
		if (num==0)
		{
			continue;
		}

        if (((unsigned int) ptr<=pAddr[num-1]) && ((unsigned int) ptr>=pAddr[0]))
        {
#if _OPERATING_SYSTEM != _OS_NONE
            if(SW_MM_LOCK() != MM_STATUS_OK)
            {
                return MM_STATUS_FAIL;
            }
#endif
            for (i=0; i<num; i++)
            {
                if ((unsigned int) ptr == pAddr[i])
                {
                    pFlag[i>>3] &= ~(1 << (i&0x7));
                    if ((*pAlloc_count)==0)
                    {
                        mm_print(0, "MM fix is 0\r\n");
                    }
                    else
                    {
                        *pAlloc_count = (*pAlloc_count)-1;
                    }
#if _OPERATING_SYSTEM != _OS_NONE
                    SW_MM_UNLOCK();
#endif

                    return MM_STATUS_FAIL;
                }
            }
#if _OPERATING_SYSTEM != _OS_NONE
            SW_MM_UNLOCK();
#endif

            return MM_STATUS_FAIL;
        }
    }

    mm_print(1, "sp_fixed_size_free can not find allocate ptr=0x%x\r\n", (unsigned int)ptr);

    return MM_STATUS_FAIL;
}



///////////////////////////////////////
//			Memory API
///////////////////////////////////////
void sp_mm_init(mm_type *p, unsigned int free_memory_start, unsigned int free_memory_end)
{
    unsigned int ptr;
    unsigned int i;
    unsigned char *pFlag;
    unsigned int  *pAddr;
    unsigned int  num;


    ptr = (free_memory_start + 3) & (~0x3);		// Align to 4-bytes boundary

    // Initate management structure for fixed size free SDRAM
    // Initiate 64-byte fixed size memory blocks
    pFlag = p->mm_fix_64_flag;
    pAddr = p->mm_fix_64_addr;
    num = p->mm_fix_64_num;
    for (i=0; i<num; i++)
    {
        pAddr[i] = ptr;
        ptr += 64;
    }
    for (i=0; i<num/8; i++)
    {
        pFlag[i] = 0x0;
    }

    // Initiate 256-byte fixed size memory blocks
    pFlag = p->mm_fix_256_flag;
    pAddr = p->mm_fix_256_addr;
    num = p->mm_fix_256_num;
    for (i=0; i<num; i++)
    {
        pAddr[i] = ptr;
        ptr += 256;
    }
    for (i=0; i<num/8; i++)
    {
        pFlag[i] = 0x0;
    }

    // Initiate 1024-byte fixed size memory blocks
    pFlag = p->mm_fix_1024_flag;
    pAddr = p->mm_fix_1024_addr;
    num = p->mm_fix_1024_num;
    for (i=0; i<num; i++)
    {
        pAddr[i] = ptr;
        ptr += 1024;
    }
    for (i=0; i<num/8; i++)
    {
        pFlag[i] = 0x0;
    }

    // Initiate 2048-byte fixed size memory blocks
    pFlag = p->mm_fix_2048_flag;
    pAddr = p->mm_fix_2048_addr;
    num = p->mm_fix_2048_num;
    for (i=0; i<num; i++)
    {
        pAddr[i] = ptr;
        ptr += 2048;
    }
    for (i=0; i<num/8; i++)
    {
        pFlag[i] = 0x0;
    }

    // Initiate 4096-byte fixed size memory blocks
    pFlag = p->mm_fix_4096_flag;
    pAddr = p->mm_fix_4096_addr;
    num = p->mm_fix_4096_num;
    for (i=0; i<num; i++)
    {
        pAddr[i] = ptr;
        ptr += 4096;
    }
    for (i=0; i<num/8; i++)
    {
        pFlag[i] = 0x0;
    }

    p->mm_fix_total_num = (p->mm_fix_64_num)+(p->mm_fix_256_num)+(p->mm_fix_1024_num)+(p->mm_fix_2048_num)+(p->mm_fix_4096_num);

    // Initate management structure for variable size free SDRAM
    p->free_sdram_start = ptr;
    p->free_sdram_end = free_memory_end & (~0x3);				// Align to 4-bytes boundary

    p->gp_mm_sdram_head = (MM_STRUCT *) (p->free_sdram_start);
    p->gp_mm_sdram_head->end = (unsigned int *) &(p->gp_mm_sdram_head->end);
    p->gp_mm_sdram_head->next = (MM_STRUCT *) (p->free_sdram_end) - 1;
    p->gp_mm_sdram_head->next->next = NULL;

    SW_MM_LOCK();
    SW_MM_UNLOCK();
}


void * sp_malloc(mm_type *p, unsigned int size, const char *name)
{
    MM_STRUCT *pmm = p->gp_mm_sdram_head;
    int flag = p->mm_fix_total_num;

    if (!size)
    {
        return (NULL);
    }

    if ( (size <= 4096)&&(flag!=0) )
    {
        unsigned int ptr;

        ptr = (unsigned int) sp_fixed_size_malloc(p, size);
        if (ptr)
        {
            return (void *) ptr;
        }
        mm_print(1, "Fix number is not enough! size=%d\r\n", size);
    }

    size = (size+3) >> 2;

#if _OPERATING_SYSTEM != _OS_NONE
    if(SW_MM_LOCK() != MM_STATUS_OK)
    {
        return NULL;
    }
#endif

    while (pmm->next)
    {
        if (((unsigned int *) pmm->next - (unsigned int *) pmm->end) > size+MEM_INFO)
        {
            ((MM_STRUCT *) (pmm->end+1))->next = pmm->next;
            pmm->next = (MM_STRUCT *) (pmm->end+1);
            pmm->next->end = (unsigned int *) &pmm->next->end + size;
            p->sdram_alloc_count++;
            if (p->sdram_alloc_count > p->sdram_alloc_count_max) p->sdram_alloc_count_max = p->sdram_alloc_count;
            {
                // keep function name
                int str_len = strlen(name);
                if (str_len >= FUNCTION_NAME_LEN)
                {
                    str_len = FUNCTION_NAME_LEN-1;
                }
                strncpy(pmm->next->name, name, str_len);
                pmm->next->name[str_len] = '\0';
            }
#if _OPERATING_SYSTEM != _OS_NONE
            SW_MM_UNLOCK();
#endif
            return ((unsigned int *) &pmm->next->end + 1);
        }

        pmm=pmm->next;
        if ((unsigned int) pmm<(unsigned int) p->free_sdram_start || (unsigned int) pmm>(unsigned int) p->free_sdram_end)
        {
            mm_print(0, "MM Error 2\r\n");
#if _OPERATING_SYSTEM != _OS_NONE
            SW_MM_UNLOCK();
#endif
            return NULL;
        }
    }
    mm_print(1, "MM fail in sp_malloc(size=%d)\r\n", size<<2);
    sp_mm_dump(p);
#if _OPERATING_SYSTEM != _OS_NONE
    SW_MM_UNLOCK();
#endif

    return (NULL);

}

void * sp_malloc_align(mm_type *p, unsigned int size, unsigned int align, const char *name)
{
    MM_STRUCT *pmm;
    unsigned int new_head_addr;
    unsigned int new_data_addr;

    if (!size)
    {
        return (NULL);
    }

    pmm = p->gp_mm_sdram_head;
    if ((align<=4) || (align & (align-1)))  				// align value must be power of 2, eg: 2, 4, 8, 16, 32...
    {
        return sp_malloc(p, size, name);
    }
    size = (size+3) >> 2;

#if _OPERATING_SYSTEM != _OS_NONE
    if(SW_MM_LOCK() != MM_STATUS_OK)
    {
        return NULL;
    }
#endif
    while (pmm->next)
    {
        new_data_addr = (((unsigned int) (pmm->end+1))+sizeof(MM_STRUCT)+(align-1)) & ~(align-1);
        if (new_data_addr >= (unsigned int) pmm->next)
        {
            pmm=pmm->next;
        }
        else if (((unsigned int *) pmm->next - (unsigned int *) new_data_addr) >= size)
        {
            new_head_addr = new_data_addr - sizeof(MM_STRUCT);
            ((MM_STRUCT *) new_head_addr)->next = pmm->next;
            ((MM_STRUCT *) new_head_addr)->end = (unsigned int *) (new_data_addr + (size<<2)) - 1;
            pmm->next = (MM_STRUCT *) new_head_addr;
            p->sdram_alloc_count++;
            if (p->sdram_alloc_count > p->sdram_alloc_count_max) p->sdram_alloc_count_max = p->sdram_alloc_count;
            {
                // keep function name
                int str_len = strlen(name);
                if (str_len >= FUNCTION_NAME_LEN)
                {
                    str_len = FUNCTION_NAME_LEN-1;
                }
                strncpy(pmm->next->name, name, str_len);
                pmm->next->name[str_len] = '\0';
            }

#if _OPERATING_SYSTEM != _OS_NONE
            SW_MM_UNLOCK();
#endif
            return ((void *) new_data_addr);
        }
        else
        {
            pmm=pmm->next;
        }
        if ((unsigned int) pmm<(unsigned int) p->free_sdram_start || (unsigned int) pmm>(unsigned int) p->free_sdram_end)
        {
            mm_print(0, "MM Error 1\r\n");
#if _OPERATING_SYSTEM != _OS_NONE
            SW_MM_UNLOCK();
#endif
            return NULL;
        }
    }
    mm_print(2, "MM fail in gp_malloc_align(size=%d align=%d)\r\n", size<<2, align);
    sp_mm_dump(p);
#if _OPERATING_SYSTEM != _OS_NONE
    SW_MM_UNLOCK();
#endif

    return (NULL);

}


int sp_free(mm_type *p, void *ptr)
{
    MM_STRUCT *pmm = p->gp_mm_sdram_head;

    if ((unsigned int) ptr < p->free_sdram_start)
    {
        sp_fixed_size_free(p, ptr);
        return MM_STATUS_OK;
    }
    else if ((unsigned int) ptr > p->free_sdram_end)
    {
        return MM_STATUS_FAIL;
    }

#if _OPERATING_SYSTEM != _OS_NONE
    if(SW_MM_LOCK() != MM_STATUS_OK)
    {
        return MM_STATUS_FAIL;
    }
#endif

    while (pmm->next)
    {
        if ((unsigned int *) pmm->next+MEM_INFO == ptr)
        {
            // Merge previous free memory with this one
            if (p->sdram_alloc_count==0)
            {
                mm_print(0, "MM variable is 0\r\n");
            }
            else
            {
                p->sdram_alloc_count--;
            }
            if ((unsigned int) pmm->next->next<(unsigned int) p->free_sdram_start || (unsigned int) pmm->next->next>(unsigned int) p->free_sdram_end)
            {
                mm_print(0, "MM Error 3\r\n");
                return MM_STATUS_FAIL;
            }
            pmm->next = pmm->next->next;
#if _OPERATING_SYSTEM != _OS_NONE
            SW_MM_UNLOCK();
#endif
            return MM_STATUS_OK;
        }
        pmm = pmm->next;
        if ((unsigned int) pmm<(unsigned int) p->free_sdram_start || (unsigned int) pmm>(unsigned int) p->free_sdram_end)
        {
#if _OPERATING_SYSTEM != _OS_NONE
            SW_MM_UNLOCK();
#endif
            mm_print(0, "MM Error 4\r\n");
            return MM_STATUS_FAIL;
        }
    }
    mm_print(1, "ptr=0x%X is not found in gp_free\r\n", (unsigned int)ptr);
    // Something must be wrong...
#if _OPERATING_SYSTEM != _OS_NONE
    SW_MM_UNLOCK();
#endif

    return MM_STATUS_FAIL;
}

int sp_fix_MM_BYTE_NUM_set(mm_type *p, int mem_type, int num, unsigned char *flag, unsigned int *addr)
{
    int ret = 0;

    if (p->free_sdram_start!=0)
    {
        mm_print(0, "sp_fix_MM_BYTE_NUM_set fail, memory has already initiated\r\n");
        return -1;
    }
    else if (num&0x7)	// dived by 8
    {
        mm_print(1, "sp_fix_MM_BYTE_NUM_set fail. num=%d must divide by 8\r\n",num);
        return -1;
    }

    switch (mem_type)
    {
        case 64:
            p->mm_fix_64_num = num;
			p->mm_fix_64_flag = flag;
			p->mm_fix_64_addr = addr;
            break;
        case 256:
            p->mm_fix_256_num = num;
			p->mm_fix_256_flag = flag;
			p->mm_fix_256_addr = addr;
            break;
        case 1024:
            p->mm_fix_1024_num = num;
			p->mm_fix_1024_flag = flag;
			p->mm_fix_1024_addr = addr;
            break;
        case 2048:
            p->mm_fix_2048_num = num;
			p->mm_fix_2048_flag = flag;
			p->mm_fix_2048_addr = addr;
            break;
        case 4096:
            p->mm_fix_4096_num = num;
			p->mm_fix_4096_flag = flag;
			p->mm_fix_4096_addr = addr;
            break;
        default:
            mm_print(1, "sp_fix_MM_BYTE_NUM_set fail. mem_type =%d error\r\n",mem_type);
    }

    return ret;
}

void sp_mm_dump(mm_type *p)
{
    MM_STRUCT *pmm;

    mm_print(0, "MM Dump\r\n");
    mm_print(3, "  Fix64 now=%d max=%d total=%d\r\n", p->fix_64_alloc_count, p->fix_64_alloc_count_max, p->mm_fix_64_num);
    mm_print(3, "  Fix256 now=%d max=%d total=%d\r\n", p->fix_256_alloc_count, p->fix_256_alloc_count_max, p->mm_fix_256_num);
    mm_print(3, "  Fix1024 now=%d max=%d total=%d\r\n", p->fix_1024_alloc_count, p->fix_1024_alloc_count_max, p->mm_fix_1024_num);
    mm_print(3, "  Fix2048 now=%d max=%d total=%d\r\n", p->fix_2048_alloc_count, p->fix_2048_alloc_count_max, p->mm_fix_2048_num);
    mm_print(3, "  Fix4096 now=%d max=%d total=%d\r\n", p->fix_4096_alloc_count, p->fix_4096_alloc_count_max, p->mm_fix_4096_num);
    mm_print(2, "  Free now=%d max=%d\r\n", p->sdram_alloc_count, p->sdram_alloc_count_max);

    pmm = p->gp_mm_sdram_head;
    while (pmm->next)
    {
        if ((unsigned int) (pmm->end) > (unsigned int) (&pmm->end))
        {
            mm_print(4, " Used 0x%06x size=%d      [%d]:%s\r\n", (unsigned int)pmm, (unsigned int) (pmm->end) - (unsigned int) (&pmm->end), strlen(pmm->name), pmm->name);
        }
        if ((unsigned int) (pmm->next) != ((unsigned int) (pmm->end)+4))
        {
            mm_print(2, " ____ 0x%06x size=%d\r\n", (unsigned int) (pmm->end)+4, (unsigned int) (pmm->next) - ((unsigned int) (pmm->end)+4));
        }
        pmm=pmm->next;
    }
}



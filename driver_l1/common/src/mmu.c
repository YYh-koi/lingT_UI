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
#include "project.h"
#include "drv_l1_arm_cp15.h"
#include <string.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	FAULT			    0x0
#define SUPERSECTION        0x01
#define SECTION             0x02
#define LARGEPAGE           0x03
#define SMALLPAGE           0x04

#define	NANA			0x0 /* AP */
#define	RWNA			0x1
#define	RWRO			0x2
#define	RWRW			0x3
#define RONA            0x5
#define RORO            0x7

#define	cb				0x0
#define	cB				0x1
#define	WT				0x2
#define	WB				0x3

#define	DOM0CLT			0x00000001  /* 0:no access,1:client,2:reserved,3:manager */
#define	DOM1CLT			0x00000004
#define	CHANGEALLDOM	0xffffffff

#define	SECION_PS		1024		//KB
#define	SUPER_SECION_PS	16318

/*CA7*/
#define LARGE_PS        64
#define SMALL_PS        4

/*CA7*/
#define L1_PTSIZE       0x1000
#define L2_PTSIZE       0x100

#define WB_OR_WT            WB

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct	{
	unsigned int vAddress;			//L2 start address
	unsigned int *ptAddress;
	unsigned int *mptAddress;
	unsigned int type;
	unsigned int domain;
}	stPagetable;

typedef struct	{
	unsigned int vAddress;
	unsigned int pagesize;
	unsigned int numpages;
	unsigned int NS;
	unsigned int nG;
	unsigned int Share; /* only apply to normal memory region */
	unsigned int TEX;
	unsigned int AP;
	unsigned int XN; /* 0:executable, 1:not-executable */
	unsigned int CB;
	unsigned int pAddress;
	stPagetable *PT;
}	stRegion;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if defined(C_RUN_ON_SPIFC) && (C_RUN_ON_SPIFC == 1)
#define L1PageTable 0xF8034000 // Note: reserve 16K byte
#define L2PageTable  0xF8038000 // Note: reserve 1K byte
#else
unsigned int __attribute__((section(".uninit"))) L1PageTable[L1_PTSIZE] __attribute__((aligned(0x4000)));
unsigned int __attribute__((section(".uninit"))) L2PageTable[L2_PTSIZE] __attribute__((aligned(0x400)));
#endif

/*	Page Table	*/
/*				              vAddress,	  ptAddress,	               mptAddress,	                type,    domain	*/
stPagetable     SectionPT =	{ 0x00000000, (unsigned int *)L1PageTable, (unsigned int *)L1PageTable, SECTION, 0 };
stPagetable	    LargePT   = { 0x03F00000, (unsigned int *)L2PageTable, (unsigned int *)L1PageTable, LARGEPAGE, 0 };


/*			              vAddress,	 	pagesize,   numpages,  NS,   nG, S, TEX,  AP,   XN, CB,       pAddress,  *PT */
stRegion OCRamRegion  =  { 0x00000000,	SECION_PS,    64,      0,    0,  0, 1,    RWRW, 0,  WB_OR_WT, 0x00000000, &SectionPT };   //64M Write-back, Write-Allocate
stRegion GICRegion    =  { 0xA2000000,  SECION_PS,    1,       0,    0,  0, 0,    RWNA, 1,  cB,       0xA2000000, &SectionPT };   //512M peripheral, strongly-orderd
stRegion DeviceRegion =  { 0xC0000000,  SECION_PS,  512,       0,    0,  0, 0,    RWNA, 1,  cB,       0xC0000000, &SectionPT };   //512M peripheral, strongly-orderd
stRegion IRam1Region  =  { 0xF8000000,  SECION_PS,    1,       0,    0,  0, 1,    RWRW, 0,  WB_OR_WT, 0xF8000000, &SectionPT };   //internal ram 1
stRegion IRam2Region  =  { 0xFA000000,  SECION_PS,    1,       0,    0,  0, 1,    RWRW, 0,  WB_OR_WT, 0xFA000000, &SectionPT };   //internal ram 2
stRegion NCacheRegion =  { 0x80000000,  SECION_PS,   64,       0,    0,  0, 1,    RWRW, 0,  cb,       0x00000000, &SectionPT };   //64M noncacheable DRAM address
stRegion SpifcRegion   = { 0x30000000,  SECION_PS,   256,      0,    0,  0, 1,    RORO, 0,  WB_OR_WT, 0x30000000, &SectionPT};    //32M for SPIFC
stRegion NCacheRegion3 = { 0x60000000,  SECION_PS,   32,       0,    0,  0, 1,    RWRW, 0,  WB_OR_WT, 0x60000000, &SectionPT };   //GP420 conv engine address
stRegion NCacheRegion4 = { 0x68000000,  SECION_PS,   32,       0,    0,  0, 1,    RWRW, 0,  WB_OR_WT, 0x68000000, &SectionPT };   //GP420 conv engine address
stRegion DummyRegion   = { 0x03F00000,  LARGE_PS,    15,       0,    0,  0, 1,    RWRW, 0,  WB_OR_WT, 0x03F00000, &LargePT};	  //960(64*15)kB cachable region
stRegion NCacheRegion5 = { 0x03FF0000,  LARGE_PS,     1,       0,    0,  0, 1,    RWRW, 0,  cb,       0x03FF0000, &LargePT};	  //64kB noncachable region

stRegion RomRegion   =   { 0xf0000000,  SECION_PS,    1,       0,    0,  0, 1,    RORO, 0,  WB_OR_WT, 0x80000000, &SectionPT};    //1M for ROM


static void MMU_Set_TTBR0(INT32U ttbr0)
{
	asm volatile (
		"mcr	p15, 0, %1, c2, c0, 0;"
		:"=r"(ttbr0)
		:"0"(ttbr0)
	);
}

static void MMU_Set_TTBCR(INT32U ttbcr)
{
	asm volatile (
		"mcr	p15, 0, %1, c2, c0, 2;"
		:"=r"(ttbcr)
		:"0"(ttbcr)
	);
}

static void MMU_Set_ASID(INT32U reg){
	asm volatile (
		"mcr	p15, 0, %1, c13, c0, 1;"
		:"=r"(reg)
		:"0"(reg)
	);
}

static void MMU_Set_DACR(INT32U dacr){
	asm volatile (
		"mcr	p15, 0, %1, c3, c0, 0;"
		:"=r"(dacr)
		:"0"(dacr)
	);
}

/*
 * @brief: MMU section mapping region
 * @param: region: Pointer of MMU section mapping region
 * @return: none
*/
void MMU_SectionRegion( stRegion *region )
{
	int i,j;
	unsigned int *piPTptr, iPTset;

	piPTptr = region->PT->ptAddress;			//Start Address of page table
	piPTptr += region->vAddress >> 20;			//The first page,use MVA[31:20] as table index
	//section first level descriptor
	if (region->pagesize == SECION_PS) {
		iPTset = region->pAddress & 0xfff00000; // use PA[31:20] as section base address
	} else { /* super section */
		iPTset = region->pAddress & 0xff000000;	// use PA[31:24] as section base address
		iPTset |= 1 << 18;
	}

	iPTset |= (region->TEX & 0x7) << 12;
	iPTset |= (region->AP & 0x3) << 10;
	iPTset |= region->PT->domain << 5;
	iPTset |= (region->CB & 0x3) << 2;
	iPTset |= 0x2; // section descriptor

	iPTset |= (region->NS & 0x1) << 19;
	iPTset |= (region->nG & 0x1) << 17;
	iPTset |= (region->Share & 0x1) << 16;
	iPTset |= (region->AP & 0x4) << 13;     /*AP[2] to bit15*/
	iPTset |= (region->XN & 0x1) << 4;

	for ( i=0; i<region->numpages; i++ ) {//entry number
		if (region->pagesize == SECION_PS) {
			*piPTptr++ = iPTset + (i << 20); //1M block
		} else {
			for (j=0;j<16;j++) {//repeat 16 times
				*piPTptr++ = iPTset + (i<<24); //16M block
			}
		}
	}
}

/*
 * @brief: MMU page table mapping region
 * @param: region: Pointer of MMU page mapping region
 * @return: none
*/
void MMU_PageTableRegion( stRegion *region )
{
	int i, j;
	unsigned int *piPTptr, iPTset;

	piPTptr = region->PT->ptAddress;

	switch ( region->pagesize ) {
		case LARGE_PS:
			piPTptr += (region->vAddress & 0x000ff000) >> 12; //use MVA[19:12] as Level 2 table index, VA[15:12] should be zero

			iPTset = region->pAddress & 0xffff0000; //use PA[31:16] as page base address
			iPTset |= (region->XN & 0x1) << 15;
			iPTset |= (region->TEX & 0x7) << 12;
			iPTset |= (region->nG & 0x1) << 11;
			iPTset |= (region->Share & 0x1) << 10;
			iPTset |= (region->AP & 0x4) << 7;     /*AP[2] to bit9*/
			iPTset |= (region->AP & 0x3) << 4;     /*AP[1:0] */
			iPTset |= (region->CB & 0x3) << 2;
			iPTset |= 0x1;    	//Set as large page


			for ( i=0; i< region->numpages; i++ ){
				for ( j=0; j<16; j++ ) {//repeat 16 times
					*piPTptr++ = iPTset + (i<<16); //64k byte per-page
                }
            }
            break;

		case SMALL_PS:
			piPTptr += (region->vAddress & 0x000ff000) >> 12;

			iPTset = region->pAddress & 0xfffff000; //use PA[31:12] as page base address
			iPTset |= (region->nG & 0x1) << 11;
			iPTset |= (region->Share & 0x1) << 10;
			iPTset |= (region->AP & 0x4) << 7;  /*AP[2] to bit9*/
			iPTset |= (region->TEX & 0x7) << 6;
            iPTset |= (region->AP & 0x3) << 4;     /*AP[1:0] */
			iPTset |= (region->CB & 0x3) << 2;
            //Set as extended small page
			iPTset |= (region->XN & 0x1);
			iPTset |= 0x2;

			for ( i=0; i<region->numpages; i++ ) {
				*piPTptr++ = iPTset + (i<<12);
            }
            break;
	}
}

/*
 * @brief: MMU mapping region
 * @param: region: Pointer of MMU mapping region table
 * @return: none
*/
void MMU_MapRegion( stRegion *region )
{
	switch ( region->PT->type ) {
		case SECTION:
			MMU_SectionRegion(region);
		break;

		case LARGEPAGE:
		case SMALLPAGE:
			MMU_PageTableRegion(region);
		break;
	}
}

/*
 * @brief: Attach MMU page table
 * @param: pt: pointer of page table.
 * @return: none
*/
void MMU_AttachPT( stPagetable *pt )
{
	unsigned int *piTtb, iPTset, iOffset;

	piTtb = pt->mptAddress;
	iOffset = (pt->vAddress) >> 20;

	switch ( pt->type ) {
		case SECTION:
			/* Use TTBR translation, with 16KB L1Table size (N=0) */
			MMU_Set_TTBCR(0);

			/* Set TTBR0 with inner/outer write back write allocate and not shareable, [4:3]=01, [1]=0, [6,0]=01 */
			MMU_Set_TTBR0(((INT32U)piTtb&0xFFFFC000UL)| 0x9UL);
			MMU_Set_ASID(0);
            break;

		case LARGEPAGE:
		case SMALLPAGE:
			iPTset = (int)(pt->ptAddress) & 0xfffffc00;
			iPTset |= pt->domain << 5;
			iPTset |= 0x01;
			piTtb[iOffset] = iPTset;
            break;

	}
}

/*
 * @brief:MMU page table clear
 * @param: none
 * @return: none
*/
void MMU_PTClear( stPagetable *pt )
{
	int index = 0;
	unsigned int *piPTptr, iPTset;
	unsigned int temp = 0;

	piPTptr = pt->ptAddress;
	iPTset = FAULT;


	switch ( pt->type ) {
		case LARGEPAGE:
		case SMALLPAGE:
            index = L2_PTSIZE;
		break;

		case SUPERSECTION:
		case SECTION:
            index = L1_PTSIZE;
		break;
	}

	temp = iPTset;

	for (; index!=0; index--) {
		*piPTptr++ = temp;
	}

    asm("	NOP	");
}

/*
 * @brief:MMU init, L1Table must be 16KB aligned (bit [13:0] are all 0) with size 16KB
 * @param: L1Table: Level 1 table. L1Table must be 16KB aligned (bit [13:0] are all 0) with size 16KB
 * @return: none
*/
void MMU_Init(uint32_t *L1Table)
{
  uint32_t L1Base = (uint32_t)L1Table;

  /* Use TTBR translation, with 16KB L1Table size (N=0) */
  MMU_Set_TTBCR(0);

  /* Set TTBR0 with inner/outer write back write allocate and not shareable, [4:3]=01, [1]=0, [6,0]=01 */
  MMU_Set_TTBR0((L1Base & 0xFFFFC000UL) | 0x9UL);

  /* Set all domains to client */
  MMU_Set_DACR(0x55555555UL);

  /* Set PROCID and ASID to 0 */
  __MCR(15, 0, 0, 13, 0, 1);

  /* Set all virtual space to invalid */
  memset((void *)L1Table, 0, 4096*4);
}

/*
 * @brief:MMU page table create
 * @param: none
 * @return: none
*/
void MMU_PTCreate(void)
{
    MMU_Init((uint32_t *)L1PageTable);
    MMU_PTClear(&SectionPT);
    MMU_PTClear(&LargePT);

    MMU_MapRegion(&OCRamRegion);    /*On Chip Ram*/
    MMU_MapRegion(&GICRegion);      /*GIC*/
    MMU_MapRegion(&DeviceRegion);   /*Device*/
    MMU_MapRegion(&IRam1Region);    /*Internal ram 1*/
    MMU_MapRegion(&IRam2Region);    /*Internal ram 2*/
    MMU_MapRegion(&NCacheRegion);

    MMU_MapRegion(&NCacheRegion3);
    MMU_MapRegion(&NCacheRegion4);
    MMU_MapRegion(&DummyRegion);
    MMU_MapRegion(&NCacheRegion5);
    MMU_MapRegion(&SpifcRegion);
    MMU_MapRegion(&RomRegion);

    MMU_AttachPT(&SectionPT);
    MMU_AttachPT(&LargePT);

	drv_l1_cp15_mmu_domain_access_set( DOM0CLT, CHANGEALLDOM);
}

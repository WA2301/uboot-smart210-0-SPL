#include <common.h>
#include <s5pc110.h>

#include <asm/io.h>


#define MAGIC_NUMBER_MOVI       (0x24564236)

#define SS_SIZE                 (8 * 1024)

#if defined(CONFIG_EVT1)
#define eFUSE_SIZE              (1 * 512)       // 512 Byte eFuse, 512 Byte reserved
#else
#define eFUSE_SIZE              (1 * 1024)      // 1 kB eFuse, 1 KB reserved
#endif /* CONFIG_EVT1 */


#define MOVI_BLKSIZE            (1<<9) /* 512 bytes */

/* partition information */
#define PART_SIZE_BL            (512 * 1024)
#define PART_SIZE_KERNEL        (4 * 1024 * 1024)
#define PART_SIZE_ROOTFS        (26 * 1024 * 1024)

//#define MOVI_LAST_BLKPOS        (MOVI_TOTAL_BLKCNT - (eFUSE_SIZE / MOVI_BLKSIZE))

/* Add block count at fused chip */
// #if defined(CONFIG_SECURE) || defined(CONFIG_FUSED)
// #define MOVI_FWBL1_BLKCNT       (FWBL1_SIZE / MOVI_BLKSIZE)     /* 4KB */
// #endif

#define MOVI_BL1_BLKCNT         (SS_SIZE / MOVI_BLKSIZE)        /* 8KB */
#define MOVI_ENV_BLKCNT         (CONFIG_ENV_SIZE / MOVI_BLKSIZE)   /* 16KB */
#define MOVI_BL2_BLKCNT         (PART_SIZE_BL / MOVI_BLKSIZE)   /* 512KB */
#define MOVI_ZIMAGE_BLKCNT      (PART_SIZE_KERNEL / MOVI_BLKSIZE)       /* 4MB */

/* Change writing block position at fused chip */
#if defined(CONFIG_EVT1)
        #if defined(CONFIG_SECURE) || defined(CONFIG_FUSED)
#define MOVI_BL2_POS            ((eFUSE_SIZE / MOVI_BLKSIZE) + (FWBL1_SIZE / MOVI_BLKSIZE) + MOVI_BL1_BLKCNT + MOVI_ENV_BLKCNT)
        #else
#define MOVI_BL2_POS            ((eFUSE_SIZE / MOVI_BLKSIZE) + MOVI_BL1_BLKCNT + MOVI_ENV_BLKCNT)
        #endif
#else
//#define MOVI_BL2_POS            (MOVI_LAST_BLKPOS - MOVI_BL1_BLKCNT - MOVI_BL2_BLKCNT - MOVI_ENV_BLKCNT)
#endif





typedef u32(*copy_sd_mmc_to_mem)
(u32 channel, u32 start_block, u16 block_size, u32 *trg, u32 init);

//WA2301: WTF,so much magic number
void copy_uboot_to_ram(void)
{
	ulong ch;
#if defined(CONFIG_EVT1)
	ch = *(volatile u32 *)(0xD0037488);				//WA2301: V210_SDMMC_BASE
	copy_sd_mmc_to_mem copy_bl2 =
	    (copy_sd_mmc_to_mem) (*(u32 *) (0xD0037F98));//WA2301: CopySDMMCtoMem[0xD0037F98] by Samsung 

#endif
	u32 ret;
	if (ch == 0xEB000000) {
		ret = copy_bl2(0, MOVI_BL2_POS, MOVI_BL2_BLKCNT,
			CONFIG_SYS_TEXT_BASE, 0);

	}
	else if (ch == 0xEB200000) {
		ret = copy_bl2(2, MOVI_BL2_POS, MOVI_BL2_BLKCNT,
			CONFIG_SYS_TEXT_BASE, 0);

	}
	else
		return;

	if (ret == 0)
		while (1)
			;
	else
		return;
}



#define 	GPJ2CON 	(*(volatile unsigned long *) 0xE0200280)
#define 	GPJ2DAT		(*(volatile unsigned long *) 0xE0200284)

//WA2301: This 'board_init_f' is in u-boot-spl.bin
void board_init_f(unsigned long bootflag)
{
	__attribute__((noreturn)) void (*uboot)(void);
	
	copy_uboot_to_ram();

	GPJ2CON = 0x00001111;			// 配置引脚
	GPJ2DAT = 0x0E;					// LED1 On-0  (Off-1)

	/* Jump to U-Boot image */
	uboot = (void *)CONFIG_SYS_TEXT_BASE; //WA2301: Go to uboot...

	(*uboot)();
	/* Never returns Here */
}
/* Place Holders */
void board_init_r(gd_t *id, ulong dest_addr)
{
        /* Function attribute is no-return */
        /* This Function never executes */
        while (1)
                ;
}

void save_boot_params(u32 r0, u32 r1, u32 r2, u32 r3) {}

void board_init_f_nand(unsigned long bootflag){}

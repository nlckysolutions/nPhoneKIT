#ifndef  MT_SD_H
#define  MT_SD_H

#include "types.h"
#include "sync_write.h"
#include "read.h"

//#define MSDC_DMA_ADDR_DEBUG
//ccyeh #define MSDC_HQA

#define MTK_MSDC_USE_CMD23
#if defined(CONFIG_MTK_EMMC_CACHE) && defined(MTK_MSDC_USE_CMD23)
#define MTK_MSDC_USE_CACHE
#define MTK_MSDC_USE_EDC_EMMC_CACHE (1)
#else
#define MTK_MSDC_USE_EDC_EMMC_CACHE (0)
#endif

#ifdef MTK_MSDC_USE_CMD23
#define MSDC_USE_AUTO_CMD23   (1)
#endif

#ifdef MTK_MSDC_USE_CACHE
#ifndef MMC_ENABLED_EMPTY_QUEUE_FLUSH
/* #define MTK_MSDC_FLUSH_BY_CLK_GATE */
#endif
#endif

#define HOST_MAX_NUM        			 (4)
#define MAX_REQ_SZ                       (512 * 1024)

#define CMD_TUNE_UHS_MAX_TIME            (2*32*8*8)
#define CMD_TUNE_HS_MAX_TIME             (2*32)

#define READ_TUNE_UHS_CLKMOD1_MAX_TIME   (2*32*32*8)
#define READ_TUNE_UHS_MAX_TIME           (2*32*32)
#define READ_TUNE_HS_MAX_TIME            (2*32)

#define WRITE_TUNE_HS_MAX_TIME           (2*32)
#define WRITE_TUNE_UHS_MAX_TIME          (2*32*8)

#ifdef CONFIG_EMMC_50_FEATURE
#define MAX_HS400_TUNE_COUNT (576) //(32*18)
#endif

#define MAX_GPD_NUM         (1 + 1)	/* one null gpd */
#define MAX_BD_NUM          (1024)
#define MAX_BD_PER_GPD      (MAX_BD_NUM)
#define CLK_SRC_MAX_NUM        (1)

#define EINT_MSDC1_INS_POLARITY         (0)
#define SDIO_ERROR_BYPASS

//#define MSDC_CLKSRC_REG     (0xf100000C)
#define MSDC_DESENSE_REG    (0xf0007070)

#ifdef CONFIG_SDIOAUTOK_SUPPORT
#define MTK_SDIO30_ONLINE_TUNING_SUPPORT
//#define OT_LATENCY_TEST
#endif  // CONFIG_SDIOAUTOK_SUPPORT
//#define ONLINE_TUNING_DVTTEST

#ifdef CONFIG_FPGA_EARLY_PORTING
#define FPGA_PLATFORM
#else
#define MTK_MSDC_BRINGUP_DEBUG
#endif
//#define MTK_MSDC_DUMP_FIFO

#define CMD_SET_FOR_MMC_TUNE_CASE1 (0x00000000FB260140ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE2 (0x0000000000000080ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE3 (0x0000000000001000ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE4 (0x0000000000000020ULL)
//#define CMD_SET_FOR_MMC_TUNE_CASE5 (0x0000000000084000ULL)


#define CMD_SET_FOR_SD_TUNE_CASE1  (0x000000007B060040ULL)
#define CMD_SET_FOR_APP_TUNE_CASE1 (0x0008000000402000ULL)

#define IS_IN_CMD_SET(cmd_num, set) ((0x1ULL << cmd_num) & (set))

#define MSDC_VERIFY_NEED_TUNE     (0)
#define MSDC_VERIFY_ERROR         (1)
#define MSDC_VERIFY_NEED_NOT_TUNE (2)

/*--------------------------------------------------------------------------*/
/* Common Macro                                                             */
/*--------------------------------------------------------------------------*/
#define REG_ADDR(x)                 ((volatile u32*)(0x11230000 + OFFSET_##x))

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD           (64)	/* (128) */
#define MSDC_NUM                (4)

#define MSDC_MS                 (0)	/* No memory stick mode, 0 use to gate clock */
#define MSDC_SDMMC              (1)

#define MSDC_MODE_UNKNOWN       (0)
#define MSDC_MODE_PIO           (1)
#define MSDC_MODE_DMA_BASIC     (2)
#define MSDC_MODE_DMA_DESC      (3)
#define MSDC_MODE_DMA_ENHANCED  (4)
#define MSDC_MODE_MMC_STREAM    (5)

#define MSDC_BUS_1BITS          (0)
#define MSDC_BUS_4BITS          (1)
#define MSDC_BUS_8BITS          (2)

#define MSDC_BRUST_8B           (3)
#define MSDC_BRUST_16B          (4)
#define MSDC_BRUST_32B          (5)
#define MSDC_BRUST_64B          (6)

#define MSDC_PIN_PULL_NONE      (0)
#define MSDC_PIN_PULL_DOWN      (1)
#define MSDC_PIN_PULL_UP        (2)
#define MSDC_PIN_KEEP           (3)

#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0003)
#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
#define MSDC_AUTOCMD53          (0x0004)
#endif	// #if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)

#define MSDC_EMMC_BOOTMODE0     (0)	/* Pull low CMD mode */
#define MSDC_EMMC_BOOTMODE1     (1)	/* Reset CMD mode */

enum {
	RESP_NONE = 0,
	RESP_R1,
	RESP_R2,
	RESP_R3,
	RESP_R4,
	RESP_R5,
	RESP_R6,
	RESP_R7,
	RESP_R1B
};

/*--------------------------------------------------------------------------*/
/* Register Offset                                                          */
/*--------------------------------------------------------------------------*/
#define OFFSET_MSDC_CFG                  (0x0)
#define OFFSET_MSDC_IOCON                (0x04)
#define OFFSET_MSDC_PS                   (0x08)
#define OFFSET_MSDC_INT                  (0x0c)
#define OFFSET_MSDC_INTEN                (0x10)
#define OFFSET_MSDC_FIFOCS               (0x14)
#define OFFSET_MSDC_TXDATA               (0x18)
#define OFFSET_MSDC_RXDATA               (0x1c)
#define OFFSET_SDC_CFG                   (0x30)
#define OFFSET_SDC_CMD                   (0x34)
#define OFFSET_SDC_ARG                   (0x38)
#define OFFSET_SDC_STS                   (0x3c)
#define OFFSET_SDC_RESP0                 (0x40)
#define OFFSET_SDC_RESP1                 (0x44)
#define OFFSET_SDC_RESP2                 (0x48)
#define OFFSET_SDC_RESP3                 (0x4c)
#define OFFSET_SDC_BLK_NUM               (0x50)
#define OFFSET_SDC_VOL_CHG               (0x54)
#define OFFSET_SDC_CSTS                  (0x58)
#define OFFSET_SDC_CSTS_EN               (0x5c)
#define OFFSET_SDC_DCRC_STS              (0x60)
#define OFFSET_EMMC_CFG0                 (0x70)
#define OFFSET_EMMC_CFG1                 (0x74)
#define OFFSET_EMMC_STS                  (0x78)
#define OFFSET_EMMC_IOCON                (0x7c)
#define OFFSET_SDC_ACMD_RESP             (0x80)
#define OFFSET_SDC_ACMD19_TRG            (0x84)
#define OFFSET_SDC_ACMD19_STS            (0x88)
#define OFFSET_MSDC_DMA_SA_HIGH4BIT      (0x8C)
#define OFFSET_MSDC_DMA_SA               (0x90)
#define OFFSET_MSDC_DMA_CA               (0x94)
#define OFFSET_MSDC_DMA_CTRL             (0x98)
#define OFFSET_MSDC_DMA_CFG              (0x9c)
#define OFFSET_MSDC_DBG_SEL              (0xa0)
#define OFFSET_MSDC_DBG_OUT              (0xa4)
#define OFFSET_MSDC_DMA_LEN              (0xa8)
#define OFFSET_MSDC_PATCH_BIT0           (0xb0)
#define OFFSET_MSDC_PATCH_BIT1           (0xb4)
#define OFFSET_MSDC_PATCH_BIT2           (0xb8)
#define OFFSET_DAT0_TUNE_CRC             (0xc0)
#define OFFSET_DAT1_TUNE_CRC             (0xc4)
#define OFFSET_DAT2_TUNE_CRC             (0xc8)
#define OFFSET_DAT3_TUNE_CRC             (0xcc)
#define OFFSET_CMD_TUNE_CRC              (0xd0)
#define OFFSET_SDIO_TUNE_WIND            (0xd4)
#define OFFSET_MSDC_PAD_TUNE0            (0xf0)
#define OFFSET_MSDC_PAD_TUNE1            (0xf4)
#define OFFSET_MSDC_DAT_RDDLY0           (0xf8)
#define OFFSET_MSDC_DAT_RDDLY1           (0xfc)
#define OFFSET_MSDC_DAT_RDDLY2           (0x100)
#define OFFSET_MSDC_DAT_RDDLY3           (0x104)
#define OFFSET_MSDC_HW_DBG               (0x110)
#define OFFSET_MSDC_VERSION              (0x114)
#define OFFSET_MSDC_ECO_VER              (0x118)

#define OFFSET_EMMC50_PAD_CTL0           (0x180)
#define OFFSET_EMMC50_PAD_DS_CTL0        (0x184)
#define OFFSET_EMMC50_PAD_DS_TUNE        (0x188)
#define OFFSET_EMMC50_PAD_CMD_TUNE       (0x18c)
#define OFFSET_EMMC50_PAD_DAT01_TUNE     (0x190)
#define OFFSET_EMMC50_PAD_DAT23_TUNE     (0x194)
#define OFFSET_EMMC50_PAD_DAT45_TUNE     (0x198)
#define OFFSET_EMMC50_PAD_DAT67_TUNE     (0x19c)
#define OFFSET_EMMC51_CFG0               (0x204)
#define OFFSET_EMMC50_CFG0               (0x208)
#define OFFSET_EMMC50_CFG1               (0x20c)
#define OFFSET_EMMC50_CFG2               (0x21c)
#define OFFSET_EMMC50_CFG3               (0x220)
#define OFFSET_EMMC50_CFG4               (0x224)


/*--------------------------------------------------------------------------*/
/* Register Address                                                         */
/*--------------------------------------------------------------------------*/

/* common register */
#define MSDC_CFG                    REG_ADDR(MSDC_CFG)
#define MSDC_IOCON                  REG_ADDR(MSDC_IOCON)
#define MSDC_PS                     REG_ADDR(MSDC_PS)
#define MSDC_INT                    REG_ADDR(MSDC_INT)
#define MSDC_INTEN                  REG_ADDR(MSDC_INTEN)
#define MSDC_FIFOCS                 REG_ADDR(MSDC_FIFOCS)
#define MSDC_TXDATA                 REG_ADDR(MSDC_TXDATA)
#define MSDC_RXDATA                 REG_ADDR(MSDC_RXDATA)

/* sdmmc register */
#define SDC_CFG                     REG_ADDR(SDC_CFG)
#define SDC_CMD                     REG_ADDR(SDC_CMD)
#define SDC_ARG                     REG_ADDR(SDC_ARG)
#define SDC_STS                     REG_ADDR(SDC_STS)
#define SDC_RESP0                   REG_ADDR(SDC_RESP0)
#define SDC_RESP1                   REG_ADDR(SDC_RESP1)
#define SDC_RESP2                   REG_ADDR(SDC_RESP2)
#define SDC_RESP3                   REG_ADDR(SDC_RESP3)
#define SDC_BLK_NUM                 REG_ADDR(SDC_BLK_NUM)
#define SDC_VOL_CHG                 REG_ADDR(SDC_VOL_CHG)
#define SDC_CSTS                    REG_ADDR(SDC_CSTS)
#define SDC_CSTS_EN                 REG_ADDR(SDC_CSTS_EN)
#define SDC_DCRC_STS                REG_ADDR(SDC_DCRC_STS)

/* emmc register*/
#define EMMC_CFG0                   REG_ADDR(EMMC_CFG0)
#define EMMC_CFG1                   REG_ADDR(EMMC_CFG1)
#define EMMC_STS                    REG_ADDR(EMMC_STS)
#define EMMC_IOCON                  REG_ADDR(EMMC_IOCON)

/* auto command register */
#define SDC_ACMD_RESP               REG_ADDR(SDC_ACMD_RESP)
#define SDC_ACMD19_TRG              REG_ADDR(SDC_ACMD19_TRG)
#define SDC_ACMD19_STS              REG_ADDR(SDC_ACMD19_STS)

/* dma register */
#define MSDC_DMA_SA_HIGH4BIT        REG_ADDR(MSDC_DMA_SA_HIGH4BIT)
#define MSDC_DMA_SA                 REG_ADDR(MSDC_DMA_SA)
#define MSDC_DMA_CA                 REG_ADDR(MSDC_DMA_CA)
#define MSDC_DMA_CTRL               REG_ADDR(MSDC_DMA_CTRL)
#define MSDC_DMA_CFG                REG_ADDR(MSDC_DMA_CFG)
#define MSDC_DMA_LEN                REG_ADDR(MSDC_DMA_LEN)

/* data read delay */
#define MSDC_DAT_RDDLY0             REG_ADDR(MSDC_DAT_RDDLY0)
#define MSDC_DAT_RDDLY1             REG_ADDR(MSDC_DAT_RDDLY1)
#define MSDC_DAT_RDDLY2             REG_ADDR(MSDC_DAT_RDDLY2)
#define MSDC_DAT_RDDLY3             REG_ADDR(MSDC_DAT_RDDLY3)

/* debug register */
#define MSDC_DBG_SEL                REG_ADDR(MSDC_DBG_SEL)
#define MSDC_DBG_OUT                REG_ADDR(MSDC_DBG_OUT)

/* misc register */
#define MSDC_PATCH_BIT0             REG_ADDR(MSDC_PATCH_BIT0)
#define MSDC_PATCH_BIT1             REG_ADDR(MSDC_PATCH_BIT1)
#define MSDC_PATCH_BIT2             REG_ADDR(MSDC_PATCH_BIT2)
#define DAT0_TUNE_CRC               REG_ADDR(DAT0_TUNE_CRC)
#define DAT1_TUNE_CRC               REG_ADDR(DAT1_TUNE_CRC)
#define DAT2_TUNE_CRC               REG_ADDR(DAT2_TUNE_CRC)
#define DAT3_TUNE_CRC               REG_ADDR(DAT3_TUNE_CRC)
#define CMD_TUNE_CRC                REG_ADDR(CMD_TUNE_CRC)
#define SDIO_TUNE_WIND              REG_ADDR(SDIO_TUNE_WIND)
#define MSDC_PAD_TUNE0              REG_ADDR(MSDC_PAD_TUNE0)
#define MSDC_PAD_TUNE1              REG_ADDR(MSDC_PAD_TUNE1)
#define MSDC_HW_DBG                 REG_ADDR(MSDC_HW_DBG)
#define MSDC_VERSION                REG_ADDR(MSDC_VERSION)
#define MSDC_ECO_VER                REG_ADDR(MSDC_ECO_VER)

/* eMMC 5.0 register */
#define EMMC50_PAD_CTL0             REG_ADDR(EMMC50_PAD_CTL0)
#define EMMC50_PAD_DS_CTL0          REG_ADDR(EMMC50_PAD_DS_CTL0)
#define EMMC50_PAD_DS_TUNE          REG_ADDR(EMMC50_PAD_DS_TUNE)
#define EMMC50_PAD_CMD_TUNE         REG_ADDR(EMMC50_PAD_CMD_TUNE)
#define EMMC50_PAD_DAT01_TUNE       REG_ADDR(EMMC50_PAD_DAT01_TUNE)
#define EMMC50_PAD_DAT23_TUNE       REG_ADDR(EMMC50_PAD_DAT23_TUNE)
#define EMMC50_PAD_DAT45_TUNE       REG_ADDR(EMMC50_PAD_DAT45_TUNE)
#define EMMC50_PAD_DAT67_TUNE       REG_ADDR(EMMC50_PAD_DAT67_TUNE)
#define EMMC51_CFG0                 REG_ADDR(EMMC51_CFG0)
#define EMMC50_CFG0                 REG_ADDR(EMMC50_CFG0)
#define EMMC50_CFG1                 REG_ADDR(EMMC50_CFG1)
#define EMMC50_CFG2                 REG_ADDR(EMMC50_CFG2)
#define EMMC50_CFG3                 REG_ADDR(EMMC50_CFG3)
#define EMMC50_CFG4                 REG_ADDR(EMMC50_CFG4)


/*--------------------------------------------------------------------------*/
/* Register Mask                                                            */
/*--------------------------------------------------------------------------*/

/* MSDC_CFG mask */
#define MSDC_CFG_MODE           (0x1  << 0)     /* RW */
#define MSDC_CFG_CKPDN          (0x1  << 1)     /* RW */
#define MSDC_CFG_RST            (0x1  << 2)     /* A0 */
#define MSDC_CFG_PIO            (0x1  << 3)     /* RW */
#define MSDC_CFG_CKDRVEN        (0x1  << 4)     /* RW */
#define MSDC_CFG_BV18SDT        (0x1  << 5)     /* RW */
#define MSDC_CFG_BV18PSS        (0x1  << 6)     /* R  */
#define MSDC_CFG_CKSTB          (0x1  << 7)     /* R  */
#define MSDC_CFG_CKDIV          (0xfff<< 8)     /* RW */
#define MSDC_CFG_CKMOD          (0x3  << 20)    /* W1C */
#define MSDC_CFG_CKMOD_HS400    (0x1  << 22)    /* RW */
#define MSDC_CFG_START_BIT      (0x3  << 23)    /* RW */
#define MSDC_CFG_SCLK_STOP_DDR  (0x1  << 25)    /* RW */


/* MSDC_IOCON mask */
#define MSDC_IOCON_SDR104CKS    (0x1  << 0)     /* RW */
#define MSDC_IOCON_RSPL         (0x1  << 1)     /* RW */
#define MSDC_IOCON_R_D_SMPL     (0x1  << 2)     /* RW */
#define MSDC_IOCON_DDLSEL       (0x1  << 3)     /* RW */
#define MSDC_IOCON_DDR50CKD     (0x1  << 4)     /* RW */
#define MSDC_IOCON_R_D_SMPL_SEL (0x1  << 5)     /* RW */
#define MSDC_IOCON_W_D_SMPL     (0x1  << 8)     /* RW */
#define MSDC_IOCON_W_D_SMPL_SEL (0x1  << 9)     /* RW */
#define MSDC_IOCON_W_D0SPL      (0x1  << 10)    /* RW */
#define MSDC_IOCON_W_D1SPL      (0x1  << 11)    /* RW */
#define MSDC_IOCON_W_D2SPL      (0x1  << 12)    /* RW */
#define MSDC_IOCON_W_D3SPL      (0x1  << 13)    /* RW */
#define MSDC_IOCON_R_D0SPL      (0x1  << 16)    /* RW */
#define MSDC_IOCON_R_D1SPL      (0x1  << 17)    /* RW */
#define MSDC_IOCON_R_D2SPL      (0x1  << 18)    /* RW */
#define MSDC_IOCON_R_D3SPL      (0x1  << 19)    /* RW */
#define MSDC_IOCON_R_D4SPL      (0x1  << 20)    /* RW */
#define MSDC_IOCON_R_D5SPL      (0x1  << 21)    /* RW */
#define MSDC_IOCON_R_D6SPL      (0x1  << 22)    /* RW */
#define MSDC_IOCON_R_D7SPL      (0x1  << 23)    /* RW */

/* MSDC_PS mask */
#define MSDC_PS_CDEN            (0x1  << 0)     /* RW */
#define MSDC_PS_CDSTS           (0x1  << 1)     /* R  */
#define MSDC_PS_CDDEBOUNCE      (0xf  << 12)    /* RW */
#define MSDC_PS_DAT             (0xff << 16)    /* R  */
#define MSDC_PS_CMD             (0x1  << 24)    /* R  */
#define MSDC_PS_WP              (0x1UL<< 31)    /* R  */

/* MSDC_INT mask */
#define MSDC_INT_MMCIRQ         (0x1  << 0)     /* W1C */
#define MSDC_INT_CDSC           (0x1  << 1)     /* W1C */
#define MSDC_INT_ACMDRDY        (0x1  << 3)     /* W1C */
#define MSDC_INT_ACMDTMO        (0x1  << 4)     /* W1C */
#define MSDC_INT_ACMDCRCERR     (0x1  << 5)     /* W1C */
#define MSDC_INT_DMAQ_EMPTY     (0x1  << 6)     /* W1C */
#define MSDC_INT_SDIOIRQ        (0x1  << 7)     /* W1C */
#define MSDC_INT_CMDRDY         (0x1  << 8)     /* W1C */
#define MSDC_INT_CMDTMO         (0x1  << 9)     /* W1C */
#define MSDC_INT_RSPCRCERR      (0x1  << 10)    /* W1C */
#define MSDC_INT_CSTA           (0x1  << 11)    /* R   */
#define MSDC_INT_XFER_COMPL     (0x1  << 12)    /* W1C */
#define MSDC_INT_DXFER_DONE     (0x1  << 13)    /* W1C */
#define MSDC_INT_DATTMO         (0x1  << 14)    /* W1C */
#define MSDC_INT_DATCRCERR      (0x1  << 15)    /* W1C */
#define MSDC_INT_ACMD19_DONE    (0x1  << 16)    /* W1C */
#define MSDC_INT_BDCSERR        (0x1  << 17)    /* W1C */
#define MSDC_INT_GPDCSERR       (0x1  << 18)    /* W1C */
#define MSDC_INT_DMAPRO         (0x1  << 19)    /* W1C */
#define MSDC_INT_AXI_RESP_ERR   (0x1  << 23)    /* W1C */
#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
#define MSDC_INT_GEAR_OUT_BOUND (0x1  << 20)    /* W1C */
#define MSDC_INT_ACMD53_DONE    (0x1  << 21)    /* W1C */
#define MSDC_INT_ACMD53_FAIL    (0x1  << 22)    /* W1C */
#endif	// #if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)

/* MSDC_INTEN mask */
#define MSDC_INTEN_MMCIRQ       (0x1  << 0)     /* RW */
#define MSDC_INTEN_CDSC         (0x1  << 1)     /* RW */
#define MSDC_INTEN_ACMDRDY      (0x1  << 3)     /* RW */
#define MSDC_INTEN_ACMDTMO      (0x1  << 4)     /* RW */
#define MSDC_INTEN_ACMDCRCERR   (0x1  << 5)     /* RW */
#define MSDC_INTEN_DMAQ_EMPTY   (0x1  << 6)     /* RW */
#define MSDC_INTEN_SDIOIRQ      (0x1  << 7)     /* RW */
#define MSDC_INTEN_CMDRDY       (0x1  << 8)     /* RW */
#define MSDC_INTEN_CMDTMO       (0x1  << 9)     /* RW */
#define MSDC_INTEN_RSPCRCERR    (0x1  << 10)    /* RW */
#define MSDC_INTEN_CSTA         (0x1  << 11)    /* RW */
#define MSDC_INTEN_XFER_COMPL   (0x1  << 12)    /* RW */
#define MSDC_INTEN_DXFER_DONE   (0x1  << 13)    /* RW */
#define MSDC_INTEN_DATTMO       (0x1  << 14)    /* RW */
#define MSDC_INTEN_DATCRCERR    (0x1  << 15)    /* RW */
#define MSDC_INTEN_ACMD19_DONE  (0x1  << 16)    /* RW */
#define MSDC_INTEN_BDCSERR      (0x1  << 17)    /* RW */
#define MSDC_INTEN_GPDCSERR     (0x1  << 18)    /* RW */
#define MSDC_INTEN_DMAPRO       (0x1  << 19)    /* RW */
#define MSDC_INTEN_GOBOUND      (0x1  << 20)    /* RW */
#define MSDC_INTEN_ACMD53_DONE  (0x1  << 21)    /* RW */
#define MSDC_INTEN_ACMD53_FAIL  (0x1  << 22)    /* RW */
#define MSDC_INTEN_AXI_RESP_ERR (0x1  << 23)    /* RW */


/* MSDC_FIFOCS mask */
#define MSDC_FIFOCS_RXCNT       (0xff << 0)     /* R  */
#define MSDC_FIFOCS_TXCNT       (0xff << 16)    /* R  */
#define MSDC_FIFOCS_CLR         (0x1UL<< 31)    /* RW */

/* SDC_CFG mask */
#define SDC_CFG_SDIOINTWKUP     (0x1  << 0)     /* RW */
#define SDC_CFG_INSWKUP         (0x1  << 1)     /* RW */
#define SDC_CFG_BUSWIDTH        (0x3  << 16)    /* RW */
#define SDC_CFG_SDIO            (0x1  << 19)    /* RW */
#define SDC_CFG_SDIOIDE         (0x1  << 20)    /* RW */
#define SDC_CFG_INTATGAP        (0x1  << 21)    /* RW */
#define SDC_CFG_DTOC            (0xffUL << 24)  /* RW */

/* SDC_CMD mask */
#define SDC_CMD_OPC             (0x3f << 0)     /* RW */
#define SDC_CMD_BRK             (0x1  << 6)     /* RW */
#define SDC_CMD_RSPTYP          (0x7  << 7)     /* RW */
#define SDC_CMD_DTYP            (0x3  << 11)    /* RW */
#define SDC_CMD_RW              (0x1  << 13)    /* RW */
#define SDC_CMD_STOP            (0x1  << 14)    /* RW */
#define SDC_CMD_GOIRQ           (0x1  << 15)    /* RW */
#define SDC_CMD_BLKLEN          (0xfff<< 16)    /* RW */
#define SDC_CMD_AUTOCMD         (0x3  << 28)    /* RW */
#define SDC_CMD_VOLSWTH         (0x1  << 30)    /* RW */
#define SDC_CMD_ACMD53          (0x1UL  << 31)    /* RW */

/* SDC_VOL_CHG mask */
#define SDC_VOL_CHG_CNT         (0xffff  << 0)     /* RW  */

/* SDC_STS mask */
#define SDC_STS_SDCBUSY         (0x1  << 0)     /* RW  */
#define SDC_STS_CMDBUSY         (0x1  << 1)     /* RW  */
#define SDC_STS_CMD_WR_BUSY     (0x1  << 16)    /* W1C */
#define SDC_STS_SWR_COMPL       (0x1UL  << 31)  /* RO  */

/* SDC_DCRC_STS mask */
#define SDC_DCRC_STS_POS        (0xff << 0)     /* RO */
#define SDC_DCRC_STS_NEG        (0xff << 8)     /* RO */

/* EMMC_CFG0 mask */
#define EMMC_CFG0_BOOTSTART     (0x1  << 0)     /* W  */
#define EMMC_CFG0_BOOTSTOP      (0x1  << 1)     /* W  */
#define EMMC_CFG0_BOOTMODE      (0x1  << 2)     /* RW */
#define EMMC_CFG0_BOOTACKDIS    (0x1  << 3)     /* RW */
#define EMMC_CFG0_BOOTWDLY      (0x7  << 12)    /* RW */
#define EMMC_CFG0_BOOTSUPP      (0x1  << 15)    /* RW */

/* EMMC_CFG1 mask */
#define EMMC_CFG1_BOOTDATTMC    (0xfffff << 0)  /* RW */
#define EMMC_CFG1_BOOTACKTMC    (0xfffUL << 20) /* RW */

/* EMMC_STS mask */
#define EMMC_STS_BOOTCRCERR     (0x1  << 0)     /* W1C */
#define EMMC_STS_BOOTACKERR     (0x1  << 1)     /* W1C */
#define EMMC_STS_BOOTDATTMO     (0x1  << 2)     /* W1C */
#define EMMC_STS_BOOTACKTMO     (0x1  << 3)     /* W1C */
#define EMMC_STS_BOOTUPSTATE    (0x1  << 4)     /* R   */
#define EMMC_STS_BOOTACKRCV     (0x1  << 5)     /* W1C */
#define EMMC_STS_BOOTDATRCV     (0x1  << 6)     /* R   */

/* EMMC_IOCON mask */
#define EMMC_IOCON_BOOTRST      (0x1  << 0)     /* RW */

/* SDC_ACMD19_TRG mask */
#define SDC_ACMD19_TRG_TUNESEL  (0xf  << 0)     /* RW */

/* MSDC_DMA_SA_HIGH4BIT */
#define MSDC_DMA_SURR_ADDR_HIGH4BIT (0xf  << 0)     /* RW */

/* MSDC_DMA_CTRL mask */
#define MSDC_DMA_CTRL_START     (0x1  << 0)     /* W  */
#define MSDC_DMA_CTRL_STOP      (0x1  << 1)     /* W  */
#define MSDC_DMA_CTRL_RESUME    (0x1  << 2)     /* W  */
#define MSDC_DMA_CTRL_REDAYM    (0x1  << 3)     /* RO */
#define MSDC_DMA_CTRL_MODE      (0x1  << 8)     /* RW */
#define MSDC_DMA_CTRL_ALIGN     (0x1  << 9)     /* RW */
#define MSDC_DMA_CTRL_LASTBUF   (0x1  << 10)    /* RW */
#define MSDC_DMA_CTRL_SPLIT1K   (0x1  << 11)    /* RW */
#define MSDC_DMA_CTRL_BRUSTSZ   (0x7  << 12)    /* RW */

/* MSDC_DMA_CFG mask */
#define MSDC_DMA_CFG_STS        (0x1  << 0)     /* R */
#define MSDC_DMA_CFG_DECSEN     (0x1  << 1)     /* RW */
#define MSDC_DMA_CFG_LOCKDISABLE     (0x1  << 2)     /* RW */
#define MSDC_DMA_CFG_AHBEN      (0x3  << 8)     /* RW */
#define MSDC_DMA_CFG_ACTEN      (0x3  << 12)    /* RW */
#define MSDC_DMA_CFG_CS12B      (0x1  << 16)    /* RW */

/* MSDC_PATCH_BIT0 mask */
#define MSDC_PB0_RESV1           (0x1 << 0)
#define MSDC_PB0_EN_8BITSUP      (0x1 << 1)
#define MSDC_PB0_DIS_RECMDWR     (0x1 << 2)
#define MSDC_PB0_RESV2           (0x7 << 3)
#define MSDC_PB0_DESCUP          (0x1 << 6)
#define MSDC_PB0_INT_DAT_LATCH_CK_SEL (0x7 << 7)
#define MSDC_PB0_CKGEN_MSDC_DLY_SEL   (0x1F<<10)
#define MSDC_PB0_FIFORD_DIS      (0x1 << 15)
#define MSDC_PB0_BLKNUM_SEL      (0x1 << 16)
#define MSDC_PB0_SDIO_INTCSEL    (0x1 << 17)
#define MSDC_PB0_SDC_BSYDLY      (0xf << 18)
#define MSDC_PB0_SDC_WDOD        (0xf << 22)
#define MSDC_PB0_CMDIDRTSEL      (0x1 << 26)
#define MSDC_PB0_CMDFAILSEL      (0x1 << 27)
#define MSDC_PB0_SDIO_INTDLYSEL  (0x1 << 28)
#define MSDC_PB0_SPCPUSH         (0x1 << 29)
#define MSDC_PB0_DETWR_CRCTMO    (0x1 << 30)
#define MSDC_PB0_EN_DRVRSP       (0x1UL << 31)

/* MSDC_PATCH_BIT1 mask */
#define MSDC_PB1_WRDAT_CRCS_TA_CNTR   (0x7 << 0)
#define MSDC_PB1_CMD_RSP_TA_CNTR      (0x7 << 3)
#define MSDC_PB1_GET_BUSY_MA     (0x1 << 6)
#define MSDC_PB1_GET_CRC_MA      (0x1 << 7)
#define MSDC_PB1_BIAS_TUNE_28NM  (0xf << 8)
#define MSDC_PB1_BIAS_EN18IO_28NM (0x1 << 12)
#define MSDC_PB1_BIAS_EXT_28NM   (0x1 << 13)
#define MSDC_PB1_RESV2           (0x1 << 14)
#define MSDC_PB1_RESET_GDMA      (0x1 << 15)
#define MSDC_PB1_SINGLEBURST     (0x1 << 16)
#define MSDC_PB1_FROCE_STOP      (0x1 << 17)
#define MSDC_PB1_DCM_DEV_SEL2    (0x3 << 18)
#define MSDC_PB1_DCM_DEV_SEL1    (0x1 << 20)
#define MSDC_PB1_DCM_EN          (0x1 << 21)
#define MSDC_PB1_AXI_WRAP_CKEN   (0x1 << 22)
#define MSDC_PB1_AHBCKEN         (0x1 << 23)
#define MSDC_PB1_CKSPCEN         (0x1 << 24)
#define MSDC_PB1_CKPSCEN         (0x1 << 25)
#define MSDC_PB1_CKVOLDETEN      (0x1 << 26)
#define MSDC_PB1_CKACMDEN        (0x1 << 27)
#define MSDC_PB1_CKSDEN          (0x1 << 28)
#define MSDC_PB1_CKWCTLEN        (0x1 << 29)
#define MSDC_PB1_CKRCTLEN        (0x1 << 30)
#define MSDC_PB1_CKSHBFFEN       (0x1UL << 31)

/* MSDC_PATCH_BIT2 mask */
#define MSDC_PB2_ENHANCEGPD      (0x1 << 0)
#define MSDC_PB2_SUPPORT64G      (0x1 << 1)
#define MSDC_PB2_RESPWAITCNT     (0x3 << 2)
#define MSDC_PB2_CFGRDATCNT      (0x1f << 4)
#define MSDC_PB2_CFGRDAT         (0x1 << 9)
#define MSDC_PB2_INTCRESPSEL     (0x1 << 11)
#define MSDC_PB2_CFGRESPCNT      (0x7 << 12)
#define MSDC_PB2_CFGRESP         (0x1 << 15)
#define MSDC_PB2_RESPSTENSEL     (0x7 << 16)
#define MSDC_PB2_POPENCNT        (0xf << 20)
#define MSDC_PB2_CFG_CRCSTS_SEL  (0x1 << 24)
#define MSDC_PB2_CFGCRCSTSEDGE   (0x1 << 25)
#define MSDC_PB2_CFGCRCSTSCNT    (0x3 << 26)
#define MSDC_PB2_CFGCRCSTS       (0x1 << 28)
#define MSDC_PB2_CRCSTSENSEL     (0x7UL << 29)

#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
#define MSDC_MASK_ACMD53_CRC_ERR_INTR   (0x1<<4)
#define MSDC_ACMD53_FAIL_ONE_SHOT       (0X1<<5)
#endif	// #if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)

/* MSDC_PAD_TUNE0 mask */
#define MSDC_PAD_TUNE0_DATWRDLY      (0x1F <<  0)     /* RW */
#define MSDC_PAD_TUNE0_DELAYEN       (0x1  <<  7)     /* RW */
#define MSDC_PAD_TUNE0_DATRRDLY      (0x1F <<  8)     /* RW */
#define MSDC_PAD_TUNE0_DATRRDLYSEL   (0x1  << 13)     /* RW */
#define MSDC_PAD_TUNE0_RXDLYSEL      (0x1  << 15)     /* RW */
#define MSDC_PAD_TUNE0_CMDRDLY       (0x1F << 16)     /* RW */
#define MSDC_PAD_TUNE0_CMDRRDLYSEL   (0x1  << 21)     /* RW */
#define MSDC_PAD_TUNE0_CMDRRDLY      (0x1FUL << 22)   /* RW */
#define MSDC_PAD_TUNE0_CLKTXDLY      (0x1FUL << 27)   /* RW */

/* MSDC_PAD_TUNE1 mask */
#define MSDC_PAD_TUNE1_DATRRDLY2     (0x1F <<  8)     /* RW */
#define MSDC_PAD_TUNE1_DATRRDLY2SEL  (0x1  << 13)     /* RW */
#define MSDC_PAD_TUNE1_CMDRDLY2      (0x1F << 16)     /* RW */
#define MSDC_PAD_TUNE1_CMDRRDLY2SEL  (0x1  << 21)     /* RW */

/* MSDC_DAT_RDDLY0/1/2/3 mask */
#define MSDC_DAT_RDDLY0_D3      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY0_D2      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY0_D1      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY0_D0      (0x1FUL << 24)  /* RW */

#define MSDC_DAT_RDDLY1_D7      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY1_D6      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY1_D5      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY1_D4      (0x1FUL << 24)  /* RW */

#define MSDC_DAT_RDDLY2_D3      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY2_D2      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY2_D1      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY2_D0      (0x1FUL << 24)  /* RW */

#define MSDC_DAT_RDDLY3_D7      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY3_D6      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY3_D5      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY3_D4      (0x1FUL << 24)  /* RW */

/* MSDC_HW_DBG_SEL mask */
#define MSDC_HW_DBG0_SEL         (0xff << 0)
#define MSDC_HW_DBG1_SEL         (0xff << 8)
#define MSDC_HW_DBG2_SEL         (0xff << 16)
#define MSDC_HW_DBG3_SEL         (0x1f << 24)
#define MSDC_HW_DBG_WRAPTYPE_SEL (0x3UL  << 29)

/* EMMC50_PAD_DS_TUNE mask */
#define MSDC_EMMC50_PAD_DS_TUNE_DLYSEL  (0x1 << 0)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY2SEL (0x1 << 1)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY1    (0x1f << 2)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY2    (0x1f << 7)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY3    (0x1F << 12)

/* EMMC50_PAD_CMD_TUNE mask */
#define MSDC_EMMC50_PAD_CMD_TUNE_DLY3SEL (0x1 << 0)
#define MSDC_EMMC50_PAD_CMD_TUNE_RXDLY3  (0x1f << 1)
#define MSDC_EMMC50_PAD_CMD_TUNE_TXDLY   (0x1f << 6)

/* EMMC50_PAD_DAT01_TUNE mask */
#define MSDC_EMMC50_PAD_DAT0_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT0_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT0_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT1_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT1_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT1_TXDLY       (0x1f << 22)

/* EMMC50_PAD_DAT23_TUNE mask */
#define MSDC_EMMC50_PAD_DAT2_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT2_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT2_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT3_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT3_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT3_TXDLY       (0x1f << 22)

/* EMMC50_PAD_DAT45_TUNE mask */
#define MSDC_EMMC50_PAD_DAT4_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT4_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT4_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT5_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT5_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT5_TXDLY       (0x1f << 22)

/* EMMC50_PAD_DAT67_TUNE mask */
#define MSDC_EMMC50_PAD_DAT6_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT6_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT6_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT7_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT7_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT7_TXDLY       (0x1f << 22)

/* EMMC51_CFG0 mask */
#define MSDC_EMMC51_CFG0_CMDQ_EN         (0x1 << 0)
#define MSDC_EMMC51_CFG0_WDAT_CNT        (0x3ff << 1)
#define MSDC_EMMC51_CFG0_RDAT_CNT        (0x3ff << 11)
#define MSDC_EMMC51_CFG0_CMDQ_CMD_EN     (0x1 << 21)

/* EMMC50_CFG0 mask */
#define MSDC_EMMC50_CFG_PADCMD_LATCHCK      (0x1 << 0)
#define MSDC_EMMC50_CFG_CRC_STS_CNT         (0x3 << 1)
#define MSDC_EMMC50_CFG_CRC_STS_EDGE        (0x1 << 3)
#define MSDC_EMMC50_CFG_CRC_STS_SEL         (0x1 << 4)
#define MSDC_EMMC50_CFG_END_BIT_CHK_CNT     (0xf << 5)
#define MSDC_EMMC50_CFG_CMD_RESP_SEL        (0x1 << 9)
#define MSDC_EMMC50_CFG_CMD_EDGE_SEL        (0x1 << 10)
#define MSDC_EMMC50_CFG_ENDBIT_CNT          (0x3ff << 11)
#define MSDC_EMMC50_CFG_READ_DAT_CNT        (0x7 << 21)
#define MSDC_EMMC50_CFG_EMMC50_MON_SEL      (0x1 << 24)
#define MSDC_EMMC50_CFG_MSDC_WR_VALID       (0x1 << 25)
#define MSDC_EMMC50_CFG_MSDC_RD_VALID       (0x1 << 26)
#define MSDC_EMMC50_CFG_MSDC_WR_VALID_SEL   (0x1 << 27)
#define MSDC_EMMC50_CFG_EMMC50_MON_SE       (0x1 << 28)
#define MSDC_EMMC50_CFG_TXSKEWSEL           (0x1 << 29)

/* EMMC50_CFG1 mask */
#define MSDC_EMMC50_CFG1_WRPTR_MARGIN    (0xff << 0)
#define MSDC_EMMC50_CFG1_CKSWITCH_CNT    (0x7  << 8)
#define MSDC_EMMC50_CFG1_RDDAT_STOP      (0x1  << 11)
#define MSDC_EMMC50_CFG1_WAITCLK_CNT     (0xf  << 12)
#define MSDC_EMMC50_CFG1_DBG_SEL         (0xff << 16)
#define MSDC_EMMC50_CFG1_PSHCNT          (0x7  << 24)
#define MSDC_EMMC50_CFG1_PSHPSSEL        (0x1  << 27)
#define MSDC_EMMC50_CFG1_DSCFG           (0x1  << 28)
#define MSDC_EMMC50_CFG1_RESV0           (0x7UL  << 29)

/* EMMC50_CFG2_mask */
//#define MSDC_EMMC50_CFG2_AXI_GPD_UP            (0x1 << 0)
#define MSDC_EMMC50_CFG2_AXI_IOMMU_WR_EMI      (0x1 << 1)
#define MSDC_EMMC50_CFG2_AXI_SHARE_EN_WR_EMI   (0x1 << 2)
#define MSDC_EMMC50_CFG2_AXI_IOMMU_RD_EMI      (0x1 << 7)
#define MSDC_EMMC50_CFG2_AXI_SHARE_EN_RD_EMI   (0x1 << 8)
#define MSDC_EMMC50_CFG2_AXI_BOUND_128B        (0x1 << 13)
#define MSDC_EMMC50_CFG2_AXI_BOUND_256B        (0x1 << 14)
#define MSDC_EMMC50_CFG2_AXI_BOUND_512B        (0x1 << 15)
#define MSDC_EMMC50_CFG2_AXI_BOUND_1K          (0x1 << 16)
#define MSDC_EMMC50_CFG2_AXI_BOUND_2K          (0x1 << 17)
#define MSDC_EMMC50_CFG2_AXI_BOUND_4K          (0x1 << 18)
#define MSDC_EMMC50_CFG2_AXI_RD_OUTS_NUM       (0x1f<< 19)
#define MSDC_EMMC50_CFG2_AXI_SET_LEN           (0xf << 24)
#define MSDC_EMMC50_CFG2_AXI_RESP_ERR_TYPE     (0x3 << 28)
#define MSDC_EMMC50_CFG2_AXI_BUSY              (0x1 << 30)

/* EMMC50_CFG3_mask */
#define MSDC_EMMC50_CFG3_OUTS_WR               (0x1f << 0)
#define MSDC_EMMC50_CFG3_ULTRA_SET_WR          (0x3f << 5)
#define MSDC_EMMC50_CFG3_PREULTRA_SET_WR       (0x3f << 11)
#define MSDC_EMMC50_CFG3_ULTRA_SET_RD          (0x3f << 17)
#define MSDC_EMMC50_CFG3_PREULTRA_SET_RD       (0x3f << 23)

/* EMMC50_CFG4_mask */
#define MSDC_EMMC50_CFG4_IMPR_ULTRA_SET_WR     (0xff << 0)
#define MSDC_EMMC50_CFG4_IMPR_ULTRA_SET_RD     (0xff << 8)
#define MSDC_EMMC50_CFG4_ULTRA_EN              (0x3  << 16)
#define MSDC_EMMC50_CFG4_RESV0                 (0x3f << 18)

/* SDIO_TUNE_WIND mask*/
#define MSDC_SDIO_TUNE_WIND      (0x1f << 0)

/* MSDC_CFG[START_BIT] value */
#define START_AT_RISING             (0x0)
#define START_AT_FALLING            (0x1)
#define START_AT_RISING_AND_FALLING (0x2)
#define START_AT_RISING_OR_FALLING  (0x3)

#define MSDC_SMPL_RISING        (0)
#define MSDC_SMPL_FALLING       (1)
#define MSDC_SMPL_SEPERATE      (2)


#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

#define CARD_READY_FOR_DATA             (1<<8)
#define CARD_CURRENT_STATE(x)           ((x&0x00001E00)>>9)



/*
 * MSDC0~3 IO Configuration Base.
 */

#define GPIO_REG_BASE             (0x10211000)


/*--------------------------------------------------------------------------*/
/* MSDC0 GPIO Related Register                                              */
/*--------------------------------------------------------------------------*/

/* MSDC0 related register base*/
#define MSDC0_GPIO_MODE18_ADDR			(GPIO_REG_BASE + 0x410)
#define MSDC0_GPIO_MODE19_ADDR			(GPIO_REG_BASE + 0x420)

#define MSDC0_GPIO_IES_G5_ADDR			(GPIO_REG_BASE + 0xD00)
#define MSDC0_GPIO_SMT_G5_ADDR			(GPIO_REG_BASE + 0xD10)
#define MSDC0_GPIO_TDSEL0_G5_ADDR		(GPIO_REG_BASE + 0xD20)
#define MSDC0_GPIO_RDSEL0_G5_ADDR		(GPIO_REG_BASE + 0xD28)
#define MSDC0_GPIO_DRV0_G5_ADDR			(GPIO_REG_BASE + 0xD70)

#define MSDC0_GPIO_PUPD0_G5_ADDR		(GPIO_REG_BASE + 0xD80)
#define MSDC0_GPIO_PUPD1_G5_ADDR		(GPIO_REG_BASE + 0xD90)


/* MSDC0 mode mask*/
#define MSDC0_MODE_CMD_MASK				(0x7  << 6 )
#define MSDC0_MODE_DSL_MASK				(0x7  << 9 )
#define MSDC0_MODE_CLK_MASK				(0x7  << 12)
#define MSDC0_MODE_DAT0_MASK			(0x7  << 16)
#define MSDC0_MODE_DAT1_MASK			(0x7  << 19)
#define MSDC0_MODE_DAT2_MASK			(0x7  << 22)
#define MSDC0_MODE_DAT3_MASK			(0x7  << 25)
#define MSDC0_MODE_DAT4_MASK			(0x7  << 28)

#define MSDC0_MODE_DAT5_MASK			(0x7  << 0 )
#define MSDC0_MODE_DAT6_MASK			(0x7  << 3 )
#define MSDC0_MODE_DAT7_MASK			(0x7  << 6 )
#define MSDC0_MODE_RSTB_MASK			(0x7  << 9 )

/* MSDC0 IES mask*/
#define MSDC0_IES_CMD_MASK				(0x1  << 0 )
#define MSDC0_IES_DSL_MASK				(0x1  << 1 )
#define MSDC0_IES_CLK_MASK				(0x1  << 2 )
#define MSDC0_IES_DAT_MASK				(0x1  << 3 )
#define MSDC0_IES_RSTB_MASK				(0x1  << 4 )
#define MSDC0_IES_ALL_MASK				(0x1f << 0 )

/* MSDC0 SMT mask*/
#define MSDC0_SMT_CMD_MASK				(0x1  << 0 )
#define MSDC0_SMT_DSL_MASK				(0x1  << 1 )
#define MSDC0_SMT_CLK_MASK				(0x1  << 2 )
#define MSDC0_SMT_DAT_MASK				(0x1  << 3 )
#define MSDC0_SMT_RSTB_MASK				(0x1  << 4 )
#define MSDC0_SMT_ALL_MASK				(0x1f << 0 )

/* MSDC0 TDSEL mask*/
#define MSDC0_TDSEL_CMD_MASK			(0xF  << 0 )
#define MSDC0_TDSEL_DSL_MASK			(0xF  << 4 )
#define MSDC0_TDSEL_CLK_MASK			(0xF  << 8 )
#define MSDC0_TDSEL_DAT_MASK			(0xF  << 12)
#define MSDC0_TDSEL_RSTB_MASK			(0xF  << 16)
#define MSDC0_TDSEL_ALL_MASK			(0xFFFFF << 0)

/* MSDC0 RDSEL mask*/
#define MSDC0_RDSEL_CMD_MASK			(0x1F  << 0 )
#define MSDC0_RDSEL_DSL_MASK			(0x1F  << 6 )
#define MSDC0_RDSEL_CLK_MASK			(0x1F  << 12)
#define MSDC0_RDSEL_DAT_MASK			(0x1F  << 18)
#define MSDC0_RDSEL_RSTB_MASK			(0x1F  << 24)
#define MSDC0_RDSEL_ALL_MASK			(0x3FFFFFFF << 0)

/* MSDC0 SR mask*/
#define MSDC0_SR_CMD_MASK				(0x1  << 3 )
#define MSDC0_SR_DSL_MASK				(0x1  << 7 )
#define MSDC0_SR_CLK_MASK				(0x1  << 11)
#define MSDC0_SR_DAT_MASK				(0x1  << 15)
#define MSDC0_SR_RSTB_MASK				(0x1  << 19)
#define MSDC0_SR_ALL_MASK				(0x11111 << 3)

/* MSDC0 DRV mask*/
#define MSDC0_DRV_CMD_MASK				(0x7  << 0 )
#define MSDC0_DRV_DSL_MASK				(0x7  << 4 )
#define MSDC0_DRV_CLK_MASK				(0x7  << 8 )
#define MSDC0_DRV_DAT_MASK				(0x7  << 12)
#define MSDC0_DRV_RSTB_MASK				(0x7  << 16)
#define MSDC0_DRV_ALL_MASK				(0x77777 << 0)

/* MSDC0 PUPD mask*/
#define MSDC0_PUPD_CMD_MASK				(0x7  << 0 )
#define MSDC0_PUPD_DSL_MASK				(0x7  << 4 )
#define MSDC0_PUPD_CLK_MASK				(0x7  << 8 )
#define MSDC0_PUPD_DAT0_MASK			(0x7  << 12)
#define MSDC0_PUPD_DAT1_MASK			(0x7  << 16)
#define MSDC0_PUPD_DAT2_MASK			(0x7  << 20)
#define MSDC0_PUPD_DAT3_MASK			(0x7  << 24)
#define MSDC0_PUPD_DAT4_MASK			(0x7  << 28)
#define MSDC0_PUPD_CMD_DSL_CLK_DAT04_MASK (0x77777777 << 0)

#define MSDC0_PUPD_DAT5_MASK			(0x7  << 0 )
#define MSDC0_PUPD_DAT6_MASK			(0x7  << 4 )
#define MSDC0_PUPD_DAT7_MASK			(0x7  << 8 )
#define MSDC0_PUPD_RSTB_MASK			(0x7  << 12)
#define MSDC0_PUPD_DAT57_RSTB_MASK		(0x7777 << 0)
#define MSDC0_PUPD_DAT567_MASK			(0x777 << 0)





/*--------------------------------------------------------------------------*/
/* MSDC1 GPIO Related Register                                              */
/*--------------------------------------------------------------------------*/
/* MSDC1 related register base*/
#define MSDC1_GPIO_MODE17_ADDR			(GPIO_REG_BASE + 0x400)
#define MSDC1_GPIO_MODE18_ADDR			(GPIO_REG_BASE + 0x410)

#define MSDC1_GPIO_IES_G4_ADDR			(GPIO_REG_BASE + 0xC00)
#define MSDC1_GPIO_SMT_G4_ADDR			(GPIO_REG_BASE + 0xC10)
#define MSDC1_GPIO_TDSEL0_G4_ADDR		(GPIO_REG_BASE + 0xC20)
#define MSDC1_GPIO_RDSEL0_G4_ADDR		(GPIO_REG_BASE + 0xC28)
#define MSDC1_GPIO_DRV0_G4_ADDR			(GPIO_REG_BASE + 0xC70)
#define MSDC1_GPIO_PUPD0_G4_ADDR		(GPIO_REG_BASE + 0xC80)


/* MSDC1 mode mask*/
#define MSDC1_MODE_CMD_MASK				(0x7  << 19)
#define MSDC1_MODE_CLK_MASK				(0x7  << 22)
#define MSDC1_MODE_DAT0_MASK			(0x7  << 25)
#define MSDC1_MODE_DAT1_MASK			(0x7  << 28)

#define MSDC1_MODE_DAT2_MASK			(0x7  << 0 )
#define MSDC1_MODE_DAT3_MASK			(0x7  << 3 )


/* MSDC1 IES mask*/
#define MSDC1_IES_CMD_MASK				(0x1  << 2 )
#define MSDC1_IES_CLK_MASK				(0x1  << 3 )
#define MSDC1_IES_DAT_MASK				(0x1  << 4 )
#define MSDC1_IES_ALL_MASK				(0x7  << 2 )

/* MSDC1 SMT mask*/
#define MSDC1_SMT_CMD_MASK				(0x1  << 2 )
#define MSDC1_SMT_CLK_MASK				(0x1  << 3 )
#define MSDC1_SMT_DAT_MASK				(0x1  << 4 )
#define MSDC1_SMT_ALL_MASK				(0x7  << 2 )

/* MSDC1 TDSEL mask*/
#define MSDC1_TDSEL_CMD_MASK			(0xF  << 8 )
#define MSDC1_TDSEL_CLK_MASK			(0xF  << 12)
#define MSDC1_TDSEL_DAT_MASK			(0xF  << 16)
#define MSDC1_TDSEL_ALL_MASK			(0xFFF << 8)

/* MSDC1 RDSEL mask*/
#define MSDC1_RDSEL_CMD_MASK			(0x1F << 12)
#define MSDC1_RDSEL_CLK_MASK			(0x1F << 18)
#define MSDC1_RDSEL_DAT_MASK			(0x1F << 24)
#define MSDC1_RDSEL_ALL_MASK			(0x7FFF <<12)

/* MSDC1 SR mask*/
#define MSDC1_SR_CMD_MASK				(0x1  << 11)
#define MSDC1_SR_CLK_MASK				(0x1  << 15)
#define MSDC1_SR_DAT_MASK				(0x1  << 19)
#define MSDC1_SR_ALL_MASK				(0x111 <<11)

/* MSDC1 DRV mask*/
#define MSDC1_DRV_CMD_MASK				(0x7  << 8 )
#define MSDC1_DRV_CLK_MASK				(0x7  << 12)
#define MSDC1_DRV_DAT_MASK				(0x7  << 16)
#define MSDC1_DRV_ALL_MASK				(0x777 << 8)

/* MSDC1 PUPD mask*/
#define MSDC1_PUPD_CMD_MASK				(0x7  << 0 )
#define MSDC1_PUPD_CLK_MASK				(0x7  << 4 )
#define MSDC1_PUPD_DAT0_MASK			(0x7  << 8 )
#define MSDC1_PUPD_DAT1_MASK			(0x7  << 12)
#define MSDC1_PUPD_DAT2_MASK			(0x7  << 16)
#define MSDC1_PUPD_DAT3_MASK			(0x7  << 20)
#define MSDC1_BIAS_TUNE_MASK			(0xF  << 24)
#define MSDC1_PUPD_CMD_CLK_DAT_MASK		(0x777777 << 0)



/*--------------------------------------------------------------------------*/
/* MSDC2 GPIO Related Register                                              */
/*--------------------------------------------------------------------------*/
/* msdc2 related register base*/
#define MSDC2_GPIO_MODE20_ADDR			(GPIO_REG_BASE + 0x430)
#define MSDC2_GPIO_MODE21_ADDR			(GPIO_REG_BASE + 0x440)

#define MSDC2_GPIO_IES_G0_ADDR			(GPIO_REG_BASE + 0x800)
#define MSDC2_GPIO_SMT_G0_ADDR			(GPIO_REG_BASE + 0x810)
#define MSDC2_GPIO_TDSEL0_G0_ADDR		(GPIO_REG_BASE + 0x820)
#define MSDC2_GPIO_RDSEL0_G0_ADDR		(GPIO_REG_BASE + 0x828)
#define MSDC2_GPIO_DRV0_G0_ADDR			(GPIO_REG_BASE + 0x870)
#define MSDC2_GPIO_PUPD0_G0_ADDR		(GPIO_REG_BASE + 0x880)

/* MSDC2 mode mask*/
#define MSDC2_MODE_CMD_MASK				(0x7  << 25)
#define MSDC2_MODE_CLK_MASK				(0x7  << 28)

#define MSDC2_MODE_DAT0_MASK			(0x7  << 0 )
#define MSDC2_MODE_DAT1_MASK			(0x7  << 3 )
#define MSDC2_MODE_DAT2_MASK			(0x7  << 6 )
#define MSDC2_MODE_DAT3_MASK			(0x7  << 9 )

/* MSDC2 IES mask*/
#define MSDC2_IES_CMD_MASK				(0x1  << 2 )
#define MSDC2_IES_CLK_MASK				(0x1  << 3 )
#define MSDC2_IES_DAT_MASK				(0x1  << 4 )
#define MSDC2_IES_ALL_MASK				(0x7  << 2 )

/* MSDC2 SMT mask*/
#define MSDC2_SMT_CMD_MASK				(0x1  << 2 )
#define MSDC2_SMT_CLK_MASK				(0x1  << 3 )
#define MSDC2_SMT_DAT_MASK				(0x1  << 4 )
#define MSDC2_SMT_ALL_MASK				(0x7  << 2 )

/* MSDC2 TDSEL mask*/
#define MSDC2_TDSEL_CMD_MASK			(0xF  << 8 )
#define MSDC2_TDSEL_CLK_MASK			(0xF  << 12)
#define MSDC2_TDSEL_DAT_MASK			(0xF  << 16)
#define MSDC2_TDSEL_ALL_MASK			(0xFFF << 8)


/* MSDC2 RDSEL mask*/
#define MSDC2_RDSEL_CMD_MASK			(0x1F << 4 )
#define MSDC2_RDSEL_CLK_MASK			(0x1F << 10)
#define MSDC2_RDSEL_DAT_MASK			(0x1F << 16)
#define MSDC2_RDSEL_ALL_MASK			(0x7FFF <<4)

/* MSDC2 SR mask*/
#define MSDC2_SR_CMD_MASK				(0x1  << 11)
#define MSDC2_SR_CLK_MASK				(0x1  << 15)
#define MSDC2_SR_DAT_MASK				(0x1  << 19)
#define MSDC2_SR_ALL_MASK				(0x111 <<11)

/* MSDC2 DRV mask*/
#define MSDC2_DRV_CMD_MASK				(0x7  << 8 )
#define MSDC2_DRV_CLK_MASK				(0x7  << 12)
#define MSDC2_DRV_DAT_MASK				(0x7  << 16)
#define MSDC2_DRV_ALL_MASK				(0x777 << 8)

/* MSDC2 PUPD mask*/
#define MSDC2_PUPD_CMD_MASK				(0x7  << 0 )
#define MSDC2_PUPD_CLK_MASK				(0x7  << 4 )
#define MSDC2_PUPD_DAT0_MASK			(0x7  << 8 )
#define MSDC2_PUPD_DAT1_MASK			(0x7  << 12)
#define MSDC2_PUPD_DAT2_MASK			(0x7  << 16)
#define MSDC2_PUPD_DAT3_MASK			(0x7  << 20)
#define MSDC2_BIAS_TUNE_MASK			(0xF  << 24)
#define MSDC2_PUPD_CMD_CLK_DAT_MASK		(0x777777 << 0)


/* add pull down/up mode define */
#define MSDC_GPIO_PULL_UP        (0)
#define MSDC_GPIO_PULL_DOWN      (1)

/* define clock releated register macro */
#define MSDC_MSDCPLL_CON0_OFFSET            	(0x240 )
#define MSDC_MSDCPLL_CON1_OFFSET				(0x244 )
#define MSDC_MSDCPLL_PWR_CON0_OFFSET  			(0x24C )
#define MSDC_CLK_CFG_2_OFFSET					(0x060 )
#define MSDC_CLK_CFG_3_OFFSET					(0x070 )

#define MSDC_PERI_PDN_SET0_OFFSET				(0x0008)
#define MSDC_PERI_PDN_CLR0_OFFSET				(0x0010)
#define MSDC_PERI_PDN_STA0_OFFSET				(0x0018)




typedef enum MSDC_POWER {
	MSDC_VIO18_MC1 = 0,
	MSDC_VIO18_MC2,
	MSDC_VIO28_MC1,
	MSDC_VIO28_MC2,
	MSDC_VMC,
	MSDC_VGP6,
} MSDC_POWER_DOMAIN;


/*--------------------------------------------------------------------------*/
/* Descriptor Structure                                                     */
/*--------------------------------------------------------------------------*/
typedef struct {
    u32  hwo:1; /* could be changed by hw */
    u32  bdp:1;
    u32  rsv0:6;
    u32  chksum:8;
    u32  intr:1;
    u32  rsv1:7;
	u32  nexth4:4;
	u32  ptrh4:4;
    u32  next;
    u32  ptr;
    u32  buflen:24;
    u32  extlen:8;
    u32  arg;
    u32  blknum;
    u32  cmd;
} gpd_t;

typedef struct {
    u32  eol:1;
    u32  rsv0:7;
    u32  chksum:8;
    u32  rsv1:1;
    u32  blkpad:1;
    u32  dwpad:1;
    u32  rsv2:5;
	u32  nexth4:4;
	u32  ptrh4:4;
    u32  next;
    u32  ptr;
    u32  buflen:24;
    u32  rsv3:8;
} bd_t;


struct scatterlist_ex {
	u32 cmd;
	u32 arg;
	u32 sglen;
	struct scatterlist *sg;
};

#define DMA_FLAG_NONE       (0x00000000)
#define DMA_FLAG_EN_CHKSUM  (0x00000001)
#define DMA_FLAG_PAD_BLOCK  (0x00000002)
#define DMA_FLAG_PAD_DWORD  (0x00000004)
#if 0
struct msdc_dma {
    u32 flags;                   /* flags */
    u32 xfersz;                  /* xfer size in bytes */
    u32 sglen;                   /* size of scatter list */
    u32 blklen;                  /* block size */
    struct scatterlist *sg;      /* I/O scatter list */
    struct scatterlist_ex *esg;  /* extended I/O scatter list */
    u8  mode;                    /* dma mode        */
    u8  burstsz;                 /* burst size      */
    u8  intr;                    /* dma done interrupt */
    u8  padding;                 /* padding */
    u32 cmd;                     /* enhanced mode command */
    u32 arg;                     /* enhanced mode arg */
    u32 rsp;                     /* enhanced mode command response */
    u32 autorsp;                 /* auto command response */

    gpd_t *gpd;                  /* pointer to gpd array */
    bd_t  *bd;                   /* pointer to bd array */
    dma_addr_t gpd_addr;         /* the physical address of gpd array */
    dma_addr_t bd_addr;          /* the physical address of bd array */
    u32 used_gpd;                /* the number of used gpd elements */
    u32 used_bd;                 /* the number of used bd elements */
};
#endif
struct tune_counter {
	u32 time_cmd;
	u32 time_read;
	u32 time_write;
	u32 time_hs400;
};
struct msdc_saved_para {
	u32 pad_tune0;
	u32 ddly0;
	u32 ddly1;
	u8 cmd_resp_ta_cntr;
	u8 wrdat_crc_ta_cntr;
	u8 suspend_flag;
	u32 msdc_cfg;
	u32 mode;
	u32 div;
	u32 sdc_cfg;
	u32 iocon;
	u32 state;
	u32 hz;
	u8 int_dat_latch_ck_sel;
	u8 ckgen_msdc_dly_sel;
	u8 inten_sdio_irq;
	u8     write_busy_margin; /* for write: 3T need wait before host check busy after crc status */
	u8     write_crc_margin; /* for write: host check timeout change to 16T */
#ifdef CONFIG_EMMC_50_FEATURE
	u8 ds_dly1;
	u8 ds_dly3;
	u32 emmc50_pad_cmd_tune;
#endif
	u8 cfg_cmdrsp_path;
	u8 cfg_crcsts_path;
	u8 resp_wait_cnt;
};

#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)

#define DMA_ON 0
#define DMA_OFF 1

struct ot_work_t
{
	struct      msdc_host *host;
	int         chg_volt;
	atomic_t    ot_disable;
	atomic_t    autok_done;
};
#endif // #if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)

struct msdc_host
{
    struct device               *dev;
    struct msdc_hw              *hw;

    struct mmc_host             *mmc;           /* mmc structure */
    struct mmc_command          *cmd;
    struct mmc_data             *data;
    struct mmc_request          *mrq;
    int                         cmd_rsp;
    int                         cmd_rsp_done;
    int                         cmd_r1b_done;

    int                         error;
#if 0
    spinlock_t                  lock;           /* mutex */
    spinlock_t                  clk_gate_lock;
    spinlock_t                  remove_bad_card;    /*to solve removing bad card race condition with hot-plug enable*/
    spinlock_t                  sdio_irq_lock;  //avoid race condition @ DATA-1 interrupt case
#endif
    int                         clk_gate_count;
#if 0
    struct semaphore            sem;
#endif

    u32                         blksz;          /* host block size */
    u32                         base;           /* host base address */
    int                         id;             /* host id */
    int                         pwr_ref;        /* core power reference count */

    u32                         xfer_size;      /* total transferred size */
#if 0
    struct msdc_dma             dma;            /* dma channel */
    u32                         dma_addr;       /* dma transfer address */
    u32                         dma_left_size;  /* dma transfer left size */
    u32                         dma_xfer_size;  /* dma transfer size in bytes */
    int                         dma_xfer;       /* dma transfer mode */
#endif
    u32                         timeout_ns;     /* data timeout ns */
    u32                         timeout_clks;   /* data timeout clks */

    atomic_t                    abort;          /* abort transfer */

    int                         irq;            /* host interrupt */
#if 0
    struct tasklet_struct       card_tasklet;
#endif
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
    struct tasklet_struct       flush_cache_tasklet
;
#endif
    //struct delayed_work           remove_card;
#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
    struct ot_work_t            ot_work;
    atomic_t                    ot_done;
    u32                         sdio_performance_vcore; //vcore_fixed_during_sdio_transfer
    struct delayed_work         set_vcore_workq;        //vcore_fixed_during_sdio_transfer
#endif // #if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
    atomic_t                    sdio_stopping;
#if 0
    struct completion           cmd_done;
    struct completion           xfer_done;
    struct pm_message           pm_state;
#endif
    u32                         mclk;           /* mmc subsystem clock */
    u32                         hclk;           /* host clock speed */
    u32                         sclk;           /* SD/MS clock speed */
    u8                          core_clkon;     /* Host core clock on ? */
    u8                          card_clkon;     /* Card clock on ? */
    u8                          core_power;     /* core power */
    u8                          power_mode;     /* host power mode */
    u8                          card_inserted;  /* card inserted ? */
    u8                          suspend;        /* host suspended ? */
    u8                          reserved;
    u8                          app_cmd;        /* for app command */
    u32                         app_cmd_arg;
    u64                         starttime;
#if 0
    struct timer_list           timer;
#endif
    struct tune_counter         t_counter;
    u32                         rwcmd_time_tune;
    int                         read_time_tune;
    int                         write_time_tune;
    u32                         write_timeout_uhs104;
    u32                         read_timeout_uhs104;
    u32                         write_timeout_emmc;
    u32                         read_timeout_emmc;
    u8                          autocmd;
    u32                         sw_timeout;
    u32                         power_cycle; /* power cycle done in tuning flow*/
	bool			power_cycle_enable;/*Enable power cycle*/
	u32			continuous_fail_request_count;
    u32                         sd_30_busy;
    bool                        tune;
    MSDC_POWER_DOMAIN           power_domain;
    u32                         state;
    struct msdc_saved_para      saved_para;
    int                         sd_cd_polarity;
    int                         sd_cd_insert_work; //to make sure insert mmc_rescan this work in start_host when boot up
                                                   //driver will get a EINT(Level sensitive) when boot up phone with card insert
    //struct wake_lock            trans_lock;
    bool                        block_bad_card;
#ifdef SDIO_ERROR_BYPASS
    int                         sdio_error;     /* sdio error can't recovery */
#endif
#if 1 //check if needed
	u32 crc_count;	/* total crc count */
	u32 crc_invalid_count;	/* total crc invalid count eg CMD19 */
	u32 req_count; /* total request count */
	u32 datatimeout_count; /* total data timeout count */
	u32 cmdtimeout_count; /* total cmd timeout count */
	u32 reqtimeout_count; /* total req timeout count */
	u32 pc_count;	/* total power cycle count */
	u32 pc_suspend;	/* suspend/resume count */
	u32 ocr_avail;
#endif
    void    (*power_control)(struct msdc_host *host,u32 on);
    void    (*power_switch)(struct msdc_host *host,u32 on);

    u32 f_min;        /* host min. frequency */
    u32 f_max;        /* host max. frequency */
    u32 blklen;       /* block len */
    u32 blkbits;      /* block bits */
    u32 caps;         /* Host capabilities */
    u64 max_hw_segs;
    u64 max_phys_segs;
    u64 max_seg_size;
    u32 max_blk_size;
    u32 max_blk_count;
    u32 src_clk;      /* msdc host input clock speed (msdc_src_clk) */
    u8  pll_mux_clk;  /* the mux of system pll clock */
    u32 cur_bus_clk;  /* SD/MS bus clock speed (msdc_clk) */
};

typedef enum {
	cmd_counter = 0,
	read_counter,
	write_counter,
	all_counter,
} TUNE_COUNTER;

typedef enum {
   MSDC_STATE_DEFAULT = 0,
   MSDC_STATE_DDR,
   MSDC_STATE_HS200,
   MSDC_STATE_HS400
}msdc_state;

typedef enum {
	TRAN_MOD_PIO,
	TRAN_MOD_DMA,
	TRAN_MOD_NUM
} transfer_mode;

typedef enum {
	OPER_TYPE_READ,
	OPER_TYPE_WRITE,
	OPER_TYPE_NUM
} operation_type;

struct dma_addr{
	u32 start_address;
	u32 size;
	u8 end;
	struct dma_addr *next;
};

#if 1
struct msdc_reg_control {
    unsigned long addr;
    u32 mask;
    u32 value;
    u32 default_value;
    int (*restore_func)(int restore);
};
#endif
static inline unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

#define sdr_read8(reg)           __raw_readb((const volatile void *)reg)
#define sdr_read16(reg)          __raw_readw((const volatile void *)reg)
#define sdr_read32(reg)          __raw_readl((const volatile void *)reg)
#define sdr_write8(reg,val)      mt_reg_sync_writeb(val,reg)
#define sdr_write16(reg,val)     mt_reg_sync_writew(val,reg)
#define sdr_write32(reg,val)     mt_reg_sync_writel(val,reg)
#define sdr_set_bits(reg,bs) \
    do{\
        volatile unsigned int tv = sdr_read32(reg);\
        tv |= (u32)(bs); \
        sdr_write32(reg,tv); \
    }while(0)
#define sdr_clr_bits(reg,bs) \
    do{\
        volatile unsigned int tv = sdr_read32(reg);\
        tv &= ~((u32)(bs)); \
        sdr_write32(reg,tv); \
    }while(0)


#define sdr_set_field(reg,field,val) \
    do {    \
        volatile unsigned int tv = sdr_read32(reg);    \
        tv &= ~(field); \
        tv |= ((val) << (uffs((unsigned int)field) - 1)); \
        sdr_write32(reg,tv); \
    } while(0)
#define sdr_get_field(reg,field,val) \
    do {    \
        volatile unsigned int tv = sdr_read32(reg);    \
        val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
    } while(0)
#define sdr_set_field_discrete(reg,field,val) \
    do {    \
        volatile unsigned int tv = sdr_read32(reg); \
        tv = (val == 1) ? (tv|(field)):(tv & ~(field));\
        sdr_write32(reg,tv); \
    } while(0)
#define sdr_get_field_discrete(reg,field,val) \
    do {    \
        volatile unsigned int tv = sdr_read32(reg); \
        val = tv & (field) ; \
        val = (val == field) ? 1 :0;\
    } while(0)

extern void mmc_remove_card(struct mmc_card *card);
extern void mmc_detach_bus(struct mmc_host *host);
extern void mmc_power_off(struct mmc_host *host);

void msdc_set_blknum(struct msdc_host *host, u32 blknum);
int msdc_pio_read(struct msdc_host *host, void *buf);
int msdc_pio_write(struct msdc_host* host, void *buf);

#define MSDC_MAX_SCLK           (200000000)
#define MSDC_MIN_SCLK           (260000)
#define MMC_BLOCK_BITS                  (9)
#define MMC_CAP_MULTIWRITE      (1 << 1) /* Can accurately report bytes sent to card on error */
#define MMC_CAP_MMC_HIGHSPEED   (1 << 3) /* Can do MMC high-speed timing */
#define MMC_CAP_SD_HIGHSPEED    (1 << 4) /* Can do SD high-speed timing */
#define MSDC_MAX_NUM            (1)
#define MSDC_HIGHSPEED      (1 << 7)  /* high-speed mode support       */
#define MSDC_DDR            (1 << 9)  /* ddr mode support              */
#define MMC_CAP_DDR             (1 << 7) /* The host support dual data rate */
#define MMC_CAP_EMMC_HS200      (1 << 8) /* The host support dual data rate */
#define MMC_CAP_EMMC_HS400      (1 << 9) /* The host support dual data rate */
#define MSDC_HS200          (1 << 10) /* hs200 mode support(eMMC4.5)   */
#define MSDC_HS400          (1 << 11) /* hs200 mode support(eMMC5.0)   */
#define MMC_VDD_27_36       0x00FF8000
#define MAX_BD_POOL_SZ      (4)
#define MAX_DMA_CNT     (8*1024*1024)
#define MAX_SG_POOL_SZ      (MAX_BD_POOL_SZ)
#define MAX_SG_BUF_SZ       (MAX_DMA_CNT)
#define MAX_DMA_TRAN_SIZE   ((u64)MAX_SG_POOL_SZ*MAX_SG_BUF_SZ)
#define HOST_BUS_WIDTH_1            (1)
#define HOST_BUS_WIDTH_4            (4)
#define HOST_BUS_WIDTH_8            (8)
#define MMC_CAP_4_BIT_DATA      (1 << 0) /* Can the host do 4 bit transfers */
#define MMC_CAP_8_BIT_DATA      (1 << 5) /* Can the host do 8 bit transfers */

/* each PLL have different gears for select
 * software can used mux interface from clock management module to select */
enum {
    MSDC0_CLKSRC_26MHZ  = 0,
    MSDC0_CLKSRC_400MHZ,
    MSDC0_CLKSRC_MAX
};

enum {
    MSDC1_CLKSRC_26MHZ   = 0,
    MSDC1_CLKSRC_208MHZ,
    MSDC1_CLKSRC_200MHZ,
    MSDC1_CLKSRC_MAX
};

#define MSDC0_CLKSRC_DEFAULT    MSDC0_CLKSRC_400MHZ
#define MSDC1_CLKSRC_DEFAULT    MSDC1_CLKSRC_200MHZ
#define MSDC_DRVN_GEAR1         0x1
#define MSDC_DRVN_DONT_CARE     0x0
#define MSDC_SMPL_RISING        (0)

void mdelay (unsigned long msec);

#define msdc_retry(expr, retry, cnt,id) \
    do { \
        int backup = cnt; \
        while (retry) { \
            if (!(expr)) break; \
            if (cnt-- == 0) { \
                retry--; mdelay(1); cnt = backup; \
            } \
        } \
        if (retry == 0) { \
        } \
    } while(0)

#define msdc_reset(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
        mb(); \
        msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt,id); \
    } while(0)

#define msdc_clr_int() \
    do { \
        volatile u32 val = sdr_read32(MSDC_INT); \
        sdr_write32(MSDC_INT, val); \
    } while(0)

#define msdc_clr_fifo(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
        msdc_retry(sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, retry, cnt,id); \
    } while(0)

#define msdc_reset_hw(id) \
        msdc_reset(id); \
        msdc_clr_fifo(id); \
        msdc_clr_int();

#define MSDC_CFG_CKMOD_BITS     (2)
#define MSDC_CFG_CKDIV_BITS     (12)

#endif /* end of  MT_SD_H */

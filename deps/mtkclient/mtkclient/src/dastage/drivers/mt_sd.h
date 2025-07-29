#ifndef MT6582_SD_H
#define MT6582_SD_H

#include "types.h"
#include "sync_write.h"
#include "read.h"

#define MTK_MSDC_USE_CMD23
#ifdef MTK_MSDC_USE_CMD23
#define MSDC_USE_AUTO_CMD23   1 
#endif

/* the macro need removed after SQC */
#define REMOVEED_FOR_MT6582 (1)
#define MODIFY_FOR_MT6582   (1)
//#define MSDC_DMA_BOUNDARY_LIMITAION


#define MAX_GPD_NUM         (1 + 1)  /* one null gpd */
#define MAX_BD_NUM          (1024)
#define MAX_BD_PER_GPD      (MAX_BD_NUM)
#define HOST_MAX_NUM        (3)
#define CLK_SRC_MAX_NUM		(1)

#if (0 == REMOVEED_FOR_MT6582)
#define PERI_MSDC0_PDN      (13)
#define PERI_MSDC1_PDN      (14)
#define PERI_MSDC2_PDN      (15)
#define PERI_MSDC3_PDN      (16)
#define PERI_MSDC4_PDN      (17)
#endif

#define CUST_EINT_POLARITY_LOW              0
#define CUST_EINT_POLARITY_HIGH             1
#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
#define CUST_EINT_EDGE_SENSITIVE            0
#define CUST_EINT_LEVEL_SENSITIVE           1
#define SDIO_ERROR_BYPASS
//////////////////////////////////////////////////////////////////////////////

/* msdc2 EINT just for evb only */
//#define EINT_MSDC2_INS_NUM              11
//#define EINT_MSDC2_INS_DEBOUNCE_CN      1
//#define EINT_MSDC2_INS_POLARITY         CUST_EINT_POLARITY_LOW
//#define EINT_MSDC2_INS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
//#define EINT_MSDC2_INS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

//#define EINT_MSDC1_INS_NUM              16
#define EINT_MSDC1_INS_DEBOUNCE_CN     500 
#define EINT_MSDC1_INS_POLARITY         CUST_EINT_POLARITY_LOW
#define EINT_MSDC1_INS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define EINT_MSDC1_INS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

//#define MSDC_CLKSRC_REG     (0xf100000C)
#define MSDC_DESENSE_REG	(0xf0007070)

//#define MTK_SDIO30_ONLINE_TUNING_SUPPORT
//#define CONFIG_MT6582_FPGA

#ifdef CONFIG_MT8127_FPGA
#define FPGA_PLATFORM
#endif
#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0008)
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
#define MSDC_AUTOCMD53          (0x0004)
#endif	// MTK_SDIO30_ONLINE_TUNING_SUPPORT
/*--------------------------------------------------------------------------*/
/* Common Macro                                                             */
/*--------------------------------------------------------------------------*/
#define REG_ADDR(x)                 ((volatile u32*)(0x11230000 + OFFSET_##x))

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD           (64)  // (128)
#define MSDC_NUM                (4)

#define MSDC_MS                 (0) //No memory stick mode, 0 use to gate clock
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

#define MSDC_EMMC_BOOTMODE0     (0)     /* Pull low CMD mode */
#define MSDC_EMMC_BOOTMODE1     (1)     /* Reset CMD mode */

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
#define OFFSET_MSDC_CFG         (0x0)
#define OFFSET_MSDC_IOCON       (0x04)
#define OFFSET_MSDC_PS          (0x08)
#define OFFSET_MSDC_INT         (0x0c)
#define OFFSET_MSDC_INTEN       (0x10)
#define OFFSET_MSDC_FIFOCS      (0x14)
#define OFFSET_MSDC_TXDATA      (0x18)
#define OFFSET_MSDC_RXDATA      (0x1c)
#define OFFSET_SDC_CFG          (0x30)
#define OFFSET_SDC_CMD          (0x34)
#define OFFSET_SDC_ARG          (0x38)
#define OFFSET_SDC_STS          (0x3c)
#define OFFSET_SDC_RESP0        (0x40)
#define OFFSET_SDC_RESP1        (0x44)
#define OFFSET_SDC_RESP2        (0x48)
#define OFFSET_SDC_RESP3        (0x4c)
#define OFFSET_SDC_BLK_NUM      (0x50)
#define OFFSET_SDC_CSTS         (0x58)
#define OFFSET_SDC_CSTS_EN      (0x5c)
#define OFFSET_SDC_DCRC_STS     (0x60)
#define OFFSET_SDC_ADV_CFG0             (0x64)
#define OFFSET_EMMC_CFG0        (0x70)
#define OFFSET_EMMC_CFG1        (0x74)
#define OFFSET_EMMC_STS         (0x78)
#define OFFSET_EMMC_IOCON       (0x7c)
#define OFFSET_SDC_ACMD_RESP    (0x80)
#define OFFSET_SDC_ACMD19_TRG   (0x84)
#define OFFSET_SDC_ACMD19_STS   (0x88)
#define OFFSET_MSDC_DMA_SA      (0x90)
#define OFFSET_MSDC_DMA_CA      (0x94)
#define OFFSET_MSDC_DMA_CTRL    (0x98)
#define OFFSET_MSDC_DMA_CFG     (0x9c)
#define OFFSET_MSDC_DBG_SEL     (0xa0)
#define OFFSET_MSDC_DBG_OUT     (0xa4)
#define OFFSET_MSDC_DMA_LEN     (0xa8)
#define OFFSET_MSDC_PATCH_BIT0   (0xb0)
#define OFFSET_MSDC_PATCH_BIT1  (0xb4)
#define OFFSET_MSDC_PATCH_BIT2  (0xb8)
#define OFFSET_DAT0_TUNE_CRC            (0xc0)
#define OFFSET_DAT1_TUNE_CRC            (0xc4)
#define OFFSET_DAT2_TUNE_CRC            (0xc8)
#define OFFSET_DAT3_TUNE_CRC            (0xcc)
#define OFFSET_CMD_TUNE_CRC             (0xd0)
#define OFFSET_SDIO_TUNE_WIND           (0xd4)
#define OFFSET_MSDC_PAD_TUNE0           (0xf0)
#define OFFSET_MSDC_PAD_TUNE1           (0xf4)
#define OFFSET_MSDC_DAT_RDDLY0          (0xf8)
#define OFFSET_MSDC_DAT_RDDLY1          (0xfc)
#define OFFSET_MSDC_DAT_RDDLY2          (0x100)
#define OFFSET_MSDC_DAT_RDDLY3          (0x104)
#define OFFSET_MSDC_HW_DBG              (0x110)
#define OFFSET_MSDC_VERSION             (0x114)
#define OFFSET_MSDC_ECO_VER             (0x118)
#define OFFSET_EMMC50_PAD_CTL0          (0x180)
#define OFFSET_EMMC50_PAD_DS_CTL0       (0x184)
#define OFFSET_EMMC50_PAD_DS_TUNE       (0x188)
#define OFFSET_EMMC50_PAD_CMD_TUNE      (0x18c)
#define OFFSET_EMMC50_PAD_DAT01_TUNE    (0x190)
#define OFFSET_EMMC50_PAD_DAT23_TUNE    (0x194)
#define OFFSET_EMMC50_PAD_DAT45_TUNE    (0x198)
#define OFFSET_EMMC50_PAD_DAT67_TUNE    (0x19c)
#define OFFSET_EMMC51_CFG0              (0x204)
#define OFFSET_EMMC50_CFG0              (0x208)
#define OFFSET_EMMC50_CFG1              (0x20c)
#define OFFSET_EMMC50_CFG2              (0x21c)
#define OFFSET_EMMC50_CFG3              (0x220)
#define OFFSET_EMMC50_CFG4              (0x224)
#define OFFSET_SDC_FIFO_CFG             (0x228)

/*--------------------------------------------------------------------------*/
/* Register Address                                                         */
/*--------------------------------------------------------------------------*/

/* common register */
#define MSDC_CFG                REG_ADDR(MSDC_CFG)
#define MSDC_IOCON              REG_ADDR(MSDC_IOCON)
#define MSDC_PS                 REG_ADDR(MSDC_PS)
#define MSDC_INT                REG_ADDR(MSDC_INT)
#define MSDC_INTEN              REG_ADDR(MSDC_INTEN)
#define MSDC_FIFOCS             REG_ADDR(MSDC_FIFOCS)
#define MSDC_TXDATA             REG_ADDR(MSDC_TXDATA)
#define MSDC_RXDATA             REG_ADDR(MSDC_RXDATA)

/* sdmmc register */
#define SDC_CFG                 REG_ADDR(SDC_CFG)
#define SDC_CMD                 REG_ADDR(SDC_CMD)
#define SDC_ARG                 REG_ADDR(SDC_ARG)
#define SDC_STS                 REG_ADDR(SDC_STS)
#define SDC_RESP0               REG_ADDR(SDC_RESP0)
#define SDC_RESP1               REG_ADDR(SDC_RESP1)
#define SDC_RESP2               REG_ADDR(SDC_RESP2)
#define SDC_RESP3               REG_ADDR(SDC_RESP3)
#define SDC_BLK_NUM             REG_ADDR(SDC_BLK_NUM)
#define SDC_CSTS                REG_ADDR(SDC_CSTS)
#define SDC_CSTS_EN             REG_ADDR(SDC_CSTS_EN)
#define SDC_DCRC_STS            REG_ADDR(SDC_DCRC_STS)

/* emmc register*/
#define EMMC_CFG0               REG_ADDR(EMMC_CFG0)
#define EMMC_CFG1               REG_ADDR(EMMC_CFG1)
#define EMMC_STS                REG_ADDR(EMMC_STS)
#define EMMC_IOCON              REG_ADDR(EMMC_IOCON)

/* auto command register */
#define SDC_ACMD_RESP           REG_ADDR(SDC_ACMD_RESP)
#define SDC_ACMD19_TRG          REG_ADDR(SDC_ACMD19_TRG)
#define SDC_ACMD19_STS          REG_ADDR(SDC_ACMD19_STS)

/* dma register */
#define MSDC_DMA_SA             REG_ADDR(MSDC_DMA_SA)
#define MSDC_DMA_CA             REG_ADDR(MSDC_DMA_CA)
#define MSDC_DMA_CTRL           REG_ADDR(MSDC_DMA_CTRL)
#define MSDC_DMA_CFG            REG_ADDR(MSDC_DMA_CFG)

/* pad ctrl register */
#define MSDC_PAD_CTL0           REG_ADDR(MSDC_PAD_CTL0)
#define MSDC_PAD_CTL1           REG_ADDR(MSDC_PAD_CTL1)
#define MSDC_PAD_CTL2           REG_ADDR(MSDC_PAD_CTL2)

/* data read delay */
#define MSDC_DAT_RDDLY0         REG_ADDR(MSDC_DAT_RDDLY0)
#define MSDC_DAT_RDDLY1         REG_ADDR(MSDC_DAT_RDDLY1)
#define MSDC_DAT_RDDLY2                 REG_ADDR(MSDC_DAT_RDDLY2)
#define MSDC_DAT_RDDLY3                 REG_ADDR(MSDC_DAT_RDDLY3)

/* debug register */
#define MSDC_DBG_SEL            REG_ADDR(MSDC_DBG_SEL)
#define MSDC_DBG_OUT            REG_ADDR(MSDC_DBG_OUT)

/* misc register */
#define MSDC_PATCH_BIT0                 REG_ADDR(MSDC_PATCH_BIT0)
#define MSDC_PATCH_BIT1                 REG_ADDR(MSDC_PATCH_BIT1)
#define MSDC_PATCH_BIT2                 REG_ADDR(MSDC_PATCH_BIT2)
#define DAT0_TUNE_CRC                   REG_ADDR(DAT0_TUNE_CRC)
#define DAT1_TUNE_CRC                   REG_ADDR(DAT1_TUNE_CRC)
#define DAT2_TUNE_CRC                   REG_ADDR(DAT2_TUNE_CRC)
#define DAT3_TUNE_CRC                   REG_ADDR(DAT3_TUNE_CRC)
#define CMD_TUNE_CRC                    REG_ADDR(CMD_TUNE_CRC)
#define SDIO_TUNE_WIND                  REG_ADDR(SDIO_TUNE_WIND)
#define MSDC_PAD_TUNE0                  REG_ADDR(MSDC_PAD_TUNE0)
#define MSDC_PAD_TUNE1                  REG_ADDR(MSDC_PAD_TUNE1)
#define MSDC_HW_DBG                     REG_ADDR(MSDC_HW_DBG)
#define MSDC_VERSION                    REG_ADDR(MSDC_VERSION)
#define MSDC_ECO_VER                    REG_ADDR(MSDC_ECO_VER)

/* eMMC 5.0 register */
#define EMMC50_PAD_CTL0                 REG_ADDR(EMMC50_PAD_CTL0)
#define EMMC50_PAD_DS_CTL0              REG_ADDR(EMMC50_PAD_DS_CTL0)
#define EMMC50_PAD_DS_TUNE              REG_ADDR(EMMC50_PAD_DS_TUNE)
#define EMMC50_PAD_CMD_TUNE             REG_ADDR(EMMC50_PAD_CMD_TUNE)
#define EMMC50_PAD_DAT01_TUNE           REG_ADDR(EMMC50_PAD_DAT01_TUNE)
#define EMMC50_PAD_DAT23_TUNE           REG_ADDR(EMMC50_PAD_DAT23_TUNE)
#define EMMC50_PAD_DAT45_TUNE           REG_ADDR(EMMC50_PAD_DAT45_TUNE)
#define EMMC50_PAD_DAT67_TUNE           REG_ADDR(EMMC50_PAD_DAT67_TUNE)
#define EMMC51_CFG0                     REG_ADDR(EMMC51_CFG0)
#define EMMC50_CFG0                     REG_ADDR(EMMC50_CFG0)
#define EMMC50_CFG1                     REG_ADDR(EMMC50_CFG1)
#define EMMC50_CFG2                     REG_ADDR(EMMC50_CFG2)
#define EMMC50_CFG3                     REG_ADDR(EMMC50_CFG3)
#define EMMC50_CFG4                     REG_ADDR(EMMC50_CFG4)
#define SDC_FIFO_CFG                    REG_ADDR(SDC_FIFO_CFG)
#define MSDC_AES_SEL                    REG_ADDR(MSDC_AES_SEL)

/* HW CMDQ registger */
#define MTKCQ_CFG0                      REG_ADDR(MTKCQ_CFG0)
#define MTKCQ_CFG1                      REG_ADDR(MTKCQ_CFG1)
#define MTKCQ_CFG2                      REG_ADDR(MTKCQ_CFG2)
#define MTKCQ_ERR_ST                    REG_ADDR(MTKCQ_ERR_ST)
#define MTKCQ_CMD45_READY               REG_ADDR(MTKCQ_CMD45_READY)
#define MTKCQ_TASK_READY_ST             REG_ADDR(MTKCQ_TASK_READY_ST)
#define MTKCQ_TASK_DONE_ST              REG_ADDR(MTKCQ_TASK_DONE_ST)
#define MTKCQ_ERR_ST_CLR                REG_ADDR(MTKCQ_ERR_ST_CLR)
#define MTKCQ_CMD_DONE_CLR              REG_ADDR(MTKCQ_CMD_DONE_CLR)
#define MTKCQ_SW_CTL_CQ                 REG_ADDR(MTKCQ_SW_CTL_CQ)
#define MTKCQ_CMD44_RESP                REG_ADDR(MTKCQ_CMD44_RESP)
#define MTKCQ_CMD45_RESP                REG_ADDR(MTKCQ_CMD45_RESP)
#define MTKCQ_CMD13_RCA                 REG_ADDR(MTKCQ_CMD13_RCA)
#define EMMC51_CQCB_CFG3                REG_ADDR(EMMC51_CQCB_CFG3)
#define EMMC51_CQCB_CMD44               REG_ADDR(EMMC51_CQCB_CMD44)
#define EMMC51_CQCB_CMD45               REG_ADDR(EMMC51_CQCB_CMD45)
#define EMMC51_CQCB_TIDMAP              REG_ADDR(EMMC51_CQCB_TIDMAP)
#define EMMC51_CQCB_TIDMAPCLR           REG_ADDR(EMMC51_CQCB_TIDMAPCLR)
#define EMMC51_CQCB_CURCMD              REG_ADDR(EMMC51_CQCB_CURCMD)
#define SDC_ADV_CFG0                    REG_ADDR(SDC_ADV_CFG0)

/*--------------------------------------------------------------------------*/
/* Register Mask                                                            */
/*--------------------------------------------------------------------------*/

/* MSDC_CFG mask */
#define MSDC_CFG_MODE           (0x1  << 0)     /* RW */
#define MSDC_CFG_CKPDN          (0x1  << 1)     /* RW */
#define MSDC_CFG_RST            (0x1  << 2)     /* RW */
#define MSDC_CFG_PIO            (0x1  << 3)     /* RW */
#define MSDC_CFG_CKDRVEN        (0x1  << 4)     /* RW */
#define MSDC_CFG_BV18SDT        (0x1  << 5)     /* RW */
#define MSDC_CFG_BV18PSS        (0x1  << 6)     /* R  */
#define MSDC_CFG_CKSTB          (0x1  << 7)     /* R  */
#define MSDC_CFG_CKDIV          (0xff << 8)     /* RW */
#define MSDC_CFG_CKMOD          (0x3  << 16)    /* RW */

/* MSDC_IOCON mask */
#define MSDC_IOCON_SDR104CKS    (0x1  << 0)     /* RW */
#define MSDC_IOCON_RSPL         (0x1  << 1)     /* RW */
#define MSDC_IOCON_DSPL         (0x1  << 2)     /* RW */
#define MSDC_IOCON_DDLSEL       (0x1  << 3)     /* RW */
#define MSDC_IOCON_DDR50CKD     (0x1  << 4)     /* RW */
#define MSDC_IOCON_DSPLSEL      (0x1  << 5)     /* RW */
#define MSDC_IOCON_RDSPLSEL     (0x1  << 5)     /* RW */	/* New definition of MSDC_CODA_SD30_v2.13 */
#define MSDC_IOCON_WDSPLSEL     (0x1  << 9)     /* RW */	/* New definition of MSDC_CODA_SD30_v2.13 */
#define MSDC_IOCON_W_DSPL       (0x1  << 8)     /* RW */
#define MSDC_IOCON_WD0_SMPL     (0x1  << 10)    /* RW */
#define MSDC_IOCON_WD1_SMPL     (0x1  << 11)    /* RW */
#define MSDC_IOCON_WD2_SMPL     (0x1  << 12)    /* RW */
#define MSDC_IOCON_WD3_SMPL     (0x1  << 13)    /* RW */
#define MSDC_IOCON_D0SPL        (0x1  << 16)    /* RW */
#define MSDC_IOCON_D1SPL        (0x1  << 17)    /* RW */
#define MSDC_IOCON_D2SPL        (0x1  << 18)    /* RW */
#define MSDC_IOCON_D3SPL        (0x1  << 19)    /* RW */
#define MSDC_IOCON_D4SPL        (0x1  << 20)    /* RW */
#define MSDC_IOCON_D5SPL        (0x1  << 21)    /* RW */
#define MSDC_IOCON_D6SPL        (0x1  << 22)    /* RW */
#define MSDC_IOCON_D7SPL        (0x1  << 23)    /* RW */
#define MSDC_IOCON_RISCSZ       (0x3  << 24)    /* RW */

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
#define MSDC_INT_CSTA           (0x1  << 11)    /* R */
#define MSDC_INT_XFER_COMPL     (0x1  << 12)    /* W1C */
#define MSDC_INT_DXFER_DONE     (0x1  << 13)    /* W1C */
#define MSDC_INT_DATTMO         (0x1  << 14)    /* W1C */
#define MSDC_INT_DATCRCERR      (0x1  << 15)    /* W1C */
#define MSDC_INT_ACMD19_DONE    (0x1  << 16)    /* W1C */
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
#define MSDC_INT_GEAR_OUT_BOUND (0x1  << 20)    /* W1C */
#define MSDC_INT_ACMD53_DONE    (0x1  << 21)    /* W1C */
#define MSDC_INT_ACMD53_FAIL    (0x1  << 22)    /* W1C */
#endif	// MTK_SDIO30_ONLINE_TUNING_SUPPORT

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

/* MSDC_FIFOCS mask */
#define MSDC_FIFOCS_RXCNT       (0xff << 0)     /* R */
#define MSDC_FIFOCS_TXCNT       (0xff << 16)    /* R */
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
#define SDC_CMD_DTYP            (0x3  << 11)    /* RW */
#define SDC_CMD_RW              (0x1  << 13)    /* RW */
#define SDC_CMD_STOP            (0x1  << 14)    /* RW */
#define SDC_CMD_GOIRQ           (0x1  << 15)    /* RW */
#define SDC_CMD_BLKLEN          (0xfff<< 16)    /* RW */
#define SDC_CMD_AUTOCMD         (0x3  << 28)    /* RW */
#define SDC_CMD_VOLSWTH         (0x1  << 30)    /* RW */

/* SDC_STS mask */
#define SDC_STS_SDCBUSY         (0x1  << 0)     /* RW */
#define SDC_STS_CMDBUSY         (0x1  << 1)     /* RW */
#define SDC_STS_SWR_COMPL       (0x1  << 31)    /* RW */

/* SDC_DCRC_STS mask */
#define SDC_DCRC_STS_NEG        (0xff << 8)     /* RO */
#define SDC_DCRC_STS_POS        (0xff << 0)     /* RO */

/* EMMC_CFG0 mask */
#define EMMC_CFG0_BOOTSTART     (0x1  << 0)     /* W */
#define EMMC_CFG0_BOOTSTOP      (0x1  << 1)     /* W */
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
#define EMMC_STS_BOOTUPSTATE    (0x1  << 4)     /* R */
#define EMMC_STS_BOOTACKRCV     (0x1  << 5)     /* W1C */
#define EMMC_STS_BOOTDATRCV     (0x1  << 6)     /* R */

/* EMMC_IOCON mask */
#define EMMC_IOCON_BOOTRST      (0x1  << 0)     /* RW */

/* SDC_ACMD19_TRG mask */
#define SDC_ACMD19_TRG_TUNESEL  (0xf  << 0)     /* RW */

/* MSDC_DMA_CTRL mask */
#define MSDC_DMA_CTRL_START     (0x1  << 0)     /* W */
#define MSDC_DMA_CTRL_STOP      (0x1  << 1)     /* W */
#define MSDC_DMA_CTRL_RESUME    (0x1  << 2)     /* W */
#define MSDC_DMA_CTRL_MODE      (0x1  << 8)     /* RW */
#define MSDC_DMA_CTRL_LASTBUF   (0x1  << 10)    /* RW */
#define MSDC_DMA_CTRL_BRUSTSZ   (0x7  << 12)    /* RW */
#define MSDC_DMA_CTRL_XFERSZ    (0xffffUL << 16)/* RW */

/* MSDC_DMA_CFG mask */
#define MSDC_DMA_CFG_STS        (0x1  << 0)     /* R */
#define MSDC_DMA_CFG_DECSEN     (0x1  << 1)     /* RW */
#define MSDC_DMA_CFG_BDCSERR    (0x1  << 4)     /* R */
#define MSDC_DMA_CFG_GPDCSERR   (0x1  << 5)     /* R */

/* MSDC_PATCH_BIT mask */
//#define CKGEN_RX_SDClKO_SEL     (0x1  << 0)     /*This bit removed on MT6589/MT8585*/ 
#define MSDC_PATCH_BIT_ODDSUPP    (0x1  <<  1)     /* RW */

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
#define MSDC_MASK_ACMD53_CRC_ERR_INTR   (0x1<<4)
#define MSDC_ACMD53_FAIL_ONE_SHOT       (0X1<<5)
#endif	// MTK_SDIO30_ONLINE_TUNING_SUPPORT

//#define MSDC_PATCH_BIT_CKGEN_CK (0x1  << 6)     /* This bit removed on MT6589/MT8585 (Only use internel clock) */
#define MSDC_INT_DAT_LATCH_CK_SEL (0x7  <<  7)
#define MSDC_CKGEN_MSDC_DLY_SEL   (0x1F << 10)
#define MSDC_PATCH_BIT_IODSSEL    (0x1  << 16)    /* RW */
#define MSDC_PATCH_BIT_IOINTSEL   (0x1  << 17)    /* RW */
#define MSDC_PATCH_BIT_BUSYDLY    (0xf  << 18)    /* RW */
#define MSDC_PATCH_BIT_WDOD       (0xf  << 22)    /* RW */
#define MSDC_PATCH_BIT_IDRTSEL    (0x1  << 26)    /* RW */
#define MSDC_PATCH_BIT_CMDFSEL    (0x1  << 27)    /* RW */
#define MSDC_PATCH_BIT_INTDLSEL   (0x1  << 28)    /* RW */
#define MSDC_PATCH_BIT_SPCPUSH    (0x1  << 29)    /* RW */
#define MSDC_PATCH_BIT_DECRCTMO   (0x1  << 30)    /* RW */

/* MSDC_PATCH_BIT1 mask */
#define MSDC_PATCH_BIT1_WRDAT_CRCS  (0x7 << 0)
#define MSDC_PATCH_BIT1_CMD_RSP     (0x7 << 3)
#define MSDC_PATCH_BIT1_ECO_1     (0x1 << 6)    /* for write: 3T need wait before host check busy after crc status */
#define MSDC_PATCH_BIT1_ECO_2     (0x1 << 7)    /* for write: host check timeout change to 16T */

/* MSDC_PAD_CTL0 mask */
#define MSDC_PAD_CTL0_CLKDRVN   (0x7  << 0)     /* RW */
#define MSDC_PAD_CTL0_CLKDRVP   (0x7  << 4)     /* RW */
#define MSDC_PAD_CTL0_CLKSR     (0x1  << 8)     /* RW */
#define MSDC_PAD_CTL0_CLKPD     (0x1  << 16)    /* RW */
#define MSDC_PAD_CTL0_CLKPU     (0x1  << 17)    /* RW */
#define MSDC_PAD_CTL0_CLKSMT    (0x1  << 18)    /* RW */
#define MSDC_PAD_CTL0_CLKIES    (0x1  << 19)    /* RW */
#define MSDC_PAD_CTL0_CLKTDSEL  (0xf  << 20)    /* RW */
#define MSDC_PAD_CTL0_CLKRDSEL  (0xffUL<< 24)   /* RW */

/* MSDC_PAD_CTL1 mask */
#define MSDC_PAD_CTL1_CMDDRVN   (0x7  << 0)     /* RW */
#define MSDC_PAD_CTL1_CMDDRVP   (0x7  << 4)     /* RW */
#define MSDC_PAD_CTL1_CMDSR     (0x1  << 8)     /* RW */
#define MSDC_PAD_CTL1_CMDPD     (0x1  << 16)    /* RW */
#define MSDC_PAD_CTL1_CMDPU     (0x1  << 17)    /* RW */
#define MSDC_PAD_CTL1_CMDSMT    (0x1  << 18)    /* RW */
#define MSDC_PAD_CTL1_CMDIES    (0x1  << 19)    /* RW */
#define MSDC_PAD_CTL1_CMDTDSEL  (0xf  << 20)    /* RW */
#define MSDC_PAD_CTL1_CMDRDSEL  (0xffUL<< 24)   /* RW */

/* MSDC_PAD_CTL2 mask */
#define MSDC_PAD_CTL2_DATDRVN   (0x7  << 0)     /* RW */
#define MSDC_PAD_CTL2_DATDRVP   (0x7  << 4)     /* RW */
#define MSDC_PAD_CTL2_DATSR     (0x1  << 8)     /* RW */
#define MSDC_PAD_CTL2_DATPD     (0x1  << 16)    /* RW */
#define MSDC_PAD_CTL2_DATPU     (0x1  << 17)    /* RW */
#define MSDC_PAD_CTL2_DATIES    (0x1  << 19)    /* RW */
#define MSDC_PAD_CTL2_DATSMT    (0x1  << 18)    /* RW */
#define MSDC_PAD_CTL2_DATTDSEL  (0xf  << 20)    /* RW */
#define MSDC_PAD_CTL2_DATRDSEL  (0xffUL<< 24)   /* RW */

/* MSDC_PAD_TUNE mask */
#define MSDC_PAD_TUNE_DATWRDLY  (0x1F << 0)     /* RW */
#define MSDC_PAD_TUNE_DATRRDLY  (0x1F << 8)     /* RW */
#define MSDC_PAD_TUNE_CMDRDLY   (0x1F << 16)    /* RW */
#define MSDC_PAD_TUNE_CMDRRDLY  (0x1FUL << 22)  /* RW */
#define MSDC_PAD_TUNE_CLKTXDLY  (0x1FUL << 27)  /* RW */

/* MSDC_DAT_RDDLY0/1 mask */
#define MSDC_DAT_RDDLY0_D3      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY0_D2      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY0_D1      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY0_D0      (0x1FUL << 24)    /* RW */

#define MSDC_DAT_RDDLY1_D7      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY1_D6      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY1_D5      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY1_D4      (0x1FUL << 24)    /* RW */

#define CARD_READY_FOR_DATA             (1<<8)
#define CARD_CURRENT_STATE(x)           ((x&0x00001E00)>>9)

/* SD_PAD_CTL0-2 were removed from MSDC IP. For IO port attr, like IES\SMT\SR\E16\E8\E4\E2\PU\PD\PUPD\R1\R0 
 * up level to msdc top. msdc top impliment by GPIO, so need sync with the GPIO designer(MingTe MT6582) 
 * for detail */
#define MSDC0_GPIO_CLK_BASE                   (GPIO_BASE + 0xC00)
#define MSDC0_GPIO_CMD_BASE                   (GPIO_BASE + 0xC10)
#define MSDC0_GPIO_DAT_BASE                   (GPIO_BASE + 0xC20)
//#define MSDC0_GPIO_PAD_BASE                   (GPIO_BASE + 0xC30)
#define MDSC0_DAT_PAD_1						  (GPIO_BASE + 0xC30)
#define MDSC0_DAT_PAD_2						  (GPIO_BASE + 0xC40)
#define MDSC0_DAT_PAD_3						  (GPIO_BASE + 0xC50)
#define MDSC0_DAT_PAD_TD_RDSEL				  (GPIO_BASE + 0xC60) //0x000A default

#define MSDC1_GPIO_CLK_BASE                   (GPIO_BASE + 0xC70)
#define MSDC1_GPIO_CMD_BASE                   (GPIO_BASE + 0xC80)
#define MSDC1_GPIO_DAT_BASE                   (GPIO_BASE + 0xC90)
#define MDSC1_DAT_PAD_1						  (GPIO_BASE + 0xCA0)
#define MDSC1_DAT_PAD_2						  (GPIO_BASE + 0xCB0)
#define MDSC1_DAT_PAD_TD_RDSEL				  (GPIO_BASE + 0xCC0) //0x00CA default
//#define MSDC1_GPIO_PAD_BASE                   (GPIO_BASE + 0xC70) //should remove it later

#define MSDC2_GPIO_CLK_BASE                   (GPIO_BASE + 0xCD0)
#define MSDC2_GPIO_CMD_BASE                   (GPIO_BASE + 0xCE0)
#define MSDC2_GPIO_DAT_BASE                   (GPIO_BASE + 0xCF0)
#define MDSC2_DAT_PAD_1						  (GPIO_BASE + 0xD00)
#define MDSC2_DAT_PAD_2						  (GPIO_BASE + 0xD10)
#define MDSC2_DAT_PAD_TD_RDSEL				  (GPIO_BASE + 0xD20) //0x00 default
//#define MSDC2_GPIO_PAD_BASE                   (GPIO_BASE + 0xC70) //should remove it later
//#define MSDC2_GPIO_PAD_BASE                   (GPIO_BASE + 0xCB0)

//#define GPIO_MSDC0_TDSEL_MASK				(0x0FUL << 0)
//#define GPIO_MSDC0_RDSEL_MASK				(0x3FUL << 4)
//These TDSEL and RDSEL mask are the same for MSDC0, 1, 2 on MT8127
#define GPIO_PAD_TDSEL_MASK                   (0xFUL  <<  0) 
#define GPIO_PAD_RDSEL_MASK                   (0x3FUL <<  4)
#define GPIO_PAD_TUNE_MASK                    (0xFUL  << 10)

#if 1
//jct debug, just for build pass, will remove it later
#define GPIO_R0_MASK                          (0x1UL  <<  0)
#define GPIO_R1_MASK                          (0x1UL  <<  1)
#endif
#if 0
#define GPIO_R0_MASK                          (0x1UL  <<  0)
#define GPIO_R1_MASK                          (0x1UL  <<  1)
#else
//these are same on MSDC0, MSDC1, MSDC2 on MT8127
#define GPIO_MSDC_CLK_R0_MASK			(0x01UL << 10)
#define GPIO_MSDC_CLK_R1_MASK			(0x01UL <<  9)
#define GPIO_MSDC_CMD_R0_MASK			(0x01UL << 10)
#define GPIO_MSDC_CMD_R1_MASK			(0x01UL <<  9)
#define GPIO_MSDC_DAT0_R0				(0x01UL << 2)
#define GPIO_MSDC_DAT1_R0				(0x01UL << 6)
#define GPIO_MSDC_DAT2_R0				(0x01UL << 10)
#define GPIO_MSDC_DAT3_R0				(0x01UL << 14)
#define GPIO_MSDC_DAT_R0_MASK	(GPIO_MSDC_DAT0_R0| GPIO_MSDC_DAT1_R0| GPIO_MSDC_DAT2_R0 |GPIO_MSDC_DAT3_R0)
#define GPIO_MSDC_DAT0_R1				(0x01UL << 1)
#define GPIO_MSDC_DAT1_R1				(0x01UL << 5)
#define GPIO_MSDC_DAT2_R1				(0x01UL << 9)
#define GPIO_MSDC_DAT3_R1				(0x01UL << 13)
#define GPIO_MSDC_DAT_R1_MASK	(GPIO_MSDC_DAT0_R1| GPIO_MSDC_DAT1_R1| GPIO_MSDC_DAT2_R1 |GPIO_MSDC_DAT3_R1)

#endif
//#define GPIO_PUPD_MASK                        (0x1UL  <<  2)
//MSDC0 
#define GPIO_MSDC0_CLK_PUPD_MASK			  (0x01UL << 8)
#define GPIO_MSDC0_CMD_PUPD_MASK			  (0x01UL << 8)

#define MSDC0_DAT_PAD_DAT0_PUPD					(0x01UL << 0)
#define MSDC0_DAT_PAD_DAT1_PUPD					(0x01UL << 4)
#define MSDC0_DAT_PAD_DAT2_PUPD					(0x01UL << 8)
#define MSDC0_DAT_PAD_DAT3_PUPD					(0x01UL << 12)
#define MSDC0_DAT_PAD_PUPD_MASK					(MSDC0_DAT_PAD_DAT0_PUPD | MSDC0_DAT_PAD_DAT1_PUPD | MSDC0_DAT_PAD_DAT2_PUPD| MSDC0_DAT_PAD_DAT3_PUPD)
#define MSDC0_DAT_PAD_DAT4_PUPD					(0x01UL << 0)
#define MSDC0_DAT_PAD_DAT5_PUPD					(0x01UL << 4)
#define MSDC0_DAT_PAD_DAT6_PUPD					(0x01UL << 8)
#define MSDC0_DAT_PAD_DAT7_PUPD					(0x01UL << 12)

//MSDC1 
#define GPIO_MSDC1_CLK_PUPD_MASK			  (0x01UL << 8)
#define GPIO_MSDC1_CMD_PUPD_MASK			  (0x01UL << 8)

#define MSDC1_DAT_PAD_DAT0_PUPD					(0x01UL << 0)
#define MSDC1_DAT_PAD_DAT1_PUPD					(0x01UL << 4)
#define MSDC1_DAT_PAD_DAT2_PUPD					(0x01UL << 8)
#define MSDC1_DAT_PAD_DAT3_PUPD					(0x01UL << 12)
#define MSDC1_DAT_PAD_PUPD_MASK					(MSDC1_DAT_PAD_DAT0_PUPD | MSDC1_DAT_PAD_DAT1_PUPD | MSDC1_DAT_PAD_DAT2_PUPD| MSDC1_DAT_PAD_DAT3_PUPD)

//MSDC2 
#define GPIO_MSDC2_CLK_PUPD_MASK			  (0x01UL << 8)
#define GPIO_MSDC2_CMD_PUPD_MASK			  (0x01UL << 8)

#define MSDC2_DAT_PAD_DAT0_PUPD					(0x01UL << 0)
#define MSDC2_DAT_PAD_DAT1_PUPD					(0x01UL << 4)
#define MSDC2_DAT_PAD_DAT2_PUPD					(0x01UL << 8)
#define MSDC2_DAT_PAD_DAT3_PUPD					(0x01UL << 12)
#define MSDC2_DAT_PAD_PUPD_MASK					(MSDC2_DAT_PAD_DAT0_PUPD | MSDC2_DAT_PAD_DAT1_PUPD | MSDC2_DAT_PAD_DAT2_PUPD| MSDC2_DAT_PAD_DAT3_PUPD)


#define GPIO_PD_MASK                          (0x1UL  <<  4)
#define GPIO_PU_MASK                          (0x1UL  <<  5)

#define GPIO_DAT0_PD_MASK                     (0x1UL  <<  0)
#define GPIO_DAT0_PU_MASK                     (0x1UL  <<  1)
#define GPIO_DAT1_PD_MASK                     (0x1UL  <<  2)
#define GPIO_DAT1_PU_MASK                     (0x1UL  <<  3)
#define GPIO_DAT2_PD_MASK                     (0x1UL  <<  4)
#define GPIO_DAT2_PU_MASK                     (0x1UL  <<  5)
#define GPIO_DAT3_PD_MASK                     (0x1UL  <<  6)
#define GPIO_DAT3_PU_MASK                     (0x1UL  <<  7)

#define GPIO_MSDC0_E2_MASK                    (0x1UL  <<  0)
#define GPIO_MSDC0_E4_MASK                    (0x1UL  <<  1)
#define GPIO_MSDC0_E8_MASK                    (0x1UL  <<  2)

#define GPIO_MSDC1_E2_MASK                    (0x1UL  <<  0)
#define GPIO_MSDC1_E4_MASK                    (0x1UL  <<  1)
#define GPIO_MSDC1_E8_MASK                    (0x1UL  <<  2)

#define GPIO_MSDC1_MSDC2_E4_MASK              (0x1UL  <<  8)
#define GPIO_MSDC1_MSDC2_E8_MASK              (0x1UL  <<  9)
#define GPIO_MSDC1_MSDC2_E16_MASK             (0x1UL  << 10)

/* T28ns PAD TX Pull-Down Driving Strength Control */
#define GPIO_MSDC0_DRVN                       (GPIO_MSDC0_E2_MASK | GPIO_MSDC0_E4_MASK | GPIO_MSDC0_E8_MASK)
#define GPIO_MSDC1_DRVN                       (GPIO_MSDC1_E2_MASK | GPIO_MSDC1_E4_MASK | GPIO_MSDC1_E8_MASK)
#define GPIO_MSDC1_MSDC2_DRVN                 (GPIO_MSDC1_MSDC2_E4_MASK | GPIO_MSDC1_MSDC2_E8_MASK | GPIO_MSDC1_MSDC2_E16_MASK)

#if 1
//jct debug, below only for build pass, will remove it later
#define GPIO_PUPD_MASK                        (0x1UL  <<  8)
#define GPIO_SR_MASK                          (0x1UL  << 12)
#define GPIO_SMT_MASK                         (0x1UL  << 13)
#define GPIO_IES_MASK                         (0x1UL  << 14)
#endif
#if 0
#define GPIO_SR_MASK                          (0x1UL  << 12)
#define GPIO_SMT_MASK                         (0x1UL  << 13)
#define GPIO_IES_MASK                         (0x1UL  << 14)
#else
//MSDC0
#define GPIO_MSDC0_CLK_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC0_CLK_IES_MASK			(0x01UL << 4)  //default 1
#define GPIO_MSDC0_CLK_SMT_MASK			(0x01UL << 11)

#define GPIO_MSDC0_CMD_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC0_CMD_IES_MASK			(0x01UL << 4)
#define GPIO_MSDC0_CMD_SMT_MASK			(0x01UL << 11)  //default 1

#define GPIO_MSDC0_DAT_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC0_DAT_IES_MASK			(0x01UL << 4)
#define GPIO_MSDC0_DAT0_SMT				(0x01UL << 3)
#define GPIO_MSDC0_DAT1_SMT				(0x01UL << 7)
#define GPIO_MSDC0_DAT2_SMT				(0x01UL << 11)
#define GPIO_MSDC0_DAT3_SMT				(0x01UL << 15)
#define GPIO_MSDC0_DAT_SMT_MASK			(GPIO_MSDC0_DAT0_SMT| GPIO_MSDC0_DAT1_SMT| GPIO_MSDC0_DAT2_SMT| GPIO_MSDC0_DAT3_SMT)
#define GPIO_MSDC0_DAT4_SMT				(0x01UL << 3)
#define GPIO_MSDC0_DAT5_SMT				(0x01UL << 7)
#define GPIO_MSDC0_DAT6_SMT				(0x01UL << 11)
#define GPIO_MSDC0_DAT7_SMT				(0x01UL << 15)
//MSDC1
#define GPIO_MSDC1_CLK_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC1_CLK_IES_MASK			(0x01UL << 4)  //default 1
#define GPIO_MSDC1_CLK_SMT_MASK			(0x01UL << 11)

#define GPIO_MSDC1_CMD_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC1_CMD_IES_MASK			(0x01UL << 4)
#define GPIO_MSDC1_CMD_SMT_MASK			(0x01UL << 11)  //default 1

#define GPIO_MSDC1_DAT_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC1_DAT_IES_MASK			(0x01UL << 4)
#define GPIO_MSDC1_DAT0_SMT				(0x01UL << 3)
#define GPIO_MSDC1_DAT1_SMT				(0x01UL << 7)
#define GPIO_MSDC1_DAT2_SMT				(0x01UL << 11)
#define GPIO_MSDC1_DAT3_SMT				(0x01UL << 15)
#define GPIO_MSDC1_DAT_SMT_MASK			(GPIO_MSDC1_DAT0_SMT| GPIO_MSDC1_DAT1_SMT| GPIO_MSDC1_DAT2_SMT| GPIO_MSDC1_DAT3_SMT)
//MSDC2
#define GPIO_MSDC2_CLK_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC2_CLK_IES_MASK			(0x01UL << 4)  //default 1
#define GPIO_MSDC2_CLK_SMT_MASK			(0x01UL << 11)

#define GPIO_MSDC2_CMD_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC2_CMD_IES_MASK			(0x01UL << 4)
#define GPIO_MSDC2_CMD_SMT_MASK			(0x01UL << 11)  //default 1

#define GPIO_MSDC2_DAT_SR_MASK			(0x01UL << 3)
#define GPIO_MSDC2_DAT_IES_MASK			(0x01UL << 4)
#define GPIO_MSDC2_DAT0_SMT				(0x01UL << 3)
#define GPIO_MSDC2_DAT1_SMT				(0x01UL << 7)
#define GPIO_MSDC2_DAT2_SMT				(0x01UL << 11)
#define GPIO_MSDC2_DAT3_SMT				(0x01UL << 15)
#define GPIO_MSDC2_DAT_SMT_MASK			(GPIO_MSDC2_DAT0_SMT| GPIO_MSDC2_DAT1_SMT| GPIO_MSDC2_DAT2_SMT| GPIO_MSDC2_DAT3_SMT)

#endif

/* add pull down/up mode define */
#define MSDC0_PULL_NONE     (2)
#define MSDC0_PU_10K        (1)
#define MSDC0_PU_50K        (3)
#define MSDC0_PU_8K         (4)
#define MSDC0_PD_10K        (0)
#define MSDC0_PD_50K        (5)
#define MSDC0_PD_8K         (6)        

#define MSDC1_PULL_NONE     (2)
#define MSDC1_PU_50K        (1)
#define MSDC1_PD_50K        (0) 

#define MSDC2_PULL_NONE     (2)
#define MSDC2_PU_50K        (1)
#define MSDC2_PD_50K        (0)

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
    u32  rsv1:15;
    void *next;
    void *ptr;
    u32  buflen:16;
    u32  extlen:8;
    u32  rsv2:8;
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
    u32  rsv2:13;
    void *next;
    void *ptr;
    u32  buflen:16;
    u32  rsv3:16;
} bd_t;

/*--------------------------------------------------------------------------*/
/* Register Debugging Structure                                             */
/*--------------------------------------------------------------------------*/

typedef struct {
    u32 msdc:1;
    u32 ckpwn:1;
    u32 rst:1;
    u32 pio:1;
    u32 ckdrven:1;
    u32 start18v:1;
    u32 pass18v:1;
    u32 ckstb:1;
    u32 ckdiv:8;
    u32 ckmod:2;
    u32 pad:14;		
} msdc_cfg_reg;
typedef struct {
    u32 sdr104cksel:1;
    u32 rsmpl:1;
    u32 dsmpl:1;
    u32 ddlysel:1;
    u32 ddr50ckd:1;
    u32 dsplsel:1;
    u32 pad1:10;
    u32 d0spl:1;
    u32 d1spl:1;
    u32 d2spl:1;
    u32 d3spl:1;
    u32 d4spl:1;
    u32 d5spl:1;
    u32 d6spl:1;
    u32 d7spl:1;
    u32 riscsz:1;
    u32 pad2:7;
} msdc_iocon_reg;
typedef struct {
    u32 cden:1;
    u32 cdsts:1;
    u32 pad1:10;
    u32 cddebounce:4;
    u32 dat:8;
    u32 cmd:1;
    u32 pad2:6;
    u32 wp:1;
} msdc_ps_reg;
typedef struct {
    u32 mmcirq:1;
    u32 cdsc:1;
    u32 pad1:1;
    u32 atocmdrdy:1;
    u32 atocmdtmo:1;
    u32 atocmdcrc:1;
    u32 dmaqempty:1;
    u32 sdioirq:1;
    u32 cmdrdy:1;
    u32 cmdtmo:1;
    u32 rspcrc:1;
    u32 csta:1;
    u32 xfercomp:1;
    u32 dxferdone:1;
    u32 dattmo:1;
    u32 datcrc:1;
    u32 atocmd19done:1;
    u32 pad2:15;
} msdc_int_reg;
typedef struct {
    u32 mmcirq:1;
    u32 cdsc:1;
    u32 pad1:1;
    u32 atocmdrdy:1;
    u32 atocmdtmo:1;
    u32 atocmdcrc:1;
    u32 dmaqempty:1;
    u32 sdioirq:1;
    u32 cmdrdy:1;
    u32 cmdtmo:1;
    u32 rspcrc:1;
    u32 csta:1;
    u32 xfercomp:1;
    u32 dxferdone:1;
    u32 dattmo:1;
    u32 datcrc:1;
    u32 atocmd19done:1;
    u32 pad2:15;
} msdc_inten_reg;
typedef struct {
    u32 rxcnt:8;
    u32 pad1:8;
    u32 txcnt:8;
    u32 pad2:7;
    u32 clr:1;
} msdc_fifocs_reg;
typedef struct {
    u32 val;
} msdc_txdat_reg;
typedef struct {
    u32 val;
} msdc_rxdat_reg;
typedef struct {
    u32 sdiowkup:1;
    u32 inswkup:1;
    u32 pad1:14;
    u32 buswidth:2;
    u32 pad2:1;
    u32 sdio:1;
    u32 sdioide:1;
    u32 intblkgap:1;
    u32 pad4:2;
    u32 dtoc:8;
} sdc_cfg_reg;
typedef struct {
    u32 cmd:6;
    u32 brk:1;
    u32 rsptyp:3;
    u32 pad1:1;
    u32 dtype:2;
    u32 rw:1;
    u32 stop:1;
    u32 goirq:1;    
    u32 blklen:12;
    u32 atocmd:2;
    u32 volswth:1;
    u32 pad2:1;
} sdc_cmd_reg;
typedef struct {
    u32 arg;
} sdc_arg_reg;
typedef struct {
    u32 sdcbusy:1;
    u32 cmdbusy:1;
    u32 pad:29;
    u32 swrcmpl:1;
} sdc_sts_reg;
typedef struct {
    u32 val;
} sdc_resp0_reg;
typedef struct {
    u32 val;	
} sdc_resp1_reg;
typedef struct {
    u32 val;	
} sdc_resp2_reg;
typedef struct {
    u32 val;	
} sdc_resp3_reg;
typedef struct {
    u32 num;	
} sdc_blknum_reg;
typedef struct {
    u32 sts;
} sdc_csts_reg;
typedef struct {
    u32 sts;
} sdc_cstsen_reg;
typedef struct {
    u32 datcrcsts:8;
    u32 ddrcrcsts:4;
    u32 pad:20;
} sdc_datcrcsts_reg;
typedef struct {
    u32 bootstart:1;
    u32 bootstop:1;
    u32 bootmode:1;
    u32 pad1:9;
    u32 bootwaidly:3;
    u32 bootsupp:1;
    u32 pad2:16;
} emmc_cfg0_reg;
typedef struct {
    u32 bootcrctmc:16;
    u32 pad:4;
    u32 bootacktmc:12;
} emmc_cfg1_reg;
typedef struct {
    u32 bootcrcerr:1;
    u32 bootackerr:1;
    u32 bootdattmo:1;
    u32 bootacktmo:1;
    u32 bootupstate:1;
    u32 bootackrcv:1;
    u32 bootdatrcv:1;
    u32 pad:25;
} emmc_sts_reg;
typedef struct {
    u32 bootrst:1;
    u32 pad:31;
} emmc_iocon_reg;
typedef struct {
    u32 val;
} msdc_acmd_resp_reg;
typedef struct {
    u32 tunesel:4;
    u32 pad:28;
} msdc_acmd19_trg_reg;
typedef struct {
    u32 val;
} msdc_acmd19_sts_reg;
typedef struct {
    u32 addr;
} msdc_dma_sa_reg;
typedef struct {
    u32 addr;
} msdc_dma_ca_reg;
typedef struct {
    u32 start:1;
    u32 stop:1;
    u32 resume:1;
    u32 pad1:5;
    u32 mode:1;
    u32 pad2:1;
    u32 lastbuf:1;
    u32 pad3:1;
    u32 brustsz:3;
    u32 pad4:1;
    u32 xfersz:16;
} msdc_dma_ctrl_reg;
typedef struct {
    u32 status:1;
    u32 decsen:1;
    u32 pad1:2;
    u32 bdcsen:1;
    u32 gpdcsen:1;
    u32 pad2:26;
} msdc_dma_cfg_reg;
typedef struct {
    u32 sel:16;
    u32 pad2:16;
} msdc_dbg_sel_reg;
typedef struct {
    u32 val;
} msdc_dbg_out_reg;
typedef struct {
    u32 clkdrvn:3;
    u32 rsv0:1;
    u32 clkdrvp:3;
    u32 rsv1:1;
    u32 clksr:1;
    u32 rsv2:7;
    u32 clkpd:1;    
    u32 clkpu:1;
    u32 clksmt:1;
    u32 clkies:1;
    u32 clktdsel:4;
    u32 clkrdsel:8;
} msdc_pad_ctl0_reg;
typedef struct {
    u32 cmddrvn:3;
    u32 rsv0:1;    
    u32 cmddrvp:3;
    u32 rsv1:1;
    u32 cmdsr:1;
    u32 rsv2:7;
    u32 cmdpd:1;    
    u32 cmdpu:1;
    u32 cmdsmt:1;
    u32 cmdies:1;
    u32 cmdtdsel:4;
    u32 cmdrdsel:8;
} msdc_pad_ctl1_reg;
typedef struct {
    u32 datdrvn:3;
    u32 rsv0:1;
    u32 datdrvp:3;
    u32 rsv1:1;
    u32 datsr:1;
    u32 rsv2:7;
    u32 datpd:1;    
    u32 datpu:1;
    u32 datsmt:1;
    u32 daties:1;
    u32 dattdsel:4;
    u32 datrdsel:8;
} msdc_pad_ctl2_reg;
typedef struct {
    u32 wrrxdly:3;
    u32 pad1:5;
    u32 rdrxdly:8;
    u32 pad2:16;
} msdc_pad_tune_reg;
typedef struct {
    u32 dat0:5;
    u32 rsv0:3;
    u32 dat1:5;
    u32 rsv1:3;
    u32 dat2:5;
    u32 rsv2:3;
    u32 dat3:5;
    u32 rsv3:3;    
} msdc_dat_rddly0;
typedef struct {
    u32 dat4:5;
    u32 rsv4:3;
    u32 dat5:5;
    u32 rsv5:3;
    u32 dat6:5;
    u32 rsv6:3;
    u32 dat7:5;
    u32 rsv7:3;
} msdc_dat_rddly1;
typedef struct {
    u32 dbg0sel:8;
    u32 dbg1sel:6;
    u32 pad1:2;
    u32 dbg2sel:6;
    u32 pad2:2;
    u32 dbg3sel:6;
    u32 pad3:2;
} msdc_hw_dbg_reg;
typedef struct {
    u32 val;
} msdc_version_reg;
typedef struct {
    u32 val;
} msdc_eco_ver_reg;

struct msdc_regs {
    msdc_cfg_reg        msdc_cfg;      /* base+0x00h */
    msdc_iocon_reg      msdc_iocon;    /* base+0x04h */
    msdc_ps_reg         msdc_ps;       /* base+0x08h */
    msdc_int_reg        msdc_int;      /* base+0x0ch */
    msdc_inten_reg      msdc_inten;    /* base+0x10h */
    msdc_fifocs_reg     msdc_fifocs;   /* base+0x14h */
    msdc_txdat_reg      msdc_txdat;    /* base+0x18h */
    msdc_rxdat_reg      msdc_rxdat;    /* base+0x1ch */
    u32                 rsv1[4];
    sdc_cfg_reg         sdc_cfg;       /* base+0x30h */
    sdc_cmd_reg         sdc_cmd;       /* base+0x34h */
    sdc_arg_reg         sdc_arg;       /* base+0x38h */
    sdc_sts_reg         sdc_sts;       /* base+0x3ch */
    sdc_resp0_reg       sdc_resp0;     /* base+0x40h */
    sdc_resp1_reg       sdc_resp1;     /* base+0x44h */
    sdc_resp2_reg       sdc_resp2;     /* base+0x48h */
    sdc_resp3_reg       sdc_resp3;     /* base+0x4ch */
    sdc_blknum_reg      sdc_blknum;    /* base+0x50h */
    u32                 rsv2[1];
    sdc_csts_reg        sdc_csts;      /* base+0x58h */
    sdc_cstsen_reg      sdc_cstsen;    /* base+0x5ch */
    sdc_datcrcsts_reg   sdc_dcrcsta;   /* base+0x60h */
    u32                 rsv3[3];
    emmc_cfg0_reg       emmc_cfg0;     /* base+0x70h */
    emmc_cfg1_reg       emmc_cfg1;     /* base+0x74h */
    emmc_sts_reg        emmc_sts;      /* base+0x78h */
    emmc_iocon_reg      emmc_iocon;    /* base+0x7ch */
    msdc_acmd_resp_reg  acmd_resp;     /* base+0x80h */
    msdc_acmd19_trg_reg acmd19_trg;    /* base+0x84h */
    msdc_acmd19_sts_reg acmd19_sts;    /* base+0x88h */
    u32                 rsv4[1];
    msdc_dma_sa_reg     dma_sa;        /* base+0x90h */
    msdc_dma_ca_reg     dma_ca;        /* base+0x94h */
    msdc_dma_ctrl_reg   dma_ctrl;      /* base+0x98h */
    msdc_dma_cfg_reg    dma_cfg;       /* base+0x9ch */
    msdc_dbg_sel_reg    dbg_sel;       /* base+0xa0h */
    msdc_dbg_out_reg    dbg_out;       /* base+0xa4h */
    u32                 rsv5[2];
    u32                 patch0;        /* base+0xb0h */
    u32                 patch1;        /* base+0xb4h */
    u32                 rsv6[10];
    msdc_pad_ctl0_reg   pad_ctl0;      /* base+0xe0h */
    msdc_pad_ctl1_reg   pad_ctl1;      /* base+0xe4h */
    msdc_pad_ctl2_reg   pad_ctl2;      /* base+0xe8h */
    msdc_pad_tune_reg   pad_tune;      /* base+0xech */
    msdc_dat_rddly0     dat_rddly0;    /* base+0xf0h */
    msdc_dat_rddly1     dat_rddly1;    /* base+0xf4h */
    msdc_hw_dbg_reg     hw_dbg;        /* base+0xf8h */
    u32                 rsv7[1];       
    msdc_version_reg    version;       /* base+0x100h */
    msdc_eco_ver_reg    eco_ver;       /* base+0x104h */
};

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
struct tune_counter
{
	u32 time_cmd;
	u32 time_read;
	u32 time_write;
};
struct msdc_saved_para
{
	u32							pad_tune;
	u32							ddly0;
	u32							ddly1;
	u8							cmd_resp_ta_cntr;
	u8							wrdat_crc_ta_cntr;
	u8							suspend_flag;
	u32 						msdc_cfg;
	u32 						mode;
	u32 						div;
	u32 						sdc_cfg;
	u32 						iocon;
	int             ddr;
	u32             hz;
	u8							int_dat_latch_ck_sel;
	u8							ckgen_msdc_dly_sel;
	u8							inten_sdio_irq;
	u8							write_timeout_eco1; /* for write: 3T need wait before host check busy after crc status */
	u8							write_timeout_eco2; /* for write: host check timeout change to 16T */
};

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
struct ot_data
{
    u32 eco_ver;
    u32 orig_blknum;
    u32 orig_patch_bit0;
    u32 orig_iocon;

#define DMA_ON 0
#define DMA_OFF 1
    u32 orig_dma;
    u32 orig_cmdrdly;
    u32 orig_dat0rddly;
    u32 orig_dat1rddly;
    u32 orig_dat2rddly;
    u32 orig_dat3rddly;
    u32 orig_dtoc;

    u32 cmdrdly;
    u32 dat0rddly;
    u32 dat1rddly;
    u32 dat2rddly;
    u32 dat3rddly;

    u32 cmddlypass;
    u32 dat0rddlypass;
    u32 dat1rddlypass;
    u32 dat2rddlypass;
    u32 dat3rddlypass;

    u32 fCmdTestedGear;
    u32 fDat0TestedGear;
    u32 fDat1TestedGear;
    u32 fDat2TestedGear;
    u32 fDat3TestedGear;

    u32 rawcmd;
    u32 rawarg;
    u32 tune_wind_size;
    u32 fn;
    u32 addr;
	u32 retry;
};

struct ot_work_t
{
	struct delayed_work ot_delayed_work;
	struct msdc_host *host;
};
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT

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
	spinlock_t                  remove_bad_card;	/*to solve removing bad card race condition with hot-plug enable*/
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
	//struct delayed_work       	remove_card;
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
	int	                        pre_temper;	    /* previous set temperature */
	struct workqueue_struct     *ot_wq;         /* online tuning work queue */
	struct ot_work_t            ot_work;
	bool                        ot_work_in_wq;
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
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
	u32							rwcmd_time_tune;
	int							read_time_tune;
    int                         write_time_tune;
	u32							write_timeout_uhs104;
	u32							read_timeout_uhs104;
	u32							write_timeout_emmc;
	u32							read_timeout_emmc;
	u8						    autocmd;
	u32							sw_timeout;
	u32							power_cycle; /* power cycle done in tuning flow*/
	bool						power_cycle_enable;/*Enable power cycle*/
	u32							sd_30_busy;
	bool						tune;
	MSDC_POWER_DOMAIN			power_domain;
	bool						ddr;
	struct msdc_saved_para		saved_para;	
	int 						sd_cd_polarity;
	int							sd_cd_insert_work; //to make sure insert mmc_rescan this work in start_host when boot up
												   //driver will get a EINT(Level sensitive) when boot up phone with card insert
	bool						block_bad_card;											   
#ifdef SDIO_ERROR_BYPASS      
    int                         sdio_error;     /* sdio error can't recovery */
#endif									   
	u32 crc_count;	/* total crc count */
	u32 crc_invalid_count;	/* total crc invalid count eg CMD19 */
	u32 req_count; /* total request count */
	u32 datatimeout_count; /* total data timeout count */
	u32 cmdtimeout_count; /* total cmd timeout count */
	u32 reqtimeout_count; /* total req timeout count */
	u32 pc_count;	/* total power cycle count */
	u32 pc_suspend;	/* suspend/resume count */
	void	(*power_control)(struct msdc_host *host,u32 on);
	void	(*power_switch)(struct msdc_host *host,u32 on);
#ifdef CONFIG_AMAZON_METRICS_LOG
	struct delayed_work metrics_work;
	bool metrics_enable;
	u32 crc_count_p;	/* reported crc count */
	u32 crc_invalid_count_p;	/* reported crc invalid count eg CMD19 */
	u32 req_count_p; /* reported request count */
	u32 datatimeout_count_p; /* reported data timeout count */
	u32 cmdtimeout_count_p; /* reported cmd timeout count */
	u32 reqtimeout_count_p; /* reported req timeout count */
	u32 pc_count_p;	/* reported power cycle count */
	u32 pc_suspend_p;	/* reported suspend/resume count */
	u32 inserted_p; /* reported card detection count */
	u32 inserted; /* total card detection cound */
#endif

    u32 ocr_avail;

    u32 cur_pwr;
    u32 caps;         /* Host capabilities */
    u32 f_min;        /* host min. frequency */
    u32 f_max;        /* host max. frequency */
    u32 src_clk;      /* msdc host input clock speed (msdc_src_clk) */
    u32 cur_bus_clk;  /* SD/MS bus clock speed (msdc_clk) */
    u32 blklen;       /* block len */
    u32 blkbits;      /* block bits */
    void *priv;       /* private data */
    u64 max_hw_segs;
    u64 max_phys_segs;
    u64 max_seg_size;
    u32 max_blk_size;
    u32 max_blk_count;
    u8  pll_mux_clk;  /* the mux of system pll clock */

};
typedef enum {
   TRAN_MOD_PIO,
   TRAN_MOD_DMA,
   TRAN_MOD_NUM
}transfer_mode;

typedef enum {
   OPER_TYPE_READ,
   OPER_TYPE_WRITE,
   OPER_TYPE_NUM
}operation_type;

struct dma_addr{
   u32 start_address;
   u32 size;
   u8 end; 
   struct dma_addr *next;
};

#ifdef CONFIG_AMAZON_METRICS_LOG
#define MSDC_LOG_COUNTER_TO_VITALS(name, value) \
	do { \
		if (value != value##_p) { \
			log_counter_to_vitals(ANDROID_LOG_INFO, "Kernel", "Kernel", \
					((host->mmc) && (host->mmc->caps & MMC_CAP_SD_HIGHSPEED)) ? "SD" : "EMMC", \
#name, value - value##_p, "count", NULL, VITALS_NORMAL); \
			value##_p = value; \
		} \
	} while (0)
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
#if 0
#define sdr_write8(reg,val)      __raw_writeb(val,reg)
#define sdr_write16(reg,val)     __raw_writew(val,reg)
#define sdr_write32(reg,val)     __raw_writel(val,reg)
#define sdr_set_bits(reg,bs)     ((*(volatile u32*)(reg)) |= (u32)(bs))
#define sdr_clr_bits(reg,bs)     ((*(volatile u32*)(reg)) &= ~((u32)(bs)))
#else
#define sdr_write8(reg,val)      mt65xx_reg_sync_writeb(val,reg)
#define sdr_write16(reg,val)     mt65xx_reg_sync_writew(val,reg)
#define sdr_write32(reg,val)     mt65xx_reg_sync_writel(val,reg)
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

#endif

#define sdr_set_field(reg,field,val) \
    do {	\
        volatile unsigned int tv = sdr_read32(reg);	\
        tv &= ~(field); \
        tv |= ((val) << (uffs((unsigned int)field) - 1)); \
        sdr_write32(reg,tv); \
    } while(0)
#define sdr_get_field(reg,field,val) \
    do {	\
        volatile unsigned int tv = sdr_read32(reg);	\
        val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
    } while(0)
#define sdr_set_field_discrete(reg,field,val) \
			do {	\
				volatile unsigned int tv = sdr_read32(reg); \
				tv = (val == 1) ? (tv|(field)):(tv & ~(field));\
				sdr_write32(reg,tv); \
			} while(0)
#define sdr_get_field_discrete(reg,field,val) \
			do {	\
				volatile unsigned int tv = sdr_read32(reg); \
				val = tv & (field) ; \
				val = (val == field) ? 1 :0;\
			} while(0)
static void msdc_dump_info() {}

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
            msdc_dump_info(id); \
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


/*******************************************************************************
 * Power Definition
 ******************************************************************************/
typedef enum MSDC_POWER_VOL_TAG {
    VOL_DEFAULT,
    VOL_0900 = 900,
    VOL_1000 = 1000,
    VOL_1100 = 1100,
    VOL_1200 = 1200,
    VOL_1300 = 1300,
    VOL_1350 = 1350,
    VOL_1500 = 1500,
    VOL_1800 = 1800,
    VOL_2000 = 2000,
    VOL_2100 = 2100,
    VOL_2500 = 2500,
    VOL_2800 = 2800,
    VOL_2900 = 2900,
    VOL_3000 = 3000,
    VOL_3300 = 3300,
    VOL_3400 = 3400,
    VOL_3500 = 3500,
    VOL_3600 = 3600
} MSDC_POWER_VOLTAGE;

#define EMMC_VOL_ACTUAL         VOL_3000
#define MSDC_PB0_DEFAULT        0x403C0007
#define MSDC_PB1_DEFAULT        0xFFE20349
#define MSDC_PB2_DEFAULT        0x14801803
#define SDC_ADV_CFG0_SDC_RX_ENH_EN      (0x1  << 20) /* RW */
#define SDC_RX_ENH_EN           (0x1 << 15)     /* TW */
#define OFFSET_EMMC_TOP_CONTROL         (0x00)

#endif /* end of MT6582_SD_H */


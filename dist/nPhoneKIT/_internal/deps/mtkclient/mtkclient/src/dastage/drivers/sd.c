// Based on Linux kernel for mt8127
// License: GPLv2

#include "types.h"
#include "core.h"
#include "mt_sd.h"
#include "mmc.h"
#include "sd.h"
#include "sdio.h"
#include "errno.h"

#define msdc_txfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_fifo_write32(v)   sdr_write32(MSDC_TXDATA, (v))
#define msdc_fifo_write8(v)    sdr_write8(MSDC_TXDATA, (v))
#define msdc_fifo_read32()   sdr_read32(MSDC_RXDATA)
#define msdc_fifo_read8()    sdr_read8(MSDC_RXDATA) 

#define msdc_dma_on()        sdr_clr_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_off()       sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO)

#define sdc_is_busy()          (sdr_read32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()      (sdr_read32(SDC_STS) & SDC_STS_CMDBUSY)

#define sdc_send_cmd(cmd,arg) \
    do { \
        sdr_write32(SDC_ARG, (arg)); \
        mb(); \
        sdr_write32(SDC_CMD, (cmd)); \
    } while(0)

static int msdc_rsp[] = {
    0,  /* RESP_NONE */
    1,  /* RESP_R1 */
    2,  /* RESP_R2 */
    3,  /* RESP_R3 */
    4,  /* RESP_R4 */
    1,  /* RESP_R5 */
    1,  /* RESP_R6 */
    1,  /* RESP_R7 */
    7,  /* RESP_R1b */
};

/*
   Need to add a timeout, or WDT timeout, system reboot.      
*/
// pio mode data read/write
int msdc_pio_read(struct msdc_host *host, void *buf)
{
    u32  num = 1;
    u32 *ptr;
    u8  *u8ptr;
    u32  left = 0;
    u32  count, size = 0;
    u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL;
    u32  ints = 0;
    bool get_xfer_done = 0;
    uint32_t error = 0;
    // unsigned long tmo = jiffies + DAT_TIMEOUT;  
          
    sdr_clr_bits(MSDC_INTEN, wints);
    while (1) {
        if(!get_xfer_done){
            ints = sdr_read32(MSDC_INT);
            ints &= wints;
            sdr_write32(MSDC_INT,ints);
        }
        if(ints & MSDC_INT_DATTMO){
            printf("Data timeout\n");
            error = (unsigned int)-ETIMEDOUT;
            msdc_reset_hw(host->id); 
            break;
            }
        else if(ints & MSDC_INT_DATCRCERR){
            printf("Data CRC\n");
            error = (unsigned int)-EIO;
            msdc_reset_hw(host->id); 
            break;
            }
        else if(ints & MSDC_INT_XFER_COMPL){
            printf("Transfer complete\n");
            get_xfer_done = 1;
            if((num == 0) && (left == 0))   
                break;
        }
        // if(msdc_pio_abort(host, data, tmo)) 
        //         goto end;   
        if((num == 0) && (left == 0))
            continue;
        left = 0x200;
        ptr = buf;
        while (left) {
            // printf("left = 0x%08X fifocnt 0x%08X\n", left, msdc_rxfifocnt());
            if ((left >=  MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {
                count = MSDC_FIFO_THD >> 2;
                do {
                    *ptr++ = msdc_fifo_read32();
                } while (--count);
                left -= MSDC_FIFO_THD;
            } else if ((left < MSDC_FIFO_THD) && msdc_rxfifocnt() >= left) {
                while (left > 3) {
                    *ptr++ = msdc_fifo_read32();
                    left -= 4;
                }
                u8ptr = (u8 *)ptr;
                while(left) {
                    * u8ptr++ = msdc_fifo_read8();
                    left--;
                }
            }

            if(left) {
                ints = sdr_read32(MSDC_INT);
                if(ints & MSDC_INT_DATTMO) {
                error = (unsigned int)-ETIMEDOUT;
                break;
        }
            }
            // if (msdc_pio_abort(host, data, tmo)) {
            //     goto end;
            // }
        }

        break;
    }
    // data->bytes_xfered += size;
    N_MSG(FIO, "        PIO Read<%d>bytes\n", size);
        
    sdr_clr_bits(MSDC_INTEN, wints);    
    if(error) ERR_MSG("read pio data->error<%d> left<%d> size<%d>\n", error, left, size);
    return error;
}

/* please make sure won't using PIO when size >= 512 
   which means, memory card block read/write won't using pio
   then don't need to handle the CMD12 when data error. 
*/
int msdc_pio_write(struct msdc_host* host, void *buf)
{
    u32  num = 1;
    u32 *ptr;
    u8  *u8ptr;
    u32  left;
    u32  count, size = 0;
    u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL; 
    bool get_xfer_done = 0;
    uint32_t error = 0;
    // unsigned long tmo = jiffies + DAT_TIMEOUT;  
    u32 ints = 0;
    sdr_clr_bits(MSDC_INTEN, wints);
    while (1) {
        if(!get_xfer_done){
            ints = sdr_read32(MSDC_INT);
            ints &= wints;
            sdr_write32(MSDC_INT,ints);
        }
        if(ints & MSDC_INT_DATTMO){
            error = (unsigned int)-ETIMEDOUT;
            msdc_reset_hw(host->id); 
            break;
            }
        else if(ints & MSDC_INT_DATCRCERR){
            error = (unsigned int)-EIO;
            msdc_reset_hw(host->id); 
            break;
            }
        else if(ints & MSDC_INT_XFER_COMPL){
            get_xfer_done = 1;
            if((num == 0) && (left == 0))   
                break;
        }
        // if(msdc_pio_abort(host, data, tmo)) 
        //         goto end;   
        if((num == 0) && (left == 0))
            continue;
        left = 0x200;
        ptr = buf;

        while (left) {
            if (left >= MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
                count = MSDC_FIFO_SZ >> 2;
                do {
                    msdc_fifo_write32(*ptr); ptr++;
                } while (--count);
                left -= MSDC_FIFO_SZ;
            } else if (left < MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
                while (left > 3) {
                    msdc_fifo_write32(*ptr); ptr++;
                    left -= 4;
                } 
                
                u8ptr = (u8*)ptr; 
                while(left){
                    msdc_fifo_write8(*u8ptr);   u8ptr++;
                    left--;
                }
            }
            
            // if (msdc_pio_abort(host, data, tmo)) {
            //     goto end;   
            // }                   
        }
        size += 0x200 - left;

        // size += sg_dma_len(sg);
        // sg = sg_next(sg); num--;

        break;
    }
    // data->bytes_xfered += size;
    N_MSG(FIO, "        PIO Write<%d>bytes\n", size);
    if (size != 0x200)
        error = -EIO;
    if(error) ERR_MSG("write pio data->error<%d>\n", error);
        
    //sdr_clr_bits(MSDC_INTEN, wints);  
    return error; 
}


static u32 wints_cmd = MSDC_INT_CMDRDY  | MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO  |  
                       MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO; 
static unsigned int msdc_command_start(struct msdc_host   *host, 
                                      struct mmc_command *cmd,
                                      int                 tune,   /* not used */
                                      unsigned long       timeout)
{
    (void)tune;
    (void)timeout;

    u32 opcode = cmd->opcode;
    u32 rawcmd;
    u32 rawarg;
    u32 resp;  
    // unsigned long tmo;
    struct mmc_request *mrq = cmd->mrq;

    /* Protocol layer does not provide response type, but our hardware needs 
     * to know exact type, not just size!
     */
    if (opcode == MMC_SEND_OP_COND || opcode == SD_APP_OP_COND)
        resp = RESP_R3;
    else if (opcode == MMC_SET_RELATIVE_ADDR || opcode == SD_SEND_RELATIVE_ADDR)
        resp = (mmc_cmd_type(cmd) == MMC_CMD_BCR) ? RESP_R6 : RESP_R1;
    else if (opcode == MMC_FAST_IO)
        resp = RESP_R4;
    else if (opcode == MMC_GO_IRQ_STATE)
        resp = RESP_R5;
    else if (opcode == MMC_SELECT_CARD) {
        resp = (cmd->arg != 0) ? RESP_R1B : RESP_NONE;
        host->app_cmd_arg = cmd->arg;
        printk(KERN_WARNING "msdc%d select card<0x%.8x>", host->id,cmd->arg);  // select and de-select                  
    } else if (opcode == SD_IO_RW_DIRECT || opcode == SD_IO_RW_EXTENDED)
        resp = RESP_R1; /* SDIO workaround. */
    else if (opcode == SD_SEND_IF_COND && (mmc_cmd_type(cmd) == MMC_CMD_BCR))
        resp = RESP_R1;
    else {
        switch (mmc_resp_type(cmd)) {
        case MMC_RSP_R1:
            resp = RESP_R1;
            break;
        case MMC_RSP_R1B:
            resp = RESP_R1B;
            break;
        case MMC_RSP_R2:
            resp = RESP_R2;
            break;
        case MMC_RSP_R3:
            resp = RESP_R3;
            break;
        case MMC_RSP_NONE:
        default:
            resp = RESP_NONE;              
            break;
        }
    }

    cmd->error = 0;
    /* rawcmd :
     * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 | 
     * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
     */    
    rawcmd = opcode | msdc_rsp[resp] << 7 | host->blksz << 16;
    
    if (opcode == MMC_READ_MULTIPLE_BLOCK) {
        rawcmd |= (2 << 11);
        if ((host->autocmd & MSDC_AUTOCMD12) && mrq->stop)
            rawcmd |= (1 << 28);
#ifdef MTK_MSDC_USE_CMD23
        else if((host->autocmd & MSDC_AUTOCMD23))
            rawcmd |= (1 << 29);
#endif /* end of MTK_MSDC_USE_CMD23 */
    } else if (opcode == MMC_READ_SINGLE_BLOCK) {
        rawcmd |= (1 << 11);
    } else if (opcode == MMC_WRITE_MULTIPLE_BLOCK) {
        rawcmd |= ((2 << 11) | (1 << 13));
        if ((host->autocmd & MSDC_AUTOCMD12) && mrq->stop)
            rawcmd |= (1 << 28);
#ifdef MTK_MSDC_USE_CMD23
        else if((host->autocmd & MSDC_AUTOCMD23))
            rawcmd |= (1 << 29);
#endif /* end of MTK_MSDC_USE_CMD23 */
    } else if (opcode == MMC_WRITE_BLOCK) {
        rawcmd |= ((1 << 11) | (1 << 13));
    } else if (opcode == SD_IO_RW_EXTENDED) {
        if (cmd->data->flags & MMC_DATA_WRITE)
            rawcmd |= (1 << 13);
        if (cmd->data->blocks > 1)
            rawcmd |= (2 << 11);
        else
            rawcmd |= (1 << 11);
#ifdef MTK_SDIO30_TEST_MODE_SUPPORT
        if (host->autocmd == MSDC_AUTOCMD53)
            rawcmd |= (1 << 31);
#endif  // MTK_SDIO30_TEST_MODE_SUPPORT
    } else if (opcode == SD_IO_RW_DIRECT && cmd->flags == (unsigned int)-1) {
        rawcmd |= (1 << 14);
    } else if (opcode == SD_SWITCH_VOLTAGE) {
        rawcmd |= (1 << 30);
    } else if ((opcode == SD_APP_SEND_SCR) || 
        (opcode == SD_APP_SEND_NUM_WR_BLKS) ||
    (cmd->data && opcode == MMC_SEND_TUNING_BLOCK) ||
        (opcode == SD_SWITCH && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
        (opcode == SD_APP_SD_STATUS && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
        (opcode == MMC_SEND_EXT_CSD && (mmc_cmd_type(cmd) == MMC_CMD_ADTC))) {
        rawcmd |= (1 << 11);
    } else if (opcode == MMC_STOP_TRANSMISSION) {
        rawcmd |= (1 << 14);
        rawcmd &= ~(0x0FFF << 16);
    }

    N_MSG(CMD, "CMD<%d><0x%.8x> Arg<0x%.8x>\n", opcode , rawcmd, cmd->arg);

    // tmo = jiffies + timeout;

    if (opcode == MMC_SEND_STATUS) {
        for (;;) {
            if (!sdc_is_cmd_busy())
                break;
#if 0
            if (time_after(jiffies, tmo)) {
                ERR_MSG("XXX cmd_busy timeout: before CMD<%d>", opcode);    
                cmd->error = (unsigned int)-ETIMEDOUT;
        if (host->hw->host_function == MSDC_SD) {
            host->reqtimeout_count++;
#ifdef CONFIG_AMAZON_METRICS_LOG
            mod_delayed_work(system_wq, &host->metrics_work, METRICS_DELAY);
#endif
        }

                msdc_reset_hw(host->id);
                return cmd->error;  /* Fix me: error handling */
            } 
#endif
        }
    }else {
        for (;;) {   
            if (!sdc_is_busy())
                break;
#if 0
            if (time_after(jiffies, tmo)) {
                ERR_MSG("XXX sdc_busy timeout: before CMD<%d>", opcode);    
                cmd->error = (unsigned int)-ETIMEDOUT;
        if (host->hw->host_function == MSDC_SD) {
            host->reqtimeout_count++;
#ifdef CONFIG_AMAZON_METRICS_LOG
            mod_delayed_work(system_wq, &host->metrics_work, METRICS_DELAY);
#endif
        }

                msdc_reset_hw(host->id);
                return cmd->error;    
            }   
#endif
        }    
    }   
    
    //BUG_ON(in_interrupt());
    host->cmd     = cmd;
    host->cmd_rsp = resp;       

    /* use polling way */
    sdr_clr_bits(MSDC_INTEN, wints_cmd);             
    rawarg = cmd->arg;
#ifdef CONFIG_MTK_EMMC_SUPPORT
    if(host->hw->host_function == MSDC_EMMC              &&
                host->hw->boot == MSDC_BOOT_EN           &&
                  (cmd->opcode == MMC_READ_SINGLE_BLOCK    || 
                   cmd->opcode == MMC_READ_MULTIPLE_BLOCK  ||
                   cmd->opcode == MMC_WRITE_BLOCK          ||
                   cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
                   cmd->opcode == MMC_ERASE_GROUP_START    ||
                   cmd->opcode == MMC_ERASE_GROUP_END)   && 
                   (partition_access == 0)) {
//        if(cmd->arg == 0)
//            msdc_cal_offset(host);    
//        rawarg  += offset;
        if(cmd->opcode == MMC_ERASE_GROUP_START)
            erase_start = rawarg;
        if(cmd->opcode == MMC_ERASE_GROUP_END)
            erase_end = rawarg;
    }   
    if(cmd->opcode == MMC_ERASE                                          && 
         (cmd->arg == MMC_SECURE_ERASE_ARG || cmd->arg == MMC_ERASE_ARG) && 
           host->mmc->card                                               && 
           host->hw->host_function == MSDC_EMMC                          &&
           host->hw->boot == MSDC_BOOT_EN                                && 
           (!mmc_erase_group_aligned(host->mmc->card,erase_start,erase_end))){
        if(cmd->arg == MMC_SECURE_ERASE_ARG && mmc_can_secure_erase_trim(host->mmc->card))
            rawarg = MMC_SECURE_TRIM1_ARG;
        else if(cmd->arg == MMC_ERASE_ARG 
                ||(cmd->arg == MMC_SECURE_ERASE_ARG && !mmc_can_secure_erase_trim(host->mmc->card)))
            rawarg = MMC_TRIM_ARG;
    }
#endif
#if 0
    if (host->hw->host_function == MSDC_SD)
        host->req_count++;
#endif
    sdc_send_cmd(rawcmd, rawarg);        

    //end:      
    return 0;  // irq too fast, then cmd->error has value, and don't call msdc_command_resp, don't tune. 
}

static unsigned int msdc_command_resp_polling(struct msdc_host   *host, 
        struct mmc_command *cmd,
        int                 tune,
        unsigned long       timeout)
{
    (void)tune;
    (void)timeout;
    u32 intsts;

    u32 cmdsts = MSDC_INT_CMDRDY  | MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO;     

#ifdef MTK_MSDC_USE_CMD23
    struct mmc_command *sbc =  NULL;
    if (host->autocmd & MSDC_AUTOCMD23){
        if (host->data && host->data->mrq && host->data->mrq->sbc)
            sbc =  host->data->mrq->sbc;

        /* autocmd interupt disabled, used polling way */
        cmdsts |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO;
    }
#endif


    /*polling*/
    // tmo = jiffies + timeout;
    while (1){
        if (((intsts = sdr_read32(MSDC_INT)) & cmdsts) != 0){
            /* clear all int flag */
#ifdef MTK_MSDC_USE_CMD23
            // need clear autocmd23 comand ready interrupt
            intsts &= (cmdsts | MSDC_INT_ACMDRDY);
#else
            intsts &= cmdsts;
#endif

            sdr_write32(MSDC_INT, intsts);
            break;
        }
#if 0        
        if (time_after(jiffies, tmo)) {
            ERR_MSG("XXX CMD<%d> polling_for_completion timeout ARG<0x%.8x>",
                    cmd->opcode, cmd->arg);
            cmd->error = (unsigned int)-ETIMEDOUT;
            if (host->hw->host_function == MSDC_SD) {
                host->cmdtimeout_count++;
#ifdef CONFIG_AMAZON_METRICS_LOG
                mod_delayed_work(system_wq, &host->metrics_work,
                        METRICS_DELAY);
#endif
            }

            host->sw_timeout++;
            msdc_dump_info(host->id);
            msdc_reset_hw(host->id);
            goto out;
        }
#endif
    }

    /* command interrupts */
    if (intsts & cmdsts) {
#ifdef MTK_MSDC_USE_CMD23
        if ((intsts & MSDC_INT_CMDRDY) || (intsts & MSDC_INT_ACMD19_DONE)) {
#else
        if ((intsts & MSDC_INT_CMDRDY) || (intsts & MSDC_INT_ACMDRDY) || 
            (intsts & MSDC_INT_ACMD19_DONE)) {
#endif

            u32 *rsp = NULL;
            rsp = &cmd->resp[0];
            switch (host->cmd_rsp) {
            case RESP_NONE:
                break;
            case RESP_R2:
                *rsp++ = sdr_read32(SDC_RESP3); *rsp++ = sdr_read32(SDC_RESP2);
                *rsp++ = sdr_read32(SDC_RESP1); *rsp++ = sdr_read32(SDC_RESP0);
                break;
            default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
#ifdef MTK_SDIO30_TEST_MODE_SUPPORT
                if((intsts & MSDC_INT_ACMD53_DONE))
                {
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "Get CRC Value [B]\n");
                    *rsp++ = sdr_read32(DAT0_TUNE_CRC);
                    *rsp++ = sdr_read32(DAT1_TUNE_CRC);
                    *rsp++ = sdr_read32(DAT2_TUNE_CRC);
                    *rsp++ = sdr_read32(DAT3_TUNE_CRC);
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "Get CRC Value [E]\n");
                }
                else
#endif  // MTK_SDIO30_TEST_MODE_SUPPORT
                *rsp = sdr_read32(SDC_RESP0);    
                break;
            }
        } else if (intsts & MSDC_INT_RSPCRCERR) {
            cmd->error = (unsigned int)-EIO;
            IRQ_MSG("XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>",cmd->opcode, cmd->arg);
        /* cmd19 rsp crc error, still need receive data, so cannot call msdc_reset_hw() */
        if (cmd->opcode != MMC_SEND_TUNING_BLOCK)
            msdc_reset_hw(host->id);
#if 0
        if (host->hw->host_function == MSDC_SD) {
            if (FILTER_INVALIDCMD(cmd->opcode))
                host->crc_invalid_count++;
            else
                host->crc_count++;
#ifdef CONFIG_AMAZON_METRICS_LOG
            mod_delayed_work(system_wq, &host->metrics_work, METRICS_DELAY);
#endif
        }
#endif
    } else if (intsts & MSDC_INT_CMDTMO) {
        cmd->error = (unsigned int)-ETIMEDOUT;
        IRQ_MSG("XXX CMD<%d> MSDC_INT_CMDTMO Arg<0x%.8x>",cmd->opcode, cmd->arg);
        msdc_reset_hw(host->id);
#if 0
        if (host->hw->host_function == MSDC_SD) {
            host->cmdtimeout_count++;
#ifdef CONFIG_AMAZON_METRICS_LOG
            mod_delayed_work(system_wq, &host->metrics_work, METRICS_DELAY);
#endif
        }
#endif
    }
#ifdef MTK_MSDC_USE_CMD23
        if ((sbc != NULL) && (host->autocmd & MSDC_AUTOCMD23)) {
             if (intsts & MSDC_INT_ACMDRDY) {
                  u32 *arsp = &sbc->resp[0];
                  *arsp = sdr_read32(SDC_ACMD_RESP);
             } else if (intsts & MSDC_INT_ACMDCRCERR) {
                 printk("autocmd23 crc error\n");
                  sbc->error = (unsigned int)-EIO;
                  cmd->error = (unsigned int)-EIO; // record the error info in current cmd struct
                  //host->error |= REQ_CMD23_EIO;
                  msdc_reset_hw(host->id);
             } else if (intsts & MSDC_INT_ACMDTMO) {
                 printk("autocmd23 to error\n");
                  sbc->error =(unsigned int)-ETIMEDOUT;
                  cmd->error = (unsigned int)-ETIMEDOUT;  // record the error info in current cmd struct
                  //host->error |= REQ_CMD23_TMO;
                  msdc_reset_hw(host->id);
             }
        }
#endif /* end of MTK_MSDC_USE_CMD23 */
    }
    host->cmd = NULL;

    return cmd->error;
}

static unsigned int msdc_do_command(struct msdc_host   *host, 
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout)
{
#if 0
    if((cmd->opcode == MMC_GO_IDLE_STATE) && (host->hw->host_function == MSDC_SD)){
        mdelay(10);
    }
#endif
    
    if (msdc_command_start(host, cmd, tune, timeout)) 
        goto end;      
    if (msdc_command_resp_polling(host, cmd, tune, timeout)) 
        goto end;
end:    

    N_MSG(CMD, "        return<%d> resp<0x%.8x>\n", cmd->error, cmd->resp[0]);    
    return cmd->error;
}

unsigned int msdc_cmd(struct msdc_host *host, struct mmc_command *cmd) {
    return msdc_do_command(host, cmd, 0, 500); // 500 ms ?
}

void msdc_set_blknum(struct msdc_host *host, u32 blknum)
{
    (void)host;
    sdr_write32(SDC_BLK_NUM, blknum);
}


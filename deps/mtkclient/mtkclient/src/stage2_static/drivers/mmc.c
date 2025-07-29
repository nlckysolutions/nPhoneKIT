#include <inttypes.h>
#include "../libc.h"
#include "types.h"
#include "core.h"
#include "mmc.h"
#include "errno.h"
#include "mt_sd.h"
#include "sleepy.h"
#include "../crypto/hmac-sha256.h"

extern void hex_dump(const void* data, size_t size);

#define be32_to_cpup(addr) __builtin_bswap32(*(uint32_t*)addr)
#define be16_to_cpup(addr) __builtin_bswap16(*(uint16_t*)addr)
#define cpu_to_be16p be16_to_cpup
#define cpu_to_be32p be32_to_cpup

#define MSDC0_GPIO_PUPD0_G5_ADDR  (GPIO_REG_BASE + 0xD80)
#define MSDC0_GPIO_PUPD1_G5_ADDR  (GPIO_REG_BASE + 0xD90)

#define MSDC0_PUPD_DAT57_RSTB_MASK         (0x7777 << 0)
#define MSDC0_PUPD_CMD_DSL_CLK_DAT04_MASK  (0x77777777 << 0)

#define msdc_write32(addr, data)    mt_reg_sync_writel(data,addr)
#define msdc_read32(addr)           (*(volatile uint32_t*)(addr))

#define msdc_set_field(reg,field,val) \
    do {    \
        volatile uint32_t tv = msdc_read32(reg); \
        tv &= ~(field); \
        tv |= ((val) << (uffs(field) - 1)); \
        msdc_write32(reg,tv); \
    } while(0)

unsigned int msdc_cmd(struct msdc_host *host, struct mmc_command *cmd);

int mmc_go_idle(struct msdc_host *host)
{
    int err;
    struct mmc_command cmd = {0};

    cmd.opcode = MMC_GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_BC;

    err = msdc_cmd(host, &cmd);

    // host->use_spi_crc = 0;

    return err;
}

int mmc_all_send_cid(struct msdc_host *host, u32 *cid)
{
    int err;
    struct mmc_command cmd = {0};

    cmd.opcode = MMC_ALL_SEND_CID;
    cmd.arg = 0;
    cmd.flags = MMC_RSP_R2 | MMC_CMD_BCR;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    memcpy(cid, cmd.resp, sizeof(u32) * 4);

    return 0;
}

int mmc_send_op_cond(struct msdc_host *host, u32 ocr, u32 *rocr)
{
    struct mmc_command cmd = {0};
    int i, err = 0;

    cmd.opcode = MMC_SEND_OP_COND;
    // cmd.arg = mmc_host_is_spi(host) ? 0 : ocr;
    cmd.arg = ocr;
    cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R3 | MMC_CMD_BCR;

    for (i = 100; i; i--) {
        err = msdc_cmd(host, &cmd);
        if (err)
            break;

        /* if we're just probing, do a single pass */
        if (ocr == 0)
            break;

        /* otherwise wait until reset completes */
        if (cmd.resp[0] & MMC_CARD_BUSY)
            break;

        err = -ETIMEDOUT;

        // mmc_delay(10);
        usleep(10); // TODO
    }

    if (rocr)
        *rocr = cmd.resp[0];

    return err;
}

static u32 mmc_select_voltage(struct msdc_host *host, u32 ocr)
{
    int bit;

    ocr &= host->ocr_avail;

    bit = uffs(ocr);
    if (bit) {
        bit -= 1;
        ocr &= 3 << bit;
    } else {
        ocr = 0;
    }
    return ocr;
}

int mmc_set_relative_addr(struct msdc_host *host, uint32_t rca)
{
    int err;
    struct mmc_command cmd = {0};

    cmd.opcode = MMC_SET_RELATIVE_ADDR;
    cmd.arg = rca << 16;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    return 0;
}

static int mmc_select_card(struct mmc_host *host, uint32_t rca)
{
    int err;
    struct mmc_command cmd = {0};

    cmd.opcode = MMC_SELECT_CARD;

    cmd.arg = rca << 16;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    return 0;
}

int mmc_read(struct msdc_host *host, uint32_t blk, void *buf)
{
    int err;
    struct mmc_command cmd = { 0 };

    cmd.opcode = MMC_READ_SINGLE_BLOCK;

    cmd.arg = blk;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

    msdc_set_blknum(host, 1);

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    return msdc_pio_read(host, buf);
}

int mmc_write(struct msdc_host *host, uint32_t blk, void *buf)
{
    int err;
    struct mmc_command cmd = { 0 };

    cmd.opcode = MMC_WRITE_BLOCK;

    cmd.arg = blk;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

    msdc_set_blknum(host, 1);

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    return msdc_pio_write(host, buf);
}

int mmc_send_status(struct msdc_host *host, u32 *status)
{
    int err;
    struct mmc_command cmd = {0};

    cmd.opcode = MMC_SEND_STATUS;
    cmd.arg = 1 << 16;
    cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    /* NOTE: callers are required to understand the difference
     * between "native" and SPI format status words!
     */
    if (status)
        *status = cmd.resp[0];

    return 0;
}

/**
 *  __mmc_switch - modify EXT_CSD register
 *  @card: the MMC card associated with the data transfer
 *  @set: cmd set values
 *  @index: EXT_CSD register index
 *  @value: value to program into EXT_CSD register
 *  @timeout_ms: timeout (ms) for operation performed by register write,
 *                   timeout of zero implies maximum possible timeout
 *  @use_busy_signal: use the busy signal as response type
 *
 *  Modifies the EXT_CSD register for selected card.
 */
int __mmc_switch(struct msdc_host *host, u8 set, u8 index, u8 value,
           unsigned int timeout_ms, bool use_busy_signal)
{
    int err;
    struct mmc_command cmd = {0};
    // unsigned long timeout;
    u32 status;

    cmd.opcode = MMC_SWITCH;
    cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
          (index << 16) |
          (value << 8) |
          set;
    cmd.flags = MMC_CMD_AC;
    if (use_busy_signal)
        cmd.flags |= MMC_RSP_SPI_R1B | MMC_RSP_R1B;
    else
        cmd.flags |= MMC_RSP_SPI_R1 | MMC_RSP_R1;


    cmd.cmd_timeout_ms = timeout_ms;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    /* No need to check card status in case of unblocking command */
    if (!use_busy_signal)
        return 0;

    /* Must check status to be sure of no errors */
    // timeout = jiffies + msecs_to_jiffies(MMC_OPS_TIMEOUT_MS);
    do {
        err = mmc_send_status(host, &status);
        if (err)
            return err;
        //===ss6, bug, cmd6's status will be missed if set the WAIT_WHILE_BUSY flags
        //if (card->host->caps & MMC_CAP_WAIT_WHILE_BUSY)
            //break;
        //===
        // if (mmc_host_is_spi(card->host))
        //     break;

        /* Timeout if the device never leaves the program state. */
        // if (time_after(jiffies, timeout)) {
        //     pr_err("%s: Card stuck in programming state! %s\n",
        //         mmc_hostname(card->host), __func__);
        //     return -ETIMEDOUT;
        // }
    } while (R1_CURRENT_STATE(status) == R1_STATE_PRG);

    // if (mmc_host_is_spi(card->host)) {
    //     if (status & R1_SPI_ILLEGAL_COMMAND)
    //         return -EBADMSG;
    // } else
    {
        if (status & 0xFDFFA000)
            printf("%s: unexpected status %#x after "
                   "switch", "MSDC0", status);
        if (status & R1_SWITCH_ERROR)
            return -EBADMSG;
    }

    return 0;
}

static int mmc_rpmb_send_command(struct msdc_host *host, u8 *buf, __u16 blks,
        __u16 type, u8 req_type)
{
    struct mmc_command cmd = {0};
    struct mmc_command sbc = {0};
    int ret = 0;

    /*
     * set CMD23
     */
    sbc.opcode = MMC_SET_BLOCK_COUNT;
    // printf("blks = %d\n", blks);
    sbc.arg = blks;
    if ((req_type == RPMB_REQ) && (type == RPMB_WRITE_DATA ||
                type == RPMB_PROGRAM_KEY))
        sbc.arg |= 1 << 31;
    sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;

    ret = msdc_cmd(host, &sbc);
    if (ret) {
        printf("msdc_cmd SET_BLOCK_COUNT fail %d\n", ret);
        return ret;
    }

    msdc_set_blknum(host, blks);

    /*
     * set CMD25/18
     */
    if (req_type == RPMB_REQ) {
        cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
    } else {
        cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
    }
    cmd.arg = 0;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

    ret = msdc_cmd(host, &cmd);
    if (ret) {
        printf("msdc_cmd READ/WRITE MULTIPLE_BLOCK fail %d\n", ret);
        return ret;
    }

    // printf("and the buf:\n");
    // hex_dump(buf, 0x200);

    // this only works for a single block
    if (req_type == RPMB_REQ) {
        msdc_pio_write(host, buf);
    } else {
        msdc_pio_read(host, buf);
    }

    return 0;
}

int mmc_switch(struct msdc_host *host, u8 set, u8 index, u8 value,
        unsigned int timeout_ms)
{
    return __mmc_switch(host, set, index, value, timeout_ms, 1);
}

int mmc_set_part(struct msdc_host *host, int part) {
    return mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONFIG, 72 | part, 0);
}

int mmc_rpmb_partition_ops(struct mmc_core_rpmb_req *rpmb_req, struct msdc_host *host)
{
    int err = 0;
    struct mmc_ioc_rpmb_req *p_req;
    __u16 type, blks;
    __u8 *buf_frame;

    p_req = rpmb_req->req;
    buf_frame = rpmb_req->frame;

    if (!p_req || !rpmb_req->ready || !buf_frame) {
        printf("%s: mmc_ioc_rpmb_req is not prepared\n",
                "MSDC0");
        return -EINVAL;
    }

    type = p_req->type;
    blks = p_req->blk_cnt;

    /*
     * STEP 1: send request to RPMB partition
     */
    if (type == RPMB_WRITE_DATA)
        err = mmc_rpmb_send_command(host, buf_frame, blks,
                type, RPMB_REQ);
    else
        err = mmc_rpmb_send_command(host, buf_frame, 1, type, RPMB_REQ);

    if (err) {
        printf("%s: request write counter failed (%d)\n",
                "MSDC0", err);
        goto out;
    }

    memset(buf_frame, 0, 512 * blks);
    /*
     * STEP 2: check write result
     * Only for WRITE_DATA or Program key
     */
    if (type == RPMB_WRITE_DATA ||
            type == RPMB_PROGRAM_KEY) {
        buf_frame[RPMB_TYPE_BEG + 1] = RPMB_RESULT_READ;
        err = mmc_rpmb_send_command(host, buf_frame, 1,
                RPMB_RESULT_READ, RPMB_REQ);
        if (err) {
            printf("%s: request write counter failed (%d)\n",
                    "MSDC0", err);
            goto out;
        }
    }

    /*
     * STEP 3: get response from RPMB partition
     */

    if (type == RPMB_READ_DATA)
        err = mmc_rpmb_send_command(host, buf_frame,
                blks, type, RPMB_RESP);
    else
        err = mmc_rpmb_send_command(host, buf_frame,
                1, type, RPMB_RESP);
    if (err) {
        printf("%s: response write counter failed (%d)\n",
                "MSDC0", err);
    }
out:
    return err;
}

void mmc_rpmb_post_frame(struct mmc_core_rpmb_req *rpmb_req)
{
    int i;
    struct mmc_ioc_rpmb_req *p_req;
    __u8 *buf_frame;

    if (!rpmb_req || !rpmb_req->ready)
        return;

    p_req = rpmb_req->req;
    buf_frame = rpmb_req->frame;

    if (!p_req || !buf_frame)
        return;

    #ifdef DEBUG
    printf("post-frame:\n");
    hex_dump(buf_frame, 0x200);
    #endif
    /*
     * Regarding to the check rules, here is the post
     * rules
     * All will return result.
     * GET_WRITE_COUNTER:
     *              must: write counter, nonce
     *              optional: MAC
     * WRITE_DATA:
     *              must: MAC, write counter
     * READ_DATA:
     *              must: nonce, data
     *              optional: MAC
     * PROGRAM_KEY:
     *              must: Nothing
     *
     * Except READ_DATA, all of these operations only need to parse
     * one frame. READ_DATA needs blks frames to get DATA
     */

    memcpy(p_req->result, buf_frame + RPMB_RES_BEG, 2);
    *p_req->result = be16_to_cpup(p_req->result);

    if (p_req->type == RPMB_PROGRAM_KEY)
        goto out;

    if (p_req->type == RPMB_GET_WRITE_COUNTER ||
            p_req->type == RPMB_WRITE_DATA) {
        memcpy(p_req->wc, buf_frame + RPMB_WCOUNTER_BEG, 4);
        *p_req->wc = be32_to_cpup(p_req->wc);
    }

    if (p_req->type == RPMB_GET_WRITE_COUNTER ||
            p_req->type == RPMB_READ_DATA) {
        /* nonce copy */
        memcpy(p_req->nonce, buf_frame + RPMB_NONCE_BEG, 16);
    }
    /*
     * Take MAC within the last package
     */
    if (p_req->type == RPMB_READ_DATA) {
        __u8 *data = p_req->data;
        for (i = 0; i < p_req->blk_cnt; i++) {
            memcpy(data, buf_frame + i * 512 + RPMB_DATA_BEG, 256);
            data += 256;
        }
        /*
         * MAC stored in the last package
         */
        if (p_req->mac)
            memcpy(p_req->mac, buf_frame + i * 512 + RPMB_MAC_BEG,
                    32);
    } else if (p_req->mac)
        memcpy(p_req->mac, buf_frame + RPMB_MAC_BEG, 32);
out:
    return;
}

static int mmc_rpmb_request_check(struct msdc_host *host,
        struct mmc_ioc_rpmb_req *p_req)
{
    (void)host;
    /*
     * Some parameters are a must for the operation. Different
     * operation expect different paramters. Below code is
     * used for checking this.
     *
     * All operations will need result.
     * GET_WRITE_COUNTER:
     *              must: write counter, nonce
     *              optional: MAC
     * WRITE_DATA:
     *              must: MAC, data, write counter
     * READ_DATA:
     *              must: nonce, data
     *              optional: MAC
     * PROGRAM_KEY:
     *              must: MAC
     *
     * So here, we only check the 'must' paramters
     */
    if (!p_req->result) {
        printf("%s: Type %d has NULL pointer for result\n",
                "MSDC0", p_req->type);
        return -EINVAL;
    }

    if (p_req->type == RPMB_GET_WRITE_COUNTER) {
        if (!p_req->nonce || !p_req->wc) {
            printf("%s: Type %d has NULL pointer for nonce/wc\n",
                    "MSDC0", p_req->type);
            return -EINVAL;
        }
        /*
         * used to allocate frame
         */
        p_req->blk_cnt = 1;
    } else if (p_req->type == RPMB_WRITE_DATA ||
            p_req->type == RPMB_READ_DATA) {
#if 0
        if ((__u32)(p_req->addr + p_req->blk_cnt) >
                card->ext_csd.rpmb_size) {
            printf("%s Type %d: beyond the RPMB partition rang addr %d, blk_cnt %d, rpmb_size %d\n",
                    "MSDC0",
                    p_req->type,
                    p_req->addr,
                    p_req->blk_cnt,
                    card->ext_csd.rpmb_size);
            return -EINVAL;
        }
#endif
        if (p_req->blk_cnt == 0) {
            printf("%s: Type %d has zero block count\n",
                    "MSDC0",
                    p_req->blk_cnt);
            return -EINVAL;
        }
#if 0
 else if (p_req->blk_cnt > card->rpmb_max_req) {
            printf("%s: Type %d has invalid block count, cannot large than %d\n",
                    "MSDC0",
                    p_req->blk_cnt,
                    card->rpmb_max_req);
            return -EINVAL;
        }
#endif
        if (!p_req->data) {
            printf("%s: Type %d has NULL pointer for data\n",
                    "MSDC0", p_req->type);
            return -EINVAL;
        }
        if (p_req->type == RPMB_WRITE_DATA) {
            if (!p_req->wc || !p_req->mac) {
                printf("%s: Type %d has NULL pointer for write counter/MAC\n",
                        "MSDC0",
                        p_req->type);
                return -EINVAL;
            }
        } else {
            if (!p_req->nonce) {
                printf("%s: Type %d has NULL pointer for nonce\n",
                        "MSDC0",
                        p_req->type);
                return -EINVAL;
            }
        }
    } else if (p_req->type == RPMB_PROGRAM_KEY) {
        if (!p_req->mac) {
            printf("%s: Type %d has NULL pointer for MAC\n",
                    "MSDC0", p_req->type);
            return -EINVAL;
        }
        /*
         * used to allocate frame
         */
        p_req->blk_cnt = 1;
    } else
        return -EOPNOTSUPP;

    return 0;
}

/*
 * prepare the request of RPMB frame
 * RPMB frame is MSB first
 * convert needed bytes
 * return how many frames will be prepared
 */
int mmc_rpmb_pre_frame(struct mmc_core_rpmb_req *rpmb_req,
        struct msdc_host *host)
{
    int i, ret;
    struct mmc_ioc_rpmb_req *p_req;
    __u8 *buf_frame;
    __u16 blk_cnt, addr, type;
    __u32 w_counter;

    if (!rpmb_req || !host)
        return -EINVAL;

    p_req = rpmb_req->req;
    if (!p_req) {
        printf("%s: mmc_ioc_rpmb_req is NULL. Wrong parameter\n",
                "MSDC0");
        return -EINVAL;
    }

    /*
     * make sure these two items are clear
     */
    rpmb_req->ready = 0;

    ret = mmc_rpmb_request_check(host, p_req);
    if (ret)
        return ret;

    if (p_req->blk_cnt != 1) {
        printf("rpmb only 1 block allowed, got %d\n", p_req->blk_cnt);
        return -ENOMEM;
    }

    buf_frame = rpmb_req->frame;
    if (!buf_frame) {
        printf("%s: cannot allocate frame for type %d\n",
                "MSDC0", p_req->type);
        return -ENOMEM;
    }

    type = cpu_to_be16p(&p_req->type);
    if (p_req->type == RPMB_GET_WRITE_COUNTER ||
            p_req->type == RPMB_READ_DATA) {
        /*
         * One package prepared
         * This request needs Nonce and type
         * If is data read, then also need addr
         */
        memcpy(buf_frame + RPMB_TYPE_BEG, &type, 2);
        if (p_req->type == RPMB_READ_DATA) {
            addr = cpu_to_be16p(&p_req->addr);
            memcpy(buf_frame + RPMB_ADDR_BEG, &addr, 2);
        }
        /* convert Nonce code */
        memcpy(buf_frame + RPMB_NONCE_BEG, p_req->nonce, 16);
    } else if (p_req->type == RPMB_WRITE_DATA) {
        __u8 *data = p_req->data;
        /*
         * multiple package prepared
         * This request nees blk_cnt, addr, write_counter,
         * data and mac
         */
        blk_cnt = cpu_to_be16p(&p_req->blk_cnt);
        addr = cpu_to_be16p(&p_req->addr);
        w_counter = cpu_to_be32p(p_req->wc);
        for (i = 0; i < p_req->blk_cnt; i++) {
            memcpy(buf_frame + i * 512 + RPMB_TYPE_BEG,
                    &type, 2);
            memcpy(buf_frame + i * 512 + RPMB_BLKS_BEG,
                    &blk_cnt, 2);
            memcpy(buf_frame + i * 512 + RPMB_ADDR_BEG,
                    &addr, 2);
            memcpy(buf_frame + i * 512 + RPMB_WCOUNTER_BEG,
                    &w_counter, 4);
            memcpy(buf_frame + i * 512 + RPMB_DATA_BEG,
                    data, 256);
            data += 256;
        }
        /* convert MAC code */
        memcpy(buf_frame + 512 * (i - 1) + RPMB_MAC_BEG,
                p_req->mac, 32);
    } else if (p_req->type == RPMB_PROGRAM_KEY) {
        /*
         * One package prepared
         * This request only need mac
         */
        memcpy(buf_frame + RPMB_TYPE_BEG, &type, 2);
        /* convert MAC code */
        memcpy(buf_frame + RPMB_MAC_BEG,
                p_req->mac, 32);
    } else {
        printf("%s: We shouldn't be here\n", "MSDC0");
        return -EINVAL;
    }
    rpmb_req->ready = 1;
    return 0;
}

int mmc_rpmb_get_write_count(struct msdc_host *host, uint32_t *wc) {
    struct mmc_core_rpmb_req rpmb_req = { 0 };
    struct mmc_ioc_rpmb_req req = { 0 };
    int ret = 0;
    uint16_t result = 0;
    uint8_t nonce[32] = { 0 };
    req.type = RPMB_GET_WRITE_COUNTER;
    req.wc = wc;
    req.result = &result;
    req.nonce = nonce;
    rpmb_req.req = &req;
    /* check request */
    ret = mmc_rpmb_pre_frame(&rpmb_req, host);
    if (ret) {
        printf("%s: prepare frame failed\n", "MSDC0");
        return ret;
    }

    /*
     * before start, let's change to RPMB partition first
     */
    ret = mmc_set_part(host, 3);
    if (ret) {
        printf("mmc_set_part fail %d\n", ret);
        return ret;
    }

    ret = mmc_rpmb_partition_ops(&rpmb_req, host);
    if (ret)
        printf("%s: failed (%d) to handle RPMB request type (%d)!\n",
                "MSDC0", ret, req.type);

    mmc_rpmb_post_frame(&rpmb_req);

    printf("result = %d\n", result);

    return ret;
}

static void byteswap(uint8_t *buf, size_t sz) {
    for (size_t i = 0; i < sz / 2; ++i) {
        size_t j = sz - i - 1;
        uint8_t o = buf[j];
        buf[j] = buf[i];
        buf[i] = o;
    }
}

int mmc_rpmb_read(struct msdc_host *host, uint16_t addr, void *buf) {
    struct mmc_core_rpmb_req rpmb_req = { 0 };
    struct mmc_ioc_rpmb_req req = { 0 };
    int ret = 0;
    uint16_t result = 0;
    uint8_t nonce[32] = { 0 };
    req.type = RPMB_READ_DATA;
    req.blk_cnt = 1;
    req.result = &result;
    req.nonce = nonce;
    req.addr = addr;
    req.data = buf;
    rpmb_req.req = &req;
    /* check request */
    ret = mmc_rpmb_pre_frame(&rpmb_req, host);
    if (ret) {
        printf("%s: prepare frame failed\n", "MSDC0");
        return ret;
    }

    /*
     * before start, let's change to RPMB partition first
     */
    ret = mmc_set_part(host, 3);
    if (ret) {
        printf("mmc_set_part fail %d\n", ret);
        return ret;
    }

    ret = mmc_rpmb_partition_ops(&rpmb_req, host);
    if (ret)
        printf("%s: failed (%d) to handle RPMB request type (%d)!\n",
                "MSDC0", ret, req.type);

    mmc_rpmb_post_frame(&rpmb_req);

    printf("result = %d\n", result);

    byteswap(buf, 0x100);

    return ret;
}

uint8_t rpmb_key[32];

void rpmb_calc_mac(struct mmc_core_rpmb_req *rpmb_req) {
    struct mmc_ioc_rpmb_req *req = rpmb_req->req;

    printf("hmac over \n");
    hex_dump(rpmb_req->frame + RPMB_DATA_BEG, 512 - RPMB_DATA_BEG);
    hmac_sha256(req->mac, rpmb_req->frame + RPMB_DATA_BEG, 512 - RPMB_DATA_BEG, rpmb_key, sizeof(rpmb_key));

    printf("using key \n");
    hex_dump(rpmb_key, sizeof(rpmb_key));

    printf("results in \n");
    hex_dump(req->mac, 32);

    memcpy(rpmb_req->frame + RPMB_MAC_BEG, req->mac, 32);

    printf("frame:\n");
    hex_dump(rpmb_req->frame, 0x200);
}

int mmc_rpmb_write(struct msdc_host *host, void *buf) {
    struct mmc_core_rpmb_req rpmb_req = { 0 };
    struct mmc_ioc_rpmb_req req = { 0 };
    int ret = 0;
    uint16_t result = 0;
    uint8_t nonce[32] = { 0 };
    uint8_t mac[32] = { 0 };
    uint32_t wc;

    uint8_t tmp[0x100];
    memcpy(tmp, buf, sizeof(tmp));
    byteswap(tmp, sizeof(tmp));

    ret = mmc_rpmb_get_write_count(host, &wc);
    if (ret) {
        printf("mmc_rpmb_get_write_count %d\n", ret);
        return ret;
    }
    printf("wc = %d\n", wc);

    req.type = RPMB_WRITE_DATA;
    req.blk_cnt = 1;
    req.result = &result;
    req.nonce = nonce;
    req.addr = 0;
    req.data = tmp;
    req.wc = &wc;
    req.mac = mac;

    rpmb_req.req = &req;
    /* check request */
    ret = mmc_rpmb_pre_frame(&rpmb_req, host);
    if (ret) {
        printf("%s: prepare frame failed\n", "MSDC0");
        return ret;
    }

    rpmb_calc_mac(&rpmb_req);

    /*
     * before start, let's change to RPMB partition first
     */
    ret = mmc_set_part(host, 3);
    if (ret) {
        printf("mmc_set_part fail %d\n", ret);
        return ret;
    }

    ret = mmc_rpmb_partition_ops(&rpmb_req, host);
    if (ret)
        printf("%s: failed (%d) to handle RPMB request type (%d)!\n",
                "MSDC0", ret, req.type);

    mmc_rpmb_post_frame(&rpmb_req);

    printf("result = %d\n", result);

    return ret;
}

int mmc_init(struct msdc_host *host) {
    int ret = 0;

    host->blksz = 0x200;

    // power up msdc0
    msdc_set_field(MSDC0_GPIO_PUPD0_G5_ADDR, MSDC0_PUPD_CMD_DSL_CLK_DAT04_MASK, 0x11111661);
    msdc_set_field(MSDC0_GPIO_PUPD1_G5_ADDR, MSDC0_PUPD_DAT57_RSTB_MASK, 0x2111);

    // Disable DMA
    sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO);
    //sdr_write32(MSDC_AES_SEL, 0x0);
    usleep(1);
    sdr_write32(MSDC_CFG, sdr_read32(MSDC_CFG) | 0x1000);
    usleep(1);
    printf("MSDC_CFG: 0x%08X\n", sdr_read32(MSDC_CFG));

    ret = mmc_go_idle(host);
    printf("GO_IDLE = 0x%08X\n", ret);

    uint32_t ocr = 0;
    ret = mmc_send_op_cond(host, 0, &ocr);
    printf("SEND_OP_COND = 0x%08X ocr = 0x%08X\n", ret, ocr);

    ocr = mmc_select_voltage(host, ocr);
    ocr |= 1 << 30;
    printf("new ocr = 0x%08X\n", ocr);
    uint32_t rocr = 0;
    ret = mmc_send_op_cond(host, ocr, &rocr);
    printf("SEND_OP_COND = 0x%08X ocr = 0x%08X\n", ret, rocr);

    uint32_t cid[4] = { 0 };
    ret = mmc_all_send_cid(host, cid);
    printf("ALL_SEND_CID = 0x%08X cid = 0x%08X 0x%08X 0x%08X 0x%08X\n", ret, cid[0], cid[1], cid[2], cid[3]);

    ret = mmc_set_relative_addr(host, 1);
    printf("SET_RELATIVE_ADDR = 0x%08X\n", ret);

    ret = mmc_select_card(host, 1);
    printf("SELECT_CARD = 0x%08X\n", ret);

    return 0;
}

u32 hclks_msdc50[] = {26000000,  400000000, 200000000, 156000000,
                      182000000, 156000000, 100000000, 624000000,
                      312000000
                     };

u32 hclks_msdc30[] = {26000000,  208000000, 100000000, 156000000,
                      182000000, 156000000, 178000000, 200000000
                     };

u32 *msdc_src_clks = hclks_msdc30;

/* perloader will pre-set msdc pll and the mux channel of msdc pll */
/* note: pll will not changed */
void msdc_config_clksrc(struct mmc_host *host, u8 clksrc)
{
    (void)clksrc;
    // modify the clock
    if (host->id == 0) {
        msdc_src_clks = hclks_msdc50;
        host->pll_mux_clk = MSDC0_CLKSRC_DEFAULT;
    } else {
        msdc_src_clks = hclks_msdc30;
        host->pll_mux_clk = MSDC1_CLKSRC_DEFAULT;
    }

    /* Perloader and LK use 200 is ok, no need change source */
    //host->pll_mux_clk = MSDC_CLKSRC_DEFAULT(host->id);
    //host->src_clk = msdc_src_clks[host->pll_mux_clk];
    //msdc_printf("[info][%s] input clock is %dkHz\n", __func__, host->src_clk/1000);
}

void msdc_config_bus(u32 width)
{
    u32 val  = sdr_read32(SDC_CFG);

    val &= ~SDC_CFG_BUSWIDTH;

    switch (width) {
        case HOST_BUS_WIDTH_1:
            val |= (MSDC_BUS_1BITS << 16);
            break;
        case HOST_BUS_WIDTH_4:
            val |= (MSDC_BUS_4BITS << 16);
            break;
        case HOST_BUS_WIDTH_8:
            val |= (MSDC_BUS_8BITS << 16);
            break;
        default:
            val |= (MSDC_BUS_1BITS << 16);
            break;
    }
    sdr_write32(SDC_CFG, val);

    //MSG(INF, "[SD%d] Bus Width: %d\n", host->id, width);
}

void msdc_config_clock(struct mmc_host *host, int ddr, u32 hz, u32 hs_timing)
{
    (void)ddr;
    (void)hs_timing;
    //msdc_priv_t *priv = host->priv;
    //u32 rsmpl = msdc_cap[host->id].cmd_edge;
    //u32 base = host->base;
    u32 mode, hs400_div_dis = 0;
    u32 div;
    u32 sclk;
    //u32 orig_clksrc = host->pll_mux_clk;

    if (hz >= host->f_max) {
        hz = host->f_max;
    } else if (hz < host->f_min) {
        hz = host->f_min;
    }

    if (hz >= host->src_clk) {
        mode = 0x1; /* no divisor and divisor is ignored */
        div  = 0;
        sclk = host->src_clk;
    } else {
        mode = 0x0; /* use divisor */
        if (hz >= (host->src_clk >> 1)) {
            div  = 0;               /* mean div = 1/2 */
            sclk = host->src_clk >> 1; /* sclk = clk / 2 */
        } else {
            div  = (host->src_clk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (host->src_clk >> 2) / div;
        }
    }
    host->cur_bus_clk = sclk;

    /* set clock mode and divisor */
    sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD_HS400 | MSDC_CFG_CKMOD | MSDC_CFG_CKDIV,
                   (hs400_div_dis << (MSDC_CFG_CKMOD_BITS + MSDC_CFG_CKDIV_BITS)) |
                   (mode << MSDC_CFG_CKDIV_BITS) | div);

    //msdc_config_clksrc(host, orig_clksrc);

    /*if (mode==2 || mode==3) {
        sdr_clr_bits(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL);
    } else {
        sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL);
    }
    if (mode == 2) {
        sdr_set_bits(MSDC_IOCON, MSDC_IOCON_DDR50CKD);
    } else {
        sdr_clr_bits(MSDC_IOCON, MSDC_IOCON_DDR50CKD);
    }*/

    //msdc_init_tune_path(host, (mode ==3) ? 1 : 0);

    //msdc_pr_err("[SD%d] SET_CLK(%dkHz): SCLK(%dkHz) MODE(%d) DDR(%d) DIV(%d) DS(%d) RS(%d)\n",
    //    host->id, hz/1000, sclk/1000, mode, ddr > 0 ? 1 : 0, div,
    //    msdc_cap[host->id].data_edge, msdc_cap[host->id].cmd_edge);
}

struct msdc_cust {
    unsigned char  clk_src;           /* host clock source             */
    unsigned char  cmd_edge;          /* command latch edge            */
    unsigned char  data_edge;         /* data latch edge               */
    unsigned char  clk_drv;           /* clock pad driving             */
    unsigned char  cmd_drv;           /* command pad driving           */
    unsigned char  dat_drv;           /* data pad driving              */
    unsigned char  rst_drv;           /* reset pin pad driving         */
    unsigned char  ds_drv;            /* ds pad driving                */
    unsigned char  clk_18v_drv;       /* clock pad driving             */
    unsigned char  cmd_18v_drv;       /* command pad driving on 1.8V   */
    unsigned char  dat_18v_drv;       /* data pad driving on 1.8V      */
    unsigned char  data_pins;         /* data pins on 1.8V             */
    unsigned int   power_flash;       /* flash power status            */
    unsigned int   power_io;          /* flash power status            */
    unsigned int   flags;             /* hardware capability flags     */
};

struct msdc_cust msdc_cap[MSDC_MAX_NUM] = {
    {
        MSDC0_CLKSRC_DEFAULT, /* host clock source           */
        MSDC_SMPL_RISING,   /* command latch edge            */
        MSDC_SMPL_RISING,   /* data latch edge               */
        MSDC_DRVN_GEAR1,    /* clock pad driving             */
        MSDC_DRVN_GEAR1,    /* command pad driving           */
        MSDC_DRVN_GEAR1,    /* data pad driving              */
        MSDC_DRVN_GEAR1,    /* rst pad driving               */
        MSDC_DRVN_GEAR1,    /* ds pad driving                */
        MSDC_DRVN_DONT_CARE,/* clock pad driving on 1.8V     */
        MSDC_DRVN_DONT_CARE,/* command pad driving on 1.8V   */
        MSDC_DRVN_DONT_CARE,/* data pad driving on 1.8V      */
        8,                  /* data pins                     */
        1, 1,               /* power status                  */
        MSDC_HIGHSPEED  | MSDC_DDR
    }
};

void mmc_host_init(struct mmc_host *host){
    host->f_max = MSDC_MAX_SCLK;;
    host->f_min  = MSDC_MIN_SCLK;
    host->blkbits= MMC_BLOCK_BITS;
    host->blklen = 0;
    int clksrc = msdc_cap[host->id].clk_src;
    host->caps   = MMC_CAP_MULTIWRITE;
if (msdc_cap[host->id].flags & MSDC_HIGHSPEED)
        host->caps |= (MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED);
#if defined(FEATURE_MMC_UHS1)
    if (msdc_cap[host->id].flags & MSDC_UHS1)
        host->caps |= MMC_CAP_SD_UHS1;
#endif
    if (msdc_cap[host->id].flags & MSDC_DDR)
        host->caps |= MMC_CAP_DDR;
    if (msdc_cap[host->id].data_pins == 4)
        host->caps |= MMC_CAP_4_BIT_DATA;
    if (msdc_cap[host->id].data_pins == 8)
        host->caps |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
    if (msdc_cap[host->id].flags & MSDC_HS200)
        host->caps |= MMC_CAP_EMMC_HS200;
    if (msdc_cap[host->id].flags & MSDC_HS400)
        host->caps |= MMC_CAP_EMMC_HS400;

    host->ocr_avail = MMC_VDD_27_36;
    // msdc0 only support 1.8 IO
    if (host->caps & (MMC_CAP_EMMC_HS200 | MMC_CAP_EMMC_HS400))
        host->ocr_avail |= MMC_VDD_165_195;

    host->max_hw_segs   = MAX_DMA_TRAN_SIZE / 512;
    host->max_phys_segs = MAX_DMA_TRAN_SIZE / 512;
    host->max_seg_size  = MAX_DMA_TRAN_SIZE;
    host->max_blk_size  = 2048;
    host->max_blk_count = 65535;
    host->app_cmd = 0;
    host->app_cmd_arg = 0;

    /* set to SD/MMC mode */
    sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
    sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO);

    msdc_reset(host->id);
    msdc_clr_fifo(host->id);
    msdc_clr_int();

    /* enable SDIO mode. it's must otherwise sdio command failed */
    sdr_set_bits(SDC_CFG, SDC_CFG_SDIO);
    /* disable detect SDIO device interupt function */
    sdr_clr_bits(SDC_CFG, SDC_CFG_SDIOIDE);

    //msdc_reset_timing_register(host);

    //msdc_init_tune_path(host, 0);

    //sdr_clr_bits(MSDC_PATCH_BIT2, MSDC_PB2_SUPPORT64G);

    //void (*msdc_gpio_and_pad_init)(struct mmc_host *host) = (void *)0x220669;
    //msdc_gpio_and_pad_init(host);

    /* disable boot function, else eMMC intialization may be failed after BROM ops. */
    sdr_clr_bits(EMMC_CFG0, EMMC_CFG0_BOOTSUPP);

    /* set sampling edge */
    //sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, msdc_cap[host->id].cmd_edge);
    //sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, msdc_cap[host->id].data_edge);

    /* write crc timeout detection */
    //sdr_set_field(MSDC_PATCH_BIT0, 1 << 30, 1);
    //msdc_set_startbit(0);
    msdc_config_clksrc(host,clksrc);

    /* important lines */
    msdc_config_bus(HOST_BUS_WIDTH_1);
    msdc_config_clock(host, 0, 0x3F7A0, 0);

    //msdc_intr_unmask(0x1FF7B);
    //msdc_set_timeout(host, 100000000, 0);

    //if ((host->id == 0) || (host->id == 1)) {
        /* disable SDIO func */
    //    sdr_set_field(SDC_CFG, SDC_CFG_SDIO, 0);
    //    sdr_set_field(SDC_CFG, SDC_CFG_SDIOIDE, 0);
    //    sdr_set_field(SDC_CFG, SDC_CFG_INSWKUP, 0);
    //}
}

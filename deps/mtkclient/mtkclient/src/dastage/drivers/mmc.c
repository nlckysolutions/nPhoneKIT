#include <inttypes.h>
#include "../libc.h"
#include "types.h"
#include "core.h"
#include "mmc.h"
#include "errno.h"
#include "mt_sd.h"
#include "../crypto/hmac-sha256.h"

#define be32_to_cpup(addr) __builtin_bswap32(*(uint32_t*)addr)
#define be16_to_cpup(addr) __builtin_bswap16(*(uint16_t*)addr)
#define cpu_to_be16p be16_to_cpup
#define cpu_to_be32p be32_to_cpup

unsigned int msdc_cmd(struct msdc_host *host, struct mmc_command *cmd);
void sleepy(void);
void hex_dump(const void* data, size_t size);

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
        sleepy(); // TODO
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

static int mmc_select_card(struct msdc_host *host, uint32_t rca)
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


    printf("post-frame:\n");
    hex_dump(buf_frame, 0x200);

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

static void sej_init(int arg) {
    int param = 0;
    if ( arg )
        param = 3;
    else
        param = 2;

    sdr_write32(0x1000A020, 0);
    sdr_write32(0x1000A024, 0);
    sdr_write32(0x1000A028, 0);
    sdr_write32(0x1000A02C, 0);
    sdr_write32(0x1000A030, 0);
    sdr_write32(0x1000A034, 0);
    sdr_write32(0x1000A038, 0);
    sdr_write32(0x1000A03C, 0);
    sdr_write32(0x1000A004, 2);
    sdr_write32(0x1000A00C, 272);
    sdr_write32(0x1000A008, 2);
    sdr_write32(0x1000A040, 0x9ED40400);
    sdr_write32(0x1000A044, 0xE884A1);
    sdr_write32(0x1000A048, 0xE3F083BD);
    sdr_write32(0x1000A04C, 0x2F4E6D8A);

    uint32_t magic[12] = { 
        0x2D44BB70,
        0xA744D227,
        0xD0A9864B,
        0x83FFC244,
        0x7EC8266B,
        0x43E80FB2,
        0x1A6348A,
        0x2067F9A0,
        0x54536405,
        0xD546A6B1,
        0x1CC3EC3A,
        0xDE377A83
    };

    for (int i = 0; i < 3; ++i) {
        int pos = i * 4;
        sdr_write32(0x1000A010, magic[pos]);
        sdr_write32(0x1000A014, magic[pos + 1]);
        sdr_write32(0x1000A018, magic[pos + 2]);
        sdr_write32(0x1000A01C, magic[pos + 3]);
        sdr_write32(0x1000A008, 1);
        while ( !(sdr_read32(0x1000A008) & 0x8000) )
          ;
    }

    sdr_write32(0x1000A008, 2);
    sdr_write32(0x1000A040, 0x9ED40400);
    sdr_write32(0x1000A044, 0xE884A1);
    sdr_write32(0x1000A048, 0xE3F083BD);
    sdr_write32(0x1000A04C, 0x2F4E6D8A);
    sdr_write32(0x1000A004, param);
    sdr_write32(0x1000A00C, 0);
}

static void sej_run(uint32_t *buf1, size_t len, char *buf2) {
    char *i;
    for ( i = buf2; (size_t)(i - buf2) < len; *(uint32_t *)(i - 4) = sdr_read32(0x1000A05C) )
    {
        sdr_write32(0x1000A010, buf1[0]);
        sdr_write32(0x1000A014, buf1[1]);
        sdr_write32(0x1000A018, buf1[2]);
        sdr_write32(0x1000A01C, buf1[3]);
        sdr_write32(0x1000A008, 1);
        while ( !(sdr_read32(0x1000A008) & 0x8000) )
          ;
        buf1 += 4;
        i += 16;
        *(uint32_t *)(i - 16) = sdr_read32(0x1000A050);
        *(uint32_t *)(i - 12) = sdr_read32(0x1000A054);
        *(uint32_t *)(i - 8) = sdr_read32(0x1000A058);
    }
}

static void sej_fini() {
    sdr_write32(0x1000A008, 2);
    sdr_write32(0x1000A020, 0);
    sdr_write32(0x1000A024, 0);
    sdr_write32(0x1000A028, 0);
    sdr_write32(0x1000A02C, 0);
    sdr_write32(0x1000A030, 0);
    sdr_write32(0x1000A034, 0);
    sdr_write32(0x1000A038, 0);
    sdr_write32(0x1000A03C, 0);
}

static void sej_encrypt(void *buf, size_t len, void *buf2) {
    // printf("orig:\n");
    // hex_dump(buf, len);

    // printf("sej init\n");
    sej_init(1);
    // printf("sej run\n");
    sej_run(buf, len, buf2);
    // printf("sej fini\n");
    sej_fini();

    // printf("result:\n");
    // hex_dump(buf, len);
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
    // sej_encrypt(req->mac, 32, req->mac);
}

int mmc_rpmb_write(struct msdc_host *host, uint16_t addr, void *buf) {
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
    req.addr = addr;
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

static void derive_rpmb_key(uint8_t *in) {
    printf("in:\n");
    hex_dump(in, 16);
    printf("\n");

    uint8_t expand[32] = { 0 };
    for (int i = 0; i < 32; ++i) {
        expand[i] = in[i % 16];
    }

    printf("expand:\n");
    hex_dump(expand, 32);
    printf("\n");

    sej_encrypt(expand, 32, expand);
    printf("encrypted:\n");
    hex_dump(expand, 32);
    printf("\n");

    byteswap(expand, 32);
    printf("final:\n");
    hex_dump(expand, 32);
    printf("\n");

    memcpy(rpmb_key, expand, 32);
}

int mmc_init(struct msdc_host *host) {
    int ret = 0;

    host->blksz = 0x200;

    sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO);
    sleepy();
    sdr_write32(MSDC_CFG, sdr_read32(MSDC_CFG) | 0x1000);
    sleepy();
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

    uint32_t cid_be[4] = { 0 };
    for (int i = 0; i < 4; ++i)
        cid_be[i] = __builtin_bswap32(cid[i]);
    //derive_rpmb_key((void*)cid_be);

    ret = mmc_set_relative_addr(host, 1);
    printf("SET_RELATIVE_ADDR = 0x%08X\n", ret);

    ret = mmc_select_card(host, 1);
    printf("SELECT_CARD = 0x%08X\n", ret);

    return 0;
}


/* each PLL have different gears for select
 * software can used mux interface from clock management module to select */
#define MSDC_SMPL_RISING        (0)
#define MSDC_MAX_NUM            (1)
#define MSDC_HIGHSPEED      (1 << 7)  /* high-speed mode support       */
#define MSDC_DDR            (1 << 9)  /* ddr mode support              */
#define MSDC_DRVN_GEAR1         0x1
#define MSDC_DRVN_DONT_CARE     0x0


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

enum {
    MSDC3_CLKSRC_26MHZ   = 0,
    MSDC3_CLKSRC_208MHZ,
    MSDC3_CLKSRC_400MHZ,
    MSDC3_CLKSRC_156MHZ,
    MSDC3_CLKSRC_182MHZ,
    MSDC3_CLKSRC_312MHZ,
    MSDC3_CLKSRC_364MHZ,
    MSDC3_CLKSRC_200MHZ,
    MSDC3_CLKSRC_MAX
};


#define MSDC0_CLKSRC_DEFAULT    MSDC0_CLKSRC_400MHZ
#define MSDC1_CLKSRC_DEFAULT    MSDC1_CLKSRC_200MHZ
#define MSDC3_CLKSRC_DEFAULT    MSDC3_CLKSRC_400MHZ

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

#define MSDC_PB2_SUPPORT64G     (0x1 << 1)
#define MSDC_IOCON_R_D_SMPL     (0x1  << 2) /* RW */
#define MSDC_CFG_START_BIT      (0x3  << 23)    /* RW */

void msdc_set_startbit(u8 start_bit)
{
    /* set start bit */
    //MSG(INF, "msdc_set_startbit %d\n", (int)start_bit);
    sdr_set_field(MSDC_CFG, MSDC_CFG_START_BIT, start_bit);
}

#define HOST_BUS_WIDTH_1            (1)
#define HOST_BUS_WIDTH_4            (4)
#define HOST_BUS_WIDTH_8            (8)


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

#define MSDC0_IO_PAD_BASE   (0x10002000)/* IOCFG_1_BASE */
#define MSDC0_SMT_ALL_MASK              (0x1f <<  0)

#define MSDC0_GPIO_MODE0       (MSDC_GPIO_BASE + 0x3f0)
#define MSDC0_GPIO_MODE1       (MSDC_GPIO_BASE + 0x400)
#define MSDC0_GPIO_MODE_TRAP   (MSDC_GPIO_BASE + 0x6f0)
#define MSDC0_GPIO_IES     (MSDC0_IO_PAD_BASE + 0x10)
#define MSDC0_GPIO_SMT     (MSDC0_IO_PAD_BASE + 0x80)
#define MSDC0_GPIO_TDSEL   (MSDC0_IO_PAD_BASE + 0x90)
#define MSDC0_GPIO_RDSEL   (MSDC0_IO_PAD_BASE + 0x70)
#define MSDC0_GPIO_DRV     (MSDC0_IO_PAD_BASE + 0)
#define MSDC0_GPIO_PUPD   (MSDC0_IO_PAD_BASE + 0x30)
#define MSDC0_GPIO_R0   (MSDC0_IO_PAD_BASE + 0x50)
#define MSDC0_GPIO_R1   (MSDC0_IO_PAD_BASE + 0x60)


#define MSDC0_GPIO_MODE16       (MSDC_GPIO_BASE + 0x3F0)
#define MSDC0_GPIO_MODE17       (MSDC_GPIO_BASE + 0x400)
#define MSDC0_GPIO_IES_ADDR     (MSDC0_IO_PAD_BASE + 0x0)
#define MSDC0_GPIO_SMT_ADDR     (MSDC0_IO_PAD_BASE + 0x10)
#define MSDC0_GPIO_TDSEL0_ADDR  (MSDC0_IO_PAD_BASE + 0x20)
#define MSDC0_GPIO_RDSEL0_ADDR  (MSDC0_IO_PAD_BASE + 0x40)
#define MSDC0_GPIO_DRV0_ADDR    (MSDC0_IO_PAD_BASE + 0xa0)
#define MSDC0_GPIO_PUPD0_ADDR   (MSDC0_IO_PAD_BASE + 0xc0)
#define MSDC0_GPIO_PUPD1_ADDR   (MSDC0_IO_PAD_BASE + 0xd0)

/* MSDC1 */
#define MSDC1_GPIO_MODE4        (MSDC_GPIO_BASE + 0x330)
#define MSDC1_GPIO_MODE5        (MSDC_GPIO_BASE + 0x340)
#define MSDC1_GPIO_IES_ADDR     (MSDC1_IO_PAD_BASE + 0x0)
#define MSDC1_GPIO_SMT_ADDR     (MSDC1_IO_PAD_BASE + 0x10)
#define MSDC1_GPIO_TDSEL0_ADDR  (MSDC1_IO_PAD_BASE + 0x20)
#define MSDC1_GPIO_RDSEL0_ADDR  (MSDC1_IO_PAD_BASE + 0x40)
#define MSDC1_GPIO_DRV0_ADDR    (MSDC1_IO_PAD_BASE + 0xa0)
#define MSDC1_GPIO_PUPD0_ADDR   (MSDC1_IO_PAD_BASE + 0xc0)

void msdc_intr_unmask(u32 bits)
{
    u32 val;

    val  = sdr_read32(MSDC_INTEN);
    val |= bits;
    sdr_write32(MSDC_INTEN, val);
}

#define msdc_set_smt(host, set_smt) \
    msdc_set_smt_by_id(host->id, set_smt)

void msdc_set_smt_by_id(u32 id, int set_smt)
{
    if (id == 0) {
        sdr_set_field(MSDC0_GPIO_SMT_ADDR, MSDC0_SMT_ALL_MASK,
            (set_smt ? 0x1F : 0));
    }
}

#define MSDC_OP_SCLK            (200000000)
#define MSDC_MAX_SCLK           (200000000)
#define MSDC_MIN_SCLK           (260000)
#define MMC_BLOCK_BITS                  (9)
#define MMC_BLOCK_SIZE                  (1 << MMC_BLOCK_BITS)
#define MMC_MAX_BLOCK_SIZE              (1 << MMC_BLOCK_BITS)
#define MMC_CAP_4_BIT_DATA      (1 << 0) /* Can the host do 4 bit transfers */
#define MMC_CAP_MULTIWRITE      (1 << 1) /* Can accurately report bytes sent to card on error */
#define MMC_CAP_BYTEBLOCK       (1 << 2) /* Can do non-log2 block sizes */
#define MMC_CAP_MMC_HIGHSPEED   (1 << 3) /* Can do MMC high-speed timing */
#define MMC_CAP_SD_HIGHSPEED    (1 << 4) /* Can do SD high-speed timing */
#define MMC_CAP_8_BIT_DATA      (1 << 5) /* Can the host do 8 bit transfers */
#define MMC_CAP_SD_UHS1         (1 << 6) /* Can do SD ultra-high-speed timing */
#define MMC_CAP_DDR             (1 << 7) /* The host support dual data rate */
#define MMC_CAP_EMMC_HS200      (1 << 8) /* The host support dual data rate */
#define MMC_CAP_EMMC_HS400      (1 << 9) /* The host support dual data rate */
#define MMC_VDD_27_36       0x00FF8000
#define MAX_BD_POOL_SZ      (4)
#define MAX_DMA_CNT     (8*1024*1024)
#define MAX_SG_POOL_SZ      (MAX_BD_POOL_SZ)
#define MAX_SG_BUF_SZ       (MAX_DMA_CNT)
#define MAX_DMA_TRAN_SIZE   ((u64)MAX_SG_POOL_SZ*MAX_SG_BUF_SZ)
#define MSDC_HS200          (1 << 10) /* hs200 mode support(eMMC4.5)   */
#define MSDC_HS400          (1 << 11) /* hs200 mode support(eMMC5.0)   */


void msdc_reset_timing_register(struct mmc_host *host)
{
    u32 base = host->base;

    sdr_write32(MSDC_IOCON, 0x00000000);
    sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
    sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);
    sdr_write32(MSDC_DAT_RDDLY2, 0x00000000);
    sdr_write32(MSDC_DAT_RDDLY3, 0x00000000);
    sdr_write32(MSDC_PATCH_BIT0, MSDC_PB0_DEFAULT);
    sdr_write32(MSDC_PATCH_BIT1, MSDC_PB1_DEFAULT);
    sdr_write32(MSDC_PATCH_BIT2, MSDC_PB2_DEFAULT);
    sdr_write32(MSDC_PAD_TUNE0, 0);
    sdr_write32(MSDC_PAD_TUNE1, 0);
}

/* MSDC_PATCH_BIT0 mask */
#define MSDC_PB0_EN_START_BIT_CHK_SUP   (0x1 << 0)
#define MSDC_PB0_EN_8BITSUP             (0x1 << 1)
#define MSDC_PB0_DIS_RECMDWR            (0x1 << 2)
#define MSDC_PB0_RD_DAT_SEL             (0x1 << 3)
#define MSDC_PB0_RESV2                  (0x3 << 4)
#define MSDC_PB0_DESCUP                 (0x1 << 6)
#define MSDC_PB0_INT_DAT_LATCH_CK_SEL   (0x7 << 7)
#define MSDC_PB0_CKGEN_MSDC_DLY_SEL     (0x1F<<10)
#define MSDC_PB0_FIFORD_DIS             (0x1 << 15)
#define MSDC_PB0_BLKNUM_SEL             (0x1 << 16)
#define MSDC_PB0_SDIO_INTCSEL           (0x1 << 17)
#define MSDC_PB0_SDC_BSYDLY             (0xf << 18)
#define MSDC_PB0_SDC_WDOD               (0xf << 22)
#define MSDC_PB0_CMDIDRTSEL             (0x1 << 26)
#define MSDC_PB0_CMDFAILSEL             (0x1 << 27)
#define MSDC_PB0_SDIO_INTDLYSEL         (0x1 << 28)
#define MSDC_PB0_SPCPUSH                (0x1 << 29)
#define MSDC_PB0_DETWR_CRCTMO           (0x1 << 30)
#define MSDC_PB0_EN_DRVRSP              (0x1UL << 31)

/* MSDC_PATCH_BIT1 mask */
#define MSDC_PB1_WRDAT_CRCS_TA_CNTR     (0x7 << 0)
#define MSDC_PB1_CMD_RSP_TA_CNTR        (0x7 << 3)
#define MSDC_PB1_GET_BUSY_MA            (0x1 << 6)
#define MSDC_PB1_CHECK_BUSY_SEL         (0x1 << 7)
#define MSDC_PB1_STOP_DLY_SEL           (0xf << 8)
#define MSDC_PB1_BIAS_EN18IO_28NM       (0x1 << 12)
#define MSDC_PB1_BIAS_EXT_28NM          (0x1 << 13)
#define MSDC_PB1_DDR_CMD_FIX_SEL        (0x1 << 14)
#define MSDC_PB1_RESET_GDMA             (0x1 << 15)
#define MSDC_PB1_SINGLE_BURST           (0x1 << 16)
#define MSDC_PB1_FROCE_STOP             (0x1 << 17)
#define MSDC_PB1_DCM_EN                 (0x1 << 21)
#define MSDC_PB1_AXI_WRAP_CKEN          (0x1 << 22)
#define MSDC_PB1_CKCLK_GDMA_EN          (0x1 << 23)
#define MSDC_PB1_CKSPCEN                (0x1 << 24)
#define MSDC_PB1_CKPSCEN                (0x1 << 25)
#define MSDC_PB1_CKVOLDETEN             (0x1 << 26)
#define MSDC_PB1_CKACMDEN               (0x1 << 27)
#define MSDC_PB1_CKSDEN                 (0x1 << 28)
#define MSDC_PB1_CKWCTLEN               (0x1 << 29)
#define MSDC_PB1_CKRCTLEN               (0x1 << 30)
#define MSDC_PB1_CKSHBFFEN              (0x1UL << 31)

//#define MSDC_PB0_DEFAULT        0x403C0006
//#define MSDC_PB1_DEFAULT        0xFFE20349
//#define MSDC_PB2_DEFAULT        0x14801803

#define CMD_RSP_TA_CNTR_DEFAULT         0
#define WRDAT_CRCS_TA_CNTR_DEFAULT      0
#define BUSY_MA_DEFAULT                 1

#define CRCSTSENSEL_HS400_DEFAULT       3
#define RESPSTENSEL_HS400_DEFAULT       3
#define CRCSTSENSEL_HS_DEFAULT          1
#define RESPSTENSEL_HS_DEFAULT          1
#define CRCSTSENSEL_FPGA_DEFAULT        0


/* MSDC_PATCH_BIT2 mask */
#define MSDC_PB2_ENHANCEGPD             (0x1 << 0)
#define MSDC_PB2_SUPPORT64G             (0x1 << 1)
#define MSDC_PB2_RESPWAITCNT            (0x3 << 2)
#define MSDC_PB2_CFGRDATCNT             (0x1f << 4)
#define MSDC_PB2_CFGRDAT                (0x1 << 9)
#define MSDC_PB2_INTCRESPSEL            (0x1 << 11)
#define MSDC_PB2_CFGRESPCNT             (0x7 << 12)
#define MSDC_PB2_CFGRESP                (0x1 << 15)
#define MSDC_PB2_RESPSTENSEL            (0x7 << 16)
#define MSDC_PB2_DDR50_SEL              (0x1 << 19)
#define MSDC_PB2_POPENCNT               (0xf << 20)
#define MSDC_PB2_CFG_CRCSTS_SEL         (0x1 << 24)
#define MSDC_PB2_CFGCRCSTSEDGE          (0x1 << 25)
#define MSDC_PB2_CFGCRCSTSCNT           (0x3 << 26)
#define MSDC_PB2_CFGCRCSTS              (0x1 << 28)
#define MSDC_PB2_CRCSTSENSEL            (0x7UL << 29)


/* MSDC_PAD_TUNE mask */
#define MSDC_PAD_TUNE0_DATWRDLY         (0x1F <<  0)     /* RW */
#define MSDC_PAD_TUNE0_DELAYEN          (0x1  <<  7)     /* RW */
#define MSDC_PAD_TUNE0_DATRRDLY         (0x1F <<  8)     /* RW */
#define MSDC_PAD_TUNE0_DATRRDLYSEL      (0x1  << 13)     /* RW */
#define MSDC_PAD_TUNE0_RXDLYSEL         (0x1  << 15)     /* RW */
#define MSDC_PAD_TUNE0_CMDRDLY          (0x1F << 16)     /* RW */
#define MSDC_PAD_TUNE0_CMDRRDLYSEL      (0x1  << 21)     /* RW */
#define MSDC_PAD_TUNE0_CMDRRDLY         (0x1FUL << 22)   /* RW */
#define MSDC_PAD_TUNE0_CLKTXDLY         (0x1FUL << 27)   /* RW */

/* MSDC_PAD_TUNE1 mask */
#define MSDC_PAD_TUNE1_DATRRDLY2        (0x1F <<  8)     /* RW */
#define MSDC_PAD_TUNE1_DATRRDLY2SEL     (0x1  << 13)     /* RW */
#define MSDC_PAD_TUNE1_CMDRDLY2         (0x1F << 16)     /* RW */
#define MSDC_PAD_TUNE1_CMDRRDLY2SEL     (0x1  << 21)     /* RW */

/* MSDC_IOCON mask */
#define MSDC_IOCON_SDR104CKS            (0x1  << 0)     /* RW */
#define MSDC_IOCON_RSPL                 (0x1  << 1)     /* RW */
#define MSDC_IOCON_R_D_SMPL             (0x1  << 2)     /* RW */
#define MSDC_IOCON_DDLSEL               (0x1  << 3)     /* RW */
#define MSDC_IOCON_DDR50CKD             (0x1  << 4)     /* RW */
#define MSDC_IOCON_R_D_SMPL_SEL         (0x1  << 5)     /* RW */
#define MSDC_IOCON_W_D_SMPL             (0x1  << 8)     /* RW */
#define MSDC_IOCON_W_D_SMPL_SEL         (0x1  << 9)     /* RW */
#define MSDC_IOCON_W_D0SPL              (0x1  << 10)    /* RW */
#define MSDC_IOCON_W_D1SPL              (0x1  << 11)    /* RW */
#define MSDC_IOCON_W_D2SPL              (0x1  << 12)    /* RW */
#define MSDC_IOCON_W_D3SPL              (0x1  << 13)    /* RW */
#define MSDC_IOCON_R_D0SPL              (0x1  << 16)    /* RW */
#define MSDC_IOCON_R_D1SPL              (0x1  << 17)    /* RW */
#define MSDC_IOCON_R_D2SPL              (0x1  << 18)    /* RW */
#define MSDC_IOCON_R_D3SPL              (0x1  << 19)    /* RW */
#define MSDC_IOCON_R_D4SPL              (0x1  << 20)    /* RW */
#define MSDC_IOCON_R_D5SPL              (0x1  << 21)    /* RW */
#define MSDC_IOCON_R_D6SPL              (0x1  << 22)    /* RW */
#define MSDC_IOCON_R_D7SPL              (0x1  << 23)    /* RW */


/* EMMC51_CFG0 mask */
#define MSDC_EMMC51_CFG_CMDQEN          (0x1    <<  0)
#define MSDC_EMMC51_CFG_NUM             (0x3F   <<  1)
#define MSDC_EMMC51_CFG_RSPTYPE         (0x7    <<  7)
#define MSDC_EMMC51_CFG_DTYPE           (0x3    << 10)
#define MSDC_EMMC51_CFG_RDATCNT         (0x3FF  << 12)
#define MSDC_EMMC51_CFG_WDATCNT         (0x3FF  << 22)

/* EMMC50_CFG0 mask */
#define MSDC_EMMC50_CFG_PADCMD_LATCHCK  (0x1 << 0)
#define MSDC_EMMC50_CFG_CRC_STS_CNT     (0x3 << 1)
#define MSDC_EMMC50_CFG_CRC_STS_EDGE    (0x1 << 3)
#define MSDC_EMMC50_CFG_CRC_STS_SEL     (0x1 << 4)
#define MSDC_EMMC50_CFG_END_BIT_CHK_CNT (0xf << 5)
#define MSDC_EMMC50_CFG_CMD_RESP_SEL    (0x1 << 9)
#define MSDC_EMMC50_CFG_CMD_EDGE_SEL    (0x1 << 10)
#define MSDC_EMMC50_CFG_ENDBIT_CNT      (0x3FF << 11)
#define MSDC_EMMC50_CFG_READ_DAT_CNT    (0x7 << 21)
#define MSDC_EMMC50_CFG_EMMC50_MON_SEL  (0x1 << 24)
#define MSDC_EMMC50_CFG_TXSKEW_SEL      (0x1 << 29)

/* EMMC50_CFG1 mask */
#define MSDC_EMMC50_CFG1_CKSWITCH_CNT   (0x7  << 8)
#define MSDC_EMMC50_CFG1_RDDAT_STOP     (0x1  << 11)
#define MSDC_EMMC50_CFG1_WAITCLK_CNT    (0xF  << 12)
#define MSDC_EMMC50_CFG1_DBG_SEL        (0xFF << 16)
#define MSDC_EMMC50_CFG1_PSHCNT         (0x7  << 24)
#define MSDC_EMMC50_CFG1_PSHPSSEL       (0x1  << 27)
#define MSDC_EMMC50_CFG1_DSCFG          (0x1  << 28)
#define MSDC_EMMC50_CFG1_SPARE1         (0x7UL << 29)


void msdc_init_tune_path(struct mmc_host *host, int hs400)
{
    u32 base = host->base;

    sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_RESPWAITCNT, 3);

    sdr_clr_bits(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_RXDLYSEL);

    sdr_clr_bits(MSDC_IOCON, MSDC_IOCON_DDLSEL);
    sdr_clr_bits(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL);

    #if !defined(FPGA_PLATFORM)
    sdr_clr_bits(MSDC_IOCON, MSDC_IOCON_R_D_SMPL);
    sdr_clr_bits(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL);
    #else
    sdr_set_bits(MSDC_IOCON, MSDC_IOCON_R_D_SMPL);
    sdr_clr_bits(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL);
    #endif

    #if defined(MMC_MSDC_DRV_CTP) || defined(SLT)
    if (hs400) {
        sdr_clr_bits(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL);
        sdr_clr_bits(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL);
    } else
    #endif
    {
        sdr_set_bits(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL);
        sdr_clr_bits(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL);
    }

    #if defined(MMC_MSDC_DRV_CTP) || defined(SLT)
    if (hs400)
        sdr_clr_bits(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
    else
    #endif
        sdr_set_bits(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);

    sdr_clr_bits(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL);

    sdr_clr_bits(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP);
    sdr_set_bits(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLYSEL);
    sdr_clr_bits(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRRDLY2SEL);

    if (host->id != 1)
        sdr_clr_bits(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL);

    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB0_CKGEN_MSDC_DLY_SEL, 0);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,
        CMD_RSP_TA_CNTR_DEFAULT);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
        WRDAT_CRCS_TA_CNTR_DEFAULT);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA,
        BUSY_MA_DEFAULT);

    #if !defined(FPGA_PLATFORM)
    #if defined(MMC_MSDC_DRV_CTP)
    if (hs400) {
        sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
            CRCSTSENSEL_HS400_DEFAULT);
        sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
            RESPSTENSEL_HS400_DEFAULT);
    } else
    #endif
    {
        sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
            CRCSTSENSEL_HS_DEFAULT);
        sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL,
            RESPSTENSEL_HS_DEFAULT);
    }
    #else
    if (!hs400) {
        sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,
            CRCSTSENSEL_FPGA_DEFAULT);
    }
    #endif

    sdr_set_bits(MSDC_PATCH_BIT1, MSDC_PB1_DDR_CMD_FIX_SEL);

    /* DDR50 mode */
    sdr_set_bits(MSDC_PATCH_BIT2, MSDC_PB2_DDR50_SEL);
    /* set SDC_RX_ENHANCE_EN for async-fifo RX tune */
    sdr_set_field(SDC_ADV_CFG0, SDC_ADV_CFG0_SDC_RX_ENH_EN, 1);
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
    host->src_clk = msdc_src_clks[host->pll_mux_clk];
    //msdc_printf("[info][%s] input clock is %dkHz\n", __func__, host->src_clk/1000);
}

void msdc_set_dmode(struct mmc_host *host, int mode)
{
    /*if (mode == MSDC_MODE_PIO) {
        host->blk_read  = msdc_pio_bread;
        host->blk_write = msdc_pio_bwrite;
    }*/
}

#define GPIO_BASE   (0x10005000)/* IOCFG_BASE */
#define IOCFG_LT_BASE   (0x10002000)/* IOCFG_1_BASE */
#define IOCFG_LM_BASE   (0x10002200)/* IOCFG_2_BASE */
#define IOCFG_LB_BASE   (0x10002400)/* IOCFG_3_BASE */
#define IOCFG_BL_BASE   (0x10002600)/* IOCFG_4_BASE */
#define IOCFG_RR_BASE   (0x10002800)/* IOCFG_5_BASE */
#define IOCFG_RB_BASE   (0x10002A00)/* IOCFG_6_BASE */
#define IOCFG_RT_BASE   (0x10002C00)/* IOCFG_7_BASE */

#define MSDC_GPIO_BASE          GPIO_BASE
/* MSDC0 TDSEL0 mask */
#define MSDC0_TDSEL0_RSTB_MASK   (0xF << 16)
#define MSDC0_TDSEL0_DSL_MASK    (0xF << 12)
#define MSDC0_TDSEL0_CLK_MASK    (0xF << 8)
#define MSDC0_TDSEL0_DAT_MASK    (0xF << 4)
#define MSDC0_TDSEL0_CMD_MASK    (0xF << 0)
#define MSDC0_TDSEL0_ALL_MASK    (0xFFFFF << 0)
/* MSDC0 RDSEL0 mask */
#define MSDC0_RDSEL0_RSTB_MASK   (0x3F << 24)
#define MSDC0_RDSEL0_DSL_MASK    (0x3F << 18)
#define MSDC0_RDSEL0_CLK_MASK    (0x3F << 12)
#define MSDC0_RDSEL0_DAT_MASK    (0x3F << 6)
#define MSDC0_RDSEL0_CMD_MASK    (0x3F << 0)
#define MSDC0_RDSEL0_ALL_MASK    (0x3FFFFFFF << 0)

#define MSDC_TDRDSEL_SLEEP      (0)
#define MSDC_TDRDSEL_3V         (1)
#define MSDC_TDRDSEL_1V8        (2)
#define MSDC_TDRDSEL_CUST       (3)

void msdc_set_tdsel_by_id(u32 id, u32 flag, u32 value)
{
    u32 cust_val;

    if (id == 0) {
        if (flag == MSDC_TDRDSEL_CUST)
            cust_val = value;
        else
            cust_val = 0;
        sdr_set_field(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_CMD_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_DAT_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_CLK_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_RSTB_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_DSL_MASK,
            cust_val);
    }
}

void msdc_set_rdsel_by_id(u32 id, u32 flag, u32 value)
{
    u32 cust_val;

    if (id == 0) {
        if (flag == MSDC_TDRDSEL_CUST)
            cust_val = value;
        else
            cust_val = 0;
        sdr_set_field(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_CMD_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_DAT_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_CLK_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_RSTB_MASK,
            cust_val);
        sdr_set_field(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_DSL_MASK,
            cust_val);
    }
}

void msdc_set_tdsel_wrap(struct mmc_host *host)
{
    if (host->cur_pwr == VOL_1800)
        msdc_set_tdsel_by_id(host->id, MSDC_TDRDSEL_1V8, 0);
    else
        msdc_set_tdsel_by_id(host->id, MSDC_TDRDSEL_3V, 0);
}

void msdc_set_rdsel_wrap(struct mmc_host *host)
{
    if (host->cur_pwr == VOL_1800)
        msdc_set_rdsel_by_id(host->id, MSDC_TDRDSEL_1V8, 0);
    else
        msdc_set_rdsel_by_id(host->id, MSDC_TDRDSEL_3V, 0);
}


void msdc_set_pin_mode(struct mmc_host *host)
{
    if (host->id == 0) {
        if ((sdr_read32(MSDC0_GPIO_MODE_TRAP) & (0x3 << 13)) == 0) {
            sdr_set_field(MSDC0_GPIO_MODE0, 0x777777 << 8, 0x222222);
            sdr_set_field(MSDC0_GPIO_MODE1, 0x777777 << 0, 0x222222);
        } else {
            sdr_set_field(MSDC0_GPIO_MODE0, 0x777777 << 8, 0x111111);
            sdr_set_field(MSDC0_GPIO_MODE1, 0x777777 << 0, 0x111111);
        }

    } else if (host->id == 1) {
    } else if (host->id == 3) {
    }
}

/* MSDC0 DRV0 mask */
#define MSDC0_DRV0_DSL_MASK      (0x7 << 9)
#define MSDC0_DRV0_DAT_MASK      (0x7 << 6)
#define MSDC0_DRV0_CMD_MASK      (0x7 << 3)
#define MSDC0_DRV0_CLK_MASK      (0x7 << 0)
#define MSDC0_DRV0_ALL_MASK      (0xFFF << 0)

void msdc_set_driving_by_id(u32 id, struct msdc_cust *msdc_cap)
{
    switch (id) {
    case 0:
        sdr_set_field(MSDC0_GPIO_DRV, 0x7 << 15,
        msdc_cap->ds_drv);
        sdr_set_field(MSDC0_GPIO_DRV, 0x7 << 18,
            msdc_cap->rst_drv);
        sdr_set_field(MSDC0_GPIO_DRV, 0x7 << 9,
            msdc_cap->cmd_drv);
        sdr_set_field(MSDC0_GPIO_DRV, 0x7 << 6,
            msdc_cap->clk_drv);
        sdr_set_field(MSDC0_GPIO_DRV, 0x7 << 12,
            msdc_cap->dat_drv);
        break;
    case 1:
    default:
        break;
    }
}

void msdc_gpio_and_pad_init(struct mmc_host *host)
{
    /* set smt enable */
    //msdc_set_smt(host, 1);

    /* set pull enable */
    //msdc_pin_config(host, MSDC_PIN_PULL_UP);

    /* set gpio to msdc mode */
    msdc_set_pin_mode(host);

    /* set driving */
    msdc_set_driving_by_id(host->id, &msdc_cap[host->id]);

    /* set tdsel and rdsel */
    msdc_set_tdsel_wrap(host);
    msdc_set_rdsel_wrap(host);

    //msdc_dump_padctl_by_id(host->id);
}

#define TMO_IN_CLK_2POWER   20              /* 2^20=1048576 */

void msdc_set_timeout(struct mmc_host *host, u64 ns, u32 clks)
{
    u32 base = host->base;
    u32 timeout, clk_ns;
    u32 mode = 0;

    if (host->cur_bus_clk == 0) {
        timeout = 0;
    } else {
        clk_ns  = 1000000000UL / host->cur_bus_clk;
        timeout = (ns + clk_ns - 1) / clk_ns + clks;
        timeout = (timeout + (1 << TMO_IN_CLK_2POWER) - 1) >> TMO_IN_CLK_2POWER; /* in 1048576 sclk cycle unit */
        sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
        timeout = mode >= 2 ? timeout * 2 : timeout; //DDR mode will double the clk cycles for data timeout
        timeout = timeout > 1 ? timeout - 1 : 0;
        timeout = timeout > 255 ? 255 : timeout;
    }
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, timeout);

    //MSG(OPS, "[SD%d] Set read data timeout: %dus %dclks -> %d x 1048576 cycles, mode:%d, clk_freq=%dKHz\n",
    //    host->id, (u32)(ns/1000), clks, timeout + 1, mode, (host->cur_bus_clk / 1000));
}

#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

#define START_AT_RISING                 (0x0)
#define START_AT_FALLING                (0x1)
#define START_AT_RISING_AND_FALLING     (0x2)
#define START_AT_RISING_OR_FALLING      (0x3)

#define MSDC_SMPL_RISING        (0)
#define MSDC_SMPL_FALLING       (1)
#define MSDC_SMPL_SEPERATE      (2)

#define MSDC_CFG_CKMOD          (0x3  << 20)    /* W1C */
#define MSDC_CFG_CKMOD_BITS     (2)
#define MSDC_CFG_CKMOD_HS400        (0x1  << 22)    /* RW */
#define MSDC_CFG_CKDIV_BITS     (12)

void msdc_set_smpl(struct mmc_host *host, u8 HS400, u8 mode, u8 type)
{
    u32 base = host->base;
    //msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    if (type == TYPE_CMD_RESP_EDGE) {
        if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
        } else {
            //msdc_pr_err("[%s]: SD%d invalid resp latch parm: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
        }

    } else if (type == TYPE_WRITE_CRC_EDGE) {
        /* FIX ME, test if always using DS pin can work */
            sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin

        if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
            sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTSEDGE, mode);
        } else {
            //msdc_pr_err("[%s]: SD%d invalid wcrc latch parm: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
        }

    } else if (type == TYPE_READ_DATA_EDGE) {
        {
            //for the other mode, start bit is only output on rising edge. but DDR50 can try falling edge if error casued by pad delay
            //if (host->card && mmc_card_ddr(host->card)) {
            //    msdc_set_startbit(mode);
                //priv->start_bit = mode;
            //} else {
                msdc_set_startbit(START_AT_RISING);
                //priv->start_bit = START_AT_RISING;
            //}
        }

        if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
            sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL, mode);
        } else {
            //msdc_pr_err("[%s]: SD%d invalid read latch parm: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode);
        }
    }
}

void msdc_config_clock(struct mmc_host *host, int ddr, u32 hz, u32 hs_timing)
{
    //msdc_priv_t *priv = host->priv;
    u32 rsmpl = msdc_cap[host->id].cmd_edge;
    u32 base = host->base;
    u32 mode, hs400_div_dis = 0;
    u32 div;
    u32 sclk;
    u32 orig_clksrc = host->pll_mux_clk;

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

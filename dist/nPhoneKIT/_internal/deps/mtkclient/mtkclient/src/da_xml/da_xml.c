#include <stdint.h>
#include <stdbool.h>
#include "crypto/hmac-sha256.h"
#include "common/libc.h"
#define EXT_CSD_PART_CFG 179

#define RPMB_CUST_KEY "vutsrqponmlkjihgfedcba9876543210"

#define UFS_UPIU_RPMB_WLUN 0xC4
#define UFS_OP_SECURITY_PROTOCOL_IN 0xA2
#define UFS_OP_SECURITY_PROTOCOL_OUT 0xB5
#define SECURITY_PROTOCOL            0xEC

#define RPMB_PROGRAM_KEY       1       /* Program RPMB Authentication Key */
#define RPMB_GET_WRITE_COUNTER 2       /* Read RPMB write counter */
#define RPMB_WRITE_DATA        3       /* Write data to RPMB partition */
#define RPMB_READ_DATA         4       /* Read data from RPMB partition */
#define RPMB_RESULT_READ       5       /* Read result request */
#define RPMB_REQ               1       /* RPMB request mark */
#define RPMB_RESP              (1 << 1)/* RPMB response mark */
#define RPMB_AVALIABLE_SECTORS 8       /* 4K page size */

#define RPMB_TYPE_BEG          510  // FF
#define RPMB_RES_BEG           508  // FE
#define RPMB_BLKS_BEG          506  // FD
#define RPMB_ADDR_BEG          504  // FC
#define RPMB_WCOUNTER_BEG      500  // FA

#define RPMB_NONCE_BEG         484
#define RPMB_DATA_BEG          228
#define RPMB_MAC_BEG           196

#define UFS_UPIU_MAX_GENERAL_LUN    3

struct mmc_cid {
    unsigned int   manfid;
    char           prod_name[8];
    unsigned int   serial;
    unsigned short oemid;
    unsigned short year;
    unsigned char  hwrev;
    unsigned char  fwrev;
    unsigned char  month;
    unsigned char  cbx;                 /* device type: card(0) BGA(1) POP(2) */
};

struct mmc_csd {
    unsigned char  csd_struct;          /* csd structure version */
    unsigned char  mmca_vsn;
    unsigned short cmdclass;            /* card command classes */
    unsigned short tacc_clks;           /* data read access-time-1 in clks */
    unsigned int   tacc_ns;             /* data read access-time-2 */
    unsigned int   r2w_factor;          /* write speed factor */
    unsigned int   max_dtr;             /* max. data transfer rate */
    unsigned int   read_blkbits;        /* max. read data block length */
    unsigned int   write_blkbits;       /* max. write data block length */
    unsigned int   capacity;            /* card capacity */
    unsigned int   erase_sctsz;         /* erase sector size */
    unsigned int   write_prot_grpsz;
    unsigned int   read_partial:1,
                   read_misalign:1,
                   write_partial:1,
                   write_misalign:1,
                   write_prot_grp:1,
                   perm_wr_prot:1,
                   tmp_wr_prot:1,
                   erase_blk_en:1,
                   copy:1,
                   dsr:1;
};

struct mmc_raw_ext_csd {
    /* mode segment */
    unsigned char   rsv1[134];
    unsigned char   sec_bad_blk_mgmt;
    unsigned char   rsv2[1];
    unsigned char   enh_start_addr[4];
    unsigned char   enh_sz_mult[3];
    unsigned char   gp_sz_mult[12];
    unsigned char   part_set_cmpl;
    unsigned char   part_attr;
    unsigned char   max_enh_sz_mult[3];
    unsigned char   part_supp;
    unsigned char   rsv3[1];
    unsigned char   rst_n_func;
    unsigned char   rsv4[5];
    unsigned char   rpmb_sz_mult;
    unsigned char   fw_cfg;
    unsigned char   rsv5[1];
    unsigned char   user_wp;
    unsigned char   rsv6[1];
    unsigned char   boot_wp;
    unsigned char   rsv7[1];
    unsigned char   erase_grp_def;
    unsigned char   rsv8[1];
    unsigned char   boot_bus_width;
    unsigned char   boot_cfg_prot;
    unsigned char   part_cfg;
    unsigned char   rsv9[1];
    unsigned char   erase_mem_cont;
    unsigned char   rsv10[1];
    unsigned char   bus_width;
    unsigned char   rsv11[1];
    unsigned char   hs_timing;
    unsigned char   rsv12[1];
    unsigned char   pwr_cls;
    unsigned char   rsv13[1];
    unsigned char   cmd_set_rev;
    unsigned char   rsv14[1];
    unsigned char   cmd_set;

    /* propertities segment */
    unsigned char   ext_csd_rev;
    unsigned char   rsv15[1];
    unsigned char   csd_struct;
    unsigned char   rsv16[1];
    unsigned char   card_type;
    unsigned char   rsv17[1];
    unsigned char   pwr_cls_52_195;
    unsigned char   pwr_cls_26_195;
    unsigned char   pwr_cls_52_360;
    unsigned char   pwr_cls_26_360;
    unsigned char   rsv18[1];
    unsigned char   min_perf_r_4_26;
    unsigned char   min_perf_w_4_26;
    unsigned char   min_perf_r_8_26_4_52;
    unsigned char   min_perf_w_8_26_4_52;
    unsigned char   min_perf_r_8_52;
    unsigned char   min_perf_w_8_52;
    unsigned char   rsv19[1];
    unsigned char   sec_cnt[4];
    unsigned char   rsv20[1];
    unsigned char   slp_awake_tmo;
    unsigned char   rsv21[1];
    unsigned char   slp_curr_vccq;
    unsigned char   slp_curr_vcc;
    unsigned char   hc_wp_grp_sz;
    unsigned char   rel_wr_sec_cnt;
    unsigned char   erase_tmo_mult;
    unsigned char   hc_erase_grp_sz;
    unsigned char   acc_sz;
    unsigned char   boot_sz_mult;
    unsigned char   rsv22[1];
    unsigned char   boot_info;
    unsigned char   sec_trim_mult;
    unsigned char   sec_erase_mult;
    unsigned char   sec_supp;
    unsigned char   trim_mult;
    unsigned char   rsv23[1];
    unsigned char   min_perf_ddr_r_8_52;
    unsigned char   min_perf_ddr_w_8_52;
    unsigned char   rsv24[2];
    unsigned char   pwr_cls_ddr_52_195;
    unsigned char   pwr_cls_ddr_52_360;
    unsigned char   rsv25[1];
    unsigned char   ini_tmo_ap;
    unsigned char   rsv26[262];
    unsigned char   supp_cmd_set;
    unsigned char   rsv27[7];
};

struct mmc_ext_csd {
    unsigned int    trim_tmo_ms;
    unsigned int    hc_wp_grp_sz;
    unsigned int    hc_erase_grp_sz;
    unsigned int    sectors;
    unsigned int    hs_max_dtr;
    unsigned int    boot_part_sz;
    unsigned int    rpmb_sz;
    unsigned int    access_sz;
    unsigned int    enh_sz;
    unsigned int    enh_start_addr;
    unsigned char   rev;
    unsigned char   boot_info;
    unsigned char   part_en:1,
                    enh_attr_en:1,
                    ddr_support:1;
    unsigned char   erased_mem_cont;
    unsigned char   usr_wp;
    unsigned char   boot_wp;
};


struct sd_scr {
    unsigned char   scr_struct;
    unsigned char   sda_vsn;
    unsigned char   data_bit_after_erase;
    unsigned char   security;
    unsigned char   bus_widths;
    unsigned char   sda_vsn3;
    unsigned char   ex_security;
    unsigned char   cmd_support;
};

struct sd_switch_caps {
    unsigned int    hs_max_dtr;
    unsigned int    ddr;
    unsigned int    drv_strength;
    unsigned int    max_cur;
};

struct mmc_command {
    uint32_t opcode;
    uint32_t arg;
    uint32_t rsptyp;
    uint32_t resp[4];
    uint32_t timeout;
    uint32_t retries;    /* max number of retries */
    uint32_t error;      /* command error */
};

struct mmc_host
{
    struct mmc_card *card;
    uint64_t max_hw_segs;
    uint64_t max_phys_segs;
    uint64_t max_seg_size;
    uint32_t max_blk_size;
    uint32_t max_blk_count;
    uint32_t base;
    uint32_t caps;
    uint32_t f_min;
    uint32_t f_max;
    uint32_t clk;
    uint32_t sclk;
    uint32_t blklen;
    uint32_t blkbits;
    uint32_t ocr;
    uint32_t ocr_avail;
    uint32_t timeout_ns;
    uint32_t timeout_clks;
    uint8_t  clksrc;
    uint8_t  id;
    uint8_t  boot_type;
    uint8_t  app_cmd;
    uint32_t  app_cmd_arg;
    uint32_t  time_read;
    uint32_t  time_cmd;
    uint32_t cur_pwr;     /* current power voltage */
    struct mmc_command* cmd;
    void *priv;
    int (*blk_read)(struct mmc_host *host, uint8_t *dst, uint32_t src, uint32_t nblks);
    int (*blk_write)(struct mmc_host *host, uint32_t dst, uint8_t *src, uint32_t nblks);
};

/* MMC device */
struct mmc_card {
    struct mmc_host        *host;
    unsigned int            nblks;
    unsigned int            blklen;
    unsigned int            ocr;
    unsigned int            maxhz;
    unsigned int            uhs_mode;
    unsigned int            rca;
    unsigned int            type;
    #if defined(FEATURE_MMC_SDIO)
    unsigned int            sdio_funcs; /* number of SDIO functions */
    #endif
    unsigned short          state;
    unsigned short          ready;
    uint32_t                raw_cid[4];
    uint32_t                raw_csd[4];
    uint32_t                raw_scr[2];
    uint8_t                 raw_ext_csd[512];
    struct mmc_cid          cid;
    struct mmc_csd          csd;
    struct mmc_ext_csd      ext_csd;
    struct sd_scr           scr;
    struct sd_switch_caps   sw_caps;
    unsigned int            wp_size;    /* computed in ext_csd decode function, unit is block */
    #if defined(FEATURE_MMC_SDIO)
    struct sdio_cccr        cccr;       /* common card info */
    struct sdio_cis         cis;        /* common tuple info */
    struct sdio_func       *io_func[SDIO_MAX_FUNCS]; /* SDIO functions (devices) */
    struct sdio_func_tuple *tuples;     /* unknown common tuples */
    unsigned int            num_info;   /* number of info strings */
    unsigned int            speed_mode; /* recored which speed mode card can support */
    const char              **info;       /* info strings */
#endif
    uint8_t                 version;    /* the SD card version, 1.0, 2.0, or 3.0*/
};

#define MAX_CDB_SIZE                16

enum dma_data_direction {
    DMA_BIDIRECTIONAL = 0,
    DMA_TO_DEVICE = 1,
    DMA_FROM_DEVICE = 2,
    DMA_NONE = 3,
};

struct ufs_aio_scsi_cmd {
    uint32_t lun;
    uint32_t tag;
    uint8_t dir; //dma_data_direction
    uint8_t attr;
    uint8_t cmd_data[MAX_CDB_SIZE];
    uint8_t unknown;
    uint16_t cmd_len;
    uint32_t exp_len;
    void * data_buf;
};

/* UTP QUERY Transaction Specific Fields OpCode */
enum query_opcode {
	UPIU_QUERY_OPCODE_NOP        = 0x0,
	UPIU_QUERY_OPCODE_READ_DESC    = 0x1,
	UPIU_QUERY_OPCODE_WRITE_DESC    = 0x2,
	UPIU_QUERY_OPCODE_READ_ATTR    = 0x3,
	UPIU_QUERY_OPCODE_WRITE_ATTR    = 0x4,
	UPIU_QUERY_OPCODE_READ_FLAG    = 0x5,
	UPIU_QUERY_OPCODE_SET_FLAG    = 0x6,
	UPIU_QUERY_OPCODE_CLEAR_FLAG    = 0x7,
	UPIU_QUERY_OPCODE_TOGGLE_FLAG    = 0x8,
};

/* Flag idn for Query Requests*/
enum flag_idn {
	QUERY_FLAG_IDN_FDEVICEINIT      = 0x01,
	QUERY_FLAG_IDN_PWR_ON_WPE    = 0x03,
	QUERY_FLAG_IDN_BKOPS_EN         = 0x04,
};

/* Attribute idn for Query requests */
enum attr_idn {
	QUERY_ATTR_IDN_ACTIVE_ICC_LVL    = 0x03,
	QUERY_ATTR_IDN_BKOPS_STATUS    = 0x05,
	QUERY_ATTR_IDN_REF_CLK_FREQ    = 0x0A,
	QUERY_ATTR_IDN_EE_CONTROL    = 0x0D,
	QUERY_ATTR_IDN_EE_STATUS    = 0x0E,
};

/* Descriptor idn for Query requests */
enum desc_idn {
	QUERY_DESC_IDN_DEVICE           = 0x0,
	QUERY_DESC_IDN_CONFIGURATION    = 0x1,
	QUERY_DESC_IDN_UNIT             = 0x2,
	QUERY_DESC_IDN_RFU_0            = 0x3,
	QUERY_DESC_IDN_INTERCONNECT     = 0x4,
	QUERY_DESC_IDN_STRING           = 0x5,
	QUERY_DESC_IDN_RFU_1            = 0x6,
	QUERY_DESC_IDN_GEOMETRY         = 0x7,
	QUERY_DESC_IDN_POWER            = 0x8,
	QUERY_DESC_IDN_HEALTH		= 0x9,
	QUERY_DESC_IDN_MAX,
};

#define ufs_paddr_t uint32_t

struct ufs_pa_layer_attr {
	uint32_t gear_rx;
	uint32_t gear_tx;
	uint32_t lane_rx;
	uint32_t lane_tx;
	uint32_t pwr_rx;
	uint32_t pwr_tx;
	uint32_t hs_rate;
};

#define UFS_MAX_CMD_DATA_SIZE   (64)

enum dev_cmd_type {
	DEV_CMD_TYPE_NOP        = 0x0,
	DEV_CMD_TYPE_QUERY        = 0x1,
};

/**
 * struct utp_upiu_query - upiu request buffer structure for
 * query request.
 * @opcode: command to perform B-0
 * @idn: a value that indicates the particular type of data B-1
 * @index: Index to further identify data B-2
 * @selector: Index to further identify data B-3
 * @reserved_osf: spec reserved field B-4,5
 * @length: number of descriptor bytes to read/write B-6,7
 * @value: Attribute value to be written DW-5
 * @reserved: spec reserved DW-6,7
 */
struct utp_upiu_query {
	uint8_t opcode;
	uint8_t idn;
	uint8_t index;
	uint8_t selector;
	uint16_t reserved_osf;
	uint16_t length;
	uint32_t value;
	uint32_t reserved[2];
};

/**
 * struct ufs_query_req - parameters for building a query request
 * @query_func: UPIU header query function
 * @upiu_req: the query request data
 */
struct ufs_query_req {
	uint8_t query_func;
	struct utp_upiu_query upiu_req;
};

/**
 * struct ufs_query_resp - UPIU QUERY
 * @response: device response code
 * @upiu_res: query response data
 */
struct ufs_query_res {
	uint8_t response;
	struct utp_upiu_query upiu_res;
};

/**
 * struct ufs_query - holds relevent data structures for query request
 * @request: request upiu and function
 * @descriptor: buffer for sending/receiving descriptor
 * @response: response upiu and response
 */
struct ufs_query {
	struct ufs_query_req request;
	uint8_t *descriptor;
	struct ufs_query_res response;
};

/**
 * struct ufs_dev_cmd - all assosiated fields with device management commands
 * @type: device management command type - Query, NOP OUT
 * @lock: lock to allow one command at a time
 * @complete: internal commands completion
 * @tag_wq: wait queue until free command slot is available
 */
struct ufs_dev_cmd {
	enum dev_cmd_type type;
	struct ufs_query query;
};

#define MAX_PRODUCT_ID_LEN              (16)
#define MAX_PRODUCT_REVISION_LEVEL_LEN  (4)
#define MAX_SERAL_NUMBER_LEN            (64) /* spec (126*2), 64 because code size */

struct ufs_device_info {
	uint16_t wmanufacturerid;                     // from Device Descriptor
	uint8_t  num_active_lu;                       // from Device Descriptor
	uint16_t ufs_ver;                             // from Device Descriptor
	uint8_t  bootable;
	char product_id[MAX_PRODUCT_ID_LEN + 1];
	char product_revision_level[MAX_PRODUCT_REVISION_LEVEL_LEN + 1];
	char serial_number[MAX_SERAL_NUMBER_LEN * 2 + 1]; /* 1 byte need 2 char(ex.FF) + 1 end */
	uint8_t  serial_number_len;
	uint8_t  ud0_base_offset;
	uint8_t  ud_config_len;
	uint8_t  hpb_support;
	uint16_t hpb_ver;
	uint8_t  tw_support;
	uint8_t  tw_red;
	uint8_t  tw_type;
	uint16_t tw_ver;
	uint32_t wb_buf_au;
	uint8_t pre_eol_info;
	uint8_t life_time_est_a;
	uint8_t life_time_est_b;
};

struct ufs_custom_info {
	uint32_t  custom_flag;
	uint32_t  force_provision; /* default:0, force:1, skip:2 */
	uint32_t  tw_size_gb;
	uint32_t  tw_no_red;
	uint32_t  hpb_size_gb;
	uint64_t  lu3_size_mb;
	uint32_t  lu3_type;
	uint32_t  hpb_ctrl_mode; /* host control:0, device control:1 */
	uint16_t  hpb_pinned_start_idx;
	uint16_t  hpb_pinned_regions;
};

struct ufs_pwr_mode_info {
	bool is_valid;
	struct ufs_pa_layer_attr info;
};

struct ufs_unit_desc_cfg_param {
	uint8_t b_lu_enable;
	uint8_t b_boot_lun_id;
	uint8_t b_lu_write_protect;
	uint8_t b_memory_type;
	uint8_t d_num_alloc_units[4];
	uint8_t b_data_reliability;
	uint8_t b_logical_block_size;
	uint8_t b_provisioning_type;
	uint8_t w_context_capabilities[2];
	uint8_t reserved[3];
};

typedef enum {
	UFS_LU_0 = 0
	,UFS_LU_1 = 1
	,UFS_LU_2 = 2
	,UFS_LU_3 = 3
	,UFS_LU_INTERNAL_CNT = 3
} ufs_logical_unit_internal;

struct ufs_hba {
	void    *hci_base;
	void    *pericfg_base;
	void    *mphy_base;
	int     nutrs;
	//int     nutmrs;

	/* Virtual memory reference */
	struct utp_transfer_cmd_desc *ucdl_base_addr;
	struct utp_transfer_req_desc *utrdl_base_addr;
	//struct utp_task_req_desc *utmrdl_base_addr;
	//void * sense_buf_base_addr[UFS_AIO_MAX_NUTRS];

	/* DMA memory reference */
	ufs_paddr_t ucdl_dma_addr;
	ufs_paddr_t utrdl_dma_addr;
	//ufs_paddr_t utmrdl_dma_addr;
	//ufs_paddr_t sense_buf_dma_addr[UFS_AIO_MAX_NUTRS];

	unsigned int hci_quirks;
	unsigned int dev_quirks;

	struct uic_command *active_uic_cmd;
	struct ufs_pa_layer_attr pwr_info;

	struct ufshcd_lrb *lrb;
	unsigned long lrb_in_use;

	struct ufs_device_info dev_info;
	struct ufs_custom_info custom_info;

	uint8_t  active_tr_tag;
	uint8_t  mode;
	uint8_t  unit_desc_cfg_param_valid;
	//uint8_t  active_tm_tag;
	int  active_lun;

	unsigned long outstanding_reqs;

	struct ufs_pwr_mode_info max_pwr_info;

    uint8_t  quirk_sel;  /* selector. legacy Samsung and Micron dev takes selector 0x01 */

	/* Device management request data */
	struct ufs_dev_cmd dev_cmd;

	int (* blk_read)(struct ufs_hba * hba, uint32_t lun, uint32_t blk_start, uint32_t blk_cnt, unsigned long * buf);
	int (* blk_write)(struct ufs_hba * hba, uint32_t lun, uint32_t blk_start, uint32_t blk_cnt, unsigned long * buf);
	int (* blk_erase)(struct ufs_hba * hba, uint32_t lun, uint32_t blk_start, uint32_t blk_cnt);
	int (* nopin_nopout)(struct ufs_hba * hba);
	int (* query_flag)(struct ufs_hba *hba, enum query_opcode opcode, enum flag_idn idn, bool *flag_res);
	int (* query_attr)(struct ufs_hba *hba, enum query_opcode opcode, enum attr_idn idn, uint8_t index, uint8_t selector, uint32_t *attr_val);
	int (* read_descriptor)(struct ufs_hba * hba, enum desc_idn desc_id, int desc_index, uint8_t selector, uint8_t *buf, uint32_t size);
	int (* write_descriptor)(struct ufs_hba * hba, enum desc_idn desc_id, int desc_index, uint8_t selector, uint8_t *buf, uint32_t size);
	int (* dme_get)(struct ufs_hba *hba, uint32_t attr_sel, uint32_t *mib_val);
	int (* dme_peer_get)(struct ufs_hba *hba, uint32_t attr_sel, uint32_t *mib_val);
	int (* dme_set)(struct ufs_hba *hba, uint32_t attr_sel, uint32_t mib_val);
	int (* dme_peer_set)(struct ufs_hba *hba, uint32_t attr_sel, uint32_t mib_val);
	int (* ffu_write)(struct ufs_hba * hba, unsigned long * buf, uint32_t buf_size);

	// unit descriptor configurable parameters (in Configuration Descriptor)
	struct ufs_unit_desc_cfg_param unit_desc_cfg_param[UFS_UPIU_MAX_GENERAL_LUN];
    uint32_t blk_cnt[UFS_LU_INTERNAL_CNT];
	uint32_t drv_status;
	uint32_t irq;
};

typedef struct
{
   int (*read_packet_with_profile)(uint8_t* buffer, uint32_t* length);
   int (*write_packet_with_profile)(uint8_t* buffer, uint32_t length);
   int (*log_packet_to_pc)(const uint8_t* buffer, uint32_t length);
   int (*log_to_uart)(const uint8_t* buffer, uint32_t length);
} com_channel_struct;

typedef int (*HHANDLE)(com_channel_struct*);
extern void apmcu_dcache_clean_invalidate();
extern void apmcu_dcache_invalidate();
extern int cache_init(int param);
extern int cache_close(int param);
int (*run)();

int (*register_xml_cmd)(char* /*cmd*/, char* /*version*/, HHANDLE /*handle*/)=(const void*)0x11111111;
int (*mmc_get_card)(int /*id*/)=(const void*)0x22222222;
int (*mmc_set_part_config)(struct mmc_card */*card*/, uint8_t /*cfg*/)=(const void*)0x33333333;
int (*mmc_rpmb_send_command)(struct mmc_card */*card*/, uint8_t */*data_frame*/, uint32_t /*blks*/, int /*type*/, uint8_t /*req_type*/)=(const void*)0x44444444;
int (*ufshcd_queuecommand)(struct ufs_hba */*hba*/, struct ufs_aio_scsi_cmd */*cmd*/)=(const void*)0x55555555;
int (*ufshcd_get_free_tag)(struct ufs_hba */*hba*/, uint32_t */*tag_out*/)=(const void*)0x66666666;
uint32_t g_ufs_hba=0x77777777;

void ufshcd_put_tag(struct ufs_hba *hba, int tag)
{
	/* clear_bit(tag, &hba->lrb_in_use); */
	hba->lrb_in_use &= ~(1 << tag);
}

uint8_t rpmb_key[32]={0x64, 0x76, 0xEE, 0xF0, 0xF1, 0x6B, 0x30, 0x47, 0xE9, 0x79, 0x31, 0x58, 0xF6, 0x42, 0xDA, 0x46, 0xF7, 0x3B, 0x53, 0xFD, 0xC5, 0xF8, 0x84, 0xCE, 0x03, 0x73, 0x15, 0xBC, 0x54, 0x47, 0xD4, 0x6A};

void mcpy(uint8_t* src, uint8_t* dst, int len)
{
    uint8_t* i;
    uint8_t* m;
    uint8_t* t;

    for (i=src;len--;i++)
    {
        m=dst++;
        t=i;
        *t=*m;
    }
}

#define HW_DESC_SIZE_WORDS 6
typedef struct HwDesc {
	uint32_t word[HW_DESC_SIZE_WORDS];
} HwDesc_s;

#define DX_DSCRPTR_QUEUE0_WORD0_REG_OFFSET 	0xE80
#define DX_DSCRPTR_QUEUE0_WORD1_REG_OFFSET 	0xE84
#define DX_DSCRPTR_QUEUE0_WORD2_REG_OFFSET 	0xE88
#define DX_DSCRPTR_QUEUE0_WORD3_REG_OFFSET 	0xE8C
#define DX_DSCRPTR_QUEUE0_WORD4_REG_OFFSET 	0xE90
#define DX_DSCRPTR_QUEUE0_WORD5_REG_OFFSET 	0xE94
#define DX_DSCRPTR_QUEUE0_CONTENT_REG_OFFSET 	0xE9C
#define DX_HOST_IRR_REG_OFFSET 	0xA00
#define DX_HOST_ICR_REG_OFFSET 	0xA08
#define DX_HOST_SEP_HOST_GPR4_REG_OFFSET 	0xAA0

void SaSi_SB_AddDescSequence(volatile uint32_t *result, HwDesc_s *desc)
{
  while ( (result[DX_DSCRPTR_QUEUE0_CONTENT_REG_OFFSET/4] & 0x3FF) == 0 )
    ;
  result[DX_DSCRPTR_QUEUE0_WORD0_REG_OFFSET/4] = (volatile uint32_t)desc->word[0];
  result[DX_DSCRPTR_QUEUE0_WORD1_REG_OFFSET/4] = (volatile uint32_t)desc->word[1];
  result[DX_DSCRPTR_QUEUE0_WORD2_REG_OFFSET/4] = (volatile uint32_t)desc->word[2];
  result[DX_DSCRPTR_QUEUE0_WORD3_REG_OFFSET/4] = (volatile uint32_t)desc->word[3];
  result[DX_DSCRPTR_QUEUE0_WORD4_REG_OFFSET/4] = (volatile uint32_t)desc->word[4];
  result[DX_DSCRPTR_QUEUE0_WORD5_REG_OFFSET/4] = (volatile uint32_t)desc->word[5];
}

void SaSi_PalDmaMap(uint32_t offset, volatile uint64_t* dst)
{
    *dst=offset;
}

int SB_ReadFlag(volatile uint32_t* base)
{
    volatile uint32_t res=0;
    do {
        res=base[0xBA0/4];
    }
    while (!res);
    return res;
}

void SaSi_PalDmaUnMap(volatile uint64_t* dst)
{
    (void)dst;
}

void SB_HalInit(volatile uint32_t* base)
{
    base[DX_HOST_ICR_REG_OFFSET/4]=(volatile uint32_t)4;
}

int SB_CryptoWait(volatile uint32_t* base)
{
    volatile uint32_t val;
    do {
        val=(volatile uint32_t)base[DX_HOST_IRR_REG_OFFSET/4];
    } while (!val);
    return val;
}

int SB_HalWaitDescCompletion(volatile uint32_t* base)
{
  struct HwDesc desc;
  SB_HalInit(base);
  uint32_t outv = 0;
  volatile uint64_t out = 0;
  SaSi_PalDmaMap((uint32_t)&outv, &out);
  desc.word[0] = 0;
  desc.word[1] = 0x8000011;
  desc.word[2] = out;
  desc.word[5] = ((out>>32)&0xFFFFFFFF) << 16;
  desc.word[3] = 0x8000012;
  desc.word[4] = 0x100;
  SaSi_SB_AddDescSequence(base, &desc);
  while ( (SB_CryptoWait(base) & 4) == 0 );
  volatile uint32_t val=0;
  do
  {
    val = SB_ReadFlag(base);
  } while (!val);
  if ( val == 1 )
  {
    SB_HalInit(base);
    SaSi_PalDmaUnMap(&out);
    return 0;
  }
  else
  {
    SaSi_PalDmaUnMap(&out);
    return 0xF6000001;
    }
}

void write_reg(volatile uint32_t addr, volatile uint32_t value)
{
    *(volatile uint32_t*)addr=value;
}

int SBROM_AesCmacDriver(volatile uint32_t *base,
        uint32_t hwmode,
        uint64_t key,
        uint64_t buf,
        int bufferlen,
        uint64_t out)
{
  struct HwDesc desc;
  int keylen = 0;
  if ( hwmode == 1 )
  {
    if (((base[DX_HOST_SEP_HOST_GPR4_REG_OFFSET/4])&2)!=0)
    {
        keylen = 32;
    }
    else
    {
        keylen = 16;
    }
  }
  else
  {
    keylen = 16;
  }
  SB_HalInit(base);
  volatile uint32_t kval = (keylen << 19) - 0x800000;
  desc.word[1] = 0x8000041;
  desc.word[4] = kval | 0x1001C20;
  desc.word[0] = 0;
  desc.word[2] = 0;
  desc.word[3] = 0;
  desc.word[5] = 0;
  SaSi_SB_AddDescSequence(base, &desc);

desc.word[0] = 0;
desc.word[1] = 0;
desc.word[2] = 0;
desc.word[3] = 0;
desc.word[4] = 0;

if ( !hwmode )
{
desc.word[0] = (uint32_t)key;
desc.word[5] = (uint16_t)(key>>32);
desc.word[1] = 0x42;
}
desc.word[4] = kval | ((hwmode&3)<<15) | (((hwmode>>2)&3)<<20) | 0x4001C20;
SaSi_SB_AddDescSequence(base, &desc);

desc.word[2] = 0;
desc.word[0] = (uint32_t)buf;
desc.word[5] = (uint16_t)(buf>>32);
desc.word[3] = 0;
desc.word[1] = (4 * (bufferlen & 0xFFFFFF)) | 2;
desc.word[4] = 1;
SaSi_SB_AddDescSequence(base, &desc);

  if (hwmode!=2)
  {
      desc.word[5] = ((uint16_t)(out>>32)) << 16;
      desc.word[0] = 0;
      desc.word[4] = 0x8001C26;
      desc.word[1] = 0;
      desc.word[2] = (uint32_t)out;
      desc.word[3] = 0x42;
      SaSi_SB_AddDescSequence(base, &desc);
  }

return SB_HalWaitDescCompletion(base);
}

int SBROM_AesCMac(volatile uint32_t* base, int hwmode, uint8_t* buffer, int bufferlen, uint8_t* outbuf)
{
    int ret;
    uint64_t iv=0;
    /*if (val0)
        SaSi_PalDmaMap((uint32_t)&val0, &iv);
    if (buffer)
        SaSi_PalDmaMap((uint32_t)&buffer, &buf);
    if (outbuf)
        SaSi_PalDmaMap((uint32_t)&outbuf, &out);
    */

    ret = SBROM_AesCmacDriver(base, hwmode, iv, (uint32_t)buffer, bufferlen, (uint32_t)outbuf);
    /*if (val0)
        SaSi_PalDmaUnMap(&iv);
    if (buffer)
        SaSi_PalDmaUnMap(&buf);
    if (outbuf)
        SaSi_PalDmaUnMap(&out);
    */
    return ret;
}
int dxcc(volatile uint32_t* base, int hwmode, uint8_t* key, int keylen, uint8_t* seed, int seedlen, uint8_t* outbuf, int derivelen)
{
    uint8_t* buffer=(uint8_t*)0x200d10;
    uint8_t* tmp=(uint8_t*)0x200d00;
    uint32_t i;
    uint32_t pos=0;
    int ret;
    if (!hwmode)
        return 0xF2000002;
    if (!key && (keylen || keylen > 0x20))
        return 0xF2000003;
    if (!seed && (seedlen || seedlen > 0x20))
        return 0xF2000003;
    memset(buffer,0,0x43);
    buffer[pos++]=1;
    if (key)
    {
        mcpy(&buffer[pos++], key, keylen);
        pos += keylen;
    }
    buffer[pos]=0;
    if (seed)
    {
        mcpy(&buffer[pos], seed, seedlen);
        pos += seedlen;
    }
    buffer[pos]=(8*derivelen)&0xFF;
    for (i=0;i<((uint32_t)derivelen+15)>>4;i++)
    {
        buffer[0] = i+1;
        ret = SBROM_AesCMac(base,hwmode,buffer,keylen+seedlen+3,tmp);
        mcpy(outbuf+(16*i),tmp,0x10);
        if (ret)
            return ret;
    }
    return 0;
}

int cmd_readmem(com_channel_struct *channel, const char* /*xml*/){
    volatile uint64_t addr=0;
    uint8_t buffer2[0x20000]={0};
    uint32_t length=0;
    uint32_t cmdlen=8;
    channel->read_packet_with_profile((uint8_t*)&addr,&cmdlen);
    cmdlen=4;
    channel->read_packet_with_profile((uint8_t*)&length,&cmdlen);
    if (length>0x20000) length=0x20000;
    memcpy(buffer2,(volatile uint64_t*)addr,length);
    return channel->write_packet_with_profile((uint8_t *)buffer2,length);
}

int cmd_readregister(com_channel_struct *channel, const char* /*xml*/){
    volatile uint32_t addr=0;
    volatile uint32_t dword=0;
    uint32_t cmdlen=4;
    channel->read_packet_with_profile((uint8_t*)&addr,&cmdlen);
    cmdlen=4;
    dword=*(volatile uint32_t*)addr;
    return channel->write_packet_with_profile((uint8_t *)&dword,cmdlen);
}

int cmd_writemem(com_channel_struct *channel, const char* /*xml*/){
    volatile uint64_t addr=0;
    uint32_t length=0;
    uint32_t cmdlen=8;
    channel->read_packet_with_profile((uint8_t*)&addr,&cmdlen);
    cmdlen=4;
    channel->read_packet_with_profile((uint8_t*)&length,&cmdlen);
    return channel->read_packet_with_profile((uint8_t *)addr,&length);
}

int cmd_writeregister(com_channel_struct *channel, const char* /*xml*/){
    volatile uint32_t addr=0;
    volatile uint32_t dword=0;
    uint32_t cmdlen=4;
    channel->read_packet_with_profile((uint8_t*)&addr,&cmdlen);
    cmdlen=4;
    channel->read_packet_with_profile((uint8_t*)&dword,&cmdlen);
    *(volatile uint32_t*)addr=dword;
    return 0;
}

int cmd_ack(com_channel_struct *channel, const char* /*xml*/){
    uint32_t ack=0xA1A2A3A4;
    return channel->write_packet_with_profile((uint8_t *)&ack,4);
}

/*int register_rw(com_channel_struct* channel, const char* xml){
    volatile uint32_t addr=0;
    volatile uint32_t dword=0;
    uint32_t mode=0;
    uint32_t cmdlen=4;
    channel->read_packet_with_profile((uint8_t*)&mode,&cmdlen);
    cmdlen=4;
    if (mode==0)
    {
        channel->read_packet_with_profile((uint8_t*)&addr,&cmdlen);
        cmdlen=4;
        dword=*(volatile uint32_t*)addr;
        return channel->write_packet_with_profile((uint8_t *)&dword,cmdlen);
    }
    else if (mode==1)
    {
        channel->read_packet_with_profile((uint8_t*)&addr,&cmdlen);
        cmdlen=4;
        channel->read_packet_with_profile((uint8_t*)&dword,&cmdlen);
        *(volatile uint32_t*)addr=dword;
    }
    else if (mode==2)
    {
        channel->read_packet_with_profile((uint8_t*)&addr,&cmdlen);
        run=(void*)*(volatile uint32_t*)addr;
        run();
    }
    return channel->write_packet_with_profile("OK",3);
}*/

void send_to_device(struct ufs_aio_scsi_cmd *cmd, uint32_t lba, int tag)
{
  memset(cmd->cmd_data, 0, MAX_CDB_SIZE);
  cmd->lun = UFS_UPIU_RPMB_WLUN;
  cmd->tag = tag;
  cmd->cmd_len = 0xC;
  cmd->dir = DMA_TO_DEVICE;
  cmd->exp_len = 0x200;
  cmd->attr = 0;
  cmd->cmd_data[0] = (uint8_t)UFS_OP_SECURITY_PROTOCOL_OUT;
  cmd->cmd_data[1] = (uint8_t)SECURITY_PROTOCOL;
  cmd->cmd_data[2] = (uint8_t)0;    // specific
  cmd->cmd_data[3] = (uint8_t)1;    // specific
  cmd->cmd_data[4] = 0;             // reserved
  cmd->cmd_data[5] = 0;             // reserved
  cmd->cmd_data[6] = (uint8_t)((lba >> 24) & 0xff);
  cmd->cmd_data[7] = (uint8_t)((lba >> 16) & 0xff);
  cmd->cmd_data[8] = (uint8_t)((lba >> 8) & 0xff);
  cmd->cmd_data[9] = (uint8_t)(lba & 0xff);
  cmd->cmd_data[0xA] = 0;           // reserved
  cmd->cmd_data[0xB] = (uint8_t)0;  // control
}
void read_from_device(struct ufs_aio_scsi_cmd *cmd, uint32_t lba, uint32_t tag)
{
  memset(cmd->cmd_data, 0, MAX_CDB_SIZE);
  cmd->cmd_len = 0xC;
  cmd->lun = UFS_UPIU_RPMB_WLUN;
  cmd->tag = tag;
  cmd->exp_len = 0x200;
  cmd->dir = DMA_FROM_DEVICE;
  cmd->attr = 0;
  cmd->cmd_data[0] = (uint8_t)UFS_OP_SECURITY_PROTOCOL_IN;
  cmd->cmd_data[1] = (uint8_t)SECURITY_PROTOCOL;
  cmd->cmd_data[2] = (uint8_t)0;    // specific
  cmd->cmd_data[3] = (uint8_t)1;    // specific
  cmd->cmd_data[4] = 0;             // reserved
  cmd->cmd_data[5] = 0;             // reserved
  cmd->cmd_data[6] = (uint8_t)((lba >> 24) & 0xff);
  cmd->cmd_data[7] = (uint8_t)((lba >> 16) & 0xff);
  cmd->cmd_data[8] = (uint8_t)((lba >> 8) & 0xff);
  cmd->cmd_data[9] = (uint8_t)(lba & 0xff);
  cmd->cmd_data[0xA] = 0;           // reserved
  cmd->cmd_data[0xB] = (uint8_t)0;  // control
}

int rpmb_key_generate(com_channel_struct *channel, uint8_t* rpmb_frame)
{
    uint8_t hash[34]={0};
    uint32_t rpmbiv[3];
    uint16_t res=__builtin_bswap16(*(uint16_t *)(rpmb_frame + RPMB_RES_BEG));
    if (res)
    {
        channel->write_packet_with_profile((uint8_t*)&res,2);
        return -1;
    } else
    {
        hmac_sha256(hash, rpmb_frame + RPMB_DATA_BEG, 512 - RPMB_DATA_BEG, (const uint8_t*)rpmb_key, 32);
        if (!memcmp(rpmb_frame + RPMB_MAC_BEG, hash, 32))
        {
            res=0;
        }
        else
        {
              //channel->write_packet_with_profile(rpmb_frame,RPMB_DATA_BEG);
              //channel->write_packet_with_profile(hash,0x20);
              //channel->write_packet_with_profile(rpmb_key,0x20);
              memcpy((void*)rpmbiv, "RPMB KEYSASI", sizeof(rpmbiv));
              *(volatile uint32_t*)0x1000108C = 0x18000000;
              res=dxcc((volatile uint32_t*)0x10210000, 1, (uint8_t*)rpmbiv, 8, (uint8_t*)&rpmbiv[2], 4, rpmb_key, 32);
              if (res!=0) return res;
              *(volatile uint32_t*)0x10001088 = 0x18000000;
              hmac_sha256(hash, rpmb_frame + RPMB_DATA_BEG, 512 - RPMB_DATA_BEG, (const uint8_t*)rpmb_key, sizeof(rpmb_key));
              if ( !memcmp(rpmb_frame + RPMB_MAC_BEG, hash, 32) )
              {
                res=0;
              }
              else {
                    memcpy(rpmb_key, RPMB_CUST_KEY, sizeof(rpmb_key));
                    hmac_sha256(hash, rpmb_frame + RPMB_DATA_BEG, 512 - RPMB_DATA_BEG, (const uint8_t*)rpmb_key, sizeof(rpmb_key));
                    if ( !memcmp(rpmb_frame + RPMB_MAC_BEG, hash, 32) )
                    {
                      res=0;
                    }
                    else
                    {
                      res = -2;
                    }
                  }
        }
    }
    channel->write_packet_with_profile(rpmb_key,0x20);
    channel->write_packet_with_profile(rpmb_frame,0x200);
    channel->write_packet_with_profile(hash,32);
    return res;
}

int cmd_ufs_read_rpmb(com_channel_struct *channel, const char* /*xml*/)
{
  uint16_t buffer[0x100];
  struct ufs_aio_scsi_cmd cmd;
  uint32_t tag;
  uint32_t res;
  uint32_t size = 4;
  uint32_t address = 0;
  channel->read_packet_with_profile((uint8_t*)&address, &size);
  if (!ufshcd_get_free_tag((struct ufs_hba *)g_ufs_hba, &tag))
  {
    res=-1;
    channel->write_packet_with_profile((uint8_t*)&res, 2);
    return 0;
  }
  cmd.data_buf = buffer;
  memset(buffer, 0, sizeof(buffer));
  buffer[RPMB_ADDR_BEG/2] = __builtin_bswap16(address);
  buffer[RPMB_BLKS_BEG/2] = 0x100;
  buffer[RPMB_TYPE_BEG/2] = __builtin_bswap16(RPMB_READ_DATA);
  send_to_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  memset(buffer, 0, sizeof(buffer));
  read_from_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  res = __builtin_bswap16(buffer[RPMB_RES_BEG/2]);
  channel->write_packet_with_profile((uint8_t*)&res, 2);
  if ( !buffer[RPMB_RES_BEG/2] )
    channel->write_packet_with_profile((uint8_t*)&buffer[RPMB_DATA_BEG/2], 0x100);
  ufshcd_put_tag((struct ufs_hba *)g_ufs_hba, tag);
  return 0;
}

int cmd_ufs_write_rpmb(com_channel_struct *channel, const char* /*xml*/)
{
  uint16_t status;
  uint16_t buffer[256];
  struct ufs_aio_scsi_cmd cmd;
  uint32_t tag;

  uint32_t size = 4;
  uint32_t address = 0;
  channel->read_packet_with_profile((uint8_t*)&address, &size);
  ufshcd_get_free_tag((struct ufs_hba *)g_ufs_hba, &tag);
  cmd.data_buf = buffer;
  memset(buffer, 0, sizeof(buffer));
  buffer[RPMB_TYPE_BEG/2] = 0x200;
  send_to_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  memset(buffer, 0, sizeof(buffer));
  read_from_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  size = 0x100;
  channel->read_packet_with_profile((uint8_t*)&buffer[RPMB_DATA_BEG/2], &size);
  buffer[RPMB_ADDR_BEG/2] = __builtin_bswap16(address);
  buffer[RPMB_BLKS_BEG/2] = 0x100;
  buffer[RPMB_RES_BEG/2] = 0;
  buffer[RPMB_TYPE_BEG/2] = __builtin_bswap16(RPMB_WRITE_DATA);
  hmac_sha256((uint8_t*)&buffer[RPMB_MAC_BEG/2], (uint8_t*)&buffer[RPMB_DATA_BEG/2], 0x200 - RPMB_DATA_BEG, (const uint8_t*)rpmb_key, sizeof(rpmb_key));
  send_to_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  memset(buffer, 0, sizeof(buffer));
  buffer[RPMB_TYPE_BEG/2] = __builtin_bswap16(RPMB_RESULT_READ);
  send_to_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  memset(buffer, 0, sizeof(buffer));
  read_from_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  status = __builtin_bswap16(buffer[RPMB_RES_BEG/2]);
  channel->write_packet_with_profile((uint8_t*)&status, 2);
  ufshcd_put_tag((struct ufs_hba *)g_ufs_hba, tag);
  return 0;
}

int cmd_ufs_init(com_channel_struct *channel, const char* /*xml*/)
{
  uint16_t buffer[0x200]={0};
  struct ufs_aio_scsi_cmd cmd;
  uint32_t tag;
  ufshcd_get_free_tag((struct ufs_hba *)g_ufs_hba, &tag);
  cmd.data_buf = buffer;
  memset(buffer, 0, sizeof(buffer));
  buffer[RPMB_TYPE_BEG/2] = __builtin_bswap16(RPMB_GET_WRITE_COUNTER);
  send_to_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  memset(buffer, 0, sizeof(buffer));
  read_from_device(&cmd, 0x200, tag);
  ufshcd_queuecommand((struct ufs_hba *)g_ufs_hba, &cmd);
  ufshcd_put_tag((struct ufs_hba *)g_ufs_hba, tag);
  return rpmb_key_generate(channel, (uint8_t*)buffer);
}

int cmd_set_rpmbkey(com_channel_struct *channel, const char* /*xml*/)
{
  uint32_t size = 0x20;
  channel->read_packet_with_profile((uint8_t*)&rpmb_key[0], &size);
  channel->write_packet_with_profile(rpmb_key,0x20);
  return 0;
}

int cmd_mmc_read_rpmb(com_channel_struct *channel, const char* /*xml*/)
{
  uint8_t rpmb_frame[0x200]={0};
  struct mmc_card* card = (struct mmc_card*)mmc_get_card(0);
  mmc_set_part_config(card, (card->raw_ext_csd[EXT_CSD_PART_CFG] & 0xF8) | 3);
  uint32_t size = 4;
  uint32_t address = 0;
  channel->read_packet_with_profile((uint8_t*)&address, &size);
  memset(rpmb_frame, 0, sizeof(rpmb_frame));
  *(uint16_t*)(rpmb_frame + RPMB_ADDR_BEG) = __builtin_bswap16((uint16_t)address&0xFFFF);
  *(uint16_t*)(rpmb_frame + RPMB_TYPE_BEG) = __builtin_bswap16(RPMB_READ_DATA);

  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_READ_DATA, RPMB_REQ);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_READ_DATA, RPMB_RESP);
  uint16_t res = __builtin_bswap16(*(uint16_t*)(rpmb_frame + RPMB_RES_BEG));
  channel->write_packet_with_profile((uint8_t *)&res, 2);
  if (!*(uint16_t*)(rpmb_frame + RPMB_RES_BEG))
    channel->write_packet_with_profile(rpmb_frame + RPMB_DATA_BEG, 0x100);
  mmc_set_part_config(card, card->raw_ext_csd[EXT_CSD_PART_CFG] & 0xF8);
  return 0;
}

int cmd_mmc_write_rpmb(com_channel_struct *channel, const char* /*xml*/)
{
  uint8_t rpmb_frame[0x200]={0};
  struct mmc_card* card = (struct mmc_card*)mmc_get_card(0);
  mmc_set_part_config(card, (card->raw_ext_csd[EXT_CSD_PART_CFG] & 0xF8) | 3);
  memset(rpmb_frame, 0, sizeof(rpmb_frame));
  *(uint16_t*)(rpmb_frame + RPMB_TYPE_BEG) = __builtin_bswap16(RPMB_GET_WRITE_COUNTER);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_GET_WRITE_COUNTER, RPMB_REQ);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_GET_WRITE_COUNTER, RPMB_RESP);
  uint32_t size = 4;
  uint16_t address = 0;
  channel->read_packet_with_profile((uint8_t*)&address, &size);
  size = 0x100;
  channel->read_packet_with_profile(rpmb_frame + RPMB_DATA_BEG, &size);
  *(uint16_t*)(rpmb_frame +RPMB_ADDR_BEG) = __builtin_bswap16(address);
  *(uint16_t*)(rpmb_frame +RPMB_BLKS_BEG) = 0x100;
  *(uint16_t*)(rpmb_frame +RPMB_RES_BEG) = 0;
  *(uint16_t*)(rpmb_frame +RPMB_TYPE_BEG) = __builtin_bswap16(RPMB_WRITE_DATA);
  hmac_sha256(rpmb_frame + RPMB_MAC_BEG, rpmb_frame + RPMB_DATA_BEG, 0x200 - RPMB_DATA_BEG, (const uint8_t*)rpmb_key, 0x20);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_WRITE_DATA, RPMB_REQ);
  memset(rpmb_frame, 0, sizeof(rpmb_frame));
  *(uint16_t*)(rpmb_frame +RPMB_TYPE_BEG) = __builtin_bswap16(RPMB_RESULT_READ);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_RESULT_READ, RPMB_REQ);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_RESULT_READ, RPMB_RESP);
  uint16_t res = __builtin_bswap16(*(uint16_t*)(rpmb_frame + RPMB_RES_BEG));
  channel->write_packet_with_profile((uint8_t *)&res, 2);
  mmc_set_part_config(card, card->raw_ext_csd[EXT_CSD_PART_CFG] & 0xF8);
  return 0;
}

int cmd_mmc_init(com_channel_struct *channel, const char* /*xml*/)
{
  uint8_t rpmb_frame[0x200]={0};
  struct mmc_card* card = (struct mmc_card*)mmc_get_card(0);
  mmc_set_part_config(card, (card->raw_ext_csd[EXT_CSD_PART_CFG] & 0xF8) | 3);
  memset(rpmb_frame, 0, sizeof(rpmb_frame));
  *(uint16_t*)(rpmb_frame + RPMB_TYPE_BEG) = __builtin_bswap16(RPMB_GET_WRITE_COUNTER);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_GET_WRITE_COUNTER, RPMB_REQ);
  mmc_rpmb_send_command((struct mmc_card *)card->host, rpmb_frame, 1, RPMB_GET_WRITE_COUNTER, RPMB_RESP);
  mmc_set_part_config(card, card->raw_ext_csd[EXT_CSD_PART_CFG] & 0xF8);
  return rpmb_key_generate(channel, rpmb_frame);
}

__attribute__ ((section(".text.main"))) int main() {
    cache_init(3);
    register_xml_cmd("CMD:CUSTOMACK","1",(void*)cmd_ack);
    register_xml_cmd("CMD:CUSTOMREGW","1",(void*)cmd_writeregister);
    register_xml_cmd("CMD:CUSTOMREGR","1",(void*)cmd_readregister);
    register_xml_cmd("CMD:CUSTOMMEMR","1",(void*)cmd_readmem);
    register_xml_cmd("CMD:CUSTOMMEMW","1",(void*)cmd_writemem);
    register_xml_cmd("CMD:CUSTOMMMCINIT","1",(void*)cmd_mmc_init);
    register_xml_cmd("CMD:CUSTOMRPMBR","1",(void*)cmd_mmc_read_rpmb);
    register_xml_cmd("CMD:CUSTOMRPMBW","1",(void*)cmd_mmc_write_rpmb);
    register_xml_cmd("CMD:CUSTOMUFSINIT","1",(void*)cmd_ufs_init);
    register_xml_cmd("CMD:CUSTOMURPMBR","1",(void*)cmd_ufs_read_rpmb);
    register_xml_cmd("CMD:CUSTOMURPMBW","1",(void*)cmd_ufs_write_rpmb);
    register_xml_cmd("CMD:CUSTOMRPMBKEY","1",(void*)cmd_set_rpmbkey);
    //register_xml_cmd("CUSTOM","1",(void*)register_rw);
    cache_close(1);
    return 0;
}

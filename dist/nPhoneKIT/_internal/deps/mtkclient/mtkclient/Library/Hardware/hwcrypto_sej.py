#!/usr/bin/python3
# -*- coding: utf-8 -*-
# (c) B.Kerler 2018-2024 GPLv3 License
import logging
import os
from struct import pack, unpack
from mtkclient.Library.utils import LogBase, logsetup
from mtkclient.Library.cryptutils import CryptUtils

CustomSeed = bytes.fromhex("00be13bb95e218b53d07a089cb935255294f70d4088f3930350bc636cc49c9025ece7a62c292853ef55b23a6e" +
                           "f7b7464c7f3f2a74ae919416d6b4d9c1d6809655dd82d43d65999cf041a386e1c0f1e58849d8ed09ef07e6a9f" +
                           "0d7d3b8dad6cbae4668a2fd53776c3d26f88b0bf617c8112b8b1a871d322d9513491e07396e1638090055f4b8" +
                           "b9aa2f4ec24ebaeb917e81f468783ea771b278614cd5779a3ca50df5cc5af0edc332e2b69b2b42154bcfffd0a" +
                           "f13ce5a467abb7fb107fe794f928da44b6db7215aa53bd0398e3403126fad1f7de2a56edfe474c5a06f8dd9bc" +
                           "0b3422c45a9a132e64e48fcacf63f787560c4c89701d7c125118c20a5ee820c3a16")


# SEJ = Security Engine for JTAG protection

def bytes_to_dwords(buf):
    res = []
    for i in range(0, len(buf) // 4):
        res.append(unpack("<I", buf[i * 4:(i * 4) + 4])[0])
    return res


class SymKey:
    key = None
    key_len = 0x10
    mode = 1
    iv = None


AES_CBC_MODE = 1
AES_SW_KEY = 0
AES_HW_KEY = 1
AES_HW_WRAP_KEY = 2
AES_KEY_128 = 16
AES_KEY_256 = 32

regval = {
    "HACC_CON": 0x0000,
    "HACC_ACON": 0x0004,
    "HACC_ACON2": 0x0008,
    "HACC_ACONK": 0x000C,
    "HACC_ASRC0": 0x0010,
    "HACC_ASRC1": 0x0014,
    "HACC_ASRC2": 0x0018,
    "HACC_ASRC3": 0x001C,
    "HACC_AKEY0": 0x0020,
    "HACC_AKEY1": 0x0024,
    "HACC_AKEY2": 0x0028,
    "HACC_AKEY3": 0x002C,
    "HACC_AKEY4": 0x0030,
    "HACC_AKEY5": 0x0034,
    "HACC_AKEY6": 0x0038,
    "HACC_AKEY7": 0x003C,
    "HACC_ACFG0": 0x0040,
    "HACC_ACFG1": 0x0044,
    "HACC_ACFG2": 0x0048,
    "HACC_ACFG3": 0x004C,
    "HACC_AOUT0": 0x0050,
    "HACC_AOUT1": 0x0054,
    "HACC_AOUT2": 0x0058,
    "HACC_AOUT3": 0x005C,
    "HACC_SW_OTP0": 0x0060,
    "HACC_SW_OTP1": 0x0064,
    "HACC_SW_OTP2": 0x0068,
    "HACC_SW_OTP3": 0x006c,
    "HACC_SW_OTP4": 0x0070,
    "HACC_SW_OTP5": 0x0074,
    "HACC_SW_OTP6": 0x0078,
    "HACC_SW_OTP7": 0x007c,
    "HACC_SECINIT0": 0x0080,
    "HACC_SECINIT1": 0x0084,
    "HACC_SECINIT2": 0x0088,
    "HACC_MKJ": 0x00a0,
    "HACC_UNK": 0x00bc
}


class HaccReg:
    def __init__(self, setup):
        self.sej_base = setup.sej_base
        self.read32 = setup.read32
        self.write32 = setup.write32

    def __setattr__(self, key, value):
        if key in ("sej_base", "read32", "write32", "regval"):
            return super(HaccReg, self).__setattr__(key, value)
        if key in regval:
            addr = regval[key] + self.sej_base
            return self.write32(addr, value)
        else:
            return super(HaccReg, self).__setattr__(key, value)

    def __getattribute__(self, item):
        if item in ("sej_base", "read32", "write32", "regval"):
            return super(HaccReg, self).__getattribute__(item)
        if item in regval:
            addr = regval[item] + self.sej_base
            return self.read32(addr)
        else:
            return super(HaccReg, self).__getattribute__(item)


class Sej(metaclass=LogBase):
    encrypt = True

    HACC_AES_DEC = 0x00000000
    HACC_AES_ENC = 0x00000001
    HACC_AES_MODE_MASK = 0x00000002
    HACC_AES_ECB = 0x00000000
    HACC_AES_CBC = 0x00000002
    HACC_AES_TYPE_MASK = 0x00000030
    HACC_AES_128 = 0x00000000
    HACC_AES_192 = 0x00000010
    HACC_AES_256 = 0x00000020
    HACC_AES_CHG_BO_MASK = 0x00001000
    HACC_AES_CHG_BO_OFF = 0x00000000
    HACC_AES_CHG_BO_ON = 0x00001000
    HACC_AES_START = 0x00000001
    HACC_AES_CLR = 0x00000002
    HACC_AES_RDY = 0x00008000

    HACC_AES_BK2C = 0x00000010
    HACC_AES_R2K = 0x00000100

    HACC_SECINIT0_MAGIC = 0xAE0ACBEA
    HACC_SECINIT1_MAGIC = 0xCD957018
    HACC_SECINIT2_MAGIC = 0x46293911

    # This seems to be fixed
    g_CFG_RANDOM_PATTERN = [
        0x2D44BB70,
        0xA744D227,
        0xD0A9864B,
        0x83FFC244,
        0x7EC8266B,
        0x43E80FB2,
        0x01A6348A,
        0x2067F9A0,
        0x54536405,
        0xD546A6B1,
        0x1CC3EC3A,
        0xDE377A83
    ]

    g_HACC_CFG_1 = [
        0x9ED40400, 0x00E884A1, 0xE3F083BD, 0x2F4E6D8A,
        0xFF838E5C, 0xE940A0E3, 0x8D4DECC6, 0x45FC0989
    ]

    g_HACC_CFG_2 = [
        0xAA542CDA, 0x55522114, 0xE3F083BD, 0x55522114,
        0xAA542CDA, 0xAA542CDA, 0x55522114, 0xAA542CDA
    ]

    g_HACC_CFG_3 = [
        0x2684B690, 0xEB67A8BE, 0xA113144C, 0x177B1215,
        0x168BEE66, 0x1284B684, 0xDF3BCE3A, 0x217F6FA2
    ]

    g_HACC_CFG_MTEE = [
        0x9ED40400, 0xE884A1, 0xE3F083BD, 0x2F4E6D8A
    ]

    def __init__(self, setup, loglevel=logging.INFO):
        self.__logger, self.info, self.debug, self.warning, self.error = logsetup(self, self.__logger,
                                                                                  loglevel, None)
        self.hwcode = setup.hwcode
        self.reg = HaccReg(setup)
        # mediatek,hacc, mediatek,sej
        self.sej_base = setup.sej_base
        self.read32 = setup.read32
        self.write32 = setup.write32
        if loglevel == logging.DEBUG:
            logfilename = os.path.join("logs", "log.txt")
            fh = logging.FileHandler(logfilename, encoding='utf-8')
            self.__logger.addHandler(fh)
            self.__logger.setLevel(logging.DEBUG)
        else:
            self.__logger.setLevel(logging.INFO)

    @staticmethod
    def uffs(x):
        v1 = x
        if x & 0xFFFF:
            result = 1
        else:
            v1 >>= 16
            result = 17
        if not v1 & 0xFF:
            v1 >>= 8
            result += 8
        if not ((v1 << 28) & 0xFFFFFFFF):
            v1 >>= 4
            result += 4
        if not ((v1 << 30) & 0xFFFFFFFF):
            v1 >>= 2
            result += 2
        if not v1 & 1:
            result += 1
        return result

    def tz_dapc_set_master_transaction(self, master_index, permission_control):
        t = 1 << master_index
        v = self.read32(0x10007500) & ~t
        if t:
            t = self.uffs(t)
        val = v | permission_control << (t - 1)
        self.write32(0x10007500, val)
        return t

    def crypto_secure(self, val):
        if val:
            self.write32(0x10216024, 0x20002)
        else:
            self.write32(0x10216024, 0x0)

    def device_apc_dom_setup(self):
        self.write32(0x10007F00, 0)
        tv = self.read32(0x10007400) & 0xFFFFFFFF
        self.write32(0x10007400, tv | (1 << (self.uffs(0xF0000000) - 1)))
        # tv_0 =
        self.read32(0x10007400) & 0xF0FFFFFF
        self.write32(0x10007400, tv | (2 << (self.uffs(0xF0000000) - 1)))

    def sej_set_mode(self, mode):
        self.reg.HACC_ACON = self.reg.HACC_ACON & ((~2) & 0xFFFFFFFF)
        if mode == 1:  # CBC
            self.reg.HACC_ACON |= 2

    def sej_set_key(self, key, flag, data=None):
        # 0 uses software key (sml_aes_key)
        # 1 uses Real HW Crypto Key
        # 2 uses 32 byte hw derived key from sw key
        # 3 uses 32 byte hw derived key from rid
        # 4 uses custom key (customer key ?)
        klen = 0x10
        if flag == 0x18:
            klen = 0x10
        elif flag == 0x20:
            klen = 0x20
        self.write32(0x109E64, klen)
        self.reg.HACC_ACON = (self.reg.HACC_ACON & 0xFFFFFFCF) | klen
        self.reg.HACC_AKEY0 = 0
        self.reg.HACC_AKEY1 = 0
        self.reg.HACC_AKEY2 = 0
        self.reg.HACC_AKEY3 = 0
        self.reg.HACC_AKEY4 = 0
        self.reg.HACC_AKEY5 = 0
        self.reg.HACC_AKEY6 = 0
        self.reg.HACC_AKEY7 = 0

        if key == 1:
            self.reg.HACC_ACONK |= 0x10
        else:
            # Key has to be converted to be big endian
            keydata = [0, 0, 0, 0, 0, 0, 0, 0]
            for i in range(0, len(data), 4):
                keydata[i // 4] = unpack(">I", data[i:i + 4])[0]
            self.reg.HACC_AKEY0 = keydata[0]
            self.reg.HACC_AKEY1 = keydata[1]
            self.reg.HACC_AKEY2 = keydata[2]
            self.reg.HACC_AKEY3 = keydata[3]
            self.reg.HACC_AKEY4 = keydata[4]
            self.reg.HACC_AKEY5 = keydata[5]
            self.reg.HACC_AKEY6 = keydata[6]
            self.reg.HACC_AKEY7 = keydata[7]

    def tz_pre_init(self):
        # self.device_APC_dom_setup()
        # self.tz_dapc_set_master_transaction(4,1)
        # self.crypto_secure(1)
        return

    def sej_run(self, data):
        pdst = bytearray()
        psrc = bytes_to_dwords(data)
        plen = len(psrc)
        pos = 0
        for i in range(plen // 4):
            self.reg.HACC_ASRC0 = psrc[pos + 0]
            self.reg.HACC_ASRC1 = psrc[pos + 1]
            self.reg.HACC_ASRC2 = psrc[pos + 2]
            self.reg.HACC_ASRC3 = psrc[pos + 3]
            self.reg.HACC_ACON2 = self.HACC_AES_START
            i = 0
            while i < 20:
                if self.reg.HACC_ACON2 & self.HACC_AES_RDY != 0:
                    break
                i += 1
            if i == 20:
                self.error("SEJ Hardware seems not to be configured correctly. Results may be wrong.")
            pdst.extend(pack("<I", self.reg.HACC_AOUT0))
            pdst.extend(pack("<I", self.reg.HACC_AOUT1))
            pdst.extend(pack("<I", self.reg.HACC_AOUT2))
            pdst.extend(pack("<I", self.reg.HACC_AOUT3))
            pos += 4
        return pdst

    def sej_aes_hw_init(self, attr, key: SymKey, sej_param=3):
        # key.mode 0 = ECB
        # key.mode 1 = CBC
        if key.key is None:
            key.key = b""
        if attr << 31 and sej_param << 31:
            if key.key is None:
                return 0x6001
        if key.iv is None and key.mode == 1:
            return 0x6002

        self.reg.HACC_SECINIT0 = 1
        if attr & 1 == 0 or sej_param & 1 != 0:
            acon_setting = self.HACC_AES_128
        elif len(key.key) == 0x18:
            acon_setting = self.HACC_AES_192
        elif len(key.key) == 0x20:
            acon_setting = self.HACC_AES_256
        else:
            acon_setting = self.HACC_AES_192
        if key.mode:
            acon_setting |= self.HACC_AES_CBC
        self.reg.HACC_ACON = acon_setting
        """
        if m_src_addr<<30 or m_dst_addr << 30:
            return 0x6007
        if not m_src_len:
            return 0x600A
        if m_src_len != m_dst_len:
            return 0x6000
        if m_src_len << 29:
            return 0x6032
        memset(outbuf,0,0x20)
        if attr&4 == 0:
           CP_Power_On_SEJ_HW(1)
        """

        if attr & 1 != 0:
            self.reg.HACC_AKEY0 = 0
            self.reg.HACC_AKEY1 = 0
            self.reg.HACC_AKEY2 = 0
            self.reg.HACC_AKEY3 = 0
            self.reg.HACC_AKEY4 = 0
            self.reg.HACC_AKEY5 = 0
            self.reg.HACC_AKEY6 = 0
            self.reg.HACC_AKEY7 = 0
            if sej_param & 1 != 0:
                self.reg.HACC_ACONK = self.HACC_AES_BK2C
            else:
                keydata = [0, 0, 0, 0, 0, 0, 0, 0]
                # toDo: Is this valid ?
                for i in range(0, len(key.key), 4):
                    keydata[i // 4] = unpack(">I", key.key[i:i + 4])[0]
                if len(key.key) >= 8:
                    self.reg.HACC_AKEY0 = keydata[0]
                    self.reg.HACC_AKEY1 = keydata[1]
                if len(key.key) >= 16:
                    self.reg.HACC_AKEY2 = keydata[2]
                    self.reg.HACC_AKEY3 = keydata[3]
                if len(key.key) >= 24:
                    self.reg.HACC_AKEY4 = keydata[4]
                    self.reg.HACC_AKEY5 = keydata[5]
                if len(key.key) >= 32:
                    self.reg.HACC_AKEY6 = keydata[6]
                    self.reg.HACC_AKEY7 = keydata[7]
        if attr & 2 != 0:
            self.reg.HACC_ACON2 = self.HACC_AES_CLR
            self.reg.HACC_ACFG0 = key.iv[0]  # g_AC_CFG
            self.reg.HACC_ACFG1 = key.iv[1]
            self.reg.HACC_ACFG2 = key.iv[2]
            self.reg.HACC_ACFG3 = key.iv[3]

    def sej_aes_hw_internal(self, data, encrypt, attr, sej_param, legacy=True):
        if encrypt:
            self.reg.HACC_ACON |= 1
        if legacy:
            if (attr & 8) != 0 and (sej_param & 2) != 0:
                self.reg.HACC_ACONK |= self.HACC_AES_R2K
            else:
                self.reg.HACC_ACONK &= 0xFFFFFEFF
        pdst = bytearray()
        psrc = bytes_to_dwords(data)
        plen = len(psrc)
        pos = 0
        for i in range(plen // 4):
            self.reg.HACC_ASRC0 = psrc[pos + 0]
            self.reg.HACC_ASRC1 = psrc[pos + 1]
            self.reg.HACC_ASRC2 = psrc[pos + 2]
            self.reg.HACC_ASRC3 = psrc[pos + 3]
            self.reg.HACC_ACON2 = self.HACC_AES_START
            i = 0
            while i < 20:
                if self.reg.HACC_ACON2 & self.HACC_AES_RDY != 0:
                    break
                i += 1
            if i == 20:
                self.error("SEJ Hardware seems not to be configured correctly. Results may be wrong.")
            pdst.extend(pack("<I", self.reg.HACC_AOUT0))
            pdst.extend(pack("<I", self.reg.HACC_AOUT1))
            pdst.extend(pack("<I", self.reg.HACC_AOUT2))
            pdst.extend(pack("<I", self.reg.HACC_AOUT3))
            pos += 4
        if legacy:
            if (attr & 8) != 0 and (sej_param & 2) == 0:
                # Key_Feedback_XOR_Handler
                keylen = 0x20
                for pos in range(keylen // 4):
                    self.reg.HACC_AKEY0[pos] = pdst[pos] ^ self.reg.HACC_AKEY0[pos]
        return pdst

    def sst_init(self, attr, iv, keylen=0x10, mparam=5, key=None):
        self.reg.HACC_SECINIT0 = 1
        if keylen == 0x10 or mparam & 1 != 0 or attr & 1 != 0:
            acon_setting = 0
        elif keylen == 0x18:
            acon_setting = 0x10
        elif keylen == 0x20:
            acon_setting = 0x20
        else:
            acon_setting = 0x0
        if attr & 4 == 0:
            print("SEJ_3DES_HW_SetKey")
        if iv is not None:
            acon_setting |= self.HACC_AES_CBC  # 0
        self.reg.HACC_ACON = acon_setting

        self.reg.HACC_AKEY0 = 0
        self.reg.HACC_AKEY1 = 0
        self.reg.HACC_AKEY2 = 0
        self.reg.HACC_AKEY3 = 0
        self.reg.HACC_AKEY4 = 0
        self.reg.HACC_AKEY5 = 0
        self.reg.HACC_AKEY6 = 0
        self.reg.HACC_AKEY7 = 0
        if mparam & 1 != 0:
            self.reg.HACC_ACONK = 0x10
        else:
            self.reg.HACC_AKEY0 = key[0]
            self.reg.HACC_AKEY1 = key[1]
            self.reg.HACC_AKEY2 = key[2]
            self.reg.HACC_AKEY3 = key[3]
            self.reg.HACC_AKEY4 = key[4]
            self.reg.HACC_AKEY5 = key[5]
            self.reg.HACC_AKEY6 = key[6]
            self.reg.HACC_AKEY7 = key[7]
        if attr & 2 != 0:
            self.reg.HACC_ACON2 = self.HACC_AES_CLR
            self.reg.HACC_ACFG0 = iv[0]  # g_AC_CFG
            self.reg.HACC_ACFG1 = iv[1]
            self.reg.HACC_ACFG2 = iv[2]
            self.reg.HACC_ACFG3 = iv[3]

        """
        if attr&8!=0:
            tmp=self.reg.HACC_SECINIT0|2
        else:
            tmp=self.reg.HACC_SECINIT0&0xFFFFFFFD
        self.reg.HACC_SECINIT0=tmp
        self.reg.HACC_ACON2 |= 0x40000000
        reg = -1
        while reg>=0:
            reg = self.reg.HACC_ACON2
        v=(~1)&0xFFFFFFFF
        self.reg.HACC_SECINIT0&=v
        self.reg.HACC_ACONK=0x10
        self.reg.HACC_ACON|=acon_setting
        """

    def sst_secure_algo_with_level(self, buf, encrypt=True, aes_top_legacy=True):
        seed = (CustomSeed[2] << 16) | (CustomSeed[1] << 8) | CustomSeed[0] | (CustomSeed[3] << 24)
        _iv = [seed, (~seed) & 0xFFFFFFFF, (((seed >> 16) | (seed << 16)) & 0xFFFFFFFF),
               (~((seed >> 16) | (seed << 16)) & 0xFFFFFFFF)]
        key = SymKey()
        key.key = None
        key.key_len = 0x10
        # meta_key_len = 0x10
        key.mode = 1  # CBC
        key.iv = _iv
        if aes_top_legacy:
            sej_param = 3
        else:
            sej_param = 5
        # Cipher Internal
        if sej_param & 0xC != 0:
            if sej_param & 4 != 0:
                # sej_param 5
                attr = 0x3A
            else:
                attr = 0x32
            flag = 1
        else:
            # aes_top_legacy
            attr = 0x33
            flag = 0
        metaflag = not flag
        # CS_MTK_Cipher_Internal
        if metaflag:
            # length=0x10
            attr = 0x5B
            self.sej_aes_hw_init(attr, key, sej_param)
            for pos in range(3):
                src = b"".join([int.to_bytes(val, 4, 'little') for val in self.g_CFG_RANDOM_PATTERN])
                buf2 = self.sej_aes_hw_internal(src, encrypt=False, attr=attr, sej_param=sej_param)
            attr = attr & 0xFFFFFFFA | 4
        else:
            self.sst_init(attr=attr, iv=_iv, keylen=key.key_len, mparam=sej_param, key=key.key)
        buf2 = self.sej_aes_hw_internal(buf, encrypt=encrypt, attr=attr, sej_param=sej_param, legacy=False)
        return buf2

    def sej_terminate(self):
        self.reg.HACC_ACON2 = self.HACC_AES_CLR
        self.reg.HACC_AKEY0 = 0
        self.reg.HACC_AKEY1 = 0
        self.reg.HACC_AKEY2 = 0
        self.reg.HACC_AKEY3 = 0
        self.reg.HACC_AKEY4 = 0
        self.reg.HACC_AKEY5 = 0
        self.reg.HACC_AKEY6 = 0
        self.reg.HACC_AKEY7 = 0

    def SEJ_V3_Init(self, ben=True, iv=None, legacy=False):
        acon_setting = self.HACC_AES_CHG_BO_OFF | self.HACC_AES_128
        if iv is not None:
            acon_setting |= self.HACC_AES_CBC
        if ben:
            acon_setting |= self.HACC_AES_ENC
        else:
            acon_setting |= self.HACC_AES_DEC

        # clear key
        self.reg.HACC_AKEY0 = 0  # 0x20
        self.reg.HACC_AKEY1 = 0
        self.reg.HACC_AKEY2 = 0
        self.reg.HACC_AKEY3 = 0
        self.reg.HACC_AKEY4 = 0
        self.reg.HACC_AKEY5 = 0
        self.reg.HACC_AKEY6 = 0
        self.reg.HACC_AKEY7 = 0  # 0x3C

        # Generate META Key # 0x04
        self.reg.HACC_ACON = self.HACC_AES_CHG_BO_OFF | self.HACC_AES_CBC | self.HACC_AES_128 | self.HACC_AES_DEC

        # init ACONK, bind HUID/HUK to HACC, this may differ
        # enable R2K, so that output data is feedback to key by HACC internal algorithm
        self.reg.HACC_ACONK = self.HACC_AES_BK2C | self.HACC_AES_R2K  # 0x0C

        # clear HACC_ASRC/HACC_ACFG/HACC_AOUT
        self.reg.HACC_ACON2 = self.HACC_AES_CLR  # 0x08

        self.reg.HACC_ACFG0 = iv[0]  # g_AC_CFG
        self.reg.HACC_ACFG1 = iv[1]
        self.reg.HACC_ACFG2 = iv[2]
        self.reg.HACC_ACFG3 = iv[3]

        if legacy:
            self.reg.HACC_UNK |= 2
            # clear HACC_ASRC/HACC_ACFG/HACC_AOUT
            self.reg.HACC_ACON2 = 0x40000000 | self.HACC_AES_CLR
            i = 0
            while i < 20:
                if self.reg.HACC_ACON2 > 0x80000000:
                    break
                i += 1
            if i == 20:
                self.error("SEJ Legacy Hardware seems not to be configured correctly. Results may be wrong.")
            self.reg.HACC_UNK &= 0xFFFFFFFE
            self.reg.HACC_ACONK = self.HACC_AES_BK2C
            self.reg.HACC_ACON = acon_setting
        else:
            # The reg below needed for mtee ?
            self.reg.HACC_UNK = 1

            # encrypt fix pattern 3 rounds to generate a pattern from HUID/HUK
            for i in range(0, 3):
                pos = i * 4
                self.reg.HACC_ASRC0 = self.g_CFG_RANDOM_PATTERN[pos]
                self.reg.HACC_ASRC1 = self.g_CFG_RANDOM_PATTERN[pos + 1]
                self.reg.HACC_ASRC2 = self.g_CFG_RANDOM_PATTERN[pos + 2]
                self.reg.HACC_ASRC3 = self.g_CFG_RANDOM_PATTERN[pos + 3]
                self.reg.HACC_ACON2 = self.HACC_AES_START
                i = 0
                while i < 20:
                    if self.reg.HACC_ACON2 & self.HACC_AES_RDY != 0:
                        break
                    i += 1
                if i == 20:
                    self.error("SEJ Hardware seems not to be configured correctly. Results may be wrong.")

            self.reg.HACC_ACON2 = self.HACC_AES_CLR

            self.reg.HACC_ACFG0 = iv[0]
            self.reg.HACC_ACFG1 = iv[1]
            self.reg.HACC_ACFG2 = iv[2]
            self.reg.HACC_ACFG3 = iv[3]
            self.reg.HACC_ACON = acon_setting
            self.reg.HACC_ACONK = 0
        return acon_setting

    def hw_aes128_cbc_encrypt(self, buf, encrypt=True, iv=None):
        if iv is None:
            iv = self.g_HACC_CFG_1
        self.tz_pre_init()
        self.info("HACC init")
        self.SEJ_V3_Init(ben=encrypt, iv=iv)
        self.info("HACC run")
        buf2 = self.sej_run(buf)
        self.info("HACC terminate")
        self.sej_terminate()
        return buf2

    def sej_set_otp(self, data):
        pd = bytes_to_dwords(data)
        self.reg.HACC_SW_OTP0 = pd[0]
        self.reg.HACC_SW_OTP1 = pd[1]
        self.reg.HACC_SW_OTP2 = pd[2]
        self.reg.HACC_SW_OTP3 = pd[3]
        self.reg.HACC_SW_OTP4 = pd[4]
        self.reg.HACC_SW_OTP5 = pd[5]
        self.reg.HACC_SW_OTP6 = pd[6]
        self.reg.HACC_SW_OTP7 = pd[7]
        # self.reg.HACC_SECINIT0 = pd[8]
        # self.reg.HACC_SECINIT1 = pd[9]
        # self.reg.HACC_SECINIT2 = pd[0xA]
        # self.reg.HACC_MKJ = pd[0xB]

    def sej_do_aes(self, encrypt, iv=None, data=b"", length=16):
        self.reg.HACC_ACON2 |= self.HACC_AES_CLR
        if iv is not None:
            piv = bytes_to_dwords(iv)
            self.reg.HACC_ACFG0 = piv[0]
            self.reg.HACC_ACFG1 = piv[1]
            self.reg.HACC_ACFG2 = piv[2]
            self.reg.HACC_ACFG3 = piv[3]
        if encrypt:
            self.reg.HACC_ACON |= self.HACC_AES_ENC
        else:
            self.reg.HACC_ACON &= 0xFFFFFFFE
        pdst = bytearray()
        for pos in range(0, length, 16):
            psrc = bytes_to_dwords(data[(pos % len(data)):(pos % len(data)) + 16])
            plen = len(psrc)
            pos = 0
            for i in range(plen // 4):
                self.reg.HACC_ASRC0 = psrc[pos + 0]
                self.reg.HACC_ASRC1 = psrc[pos + 1]
                self.reg.HACC_ASRC2 = psrc[pos + 2]
                self.reg.HACC_ASRC3 = psrc[pos + 3]
                self.reg.HACC_ACON2 |= self.HACC_AES_START
                i = 0
                while i < 20:
                    if self.reg.HACC_ACON2 & self.HACC_AES_RDY != 0:
                        break
                    i += 1
                if i == 20:
                    self.error("SEJ Hardware seems not to be configured correctly. Results may be wrong.")
                pdst.extend(pack("<I", self.reg.HACC_AOUT0))
                pdst.extend(pack("<I", self.reg.HACC_AOUT1))
                pdst.extend(pack("<I", self.reg.HACC_AOUT2))
                pdst.extend(pack("<I", self.reg.HACC_AOUT3))
        return pdst

    def sej_key_config(self, swkey):
        iv = bytes.fromhex("57325A5A125497661254976657325A5A")
        self.sej_set_mode(AES_CBC_MODE)
        self.sej_set_key(AES_HW_KEY, AES_KEY_128)
        hw_key = self.sej_do_aes(True, iv, swkey, 32)
        self.sej_set_key(AES_HW_WRAP_KEY, AES_KEY_256, hw_key)

    @staticmethod
    def sej_sec_cfg_sw(data, encrypt=True):
        """
        Left for reference - hw implementation
        --------------------------------------
        self.sej_set_mode(AES_CBC_MODE)
        self.sej_set_key(AES_SW_KEY, AES_KEY_256, b"1A52A367CB12C458965D32CD874B36B2")
        iv = bytes.fromhex("57325A5A125497661254976657325A5A")
        res = self.sej_do_aes(encrypt, iv, data, len(data))
        """
        ctx = CryptUtils.Aes()
        res = ctx.aes_cbc(key=b"25A1763A21BC854CD569DC23B4782B63",
                          iv=bytes.fromhex("57325A5A125497661254976657325A5A"), data=data,
                          decrypt=not encrypt)
        return res

    def xor_data(self, data):
        i = 0
        for val in self.g_HACC_CFG_1:
            data[i:i + 4] = pack("<I", unpack("<I", data[i:i + 4])[0] ^ val)
            i += 4
            if i == 16:
                break
        return data

    def sej_sec_cfg_hw(self, data, encrypt=True):
        if encrypt:
            data = self.xor_data(bytearray(data))
        self.info("HACC init")
        self.SEJ_V3_Init(ben=encrypt, iv=self.g_HACC_CFG_1, legacy=True)
        self.info("HACC run")
        dec = self.sej_run(data)
        self.info("HACC terminate")
        self.sej_terminate()
        if not encrypt:
            dec = self.xor_data(dec)
        return dec

    def sej_sec_cfg_hw_V3(self, data, encrypt=True):
        return self.hw_aes128_cbc_encrypt(buf=data, encrypt=encrypt)

    # seclib_get_msg_auth_key
    def generate_rpmb(self, meid, otp, derivedlen=32):
        # self.sej_sec_cfg_decrypt(bytes.fromhex("1FF7EB9EEA3BA346C2C94E3D44850C2172B56BC26D2450CA9ADBAB7136604542C3B2EA50057037669A4C493BF7CC7E6E2644563808F73B3AA5AFE2D48D97597E"))
        # self.sej_key_config(b"1A52A367CB12C458965D32CD874B36B2")
        # self.sej_set_otp(bytes.fromhex("486973656E7365000023232323232323232323230A006420617320302C207468010000009400000040000000797B797B"))
        self.sej_set_otp(otp)
        buf = bytearray()
        meid = bytearray(meid)  # 0x100010
        for i in range(derivedlen):
            buf.append(meid[i % len(meid)])
        return self.hw_aes128_cbc_encrypt(buf=buf, encrypt=True, iv=self.g_HACC_CFG_1)

    def sp_hacc_internal(self, buf: bytes, b_ac: bool, user: int, b_do_lock: bool, aes_type: int, b_en: bool):
        dec = None
        if user == 0:
            iv = self.g_HACC_CFG_1
            self.info("HACC init")
            self.SEJ_V3_Init(ben=b_en, iv=iv)
            self.info("HACC run")
            dec = self.sej_run(buf)
            self.info("HACC terminate")
            self.sej_terminate()
        elif user == 1:
            iv = self.g_HACC_CFG_2
            self.info("HACC init")
            self.SEJ_V3_Init(ben=b_en, iv=iv)
            self.info("HACC run")
            dec = self.sej_run(buf)
            self.info("HACC terminate")
            self.sej_terminate()
        elif user == 2:
            self.sej_set_key(key=2, flag=32)
            iv = bytes.fromhex("57325A5A125497661254976657325A5A")
            dec = self.sej_do_aes(encrypt=aes_type, iv=iv, data=buf, length=len(buf))
        elif user == 3:
            iv = self.g_HACC_CFG_3
            self.info("HACC init")
            self.SEJ_V3_Init(ben=b_en, iv=iv)
            self.info("HACC run")
            dec = self.sej_run(buf)
            self.info("HACC terminate")
            self.sej_terminate()
        return dec

    def dev_kdf(self, buf: bytes, derivelen=16):
        res = bytearray()
        for i in range(derivelen // 16):
            res.extend(self.sp_hacc_internal(buf=buf[i * 16:(i * 16) + 16], b_ac=True, user=0, b_do_lock=False,
                                             aes_type=1, b_en=True))
        return res

    def generate_mtee(self, otp=None):
        if otp is not None:
            self.sej_set_otp(otp)
        buf = bytes.fromhex("4B65796D61737465724D617374657200")
        return self.dev_kdf(buf=buf, derivelen=16)

    def generate_mtee_meid(self, meid):
        self.sej_key_config(meid)
        res1 = self.sej_do_aes(True, None, meid, 32)
        return self.sej_do_aes(True, None, res1, 32)

    def generate_mtee_hw(self, otp=None):
        if otp is not None:
            self.sej_set_otp(otp)
        self.info("HACC init")
        self.SEJ_V3_Init(ben=True, iv=self.g_HACC_CFG_MTEE)
        self.info("HACC run")
        dec = self.sej_run(bytes.fromhex("7777772E6D6564696174656B2E636F6D30313233343536373839414243444546"))
        self.info("HACC terminate")
        self.sej_terminate()
        return dec

    def generate_hw_meta(self, otp=None, encrypt=False, data=b""):
        """
        WR8                                                                         mt65
        LR9     CRC                 RC4                     AES128-CBC              SBC=OFF
        LR11    CRC                 RC4                     AES128-CBC              SBC=ON
        LR12    CRC                 AES128-ECB              AES128-CBC              mt6750/6797
        LR12A   MD5                 AES128-ECB              AES128-CBC              mt6761/6765/6771/6777/6778/6779
        LR13    MD5                 AES128-ECB              AES128-CBC              mt6781/mt6785
        NR15    MD5                 AES128-ECB              AES128-CBC              mt6877/6889/6833
        NR16    MD5/HMAC-SHA256     AES128-CBC/AES256-CBC   AES128-CBC/AES256-CBC   mt6895
        NR17    MD5/HMAC-SHA256     AES128-CBC/AES256-CBC   AES128-CBC/AES256-CBC
        """
        if otp is not None:
            self.sej_set_otp(otp)
        seed = (CustomSeed[2] << 16) | (CustomSeed[1] << 8) | CustomSeed[0] | (CustomSeed[3] << 24)
        iv = [seed, (~seed) & 0xFFFFFFFF, (((seed >> 16) | (seed << 16)) & 0xFFFFFFFF),
              (~((seed >> 16) | (seed << 16)) & 0xFFFFFFFF)]
        self.info("HACC init")
        self.SEJ_V3_Init(ben=encrypt, iv=iv)
        self.info("HACC run")
        dec = self.sej_run(data)
        self.info("HACC terminate")
        self.sej_terminate()
        return dec


if __name__ == "__main__":
    CustomSeed = int.to_bytes(0x12345678, 4, 'little')
    seed = (CustomSeed[2] << 16) | (CustomSeed[1] << 8) | CustomSeed[0] | (CustomSeed[3] << 24)
    iv = [seed, (~seed) & 0xFFFFFFFF, (((seed >> 16) | (seed << 16)) & 0xFFFFFFFF),
          (~((seed >> 16) | (seed << 16)) & 0xFFFFFFFF)]
    print(b"".join(int.to_bytes(val, 4, 'little') for val in iv).hex())

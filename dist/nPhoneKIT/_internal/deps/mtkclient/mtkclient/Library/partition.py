#!/usr/bin/python3
# -*- coding: utf-8 -*-
# (c) B.Kerler 2018-2024 GPLv3 License
import logging

from mtkclient.Library.utils import LogBase, logsetup
from mtkclient.Library.gpt import GPT
from mtkclient.Library.pmt import PMT


class Partition(metaclass=LogBase):
    def __init__(self, mtk, readflash, read_pmt, loglevel=logging.INFO):
        self.mtk = mtk
        self.__logger, self.info, self.debug, self.warning, self.error = logsetup(self, self.__logger, 
                                                                                  loglevel, mtk.config.gui)
        self.config = self.mtk.config
        self.readflash = readflash
        self.read_pmt = read_pmt
        if self.config.gpt_file is not None:
            self.gptfilename = self.config.gpt_file
            self.readflash = self.readflash_override

    def readflash_override(self, addr: int, length: int, filename: str = "", parttype: str = "",
                           display: bool = False) -> bytes:
        with open(self.gptfilename, "rb") as rf:
            rf.seek(addr)
            data = rf.read(length)
            if filename == "":
                return data
        return b""

    def get_pmt(self, backup: bool = False, parttype: str = "user") -> tuple:
        pt = PMT()
        blocksize = self.mtk.daloader.daconfig.pagesize
        if not backup:
            addr = self.mtk.daloader.daconfig.flashsize - (2 * blocksize)
        else:
            addr = self.mtk.daloader.daconfig.flashsize - (2 * blocksize) + blocksize
        data = self.readflash(addr=addr, length=2 * self.config.pagesize, filename="", parttype=parttype, display=False)
        magic = int.from_bytes(data[:4], 'little')
        if magic in [b"PTv3", b"MPT3"]:
            partdata = data[8:]
            partitions = []
            for partpos in range(128):
                partinfo = pt.PtResident(partdata[partpos * 96:(partpos * 96) + 96])
                if partinfo[:4] == b"\x00\x00\x00\x00":
                    break

                class Partf:
                    unique = b""
                    first_lba = 0
                    last_lba = 0
                    flags = 0
                    sector = 0
                    sectors = 0
                    type = b""
                    name = ""

                pm = Partf()
                pm.name = partinfo.name.rstrip(b"\x00").decode('utf-8')
                pm.sector = partinfo.offset // self.config.pagesize
                pm.sectors = partinfo.size // self.config.pagesize
                pm.type = 1
                pm.flags = partinfo.mask_flags
                partitions.append(pm)
            return data, partitions
        return b"", None

    def get_gpt(self, gpt_settings, parttype: str = "user") -> tuple:
        data = self.readflash(addr=0, length=2 * self.config.pagesize, filename="", parttype=parttype, display=False)
        if data[:4] == b"BPI\x00":
            guid_gpt = GPT(
                num_part_entries=gpt_settings.gpt_num_part_entries,
                part_entry_size=gpt_settings.gpt_part_entry_size,
                part_entry_start_lba=gpt_settings.gpt_part_entry_start_lba,
            )
            data = self.readflash(addr=0, length=32 * self.config.pagesize, filename="",
                                  parttype=parttype, display=False)
            if data == b"":
                return None, None
            guid_gpt.parse_bpi(data, self.config.pagesize)
            return data, guid_gpt
        if data[:9] == b"EMMC_BOOT" and self.read_pmt is not None:
            partdata, partentries = self.read_pmt()
            if partdata == b"":
                return None, None
            else:
                return partdata, partentries
        elif data[:8] == b"UFS_BOOT" and self.read_pmt is not None:
            partdata, partentries = self.read_pmt()
            if partdata == b"":
                return None, None
            else:
                return partdata, partentries
        if data == b"":
            return None, None
        guid_gpt = GPT(
            num_part_entries=gpt_settings.gpt_num_part_entries,
            part_entry_size=gpt_settings.gpt_part_entry_size,
            part_entry_start_lba=gpt_settings.gpt_part_entry_start_lba,
        )
        header = guid_gpt.parseheader(data, self.config.pagesize)
        if header.signature == b'\x00\x00\x00\x00\x00\x00\x00\x00':
            data = self.readflash(addr=self.mtk.daloader.daconfig.flashsize - 0x4000, length=2 * self.config.pagesize,
                                  filename="", parttype=parttype, display=False)
            header = guid_gpt.parseheader(data, self.config.pagesize)
            if header.signature == b'\x00\x00\x00\x00\x00\x00\x00\x00':
                return None, None
        sectors = header.first_usable_lba
        if sectors == 0:
            return None, None
        data = self.readflash(addr=0, length=sectors * self.config.pagesize, filename="",
                              parttype=parttype, display=False)
        if data == b"":
            return None, None
        guid_gpt.parse(data, self.config.pagesize)
        return data, guid_gpt

    def get_backup_gpt(self, lun, gpt_num_part_entries, gpt_part_entry_size, gpt_part_entry_start_lba,
                       parttype="user") -> bytes:
        data = self.readflash(addr=0, length=2 * self.config.pagesize, filename="", parttype=parttype, display=False)
        if data == b"":
            return data
        guid_gpt = GPT(
            num_part_entries=gpt_num_part_entries,
            part_entry_size=gpt_part_entry_size,
            part_entry_start_lba=gpt_part_entry_start_lba,
        )
        header = guid_gpt.parseheader(data, self.config.SECTOR_SIZE_IN_BYTES)
        sectors = header.first_usable_lba - 1
        data = self.readflash(addr=header.backup_lba * self.config.pagesize,
                              length=sectors * self.config.pagesize, filename="",
                              display=False)
        return data

#!/usr/bin/env python3
import sys
import hashlib
from mtkclient.Library.utils import find_binary

patches = [
    ("B3F5807F01D1", "B3F5807F01D14FF000004FF000007047"),  # rsa_verify / usbdl_vfy_da
    ("B3F5807F04BF4FF4807305F011B84FF0FF307047", "B3F5807F04BF4FF480734FF000004FF000007047"),
    # rsa_verify / usbdl_vfy_da
    ("2DE9F746802B", "4FF000007047"),  # rsa_verify / usbdl_vfy_da
    ("802B2DE9", "4FF000007047"),
    ("8023BDE8", "4FF000007047"),  # DA verify fail
    ("800053E3F344", "0000A0E31EFF2FE1")
]


def patch_preloader_security(data):
    if data[:4] != b"\x4D\x4D\x4D\x01":
        return data
    patched = False
    for patchval in patches:
        pattern = bytes.fromhex(patchval[0])
        idx = data.find(pattern)
        if idx != -1:
            patch = bytes.fromhex(patchval[1])
            data[idx:idx + len(patch)] = patch
            patched = True
            break
    if patched:
        # with open(sys.argv[1]+".patched","wb") as wf:
        #    wf.write(data)
        #    print("Patched !")
        print("Patched preloader security")
    else:
        print(f"Failed to patch preloader security: {sys.argv[1]}")
    return data


def patch_da2(da2):
    # open("da2.bin","wb").write(da2)
    da2patched = bytearray(da2)
    # Patch security
    is_security_enabled = find_binary(da2, b"\x01\x23\x03\x60\x00\x20\x70\x47")
    if is_security_enabled is not None:
        da2patched[is_security_enabled:is_security_enabled + 2] = b"\x00\x23"
    else:
        print("Security check not patched.")
    # Patch hash check
    authaddr = find_binary(da2, b"\x04\x00\x07\xC0")
    if authaddr:
        da2patched[authaddr:authaddr + 4] = b"\x00\x00\x00\x00"
    elif authaddr is None:
        authaddr = find_binary(da2, b"\x4F\xF0\x04\x09\xCC\xF2\x07\x09")
        if authaddr:
            da2patched[authaddr:authaddr + 8] = b"\x4F\xF0\x00\x09\x4F\xF0\x00\x09"
        else:
            authaddr = find_binary(da2, b"\x4F\xF0\x04\x09\x32\x46\x01\x98\x03\x99\xCC\xF2\x07\x09")
            if authaddr:
                da2patched[authaddr:authaddr + 14] = b"\x4F\xF0\x00\x09\x32\x46\x01\x98\x03\x99\x4F\xF0\x00\x09"
            else:
                print("Hash check not patched.")
    # Patch write not allowed
    # open("da2.bin","wb").write(da2patched)
    idx = 0
    patched = False
    while idx != -1:
        idx = da2patched.find(b"\x37\xB5\x00\x23\x04\x46\x02\xA8")
        if idx != -1:
            da2patched[idx:idx + 8] = b"\x37\xB5\x00\x20\x03\xB0\x30\xBD"
            patched = True
        else:
            idx = da2patched.find(b"\x0C\x23\xCC\xF2\x02\x03")
            if idx != -1:
                da2patched[idx:idx + 6] = b"\x00\x23\x00\x23\x00\x23"
                idx2 = da2patched.find(b"\x2A\x23\xCC\xF2\x02\x03")
                if idx2 != -1:
                    da2patched[idx2:idx2 + 6] = b"\x00\x23\x00\x23\x00\x23"
                """
                idx3 = da2patched.find(b"\x2A\x24\xE4\xF7\x89\xFB\xCC\xF2\x02\x04")
                if idx3 != -1:
                    da2patched[idx3:idx3 + 10] = b"\x00\x24\xE4\xF7\x89\xFB\x00\x24\x00\x24"
                """
                patched = True
    if not patched:
        print("Write not allowed not patched.")
    return da2patched


def fix_hash(da1, da2, hashpos, hashmode):
    da1 = bytearray(da1)
    dahash = None
    if hashmode == 1:
        dahash = hashlib.sha1(da2).digest()
    elif hashmode == 2:
        dahash = hashlib.sha256(da2).digest()
    da1[hashpos:hashpos + len(dahash)] = dahash
    return da1


def compute_hash_pos(da1, da2):
    hashdigest = hashlib.sha1(da2).digest()
    hashdigest256 = hashlib.sha256(da2).digest()
    idx = da1.find(hashdigest)
    hashmode = 1
    if idx == -1:
        idx = da1.find(hashdigest256)
        hashmode = 2
    if idx != -1:
        return idx, hashmode
    return None, None


def main():
    """
    with open(sys.argv[1],"rb") as rf:
        data=bytearray(rf.read())
        data=patch_preloader_security(data)
    """
    da1 = open("loaders/8167_200000MTK_AllInOne_DA_5.2136.bin", "rb").read()
    da2 = open("loaders/8167_40000000MTK_AllInOne_DA_5.2136.bin", "rb").read()
    hp, hm = compute_hash_pos(da1, da2[:-0x100])
    da2 = patch_da2(da2)
    fix_hash(da1, da2, hp, hm)


if __name__ == "__main__":
    main()

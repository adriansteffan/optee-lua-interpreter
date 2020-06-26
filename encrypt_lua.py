"""
Takes a lua script or a directory of lua files and generates ".luata" files, which have the following format:

[salt (16 Bytes)][mac (64 Bytes)][nonce (8 Byte)][aes encrypted lua script]

These files are used to savely transmit scripts to the lua runtime inside the OP_TEE TA.
"""

import sys
import os

from Crypto.Cipher import AES
from Crypto.Util import Counter
from Crypto.Protocol.KDF import HKDF
from Crypto.Hash import SHA512, HMAC
from Crypto.Random import get_random_bytes


# For testing purposes only, non critical key 
master_hex = '432A462D4A614E645267556A586E3272357538782F413F4428472B4B62506553'

nonce = get_random_bytes(8)
salt = get_random_bytes(16)

# generate two keys from the masterkey, one used for encryption, one for generating a mac over the encryped
key_aes, key_hmac = HKDF(bytes.fromhex(master_hex), 32, salt, SHA512, 2)


def encrypt_and_mac_file_content(data):

    # encrypt the lua plaintext
    cipher = AES.new(key_aes, AES.MODE_CTR, nonce=nonce)
    encrypted_lua = cipher.encrypt(data)


    # HMAC the nonce + encryped lua script
    hash_target = nonce + encrypted_lua
    h = HMAC.new(key_hmac, digestmod=SHA512)
    h.update(hash_target)

    # attach salt, mac and noce to the data to be sent to the TA
    payload = salt + h.digest() + hash_target

    return payload



if len(sys.argv) != 2:
    print("Invalid arguments")
    sys.exit(0)

path = sys.argv[1]
lua_files = []

if os.path.isfile(path):
    lua_files.append(path.split(".")[0])

elif os.path.isdir(path):
    for script in os.listdir(path):
        if script.endswith(".lua"):
            lua_files.append(path+"/"+script.split(".")[0])


for filename in lua_files:
    
    with open(filename+'.lua', 'rb') as file:
            data = file.read()

    payload = encrypt_and_mac_file_content(data)

    with open(filename+'.luata', 'wb') as file:
            file.write(payload)
"""
Takes a lua script named "script.lua" and generates "encrypted.lua", which is a file in the following format:

[salt (16 Bytes)][mac (64 Bytes)][nonce (8 Byte)][aes encrypted lua script]

This file is used to savely transmit scripts to the lua runtime inside the OP_TEE TA.
"""

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


with open('script.lua', 'rb') as file:
    data = file.read()


# encrypt the lua plaintext
cipher = AES.new(key_aes, AES.MODE_CTR, nonce=nonce)
encrypted_lua = cipher.encrypt(data)


# HMAC the nonce + encryped lua script
hash_target = nonce + encrypted_lua
h = HMAC.new(key_hmac, digestmod=SHA512)
h.update(hash_target)

# attach salt, mac and noce to the data to be sent to the TA
payload = salt + h.digest() + hash_target

with open('encrypted.luata', 'wb') as file:
    file.write(payload)

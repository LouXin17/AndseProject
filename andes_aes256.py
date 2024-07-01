import json, secrets
from base64 import b64encode
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad


class AES_PROSSES:
    # QRcode
    def gen_aes_data(data_payload):
        key = "imacGetTopRewardimacGetTopReward".encode("utf-8")
        iv = secrets.token_bytes(8).hex().encode("utf-8")
        data = json.dumps(data_payload).encode("utf-8")
        cipher = AES.new(key, AES.MODE_CBC, iv)
        Crypto_data = cipher.encrypt(pad(data, AES.block_size))
        output = str(iv.decode("utf-8")) + str(b64encode(Crypto_data).decode("utf-8"))
        return output

    # All_Photo
    def gen_all_aes_data(data_payload):
        key = "imacGetTopRewardimacGetTopReward".encode("utf-8")
        iv = secrets.token_bytes(16)
        data = data_payload
        cipher = AES.new(key, AES.MODE_CBC, iv)
        Crypto_data = cipher.encrypt(pad(data, AES.block_size))
        output_all = iv + Crypto_data
        print("AES iv: ", iv)
        print("AES data len: ", len(Crypto_data))
        return output_all

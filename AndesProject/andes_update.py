from andes_epaper import GenerateEPaperImage
from andes_file import PROSSES_FILE
from andes_aes256 import AES_PROSSES
from andes_mqtt import MQTT_SEND

import zlib

# --------------- 流程圖 ---------------- #

# backend: 原圖 > Zlib壓縮 > AES256加密 > MQTT發送

# esp32:   MQTT接收 > UART(Serial2 [RX:16 TX:17]) 發送

# ads:     UART(SoftWareSerial [RX:2 TX:3]) 接收 > AES解密 > Miniz解壓 > 電子紙

# -------------------------------------- #

# EPAPER(W、H)
EPAPER_W, EPAPER_H = 640, 384

# EX data
Payload = {
    "number": "P-12345678",
    "id": "1",
    "name": "XXXX-XXX-XXX",
    "case": "frcture",
    "medication": "Sumatriptan, Sanikit",
    "notice": "none",
    "location": "2F-01",
    "bed number": "1",
}

# EPAPER_payload
Epaper_Payload = {
    "Location": Payload["location"],
    "Bed number": Payload["bed number"],
    "Number": Payload["number"],
    "Name": Payload["name"],
    "Case": Payload["case"],
    "Medication": Payload["medication"],
    "Notice": Payload["notice"],
}

# QRcode_payload
QR_Payload = {"number": Payload["number"], "id": Payload["id"]}

# Gen_Image
GenerateEPaperImage(EPAPER_W, EPAPER_H).gen_image(Epaper_Payload)

# Gen_QRcode & Convert
QR_Image = GenerateEPaperImage(EPAPER_W, EPAPER_H).add_qrcode(QR_Payload)

# Zlib_Compress
Compress_Data = zlib.compress(QR_Image.byte_data, 9)
print("Compress_Data len: ", len(Compress_Data))

# AES_All_Image
AES_Bytes = AES_PROSSES.gen_all_aes_data(Compress_Data)
print("AES_Bytes len: ", len(AES_Bytes))

# Save_Json
PROSSES_FILE.binary_image_2_base64_json(AES_Bytes, "CompressAllPhoto.json")

# MQTT_Send
MQTT_SEND.send_data()

import cv2, qrcode, zlib
import numpy as np
from typing import Text, List, Dict
from andes_aes256 import AES_PROSSES


class GenerateEPaperImage:
    text_max_length: int = 16
    color_depth: int = 2  # bit
    color: Dict = {"white": 0b11, "red": 0b01, "black": 0b00}
    image = None
    img_data = None
    byte_data = None

    def __init__(self, width: int = 640, height: int = 384, color: Dict = None):
        self.width = width
        self.height = height
        if color:
            self.color = color
            self.color_depth = len(bin(max(color.values()))) - 2

    # 顏色分類白紅黑
    @classmethod
    def _convert_color(cls, color_value: List[int], threshold: int = 128) -> int:
        if (
            color_value[0] > threshold
            and color_value[1] > threshold
            and color_value[2] > threshold
        ):
            return cls.color["white"]
        elif color_value[2] > threshold:
            return cls.color["red"]
        else:
            return cls.color["black"]

    # 圖像文字換行排版
    @classmethod
    def _split_text(cls, raw_text: Text) -> List[Text]:
        text_list = [""]
        for word in raw_text.split("_"):
            word += " "
            if len(text_list[-1] + word) >= cls.text_max_length:
                text_list.append(word)
            else:
                text_list[-1] += word
        return [word.strip() for word in text_list]

    # 圖像生成
    def gen_image(self, data: Dict):
        image = np.zeros((self.height, self.width, 3), np.uint8)
        image.fill(255)
        max_chars_per_line = 36
        y_offset = 50
        line_spacing = 30
        font_size = 0.8
        used_keys = set()

        for key, value in data.items():
            if key in used_keys:
                continue
            used_keys.add(key)
            text = f"{key}: {value}"
            if len(text) < max_chars_per_line:
                if "," in value:
                    value_lines = value.split(", ")
                    cv2.putText(
                        image,
                        key + ":",
                        (20, y_offset),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        font_size,
                        (0, 0, 0),
                        2,
                        cv2.LINE_AA,
                    )
                    y_offset += line_spacing
                    for line in value_lines:
                        cv2.putText(
                            image,
                            line,
                            (20, y_offset),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            font_size,
                            (0, 0, 0),
                            2,
                            cv2.LINE_AA,
                        )
                        y_offset += line_spacing
                else:
                    cv2.putText(
                        image,
                        text,
                        (20, y_offset),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        font_size,
                        (0, 0, 0),
                        2,
                        cv2.LINE_AA,
                    )
                    y_offset += line_spacing
            else:
                lines = [
                    text[i : i + max_chars_per_line]
                    for i in range(0, len(text), max_chars_per_line)
                ]
                for line in lines:
                    cv2.putText(
                        image,
                        line,
                        (20, y_offset),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        font_size,
                        (0, 0, 0),
                        2,
                        cv2.LINE_AA,
                    )
                    y_offset += line_spacing
        self.image = cv2.vconcat(image)
        cv2.imwrite("./photo_temp/photo.png", image)
        return self

    # QRcode生成
    def gan_qrcode(self, encrypt_aes_data):  # QR資料
        qr = qrcode.QRCode(
            version=1,
            error_correction=qrcode.constants.ERROR_CORRECT_L,
            box_size=4,
            border=0.6,
        )
        qr.add_data(encrypt_aes_data)
        qr.make(fit=True)
        qr_img = qr.make_image(fill_color="black", back_color="white")
        qr_img.save(f"./photo_temp/AES_QRcode.png")

    # QRcode加密與合併
    def add_qrcode(self, qr_data):
        encrypt_aes_data = AES_PROSSES.gen_aes_data(qr_data)
        self.gan_qrcode(encrypt_aes_data)  # 在此AES加密QRcode
        qr_image = cv2.imread("./photo_temp/AES_QRcode.png")
        e_paper_image = cv2.imread("./photo_temp/photo.png")
        qr_image_np = cv2.cvtColor(
            cv2.cvtColor(qr_image, cv2.COLOR_RGB2BGR), cv2.COLOR_BGR2RGB
        )
        qr_x, qr_y = 470, 220
        e_paper_image[
            qr_y : qr_y + qr_image_np.shape[0], qr_x : qr_x + qr_image_np.shape[1]
        ] = qr_image_np
        cv2.imwrite("./photo_temp/result_image.png", e_paper_image)
        self.result_image = cv2.imread("./photo_temp/result_image.png")
        self.convert_image_to_data()
        return self

    # 圖像資料處理
    def convert_image_to_data(self):
        step = 8 // self.color_depth
        image_length = self.result_image.shape[0] * self.result_image.shape[1] // step

        self.img_data = np.zeros((image_length,), dtype=np.uint8)
        for i in range(self.result_image.shape[0]):
            for j in range(0, self.result_image.shape[1], step):
                val = 0
                for k in range(step):
                    val |= self._convert_color(self.result_image[i][j + k])
                    val <<= self.color_depth
                val >>= self.color_depth
                self.img_data[(i * self.result_image.shape[1] + j) // step] = val
        self.byte_data = self.img_data.tobytes()
        self.compress_data = zlib.compress(self.byte_data, 9)
        return self

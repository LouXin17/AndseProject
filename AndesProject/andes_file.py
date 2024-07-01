import json, base64


class PROSSES_FILE:
    @staticmethod  # 二進制圖像資料存json
    def binary_image_2_base64_json(byte_image, json_load):
        binary_image_data = {
            "base64_data": base64.b64encode(byte_image).decode("UTF-8")
        }
        with open(json_load, "w") as json_file:
            json.dump(binary_image_data, json_file)

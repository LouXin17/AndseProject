#define MINIZ_NO_TIME
/* Import */
#include <Arduino.h>
#include "miniz/miniz.c"
#include <SoftwareSerial.h>
#include <SPI.h>
#include "epd7in5b.h"
#include "epd7in5b_ext.h"

#include <Crypto.h>
#include <AES.h>
#include <string.h>

/* --------------- 流程圖 ---------------- */

// backend: 原圖 > Zlib壓縮 > AES256加密 > MQTT發送

// esp32:   MQTT接收 > UART(Serial2 [RX:16 TX:17]) 發送

// ads:     UART(SoftWareSerial [RX:2 TX:3]) 接收 > AES解密 > Miniz解壓 > 電子紙

/* ----------------- Buffer ----------------- */

// IMAGE
#define IMAGE_DISP_BUF 61440
// Base64、AES、Miniz
#define PRES_BUFFER 5000

/* ---------------- Prepare ----------------- */

// E-Paper
#define EPD_COLOR_DEEPTH 2
#ifndef EPD_COLOR_DEEPTH
#define IMAGE_LENGTH EPD_WIDTH *EPD_HEIGHT / 8
#else
#define IMAGE_LENGTH EPD_WIDTH *EPD_HEIGHT / 8 * EPD_COLOR_DEEPTH
#endif

Epd epd;

SoftwareSerial N25Serial(2, 3); // RX, TX

/* --------------- Variable ----------------- */

// BufferV
uint8_t processBuf[PRES_BUFFER];
uint8_t IMGBuf[IMAGE_DISP_BUF];

// Dynamic Buffer
char *DynamicBuffer = NULL;
byte *ByteDynamicBuffer = NULL;

// AES KEY、IV
const byte KEY[32] = {
    'i', 'm', 'a', 'c', 'G', 'e', 't', 'T', 'o', 'p', 'R', 'e', 'w', 'a', 'r', 'd',
    'i', 'm', 'a', 'c', 'G', 'e', 't', 'T', 'o', 'p', 'R', 'e', 'w', 'a', 'r', 'd'};

// Base64 chars
const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

boolean NewSerMsg = false;

/* -------------- Subroutine ---------------- */

void GetSerMsg()
{
    static int index = 0;
    int SerMsgSize = 0;
    if (DynamicBuffer == NULL)
    {
        SerMsgSize = N25Serial.available();
        DynamicBuffer = (char *)malloc((SerMsgSize + 1) * sizeof(char));
        if (DynamicBuffer == NULL)
        {
            Serial.println("[GetSerMsg] Failed to allocate memory");
            return;
        }
    }
    while (N25Serial.available() && !NewSerMsg)
    {
        char rc = N25Serial.read();
        if (rc != '\n')
        {
            if (index < PRES_BUFFER - 1)
            {
                DynamicBuffer[index] = rc;
                index++;
            }
        }
        else
        {
            DynamicBuffer[index] = '\0';
            index = 0;
            NewSerMsg = true;
        }
    }
}

bool isBase64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

int Base64Decode(const char *input, size_t inputLength, uint8_t *output)
{
    Serial.println("------------- decodeBase64 -------------");
    int i = 0, j = 0, in = 0;
    uint8_t char_array_4[4], char_array_3[3];
    size_t outputLength = 0;

    while (inputLength-- && (input[in] != '=') && isBase64(input[in]))
    {
        char_array_4[i++] = input[in];
        in++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
                char_array_4[i] = strchr(base64_chars, char_array_4[i]) - base64_chars;
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
            {
                output[outputLength++] = char_array_3[i];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = strchr(base64_chars, char_array_4[j]) - base64_chars;

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++)
        {
            output[outputLength++] = char_array_3[j];
        }
    }
    return outputLength;
}

uint8_t *AESDecode(uint8_t *C_Data, size_t C_Length, size_t &D_Length)
{
    Serial.println("-------------- AESDecode --------------");

    size_t Encrypt_D_Length = C_Length - 16;
    byte *Encrypt_Data = C_Data + 16;

    ByteDynamicBuffer = (byte *)malloc(Encrypt_D_Length);
    if (!ByteDynamicBuffer)
    {
        Serial.println("[AESDecode] Memory allocation failed!");
        return nullptr;
    }

    AES256 cipher;
    cipher.setKey(KEY, sizeof(KEY));
    int blockSize = cipher.blockSize();

    byte iv[16];
    memcpy(iv, C_Data, 16);

    Serial.print("[AESDecode] IV: ");
    for (int Count = 0; Count < 16; Count++)
    {
        Serial.print((char)iv[Count]);
    }
    Serial.println();

    for (unsigned int i = 0; i < Encrypt_D_Length; i += blockSize)
    {
        cipher.decryptBlock(ByteDynamicBuffer + i, Encrypt_Data + i);
        for (int j = 0; j < blockSize; j++)
        {
            ByteDynamicBuffer[i + j] ^= iv[j];
        }
        memcpy(iv, Encrypt_Data + i, blockSize);
    }

    int PadValue = ByteDynamicBuffer[Encrypt_D_Length - 1];
    int UnpadLength = Encrypt_D_Length - PadValue;

    Serial.print("[AESDecode] blockSize: ");
    Serial.println(blockSize);
    Serial.print("[AESDecode] PadValue: ");
    Serial.println(PadValue);
    Serial.print("[AESDecode] UnpadLength: ");
    Serial.println(UnpadLength);

    D_Length = UnpadLength;
    return ByteDynamicBuffer;
}

void MinizDecompress(uint8_t *C_Data, size_t C_Length) // decompressData
{
    mz_ulong D_Size = IMAGE_DISP_BUF;

    Serial.print("[MinizDecompress] D_Size: ");
    Serial.println(C_Length);

    int Result = mz_uncompress(IMGBuf, &D_Size, C_Data, C_Length);
    if (Result == MZ_OK)
    {
        Serial.println("[MinizDecompress] Miniz success:");
        N25Serial.write('0');
        DisplayFrame(IMGBuf);
    }
    else
    {
        Serial.print("[MinizDecompress] Miniz failed, rc: ");
        Serial.println(Result);
        N25Serial.write('F');
    }
}

void freeDynamicBuffer(char **buffer)
{
    if (*buffer != NULL)
    {
        free(*buffer);
        *buffer = NULL;
    }
}

void freeByteDynamicBuffer(byte **buffer)
{
    if (*buffer != NULL)
    {
        free(*buffer);
        *buffer = NULL;
    }
}

void setup()
{
    // Serial
    Serial.begin(115200);
    N25Serial.begin(19200);

    // E-Paper
    if (epd.Init() != 0)
    {
        Serial.print("[setup] e-Paper init failed");
        return;
    }
    Serial.println("[setup] Start clean");
    epd.Clean();
    Serial.println("[setup] End clean");
}

void loop()
{
    GetSerMsg();
    if (NewSerMsg)
    {
        const char *SerData = DynamicBuffer;
        freeDynamicBuffer(&DynamicBuffer);

        Serial.print("[loop] SerData: ");
        Serial.println(strlen(SerData));

        int Base64_C_Length = Base64Decode(SerData, strlen(SerData), processBuf);
        Serial.print("[loop] Base64_C_Length: ");
        Serial.println(Base64_C_Length);
        if (Base64_C_Length > 0)
        {
            size_t D_Length;
            uint8_t *AESD_Data = AESDecode(processBuf, Base64_C_Length, D_Length);
            freeByteDynamicBuffer(&ByteDynamicBuffer);
            Serial.print("[loop] AESDecode D_Length: ");
            Serial.println(D_Length);
            MinizDecompress(AESD_Data, D_Length);
        }
        else
        {
            Serial.println("[loop] Base64 Decode failed, Retry...");
            N25Serial.write('F');
        }
        NewSerMsg = false;
    }
}
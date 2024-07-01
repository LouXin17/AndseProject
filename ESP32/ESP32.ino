#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* --------------- 流程圖 ---------------- */

// backend: 原圖 > Zlib壓縮 > AES256加密 > MQTT發送

// esp32:   MQTT接收 > UART(Serial2 [RX:16 TX:17]) 發送

// ads:     UART(SoftWareSerial [RX:2 TX:3]) 接收 > AES解密 > Miniz解壓 > 電子紙

/* -------------------------------------- */

// Serial 2
#define RXD2 16
#define TXD2 17

WiFiClient espClient;
PubSubClient client(espClient);
char *dataValue = NULL;

const char *ssid = "2706";
const char *password = "rootroot";
const char *mqtt_server = "10.20.1.227";
const int mqtt_port = 1883;

const int chunkSize = 128;
bool send_status = false;
bool Get_Mqtt_payload = false;

void setup_wifi()
{
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.print(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        messageTemp += (char)message[i];
    }
    Serial.println(messageTemp);

    StaticJsonDocument<2000> doc;
    DeserializationError error = deserializeJson(doc, messageTemp);

    if (!error)
    {
        const char *data = doc["data"];       // 提取 data 鍵值
        int part = doc["part"];               // 提取 part 鍵值
        int total_parts = doc["total_parts"]; // 提取 total_parts 鍵值
        Serial.print("total_parts: ");
        Serial.println(total_parts);
        Serial.print("part: ");
        Serial.println(part);

        if (part == 1)
        {
            if (dataValue != NULL)
            {
                free(dataValue);
            }
            dataValue = (char *)malloc(1);
            dataValue[0] = '\0';
        }
        if (data)
        {
            size_t newLen = strlen(dataValue) + strlen(data) + 1;
            dataValue = (char *)realloc(dataValue, newLen);
            strcat(dataValue, data);
            if (part == total_parts)
            {
                Serial.print("dataValue: ");
                Serial.println(strlen(dataValue));
                Get_Mqtt_payload = true;
            }
        }
        else
        {
            Serial.println("No data key found in JSON");
        }
    }
    else
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
    }
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client"))
        {
            Serial.println("connected");
            client.subscribe("ACET/image/payload");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}
void loop()
{
    if (Get_Mqtt_payload == true && send_status == false)
        sendBase64Chunks(dataValue, chunkSize);
    while (Serial2.available() && send_status == true)
    {
        char rc = Serial2.read();
        if (rc == 'F')
        {
            Serial.println("failed... Retry");
            send_status = false;
            sendBase64Chunks(dataValue, chunkSize);
        }
        else if (rc == '0')
        {
            Serial.println("sended.");
        }
        else
        {
            Serial.println("no command... Retry");
        }
        Get_Mqtt_payload = false;
    }
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
}

void sendBase64Chunks(const char *data, int chunkSize)
{
    Serial.print("sending...");
    int dataLength = strlen(data);
    for (int i = 0; i < dataLength; i += chunkSize)
    {
        Serial.print(".");
        char chunk[chunkSize + 1];
        int j;
        for (j = 0; j < chunkSize && (i + j) < dataLength; j++)
        {
            chunk[j] = data[i + j];
        }
        chunk[j] = '\0';
        Serial2.write(chunk);
        delay(10);
    }
    Serial2.write('\n');
    Serial.println("done.");
    send_status = true;
}
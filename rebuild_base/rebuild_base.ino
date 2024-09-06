#include <WiFi.h>
#include <PubSubClient.h>

#define MAX_PACKET_SIZE 700

const char* ssid = "OPTIMAROBOTICS2023";
const char* password = "3202SCITOBORAMITPO";
const char* mqttServer = "broker.mqtt-dashboard.com";
char status[2]; // Cần ít nhất 2 ký tự để chứa chuỗi 'check_connect'
int checkConnect = 0;

WiFiClient espClient;
PubSubClient client(espClient);

size_t packetIndex = 0; // Tên biến rõ ràng hơn

// Hàm kết nối WiFi
void connectToWiFi() {
    delay(10);
    Serial.println();
    Serial.print("Kết nối tới WiFi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);  // Chế độ station
    WiFi.begin(ssid, password);

    // Đợi cho đến khi WiFi kết nối thành công
    while (WiFi.status() != WL_CONNECTED) {
        delay(50);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Kết nối WiFi thành công");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());
}

// Hàm kết nối lại MQTT
void reconnectToMQTT() {
    // Tiếp tục thử kết nối lại nếu chưa kết nối thành công
    while (!client.connected()) {
        Serial.print("Đang kết nối tới MQTT...");
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);  // Tạo client ID ngẫu nhiên

        if (client.connect(clientId.c_str())) {
            Serial.println("Đã kết nối MQTT thành công");
        } else {
            Serial.print("Kết nối thất bại, mã lỗi: ");
            Serial.print(client.state());
            Serial.println(". Thử lại sau 0.5 giây...");
            delay(500);
        }
    }
}

// Hàm chuyển đổi từ mảng ký tự sang mảng byte
void charArrayToByteArray(const char* charArray, byte* byteArray, size_t length) {
    for (size_t i = 0; i < length; i++) {
        byteArray[i] = static_cast<byte>(charArray[i]);
    }
}

// Cấu hình khởi động
void setup() {
    Serial.begin(9600);  // Kết nối với Serial monitor
    Serial1.begin(9600, SERIAL_8N1, 19, 18);  // Khởi động Serial1 với các thông số UART
    Serial1.setTimeout(200);
    Serial.setTimeout(200);

    connectToWiFi();  // Gọi hàm kết nối WiFi
    client.setServer(mqttServer, 1883);  // Đặt máy chủ MQTT và cổng
}

// Vòng lặp chính
void loop() {
    if (!client.connected()) {
        reconnectToMQTT();
        checkConnect++;
        snprintf(status, sizeof(status), "%d", checkConnect);  // Sử dụng snprintf an toàn hơn
    }
    client.loop();  // Duy trì kết nối với MQTT

    char buffer[MAX_PACKET_SIZE];

    // Kiểm tra xem Serial1 có dữ liệu không
    if (Serial1.available()) {
        size_t bytesRead = Serial1.readBytes(buffer, MAX_PACKET_SIZE);

        // Nếu đọc được ít nhất 250 byte
        if (bytesRead >= 250) {
            char hexData[3 * bytesRead + 1];  // Chuỗi để lưu dữ liệu hex
            size_t hexIndex = 0;

            // Chuyển đổi dữ liệu sang dạng chuỗi hex
            for (packetIndex = 0; packetIndex < bytesRead; packetIndex++) {
                snprintf(&hexData[hexIndex], sizeof(hexData) - hexIndex, "%02X ", buffer[packetIndex]);
                hexIndex += 3;
            }

            // Chuyển đổi mảng ký tự thành mảng byte
            byte byteData[bytesRead];
            charArrayToByteArray(buffer, byteData, bytesRead);

            // Gửi dữ liệu tới MQTT
            client.beginPublish("orc_rtk", bytesRead, false);
            client.write(byteData, bytesRead);
            client.endPublish();

            // Gửi thông tin trạng thái qua MQTT
            client.publish("orc_stt_send", status);

            // Hiển thị dữ liệu qua Serial monitor
            Serial.print("Dữ liệu: ");
            Serial.write(byteData, bytesRead);
            Serial.println();
            Serial.print("Hex: ");
            Serial.println(hexData);
            Serial.println("-------------");
            Serial.print("Số byte đã đọc: ");
            Serial.println(bytesRead);
        }
    }
}

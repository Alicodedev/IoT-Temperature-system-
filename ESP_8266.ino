
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char* ssid = "xx";
const char* password = "xx";
const char* mqtt_server = "xx";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WIFI connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, 1883);
}


void setup() {
  Serial.begin(9600);
  Serial.flush(); // Clear any garbage data
  setup_wifi();
}

char* format_serial(char* data) {
    StaticJsonDocument<100> doc;
    char output[100];
    
    // Convert string to float instead of int for better precision
    float sensorValue = atof(data);
    
    // Validate sensor range (adjust range for temperature values if needed)
    if (sensorValue >= 0 && sensorValue <= 1024) {
        // Store as number, not string
        doc["t"] = sensorValue;
        
        serializeJson(doc, output);
        
        // Publish to MQTT topic with retained flag for persistence
        client.publish("/home/sensors", output, true);
        Serial.println("Sent");
        return strdup(output);
    }
    return nullptr;
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    static String serialBuffer = "";
    static unsigned long lastCharTime = 0;
    
    while (Serial.available() > 0) {
        char inChar = (char)Serial.read();
        lastCharTime = millis();
        
        if (inChar == '\n' || inChar == '\r') {
            if (serialBuffer.length() > 0) {
                // Convert to char array for format_serial function
                float tempval = serialBuffer.toFloat() / 10.0;
                
                // Round to one decimal place
                tempval = round(tempval * 10) / 10;
                
                // Convert float to char array with one decimal place
                char tempBuffer[10];
                dtostrf(tempval, 4, 1, tempBuffer);  // Format as "24.5"
                
                char* result = format_serial(tempBuffer);
                if (result != nullptr) {
                    Serial.print("Received and processed: ");
                    Serial.println(result);
                    free(result);
                }
                serialBuffer = ""; // Clear buffer for next reading
            }
        } else if (isDigit(inChar) || inChar == '.') {
            if (serialBuffer.length() < 9) { // Prevent buffer overflow
                serialBuffer += inChar;
            }
        }
    }
    
    // Clear buffer if no new characters received for too long
    if (serialBuffer.length() > 0 && (millis() - lastCharTime > 100)) {
        serialBuffer = "";
    }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("WIFI status: Connected");
    } else {
      Serial.print("WIFI status: Connection Failed:");
      Serial.print(client.state());
      Serial.println(" reconnecting in 5 sec");
      delay(5000);
    }
  }
}
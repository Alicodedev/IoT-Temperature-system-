#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


// Defined WiFi credentials
const char* ssid = "xx"; // ssid of your network
const char* password = "xx"; // pasword of the network
const char* mqtt_server = "xx"; // Mqtt broker ip

WiFiClient espClient;//
PubSubClient client(espClient); // MQTT client WiFi connection

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { // start the WiFi connection
    delay(500);
    Serial.print("."); // indicator wifi is attempting to connect .....
  }
  // Sucessful connection message
  Serial.println("");
  Serial.print("WIFI connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, 1883); // Mqtt server on port 1883
}



void setup() { // intalize serial communication 
  Serial.begin(9600); // setting a baud rate to match sensor system 
  Serial.flush(); // Clear any garbage data
  setup_wifi(); // connect to wifi
}

char* format_serial(char* data) { // format and publish sensor data to mqtt
    StaticJsonDocument<100> doc; // creating json document 
    char output[100]; // buffer for formatted output

    // Convert string to float instead of int for better precision
    float sensorValue = atof(data);

    // Validate sensor range (adjust range for temperature values if needed)
    if (sensorValue >= 0 && sensorValue <= 1024) {
        // Stores value as number in JSON document with key "t"
        doc["t"] = sensorValue;

        serializeJson(doc, output); // serialize json to character aray

        // Publish to MQTT topic 
        client.publish("/plant/sensors", output, true);
        Serial.println("Sent"); 
        return strdup(output); // returns copy of the output string
    }
    return nullptr;
}

void loop() {
    if (!client.connected()) { // check mqtt connection 
        reconnect(); // reconnect if necssary
    }
    client.loop(); // process Mqtt message and maintain connection

    static String serialBuffer = ""; // Buffer to collect incoming characters
    static unsigned long lastCharTime = 0; // timestamp of last received character

    while (Serial.available() > 0) {
        char inChar = (char)Serial.read(); // read single character
        lastCharTime = millis();//update timestamp

        if (inChar == '\n' || inChar == '\r') {// check for line termintors
            if (serialBuffer.length() > 0) {
                // Convert string to float and dive by 10
                // ex. "245" becomes 24.5 degrees
                float tempval = serialBuffer.toFloat() / 10.0;

                // Round to one decimal place
                tempval = round(tempval * 10) / 10;

                // Convert float to char array with one decimal place
                char tempBuffer[10];
                dtostrf(tempval, 4, 1, tempBuffer);  // Format as "24.5"

                char* result = format_serial(tempBuffer);// data sent to me formatted adn published
                if (result != nullptr) {
                    Serial.print("Received and processed: ");
                    Serial.println(result);
                    free(result);// free memory
                }
                serialBuffer = ""; // Clear buffer for next reading
            }
        } else if (isDigit(inChar) || inChar == '.') { // allow only digits and decimals points into buffer
            if (serialBuffer.length() < 9) { // Prevent buffer overflow
                serialBuffer += inChar;// adds character to buffer
            }
        }
    }

    // Clear buffer if no new characters received for too long
    if (serialBuffer.length() > 0 && (millis() - lastCharTime > 100)) {
        serialBuffer = ""; // empty buffer
    }
}

void reconnect() { // Mqtt broker reconnect
  while (!client.connected()) {// attempting to reconnect
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX); // create random client IDS to avoid collisions

    if (client.connect(clientId.c_str())) { // Attempt to connec to broker
      Serial.println("WIFI status: Connected");
    } else {
      Serial.print("WIFI status: Connection Failed:");// connection failed status 
      Serial.print(client.state());
      Serial.println(" reconnecting in 5 sec");
      delay(5000);
    }
  }
}

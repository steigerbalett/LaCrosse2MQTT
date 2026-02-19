// fhem_connector.cpp
#include "fhem_connector.h"
#include <Arduino.h>

void FHEMConnector::init() {
    if (config.fhem_mode) {
        Serial.println("=== FHEM LaCrosseGateway Mode ACTIVE ===");
        sendVersionInfo();
    }
}

void FHEMConnector::sendVersionInfo() {
    if (!config.fhem_mode) return;
    Serial.println("OK LaCrosse2MQTT v" LACROSSE2MQTT_VERSION);
    Serial.print("Freq: "); Serial.println(freq);
    Serial.print("Rate: "); Serial.println(get_current_datarate());
    Serial.println("Ready");
}

void FHEMConnector::sendSensorData(const String& line) {
    if (!config.fhem_mode) return;
    Serial.println(line);
}

bool FHEMConnector::isEnabled() {
    return config.fhem_mode;
}

void FHEMConnector::handleSerialCommand() {
    static String command_buffer = "";
    
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (command_buffer.length() > 0) {
                command_buffer.trim();
                Serial.println("FHEM CMD: " + command_buffer);
                
                if (command_buffer == "version") {
                    sendVersionInfo();
                } else if (command_buffer.startsWith("rate ")) {
                    // Rate setzen (falls gewünscht)
                    Serial.println("OK");
                } else {
                    Serial.println("ERROR unknown command");
                }
                command_buffer = "";
            }
        } else {
            command_buffer += c;
        }
    }
}
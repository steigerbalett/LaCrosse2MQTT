// fhem_connector.cpp
#include "fhem_connector.h"
#include <Arduino.h>

// Statische Membervariablen
WiFiServer* FHEMConnector::tcpServer = nullptr;
WiFiClient FHEMConnector::clients[MAX_FHEM_CLIENTS];
bool FHEMConnector::clientConnected[MAX_FHEM_CLIENTS] = {false};
unsigned long FHEMConnector::lastClientCheck = 0;

void FHEMConnector::init() {
    if (config.fhem_mode) {
        Serial.println("=== FHEM LaCrosseGateway Mode ACTIVE ===");
        sendVersionInfo();
        
        // TCP-Server initialisieren
        initTCPServer();
    }
}

void FHEMConnector::initTCPServer() {
    if (!config.fhem_mode) return;
    
    if (tcpServer != nullptr) {
        delete tcpServer;
    }
    
    tcpServer = new WiFiServer(FHEM_TCP_PORT);
    tcpServer->begin();
    tcpServer->setNoDelay(true);
    
    Serial.printf("FHEM TCP Server started on port %d\n", FHEM_TCP_PORT);
    Serial.printf("Connect FHEM with: define myLGW LaCrosseGateway %s:%d\n", 
                  WiFi.localIP().toString().c_str(), FHEM_TCP_PORT);
    
    // Initialisiere Client-Array
    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        clientConnected[i] = false;
    }
}

void FHEMConnector::handleTCPClients() {
    if (!config.fhem_mode || tcpServer == nullptr) return;
    
    unsigned long now = millis();
    
    // Prüfe auf neue Clients (alle 100ms)
    if (now - lastClientCheck > 100) {
        lastClientCheck = now;
        WiFiClient newClient = tcpServer->accept();
        
        if (newClient) {
            // Finde einen freien Slot
            bool slotFound = false;
            for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
                if (!clientConnected[i] || !clients[i].connected()) {
                    if (clientConnected[i]) {
                        clients[i].stop();
                    }
                    clients[i] = newClient;
                    clientConnected[i] = true;
                    slotFound = true;
                    
                    Serial.printf("FHEM Client #%d connected from %s\n", 
                                  i, clients[i].remoteIP().toString().c_str());
                    
                    // Sende Versions-Info an neuen Client
                    sendVersionToClient(&clients[i]);
                    break;
                }
            }
            
            if (!slotFound) {
                Serial.println("FHEM: Max clients reached, rejecting connection");
                newClient.stop();
            }
        }
    }
    
    // Verarbeite Daten von bestehenden Clients
    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        if (clientConnected[i]) {
            // Prüfe ob Client noch verbunden ist
            if (!clients[i].connected()) {
                Serial.printf("FHEM Client #%d disconnected\n", i);
                clients[i].stop();
                clientConnected[i] = false;
                continue;
            }
            while (clients[i].available()) {
                String command = clients[i].readStringUntil('\n');
                command.trim();
                if (command.length() > 0) {
                    handleClientCommand(&clients[i], command);
                }
            }
        }
    }
}

void FHEMConnector::sendVersionInfo() {
    if (!config.fhem_mode) return;
    
    String versionMsg = "OK LaCrosse2MQTT v" + String(LACROSSE2MQTT_VERSION) + "\n";
    String freqMsg = "Freq: " + String(freq) + "\n";
    String rateMsg = "Rate: " + String(get_current_datarate()) + "\n";
    String readyMsg = "Ready\n";
    
    // An Serial senden
    Serial.print(versionMsg);
    Serial.print(freqMsg);
    Serial.print(rateMsg);
    Serial.print(readyMsg);
    
    // An alle TCP-Clients senden
    sendToClients(versionMsg + freqMsg + rateMsg + readyMsg);
}

void FHEMConnector::sendVersionToClient(WiFiClient* client) {
    if (!config.fhem_mode || client == nullptr || !client->connected()) return;
    
    client->printf("OK LaCrosse2MQTT v%s\n", LACROSSE2MQTT_VERSION);
    client->printf("Freq: %d\n", freq);
    client->printf("Rate: %d\n", get_current_datarate());
    client->println("Ready");
    client->clear();
}

void FHEMConnector::sendSensorData(const String& line) {
    if (!config.fhem_mode) return;
    
    // An Serial senden
    Serial.println(line);
    
    // An alle verbundenen TCP-Clients senden
    sendToClients(line + "\n");
}

void FHEMConnector::sendToClients(const String& data) {
    if (!config.fhem_mode || tcpServer == nullptr) return;
    
    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        if (clientConnected[i] && clients[i].connected()) {
            clients[i].print(data);
            clients[i].clear();  // Sofort senden und Buffer leeren
        }
    }
}

void FHEMConnector::handleClientCommand(WiFiClient* client, String command) {
    if (client == nullptr || !client->connected()) return;
    
    Serial.printf("FHEM TCP CMD from %s: %s\n", 
                  client->remoteIP().toString().c_str(), 
                  command.c_str());
    
    command.trim();
    
    if (command == "version" || command == "V") {
        sendVersionToClient(client);
    } 
    else if (command.startsWith("rate ")) {
        // Rate-Änderung (optional implementierbar)
        client->println("OK");
        client->clear();
    }
    else if (command.startsWith("freq ")) {
        // Frequenz-Änderung (optional implementierbar)
        client->println("OK");
        client->clear();
    }
    else if (command == "help" || command == "?") {
        client->println("Available commands:");
        client->println("  version - Show version info");
        client->println("  rate <value> - Set data rate");
        client->println("  freq <value> - Set frequency");
        client->println("OK");
        client->clear();
    }
    else {
        client->println("ERROR: Unknown command");
        client->clear();
    }
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
                Serial.println("FHEM Serial CMD: " + command_buffer);
                
                if (command_buffer == "version") {
                    sendVersionInfo();
                } else if (command_buffer.startsWith("rate ")) {
                    Serial.println("OK");
                } else {
                    Serial.println("ERROR: Unknown command");
                }
                command_buffer = "";
            }
        } else {
            command_buffer += c;
        }
    }
}

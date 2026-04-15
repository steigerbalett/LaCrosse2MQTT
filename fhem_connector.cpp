#include "fhem_connector.h"
#include <Arduino.h>

extern int freq;
extern int get_current_datarate();

// Statische Member-Definitionen
WiFiServer*   FHEMConnector::tcpServer = nullptr;
WiFiClient    FHEMConnector::clients[MAX_FHEM_CLIENTS];
bool          FHEMConnector::clientConnected[MAX_FHEM_CLIENTS] = {false};
unsigned long FHEMConnector::clientConnectedSince[MAX_FHEM_CLIENTS] = {0};
bool          FHEMConnector::jeelinkMode = false;

// =============================================
void FHEMConnector::init() {
    if (config.fhem_mode) {
        jeelinkMode = config.fhem_format;
        Serial.printf("=== FHEM Mode ACTIVE: %s ===\n", getCurrentFormatName().c_str());
        initTCPServer();
    }
}

// =============================================
void FHEMConnector::initTCPServer() {
    if (!config.fhem_mode) return;

    if (tcpServer != nullptr) {
        tcpServer->end();
        delete tcpServer;
        tcpServer = nullptr;
    }

    tcpServer = new WiFiServer(FHEM_TCP_PORT);
    tcpServer->begin();
    tcpServer->setNoDelay(true);

    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        clientConnected[i] = false;
        clientConnectedSince[i] = 0;
    }

    Serial.printf("[FHEM] TCP Server gestartet auf Port %d\n", FHEM_TCP_PORT);
    Serial.printf("[FHEM] FHEM define: define myLGW %s %s:%d\n",
                  jeelinkMode ? "JeeLink" : "LaCrosseGateway",
                  WiFi.localIP().toString().c_str(), FHEM_TCP_PORT);
}

// =============================================
void FHEMConnector::handleTCPClients() {
    if (!config.fhem_mode || tcpServer == nullptr) return;

    WiFiClient newClient = tcpServer->accept();
    if (newClient && newClient.connected()) {
        bool slotFound = false;
        for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
            if (!clientConnected[i] || !clients[i].connected()) {
                if (clientConnected[i]) clients[i].stop();
                clients[i] = newClient;
                clientConnected[i] = true;
                clientConnectedSince[i] = millis();
                slotFound = true;
                Serial.printf("[FHEM] Client #%d verbunden: %s\n",
                              i, clients[i].remoteIP().toString().c_str());
                sendLaCrosseGatewayHandshake(&clients[i]);
                break;
            }
        }
        if (!slotFound) {
            Serial.println("[FHEM] Max Clients erreicht, Verbindung abgelehnt");
            newClient.stop();
        }
    }

    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        if (!clientConnected[i]) continue;
        if (!clients[i].connected()) {
            Serial.printf("[FHEM] Client #%d getrennt\n", i);
            clients[i].stop();
            clientConnected[i] = false;
            clientConnectedSince[i] = 0;
            continue;
        }
        if (clients[i].available()) {
            String cmd = clients[i].readStringUntil('\n');
            cmd.trim();
            if (cmd.length() > 0) handleClientCommand(&clients[i], cmd);
        }
    }
}

// =============================================
void FHEMConnector::sendLaCrosseGatewayHandshake(WiFiClient* client) {
    if (!client || !client->connected()) return;
    if (jeelinkMode) {
        // JeeLink Handshake
        client->println("[JeeLink v3c]");
        client->println("Available commands:");
        client->println("l -> list configuration");
        client->println("v -> show version");
        client->println("Ready");
    } else {
        // LaCrosseGateway Handshake
        client->println("[LaCrosseGateway32 V2026]");
        client->print("Freq: ");  client->println(freq);
        client->print("Rate: ");  client->println(get_current_datarate());
        client->println("Ready");
    }
    Serial.printf("[FHEM] Handshake gesendet (%s)\n", getCurrentFormatName().c_str());
}

// =============================================
void FHEMConnector::handleClientCommand(WiFiClient* client, String command) {
    command.trim();
    String lower = command;
    lower.toLowerCase();
    Serial.printf("[FHEM] CMD: '%s'\n", command.c_str());

    if (lower == "v" || lower == "version") {
        sendLaCrosseGatewayHandshake(client);
    } else if (lower.startsWith("rate ") || lower.startsWith("freq ")) {
        client->println("OK");
    } else {
        client->println("ERROR unknown command");
    }
}

// =============================================
void FHEMConnector::setFormat(bool jeelink) {
    jeelinkMode = jeelink;
    Serial.printf("[FHEM] Format gewechselt: %s\n", getCurrentFormatName().c_str());
}

bool FHEMConnector::isJeeLinkFormat() {
    return jeelinkMode;
}

String FHEMConnector::getCurrentFormatName() {
    return jeelinkMode ? "JeeLink (OK 9 ...)" : "LaCrosseGateway (KVP/LC ASCII)";
}

// =============================================
// Haupt-Sendefunktion: fertig formatierte Zeile an alle Clients
// Aufgerufen aus lacrosse.cpp / main.cpp mit dem passenden Format-String
void FHEMConnector::sendSensorData(const String& line) {
    if (!config.fhem_mode) return;

    Serial.printf("%s", line.c_str());

    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        if (clientConnected[i] && clients[i].connected()) {
            clients[i].print(line);
        }
    }
}

// =============================================
bool FHEMConnector::isEnabled() {
    return config.fhem_mode;
}

int FHEMConnector::getActiveClientCount() {
    int count = 0;
    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        if (clientConnected[i] && clients[i].connected()) count++;
    }
    return count;
}

String FHEMConnector::getClientStatusHTML() {
    String html = "";
    int count = 0;
    for (int i = 0; i < MAX_FHEM_CLIENTS; i++) {
        if (clientConnected[i] && clients[i].connected()) {
            unsigned long secs = (millis() - clientConnectedSince[i]) / 1000;
            html += "<span class='status-badge status-ok'>✓ ";
            html += clients[i].remoteIP().toString();
            html += " (" + String(secs) + "s)</span> ";
            count++;
        }
    }
    if (count == 0)
        html = "<span class='status-badge status-error'>Keine Verbindung</span>";
    return html;
}

// =============================================
void FHEMConnector::handleSerialCommand() {
    static String buffer = "";
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            buffer.trim();
            if (buffer == "version") sendVersionInfo();
            buffer = "";
        } else {
            buffer += c;
        }
    }
}

void FHEMConnector::sendVersionInfo() {
    Serial.println("[LaCrosseITPlusReader V2026]");
    Serial.printf("Freq: %d\n", freq);
    Serial.printf("Rate: %d\n", get_current_datarate());
    Serial.println("Ready");
}
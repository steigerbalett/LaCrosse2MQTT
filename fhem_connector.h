// fhem_connector.h
#ifndef FHEM_CONNECTOR_H
#define FHEM_CONNECTOR_H

#include <Arduino.h>
#include <WiFi.h>
#include "globals.h"

#define FHEM_TCP_PORT 81
#define MAX_FHEM_CLIENTS 4

class FHEMConnector {
public:
    static void init();
    static void sendVersionInfo();
    static void sendSensorData(const String& line);
    static bool isEnabled();
    static void handleSerialCommand();
    
    // Neue TCP-Server Funktionen
    static void initTCPServer();
    static void handleTCPClients();
    static void sendToClients(const String& data);
    static void sendVersionToClient(WiFiClient* client);
    static void handleClientCommand(WiFiClient* client, String command);
    
private:
    static WiFiServer* tcpServer;
    static WiFiClient clients[MAX_FHEM_CLIENTS];
    static bool clientConnected[MAX_FHEM_CLIENTS];
    static unsigned long lastClientCheck;
};

#endif
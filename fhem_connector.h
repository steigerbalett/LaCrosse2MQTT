#ifndef FHEM_CONNECTOR_H
#define FHEM_CONNECTOR_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "globals.h"

#define FHEM_TCP_PORT 81
#define MAX_FHEM_CLIENTS 4

class FHEMConnector {
private:
    static WiFiServer* tcpServer;
    static WiFiClient clients[MAX_FHEM_CLIENTS];
    static bool clientConnected[MAX_FHEM_CLIENTS];
    static unsigned long clientConnectedSince[MAX_FHEM_CLIENTS];
    static bool jeelinkMode;

    static void sendLaCrosseGatewayHandshake(WiFiClient* client);
    static void handleClientCommand(WiFiClient* client, String command);

public:
    static void init();
    static void initTCPServer();
    static void handleTCPClients();

    // Format-Wechsel (aus webfrontend/setup aufrufbar)
    static void setFormat(bool jeelink);
    static bool isJeeLinkFormat();
    static String getCurrentFormatName();

    // Haupt-Sendefunktion: String-Zeile an alle Clients
    static void sendSensorData(const String& line);

    // Status
    static bool isEnabled();
    static int getActiveClientCount();
    static String getClientStatusHTML();

    // Serial-Kommandos
    static void handleSerialCommand();
    static void sendVersionInfo();
};

#endif

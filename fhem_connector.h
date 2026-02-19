// fhem_connector.h
#ifndef FHEM_CONNECTOR_H
#define FHEM_CONNECTOR_H

#include "globals.h"  // config.fhem_mode

class FHEMConnector {
public:
    static void init();
    static bool isEnabled();
    static void sendSensorData(const String& line);
    static void handleSerialCommand();
    static void sendVersionInfo();
};

#endif

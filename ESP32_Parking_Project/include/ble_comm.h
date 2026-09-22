#ifndef BLE_COMM_H
#define BLE_COMM_H

#include <Arduino.h>

void bleInit();

void bleUpdate();

bool bleIsConnected();

void bleSend(const String &message);

void bleSendVehicleDetected(uint16_t distance);

void bleSendFingerprintOK(
    int id,
    int confidence
);

void bleSendFingerprintFail();

void bleSendBarrierOpen();

void bleSendBarrierClose();

void bleSendVehicleExit();

#endif
#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <Arduino.h>

// =====================================================
// HỆ THỐNG
// =====================================================

void fingerprintInit();

void fingerprintUpdate();

bool fingerprintHasResult();

int fingerprintGetID();

void fingerprintClearResult();

// =====================================================
// TCH
// =====================================================

bool fingerprintTouchAvailable();

void fingerprintTouchClear();

// =====================================================
// SETUP / QUẢN LÝ VÂN TAY
// =====================================================

bool fingerprintEnroll(uint8_t id);

bool fingerprintSearch();

bool fingerprintDelete(uint8_t id);

bool fingerprintEmptyDatabase();

uint16_t fingerprintShowCount();

#endif
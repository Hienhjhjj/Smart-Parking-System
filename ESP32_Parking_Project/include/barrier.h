#ifndef BARRIER_H
#define BARRIER_H

#include <Arduino.h>

void barrierInit();

void barrierOpen();
void barrierClose();

bool barrierIsOpen();

#endif
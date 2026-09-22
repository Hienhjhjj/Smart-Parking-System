#ifndef BLECLIENT_H
#define BLECLIENT_H

#include <QObject>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>

#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyDescriptor>


class BLEClient : public QObject
{
    Q_OBJECT

public:

    explicit BLEClient(QObject *parent = nullptr);

    void startScan();

    void sendCommand(
        const QString &command
        );

    void startFingerprintEnroll(
        int id
        );


signals:

    // ========================================================
    // BLE
    // ========================================================

    void bleConnected();
    void bleDisconnected();

    void logMessage(
        const QString &message
        );

    void statusReceived(
        const QString &status
        );


    // ========================================================
    // PARKING
    // ========================================================

    void vehicleDetected(
        int distance
        );

    void fingerprintOK(
        int id,
        int confidence
        );

    void fingerprintFail();

    void barrierOpened();

    void barrierClosed();

    void vehicleExit();


    // ========================================================
    // FINGERPRINT ENROLL
    // ========================================================

    void fingerprintEnrollSuccess(
        int id
        );

    void fingerprintEnrollFailed(
        const QString &reason
        );


private slots:

    void deviceDiscovered(
        const QBluetoothDeviceInfo &device
        );

    void scanFinished();

    void controllerConnected();

    void controllerDisconnected();

    void serviceDiscovered(
        const QBluetoothUuid &uuid
        );

    void serviceDiscoveryFinished();

    void serviceStateChanged(
        QLowEnergyService::ServiceState state
        );

    void characteristicChanged(
        const QLowEnergyCharacteristic &characteristic,
        const QByteArray &value
        );

    void controllerError(
        QLowEnergyController::Error error
        );


private:

    // ========================================================
    // BLE OBJECT
    // ========================================================

    QBluetoothDeviceDiscoveryAgent *discoveryAgent;

    QLowEnergyController *controller;

    QLowEnergyService *service;


    // ========================================================
    // CHARACTERISTICS
    // ========================================================

    QLowEnergyCharacteristic statusCharacteristic;

    QLowEnergyCharacteristic commandCharacteristic;


    // ========================================================
    // DEVICE
    // ========================================================

    bool foundDevice;


    // ========================================================
    // BLE NAME
    // ========================================================

    const QString targetName =
        "ParkingSystem-ESP32";


    // ========================================================
    // UUID
    //
    // Service:
    // 12345678-1234-1234-1234-123456789000
    //
    // Status:
    // 12345678-1234-1234-1234-123456789001
    //
    // Command:
    // 12345678-1234-1234-1234-123456789002
    // ========================================================

    const QBluetoothUuid serviceUuid =
        QBluetoothUuid(
            QString(
                "12345678-1234-1234-1234-123456789000"
                )
            );

    const QBluetoothUuid statusUuid =
        QBluetoothUuid(
            QString(
                "12345678-1234-1234-1234-123456789001"
                )
            );

    const QBluetoothUuid commandUuid =
        QBluetoothUuid(
            QString(
                "12345678-1234-1234-1234-123456789002"
                )
            );
};

#endif // BLECLIENT_H

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

#define PACKET_LENGTH 125


#include <cstdint>

struct Packet
{
    uint32_t PacketCounter;                  // 4 bytes
    uint32_t FrameTime;                      // 4 bytes

    uint16_t avionicBatteryVoltage;          // 2
    uint16_t avionicBatteryCurrent;          // 2
    uint16_t generatorVoltage;               // 2
    uint16_t generatorCurrent;               // 2
    uint16_t groundSupplyVoltage;            // 2
    uint16_t groundSupplyCurrent;            // 2
    uint16_t vBusVoltage;                    // 2
    uint16_t vBusCurrent;                    // 2
    uint16_t b1_24VoltVoltage;                // 2
    uint16_t b1_24VoltCurrent;                // 2
    uint16_t b1A_24VoltVoltage;               // 2
    uint16_t b1A_24VoltCurrent;               // 2
    uint16_t b2v2_8VoltVoltage;                // 2
    uint16_t b2v2_8VoltCurrent;                // 2
    uint16_t b3v2_8VoltVoltage;                // 2
    uint16_t b3v2_8VoltCurrent;                // 2
    uint16_t b3av2_8VoltVoltage;                // 2
    uint16_t b3av2_8VoltCurrent;                // 2
    uint16_t b4_12VoltVoltage;                // 2
    uint16_t b4_12VoltCurrent;                // 2
    uint16_t b4a_12VoltVoltage;                // 2
    uint16_t b4a_12VoltCurrent;                // 2
    uint16_t b5_24VoltVoltage;                // 2
    uint16_t b5_24VoltCurrent;                // 2
    uint16_t b5a_24VoltVoltage;                // 2
    uint16_t b5a_24VoltCurrent;                // 2
    uint16_t b5b_24VoltVoltage;                // 2
    uint16_t b5b_24VoltCurrent;                // 2
    uint16_t b6_5VoltVoltage;                // 2
    uint16_t b6_5VoltCurrent;                // 2

    uint16_t aIn1Voltage;                    // 2
    uint16_t aIn2Voltage;                    // 2
    uint16_t aIn3Voltage;                    // 2
    uint16_t tempSensorInternal;             // 2

    uint16_t debugVoltage1_BST;              // 2
    uint16_t debugCurrent1_BST;              // 2
    uint16_t debugVoltage2_AVIN;             // 2
    uint16_t debugVoltage3_SP1;              // 2
    uint16_t debugVoltage4_SP2;              // 2
    uint16_t debugVoltage5_SP3;              // 2
    uint16_t debugVoltage6_VIN_BP;           // 2

    bool dOutStatus1;
    bool dOutStatus2;
    bool pwmStatus1;
    bool pwmStatus2;
    bool pyroArmStatus;
    bool pyroAgnStatus;
    bool batSwitchStatus;
    bool spareFlag;

    bool b1a_24V_status;
    bool b3a_8V2_status;
    bool b4a_12V_status;
    bool b5a_24V_status;
    bool b5b_24V_status;
    bool spareFlag2;
    bool spareFlag3;
    bool spareFlag4;



    uint16_t engineRPM;                      // 2
    uint16_t commandCounter;                 // 2
    uint8_t  lastAppliedCommand;             // 1

    uint16_t packetErrorCounter;             // 2
    uint16_t headerErrorCounter;             // 2
    uint16_t crcErrorCounter;                // 2
    uint16_t serialTimeoutErrorCounter;      // 2
    uint16_t frameLengthMismatchErrorCounter;// 2
};

enum CommandID{
    b1A_24V = 0x2c,
    b3A_8V2 = 0x4c,
    b4A_12V = 0x6C,
    b5A_24V = 0x8c,
    b5b_24V = 0xAc,
    ARM = 0xCC,
    IGN= 0xFC,
    Battery_Switch = 0x2E,
    DOUT_1 = 0x4E,
    DOUT_2 = 0x6E,
    PWM_1 = 0x8E,
    PWM_2 = 0xAE,

};

enum CommandStatus{
    On = 0xAA,
    Off = 0x55
};

struct Command {
    CommandID id;          // which system this command targets
    CommandStatus status;  // On or Off

    Command(CommandID cmdId, CommandStatus cmdStatus)
        : id(cmdId), status(cmdStatus) {}
};


class SerialPort : public QObject
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = nullptr);

signals:
    void connected();
    void disconnected();
    void dataReceived(const Packet &data);
    void WrittenToPort(const QString &packet);

public slots:
    void connectToSerialPort(const QString &portName, int baudRate);
    void disconnectFromSerialPort();
    void sendCommand(const Command &cmd);
    void clearReadBuffer();
    QStringList availablePorts();

private slots:
    void onDataRx();
private:
    QSerialPort m_serialPort;
    QByteArray m_readBuffer;
    const uint8_t startSeq0 = 0xEA;
    const uint8_t startSeq1 = 0x9B;

    QByteArray buffer;
    uint8_t char_prev = 0;
    bool headerFound = false;
    bool footerFirstfound = false;
    bool footerSecondFound = false;
    int s_counter = 0;
    uint16_t cmdCount = 0;

    uint16_t calculateBufferCRC16();
    Packet DeSerializePacket();


};

#endif // SERIALPORT_H

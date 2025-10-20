#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

#define PACKET_LENGTH 68


#include <cstdint>

struct Packet {
    uint32_t PacketCounter;
    uint32_t FrameTime;

    uint16_t BatteryVoltage;
    uint16_t BatteryCurrent;
    uint16_t InasVoltage;
    uint16_t InasCurrent;
    uint16_t DataLinkVoltage;
    uint16_t DataLinkCurrent;
    uint16_t SeekerVoltage;
    uint16_t SeekerCurrent;
    uint16_t MagnoSoleVoltage;
    uint16_t MagnoSoleCurrent;
    uint16_t TwelveVoltVoltage;
    uint16_t TwelveVoltCurrent;
    uint16_t ActuatorOneVoltage;
    uint16_t ActuatorOneCurrent;
    uint16_t ActuatorTwoVoltage;
    uint16_t ActuatorTwoCurrent;
    uint16_t ActuatorThreeVoltage;
    uint16_t ActuatorThreeCurrent;
    uint16_t fiveVoltVoltage;
    uint16_t fiveVoltCurrent;
    uint16_t spareVolt1;
    uint16_t spareVolt2;

    int16_t internalTemp;

    // Status flags from 1 byte
    bool DataLinkStatus;
    bool SeekerStatus;
    bool TwelveVoltStatus;

    uint8_t commandCounter;
    uint8_t lastCommand;
    uint8_t pktErrorCounter;
    uint8_t headerErrCounter;
    uint8_t crcErrorCounter;
    uint8_t serialTimeoutCounter;
    uint8_t frameLengthMismatchErrCounter;
};

enum CommandID{
    DATA_LINK_CMD_ID = 0x2c,
    SEEKER_CMD_ID = 0x4c,
    TWELVE_V_CMD_ID = 0x6C

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

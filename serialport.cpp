#include "serialport.h"

SerialPort::SerialPort(QObject *parent)
    : QObject{parent}
{
    buffer.resize(PACKET_LENGTH);
    connect(&m_serialPort, &QSerialPort::readyRead, this, &SerialPort::onDataRx);
}


void SerialPort::onDataRx()
{
    //qDebug() << "data Read";
    QByteArray dataRead = m_serialPort.readAll();
    int length = dataRead.length();
    //qDebug() << "read length: " << length;

    for(int i = 0; i < length; i++){
        if(headerFound){

            //qDebug() << "appending to buffer: " <<static_cast<uchar>(dataRead.at(i)) << " at: " << s_counter;
            buffer[s_counter] = dataRead.at(i);
            //qDebug() << "appending complete";

            s_counter++;

            if (s_counter == PACKET_LENGTH - 2) {
                // Compute CRC of the packet (excluding last 2 bytes)
                uint16_t computedCrc = calculateBufferCRC16();

                // Extract last two bytes from packet
                uint8_t crcLow = static_cast<uint8_t>(dataRead.at(i - 1));
                uint8_t crcHigh  = static_cast<uint8_t>(dataRead.at(i));
                uint16_t receivedCrc = (static_cast<uint16_t>(crcHigh) << 8) | crcLow;
                  qDebug().nospace()
                      << "Received CRC: 0x"
                    << QString("%1").arg(receivedCrc, 4, 16, QLatin1Char('0')).toUpper();

                qDebug().nospace()
                    << "Calculated CRC: 0x"
                    << QString("%1").arg(computedCrc, 4, 16, QLatin1Char('0')).toUpper();


                if (computedCrc == receivedCrc) {
                    //qDebug() << "CRC valid";
                    Packet pkt = DeSerializePacket();
                    emit dataReceived(pkt);
                    buffer.clear();
                    buffer.resize(PACKET_LENGTH);
                }

                headerFound = false;
                continue;
            }


        }
        else if(static_cast<uchar>(dataRead.at(i)) == startSeq1 && char_prev == startSeq0){
            //qDebug() << "header found at byte: " << i;
            headerFound = true;
            s_counter = 0;
        }
        char_prev = dataRead.at(i);
    }


}

Packet SerialPort::DeSerializePacket()
{
    qDebug() << "Deserializing Packet";
    Packet pkt{};
    QDataStream stream(buffer);
    stream.setByteOrder(QDataStream::LittleEndian); // Assuming little-endian protocol

    // --- Header ---
    stream >> pkt.PacketCounter;
    stream >> pkt.FrameTime;

    // --- Voltages & Currents ---
    stream >> pkt.avionicBatteryVoltage;
    stream >> pkt.avionicBatteryCurrent;
    stream >> pkt.generatorVoltage;
    stream >> pkt.generatorCurrent;
    stream >> pkt.groundSupplyVoltage;
    stream >> pkt.groundSupplyCurrent;
    stream >> pkt.vBusVoltage;
    stream >> pkt.vBusCurrent;
    stream >> pkt.b1_24VoltVoltage;
    stream >> pkt.b1_24VoltCurrent;
    stream >> pkt.b1A_24VoltVoltage;
    stream >> pkt.b1A_24VoltCurrent;
    stream >> pkt.b2v2_8VoltVoltage;
    stream >> pkt.b2v2_8VoltCurrent;
    stream >> pkt.b3v2_8VoltVoltage;
    stream >> pkt.b3v2_8VoltCurrent;
    stream >> pkt.b3av2_8VoltVoltage;
    stream >> pkt.b3av2_8VoltCurrent;
    stream >> pkt.b4_12VoltVoltage;
    stream >> pkt.b4_12VoltCurrent;
    stream >> pkt.b4a_12VoltVoltage;
    stream >> pkt.b4a_12VoltCurrent;
    stream >> pkt.b5_24VoltVoltage;
    stream >> pkt.b5_24VoltCurrent;
    stream >> pkt.b5a_24VoltVoltage;
    stream >> pkt.b5a_24VoltCurrent;
    stream >> pkt.b5b_24VoltVoltage;
    stream >> pkt.b5b_24VoltCurrent;
    stream >> pkt.b6_5VoltVoltage;
    stream >> pkt.b6_5VoltCurrent;

    // --- Analog Inputs & Temp ---
    stream >> pkt.aIn1Voltage;
    stream >> pkt.aIn2Voltage;
    stream >> pkt.aIn3Voltage;
    stream >> pkt.tempSensorInternal;

    // --- Debug readings ---
    stream >> pkt.debugVoltage1_BST;
    stream >> pkt.debugCurrent1_BST;
    stream >> pkt.debugVoltage2_AVIN;
    stream >> pkt.debugVoltage3_SP1;
    stream >> pkt.debugVoltage4_SP2;
    stream >> pkt.debugVoltage5_SP3;
    stream >> pkt.debugVoltage6_VIN_BP;

    // --- Boolean Flags (packed) ---
    // Assuming they are serialized as bytes (8 bits = 8 bools)
    quint8 flags1, flags2;
    stream >> flags1;
    stream >> flags2;

    // First byte (flags1)
    pkt.dOutStatus1     = flags1 & 0x01;
    pkt.dOutStatus2     = flags1 & 0x02;
    pkt.pwmStatus1      = flags1 & 0x04;
    pkt.pwmStatus2      = flags1 & 0x08;
    pkt.pyroArmStatus   = flags1 & 0x10;
    pkt.pyroAgnStatus   = flags1 & 0x20;
    pkt.batSwitchStatus = flags1 & 0x40;
    pkt.spareFlag       = flags1 & 0x80;

    // Second byte (flags2)
    pkt.b1a_24V_status  = flags2 & 0x01;
    pkt.b3a_8V2_status  = flags2 & 0x02;
    pkt.b4a_12V_status  = flags2 & 0x04;
    pkt.b5a_24V_status  = flags2 & 0x08;
    pkt.b5b_24V_status  = flags2 & 0x10;
    pkt.spareFlag2      = flags2 & 0x20;
    pkt.spareFlag3      = flags2 & 0x40;
    pkt.spareFlag4      = flags2 & 0x80;

    // --- Engine & Diagnostics ---

    stream >> pkt.boostTempSensor1;
    stream >> pkt.boostTempSensor2;
    stream >> pkt.engineRPM;
    stream >> pkt.commandCounter;
    stream >> pkt.lastAppliedCommand;

    // --- Error Counters ---
    stream >> pkt.packetErrorCounter;
    stream >> pkt.headerErrorCounter;
    stream >> pkt.crcErrorCounter;
    stream >> pkt.serialTimeoutErrorCounter;
    stream >> pkt.frameLengthMismatchErrorCounter;

    return pkt;
}

void SerialPort::connectToSerialPort(const QString &portName, int baudRate)
{
    qDebug() <<"connecting to serial port";
    m_serialPort.setPortName(portName);
    m_serialPort.setBaudRate(baudRate);
    if (m_serialPort.open(QIODevice::ReadWrite))
    {
        qDebug() << "Connected";
        emit connected();
    }
}

void SerialPort::disconnectFromSerialPort()
{
    m_serialPort.close();
    emit disconnected();
}

void SerialPort::sendCommand(const Command &cmd)
{
    cmdCount++;
    QByteArray buff;
    buff.append(0xEB);
    buff.append(0x90);
    buff.append(static_cast<char>(cmdCount & 0xFF));        // low byte
    buff.append(static_cast<char>((cmdCount >> 8) & 0xFF)); // high byte
    buff.append(cmd.id);
    buff.append(cmd.status);

    uint16_t crc = 0xFFFF;           // initial value
    const uint16_t polynomial = 0x1021;

    for (int i = 0; i < buff.size(); i++) {
        crc ^= (static_cast<uint8_t>(buff.at(i)) << 8);
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    // Append CRC (big endian: high then low)
    buff.append(static_cast<char>(crc & 0xFF));
    buff.append(static_cast<char>((crc >> 8) & 0xFF));

    QString msg = QString("%1")
                      .arg(QString(buff.toHex(' ').toUpper()));
    emit WrittenToPort(msg);


    m_serialPort.write(buff);


}


void SerialPort::clearReadBuffer()
{
    m_readBuffer.clear();
}

QStringList SerialPort::availablePorts()
{
    QStringList ports;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ports.append(info.portName());
    }
    return ports;
}

uint16_t SerialPort::calculateBufferCRC16()
{
    uint16_t crc = 0xFFFF;              // Initial value
    const uint16_t polynomial = 0x1021; // Polynomial used for CRC-16-CCITT

    // Step 1: process EA 9B as the first two bytes
    uint8_t prefix[2] = { 0xEA, 0x9B };
    for (int i = 0; i < 2; i++) {
        crc ^= (static_cast<uint16_t>(prefix[i]) << 8);

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    // Step 2: process the first 64 bytes of buffer
    int length = qMin(buffer.size(), 121);
    for (int i = 0; i < length; i++) {
        crc ^= (static_cast<uint16_t>(static_cast<uint8_t>(buffer[i])) << 8);

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

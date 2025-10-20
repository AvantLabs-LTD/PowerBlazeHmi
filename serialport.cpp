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
                // qDebug().nospace()
                //     << "Received CRC: 0x"
                //     << QString("%1").arg(receivedCrc, 4, 16, QLatin1Char('0')).toUpper();


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

Packet SerialPort::DeSerializePacket(){
        Packet pkt{};
        QDataStream stream(buffer);
        stream.setByteOrder(QDataStream::LittleEndian); // assuming your protocol is little-endian

        stream >> pkt.PacketCounter;
        stream >> pkt.FrameTime;

        stream >> pkt.BatteryVoltage;
        stream >> pkt.BatteryCurrent;
        stream >> pkt.InasVoltage;
        stream >> pkt.InasCurrent;
        stream >> pkt.DataLinkVoltage;
        stream >> pkt.DataLinkCurrent;
        stream >> pkt.SeekerVoltage;
        stream >> pkt.SeekerCurrent;
        stream >> pkt.MagnoSoleVoltage;
        stream >> pkt.MagnoSoleCurrent;
        stream >> pkt.TwelveVoltVoltage;
        stream >> pkt.TwelveVoltCurrent;
        stream >> pkt.ActuatorOneVoltage;
        stream >> pkt.ActuatorOneCurrent;
        stream >> pkt.ActuatorTwoVoltage;
        stream >> pkt.ActuatorTwoCurrent;
        stream >> pkt.ActuatorThreeVoltage;
        stream >> pkt.ActuatorThreeCurrent;
        stream >> pkt.fiveVoltVoltage;
        stream >> pkt.fiveVoltCurrent;

        stream >> pkt.spareVolt1;
        stream >> pkt.spareVolt2;

        stream >> pkt.internalTemp;

        quint8 bitmap;
        stream >> bitmap;
        pkt.DataLinkStatus   = bitmap & 0x01;
        pkt.SeekerStatus     = bitmap & 0x02;
        pkt.TwelveVoltStatus = bitmap & 0x04;

        stream >> pkt.commandCounter;
        stream >> pkt.lastCommand;
        stream >> pkt.pktErrorCounter;
        stream >> pkt.headerErrCounter;
        stream >> pkt.crcErrorCounter;
        stream >> pkt.serialTimeoutCounter;
        stream >> pkt.frameLengthMismatchErrCounter;

        // --- Debug dump ---
        // qDebug() << "PacketCounter:" << pkt.PacketCounter;
        // qDebug() << "FrameTime:" << pkt.FrameTime;
        // qDebug() << "Battery V/I:" << pkt.BatteryVoltage << pkt.BatteryCurrent;
        // qDebug() << "INAS V/I:" << pkt.InasVoltage << pkt.InasCurrent;
        // qDebug() << "DataLink V/I:" << pkt.DataLinkVoltage << pkt.DataLinkCurrent;
        // qDebug() << "Seeker V/I:" << pkt.SeekerVoltage << pkt.SeekerCurrent;
        // qDebug() << "MagnoSole V/I:" << pkt.MagnoSoleVoltage << pkt.MagnoSoleCurrent;
        // qDebug() << "12V Ctrl V/I:" << pkt.TwelveVoltVoltage << pkt.TwelveVoltCurrent;
        // qDebug() << "Actuator1 V/I:" << pkt.ActuatorOneVoltage << pkt.ActuatorOneCurrent;
        // qDebug() << "Actuator2 V/I:" << pkt.ActuatorTwoVoltage << pkt.ActuatorTwoCurrent;
        // qDebug() << "Actuator3 V/I:" << pkt.ActuatorThreeVoltage << pkt.ActuatorThreeCurrent;
        // qDebug() << "5V V/I:" << pkt.fiveVoltVoltage << pkt.fiveVoltCurrent;
        // qDebug() << "Internal Temp:" << pkt.internalTemp;
        // qDebug() << "Statuses -> DataLink:" << pkt.DataLinkStatus
        //          << " Seeker:" << pkt.SeekerStatus
        //          << " 12V:" << pkt.TwelveVoltStatus;
        // qDebug() << "Counters -> Cmd:" << pkt.commandCounter
        //          << " LastCmd:" << pkt.lastCommand
        //          << " PktErr:" << pkt.pktErrorCounter
        //          << " HeaderErr:" << pkt.headerErrCounter
        //          << " CrcErr:" << pkt.crcErrorCounter
        //          << " TimeoutErr:" << pkt.serialTimeoutCounter
        //          << " FrameLenErr:" << pkt.frameLengthMismatchErrCounter;
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
    int length = qMin(buffer.size(), 64);
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

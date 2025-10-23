#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

#include <QGraphicsOpacityEffect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{


    ui->setupUi(this);

    auto applyShadow = [](QWidget *widget) {
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(widget);
        shadow->setBlurRadius(10);
        shadow->setOffset(2, 2);
        shadow->setColor(QColor(0, 0, 0, 160));
        widget->setGraphicsEffect(shadow);
    };

    // Apply to both group boxes
    applyShadow(ui->groupBox_bus_monitor);
    applyShadow(ui->navBarGroup);
    applyShadow(ui->CommBox);
    applyShadow(ui->analogInputBox);
    applyShadow(ui->indicatorsgroupBox);
    applyShadow(ui->gridGroupBox);
    // applyShadow(ui->chargingBox_2);

    QPixmap logo(":/Resources/logo_white_bg.png");
    ui->logoLbl->setPixmap(logo);
    ui->logoLbl->setScaledContents(true);  // Optional: scale to fit the label size

    QIcon icon(":/Resources/app_icon.png");
    this->setWindowIcon(icon);

    this->showMaximized();
    this->setWindowTitle("PowerBlazev1.0.2");
    ui->versionLabel->setText("v1.0.2");


    timeTimer = new QTimer(this);
    timeTimer->setInterval(1000);
    timeTimer->start();

    connect(timeTimer, &QTimer::timeout, this, &MainWindow::onTimeTimerTimeout);


    voltageMap.insert(3, 20);
    voltageMap.insert(7, 28);
    voltageMap.insert(11, 28);
    voltageMap.insert(13, 12);
    voltageMap.insert(9, 12);
    voltageMap.insert(5, 12);
    voltageMap.insert(15, 7.4);
    voltageMap.insert(28,5);
    voltageMap.insert(31,28);


    connect(&m_serialPort, &SerialPort::connected, this, &MainWindow::handleConnected);
    connect(&m_serialPort, &SerialPort::disconnected, this, &MainWindow::handleDisconnected);
    connect(&m_serialPort, &SerialPort::dataReceived, this, &MainWindow::onDataReceived);
    connect(&m_serialPort, &SerialPort::WrittenToPort, this, &MainWindow::onDataSent);
    connect(this, &MainWindow::sendCommand, &m_serialPort, &SerialPort::sendCommand);

    refreshPortList();
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateAll);
    timer->start(100);

    ui->avionicBatteryVolate->display(formatFloat(0));
    ui->avionicBatteryCurrent->display(formatFloat(0));
    ui->GeneratorCurrent->display(formatFloat(0));
    ui->GeneratorVoltage->display(formatFloat(0));
    ui->groundSupplyVoltage->display(formatFloat(0));
    ui->groundSupplyCurrent->display(formatFloat(0));
    ui->vBusVoltage->display(formatFloat(0));
    ui->vBusCurrent->display(formatFloat(0));
    ui->b3_8v2Voltage->display(formatFloat(0));
    ui->b3_8v2Current->display(formatFloat(0));
    ui->b1_24vVoltage->display(formatFloat(0));
    ui->b1_24vCurrent->display(formatFloat(0));
    ui->b1A_24vVoltage->display(formatFloat(0));
    ui->b1A_24vCurrent->display(formatFloat(0));
    ui->b2_8v2Voltage->display(formatFloat(0));
    ui->b2_8v2Current->display(formatFloat(0));
    ui->b3_8v2Voltage->display(formatFloat(0));
    ui->b3_8v2Current->display(formatFloat(0));
    ui->b3a_8v2Voltage->display(formatFloat(0));
    ui->b3a_8v2Current->display(formatFloat(0));
    ui->b4_12vVoltage->display(formatFloat(0));
    ui->b4_12vCurrent->display(formatFloat(0));
    ui->b4a_12vVoltage->display(formatFloat(0));
    ui->b4a_12vCurrent->display(formatFloat(0));
    ui->b5_24vVoltage->display(formatFloat(0));
    ui->b5_24vCurrent->display(formatFloat(0));
    ui->b5a_24vVoltage->display(formatFloat(0));
    ui->b5a_24vCurrent->display(formatFloat(0));
    ui->b5b_24vVoltage->display(formatFloat(0));
    ui->b5b_24vCurrent->display(formatFloat(0));
    ui->b6_5vVoltage->display(formatFloat(0));
    ui->b6_5vCurrent->display(formatFloat(0));
    ui->ain_1Voltage->display(formatFloat(0));
    ui->ain_2Voltage->display(formatFloat(0));
    ui->ain_3Voltage->display(formatFloat(0));
    ui->temperature->display(formatFloat(0));
    ui->debugV1->display(formatFloat(0));
    ui->debugC1->display(formatFloat(0));
    ui->debugV2->display(formatFloat(0));
    ui->debugV3->display(formatFloat(0));
    ui->debugV4->display(formatFloat(0));
    ui->debugV5->display(formatFloat(0));
    ui->debugV6->display(formatFloat(0));

    // ui->debugInfoBtn->setIcon(QIcon(":/Resources/searching.png"));
    // ui->spareVolt1Disp->setVisible(false);
    // ui->spareVolt2Disp->setVisible(false);
    // ui->spVolt1Lbl->setVisible(false);
    // ui->spVolt2Lbl->setVisible(false);

    logFileInit();
}

MainWindow::~MainWindow()
{
    ////qDebug("closing file");
    // Close the file
    file.close();
    cmdLogFile.close();

    delete ui;
}

void MainWindow::refreshPortList()
{
    QStringList ports = m_serialPort.availablePorts();
    ui->portComboBox->clear();
    ui->portComboBox->addItems(ports);
}


void MainWindow::on_connectBtn_clicked()
{

    if(isConnected){
        m_serialPort.disconnectFromSerialPort();
        return;
    }
    QString portName = ui->portComboBox->currentText();
    int baudRate = ui->baudRateComboBox->currentText().toInt();
    m_serialPort.connectToSerialPort(portName, baudRate);
}


void MainWindow::on_refreshBtn_clicked()
{
    refreshPortList();
}

void MainWindow::handleConnected()
{
    //qDebug() << "connected to serial port";
    isConnected = true;
    ui->connectBtn->setText("Disconnect");
}

void MainWindow::handleDisconnected()
{
    //qDebug() << "disconnected from serial port";
    isConnected = false;
    ui->connectBtn->setText("Connect");
}

void MainWindow::onDataReceived(const Packet &data)
{

    dataToShow = data;
    toUpdate = true;
    //qDebug() << "Received data:" << data;
    logDataToCSV();
    // Print the total time spent
    //qint64 elapsedTime = timer.elapsed();
    //qDebug() << "Total time spent in all operations (milliseconds):" << elapsedTime;

}

void MainWindow::onDataSent(const QString &data)
{

    cmdLogOut <<  QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << ", " << data << "\n";
    cmdLogOut.flush();
    cmdLogFile.flush();


    ui->serialDebug->setText(data);
}


void MainWindow::updateAll(){
    if(!toUpdate){
        return;
    }
    ui->pktCounter->display(static_cast<int>(dataToShow.PacketCounter));
    ui->frameTime->display(formatFloat(static_cast<float>(dataToShow.FrameTime) / 1000));

    ui->avionicBatteryVolate->display(formatFloat(static_cast<float>(dataToShow.avionicBatteryVoltage) / 1000));
    ui->avionicBatteryCurrent->display(formatFloat(static_cast<float>(dataToShow.avionicBatteryCurrent) / 1000));
    ui->GeneratorVoltage->display(formatFloat(static_cast<float>(dataToShow.generatorVoltage) / 1000));
    ui->GeneratorCurrent->display(formatFloat(static_cast<float>(dataToShow.generatorCurrent) / 1000));
    ui->groundSupplyVoltage->display(formatFloat(static_cast<float>(dataToShow.groundSupplyVoltage) / 1000));
    ui->groundSupplyCurrent->display(formatFloat(static_cast<float>(dataToShow.groundSupplyCurrent) / 1000));
    ui->vBusVoltage->display(formatFloat(static_cast<float>(dataToShow.vBusVoltage) / 1000));
    ui->vBusCurrent->display(formatFloat(static_cast<float>(dataToShow.vBusCurrent) / 1000));

    ui->b1_24vVoltage->display(formatFloat(static_cast<float>(dataToShow.b1_24VoltVoltage) / 1000));
    ui->b1_24vCurrent->display(formatFloat(static_cast<float>(dataToShow.b1_24VoltCurrent) / 1000));
    ui->b1A_24vVoltage->display(formatFloat(static_cast<float>(dataToShow.b1A_24VoltVoltage) / 1000));
    ui->b1A_24vCurrent->display(formatFloat(static_cast<float>(dataToShow.b1A_24VoltCurrent) / 1000));

    ui->b2_8v2Voltage->display(formatFloat(static_cast<float>(dataToShow.b2v2_8VoltVoltage) / 1000));
    ui->b2_8v2Current->display(formatFloat(static_cast<float>(dataToShow.b2v2_8VoltCurrent) / 1000));

    ui->b3_8v2Voltage->display(formatFloat(static_cast<float>(dataToShow.b3v2_8VoltVoltage) / 1000));
    ui->b3_8v2Current->display(formatFloat(static_cast<float>(dataToShow.b3v2_8VoltCurrent) / 1000));
    ui->b3a_8v2Voltage->display(formatFloat(static_cast<float>(dataToShow.b3av2_8VoltVoltage) / 1000));
    ui->b3a_8v2Current->display(formatFloat(static_cast<float>(dataToShow.b3av2_8VoltCurrent) / 1000));

    ui->b4_12vVoltage->display(formatFloat(static_cast<float>(dataToShow.b4_12VoltVoltage) / 1000));
    ui->b4_12vCurrent->display(formatFloat(static_cast<float>(dataToShow.b4_12VoltCurrent) / 1000));
    ui->b4a_12vVoltage->display(formatFloat(static_cast<float>(dataToShow.b4a_12VoltVoltage) / 1000));
    ui->b4a_12vCurrent->display(formatFloat(static_cast<float>(dataToShow.b4a_12VoltCurrent) / 1000));

    ui->b5_24vVoltage->display(formatFloat(static_cast<float>(dataToShow.b5_24VoltVoltage) / 1000));
    ui->b5_24vCurrent->display(formatFloat(static_cast<float>(dataToShow.b5_24VoltCurrent) / 1000));
    ui->b5a_24vVoltage->display(formatFloat(static_cast<float>(dataToShow.b5a_24VoltVoltage) / 1000));
    ui->b5a_24vCurrent->display(formatFloat(static_cast<float>(dataToShow.b5a_24VoltCurrent) / 1000));
    ui->b5b_24vVoltage->display(formatFloat(static_cast<float>(dataToShow.b5b_24VoltVoltage) / 1000));
    ui->b5b_24vCurrent->display(formatFloat(static_cast<float>(dataToShow.b5b_24VoltCurrent) / 1000));

    ui->b6_5vVoltage->display(formatFloat(static_cast<float>(dataToShow.b6_5VoltVoltage) / 1000));
    ui->b6_5vCurrent->display(formatFloat(static_cast<float>(dataToShow.b6_5VoltCurrent) / 1000));

    // Power all and total
    ui->avionicBatteryPower->display(formatFloat((static_cast<float>(dataToShow.avionicBatteryVoltage) / 1000.0f) * (static_cast<float>(dataToShow.avionicBatteryCurrent) / 1000.0f)));
    ui->GeneratorPower->display(formatFloat((static_cast<float>(dataToShow.generatorVoltage) / 1000.0f) * (static_cast<float>(dataToShow.generatorCurrent) / 1000.0f)));
    ui->groundSupplyPower->display(formatFloat((static_cast<float>(dataToShow.groundSupplyVoltage) / 1000.0f) * (static_cast<float>(dataToShow.groundSupplyCurrent) / 1000.0f)));
    ui->vBusPower->display(formatFloat((static_cast<float>(dataToShow.vBusVoltage) / 1000.0f) * (static_cast<float>(dataToShow.vBusCurrent) / 1000.0f)));
    ui->b1_24vPower->display(formatFloat((static_cast<float>(dataToShow.b1_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b1_24VoltCurrent) / 1000.0f)));
    ui->b1A_24vPower->display(formatFloat((static_cast<float>(dataToShow.b1A_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b1A_24VoltCurrent) / 1000.0f)));
    ui->b2_8v2Power->display(formatFloat((static_cast<float>(dataToShow.b2v2_8VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b2v2_8VoltCurrent) / 1000.0f)));
    ui->b3_8v2Power->display(formatFloat((static_cast<float>(dataToShow.b3v2_8VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b3v2_8VoltCurrent) / 1000.0f)));
    ui->b3a_8v2Power->display(formatFloat((static_cast<float>(dataToShow.b3av2_8VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b3av2_8VoltCurrent) / 1000.0f)));
    ui->b4_12vPower->display(formatFloat((static_cast<float>(dataToShow.b4_12VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b4_12VoltCurrent) / 1000.0f)));
    ui->b4a_12vPower->display(formatFloat((static_cast<float>(dataToShow.b4a_12VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b4a_12VoltCurrent) / 1000.0f)));
    ui->b5_24vPower->display(formatFloat((static_cast<float>(dataToShow.b5_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b5_24VoltCurrent) / 1000.0f)));
    ui->b5a_24vPower->display(formatFloat((static_cast<float>(dataToShow.b5a_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b5a_24VoltCurrent) / 1000.0f)));
    ui->b5b_24vPower->display(formatFloat((static_cast<float>(dataToShow.b5b_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b5b_24VoltCurrent) / 1000.0f)));
    ui->b6_5vPower->display(formatFloat((static_cast<float>(dataToShow.b6_5VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b6_5VoltCurrent) / 1000.0f)));
    ui->totalOutputPower->display(formatFloat(
        ((static_cast<float>(dataToShow.b1_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b1_24VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b1A_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b1A_24VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b2v2_8VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b2v2_8VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b3v2_8VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b3v2_8VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b3av2_8VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b3av2_8VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b4_12VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b4_12VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b4a_12VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b4a_12VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b5_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b5_24VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b5a_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b5a_24VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b5b_24VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b5b_24VoltCurrent) / 1000.0f)) +
        ((static_cast<float>(dataToShow.b6_5VoltVoltage) / 1000.0f) * (static_cast<float>(dataToShow.b6_5VoltCurrent) / 1000.0f))
        ));



    ui->ain_1Voltage->display(formatFloat(static_cast<float>(dataToShow.aIn1Voltage) / 1000));
    ui->ain_2Voltage->display(formatFloat(static_cast<float>(dataToShow.aIn2Voltage) / 1000));
    ui->ain_3Voltage->display(formatFloat(static_cast<float>(dataToShow.aIn3Voltage) / 1000));
    ui->temperature->display(formatFloat(static_cast<float>(dataToShow.tempSensorInternal) / 1000));
    ui->boostTemp1->display(formatFloat(static_cast<float>(dataToShow.boostTempSensor1) / 100));
    ui->boostTemp2->display(formatFloat(static_cast<float>(dataToShow.boostTempSensor2) / 100));

    ui->debugV1->display(formatFloat(static_cast<float>(dataToShow.debugVoltage1_BST) / 1000));
    ui->debugC1->display(formatFloat(static_cast<float>(dataToShow.debugCurrent1_BST) / 1000));
    ui->debugV2->display(formatFloat(static_cast<float>(dataToShow.debugVoltage2_AVIN) / 1000));
    ui->debugV3->display(formatFloat(static_cast<float>(dataToShow.debugVoltage3_SP1) / 1000));
    ui->debugV4->display(formatFloat(static_cast<float>(dataToShow.debugVoltage4_SP2) / 1000));
    ui->debugV5->display(formatFloat(static_cast<float>(dataToShow.debugVoltage5_SP3) / 1000));
    ui->debugV6->display(formatFloat(static_cast<float>(dataToShow.debugVoltage6_VIN_BP) / 1000));



    ui->cmdCount->display(dataToShow.commandCounter);
    ui->lastCmd->display(dataToShow.lastAppliedCommand);
    ui->pktErrorCountDisp->display(dataToShow.packetErrorCounter);
    ui->headerErrorCountDisp->display(dataToShow.headerErrorCounter);
    ui->crcErrorCountDisp->display(dataToShow.crcErrorCounter);
    ui->serialTimeoutErrorCount->display(dataToShow.serialTimeoutErrorCounter);
    ui->lengthMismatchErrorCount->display(dataToShow.frameLengthMismatchErrorCounter);


    dataToShow.b1a_24V_status ? ui->b1aCheckbox->setCheckState(Qt::Checked) : ui->b1aCheckbox->setCheckState(Qt::Unchecked);
    dataToShow.b3a_8V2_status ? ui->b3aCheckBox->setCheckState(Qt::Checked) : ui->b3aCheckBox->setCheckState(Qt::Unchecked);
    dataToShow.b4a_12V_status ? ui->b4aCheckbox->setCheckState(Qt::Checked) : ui->b4aCheckbox->setCheckState(Qt::Unchecked);
    dataToShow.b5a_24V_status ? ui->b5aCheckBox->setCheckState(Qt::Checked) : ui->b5aCheckBox->setCheckState(Qt::Unchecked);
    dataToShow.b5b_24V_status ? ui->b5bCheckBox->setCheckState(Qt::Checked) : ui->b5bCheckBox->setCheckState(Qt::Unchecked);


    dataToShow.dOutStatus1 ? ui->dout1CHeckbox->setCheckState(Qt::Checked) : ui->dout1CHeckbox->setCheckState(Qt::Unchecked);
    dataToShow.dOutStatus2 ? ui->dout2Checkbox->setCheckState(Qt::Checked) : ui->dout2Checkbox->setCheckState(Qt::Unchecked);
    dataToShow.pwmStatus1 ? ui->pwm1Checkbox->setCheckState(Qt::Checked) : ui->pwm1Checkbox->setCheckState(Qt::Unchecked);
    dataToShow.pwmStatus2 ? ui->pwm2Checkbox->setCheckState(Qt::Checked) : ui->pwm2Checkbox->setCheckState(Qt::Unchecked);
    dataToShow.pyroArmStatus ? ui->pyraArmCheckbox->setCheckState(Qt::Checked) : ui->pyraArmCheckbox->setCheckState(Qt::Unchecked);
    dataToShow.pyroAgnStatus ? ui->pyroIgnCheckbox->setCheckState(Qt::Checked) : ui->pyroIgnCheckbox->setCheckState(Qt::Unchecked);
    dataToShow.batSwitchStatus ? ui->batSwitchCheckbox->setCheckState(Qt::Checked) : ui->batSwitchCheckbox->setCheckState(Qt::Unchecked);






    toUpdate = false;

}

void MainWindow::onTimeTimerTimeout()
{
    QTime currentTime = QTime::currentTime();
    ui->timeLabel->setText(currentTime.toString("HH:mm:ss"));
}


QString MainWindow::formatFloat(float value)
{
    QString floatValStr = QString("%1").arg(value, 4, 'f', 1, '0');
    return floatValStr;

            // double mapValue = voltageMap.value(id);
            // double lowerBound = mapValue * 0.95 - 1; // 1 - 0.5% of the value from the map
            // double upperBound = mapValue * 1.05 + 1; // 1 + 0.5% of the value from the map

            // if (value >= lowerBound && value <= upperBound) {
            //     lcdNumber->setStyleSheet("QLCDNumber { color: green; }");
            // } else {
            //     lcdNumber->setStyleSheet("QLCDNumber { color: red; }");
            // }


}





void MainWindow::logDataToCSV() {
    QString csvString = stateparamsToCsv();
    out <<  QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << ", " << csvString << "\n";
    out.flush();
    file.flush();
}

void MainWindow::logFileInit()
{
    // Get the current date and time
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");

    // Get the path to the "My Documents" folder
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documentsPath.isEmpty()) {
        qWarning() << "Unable to locate the My Documents folder.";
        return;
    }

    // Create the directory path for the logs
    QString logsDirPath = QDir(documentsPath).filePath("PowerBlaze");
    QDir logsDir(logsDirPath);

    // Check if the directory exists and create it if it does not
    if (!logsDir.exists()) {
        if (!logsDir.mkpath(logsDirPath)) {
            qWarning() << "Unable to create directory:" << logsDirPath;
            return;
        }
    }


    // Create the file name
    QString fileName = QString("PowerBlaze/logs-%1.csv").arg(currentDateTime);

    QString cmdLogFileName = QString("PowerBlaze/cmd-logs-%1.csv").arg(currentDateTime);


    // Construct the full file path
    QString filePath = QDir(documentsPath).filePath(fileName);
    QString cmdFilePath = QDir(documentsPath).filePath(cmdLogFileName);
    file.setFileName(filePath);
    cmdLogFile.setFileName(cmdFilePath);

    // Open or create the file
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file for writing:" << filePath;
        return;
    }

    // Open or create the file
    if (!cmdLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file for writing:" << filePath;
        return;
    }

    out.setDevice(&file);
     out << getCSVHeaders() << "\n";
    out.flush();
    file.flush();

    cmdLogOut.setDevice(&cmdLogFile);
    cmdLogOut << getCmdLogCSVHeaders() << "\n";
    cmdLogOut.flush();
    cmdLogFile.flush();


}

QString MainWindow::stateparamsToCsv() {
    QString csv;
    QTextStream stream(&csv);

    stream << dataToShow.PacketCounter << ","
           << dataToShow.FrameTime << ","

           // --- Power Channels ---
           << dataToShow.avionicBatteryVoltage << "," << dataToShow.avionicBatteryCurrent << ","
           << dataToShow.generatorVoltage << "," << dataToShow.generatorCurrent << ","
           << dataToShow.groundSupplyVoltage << "," << dataToShow.groundSupplyCurrent << ","
           << dataToShow.vBusVoltage << "," << dataToShow.vBusCurrent << ","
           << dataToShow.b1_24VoltVoltage << "," << dataToShow.b1_24VoltCurrent << ","
           << dataToShow.b1A_24VoltVoltage << "," << dataToShow.b1A_24VoltCurrent << ","
           << dataToShow.b2v2_8VoltVoltage << "," << dataToShow.b2v2_8VoltCurrent << ","
           << dataToShow.b3v2_8VoltVoltage << "," << dataToShow.b3v2_8VoltCurrent << ","
           << dataToShow.b3av2_8VoltVoltage << "," << dataToShow.b3av2_8VoltCurrent << ","
           << dataToShow.b4_12VoltVoltage << "," << dataToShow.b4_12VoltCurrent << ","
           << dataToShow.b4a_12VoltVoltage << "," << dataToShow.b4a_12VoltCurrent << ","
           << dataToShow.b5_24VoltVoltage << "," << dataToShow.b5_24VoltCurrent << ","
           << dataToShow.b5a_24VoltVoltage << "," << dataToShow.b5a_24VoltCurrent << ","
           << dataToShow.b5b_24VoltVoltage << "," << dataToShow.b5b_24VoltCurrent << ","
           << dataToShow.b6_5VoltVoltage << "," << dataToShow.b6_5VoltCurrent << ","

           // --- Analog Inputs ---
           << dataToShow.aIn1Voltage << "," << dataToShow.aIn2Voltage << "," << dataToShow.aIn3Voltage << ","
           << dataToShow.tempSensorInternal << ","

           // --- Debug Voltages/Currents ---
           << dataToShow.debugVoltage1_BST << "," << dataToShow.debugCurrent1_BST << ","
           << dataToShow.debugVoltage2_AVIN << "," << dataToShow.debugVoltage3_SP1 << ","
           << dataToShow.debugVoltage4_SP2 << "," << dataToShow.debugVoltage5_SP3 << ","
           << dataToShow.debugVoltage6_VIN_BP << ","

           // --- Boolean Flags ---
           << dataToShow.dOutStatus1 << "," << dataToShow.dOutStatus2 << ","
           << dataToShow.pwmStatus1 << "," << dataToShow.pwmStatus2 << ","
           << dataToShow.pyroArmStatus << "," << dataToShow.pyroAgnStatus << ","
           << dataToShow.batSwitchStatus << "," << dataToShow.spareFlag << ","
           << dataToShow.b1a_24V_status << "," << dataToShow.b3a_8V2_status << ","
           << dataToShow.b4a_12V_status << "," << dataToShow.b5a_24V_status << ","
           << dataToShow.b5b_24V_status << "," << dataToShow.spareFlag2 << ","
           << dataToShow.spareFlag3 << "," << dataToShow.spareFlag4 << ","

           // --- Engine & Diagnostics ---
           << dataToShow.engineRPM << ","
           << dataToShow.commandCounter << ","
           << dataToShow.lastAppliedCommand << ","

           // --- Error Counters ---
           << dataToShow.packetErrorCounter << ","
           << dataToShow.headerErrorCounter << ","
           << dataToShow.crcErrorCounter << ","
           << dataToShow.serialTimeoutErrorCounter << ","
           << dataToShow.frameLengthMismatchErrorCounter;

    return csv;
}

QString MainWindow::getCSVHeaders() {
    QStringList headers;

    headers
        << "Packet Counter"
        << "Frame Time"

        // --- Power Channels ---
        << "Avionic Battery Voltage" << "Avionic Battery Current"
        << "Generator Voltage" << "Generator Current"
        << "Ground Supply Voltage" << "Ground Supply Current"
        << "VBus Voltage" << "VBus Current"
        << "B1 24V Voltage" << "B1 24V Current"
        << "B1A 24V Voltage" << "B1A 24V Current"
        << "B2 2.8V Voltage" << "B2 2.8V Current"
        << "B3 2.8V Voltage" << "B3 2.8V Current"
        << "B3A 2.8V Voltage" << "B3A 2.8V Current"
        << "B4 12V Voltage" << "B4 12V Current"
        << "B4A 12V Voltage" << "B4A 12V Current"
        << "B5 24V Voltage" << "B5 24V Current"
        << "B5A 24V Voltage" << "B5A 24V Current"
        << "B5B 24V Voltage" << "B5B 24V Current"
        << "B6 5V Voltage" << "B6 5V Current"

        // --- Analog Inputs ---
        << "Analog In 1 Voltage"
        << "Analog In 2 Voltage"
        << "Analog In 3 Voltage"
        << "Internal Temp Sensor"

        // --- Debug Values ---
        << "Debug Voltage 1 (BST)"
        << "Debug Current 1 (BST)"
        << "Debug Voltage 2 (AVIN)"
        << "Debug Voltage 3 (SP1)"
        << "Debug Voltage 4 (SP2)"
        << "Debug Voltage 5 (SP3)"
        << "Debug Voltage 6 (VIN_BP)"

        // --- Boolean Flags ---
        << "DOut Status 1"
        << "DOut Status 2"
        << "PWM Status 1"
        << "PWM Status 2"
        << "Pyro Arm Status"
        << "Pyro Agn Status"
        << "Battery Switch Status"
        << "Spare Flag"
        << "B1A 24V Status"
        << "B3A 8V2 Status"
        << "B4A 12V Status"
        << "B5A 24V Status"
        << "B5B 24V Status"
        << "Spare Flag 2"
        << "Spare Flag 3"
        << "Spare Flag 4"

        // --- Engine & Diagnostics ---
        << "Engine RPM"
        << "Command Counter"
        << "Last Applied Command"

        // --- Error Counters ---
        << "Packet Error Counter"
        << "Header Error Counter"
        << "CRC Error Counter"
        << "Serial Timeout Error Counter"
        << "Frame Length Mismatch Error Counter";

    return headers.join(", ");
}



QString MainWindow::getCmdLogCSVHeaders() {
    QStringList headers;

    headers << "Timestamp"
            << "Packet";






    return headers.join(", ");
}













void MainWindow::on_b1aCheckbox_clicked()
{
    if(ui->b1aCheckbox->checkState() == Qt::Checked){
        Command cmd(b1A_24V, On);
        sendCommand(cmd);
    }else{
        Command cmd(b1A_24V, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_b3aCheckBox_clicked()
{
    if(ui->b3aCheckBox->checkState() == Qt::Checked){
        Command cmd(b3A_8V2, On);
        sendCommand(cmd);
    }else{
        Command cmd(b3A_8V2, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_b4aCheckbox_clicked()
{
    if(ui->b4aCheckbox->checkState() == Qt::Checked){
        Command cmd(b4A_12V, On);
        sendCommand(cmd);
    }else{
        Command cmd(b4A_12V, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_b5aCheckBox_clicked()
{
    if(ui->b5aCheckBox->checkState() == Qt::Checked){
        Command cmd(b5A_24V, On);
        sendCommand(cmd);
    }else{
        Command cmd(b5A_24V, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_b5bCheckBox_clicked()
{
    if(ui->b5bCheckBox->checkState() == Qt::Checked){
        Command cmd(b5b_24V, On);
        sendCommand(cmd);
    }else{
        Command cmd(b5b_24V, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_dout1CHeckbox_clicked()
{
    if(ui->dout1CHeckbox->checkState() == Qt::Checked){
        Command cmd(DOUT_1, On);
        sendCommand(cmd);
    }else{
        Command cmd(DOUT_1, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_dout2Checkbox_clicked()
{
    if(ui->dout2Checkbox->checkState() == Qt::Checked){
        Command cmd(DOUT_2, On);
        sendCommand(cmd);
    }else{
        Command cmd(DOUT_2, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_pwm1Checkbox_clicked()
{
    QString value = ui->pwmLineEdit->text();
    bool ok;
    int pwmValue = value.toInt(&ok);  // Try to convert to int

    if (!ok) {
        QMessageBox::warning(this, "Invalid Input",
                             "Please enter a valid integer value for PWM.");
        return;
    }

    pwmValue /= 10;  // Divide by 10

    if (pwmValue < 1 || pwmValue > 255) {
        QMessageBox::warning(this, "Out of Range",
                             "PWM value must be between 10 and 2550.");
        return;
    }

    qDebug() << "PWM value after scaling:" << pwmValue;

    if(ui->pwm1Checkbox->checkState() == Qt::Checked){
        Command cmd(PWM_1, pwmValue);
        sendCommand(cmd);
    }else{
        Command cmd(PWM_1, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_pwm2Checkbox_clicked()
{
    QString value = ui->pwmLineEdit->text();
    bool ok;
    int pwmValue = value.toInt(&ok);  // Try to convert to int

    if (!ok) {
        QMessageBox::warning(this, "Invalid Input",
                             "Please enter a valid integer value for PWM.");
        return;
    }

    pwmValue /= 10;  // Divide by 10

    if (pwmValue < 1 || pwmValue > 255) {
        QMessageBox::warning(this, "Out of Range",
                             "PWM value must be between 10 and 2550.");
        return;
    }

    qDebug() << "PWM value after scaling:" << pwmValue;
    if(ui->pwm2Checkbox->checkState() == Qt::Checked){
        Command cmd(PWM_2, pwmValue);
        sendCommand(cmd);
    }else{
        Command cmd(PWM_2, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_pyraArmCheckbox_clicked()
{
    if(ui->pyraArmCheckbox->checkState() == Qt::Checked){
        Command cmd(ARM, On);
        sendCommand(cmd);
    }else{
        Command cmd(ARM, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_pyroIgnCheckbox_clicked()
{
    if(ui->pyroIgnCheckbox->checkState() == Qt::Checked){
        Command cmd(IGN, On);
        sendCommand(cmd);
    }else{
        Command cmd(IGN, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_batSwitchCheckbox_clicked()
{
    if(ui->batSwitchCheckbox->checkState() == Qt::Checked){
        Command cmd(Battery_Switch, On);
        sendCommand(cmd);
    }else{
        Command cmd(Battery_Switch, Off);
        sendCommand(cmd);

    }
}


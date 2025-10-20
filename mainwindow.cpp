#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    applyShadow(ui->chargingBox);
    applyShadow(ui->CommBox);
    applyShadow(ui->analogInputBox);
    // applyShadow(ui->chargingBox_2);

    QPixmap logo(":/Resources/logo_white_bg.png");
    ui->logoLbl->setPixmap(logo);
    ui->logoLbl->setScaledContents(true);  // Optional: scale to fit the label size

    QIcon icon(":/Resources/app_icon.png");
    this->setWindowIcon(icon);

    this->showMaximized();
    this->setWindowTitle("PowerBlazev2.1.0");
    ui->versionLabel->setText("v2.1.0");


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

    ui->batteryVoltageDisp->display(formatFloat(0));
    ui->batteryCurrentDisp->display(formatFloat(0));
    ui->InasVoltageDisp->display(formatFloat(0));
    ui->InasCurrentDisp->display(formatFloat(0));
    ui->dataLinkVoltageDisp->display(formatFloat(0));
    ui->dataLinkCurrentDisp->display(formatFloat(0));
    ui->seekerVoltageDisp->display(formatFloat(0));
    ui->seekerCurrentDisp->display(formatFloat(0));
    ui->MagnoSoleVoltageDisp->display(formatFloat(0));
    ui->MagnoSoleCurrentDisp->display(formatFloat(0));
    // ui->twelveVVoltageDisp->display(formatFloat(0));
    // ui->twelveVCurrentDisp->display(formatFloat(0));
    ui->ActuatorOneVoltageDisp->display(formatFloat(0));
    ui->ActuatorOneCurrentDisp->display(formatFloat(0));
    ui->ActuatorTwoVoltageDisp->display(formatFloat(0));
    ui->ActuatorTwoCurrentDisp->display(formatFloat(0));
    ui->ActuatorThreeVoltageDisp->display(formatFloat(0));
    ui->ActuatorThreeCurrentDisp->display(formatFloat(0));
    ui->fiveVoltVoltageDisp->display(formatFloat(0));
    ui->fiveVoltCurrentDisp->display(formatFloat(0));
    ui->spareVolt1Disp->display(formatFloat(0));
    ui->spareVolt2Disp->display(formatFloat(0));
    ui->batteryPowerDisp->display(formatFloat(0));

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
    ui->batteryVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.BatteryVoltage) / 1000));
    ui->batteryCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.BatteryCurrent) / 1000));
    float current_mA = roundf((dataToShow.BatteryCurrent / 1000.0f) * 10.0f) / 10.0f;
    float voltage_V  = roundf((dataToShow.BatteryVoltage  / 1000.0f) * 10.0f) / 10.0f;

    float power_W = current_mA * voltage_V; // since mA * V = mW, divide by 1000 → W

    ui->batteryPowerDisp->display(formatFloat(power_W));
    ui->InasVoltageDisp->display(formatFloat(dataToShow.InasVoltage / 1000));
    ui->InasCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.InasCurrent) / 1000));
    ui->dataLinkVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.DataLinkVoltage) / 1000));
    ui->dataLinkCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.DataLinkCurrent) / 1000));
    ui->seekerVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.SeekerVoltage) / 1000));
    ui->seekerCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.SeekerCurrent) / 1000));
    ui->MagnoSoleVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.MagnoSoleVoltage) / 1000));
    ui->MagnoSoleCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.MagnoSoleCurrent) / 1000));
    ui->ActuatorOneVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.ActuatorOneVoltage) / 1000));
    ui->ActuatorOneCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.ActuatorOneCurrent) / 1000));
    ui->ActuatorTwoVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.ActuatorTwoVoltage) / 1000));
    ui->ActuatorTwoCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.ActuatorTwoCurrent) / 1000));
    ui->ActuatorThreeVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.ActuatorThreeVoltage) / 1000));
    ui->ActuatorThreeCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.ActuatorThreeCurrent) / 1000));
    ui->fiveVoltVoltageDisp->display(formatFloat(static_cast<float>(dataToShow.fiveVoltVoltage) / 1000));
    ui->fiveVoltCurrentDisp->display(formatFloat(static_cast<float>(dataToShow.fiveVoltCurrent) / 1000));
    ui->spareVolt1Disp->display(formatFloat(static_cast<float>(dataToShow.spareVolt1) / 1000));
    ui->spareVolt2Disp->display(formatFloat(static_cast<float>(dataToShow.spareVolt2) / 1000));
    ui->temperature->display(dataToShow.internalTemp);


    if(dataToShow.SeekerStatus){
        ui->seekerCheckBox->setCheckState(Qt::Checked);
    }else{
        ui->seekerCheckBox->setCheckState(Qt::Unchecked);

    }
    if(dataToShow.TwelveVoltStatus){
        ui->twelveVoltCheckBox->setCheckState(Qt::Checked);
    }else{
        ui->twelveVoltCheckBox->setCheckState(Qt::Unchecked);

    }

    // if(dataToShow.TwelveVoltStatus){
    //     ui->twelveVCtrlCheckBox->setCheckState(Qt::Checked);
    // }else{
    //     ui->twelveVCtrlCheckBox->setCheckState(Qt::Unchecked);

    // }
    ui->cmdCount->display(dataToShow.commandCounter);
    ui->lastCmd->display(dataToShow.lastCommand);
    ui->pktErrorCountDisp->display(dataToShow.pktErrorCounter);
    ui->headerErrorCountDisp->display(dataToShow.headerErrCounter);
    ui->crcErrorCountDisp->display(dataToShow.crcErrorCounter);
    ui->serialTimeoutErrorCount->display(dataToShow.serialTimeoutCounter);
    ui->lengthMismatchErrorCount->display(dataToShow.frameLengthMismatchErrCounter);






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

    // Adding boolean values
    stream << dataToShow.PacketCounter << ","
           << dataToShow.FrameTime << ","
           << dataToShow.BatteryVoltage << ","
           << dataToShow.BatteryCurrent << ","
           << dataToShow.InasVoltage << ","
           << dataToShow.InasCurrent << ","
           << dataToShow.DataLinkVoltage << ","
           << dataToShow.DataLinkCurrent << ","
           << dataToShow.SeekerVoltage << ","
           << dataToShow.SeekerCurrent << ","
           << dataToShow.MagnoSoleVoltage << ","
           << dataToShow.MagnoSoleCurrent << ","
           << dataToShow.TwelveVoltVoltage << ","
           << dataToShow.TwelveVoltCurrent << ","
           << dataToShow.ActuatorOneVoltage << ","
           << dataToShow.ActuatorOneCurrent << ","
           << dataToShow.ActuatorTwoVoltage << ","
           << dataToShow.ActuatorTwoCurrent << ","
           << dataToShow.ActuatorThreeVoltage << ","
           << dataToShow.ActuatorThreeCurrent << ","
           << dataToShow.fiveVoltVoltage << ","
           << dataToShow.fiveVoltCurrent << ","
           << dataToShow.internalTemp << ","
           << dataToShow.DataLinkStatus << ","
           << dataToShow.SeekerStatus << ","
           << dataToShow.TwelveVoltStatus << ","
           << dataToShow.commandCounter << ","
           << dataToShow.lastCommand << ","
           << dataToShow.pktErrorCounter << ","
           << dataToShow.headerErrCounter << ","
           << dataToShow.crcErrorCounter << ","
           << dataToShow.serialTimeoutCounter << ","
           << dataToShow.frameLengthMismatchErrCounter;



    return csv;
}

QString MainWindow::getCSVHeaders() {
    QStringList headers;

    headers << "Timestamp"
            << "Packet Counter"
            << "Frame Time"
            << "Battery Input Voltage"
            << "Battery Input Current"
            << "INAS Bus Voltage"
            << "INAS Bus Current"
            << "DataLink Bus Voltage"
            << "DataLink Bus Current"
            << "Seeker Bus Voltage"
            << "Seeker Bus Current"
            << "MagnoSole Bus Voltage"
            << "MagnoSole Bus Current"
            << "12V Ctrl Bus Voltage"
            << "12V Ctrl Bus Current"
            << "Actuator One Bus Voltage"
            << "Actuator One Bus Current"
            << "Actuator Two Bus Voltage"
            << "Actuator Two Bus Current"
            << "Actuator Three Bus Voltage"
            << "Actuator Three Bus Current"
            << "5V Bus Voltage"
            << "5V Bus Current"
            << "Temperature"
            << "DataLink Bus Status"
            << "Seeker Bus Status"
            << "12V Ctrl Bus Status"
            << "Command Counter"
            << "Last Command"
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








void MainWindow::on_seekerCheckBox_clicked()
{
    if(ui->seekerCheckBox->checkState() == Qt::Checked){
        Command cmd(SEEKER_CMD_ID, On);
        sendCommand(cmd);
    }else{
        Command cmd(SEEKER_CMD_ID, Off);
        sendCommand(cmd);

    }
}


void MainWindow::on_twelveVCtrlCheckBox_clicked()
{
    // if(ui->twelveVCtrlCheckBox->checkState() == Qt::Checked){
    //     Command cmd(TWELVE_V_CMD_ID, On);
    //     sendCommand(cmd);
    // }else{
    //     Command cmd(TWELVE_V_CMD_ID, Off);
    //     sendCommand(cmd);

    // }
}



void MainWindow::on_debugInfoBtn_clicked()
{
    static bool visible = false;

    // visible = !visible;
    // ui->spareVolt1Disp->setVisible(visible);
    // ui->spareVolt2Disp->setVisible(visible);
    // ui->spVolt1Lbl->setVisible(visible);
    // ui->spVolt2Lbl->setVisible(visible);

}




void MainWindow::on_twelveVoltCheckBox_clicked()
{
    if(ui->twelveVoltCheckBox->checkState() == Qt::Checked){
        Command cmd(TWELVE_V_CMD_ID, On);
        sendCommand(cmd);
    }else{
        Command cmd(TWELVE_V_CMD_ID, Off);
        sendCommand(cmd);

    }
}


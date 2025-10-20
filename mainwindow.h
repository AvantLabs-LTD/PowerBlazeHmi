#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>
#include <QTime>
#include "serialport.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
#include <QString>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void refreshPortList();
    void on_connectBtn_clicked();
    void on_refreshBtn_clicked();
    void handleConnected();
    void handleDisconnected();
    void onDataReceived(const Packet &data);
    void onDataSent(const QString &data);

    void updateAll();
    void onTimeTimerTimeout();


    void on_seekerCheckBox_clicked();

    void on_twelveVCtrlCheckBox_clicked();

    void on_debugInfoBtn_clicked();


    void on_twelveVoltCheckBox_clicked();

private:
    Ui::MainWindow *ui;
    SerialPort m_serialPort;
    bool isConnected = false;
    Packet dataToShow;
    QTimer *timer;
    QTimer *timeTimer;
    bool toUpdate = false;
    int forcedChannel = 0;

    QTime time = QTime::currentTime();  // Store this somewhere


    QFile file;
    QFile cmdLogFile;
    QTextStream out;
    QTextStream cmdLogOut;
    QMap<int, double> voltageMap;

    QString formatFloat(float value);
    void logDataToCSV();
    void logFileInit();
    QString stateparamsToCsv();
    QString getCSVHeaders();
    QString getCmdLogCSVHeaders();
signals:
    void sendCommand(const Command &cmd);
};
#endif // MAINWINDOW_H

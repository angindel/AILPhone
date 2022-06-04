#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSqlDatabase>
#include <QStandardItemModel>
#include <QTimer>
#include <QPrinter>
#include <QPrintDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openSerialPort();
    void closeSerialPort();
    void readData();
    void handleError(QSerialPort::SerialPortError error);
    void handleTimeout();
//    void on_tableView_clicked(const QModelIndex &index);
    void on_actionPrint_triggered();

    void on_actionRefresh_triggered();

    void on_actiondelete_triggered();

private:
    void putData(const QByteArray &data);
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QByteArray allData;
    QTimer m_timer;
    QSqlDatabase db;
    int initDb();
    void readAndDisplayTable();
    void insertDataPhone(QStringList data);
    QStandardItemModel *model;
    QString pwaktu(QString waktu, QString durasi);
    void printke(QStringList data, int notad, QString biaya);
    QString cost(QString biaya);
    void procFile(const QByteArray &data);
    QString ptgl(QString tgl);
    void initPhone();
    void deleteDataPhone(int id);
};
#endif // MAINWINDOW_H

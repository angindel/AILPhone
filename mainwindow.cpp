#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QMessageBox>
#include <QPrinterInfo>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QPainter>
#include <QTextStream>
#include <QDir>
#include <QSqlError>
//#include <QDebug>
#include <QSqlResult>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setStyleSheet("background-color: rgb(255, 255, 255);"
                        "border-color: rgb(0, 0, 0);");
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->verticalHeader()->hide();
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    serial = new QSerialPort(this);
    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &MainWindow::handleError);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer.start(5000);
    if(initDb() == 0)
    {
        openSerialPort();
        readAndDisplayTable();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    serial->close();
    db.close();
}

void MainWindow::openSerialPort()
{
    serial->setPortName("COM1");
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    QMessageBox msgBox;
    if (serial->open(QIODevice::ReadWrite)) {
        ui->status->setText("CONNECTED COM 1");
    }
    else {
        ui->status->setText("NOT CONNECTED");
    }
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
}

void MainWindow::readData()
{
    allData.append(serial->readAll());

    if (!m_timer.isActive())
        m_timer.start(5000);
}

void MainWindow::handleTimeout()
{
    if (!allData.isEmpty()) {
        putData(allData);
    }
}

void MainWindow::putData(const QByteArray &data)
{
    procFile(data);
    QString proData = QString(data);
    QRegExp ex("(#+|<|>)",Qt::CaseInsensitive);
    QRegularExpression ex3("(?:(?:[1-9]|1[0-2])/(?:\\s[1-9]|[1-9]|[12][0-9]|3[01])/(?:\\d{2}))");
    QRegularExpression ex2("(?:(?:[1-9]|1[0-2])/(?:\\s[1-9]|[1-9]|[12][0-9]|3[01])/(?:\\d{2}))|(?:(?:[1-9]|1[0-2]):(?:\\d{2})(?:[A|P]M)).*", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator match = ex2.globalMatch(proData);
    if(match.isValid())
    {
        QString dats;
        while(match.hasNext())
        {
            QRegularExpressionMatch match2 = match.next();
            QString dat = match2.captured();
            if(ex3.match(dat).hasMatch())
                dats += dat.replace(" ", "") + " ";
            else
                dats += dat;
            proData = dats;
        }
        proData = proData.replace("\r","#").replace("\n","#").replace(" ","#");
        proData = proData.replace(ex,"|");
        QStringList list1 = proData.split('|', QString::SkipEmptyParts);
        insertDataPhone(list1);
        allData.clear();
    }
    else
    {
        QMessageBox pesan;
        pesan.setText("DATA : \""+ proData + "\" ADA KESALAHAN!!!!");
        pesan.exec();
        allData.clear();
    }
}


void MainWindow::readAndDisplayTable()
{
    initPhone();
    QSqlQuery query("SELECT ID,TGL,WAKTU,EXT,CO,DIAL_NUMBER,DURASI,CODE FROM TELP ORDER BY ID ASC");
    QSqlQuery qry("SELECT TGL FROM TELP");
    QSqlRecord rec = query.record();
    int row = 0;
    while(qry.next())
    {
        row++;
    }
    model = new QStandardItemModel(row,rec.count(),this);
    QStringList listHeader;
    listHeader << "NO" << "TGL" << "WAKTU" << "EXT" << "CO" << "NO TELP" << "DURASI" << "BIAYA";
    model->setHeaderData(0, Qt::Horizontal, listHeader.at(0));
    model->setHeaderData(1, Qt::Horizontal, listHeader.at(1));
    model->setHeaderData(2, Qt::Horizontal, listHeader.at(2));
    model->setHeaderData(3, Qt::Horizontal, listHeader.at(3));
    model->setHeaderData(4, Qt::Horizontal, listHeader.at(4));
    model->setHeaderData(5, Qt::Horizontal, listHeader.at(5));
    model->setHeaderData(6, Qt::Horizontal, listHeader.at(6));
    model->setHeaderData(7, Qt::Horizontal, listHeader.at(7));

    int r = 0;
    ulong total = 0;
    while(query.next())
    {
        QModelIndex no = model->index(r, 0, QModelIndex());
        QModelIndex tgl = model->index(r, 1, QModelIndex());
        QModelIndex waktu = model->index(r, 2, QModelIndex());
        QModelIndex ext = model->index(r, 3, QModelIndex());
        QModelIndex co = model->index(r, 4, QModelIndex());
        QModelIndex dial = model->index(r, 5, QModelIndex());
        QModelIndex durasi = model->index(r, 6, QModelIndex());
        QModelIndex cos = model->index(r, 7, QModelIndex());
        QVariant nod = query.value(0);
        QString tgld = ptgl(query.value(1).toString());
        QString waktud = query.value(2).toString();
        QString extd = query.value(3).toString();
        QString cod = query.value(4).toString();
        QString diald = query.value(5).toString();
        QString durasid = query.value(6).toString();
        QString cosd = cost(durasid);
        durasid = durasid.replace("'",":").replace('\"', "");
        total += cosd.toULong();

        model->setData(no, nod);
        model->setData(tgl, tgld);
        model->setData(waktu, pwaktu(waktud, durasid));
        model->setData(ext, extd);
        model->setData(co, cod);
        model->setData(dial, diald);
        model->setData(durasi, durasid);
        model->setData(cos, cosd);
        r++;
    }
    ui->tableView->setModel(model);
    qry.clear();
    query.clear();
}

void MainWindow::insertDataPhone(QStringList data)
{
    QSqlQuery *query = new QSqlQuery(db);
    query->prepare("INSERT INTO TELP(TGL, WAKTU, EXT, CO, DIAL_NUMBER, DURASI, CODE) VALUES (:tgl, :waktu, :ext, :co, :dial_number, :durasi, :code)");
    query->bindValue(":tgl", QString(data.at(0)));
    query->bindValue(":waktu", QString(data.at(1)));
    query->bindValue(":ext", QString(data.at(2)));
    query->bindValue(":co", QString(data.at(3)));
    query->bindValue(":dial_number", QString(data.at(4)));
    query->bindValue(":durasi", QString(data.at(5)));
    query->bindValue(":code", QString(data.at(6)));
    if(!query->exec())
    {
        QMessageBox msgBox;
        msgBox.setText("Insert GAGAL");
        msgBox.exec();
        return;
    }
    query->clear();
    QSqlQuery qry("SELECT TGL FROM TELP");
    int row = 0;
    while(qry.next())
    {
        row++;
    }
    QString biaya = data.at(5);
    biaya = biaya.replace("'",":").replace('\"', "");
    QString biayad = cost(biaya);
    readAndDisplayTable();
    qry.clear();
    printke(data, row, biayad);

}

QString MainWindow::pwaktu(QString waktu, QString durasi)
{
    QDateTime dt;
    int h = waktu.split(":")[0].toInt();
    int m = waktu.split(":")[1].leftRef(2).toInt();
    int ampm;
    if(waktu.right(2) == "PM")
    {
        if(h == 12)
            ampm = h;
        else
            ampm = h + 12;
    }
    else
    {
        if(h == 12)
            ampm = 0;
        else
            ampm = h;
    }
    int s = 0;
    QStringList dur = durasi.replace("'",":").replace('\"', "").split(":");
    QString hd = dur.at(0);
    QString md = dur.at(1);
    QString sd = dur.at(2);
    if(hd != "00")
        ampm += hd.toInt();

    if(md != "00")
    {
        m += md.toInt();
        if (m > 59)
        {
            ampm += 1;
            m = m % 60;
        }
    }

    if(sd != "00")
        s += sd.toInt();

   dt.setTime(QTime(ampm,m,s));
   return dt.time().toString("hh:mm:ss");
}

void MainWindow::printke(QStringList data,int notad ,QString biaya)
{
    QPrinterInfo pi;
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setPrinterName(pi.defaultPrinterName());
    printer.setOrientation(QPrinter::Portrait);
    printer.setPageOrder(QPrinter::FirstPageFirst);
    printer.setPaperSource(QPrinter::Auto);
    printer.setPaperName("MYPHONE");
    printer.setDocName("MYPHONE");
    printer.setPrintRange(QPrinter::AllPages);
    QPainter painter(&printer);
    painter.setPen(Qt::black);
    QFont f;
    f.setFamily("MS Sans Serif");
    f.setPointSize(14);
    f.setWeight(65);
    f.setStretch(100);
    f.setStyle(QFont::StyleNormal);
    painter.setFont(f);
    QRect rec(10,5,380,430);
    QString tgl = ptgl(data.at(0));
    int nota = notad;
    QString garis = "-----------------------------------";
    QString ext = data.at(2);
    QString nomor = data.at(4);
    QString dest = "HANDPHONE";
    QString dura = data.at(5);
    QString waktu = pwaktu(data.at(1), dura.replace("'",":").replace('\"', "")) ;
    QString cos = biaya;
    QString str;
    QTextStream out(&str);
    out << "LAPAS Kelas II Banyuasin\n";
    out << "Banyuasin\nBanyuasin\n";
    out << tgl << "\n";
    out << "\t\t\t" << waktu << "\n";
    out << "Nota :\n";
    out << "\t\t\t" << nota << "\n";
    out << garis << "\n";
    out << "Ext :\n";
    out << "\t\t\t" << ext.toInt() << "\n";
    out << "Dial :\n";
    out << "\t\t\t" << nomor << "\n";
    out << "Destination :\n";
    out << "\t\t\t" << dest << "\n";
    out << "Duration :\n";
    out << "\t\t\t" << dura.replace("'",":").replace('\"', "") << "\n";
    out << "Cost :\n";
    out << "\t\t\t" << cos<< "\n";
    out << garis << "\n";
    out << "terima kasih";
    painter.drawText(rec, Qt::AlignLeft,str);
}

QString MainWindow::cost(QString biaya)
{
    biaya = biaya.replace("'",":").replace('\"', "");
    int j = biaya.split(":")[0].toInt();
    int m = biaya.split(":")[1].toInt();
    int d = biaya.split(":")[2].toInt();

    int totj = j*60*2500;
    int totm = m*2500;
    int tots = (d <= 59 && d != 0) ? 1 * 2500 : 0 * 2500;
    return QString::number(totj+totm+tots);
}

void MainWindow::procFile(const QByteArray &data)
{
    QDateTime now = QDateTime::currentDateTime();
    QString str = QString("AP") + QString::number(now.date().day()) + QString::number(now.date().month()) + QString::number(now.date().year()) + QString(".ILT");
    QDir dir("C:\\ailphone\\txt");
    if(!dir.exists())
    {
        dir.mkdir(dir.path());
    }
    QFile f(dir.path() + "\\" + str);
    f.open(QIODevice::WriteOnly | QIODevice::Append);
    if(f.write(data) == -1)
    {
        f.close();
    }
    else
    {
        f.close();
    }
}

QString MainWindow::ptgl(QString tgl)
{
    QStringList tanggal = tgl.split("/");
    QString tahun = tanggal.at(1) + "/" + tanggal.at(0) + "/" + QString("20") + tanggal.at(2);
    return tahun;
}

void MainWindow::initPhone()
{
    QDateTime dt = QDateTime::currentDateTime();
    QString now = dt.toString("M/d/yy");
    QString y = dt.toString("yy");
    QString m = dt.toString("M");
    QString d = dt.toString("d");
    QString bulanIni = m + "/%/" + y;
    QString bulanKemarin = QString::number(m.toInt() - 1) + "/%/" + y;

    ulong totHariIni = 0;
    ulong totBulanIni = 0;
    ulong totBulanKemarin = 0;
    ulong totSemua = 0;

    QSqlQuery q;
    q.prepare("SELECT DURASI FROM TELP WHERE TGL ='"+now+"'");
    q.exec();
    while(q.next())
    {
        QString cosd = cost(q.value(0).toString());
        totHariIni += cosd.toULong();
    }
    q.clear();

    q.prepare("SELECT DURASI FROM TELP WHERE TGL like '"+bulanIni+"'");
    q.exec();
    while(q.next())
    {
        QString cosd = cost(q.value(0).toString());
        totBulanIni += cosd.toULong();
    }
    q.clear();

    q.prepare("SELECT DURASI FROM TELP WHERE TGL like '"+bulanKemarin+"'");
    q.exec();
    while(q.next())
    {
        QString cosd = cost(q.value(0).toString());
        totBulanKemarin += cosd.toULong();
    }
    q.clear();

    q.prepare("SELECT DURASI FROM TELP");
    q.exec();
    while(q.next())
    {
        QString cosd = cost(q.value(0).toString());
        totSemua += cosd.toULong();
    }
    q.clear();

    ui->textEdit->setText(QString::number(totSemua));
    ui->textEdit_2->setText(QString::number(totBulanKemarin));
    ui->textEdit_3->setText(QString::number(totBulanIni));
    ui->textEdit_4->setText(QString::number(totHariIni));
}

void MainWindow::deleteDataPhone(int id)
{
    QSqlQuery *q = new QSqlQuery(db);
    q->prepare("DELETE FROM TELP WHERE ID=?");
    q->addBindValue(id);
    if(!q->exec())
    {
        QMessageBox msg;
        msg.setText("DELETE FAILED");
        msg.exec();
    }
}

int MainWindow::initDb()
{
    db = QSqlDatabase::addDatabase("QODBC3");
    db.setDatabaseName("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};FIL={MS Access};DBQ=c:\\ailphone\\phone.mdb;");
    db.setPassword("@Baseng696");
    if(!db.open())
    {
        QMessageBox msgBox;
        msgBox.setText(db.lastError().text());
        msgBox.setDefaultButton(QMessageBox::Close);
        msgBox.exec();
        return 1;
    }
    return 0;
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}


//void MainWindow::on_tableView_clicked(const QModelIndex &index)
//{
//    QString val = ui->tableView->model()->data(index).toString();
//    ui->textEdit->setText(val);
//    qDebug() << "Clicked";
//}


void MainWindow::on_actionPrint_triggered()
{
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedIndexes();
    QPrinterInfo pi;
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setPrinterName(pi.defaultPrinterName());
    printer.setOrientation(QPrinter::Portrait);
    printer.setPageOrder(QPrinter::FirstPageFirst);
    printer.setPaperSource(QPrinter::Auto);
    printer.setPaperName("MYPHONE");
    printer.setDocName("MYPHONE");
    printer.setPrintRange(QPrinter::AllPages);
    QPrintDialog printdialog(&printer, this);
    printdialog.setWindowTitle("Print Document");
    if(printdialog.exec() == QDialog::Rejected) return;
    QPainter painter(&printer);
    painter.setPen(Qt::black);
    QFont f;
    f.setFamily("MS Sans Serif");
    f.setPointSize(14);
    f.setWeight(65);
    f.setStretch(100);
    f.setStyle(QFont::StyleNormal);
    painter.setFont(f);
    QRect rec(10,5,380,430);
    int nota = indexes.at(0).data().toInt();
    QString tgl = indexes.at(1).data().toString();
    QString waktu = indexes.at(2).data().toString();
    QString garis = "-----------------------------------";
    QString ext = indexes.at(3).data().toString();
    QString nomor = indexes.at(5).data().toString();
    QString dest = "HANDPHONE";
    QString dura = indexes.at(6).data().toString();
    QString cos = indexes.at(7).data().toString();
    QString str;
    QTextStream out(&str);
    out << "LAPAS Kelas II Banyuasin\n";
    out << "Banyuasin\nBanyuasin\n";
    out << tgl << "\n";
    out << "\t\t\t" << waktu << "\n";
    out << "Nota :\n";
    out << "\t\t\t" << nota << "\n";
    out << garis << "\n";
    out << "Ext :\n";
    out << "\t\t\t" << ext.toInt() << "\n";
    out << "Dial :\n";
    out << "\t\t\t" << nomor << "\n";
    out << "Destination :\n";
    out << "\t\t\t" << dest << "\n";
    out << "Duration :\n";
    out << "\t\t\t" << dura.replace("'",":").replace('\"', "") << "\n";
    out << "Cost :\n";
    out << "\t\t\t" << cos<< "\n";
    out << garis << "\n";
    out << "terima kasih";
    painter.drawText(rec, Qt::AlignLeft,str);
}

void MainWindow::on_actionRefresh_triggered()
{
    readAndDisplayTable();
}


void MainWindow::on_actiondelete_triggered()
{
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedIndexes();
    deleteDataPhone(indexes.at(0).data().toInt());
    readAndDisplayTable();
}


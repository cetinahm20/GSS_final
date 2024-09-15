#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>
#include <QFile>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QUrl>
#include <QMediaContent>
#include <QMediaRecorder>
#include <QUdpSocket>
#include <QVariant>
#include <QtCore>
#include <QtGui>
#include <QRandomGenerator>
#include <QWebEngineView>
#include <QDateTime>
#include <QScreen>
#include <QGuiApplication>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

void MainWindow::readData()
{
    while (socket4->hasPendingDatagrams()) {
        QByteArray datagram(socket4->pendingDatagramSize(), Qt::Uninitialized);
        socket4->readDatagram(datagram.data(), datagram.size());

        //qDebug() << "Datagram raw data:" << datagram.toHex() << "," << datagram.size();

        int index = 0;
        auto extractData = [&](auto& value, int size) {
            value = qFromLittleEndian<typename std::remove_reference<decltype(value)>::type>(
                reinterpret_cast<const uchar*>(datagram.mid(index, size).constData()));
            index += size;
        };

        extractData(telemetri.pakNo, 2);
        telemetri.state = static_cast<unsigned char>(datagram[index++]);
        telemetri.hataKodu = static_cast<unsigned char>(datagram[index++]);
        extractData(telemetri.zaman, 8);
        extractData(telemetri.bas1, 4);
        extractData(telemetri.bas2, 4);
        extractData(telemetri.yuk1, 4);
        extractData(telemetri.yuk2, 4);
        extractData(telemetri.fark, 4);
        extractData(telemetri.hiz, 4);
        extractData(telemetri.sic, 4);
        extractData(telemetri.ger, 4);
        extractData(telemetri.lat, 4);
        extractData(telemetri.lon, 4);
        extractData(telemetri.alt, 4);
        extractData(telemetri.pitch, 4);
        extractData(telemetri.roll, 4);
        extractData(telemetri.yaw, 4);
        extractData(telemetri.rhrh, 2);
        extractData(telemetri.iot, 4);
        extractData(telemetri.takNo, 4);
    }


}

//UDP prokolüyle veri atma
void MainWindow::writeData(){

    QByteArray datagram;
    datagram.append('1');
    QHostAddress receiverAddress("192.168.166.139");

    socket2->writeDatagram(datagram,datagram.size(),receiverAddress, 65433);

    //qDebug()<<datagram.size();
}

void MainWindow::rgbData()
{
    QByteArray datagram;
    QDataStream out(&datagram, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << telemetri.rhrh;
    QHostAddress receiverAddress("192.168.166.139");

    socket1->writeDatagram(datagram, receiverAddress, 65432);
  //  qDebug()<<datagram.size()<<","<<datagram;

}

void MainWindow::iotData()
{

    QHostAddress receiverAddress("192.168.166.139");
    QByteArray datagram;
    datagram.resize(sizeof(float));

    // float değeri QByteArray'e yazma
    qToLittleEndian(iot_float, reinterpret_cast<uchar*>(datagram.data()));

    // Datagram boyutunu ve içeriğini kontrol etme
    //qDebug() << datagram.size() << "," << datagram.toHex();

    // Datagram'ı UDP ile gönderme
    socket5->writeDatagram(datagram, receiverAddress, 65434);
}

void MainWindow::baslat_transfer()
{
   /* QByteArray datagram;
    datagram.append('1');
    QHostAddress receiverAddress("192.168.166.139");

    socket6->writeDatagram(datagram,datagram.size(),receiverAddress, 65435);
*/

}

void MainWindow::readIot() {

    QByteArray iot_data = serialPort->readAll();
    QString str = QString(iot_data);

    static QRegularExpression pattern("iot(.*?)end");
    QRegularExpressionMatchIterator matchIterator = pattern.globalMatch(str);

    while (matchIterator.hasNext()) {

        QRegularExpressionMatch match = matchIterator.next();
        QString dynamicValues = match.captured(1).trimmed();
        QStringList parts = dynamicValues.split(",");

        iot_float=parts[0].toFloat();
       // telemetri.iot=iot_float;

    }
        qDebug() << "Received data:" << iot_float;
}

void MainWindow::convertImagesToVideo()
{
    QString program = "ffmpeg";
    QStringList arguments;
    arguments << "-framerate" << "18"
              << "-i" << "C:/Users/aali_/OneDrive/Belgeler/build-GroundStation1_1-Desktop_Qt_5_15_2_MSVC2019_64bit-Release/screenshots/img%d.jpg"
              << "-c:v" << "libx264"
              << "-crf" << "25"
              << "-pix_fmt" << "yuv420p"
              << "C:/Users/aali_/OneDrive/Belgeler/build-GroundStation1_1-Desktop_Qt_5_15_2_MSVC2019_64bit-Release/video/TMUY2024_ 1592571 _VIDEO.mp4";

    QProcess process;
    process.start(program, arguments);
    process.waitForFinished(-1); // Sonsuz süre bekler, süreci tamamlayana kadar

    QString stdError = process.readAllStandardError();

    if (process.exitCode() == 0) {
        qDebug() << "Video başarıyla oluşturuldu.";
    } else {
        qDebug() << "Video oluşturulurken hata oluştu:" << stdError;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
    serialPort(new QSerialPort(this))
{
    ui->setupUi(this);

    // Set up map and 3D views
    ui->harita->setSource(QUrl(QStringLiteral("qrc:/map_new.qml")));
    ui->harita->show();
    ui->threed->setSource(QUrl(QStringLiteral("qrc:/durus_new.qml")));
    ui->threed->show();

    // Connect signals to QML slots
    auto haritaObj = ui->harita->rootObject();
    auto threedObj = ui->threed->rootObject();

    connect(this, SIGNAL(eulerFunction(QVariant, QVariant, QVariant)), threedObj, SLOT(eulerFunction(QVariant, QVariant, QVariant)));
    connect(this, SIGNAL(setCenter(QVariant, QVariant)), haritaObj, SLOT(setCenter(QVariant, QVariant)));
    connect(this, SIGNAL(setLocMarker(QVariant, QVariant)), haritaObj, SLOT(setLocMarker(QVariant, QVariant)));

    emit setCenter(telemetri.lat, telemetri.lon);
    emit setLocMarker(telemetri.lat, telemetri.lon);

    // Set up web engine view
    view = new QWebEngineView(this);
    view->setUrl(QUrl("http://192.168.166.139:5000/"));
    view->setGeometry(2030, 420, 500, 340);
    view->show();

    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readIot);


    // Set up logo
    ui->logo->setPixmap(QPixmap(":/img/TayfLogo.png"));

    // Set up UDP sockets
    auto createSocket = [&](int port) {
        auto socket = new QUdpSocket(this);
        socket->bind(QHostAddress::Any, port);
        return socket;
    };
    socket2 = createSocket(65433);
    socket1 = createSocket(65432);
    socket4 = createSocket(23456);
    socket5 = createSocket(65434);
    socket6 = createSocket(65435);

    // Set up timer
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::timerFunc);

    timer_png = new QTimer(this);
    connect(timer_png, &QTimer::timeout, this, &MainWindow::timerPng);

    // Define scatter style and font for graphs
    QCPScatterStyle scatterStyle(QCPScatterStyle::ssDisc, Qt::white, 2);

    QFont plotFont = font();
    plotFont.setPointSize(8);

    // Common graph setup function
    auto setupGraph = [&](QCustomPlot* plot, const QString& xLabel, const QString& yLabel, const QPen& pen, const QCPScatterStyle& scatterStyle, int xRangeMin, int xRangeMax, int yRangeMin, int yRangeMax, const QString& title) {
        plot->addGraph();
        plot->graph(0)->setScatterStyle(scatterStyle);
        plot->graph(0)->setLineStyle(QCPGraph::lsLine);
        plot->graph(0)->setPen(pen);
        plot->setBackground(QBrush(Qt::transparent));

        plot->xAxis->setLabel(xLabel);
        plot->yAxis->setLabel(yLabel);
        plot->xAxis->setLabelFont(plotFont);
        plot->xAxis->setTickLabelFont(plotFont);
        plot->yAxis->setLabelFont(plotFont);
        plot->yAxis->setTickLabelFont(plotFont);
        plot->xAxis->setTickLabelColor(Qt::white);
        plot->xAxis->setLabelColor(Qt::white);
        plot->xAxis->setSubTickPen(QPen(Qt::white));
        plot->xAxis->setBasePen(QPen(Qt::white));
        plot->yAxis->setTickLabelColor(Qt::white);
        plot->yAxis->setLabelColor(Qt::white);
        plot->yAxis->setSubTickPen(QPen(Qt::white));
        plot->yAxis->setBasePen(QPen(Qt::white));
        plot->xAxis->setRange(xRangeMin, xRangeMax);
        plot->yAxis->setRange(yRangeMin, yRangeMax);

        // Set axis ending arrows
        plot->xAxis->setUpperEnding(QCPLineEnding(QCPLineEnding::esSpikeArrow));
        plot->yAxis->setUpperEnding(QCPLineEnding(QCPLineEnding::esSpikeArrow));

        // Add title using QCPTextElement and center it relative to x-axis
        QCPTextElement *titleElement = new QCPTextElement(plot, title, QFont("sans", 8, QFont::Bold));
        titleElement->setTextColor(Qt::white);

        // Add the title element to the layout
        plot->plotLayout()->insertRow(0); // Insert a new row at the top
        plot->plotLayout()->addElement(0, 0, titleElement); // Add the title element to the new row
        titleElement->setTextFlags(Qt::AlignHCenter); // Center the title horizontally
    };

    // Set up all graphs
    QPen pen(QColor(169, 169, 169), 8);
    //QPen penB(QColor(169, 169, 169), 13);
    setupGraph(ui->bas1p, "Paket Numarası", "Basınç(Pa)", pen, scatterStyle, -55, -5, 90000, 101325, "Basınç1 Grafiği");
    setupGraph(ui->bas2p, "Paket Numarası", "Basınç(Pa)", pen, scatterStyle, -55, 5, 90000, 101325, "Basınç2 Grafiği");
    setupGraph(ui->yuk1p, "Paket Numarası", "Yükseklik(m)", pen, scatterStyle, -55, 5, 0, 750, "Yükseklik1 Grafiği");
    setupGraph(ui->yuk2p, "Paket Numarası", "Yükseklik(m)", pen, scatterStyle, -55, 5, 0, 750, "Yükseklik2 Grafiği");
    setupGraph(ui->sicp, "Paket Numarası", "Sıcaklık(C°)", pen, scatterStyle, -55, 5, 0, 75, "Sıcaklık Grafiği");
    setupGraph(ui->inisp, "Paket Numarası", "İniş Hızı(m/s)", pen, scatterStyle, -55, 5, 0, 20, "İniş Hızı Grafiği");
    setupGraph(ui->farkp, "Paket Numarası", "Fark (m)", pen, scatterStyle, -55, 5, 0, 250, "İrtifa Farkı Grafiği");
    setupGraph(ui->voltp, "Paket Numarası", "Gerilim (Volt)", pen, scatterStyle, -55, 5, 0, 7, "Pil Gerilimi Grafiği");
    setupGraph(ui->nemp, "Paket Numarası", "Nem (Bağıl %)", pen, scatterStyle, 30, 30, 0, 100, "Nem Grafiği");

    // Write CSV header
    QFile file("TMUY2024_ 1592571 _TLM.csv");
    if (file.open(QIODevice::Append | QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << "Pak No,State,Hata Kodu,Saat,Basinc1,Basinc2,Yukseklik1,Yukseklik2,Fark,Hiz,Sicaklik,Gerilim,Enlem,Boylam,GPS Yukseklik,Pitch,Roll,Yaw,RHRH,Nem,Takim No,\n";
        file.close();
    }

    // Play video and set initial UI elements

    QPixmap pix(":/img/yesil.png");
    for (auto label : {ui->kod1, ui->kod2, ui->kod3, ui->kod4, ui->kod5}) {
        label->setPixmap(pix.scaled(35, 35, Qt::KeepAspectRatio));
    }

    // Set up sound
    sound->setMedia(QUrl("qrc:/img/buzzer.mp3"));
    sound->setVolume(5);

    QPixmap sta_0(":/img/ucusa_hazir.png");
    ui->durum->setAlignment(Qt::AlignCenter);
    ui->durum->setPixmap(sta_0.scaled(ui->durum->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

}

MainWindow::~MainWindow()
{
    convertImagesToVideo();
   delete ui;
}
//Ayrıl butonunun fonksiyonunun belirlendiği kısım
void MainWindow::on_ayril_clicked()
{

   writeData();

}


//Başlat butonunun fonksiyonunun belirlendiği kısım
void MainWindow::on_baslat_clicked(bool checked)
{

    if(checked==true){

    timer->start(1000);
    timer_png->start(50);
    baslat_transfer();

    }

 else{

    timer->stop();
    timer_png->stop();


    }

}

void MainWindow::on_rgb_clicked()
{

    telemetri.rhrh = 0;

    QString rhrh_string = ui->rhrh_kod->text();

    // Check second and fourth characters for 'G', 'B', 'R'
    auto setBits = [&](int index, int shift) {
        QChar ch = rhrh_string.at(index);
        if (ch == 'G') telemetri.rhrh |= (0x01 << shift);
        else if (ch == 'B') telemetri.rhrh |= (0x02 << shift);
        else if (ch == 'R') telemetri.rhrh |= (0x04 << shift);
    };

    setBits(1, 0);
    setBits(3, 8);

    // Set the bits for the first and third characters as digits
    telemetri.rhrh |= (rhrh_string.at(0).digitValue() << 3) | (rhrh_string.at(2).digitValue() << 11);

    rgbData();


}




void MainWindow::timerFunc(){


    readData();
    iotData();

    emit setCenter(telemetri.lat, telemetri.lon);
    emit setLocMarker(telemetri.lat, telemetri.lon);
    emit eulerFunction(telemetri.roll, telemetri.yaw, telemetri.pitch);


    //telemetri.pakNo++;
    //telemetri.pitch += 10;
    //telemetri.roll += 10;
    //telemetri.yaw += 10;
    //Update telemetry data and randomize bas1
    //telemetri.bas1 = QRandomGenerator::global()->bounded(100325, 101325);
    telemetri.hataKodu = 1;
    //telemetri.state += 1;
    //telemetri.ger=5;
    //telemetri.yuk1+=5;
    //telemetri.yuk2+=5;

    // Helper function to update the graph

    auto updateGraph = [&](auto graph, int xRangeMin, int xRangeMax, int yRangeMin, int yRangeMax, auto data) {
        if (data != 0) {
        graph->addGraph();
        graph->graph(0)->addData(telemetri.pakNo, data);
        graph->xAxis->setRange(xRangeMin, xRangeMax);
        graph->yAxis->setRange(yRangeMin, yRangeMax);
        graph->replot();
        graph->update();
        }
    };


    // Update all graphs
    updateGraph(ui->bas1p, telemetri.pakNo - 55, telemetri.pakNo + 5, 90000, 101325, telemetri.bas1);
    updateGraph(ui->bas2p, telemetri.pakNo - 55, telemetri.pakNo + 5, 90000, 101325, telemetri.bas2);
    updateGraph(ui->yuk1p, telemetri.pakNo - 55, telemetri.pakNo + 5, 0, 750, telemetri.yuk1);
    updateGraph(ui->yuk2p, telemetri.pakNo - 55, telemetri.pakNo + 5, 0, 750, telemetri.yuk2);
    updateGraph(ui->sicp, telemetri.pakNo - 55, telemetri.pakNo + 5, 20, 75, telemetri.sic);
    updateGraph(ui->inisp, telemetri.pakNo - 55, telemetri.pakNo + 5, 0, 20, telemetri.hiz);
    updateGraph(ui->farkp, telemetri.pakNo - 55, telemetri.pakNo + 5, 0, 250, telemetri.fark);
    updateGraph(ui->voltp, telemetri.pakNo - 55, telemetri.pakNo + 5, 0, 7, telemetri.ger);
    updateGraph(ui->nemp, telemetri.pakNo - 55, telemetri.pakNo + 5, 0, 100, telemetri.iot);

    // Format date and time
    auto formatDateTime = [&](const QString& datetimeStr) {
        return datetimeStr.mid(6, 2) + "/" + datetimeStr.mid(4, 2) + "/" + datetimeStr.mid(0, 4) + " " +
               datetimeStr.mid(8, 2) + ":" + datetimeStr.mid(10, 2) + ":" + datetimeStr.mid(12, 2);
    };

     formattedDateTime = formatDateTime(QString::number(telemetri.zaman));

    // Prepare data string for CSV and list widget
     data = QString::number(telemetri.pakNo) + ", " + QString::number(telemetri.state) + ", " +
                   QString::number(telemetri.hataKodu) + ", " + formattedDateTime + ", " +
                   QString::number(telemetri.bas1) + ", " + QString::number(telemetri.bas2) + ", " +
                   QString::number(telemetri.yuk1) + ", " + QString::number(telemetri.yuk2) + ", " +
                   QString::number(telemetri.fark) + ", " + QString::number(telemetri.hiz) + ", " +
                   QString::number(telemetri.sic) + ", " + QString::number(telemetri.ger) + ", " +
                   QString::number(telemetri.lat) + ", " + QString::number(telemetri.lon) + ", " +
                   QString::number(telemetri.alt) + ", " + QString::number(telemetri.pitch) + ", " +
                   QString::number(telemetri.roll) + ", " + QString::number(telemetri.yaw) + ", " +
                   QString::number(telemetri.rhrh) + ", " + QString::number(telemetri.iot) + ", " +
                   QString::number(telemetri.takNo);

     // Add data to list widget and scroll to bottom
     if (ui->telemetry->count() >= 24) {
         delete ui->telemetry->takeItem(0);  // Remove the oldest item
     }

     ui->telemetry->addItem(data);
     ui->telemetry->scrollToBottom();

    // Update altitude difference
    ui->irtifa->setText("İRTİFA FARKI: " + QString::number(telemetri.fark));

    // Append data to CSV file
    QFile file("TMUY2024_ 1592571 _TLM.csv");
    if (file.open(QIODevice::Append | QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << data + "\n";
        file.close();
    }

    // Update error indicators with pixmaps
    auto updateErrorIndicator = [&](int condition, QLabel* label, const QPixmap& pixmapOn, const QPixmap& pixmapOff) {
        label->setPixmap(((condition) ? pixmapOn : pixmapOff).scaled(35, 35, Qt::KeepAspectRatio));
        if (condition) sound->play();
    };

    QPixmap dur_1(":/img/yesil.png"), dur_2(":/img/kirmizi.png");
    updateErrorIndicator((telemetri.hataKodu & 0x01), ui->kod5, dur_2, dur_1);
    updateErrorIndicator((telemetri.hataKodu & 0x02) >> 1, ui->kod4, dur_2, dur_1);
    updateErrorIndicator((telemetri.hataKodu & 0x04) >> 2, ui->kod3, dur_2, dur_1);
    updateErrorIndicator((telemetri.hataKodu & 0x08) >> 3, ui->kod2, dur_2, dur_1);
    updateErrorIndicator((telemetri.hataKodu & 0x10) >> 4, ui->kod1, dur_2, dur_1);


    // Update error indicators with pixmaps for 5 states
    auto updateSatState = [&](int condition, QLabel* label, const QPixmap& sta_0, const QPixmap& sta_1, const QPixmap& sta_2, const QPixmap& sta_3, const QPixmap& sta_4, const QPixmap& sta_5) {
        label->setPixmap((condition == 0 ? sta_0 :
                              condition == 1 ? sta_1 :
                              condition == 2 ? sta_2 :
                              condition == 3 ? sta_3 :
                              condition == 4 ? sta_4 : sta_5).scaled(435, 340, Qt::KeepAspectRatio));
    };
   ui->durum->setAlignment(Qt::AlignCenter);
    // QPixmap images
    QPixmap sta_0(":/img/ucusa_hazir.png"),
        sta_1(":/img/yukselme.png"),
        sta_2(":/img/model_uydu_inis.png"),
        sta_3(":/img/ayrilma.png"),
        sta_4(":/img/gorev_yuku_inis.png"),
        sta_5(":/img/kurtarma.png");

    // Update labels based on different conditions
    updateSatState((telemetri.state), ui->durum, sta_0, sta_1, sta_2, sta_3, sta_4, sta_5);
    updateSatState((telemetri.state), ui->durum, sta_0, sta_1, sta_2, sta_3, sta_4, sta_5);
    updateSatState((telemetri.state), ui->durum, sta_0, sta_1, sta_2, sta_3, sta_4, sta_5);
    updateSatState((telemetri.state), ui->durum, sta_0, sta_1, sta_2, sta_3, sta_4, sta_5);
    updateSatState((telemetri.state), ui->durum, sta_0, sta_1, sta_2, sta_3, sta_4, sta_5);

    //IoT verisini çekme
    QString portName = ui->iot_port->text().section(" - ", 0, 0);
    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud115200);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->open(QIODevice::ReadWrite);

}


void MainWindow::timerPng()
{
    // Kayıt için klasör oluştur
    QString imageDir = QDir::currentPath() + "/screenshots";
    QDir().mkpath(imageDir);

    // İsimlendirme için sayaç
    static int imageCounter = 1;

    // Widget'tan ekran görüntüsü al
    if (view) {
        // Tüm widget'in ekran görüntüsünü al
        QPixmap pixmap = view->grab();

        // Widget'ten belirli bir bölgeyi kırp
        QRect regionToCapture(0, 0, 500, 340);  // Örneğin, (100, 100) konumundan 300x200 boyutunda bir bölge
        QPixmap croppedPixmap = pixmap.copy(regionToCapture);

        // Kırpılmış bölgeyi kaydet
        QString filePath = imageDir + "/img" + QString::number(imageCounter++) + ".jpg";
        croppedPixmap.save(filePath,"JPG",20);

        // qDebug() << "Widget bölgesel ekran görüntüsü kaydedildi:" << filePath;
    }



}






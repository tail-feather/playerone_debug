#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstring>
#include <sstream>

#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QMessageBox>
#include <QPixmap>

#include "ccdplayerone.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , bWaiting(false)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;

    if ( pCamera ) {
        pCamera->AbortExposure();
    }
    if ( exposureThread.joinable() ) {
        exposureThread.join();
    }
}

void MainWindow::on_pushButtonConnect_clicked()
{
    pCamera = std::make_shared<CcdPlayerOne>();
    if ( ! pCamera->Open(0) ) {
        QMessageBox::critical(this, tr("Connection failed"), tr("Connection failed"));
        return;
    }
    connect(pCamera.get(), &CcdPlayerOne::imageReady, this, &MainWindow::camera_imageReady);
    connect(pCamera.get(), &CcdPlayerOne::aborted, this, &MainWindow::camera_aborted);
    ui->pushButtonConnect->setEnabled(false);
    ui->pushButtonDisconnect->setEnabled(true);
    ui->pushButtonExposure->setEnabled(true);
    ui->pushButtonAbortExposure->setEnabled(true);
}

void MainWindow::on_pushButtonDisconnect_clicked()
{
    if ( pCamera ) {
        pCamera->AbortExposure();
    }
    ui->pushButtonConnect->setEnabled(true);
    ui->pushButtonDisconnect->setEnabled(false);
    ui->pushButtonExposure->setEnabled(false);
    ui->pushButtonAbortExposure->setEnabled(false);
}

void MainWindow::on_pushButtonExposure_clicked()
{
    if ( ! pCamera ) {
        return;
    }

    if ( exposureThread.joinable() ) {
        exposureThread.join();
    }

    std::thread thread([=](){
        bWaiting = true;
        ui->pushButtonExposure->setEnabled(false);
        for (int i = 0; i < 2; ++i) {
            if ( ! pCamera->SetExposure(1 * 1000000) ) {
                QMessageBox::critical(this, tr("Exposure failed"), tr("SetExposure failed."));
                return;
            }
            if ( ! pCamera->SetGain(180) ) {
                QMessageBox::critical(this, tr("Exposure failed"), tr("SetGain failed."));
                return;
            }
            if ( ! pCamera->SetQuality(1) ) {
                QMessageBox::critical(this, tr("Exposure failed"), tr("SetBin failed."));
                return;
            }
            if ( ! pCamera->StartExposure() ) {
                QMessageBox::critical(this, tr("Exposure failed"), tr("StartExposure failed."));
                return;
            }
            while ( bWaiting ) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        ui->pushButtonExposure->setEnabled(true);
    });
    exposureThread.swap(thread);
}

void MainWindow::on_pushButtonAbortExposure_clicked()
{
    if ( ! pCamera ) {
        return;
    }
    pCamera->AbortExposure();
    bWaiting = false;
    ui->pushButtonExposure->setEnabled(true);
}

void MainWindow::camera_imageReady(int nWidth, int nHeight, const std::vector<unsigned char> &image)
{
    // build PGM
    std::ostringstream oss;
    oss << "P5 " << nWidth << " " << nHeight << " " << 0xff << "\n";
    const auto header = oss.str();
    std::vector<unsigned char> buffer(header.size() + image.size());
    std::memcpy(&buffer[0], header.c_str(), header.size());
    std::memcpy(&buffer[header.size()], image.data(), image.size());

    // show image
    QPixmap pixmap;
    pixmap.loadFromData(buffer.data(), buffer.size(), "PGM");

    auto scene = new QGraphicsScene();
    QGraphicsPixmapItem *image_item = new QGraphicsPixmapItem(pixmap);
    scene->addItem(image_item);
    ui->graphicsView->setScene(scene);

    QMessageBox::information(this, tr("Done"), tr("captured."));
    bWaiting = false;
}

void MainWindow::camera_aborted()
{
    QMessageBox::warning(this, tr("Aborted"), tr("aborted."));
}

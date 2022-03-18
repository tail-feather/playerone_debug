#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <thread>

#include <QMainWindow>

class CcdPlayerOne;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void done();

public slots:
    void on_pushButtonConnect_clicked();
    void on_pushButtonDisconnect_clicked();
    void on_pushButtonExposure_clicked();
    void on_pushButtonAbortExposure_clicked();
    void camera_imageReady(int nWidth, int nHeight, const std::vector<unsigned char> &image);
    void camera_aborted();
    void exposure_done();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<CcdPlayerOne> pCamera;
    std::thread exposureThread;
    bool bWaiting;
};
#endif // MAINWINDOW_H

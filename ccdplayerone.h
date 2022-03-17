#ifndef CCDPLAYERONE_H
#define CCDPLAYERONE_H

#include <QObject>

#include <memory>
#include <mutex>
#include <optional>
#include <thread>


class PlayerOneCamera;

class CcdPlayerOne : public QObject
{
    Q_OBJECT
public:
    CcdPlayerOne();

    bool Open(int nNo);
    void Close();

    std::string GetDeviceName() const;

    std::tuple<long, long> GetMaxSize() const;

    bool StartExposure();
    bool IsShooting() const;
    bool AbortExposure();
    bool EndExposure();

    std::optional<double> GetExposureSec() const;
    std::optional<long> GetExposure() const;
    bool SetExposure(long nDependValue);
    std::tuple<long, long, long> GetExposureDef() const;

    std::optional<long> GetGain() const;
    bool SetGain(long nDependValue);
    std::tuple<long, long, long> GetGainDef() const;

    std::optional<long> GetQuality() const;
    bool SetQuality(long nDependValue);

    long GetBufferSize() const;

private:
    std::shared_ptr<PlayerOneCamera> pCamera;

    bool bDisconnectedSent;

    long m_nCurrentExposureCache;
    long m_nCurrentBufferSize;
    std::mutex mtxWaiting;
    std::thread imageWaitingThread;
    std::thread bulbThread;
    bool bAbortBulb;
    bool bImageWaiting;

signals:
    void imageReady(int nWidth, int nHeight, const std::vector<unsigned char> &buffer);
    void aborted();
};

#endif // CCDPLAYERONE_H

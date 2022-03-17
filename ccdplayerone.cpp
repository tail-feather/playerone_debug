#include "ccdplayerone.h"

#include <chrono>
#include <map>
#include <vector>

#include "PlayerOneCamera.h"

#include "logging.hpp"


class PlayerOneCamera {
public:
    PlayerOneCamera(const POACameraProperties &prop, const std::map<POAConfig, POAConfigAttributes> &attrib, const std::shared_ptr<logging::Logger> &pLogger)
        : m_CamProp(prop)
        , m_Attrib(attrib)
        , m_pLogger(pLogger)
    {}
    ~PlayerOneCamera()
    {}

    static std::vector<std::string> CameraList() {
        const auto nCount = POAGetCameraCount();
        if (nCount <= 0) {
            // No camera detected.
            return std::vector<std::string>();
        }

        // build camera list
        std::vector<std::string> result;
        result.reserve(nCount);
        for (int i = 0; i < nCount; ++i) {
            POACameraProperties prop = {};
            if (POAGetCameraProperties(i, &prop) != POA_OK) {
                // something wrong...
                result.push_back(std::string());
                continue;
            }
            result.push_back(prop.cameraModelName);
        }
        return result;
    }

    static std::shared_ptr<PlayerOneCamera> Open(int nIndex) {
        LOGGING_GLOBAL_DECL();
        LOGGING_GLOBAL_LOG_CREATE("gbxccd_playerone.log");

        const auto nCameraCount = POAGetCameraCount();
        if (nIndex < 0 || nIndex >= nCameraCount) {
            // Out of range
            LOGGING_GLOBAL_LOG_ERROR("Out of range: Count: ", nCameraCount, "At: ", nIndex);
            return nullptr;
        }

        POAErrors nErr = POAErrors::POA_OK;
        POACameraProperties prop = {};
        if ( (nErr = POAGetCameraProperties(nIndex, &prop)) != POA_OK ) {
            LOGGING_GLOBAL_LOG_ERROR("GetCameraProperties failed: ", nErr);
            return nullptr;
        }

        if ( (nErr = POAOpenCamera(prop.cameraID)) != POA_OK ) {
            LOGGING_GLOBAL_LOG_ERROR("OpenCamera failed: ", nErr);
            return nullptr;
        }

        if ( (nErr = POAInitCamera(prop.cameraID)) != POA_OK ) {
            LOGGING_GLOBAL_LOG_ERROR("InitCamera failed: ", nErr);
            return nullptr;
        }

        return Create(prop);
    }

    POACameraProperties m_CamProp;
    std::map<POAConfig, POAConfigAttributes> m_Attrib;
    LOGGING_DECL();

protected:
    static std::shared_ptr<PlayerOneCamera> Create(const POACameraProperties &prop) {
        std::shared_ptr<logging::Logger> pLogger;
        LOGGING_CREATE_IMPL(pLogger, std::string("gbxccd_playerone_") + prop.cameraModelName + ".log");

        std::map<POAConfig, POAConfigAttributes> attributes;
        POAErrors nErr = POAErrors::POA_OK;
        int nAttribCount = 0;
        if ( (nErr = POAGetConfigsCount(prop.cameraID, &nAttribCount)) != POA_OK ) {
            LOGGING_ERROR0(pLogger, "GetConfigsCount failed: ", nErr);
            return nullptr;
        }
        LOGGING_INFO0(pLogger, "ConfigsCount: ", nAttribCount);
        for (int i = 0; i < nAttribCount; ++i) {
            POAConfigAttributes attrib;
            if ( (nErr = POAGetConfigAttributes(prop.cameraID, i, &attrib)) == POA_OK ) {
                LOGGING_INFO0(pLogger, "GetConfigAttributes[", i, "]:");
                LOGGING_INFO0(pLogger, "  ID: ", attrib.configID);
                LOGGING_INFO0(pLogger, "  IsSupportAuto: ", attrib.isSupportAuto);
                LOGGING_INFO0(pLogger, "  IsWritable: ", attrib.isWritable);
                LOGGING_INFO0(pLogger, "  IsReadable: ", attrib.isReadable);
                LOGGING_INFO0(pLogger, "  ValueType: ", attrib.valueType);
                if ( attrib.valueType == POAValueType::VAL_BOOL ) {
                    LOGGING_INFO0(pLogger, "  MinValue: ", attrib.minValue.boolValue);
                    LOGGING_INFO0(pLogger, "  MaxValue: ", attrib.maxValue.boolValue);
                    LOGGING_INFO0(pLogger, "  DefaultValue: ", attrib.defaultValue.boolValue);
                } else if ( attrib.valueType == POAValueType::VAL_FLOAT ) {
                    LOGGING_INFO0(pLogger, "  MinValue: ", attrib.minValue.floatValue);
                    LOGGING_INFO0(pLogger, "  MaxValue: ", attrib.maxValue.floatValue);
                    LOGGING_INFO0(pLogger, "  DefaultValue: ", attrib.defaultValue.floatValue);
                } else if ( attrib.valueType == POAValueType::VAL_INT ) {
                    LOGGING_INFO0(pLogger, "  MinValue: ", attrib.minValue.intValue);
                    LOGGING_INFO0(pLogger, "  MaxValue: ", attrib.maxValue.intValue);
                    LOGGING_INFO0(pLogger, "  DefaultValue: ", attrib.defaultValue.intValue);
                }
                attributes.emplace(attrib.configID, attrib);
            }
            else {
                LOGGING_ERROR0(pLogger, "GetConfigAttributes failed [", i, "]: ", nErr);
            }
        }

        return std::make_shared<PlayerOneCamera>(prop, attributes, pLogger);
    }

public:
    auto cameraID() const -> decltype(POACameraProperties::cameraID) {
        return m_CamProp.cameraID;
    }
    void Close() {
        LOGGING_INFO("CloseCamera");
        POACloseCamera(cameraID());
    }
    std::tuple<int, int> GetMaxImageSize() const {
        return std::make_tuple(m_CamProp.maxWidth, m_CamProp.maxHeight);
    }
    std::optional<std::tuple<long, long, long>> GetExposureRange() const {
        return GetIntRange(POAConfig::POA_EXPOSURE);
    }
    std::optional<std::tuple<long, long, long>> GetGainRange() const {
        return GetIntRange(POAConfig::POA_GAIN);
    }

    std::string GetDeviceName() const {
        return m_CamProp.cameraModelName;
    }

    long GetCurrentExposure() {
        auto exp = GetExposure();
        if ( ! exp ) {
            const auto it = m_Attrib.find(POAConfig::POA_EXPOSURE);
            if ( it == m_Attrib.end() ) {
                return -1;
            }
            return it->second.defaultValue.intValue;
        }
        return *exp;
    }
    std::optional<long> GetExposure() {
        auto ret = GetIntConfig(POAConfig::POA_EXPOSURE);
        if ( ! ret ) {
            LOGGING_ERROR("GetExposure failed.");
            return std::nullopt;
        }
        return std::get<0>(*ret);
    }
    bool SetExposure(long nExposure) {
        const auto bRet = SetIntConfig(POAConfig::POA_EXPOSURE, nExposure, POABool::POA_FALSE);
        if ( ! bRet ) {
            LOGGING_ERROR("SetExposure failed.");
        }
        return bRet;
    }

    long GetCurrentGain() {
        auto gain = GetGain();
        if ( ! gain ) {
            const auto it = m_Attrib.find(POAConfig::POA_GAIN);
            if ( it == m_Attrib.end() ) {
                return -1;
            }
            return it->second.defaultValue.intValue;
        }
        return *gain;
    }
    std::optional<long> GetGain() {
        auto ret = GetIntConfig(POAConfig::POA_GAIN);
        if ( ! ret ) {
            LOGGING_ERROR("GetGain failed.");
            return std::nullopt;
        }
        return std::get<0>(*ret);
    }
    bool SetGain(long nGain) {
        const auto bRet = SetIntConfig(POAConfig::POA_GAIN, nGain, POABool::POA_FALSE);
        if ( ! bRet ) {
            LOGGING_ERROR("SetGain failed.");
        }
        return bRet;
    }

    std::optional<std::tuple<int, int>> GetImageSize() {
        int nWidth = 0, nHeight = 0;
        const auto nErr = POAGetImageSize(cameraID(), &nWidth, &nHeight);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("GetImageSize failed. code:", nErr);
            return std::nullopt;
        }
        LOGGING_INFO("GetImageSize: width:", nWidth, ", height:", nHeight);
        return std::make_tuple(nWidth, nHeight);
    }

    bool SetImageSize(int nWidth, int nHeight) {
        LOGGING_INFO("SetImageSize: width:", nWidth, " height:", nHeight);
        const auto nErr = POASetImageSize(cameraID(), nWidth, nHeight);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("SetImageSize failed. code:", nErr);
            return false;
        }
        return true;
    }

    std::optional<POAImgFormat> GetImageFormat() {
        POAImgFormat fmt;
        const auto nErr = POAGetImageFormat(cameraID(), &fmt);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("GetImageFormat failed. code:", nErr);
            return std::nullopt;
        }
        LOGGING_INFO("GetImageFormat: ", fmt);
        return fmt;
    }

    std::optional<bool> ImageReady() {
        POABool isReady = POABool::POA_FALSE;
        const auto nErr = POAImageReady(cameraID(), &isReady);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("ImageReady failed. code:", nErr);
            return std::nullopt;
        }
        LOGGING_INFO("ImageReady: ", isReady);
        return isReady == POABool::POA_TRUE;
    }

    std::optional<POACameraState> GetCameraState() {
        POACameraState state;
        const auto nErr = POAGetCameraState(cameraID(), &state);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("GetCameraState failed. code:", nErr);
            return std::nullopt;
        }
        LOGGING_INFO("GetCameraState: ", state);
        return state;
    }

    bool GetImageData(std::vector<unsigned char>& buffer, int timeout_ms = -1) {
        LOGGING_INFO("GetImageData: ", buffer.size(), "bytes timeout: ", timeout_ms);
        const auto nErr = POAGetImageData(cameraID(), buffer.data(), buffer.size(), timeout_ms);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("GetImageData failed. code:", nErr);
        }
        LOGGING_INFO("OK");
        return nErr == POAErrors::POA_OK;
    }

    bool StartExposure() {
        const auto nErr = POAStartExposure(cameraID(), POABool::POA_TRUE);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("StartExposure failed. code:", nErr);
            return false;
        }
        LOGGING_INFO("StartExposure");
        return true;
    }
    bool StartLiveView() {
        const auto nErr = POAStartExposure(cameraID(), POABool::POA_FALSE);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("StartLiveView failed. code:", nErr);
            return false;
        }
        LOGGING_INFO("StartLiveView");
        return true;
    }

    bool StopExposure() {
        const auto nErr = POAStopExposure(cameraID());
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("StopExposure failed. code:", nErr);
        }
        LOGGING_INFO("StopExposure");
        return nErr == POAErrors::POA_OK;
    }

    std::optional<int> GetImageBin() {
        int nBin = 0;
        const auto nErr = POAGetImageBin(cameraID(), &nBin);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("GetImageBin failed. code:", nErr);
            return std::nullopt;
        }
        LOGGING_INFO("GetImageBin: ", nBin);
        return nBin;
    }

    bool SetImageBin(int nBin) {
        LOGGING_INFO("SetImageBin: ", nBin);
        const auto nErr = POASetImageBin(cameraID(), nBin);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("SetImageBin failed. code:", nErr);
            return false;
        }
        return true;
    }

    std::optional<std::tuple<int, int>> GetImageStartPos() {
        int nStartX, nStartY;
        const auto nErr = POAGetImageStartPos(cameraID(), &nStartX, &nStartY);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("GetImageStartPos failed. code:", nErr);
            return std::nullopt;
        }
        LOGGING_INFO("GetImageStartPos: x:", nStartX, "y:", nStartY);
        return std::make_tuple(nStartX, nStartY);
    }

    bool SetImageStartPos(int nStartX, int nStartY) {
        LOGGING_INFO("SetImageStartPos: x:", nStartX, " y:", nStartY);
        const auto nErr = POASetImageStartPos(cameraID(), nStartX, nStartY);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("SetImageStartPos failed. code:", nErr);
            return false;
        }
        return true;
    }

protected:
    std::optional<std::tuple<long, POABool>> GetIntConfig(POAConfig config) {
        POAConfigValue value;
        POABool isAuto;
        POAErrors nErr;
        if ( (nErr = POAGetConfig(cameraID(), config, &value, &isAuto)) != POAErrors::POA_OK ) {
            LOGGING_ERROR("GetConfig (int) failed: ", nErr);
            return std::nullopt;
        }
        LOGGING_INFO("GetConfig: id:", config, " value:", value.intValue, " isAuto:", isAuto);
        return std::make_tuple(value.intValue, isAuto);
    }
    bool SetIntConfig(POAConfig config, long nValue, POABool isAuto) {
        POAConfigValue value;
        value.intValue = nValue;
        LOGGING_INFO("SetConfig: id:", config, " value:", nValue, " isAuto:", isAuto);
        const auto nErr = POASetConfig(cameraID(), config, value, isAuto);
        if ( nErr != POAErrors::POA_OK ) {
            LOGGING_ERROR("SetConfig (int) failed: ", nErr);
        }
        LOGGING_INFO("OK");
        return nErr == POAErrors::POA_OK;
    }

    // (min, max, default)
    std::optional<std::tuple<long, long, long>> GetIntRange(POAConfig configID) const {
        const auto it = m_Attrib.find(configID);
        if ( it == m_Attrib.end() ) {
            return std::nullopt;
        }
        if ( it->second.valueType != POAValueType::VAL_INT) {
            return std::nullopt;
        }
        return std::make_tuple(it->second.minValue.intValue, it->second.maxValue.intValue, it->second.defaultValue.intValue);
    }
    std::optional<std::tuple<double, double, double>> GetFloatRange(POAConfig configID) const {
        const auto it = m_Attrib.find(configID);
        if ( it == m_Attrib.end() ) {
            return std::nullopt;
        }
        if ( it->second.valueType != POAValueType::VAL_FLOAT ) {
            return std::nullopt;
        }
        return std::make_tuple(it->second.minValue.floatValue, it->second.maxValue.floatValue, it->second.defaultValue.floatValue);
    }

};


CcdPlayerOne::CcdPlayerOne()
    : pCamera()
    , m_nCurrentExposureCache(0)
    , m_nCurrentBufferSize(0)
    , mtxWaiting()
    , imageWaitingThread()
    , bAbortBulb(false)
    , bImageWaiting(false)
{}

bool CcdPlayerOne::Open(int nNo) {
    const auto aCameras = PlayerOneCamera::CameraList();
    if ( static_cast<int>(aCameras.size()) <= nNo ) {
        return false;
    }
    pCamera = PlayerOneCamera::Open(nNo);
    if ( ! pCamera ) {
        return false;
    }

    return true;
}
void CcdPlayerOne::Close() {
    if ( imageWaitingThread.joinable() ) {
        // terminate downloading thread
        imageWaitingThread.join();
    }
    bAbortBulb = true;
    if ( bulbThread.joinable() ) {
        bulbThread.join();
    }
    if ( ! pCamera ) {
        return;
    }
    pCamera->Close();
    pCamera = nullptr;
}

std::string CcdPlayerOne::GetDeviceName() const {
    if ( ! pCamera ) {
        return std::string();
    }
    return pCamera->m_CamProp.cameraModelName;
}

std::tuple<long, long> CcdPlayerOne::GetMaxSize() const {
    if ( ! pCamera ) {
        return std::make_tuple(0, 0);
    }
    return std::make_tuple(
        pCamera->m_CamProp.maxWidth,
        pCamera->m_CamProp.maxHeight
    );
}

// (bits, bytes)
inline std::tuple<int, int> PlayerOneImgFormatSize(POAImgFormat fmt) {
    switch (fmt) {
    case POAImgFormat::POA_RAW8:
    case POAImgFormat::POA_MONO8:
        return std::make_tuple(8, 1);
    case POAImgFormat::POA_RAW16:
        return std::make_tuple(16, 2);
    case POAImgFormat::POA_RGB24:
        return std::make_tuple(24, 3);
    case POAImgFormat::POA_END:
        assert(false);
        break;
    }
#ifdef _DEBUG
    assert(false);
#endif  // _DEBUG
    return std::make_tuple(1, 1);
}

bool CcdPlayerOne::StartExposure() {
    if ( ! pCamera ) {
        return false;
    }
    if ( imageWaitingThread.joinable() ) {
        // terminate previous downloading thread
        imageWaitingThread.join();
    }

    AbortExposure();

    // check state (see device log)
    const auto state = pCamera->GetCameraState();
    Q_UNUSED(state);

    bAbortBulb = false;
    const auto imageSize = pCamera->GetImageSize();
    if ( ! imageSize ) {
        return false;
    }
    const auto nWidth = std::get<0>(*imageSize);
    const auto nHeight = std::get<1>(*imageSize);

    const auto imageFormat = pCamera->GetImageFormat();
    if ( ! imageFormat ) {
        return false;
    }
    POAImgFormat fmt = *imageFormat;

    const auto [nBitpp, nBytepp] = PlayerOneImgFormatSize(fmt);
    const bool bColorCam = pCamera->m_CamProp.isColorCamera == POABool::POA_TRUE;
    Q_UNUSED(bColorCam);
    const auto optExposureTime = GetExposure();
    const auto optGain = GetGain();
    const auto optQuality = GetQuality();
    if ( ! optExposureTime || ! optGain || ! optQuality ) {
        return false;
    }
    const auto nExposureTime = *optExposureTime;
    Q_UNUSED(nExposureTime);
    const auto nGain = *optGain;
    Q_UNUSED(nGain);
    const auto nQuality = *optQuality;
    Q_UNUSED(nQuality);
    m_nCurrentBufferSize = nWidth * nHeight * nBytepp;

    auto abortProc = [=]() {
        pCamera->StopExposure();
        emit aborted();
    };
    std::thread thread([=]() {
        if (!pCamera->StartExposure()) {
            abortProc();
            return;
        }

        POACameraState state;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if ( bAbortBulb ) {
                break;
            }
            const auto result = pCamera->GetCameraState();
            if ( ! result ) {
                bAbortBulb = true;
            }
            state = *result;
        } while (state == POACameraState::STATE_EXPOSING);
        if ( bAbortBulb ) {
            abortProc();
            return;
        }
        const auto ret = pCamera->ImageReady();
        if ( ! ret || ! *ret ) {
            abortProc();
            return;
        }

        // TODO:
        const auto nSize = m_nCurrentBufferSize;
        std::vector<unsigned char> buffer(nSize);
        if ( ! pCamera->GetImageData(buffer, m_nCurrentExposureCache / 1000 + 500) ) {
            abortProc();
            return;
        }

        emit imageReady(nWidth, nHeight, buffer);
    });
    imageWaitingThread.swap(thread);
    return true;
}

bool CcdPlayerOne::AbortExposure() {
    bAbortBulb = true;
    return EndExposure();
}
bool CcdPlayerOne::EndExposure() {
    if (!pCamera) {
        return false;
    }
    pCamera->StopExposure();
    if ( imageWaitingThread.joinable() ) {
        imageWaitingThread.join();
    }
    return true;
}

std::optional<double> CcdPlayerOne::GetExposureSec() const {
    const auto value = GetExposure();
    if ( ! value ) {
        return std::nullopt;
    }
    return *value / static_cast<double>(1000000);
}
std::optional<long> CcdPlayerOne::GetExposure() const {
    if ( ! pCamera ) {
        return std::nullopt;
    }
    auto ret = pCamera->GetExposure();
    if ( ! ret ) {
        return std::nullopt;
    }
    return *ret;
}
bool CcdPlayerOne::SetExposure(long nDependValue) {
    if ( ! pCamera ) {
        return false;
    }
    bool bRet = pCamera->SetExposure(nDependValue);
    if ( ! bRet ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bRet = pCamera->SetExposure(nDependValue);
    }
    if ( bRet ) {
        m_nCurrentExposureCache = nDependValue;
    }

    return bRet;
}
std::tuple<long, long, long> CcdPlayerOne::GetExposureDef() const {
    if ( ! pCamera ) {
        return std::make_tuple(0, 0, 0);
    }
    if (const auto rng = pCamera->GetExposureRange()) {
        return *rng;
    }
    return std::make_tuple(0, 0, 0);
}

std::optional<long> CcdPlayerOne::GetGain() const {
    if ( ! pCamera ) {
        return std::nullopt;
    }
    return pCamera->GetGain();
}
bool CcdPlayerOne::SetGain(long nDependValue) {
    if ( ! pCamera ) {
        return false;
    }
    if (imageWaitingThread.joinable()) {
        // terminate previous downloading thread
        imageWaitingThread.join();
    }
    return pCamera->SetGain(nDependValue);
}
std::tuple<long, long, long> CcdPlayerOne::GetGainDef() const {
    if ( ! pCamera ) {
        return std::make_tuple(0, 0, 0);
    }
    if (const auto rng = pCamera->GetGainRange()) {
        return *rng;
    }
    return std::make_tuple(0, 0, 0);
}

std::optional<long> CcdPlayerOne::GetQuality() const {
    if ( ! pCamera ) {
        return std::nullopt;
    }
    const auto imageBin = pCamera->GetImageBin();
    if ( ! imageBin ) {
        return std::nullopt;
    }
    return *imageBin;
}
bool CcdPlayerOne::SetQuality(long nDependValue) {
    if ( ! pCamera ) {
        return false;
    }
    if (imageWaitingThread.joinable()) {
        // terminate previous downloading thread
        imageWaitingThread.join();
    }
    const auto nBin = nDependValue & 0xff;
    if ( ! pCamera->SetImageBin(nBin) ) {
        return false;
    }
    return true;
}

long CcdPlayerOne::GetBufferSize() const {
    return m_nCurrentBufferSize;
}

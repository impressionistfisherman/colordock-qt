#include "WasapiCapture.h"
#include <cmath>
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

static const GUID CD_SUBTYPE_IEEE_FLOAT =
    {0x00000003, 0x0000, 0x0010, {0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}};

static inline bool guidEq(const GUID &a, const GUID &b) {
    return memcmp(&a, &b, sizeof(GUID)) == 0;
}

#define SAFE_RELEASE(p) do { if (p) { (p)->Release(); (p)=nullptr; } } while(0)
#endif

WasapiCapture::WasapiCapture(QObject *parent) : QThread(parent) {}

WasapiCapture::~WasapiCapture()
{
    stopCapture();
    wait(3000);
}

bool WasapiCapture::startCapture()
{
    if (m_running.load()) return true;
    {
        QMutexLocker lk(&m_bufMutex);
        m_sampleBuf.clear();
        m_bufOffset = 0;
    }
    m_running = true;
    start(QThread::HighPriority);
    return true;
}

void WasapiCapture::stopCapture()
{
    m_running = false;
}

void WasapiCapture::run()
{
#ifdef Q_OS_WIN
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // RAII 래퍼 — goto 대신 람다로 goto 범위 제한 (C++ UB 회피)
    auto doCapture = [this]() -> QString {
        IMMDeviceEnumerator *enumerator = nullptr;
        IMMDevice           *device     = nullptr;
        IAudioClient        *client     = nullptr;
        IAudioCaptureClient *capture    = nullptr;
        WAVEFORMATEX        *format     = nullptr;

        auto cleanup = [&]() {
            CoTaskMemFree(format);
            SAFE_RELEASE(capture);
            SAFE_RELEASE(client);
            SAFE_RELEASE(device);
            SAFE_RELEASE(enumerator);
        };

        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
        if (FAILED(hr)) { cleanup(); return "COM: MMDeviceEnumerator 생성 실패"; }

        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        if (FAILED(hr)) { cleanup(); return "기본 오디오 출력 장치를 찾을 수 없습니다."; }

        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(&client));
        if (FAILED(hr)) { cleanup(); return "IAudioClient 활성화 실패"; }

        hr = client->GetMixFormat(&format);
        if (FAILED(hr)) { cleanup(); return "오디오 포맷 조회 실패"; }

        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_LOOPBACK,
                                 10000000, 0, format, nullptr);
        if (FAILED(hr)) { cleanup(); return "IAudioClient 초기화 실패"; }

        hr = client->GetService(__uuidof(IAudioCaptureClient),
                                 reinterpret_cast<void**>(&capture));
        if (FAILED(hr)) { cleanup(); return "IAudioCaptureClient 가져오기 실패"; }

        int channels   = static_cast<int>(format->nChannels);
        int sampleRate = static_cast<int>(format->nSamplesPerSec);

        bool isFloat = (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
        if (!isFloat && format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            auto *ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format);
            isFloat = guidEq(ext->SubFormat, CD_SUBTYPE_IEEE_FLOAT);
        }
        if (!isFloat) { cleanup(); return "float32 포맷이 아닙니다 — 데모 모드로 전환합니다."; }

        client->Start();
        while (m_running.load()) {
            UINT32 packetSize = 0;
            hr = capture->GetNextPacketSize(&packetSize);
            if (FAILED(hr)) break;
            if (packetSize == 0) { QThread::msleep(8); continue; }

            BYTE  *data   = nullptr;
            UINT32 frames = 0;
            DWORD  flags  = 0;
            hr = capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            if (data && frames > 0 && !(flags & AUDCLNT_BUFFERFLAGS_SILENT))
                processBuffer(reinterpret_cast<const float*>(data),
                              static_cast<int>(frames), channels, sampleRate);
            capture->ReleaseBuffer(frames);
        }
        client->Stop();
        cleanup();
        return {};
    };

    QString err = doCapture();
    if (!err.isEmpty()) emit captureError(err);

    CoUninitialize();
#else
    emit captureError("Windows 전용 기능입니다.");
#endif
    m_running = false;
}

void WasapiCapture::fft(std::vector<std::complex<float>> &x)
{
    int N = static_cast<int>(x.size());
    for (int i = 1, j = 0; i < N; ++i) {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(x[i], x[j]);
    }
    for (int len = 2; len <= N; len <<= 1) {
        float ang = -2.0f * static_cast<float>(M_PI) / len;
        std::complex<float> wlen(cosf(ang), sinf(ang));
        for (int i = 0; i < N; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int j = 0; j < len/2; ++j) {
                auto u = x[i+j], v = x[i+j+len/2] * w;
                x[i+j] = u + v; x[i+j+len/2] = u - v;
                w *= wlen;
            }
        }
    }
}

void WasapiCapture::processBuffer(const float *data, int frames,
                                   int channels, int sampleRate)
{
    {
        QMutexLocker lk(&m_bufMutex);
        for (int i = 0; i < frames; ++i) {
            float mono = 0;
            for (int ch = 0; ch < channels; ++ch)
                mono += data[i * channels + ch];
            m_sampleBuf.append(mono / channels);
        }
    }

    // 버퍼 크기 확인 (뮤텍스 없이 읽어도 worst-case 안전)
    while (m_sampleBuf.size() - m_bufOffset >= FFT_SIZE) {
        std::vector<std::complex<float>> buf(FFT_SIZE);
        {
            QMutexLocker lk(&m_bufMutex);
            for (int i = 0; i < FFT_SIZE; ++i) {
                float w = 0.5f * (1.0f - cosf(2.0f * static_cast<float>(M_PI) * i / (FFT_SIZE-1)));
                buf[i] = {m_sampleBuf[m_bufOffset + i] * w, 0.0f};
            }
            m_bufOffset += FFT_SIZE / 2; // 50% 오버랩

            // 버퍼가 너무 커지면 정리 (4096 이상 누적 시)
            if (m_bufOffset > 4096) {
                m_sampleBuf.erase(m_sampleBuf.begin(),
                                   m_sampleBuf.begin() + m_bufOffset);
                m_bufOffset = 0;
            }
        }

        fft(buf);

        QList<double> bars;
        bars.reserve(NUM_BARS);
        for (int b = 0; b < NUM_BARS; ++b) {
            double fLow  = 20.0 * std::pow(1000.0, static_cast<double>(b)   / NUM_BARS);
            double fHigh = 20.0 * std::pow(1000.0, static_cast<double>(b+1) / NUM_BARS);
            int kLow  = std::max(1,          static_cast<int>(fLow  * FFT_SIZE / sampleRate));
            int kHigh = std::min(FFT_SIZE/2, static_cast<int>(fHigh * FFT_SIZE / sampleRate)+1);

            double peak = 0;
            for (int k = kLow; k < kHigh; ++k) {
                double mag = std::abs(buf[k]) / (FFT_SIZE / 2.0);
                if (mag > peak) peak = mag;
            }
            double db   = 20.0 * std::log10(peak + 1e-7);
            double norm = std::max(0.0, std::min(1.0, (db + 60.0) / 60.0));
            bars.append(norm);
        }
        emit spectrumReady(bars);
    }
}

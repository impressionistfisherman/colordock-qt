#pragma once
#include <QThread>
#include <QList>
#include <QMutex>
#include <atomic>
#include <vector>
#include <complex>

class WasapiCapture : public QThread {
    Q_OBJECT
public:
    explicit WasapiCapture(QObject *parent = nullptr);
    ~WasapiCapture() override;

    bool startCapture();
    void stopCapture();
    bool isCapturing() const { return m_running.load(); }

signals:
    void spectrumReady(const QList<double> &bars);
    void captureError(const QString &msg);

protected:
    void run() override;

private:
    void processBuffer(const float *data, int frames, int channels, int sampleRate);
    static void fft(std::vector<std::complex<float>> &x);

    std::atomic<bool> m_running{false};
    QList<float>      m_sampleBuf;
    QMutex            m_bufMutex;
    int               m_bufOffset = 0; // mid() 대신 오프셋 방식

    static const int FFT_SIZE = 1024;
    static const int NUM_BARS = 32;
};

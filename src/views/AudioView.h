#pragma once
#include <QWidget>
#include <QTimer>
#include <QList>

class QPushButton;
class QLabel;
class QSlider;
class WasapiCapture;
class BarVisWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarVisWidget(QWidget *parent = nullptr);
    void setBars(const QList<double> &bars);
    void setDemoMode(bool demo) { m_demo = demo; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QList<double> m_bars;
    bool          m_demo = false;
    double        m_phase = 0.0;
};

class AudioView : public QWidget {
    Q_OBJECT
public:
    explicit AudioView(QWidget *parent = nullptr);
    ~AudioView() override;

private slots:
    void onToggleCapture();
    void onSpectrumReady(const QList<double> &bars);
    void onCaptureError(const QString &msg);
    void onDemoFrame();

private:
    void setupUi();

    WasapiCapture *m_wasapi    = nullptr;
    QTimer        *m_demoTimer = nullptr;

    BarVisWidget  *m_vis;
    QPushButton   *m_captureBtn;
    QLabel        *m_statusLabel;
    QSlider       *m_sensitivitySlider;
    QLabel        *m_sensLabel;
    QLabel        *m_bpmLabel;

    bool   m_capturing   = false;
    double m_sensitivity = 1.0;
    double m_demoPhase   = 0.0;

    // BPM 감지용
    QList<double> m_bassHistory;
    QList<qint64> m_beatTimes;
};

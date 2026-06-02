#include "AudioView.h"
#include "../utils/WasapiCapture.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QDateTime>
#include <cmath>

// ── BarVisWidget ─────────────────────────────────────────────
BarVisWidget::BarVisWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(140);
    m_bars.resize(32, 0.0);
}

void BarVisWidget::setBars(const QList<double> &bars)
{
    // 피크 홀드를 위해 기존값과 max 취함 (자연스러운 decay)
    for (int i = 0; i < m_bars.size() && i < bars.size(); ++i)
        m_bars[i] = std::max(m_bars[i] * 0.85, bars[i]);
    update();
}

void BarVisWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = width(), H = height();
    const int n = m_bars.size();
    if (n == 0) return;

    const double barW = (W - 4.0) / n;
    const double gap  = std::max(1.0, barW * 0.18);

    p.fillRect(rect(), QColor(8, 10, 20));

    // 그리드
    p.setPen(QPen(QColor(255,255,255,12), 1));
    for (int i = 1; i < 4; ++i) p.drawLine(0, H*i/4, W, H*i/4);

    // 막대
    for (int i = 0; i < n; ++i) {
        double v  = std::max(0.0, std::min(1.0, m_bars[i]));
        double bH = v * (H - 4.0);
        double x  = 2.0 + i * barW;
        double y  = H - 2.0 - bH;

        double hue = std::fmod(280.0 - i * 240.0 / n, 360.0);
        QColor top = QColor::fromHsvF(hue / 360.0, 0.9, 1.0);
        QColor bot = QColor::fromHsvF(hue / 360.0, 0.6, 0.4, 0.3);

        if (bH < 1) continue;

        QLinearGradient grad(x, y, x, H);
        grad.setColorAt(0, top);
        grad.setColorAt(1, bot);
        p.fillRect(QRectF(x + gap/2, y, barW - gap, bH), grad);

        // 픽 라인
        if (v > 0.04)
            p.fillRect(QRectF(x + gap/2, y - 3, barW - gap, 3), top.lighter(140));
    }
}

// ── AudioView ─────────────────────────────────────────────────
AudioView::AudioView(QWidget *parent) : QWidget(parent)
{
    m_wasapi = new WasapiCapture(this);
    connect(m_wasapi, &WasapiCapture::spectrumReady, this, &AudioView::onSpectrumReady);
    connect(m_wasapi, &WasapiCapture::captureError,  this, &AudioView::onCaptureError);

    m_demoTimer = new QTimer(this);
    m_demoTimer->setInterval(40); // 25fps 데모
    connect(m_demoTimer, &QTimer::timeout, this, &AudioView::onDemoFrame);

    setupUi();
}

AudioView::~AudioView()
{
    if (m_wasapi && m_wasapi->isCapturing())
        m_wasapi->stopCapture();
}

void AudioView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 헤더
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("🎵  오디오 비주얼라이저");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    header->addWidget(title);
    header->addStretch();
    mainLayout->addLayout(header);

    // 비주얼라이저
    auto *visBox = new QGroupBox("실시간 오디오 스펙트럼");
    auto *visLayout = new QVBoxLayout(visBox);
    m_vis = new BarVisWidget;
    m_vis->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    visLayout->addWidget(m_vis);
    mainLayout->addWidget(visBox, 1);

    // 컨트롤
    auto *ctrlBox = new QGroupBox("컨트롤");
    auto *ctrlLayout = new QVBoxLayout(ctrlBox);

    // 캡처 버튼 + 상태
    auto *row1 = new QHBoxLayout;
    m_captureBtn = new QPushButton("🎤 시스템 오디오 캡처 시작");
    m_captureBtn->setStyleSheet("background:#0277BD;font-weight:bold;min-width:180px;min-height:36px;");
    m_statusLabel = new QLabel("오디오 캡처 비활성화됨");
    m_statusLabel->setStyleSheet("color:#90A4AE;");
    connect(m_captureBtn, &QPushButton::clicked, this, &AudioView::onToggleCapture);
    row1->addWidget(m_captureBtn);
    row1->addWidget(m_statusLabel, 1);
    ctrlLayout->addLayout(row1);

    // 감도
    auto *row2 = new QHBoxLayout;
    row2->addWidget(new QLabel("감도:"));
    m_sensitivitySlider = new QSlider(Qt::Horizontal);
    m_sensitivitySlider->setRange(10, 400);
    m_sensitivitySlider->setValue(100);
    m_sensLabel = new QLabel("100%");
    connect(m_sensitivitySlider, &QSlider::valueChanged, this, [this](int v) {
        m_sensitivity = v / 100.0;
        m_sensLabel->setText(QString("%1%").arg(v));
    });
    row2->addWidget(m_sensitivitySlider, 1);
    row2->addWidget(m_sensLabel);
    ctrlLayout->addLayout(row2);

    // BPM
    auto *row3 = new QHBoxLayout;
    row3->addWidget(new QLabel("감지 BPM:"));
    m_bpmLabel = new QLabel("-- BPM");
    m_bpmLabel->setStyleSheet("color:#4FC3F7;font-weight:bold;font-size:14px;");
    row3->addWidget(m_bpmLabel);
    row3->addStretch();
    ctrlLayout->addLayout(row3);

    mainLayout->addWidget(ctrlBox);

    // 안내
    auto *note = new QLabel("💡 PC에서 재생 중인 모든 사운드(유튜브, 게임, 스포티파이 등)를 실시간으로 시각화합니다.\n"
                             "   WASAPI 루프백 캡처 방식이므로 별도 마이크 없이 동작합니다.");
    note->setStyleSheet("color:#546E7A;font-size:11px;");
    note->setWordWrap(true);
    mainLayout->addWidget(note);
}

void AudioView::onToggleCapture()
{
    m_capturing = !m_capturing;

    if (m_capturing) {
        m_captureBtn->setText("⏹ 캡처 중지");
        m_captureBtn->setStyleSheet("background:#B71C1C;font-weight:bold;min-width:180px;min-height:36px;");
        m_vis->setDemoMode(false);
        m_wasapi->startCapture();
        m_statusLabel->setText("🔴 시스템 오디오 캡처 중 (루프백)");
        m_statusLabel->setStyleSheet("color:#EF5350;");
    } else {
        m_captureBtn->setText("🎤 시스템 오디오 캡처 시작");
        m_captureBtn->setStyleSheet("background:#0277BD;font-weight:bold;min-width:180px;min-height:36px;");
        m_statusLabel->setText("오디오 캡처 비활성화됨");
        m_statusLabel->setStyleSheet("color:#90A4AE;");
        m_demoTimer->stop();
        m_wasapi->stopCapture();
        QList<double> zeros(32, 0.0);
        m_vis->setBars(zeros);
        m_vis->update();
    }
}

void AudioView::onSpectrumReady(const QList<double> &bars)
{
    // 감도 적용
    QList<double> scaled;
    scaled.reserve(bars.size());
    for (double v : bars)
        scaled << std::min(1.0, v * m_sensitivity);

    m_vis->setBars(scaled);

    // BPM 감지: 저음역 에너지 기반
    if (!scaled.isEmpty()) {
        double bass = 0;
        for (int i = 0; i < std::min(4, (int)scaled.size()); ++i)
            bass += scaled[i];
        bass /= 4.0;

        m_bassHistory.append(bass);
        if (m_bassHistory.size() > 20) m_bassHistory.removeFirst();

        // 평균 대비 급증 감지
        double avg = 0;
        for (double b : m_bassHistory) avg += b;
        avg /= m_bassHistory.size();

        if (bass > avg * 1.5 && bass > 0.2) {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            m_beatTimes.append(now);
            // 최근 8개 비트만 유지
            while (m_beatTimes.size() > 8) m_beatTimes.removeFirst();
            // 평균 BPM 계산
            if (m_beatTimes.size() >= 3) {
                qint64 span = m_beatTimes.last() - m_beatTimes.first();
                if (span > 0) {
                    double bpm = (m_beatTimes.size() - 1) * 60000.0 / span;
                    if (bpm >= 40 && bpm <= 220)
                        m_bpmLabel->setText(QString("%1 BPM").arg(qRound(bpm)));
                }
            }
        }
    }
}

void AudioView::onCaptureError(const QString &msg)
{
    m_statusLabel->setText("⚠ " + msg + " — 데모 모드로 전환");
    m_statusLabel->setStyleSheet("color:#FFA726;");
    m_vis->setDemoMode(true);
    m_demoTimer->start();
}

void AudioView::onDemoFrame()
{
    m_demoPhase += 0.05;
    const int N = 32;
    QList<double> bars;
    bars.reserve(N);
    for (int i = 0; i < N; ++i) {
        double fi   = i / double(N);
        double base = (1.0 - fi * 0.55) * 0.55;
        double w1   = sin(m_demoPhase * 2.3 + fi * 5.8) * 0.28;
        double w2   = sin(m_demoPhase * 1.1 + fi * 2.9) * 0.16;
        double noise= (rand() % 100 / 100.0 - 0.5) * 0.06;
        bars << std::max(0.0, std::min(1.0, (base + w1 + w2 + noise) * m_sensitivity));
    }
    m_vis->setBars(bars);
}

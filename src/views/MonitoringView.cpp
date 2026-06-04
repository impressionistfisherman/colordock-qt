#include "MonitoringView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
static quint64 toULL(FILETIME ft) {
    return (quint64(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}
#endif

// ── GaugeWidget ──────────────────────────────────────────────
GaugeWidget::GaugeWidget(const QString &title, const QString &unit, QWidget *parent)
    : QWidget(parent), m_title(title), m_unit(unit)
{
    setMinimumSize(200, 140);
    m_history.resize(HISTORY, 0.0);
}

void GaugeWidget::setValue(double v)
{
    m_value = qBound(0.0, v, 100.0);
    m_history.removeFirst();
    m_history.append(m_value);
    update();
}

void GaugeWidget::setValueLabel(const QString &s)
{
    m_valLabel = s;
    update();
}

void GaugeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = width(), H = height();
    const int chartH = H - 56;
    const int chartY = 40;

    // 배경
    p.setBrush(QColor(15, 18, 35));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 10, 10);

    // 타이틀
    p.setPen(QColor(144, 164, 174));
    p.setFont(QFont("Segoe UI", 9));
    p.drawText(QRect(12, 8, W-24, 20), Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // 현재값
    double pct = m_value / 100.0;
    QColor barColor;
    if (pct < 0.6)      barColor = QColor(0, 180, 255);
    else if (pct < 0.8) barColor = QColor(255, 165, 0);
    else                barColor = QColor(255, 60, 60);

    p.setPen(barColor);
    p.setFont(QFont("Segoe UI", 15, QFont::Bold));
    p.drawText(QRect(W - 90, 4, 80, 30), Qt::AlignRight | Qt::AlignVCenter,
               m_valLabel.isEmpty() ? QString("%1%2").arg(int(m_value)).arg(m_unit) : m_valLabel);

    // 스파크라인 영역
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(25, 30, 50));
    p.drawRoundedRect(8, chartY, W-16, chartH, 6, 6);

    // 그리드 라인
    p.setPen(QPen(QColor(255,255,255,18), 1));
    for (int i = 1; i < 4; ++i) {
        int y = chartY + chartH * i / 4;
        p.drawLine(8, y, W-8, y);
    }

    // 스파크라인 채우기
    if (m_history.size() < 2) return;
    double dx = (W - 16.0) / (HISTORY - 1);

    QPolygonF fill;
    fill << QPointF(8, chartY + chartH);
    for (int i = 0; i < m_history.size(); ++i) {
        double x = 8 + i * dx;
        double y = chartY + chartH - (m_history[i] / 100.0) * chartH;
        fill << QPointF(x, y);
    }
    fill << QPointF(8 + (m_history.size()-1) * dx, chartY + chartH);

    QLinearGradient grad(0, chartY, 0, chartY + chartH);
    grad.setColorAt(0, QColor(barColor.red(), barColor.green(), barColor.blue(), 180));
    grad.setColorAt(1, QColor(barColor.red(), barColor.green(), barColor.blue(), 20));
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawPolygon(fill);

    // 라인
    QPainterPath path;
    for (int i = 0; i < m_history.size(); ++i) {
        double x = 8 + i * dx;
        double y = chartY + chartH - (m_history[i] / 100.0) * chartH;
        if (i == 0) path.moveTo(x, y);
        else        path.lineTo(x, y);
    }
    p.setPen(QPen(barColor, 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    // 현재값 점
    double lastX = 8 + (m_history.size()-1) * dx;
    double lastY = chartY + chartH - (m_history.last() / 100.0) * chartH;
    p.setBrush(barColor);
    p.setPen(QPen(Qt::white, 1.5));
    p.drawEllipse(QPointF(lastX, lastY), 4, 4);
}

// ── MonitoringView ───────────────────────────────────────────
MonitoringView::MonitoringView(QWidget *parent) : QWidget(parent)
{
    setupUi();
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &MonitoringView::onPollTimer);
    m_timer->start();
    // 첫 호출: GetSystemTimes 기준점 초기화만 수행 (결과 버림)
    getCpuUsage();
    // 1초 후 첫 정상 샘플 표시
    QTimer::singleShot(1000, this, &MonitoringView::onPollTimer);
}

void MonitoringView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 헤더
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("📊  시스템 모니터링");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    m_timeLabel = new QLabel;
    m_timeLabel->setStyleSheet("color:#546E7A;font-size:11px;");
    header->addWidget(title);
    header->addStretch();
    header->addWidget(m_timeLabel);
    mainLayout->addLayout(header);

    // 게이지 그리드
    auto *gaugeGrid = new QHBoxLayout;
    gaugeGrid->setSpacing(16);

    m_cpuGauge = new GaugeWidget("⚙️  CPU 사용률", "%");
    m_ramGauge = new GaugeWidget("🧠  RAM 점유율", "%");
    m_cpuGauge->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_ramGauge->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    gaugeGrid->addWidget(m_cpuGauge);
    gaugeGrid->addWidget(m_ramGauge);
    mainLayout->addLayout(gaugeGrid, 1);

    // 상세 텍스트
    m_cpuLabel = new QLabel;
    m_cpuLabel->setStyleSheet("color:#90A4AE;font-size:12px;");
    m_ramLabel = new QLabel;
    m_ramLabel->setStyleSheet("color:#90A4AE;font-size:12px;");

    auto *detailBox = new QGroupBox("상세 정보");
    auto *detailLayout = new QVBoxLayout(detailBox);
    detailLayout->addWidget(m_cpuLabel);
    detailLayout->addWidget(m_ramLabel);
    mainLayout->addWidget(detailBox);

    auto *noteLabel = new QLabel("💡 GPU 온도는 OpenRGB 연결 시 표시됩니다.");
    noteLabel->setStyleSheet("color:#546E7A;font-size:11px;");
    mainLayout->addWidget(noteLabel);
}

double MonitoringView::getCpuUsage()
{
#ifdef Q_OS_WIN
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user))
        return 0.0;

    quint64 currIdle   = toULL(idle);
    quint64 currKernel = toULL(kernel);
    quint64 currUser   = toULL(user);

    quint64 idleDiff   = currIdle   - m_prevIdle;
    quint64 kernelDiff = currKernel - m_prevKernel;
    quint64 userDiff   = currUser   - m_prevUser;

    m_prevIdle   = currIdle;
    m_prevKernel = currKernel;
    m_prevUser   = currUser;

    quint64 total = kernelDiff + userDiff;
    if (total == 0) return 0.0;
    return (total - idleDiff) * 100.0 / total;
#else
    return 0.0;
#endif
}

double MonitoringView::getRamUsage(double &usedGB, double &totalGB)
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return 0.0;

    totalGB = ms.ullTotalPhys / (1024.0 * 1024 * 1024);
    usedGB  = (ms.ullTotalPhys - ms.ullAvailPhys) / (1024.0 * 1024 * 1024);
    return ms.dwMemoryLoad;
#else
    usedGB = totalGB = 0;
    return 0.0;
#endif
}

void MonitoringView::onPollTimer()
{
    double cpu = getCpuUsage();
    double usedGB = 0, totalGB = 0;
    double ram = getRamUsage(usedGB, totalGB);

    m_cpuGauge->setValue(cpu);
    m_ramGauge->setValue(ram);
    m_ramGauge->setValueLabel(QString("%1%").arg(int(ram)));

    m_cpuLabel->setText(QString("CPU 사용률: %1%").arg(cpu, 0, 'f', 1));
    m_ramLabel->setText(QString("RAM: %.1f GB / %.1f GB  (%2%)")
        .arg(usedGB).arg(totalGB).arg(int(ram)));

    m_timeLabel->setText("업데이트: " + QDateTime::currentDateTime().toString("HH:mm:ss"));
}

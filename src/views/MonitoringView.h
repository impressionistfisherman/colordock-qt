#pragma once
#include <QWidget>
#include <QTimer>
#include <QList>

class QLabel;
class QProgressBar;

class GaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit GaugeWidget(const QString &title, const QString &unit, QWidget *parent = nullptr);
    void setValue(double v);   // 0..100
    void setValueLabel(const QString &s);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString m_title;
    QString m_unit;
    QString m_valLabel;
    double  m_value = 0.0;
    QColor  m_color = QColor(0, 180, 255);
    QList<double> m_history;
    static constexpr int HISTORY = 60;
};

class MonitoringView : public QWidget {
    Q_OBJECT
public:
    explicit MonitoringView(QWidget *parent = nullptr);

private slots:
    void onPollTimer();

private:
    void setupUi();
    double getCpuUsage();
    double getRamUsage(double &usedGB, double &totalGB);

    QTimer       *m_timer;
    GaugeWidget  *m_cpuGauge;
    GaugeWidget  *m_ramGauge;
    QLabel       *m_cpuLabel;
    QLabel       *m_ramLabel;
    QLabel       *m_timeLabel;

    // GetSystemTimes state (Windows only)
    quint64 m_prevIdle   = 0;
    quint64 m_prevKernel = 0;
    quint64 m_prevUser   = 0;
};

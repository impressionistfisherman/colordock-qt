#pragma once
#include <QWidget>
#include <QList>
#include <QSet>
#include <QTimer>

class QListWidget;
class QPushButton;
class QLabel;

struct Schedule {
    QString id;
    QString name;
    QString triggerType; // "time" | "process"
    QString timeValue;   // "HH:mm"
    QList<int> days;     // 0=Mon .. 6=Sun
    QString processName;
    QString actionType;  // "scene" | "effect"
    QString actionValue; // scene id or effect name
    bool    enabled = true;
};

class DeviceManager;

class ScheduleView : public QWidget {
    Q_OBJECT
public:
    explicit ScheduleView(DeviceManager *dm = nullptr, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onDelete();
    void onToggleEnabled();
    void onSelectionChanged();
    void onCheckTriggers();
    void openEditor(Schedule *s);

private:
    void setupUi();
    void loadSchedules();
    void saveSchedules();
    void refreshList();
    QString schedulesFilePath() const;

    QListWidget  *m_list;
    QPushButton  *m_deleteBtn;
    QPushButton  *m_toggleBtn;
    QLabel       *m_detailLabel;

    DeviceManager  *m_dm = nullptr;
    QList<Schedule> m_schedules;
    QTimer         *m_checkTimer;
    QSet<QString>   m_firedToday; // 오늘 이미 발화한 스케줄 ID
};

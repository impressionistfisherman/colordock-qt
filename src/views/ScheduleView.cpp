#include "ScheduleView.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTimeEdit>
#include <QCheckBox>
#include <QButtonGroup>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QMessageBox>
#include <QTime>
#include <QDate>
#include <QProcess>

static const QStringList DAY_LABELS = {"월","화","수","목","금","토","일"};

ScheduleView::ScheduleView(DeviceManager *dm, QWidget *parent) : QWidget(parent), m_dm(dm)
{
    setupUi();
    loadSchedules();
    refreshList();

    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(30000); // 30초마다 트리거 확인
    connect(m_checkTimer, &QTimer::timeout, this, &ScheduleView::onCheckTriggers);
    m_checkTimer->start();
}

void ScheduleView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 헤더
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("⏰  자동화 스케줄");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    auto *addBtn = new QPushButton("+ 새 스케줄");
    addBtn->setStyleSheet("background:#0277BD;font-weight:bold;");
    connect(addBtn, &QPushButton::clicked, this, &ScheduleView::onAdd);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(addBtn);
    mainLayout->addLayout(header);

    auto *desc = new QLabel("시간 또는 게임 실행 시 자동으로 씬을 전환합니다.");
    desc->setStyleSheet("color:#546E7A;font-size:11px;");
    mainLayout->addWidget(desc);

    // 스케줄 목록
    auto *listBox = new QGroupBox("저장된 스케줄");
    auto *listLayout = new QVBoxLayout(listBox);

    m_list = new QListWidget;
    m_list->setStyleSheet(R"(
        QListWidget { border:none; }
        QListWidget::item { padding:10px; border-radius:6px; margin:2px 0; }
        QListWidget::item:hover { background:rgba(79,195,247,0.08); }
        QListWidget::item:selected { background:rgba(79,195,247,0.18); color:#4FC3F7; }
    )");
    connect(m_list, &QListWidget::itemSelectionChanged, this, &ScheduleView::onSelectionChanged);
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this]() {
        int idx = m_list->currentRow();
        if (idx >= 0 && idx < m_schedules.size())
            openEditor(&m_schedules[idx]);
    });
    listLayout->addWidget(m_list, 1);

    auto *btnRow = new QHBoxLayout;
    m_detailLabel = new QLabel;
    m_detailLabel->setStyleSheet("color:#90A4AE;font-size:12px;");
    m_toggleBtn = new QPushButton("⏸ 비활성화");
    m_deleteBtn = new QPushButton("🗑 삭제");
    auto *editBtn = new QPushButton("✏ 편집");
    for (auto *b : {m_toggleBtn, editBtn, m_deleteBtn}) b->setEnabled(false);
    m_deleteBtn->setStyleSheet("background:#B71C1C;min-width:70px;");
    m_toggleBtn->setStyleSheet("min-width:90px;");

    connect(m_deleteBtn, &QPushButton::clicked, this, &ScheduleView::onDelete);
    connect(m_toggleBtn, &QPushButton::clicked, this, &ScheduleView::onToggleEnabled);
    connect(editBtn, &QPushButton::clicked, this, [this]() {
        int idx = m_list->currentRow();
        if (idx >= 0 && idx < m_schedules.size())
            openEditor(&m_schedules[idx]);
    });
    connect(m_list, &QListWidget::itemSelectionChanged, this, [this, editBtn]() {
        int idx = m_list->currentRow();
        bool ok = idx >= 0 && idx < m_schedules.size();
        editBtn->setEnabled(ok);
    });

    btnRow->addWidget(m_detailLabel, 1);
    btnRow->addWidget(editBtn);
    btnRow->addWidget(m_toggleBtn);
    btnRow->addWidget(m_deleteBtn);
    listLayout->addLayout(btnRow);
    mainLayout->addWidget(listBox, 1);
}

QString ScheduleView::schedulesFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    return dir + "/schedules.json";
}

void ScheduleView::loadSchedules()
{
    QFile f(schedulesFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    auto arr = QJsonDocument::fromJson(f.readAll()).array();
    m_schedules.clear();
    for (const auto &v : arr) {
        auto o = v.toObject();
        Schedule s;
        s.id          = o["id"].toString();
        s.name        = o["name"].toString();
        s.triggerType = o["triggerType"].toString("time");
        s.timeValue   = o["timeValue"].toString("18:00");
        s.processName = o["processName"].toString();
        s.actionType  = o["actionType"].toString("effect");
        s.actionValue = o["actionValue"].toString("rainbow");
        s.enabled     = o["enabled"].toBool(true);
        for (const auto &d : o["days"].toArray())
            s.days << d.toInt();
        if (s.days.isEmpty()) s.days = {0,1,2,3,4,5,6};
        m_schedules << s;
    }
}

void ScheduleView::saveSchedules()
{
    QJsonArray arr;
    for (const Schedule &s : m_schedules) {
        QJsonObject o;
        o["id"]          = s.id;
        o["name"]        = s.name;
        o["triggerType"] = s.triggerType;
        o["timeValue"]   = s.timeValue;
        o["processName"] = s.processName;
        o["actionType"]  = s.actionType;
        o["actionValue"] = s.actionValue;
        o["enabled"]     = s.enabled;
        QJsonArray days;
        for (int d : s.days) days << d;
        o["days"] = days;
        arr << o;
    }
    QFile f(schedulesFilePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

void ScheduleView::refreshList()
{
    m_list->clear();
    for (const Schedule &s : m_schedules) {
        QString trigger;
        if (s.triggerType == "time") {
            QStringList dayStr;
            for (int d : s.days) dayStr << DAY_LABELS[d];
            trigger = QString("🕐 %1  [%2]").arg(s.timeValue).arg(dayStr.join(","));
        } else {
            trigger = QString("🎮 프로세스: %1").arg(s.processName);
        }
        QString text = QString("%1%2\n   %3  →  %4: %5")
            .arg(s.enabled ? "" : "⏸ ")
            .arg(s.name)
            .arg(trigger)
            .arg(s.actionType)
            .arg(s.actionValue);
        auto *item = new QListWidgetItem(text);
        if (!s.enabled) item->setForeground(QColor(80,80,100));
        m_list->addItem(item);
    }
    if (m_schedules.isEmpty()) {
        m_list->addItem("저장된 스케줄이 없습니다. '+ 새 스케줄'을 눌러 추가하세요.");
        m_list->item(0)->setForeground(QColor(100,130,150));
        m_list->item(0)->setFlags(m_list->item(0)->flags() & ~Qt::ItemIsSelectable);
    }
}

void ScheduleView::openEditor(Schedule *s)
{
    bool isNew = (s == nullptr);
    Schedule tmp;
    if (isNew) {
        tmp.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        tmp.days = {0,1,2,3,4,5,6};
        tmp.timeValue = "18:00";
        tmp.enabled = true;
        s = &tmp;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? "새 스케줄" : "스케줄 편집");
    dlg.setMinimumWidth(400);
    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout;
    form->setSpacing(10);

    auto *nameEdit = new QLineEdit(s->name);
    nameEdit->setPlaceholderText("스케줄 이름");
    form->addRow("이름:", nameEdit);

    // 트리거 타입
    auto *triggerCombo = new QComboBox;
    triggerCombo->addItem("🕐 시간 기반", "time");
    triggerCombo->addItem("🎮 프로세스 감지", "process");
    triggerCombo->setCurrentIndex(s->triggerType == "process" ? 1 : 0);
    form->addRow("트리거:", triggerCombo);

    // 시간 패널
    auto *timePanel = new QWidget;
    auto *timeLayout = new QVBoxLayout(timePanel);
    timeLayout->setContentsMargins(0,0,0,0);
    auto *timeEdit = new QTimeEdit(QTime::fromString(s->timeValue, "HH:mm"));
    timeEdit->setDisplayFormat("HH:mm");
    timeLayout->addWidget(timeEdit);
    auto *dayRow = new QHBoxLayout;
    QList<QCheckBox*> dayCbs;
    for (int i = 0; i < 7; ++i) {
        auto *cb = new QCheckBox(DAY_LABELS[i]);
        cb->setChecked(s->days.contains(i));
        dayCbs << cb;
        dayRow->addWidget(cb);
    }
    timeLayout->addLayout(dayRow);

    // 프로세스 패널
    auto *procPanel = new QWidget;
    auto *procLayout = new QFormLayout(procPanel);
    procLayout->setContentsMargins(0,0,0,0);
    auto *procEdit = new QLineEdit(s->processName);
    procEdit->setPlaceholderText("예: cs2.exe, lol.exe");
    procLayout->addRow("프로세스:", procEdit);

    // 패널 전환
    auto updatePanels = [timePanel, procPanel, triggerCombo]() {
        bool isTime = triggerCombo->currentData().toString() == "time";
        timePanel->setVisible(isTime);
        procPanel->setVisible(!isTime);
    };
    connect(triggerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updatePanels);
    updatePanels();

    form->addRow(timePanel);
    form->addRow(procPanel);

    // 액션
    auto *actionCombo = new QComboBox;
    actionCombo->addItem("🎬 씬 불러오기", "scene");
    actionCombo->addItem("✨ 효과 전환",   "effect");
    actionCombo->setCurrentIndex(s->actionType == "effect" ? 1 : 0);
    form->addRow("액션:", actionCombo);

    auto *actionValEdit = new QLineEdit(s->actionValue);
    actionValEdit->setPlaceholderText("씬 ID 또는 효과 이름 (rainbow, breathing 등)");
    form->addRow("값:", actionValEdit);

    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted) return;

    s->name        = nameEdit->text().trimmed();
    s->triggerType = triggerCombo->currentData().toString();
    s->timeValue   = timeEdit->time().toString("HH:mm");
    s->processName = procEdit->text().trimmed();
    s->actionType  = actionCombo->currentData().toString();
    s->actionValue = actionValEdit->text().trimmed();
    s->days.clear();
    for (int i = 0; i < 7; ++i)
        if (dayCbs[i]->isChecked()) s->days << i;
    if (s->name.isEmpty()) return;

    if (isNew) m_schedules << *s;
    saveSchedules();
    refreshList();
}

void ScheduleView::onAdd()    { openEditor(nullptr); }

void ScheduleView::onDelete()
{
    int idx = m_list->currentRow();
    if (idx < 0 || idx >= m_schedules.size()) return;
    if (QMessageBox::question(this, "스케줄 삭제",
        QString("'%1' 을 삭제합니까?").arg(m_schedules[idx].name))
        != QMessageBox::Yes) return;
    m_schedules.removeAt(idx);
    saveSchedules();
    refreshList();
    m_deleteBtn->setEnabled(false);
    m_toggleBtn->setEnabled(false);
}

void ScheduleView::onToggleEnabled()
{
    int idx = m_list->currentRow();
    if (idx < 0 || idx >= m_schedules.size()) return;
    m_schedules[idx].enabled = !m_schedules[idx].enabled;
    saveSchedules();
    refreshList();
    m_list->setCurrentRow(idx);
}

void ScheduleView::onSelectionChanged()
{
    int idx = m_list->currentRow();
    bool ok = idx >= 0 && idx < m_schedules.size();
    m_deleteBtn->setEnabled(ok);
    m_toggleBtn->setEnabled(ok);
    if (ok) {
        const Schedule &s = m_schedules[idx];
        m_toggleBtn->setText(s.enabled ? "⏸ 비활성화" : "▶ 활성화");
        m_detailLabel->setText(QString("%1  •  %2  →  %3")
            .arg(s.name).arg(s.triggerType).arg(s.actionValue));
    }
}

void ScheduleView::onCheckTriggers()
{
    QString nowTime = QTime::currentTime().toString("HH:mm");
    int nowDay = QDate::currentDate().dayOfWeek() - 1; // 0=Mon
    QString todayKey = QDate::currentDate().toString("yyyyMMdd");

    for (const Schedule &s : m_schedules) {
        if (!s.enabled) continue;
        bool fired = false;

        if (s.triggerType == "time") {
            QString key = todayKey + "_" + s.id;
            if (s.timeValue == nowTime && s.days.contains(nowDay) && !m_firedToday.contains(key)) {
                m_firedToday.insert(key);
                fired = true;
            }
        } else if (s.triggerType == "process" && !s.processName.isEmpty()) {
            // 비동기 프로세스 감지 — UI 스레드 블로킹 없이 실행
            QString procName = s.processName;
            QString schedId  = s.id;
            auto *proc = new QProcess(this);
            proc->start("tasklist", {"/FI", QString("IMAGENAME eq %1").arg(procName), "/NH", "/FO", "CSV"});
            connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this, proc, procName, schedId](int) {
                bool found = proc->readAllStandardOutput().contains(procName.toUtf8());
                QString key = "proc_" + schedId;
                if (found) {
                    if (!m_firedToday.contains(key)) {
                        m_firedToday.insert(key);
                        // 스케줄 찾아서 액션 실행
                        for (const Schedule &sc : m_schedules) {
                            if (sc.id == schedId && m_dm)
                                m_dm->applyEffect(sc.actionValue, QColor(0,180,255));
                        }
                    }
                } else {
                    m_firedToday.remove(key);
                }
                proc->deleteLater();
            });
            continue; // 비동기이므로 fired 플래그 불필요
        }

        if (fired && m_dm) {
            qDebug() << "[Schedule] Trigger fired:" << s.name << "->" << s.actionValue;
            if (s.actionType == "effect") {
                m_dm->applyEffect(s.actionValue, QColor(0, 180, 255));
            }
            // scene 액션은 ScenesView 연동 필요 (현재는 effect fallback)
        }
    }

    // 자정 이후 오늘 time 발화 기록 초기화
    static QString lastDay = QDate::currentDate().toString("yyyyMMdd");
    if (todayKey != lastDay) {
        m_firedToday.removeIf([](const QString &k) { return !k.startsWith("proc_"); });
        lastDay = todayKey;
    }
}

#include "ScenesView.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QMessageBox>
#include <QDateTime>

static const QStringList SCENE_ICONS = {"🎮","🎬","🧘","💻","🌙","🔥","❄️","🌈","🎵","⚡"};

ScenesView::ScenesView(DeviceManager *dm, QWidget *parent) : QWidget(parent), m_dm(dm)
{
    m_plTimer = new QTimer(this);
    m_plTimer->setInterval(1000);
    connect(m_plTimer, &QTimer::timeout, this, &ScenesView::onPlaylistTick);

    setupUi();
    loadScenes();
    loadPlaylists();
    refreshSceneList();
    refreshPlaylistList();
}

void ScenesView::setCurrentEffect(const QString &effect, const QColor &color)
{
    m_currentEffect = effect;
    m_currentColor  = color;
}

void ScenesView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // 헤더 + 탭 전환
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("🎬  씬 관리");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    auto *tabSceneBtn = new QPushButton("씬");
    auto *tabPlBtn    = new QPushButton("▶ 타임라인");
    for (auto *b : {tabSceneBtn, tabPlBtn}) {
        b->setCheckable(true);
        b->setStyleSheet(R"(
            QPushButton{font-size:11px;padding:4px 14px;border-radius:8px;
                        border:1px solid rgba(255,255,255,0.12);background:rgba(255,255,255,0.04);color:#90A4AE;}
            QPushButton:checked{background:rgba(79,195,247,0.15);border-color:rgba(79,195,247,0.4);color:#4FC3F7;}
        )");
    }
    tabSceneBtn->setChecked(true);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(tabSceneBtn);
    header->addWidget(tabPlBtn);
    mainLayout->addLayout(header);

    m_stack = new QStackedWidget;

    // ═══ PAGE 0: 씬 ═══════════════════════════════════════
    auto *scenePage = new QWidget;
    auto *scenePageLayout = new QVBoxLayout(scenePage);
    scenePageLayout->setContentsMargins(0,0,0,0);
    scenePageLayout->setSpacing(10);

    // 저장 영역
    auto *saveBox = new QGroupBox("현재 상태 저장");
    auto *saveLayout = new QHBoxLayout(saveBox);
    m_iconCombo = new QComboBox;
    for (const QString &ic : SCENE_ICONS) m_iconCombo->addItem(ic);
    m_iconCombo->setFixedWidth(60);
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("씬 이름 (예: 게임 모드, 영화 감상)");
    auto *saveBtn = new QPushButton("💾 현재 상태 저장");
    saveBtn->setStyleSheet("background:#0288D1;font-weight:bold;min-width:140px;");
    connect(saveBtn, &QPushButton::clicked, this, &ScenesView::onSaveScene);
    saveLayout->addWidget(m_iconCombo);
    saveLayout->addWidget(m_nameEdit, 1);
    saveLayout->addWidget(saveBtn);
    scenePageLayout->addWidget(saveBox);

    // 씬 목록
    auto *listBox = new QGroupBox("저장된 씬");
    auto *listLayout = new QVBoxLayout(listBox);
    m_sceneList = new QListWidget;
    m_sceneList->setStyleSheet(R"(
        QListWidget{border:none;}
        QListWidget::item{padding:10px;border-radius:6px;margin:2px 0;}
        QListWidget::item:hover{background:rgba(79,195,247,0.08);}
        QListWidget::item:selected{background:rgba(79,195,247,0.18);color:#4FC3F7;}
    )");
    connect(m_sceneList, &QListWidget::itemSelectionChanged, this, &ScenesView::onSceneSelectionChanged);
    connect(m_sceneList, &QListWidget::itemDoubleClicked, this, &ScenesView::onApplyScene);
    listLayout->addWidget(m_sceneList, 1);
    auto *btnRow = new QHBoxLayout;
    m_sceneDetailLabel = new QLabel;
    m_sceneDetailLabel->setStyleSheet("color:#90A4AE;font-size:12px;");
    m_applyBtn  = new QPushButton("▶ 적용");
    m_deleteBtn = new QPushButton("🗑 삭제");
    m_applyBtn->setEnabled(false); m_deleteBtn->setEnabled(false);
    m_applyBtn->setStyleSheet("background:#2E7D32;min-width:80px;");
    m_deleteBtn->setStyleSheet("background:#B71C1C;min-width:80px;");
    connect(m_applyBtn,  &QPushButton::clicked, this, &ScenesView::onApplyScene);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ScenesView::onDeleteScene);
    btnRow->addWidget(m_sceneDetailLabel, 1);
    btnRow->addWidget(m_applyBtn);
    btnRow->addWidget(m_deleteBtn);
    listLayout->addLayout(btnRow);
    scenePageLayout->addWidget(listBox, 1);

    // ═══ PAGE 1: 타임라인/플레이리스트 ═══════════════════════
    auto *plPage = new QWidget;
    auto *plPageLayout = new QVBoxLayout(plPage);
    plPageLayout->setContentsMargins(0,0,0,0);
    plPageLayout->setSpacing(10);

    // 플레이어 상태 바
    auto *playerBar = new QGroupBox("재생 중");
    auto *playerLayout = new QHBoxLayout(playerBar);
    m_plStatusLabel   = new QLabel("정지");
    m_plStatusLabel->setStyleSheet("color:#90A4AE;");
    m_plProgressLabel = new QLabel;
    m_plProgressLabel->setStyleSheet("color:#546E7A;font-size:11px;");
    m_plStopBtn = new QPushButton("⏹ 정지");
    m_plStopBtn->setEnabled(false);
    connect(m_plStopBtn, &QPushButton::clicked, this, &ScenesView::onStopPlaylist);
    playerLayout->addWidget(m_plStatusLabel, 1);
    playerLayout->addWidget(m_plProgressLabel);
    playerLayout->addWidget(m_plStopBtn);
    plPageLayout->addWidget(playerBar);

    // 플레이리스트 목록
    auto *plListBox = new QGroupBox("저장된 플레이리스트");
    auto *plListLayout = new QVBoxLayout(plListBox);
    m_plList = new QListWidget;
    m_plList->setStyleSheet(m_sceneList->styleSheet());
    plListLayout->addWidget(m_plList, 1);
    auto *plBtnRow = new QHBoxLayout;
    m_plPlayBtn   = new QPushButton("▶ 재생");
    m_plDeleteBtn = new QPushButton("🗑 삭제");
    auto *plAddBtn = new QPushButton("+ 새 플레이리스트");
    m_plPlayBtn->setEnabled(false); m_plDeleteBtn->setEnabled(false);
    m_plPlayBtn->setStyleSheet("background:#2E7D32;min-width:80px;");
    m_plDeleteBtn->setStyleSheet("background:#B71C1C;min-width:80px;");
    plAddBtn->setStyleSheet("background:#0277BD;");
    connect(plAddBtn,      &QPushButton::clicked, this, &ScenesView::onAddPlaylist);
    connect(m_plDeleteBtn, &QPushButton::clicked, this, &ScenesView::onDeletePlaylist);
    connect(m_plPlayBtn,   &QPushButton::clicked, this, &ScenesView::onPlayPlaylist);
    connect(m_plList, &QListWidget::itemSelectionChanged, this, [this]() {
        bool ok = m_plList->currentRow() >= 0 && m_plList->currentRow() < m_playlists.size();
        m_plPlayBtn->setEnabled(ok);
        m_plDeleteBtn->setEnabled(ok);
    });
    plBtnRow->addWidget(plAddBtn);
    plBtnRow->addStretch();
    plBtnRow->addWidget(m_plPlayBtn);
    plBtnRow->addWidget(m_plDeleteBtn);
    plListLayout->addLayout(plBtnRow);
    plPageLayout->addWidget(plListBox, 1);

    m_stack->addWidget(scenePage);
    m_stack->addWidget(plPage);
    mainLayout->addWidget(m_stack, 1);

    // 탭 전환 연결
    connect(tabSceneBtn, &QPushButton::clicked, this, [this, tabSceneBtn, tabPlBtn]() {
        tabSceneBtn->setChecked(true); tabPlBtn->setChecked(false);
        m_stack->setCurrentIndex(0);
    });
    connect(tabPlBtn, &QPushButton::clicked, this, [this, tabSceneBtn, tabPlBtn]() {
        tabPlBtn->setChecked(true); tabSceneBtn->setChecked(false);
        m_stack->setCurrentIndex(1);
    });
}

QString ScenesView::scenesFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    return dir + "/scenes.json";
}

void ScenesView::loadScenes()
{
    QFile f(scenesFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    auto arr = QJsonDocument::fromJson(f.readAll()).array();
    m_scenes.clear();
    for (const auto &v : arr) {
        auto o = v.toObject();
        Scene s;
        s.id     = o["id"].toString();
        s.name   = o["name"].toString();
        s.icon   = o["icon"].toString("🎬");
        s.effect = o["effect"].toString("static");
        s.color  = QColor(o["color"].toString("#00b4ff"));
        m_scenes << s;
    }
}

void ScenesView::saveScenes()
{
    QJsonArray arr;
    for (const Scene &s : m_scenes) {
        QJsonObject o;
        o["id"]     = s.id;
        o["name"]   = s.name;
        o["icon"]   = s.icon;
        o["effect"] = s.effect;
        o["color"]  = s.color.name();
        arr << o;
    }
    QFile f(scenesFilePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

void ScenesView::refreshSceneList()
{
    m_sceneList->clear();
    for (const Scene &s : m_scenes) {
        QString text = QString("%1  %2\n   효과: %3  |  색상: %4")
            .arg(s.icon).arg(s.name).arg(s.effect).arg(s.color.name());
        auto *item = new QListWidgetItem(text);
        QPixmap px(14, 14); px.fill(s.color);
        item->setIcon(QIcon(px));
        m_sceneList->addItem(item);
    }
    if (m_scenes.isEmpty()) {
        m_sceneList->addItem("저장된 씬이 없습니다. 위에서 현재 상태를 저장하세요.");
        m_sceneList->item(0)->setForeground(QColor(100,130,150));
        m_sceneList->item(0)->setFlags(m_sceneList->item(0)->flags() & ~Qt::ItemIsSelectable);
    }
}

QString ScenesView::playlistsFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    return dir + "/playlists.json";
}

void ScenesView::loadPlaylists()
{
    QFile f(playlistsFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    auto arr = QJsonDocument::fromJson(f.readAll()).array();
    m_playlists.clear();
    for (const auto &v : arr) {
        auto o = v.toObject();
        Playlist p;
        p.id   = o["id"].toString();
        p.name = o["name"].toString();
        p.loop = o["loop"].toBool(true);
        for (const auto &iv : o["items"].toArray()) {
            auto io = iv.toObject();
            PlaylistItem pi;
            pi.sceneId     = io["sceneId"].toString();
            pi.durationSec = io["duration"].toInt(5);
            p.items << pi;
        }
        m_playlists << p;
    }
}

void ScenesView::savePlaylists()
{
    QJsonArray arr;
    for (const Playlist &p : m_playlists) {
        QJsonObject o;
        o["id"] = p.id; o["name"] = p.name; o["loop"] = p.loop;
        QJsonArray items;
        for (const PlaylistItem &pi : p.items) {
            QJsonObject io;
            io["sceneId"] = pi.sceneId; io["duration"] = pi.durationSec;
            items << io;
        }
        o["items"] = items;
        arr << o;
    }
    QFile f(playlistsFilePath());
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(arr).toJson());
}

void ScenesView::refreshPlaylistList()
{
    m_plList->clear();
    for (const Playlist &p : m_playlists) {
        QString text = QString("▶  %1\n   씬 %2개  |  %3")
            .arg(p.name).arg(p.items.size())
            .arg(p.loop ? "반복" : "1회");
        m_plList->addItem(text);
    }
    if (m_playlists.isEmpty()) {
        m_plList->addItem("저장된 플레이리스트가 없습니다.");
        m_plList->item(0)->setForeground(QColor(100,130,150));
        m_plList->item(0)->setFlags(m_plList->item(0)->flags() & ~Qt::ItemIsSelectable);
    }
}

void ScenesView::openPlaylistEditor(Playlist *p)
{
    bool isNew = !p;
    Playlist tmp;
    if (isNew) { tmp.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8); p = &tmp; }

    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? "새 플레이리스트" : "플레이리스트 편집");
    dlg.setMinimumWidth(380);
    auto *lay = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout;

    auto *nameEdit = new QLineEdit(p->name);
    nameEdit->setPlaceholderText("플레이리스트 이름");
    form->addRow("이름:", nameEdit);

    // 씬 추가
    auto *itemList = new QListWidget;
    for (const PlaylistItem &pi : p->items) {
        QString label = pi.sceneId;
        for (const Scene &sc : m_scenes)
            if (sc.id == pi.sceneId) { label = sc.icon + " " + sc.name; break; }
        itemList->addItem(QString("%1  (%2초)").arg(label).arg(pi.durationSec));
    }
    form->addRow("씬 목록:", itemList);

    // 씬 추가 행
    auto *addRow = new QHBoxLayout;
    auto *sceneCombo = new QComboBox;
    for (const Scene &s : m_scenes) sceneCombo->addItem(s.icon + " " + s.name, s.id);
    auto *durSpin = new QSpinBox; durSpin->setRange(1,3600); durSpin->setValue(5);
    auto *addItemBtn = new QPushButton("+ 추가");
    QList<PlaylistItem> editItems = p->items;
    connect(addItemBtn, &QPushButton::clicked, this, [&]() {
        PlaylistItem pi;
        pi.sceneId = sceneCombo->currentData().toString();
        pi.durationSec = durSpin->value();
        editItems << pi;
        itemList->addItem(QString("%1  (%2초)").arg(sceneCombo->currentText()).arg(pi.durationSec));
    });
    addRow->addWidget(sceneCombo, 1);
    addRow->addWidget(durSpin);
    addRow->addWidget(new QLabel("초"));
    addRow->addWidget(addItemBtn);
    form->addRow(addRow);

    lay->addLayout(form);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(btns);

    if (dlg.exec() != QDialog::Accepted) return;
    p->name  = nameEdit->text().trimmed();
    p->items = editItems;
    if (p->name.isEmpty()) return;
    if (isNew) m_playlists << *p;
    savePlaylists();
    refreshPlaylistList();
}

void ScenesView::onSaveScene()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) { m_nameEdit->setPlaceholderText("⚠ 씬 이름을 입력하세요!"); return; }
    Scene s;
    s.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    s.name = name; s.icon = m_iconCombo->currentText();
    s.effect = m_currentEffect; s.color = m_currentColor;
    m_scenes << s;
    saveScenes(); refreshSceneList(); m_nameEdit->clear();
}

void ScenesView::onDeleteScene()
{
    int idx = m_sceneList->currentRow();
    if (idx < 0 || idx >= m_scenes.size()) return;
    if (QMessageBox::question(this, "씬 삭제",
        QString("'%1' 을 삭제합니까?").arg(m_scenes[idx].name)) != QMessageBox::Yes) return;
    m_scenes.removeAt(idx);
    saveScenes(); refreshSceneList();
    m_applyBtn->setEnabled(false); m_deleteBtn->setEnabled(false);
    m_sceneDetailLabel->clear();
}

void ScenesView::onApplyScene()
{
    int idx = m_sceneList->currentRow();
    if (idx < 0 || idx >= m_scenes.size()) return;
    const Scene &s = m_scenes[idx];
    if (m_dm) m_dm->applyEffect(s.effect, s.color);
    m_sceneDetailLabel->setText(QString("✅ '%1' 적용됨 — %2 / %3")
        .arg(s.name).arg(s.effect).arg(s.color.name()));
}

void ScenesView::onSceneSelectionChanged()
{
    int idx = m_sceneList->currentRow();
    bool valid = (idx >= 0 && idx < m_scenes.size());
    m_applyBtn->setEnabled(valid); m_deleteBtn->setEnabled(valid);
    if (valid) {
        const Scene &s = m_scenes[idx];
        m_sceneDetailLabel->setText(QString("%1 %2  •  효과: %3  •  색상: %4")
            .arg(s.icon).arg(s.name).arg(s.effect).arg(s.color.name()));
    }
}

void ScenesView::onAddPlaylist()    { openPlaylistEditor(nullptr); }

void ScenesView::onDeletePlaylist()
{
    int idx = m_plList->currentRow();
    if (idx < 0 || idx >= m_playlists.size()) return;
    if (QMessageBox::question(this, "삭제", QString("'%1' 삭제?").arg(m_playlists[idx].name))
        != QMessageBox::Yes) return;
    m_playlists.removeAt(idx);
    savePlaylists(); refreshPlaylistList();
}

void ScenesView::onPlayPlaylist()
{
    int idx = m_plList->currentRow();
    if (idx < 0 || idx >= m_playlists.size() || m_playlists[idx].items.isEmpty()) return;
    m_plActiveIdx = idx;
    m_plItemIdx   = 0;
    m_plElapsed   = 0;
    m_plTimer->start();
    m_plStopBtn->setEnabled(true);
    const Playlist &pl = m_playlists[m_plActiveIdx];
    const PlaylistItem &pi = pl.items[0];
    m_plStatusLabel->setText(QString("▶ %1 재생 중").arg(pl.name));
    m_plProgressLabel->setText(QString("씬 1/%1  · %2초").arg(pl.items.size()).arg(pi.durationSec));
}

void ScenesView::onStopPlaylist()
{
    m_plTimer->stop();
    m_plActiveIdx = -1;
    m_plStopBtn->setEnabled(false);
    m_plStatusLabel->setText("정지");
    m_plProgressLabel->clear();
}

void ScenesView::onPlaylistTick()
{
    if (m_plActiveIdx < 0) { m_plTimer->stop(); return; }
    const Playlist &pl = m_playlists[m_plActiveIdx];
    if (pl.items.isEmpty()) { onStopPlaylist(); return; }

    ++m_plElapsed;
    int duration = pl.items[m_plItemIdx].durationSec;
    int remaining = duration - m_plElapsed;

    m_plProgressLabel->setText(QString("씬 %1/%2  · 남은 시간 %3s")
        .arg(m_plItemIdx+1).arg(pl.items.size()).arg(remaining));

    if (m_plElapsed >= duration) {
        m_plElapsed = 0;
        ++m_plItemIdx;
        if (m_plItemIdx >= pl.items.size()) {
            if (pl.loop) m_plItemIdx = 0;
            else { onStopPlaylist(); return; }
        }
        // TODO: apply scene from pl.items[m_plItemIdx].sceneId
        m_plStatusLabel->setText(QString("▶ 씬 %1/%2").arg(m_plItemIdx+1).arg(pl.items.size()));
    }
}

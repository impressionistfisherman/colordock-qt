#include "MainWindow.h"
#include "views/EffectsView.h"
#include "views/DevicesView.h"
#include "views/KeyboardView.h"
#include "views/ScenesView.h"
#include "views/AudioView.h"
#include "views/MonitoringView.h"
#include "views/ProfilesView.h"
#include "views/SettingsView.h"
#include "views/LayoutView.h"
#include "views/ScheduleView.h"
#include "views/SdkView.h"
#include "hardware/DeviceManager.h"
#include "utils/UpdateChecker.h"
#include "utils/AppSettings.h"

#include <QCloseEvent>
#include <QApplication>
#include <QPixmap>
#include <QScreen>
#include <QTime>
#include <QPainter>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("ColorDock RGB");
    setMinimumSize(1100, 700);
    resize(1280, 800);

    m_closeToTray = AppSettings::instance().get("closeToTray", true).toBool();

    // Core systems
    m_deviceManager  = new DeviceManager(this);
    m_updateChecker  = new UpdateChecker(this);

    setupUi();
    setupTray();
    setupUpdateChecker();

    // Start device detection
    m_deviceManager->startDiscovery();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    m_central = new QWidget(this);
    setCentralWidget(m_central);

    m_mainLayout = new QHBoxLayout(m_central);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // ── 사이드바 ──────────────────────────────────────────
    m_sidebar = new QWidget;
    m_sidebar->setFixedWidth(200);
    m_sidebar->setStyleSheet("background-color: #16213E;");
    m_sidebarLayout = new QVBoxLayout(m_sidebar);
    m_sidebarLayout->setContentsMargins(10, 16, 10, 16);
    m_sidebarLayout->setSpacing(4);

    // 로고 + 서브타이틀
    auto *logoWrap = new QWidget;
    auto *logoLayout = new QVBoxLayout(logoWrap);
    logoLayout->setContentsMargins(4, 4, 4, 12);
    logoLayout->setSpacing(1);
    auto *logoTitle = new QLabel("ColorDock");
    logoTitle->setStyleSheet("font-size:15px;font-weight:bold;color:#4FC3F7;");
    auto *logoSub   = new QLabel("RGB Control");
    logoSub->setStyleSheet("font-size:10px;color:#546E7A;");
    logoLayout->addWidget(logoTitle);
    logoLayout->addWidget(logoSub);
    m_sidebarLayout->addWidget(logoWrap);

    // ── 뷰 초기화 ──────────────────────────────────────────
    m_stack          = new QStackedWidget;
    m_effectsView    = new EffectsView(m_deviceManager);
    m_devicesView    = new DevicesView(m_deviceManager);
    m_keyboardView   = new KeyboardView(m_deviceManager);
    m_scenesView     = new ScenesView(m_deviceManager);
    m_audioView      = new AudioView;
    m_monitoringView = new MonitoringView;
    m_profilesView   = new ProfilesView(m_deviceManager);
    m_settingsView   = new SettingsView;
    m_layoutView     = new LayoutView(m_deviceManager);
    m_scheduleView   = new ScheduleView(m_deviceManager);
    m_sdkView        = new SdkView(m_deviceManager);

    // ── 섹션 레이블 헬퍼
    auto addSection = [this](const QString &label) {
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size:9px;font-weight:bold;color:#546E7A;"
                           "letter-spacing:0.08em;padding:8px 4px 2px 4px;");
        m_sidebarLayout->addWidget(lbl);
    };

    // 내비게이션 + 스택에 추가 (웹앱과 동일한 순서)
    addSection("LIGHTING");
    addNavItem("💡", "조명 효과",  m_effectsView);
    addNavItem("🎬", "씬",        m_scenesView);
    addNavItem("🎹", "키보드",    m_keyboardView);
    addNavItem("🖥️", "레이아웃",  m_layoutView);
    addNavItem("🎵", "오디오",    m_audioView);
    addSection("SYSTEM");
    addNavItem("👤", "프로필",    m_profilesView);
    addNavItem("💻", "기기",      m_devicesView);
    addNavItem("⏰", "자동화",    m_scheduleView);
    addNavItem("📊", "모니터링",  m_monitoringView);
    addNavItem("🛠️", "SDK",       m_sdkView);
    addNavItem("⚙️", "설정",      m_settingsView);

    m_sidebarLayout->addStretch();

    // 다크/라이트 테마 토글
    m_themeBtn = new QPushButton("🌙  다크 모드");
    m_themeBtn->setStyleSheet(R"(
        QPushButton {
            text-align: left; padding: 6px 12px; border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.1);
            background: rgba(255,255,255,0.04); color: #90A4AE; font-size: 12px;
        }
        QPushButton:hover { background: rgba(79,195,247,0.08); color: #E0E0E0; }
    )");
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    m_sidebarLayout->addWidget(m_themeBtn);

    // 버전 레이블
    QLabel *ver = new QLabel("v1.0.0");
    ver->setStyleSheet("color: #546E7A; font-size: 11px; padding: 4px;");
    ver->setAlignment(Qt::AlignCenter);
    m_sidebarLayout->addWidget(ver);

    // 구분선
    QFrame *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: rgba(255,255,255,0.06);");

    // ── 우측: 헤더 + 알림 패널 + 컨텐츠 ────────────────────────
    auto *rightWidget = new QWidget;
    auto *rightVBox   = new QVBoxLayout(rightWidget);
    rightVBox->setContentsMargins(0, 0, 0, 0);
    rightVBox->setSpacing(0);

    setupHeader();
    rightVBox->addWidget(m_headerWidget);
    rightVBox->addWidget(m_notifPanel);
    rightVBox->addWidget(m_stack, 1);

    m_mainLayout->addWidget(m_sidebar);
    m_mainLayout->addWidget(sep);
    m_mainLayout->addWidget(rightWidget, 1);

    // 첫 번째 뷰 활성화
    if (!m_navBtns.isEmpty()) {
        m_navBtns.first()->setChecked(true);
        m_stack->setCurrentIndex(0);
        updateNavTitle(0);
    }
}

// 뷰 인덱스 → {제목, 부제목}
static const QList<QPair<QString,QString>> NAV_TITLE_MAP = {
    {"조명 효과",  "전체 동기화와 기기별 조명을 조정합니다"},
    {"씬 관리",    "현재 기기 상태를 씬으로 저장하고 원클릭으로 불러옵니다"},
    {"키보드 에디터","키 하나하나에 색상을 지정합니다"},
    {"레이아웃",   "장치의 위치, 크기, 방향을 실제 배치에 맞춥니다"},
    {"오디오",     "마이크 입력 기반 음악 반응 효과를 조정합니다"},
    {"사용자 프로필","상황별 조명 설정을 원클릭으로 전환합니다"},
    {"기기",       "감지된 장치와 장치별 조명 설정을 관리합니다"},
    {"자동화",     "시간·프로세스 기반 씬 자동 전환을 설정합니다"},
    {"모니터링",   "실시간 하드웨어 센서 대시보드"},
    {"SDK",        "연동 가능한 드라이버 상태를 확인합니다"},
    {"설정",       "앱 동작, 업데이트, OpenRGB 연결을 관리합니다"},
};

void MainWindow::setupHeader()
{
    m_headerWidget = new QWidget;
    m_headerWidget->setFixedHeight(64);
    m_headerWidget->setStyleSheet("background:#111827;border-bottom:1px solid rgba(255,255,255,0.07);");
    auto *hLayout = new QHBoxLayout(m_headerWidget);
    hLayout->setContentsMargins(20, 0, 16, 0);
    hLayout->setSpacing(12);

    // 좌측: 뷰 제목/부제목
    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(1);
    m_headerTitle    = new QLabel("조명 효과");
    m_headerSubtitle = new QLabel("전체 동기화와 기기별 조명을 조정합니다");
    m_headerTitle->setStyleSheet("font-size:16px;font-weight:bold;color:#E0E0E0;");
    m_headerSubtitle->setStyleSheet("font-size:11px;color:#546E7A;");
    titleCol->addWidget(m_headerTitle);
    titleCol->addWidget(m_headerSubtitle);
    hLayout->addLayout(titleCol, 1);

    // 상태 배지
    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(6);

    m_statusDot = new QLabel;
    m_statusDot->setFixedSize(10, 10);
    m_statusDot->setStyleSheet("background:#FFA726;border-radius:5px;");

    m_statusText = new QLabel("SDK 연동 상태 확인 중");
    m_statusText->setStyleSheet("font-size:11px;color:#90A4AE;");

    auto *reconnectBtn = new QPushButton("다시 연결");
    reconnectBtn->setStyleSheet("font-size:11px;padding:3px 10px;border-radius:6px;"
                                "border:1px solid rgba(79,195,247,0.3);"
                                "background:rgba(79,195,247,0.08);color:#4FC3F7;");
    connect(reconnectBtn, &QPushButton::clicked, m_deviceManager, &DeviceManager::reconnectOpenRGB);

    statusRow->addWidget(m_statusDot);
    statusRow->addWidget(m_statusText);
    statusRow->addWidget(reconnectBtn);
    hLayout->addLayout(statusRow);

    // 업데이트 배너 (숨김 기본)
    m_updateBanner = new QWidget;
    m_updateBanner->setVisible(false);
    m_updateBanner->setStyleSheet("background:rgba(0,180,255,0.12);border:1px solid rgba(0,180,255,0.3);"
                                   "border-radius:8px;padding:2px;");
    auto *bannerRow = new QHBoxLayout(m_updateBanner);
    bannerRow->setContentsMargins(8,2,8,2); bannerRow->setSpacing(6);
    m_updateBannerText = new QLabel;
    m_updateBannerText->setStyleSheet("font-size:11px;color:#4FC3F7;font-weight:bold;");
    auto *updateNowBtn = new QPushButton("지금 업데이트");
    updateNowBtn->setStyleSheet("font-size:10px;padding:2px 8px;border-radius:5px;"
                                 "background:#0288D1;color:white;border:none;font-weight:bold;");
    auto *closeBannerBtn = new QPushButton("✕");
    closeBannerBtn->setStyleSheet("background:none;border:none;color:#90A4AE;font-size:12px;");
    connect(closeBannerBtn, &QPushButton::clicked, this, [this]() { m_updateBanner->setVisible(false); });
    bannerRow->addWidget(m_updateBannerText);
    bannerRow->addWidget(updateNowBtn);
    bannerRow->addWidget(closeBannerBtn);
    hLayout->addWidget(m_updateBanner);

    // 알림 벨
    auto *bellWrap = new QWidget;
    auto *bellLayout = new QHBoxLayout(bellWrap);
    bellLayout->setContentsMargins(0,0,0,0); bellLayout->setSpacing(0);
    m_notifBellBtn = new QPushButton("🔔");
    m_notifBellBtn->setFixedSize(34, 34);
    m_notifBellBtn->setStyleSheet("font-size:16px;background:none;border:none;border-radius:17px;");
    m_notifBadge = new QLabel("0");
    m_notifBadge->setFixedSize(16, 16);
    m_notifBadge->setAlignment(Qt::AlignCenter);
    m_notifBadge->setStyleSheet("background:#EF5350;color:white;border-radius:8px;font-size:9px;font-weight:bold;");
    m_notifBadge->setVisible(false);
    bellLayout->addWidget(m_notifBellBtn);
    bellLayout->addWidget(m_notifBadge);
    connect(m_notifBellBtn, &QPushButton::clicked, this, &MainWindow::toggleNotifPanel);
    hLayout->addWidget(bellWrap);

    // ── 알림 패널 ──────────────────────────────────────────────
    m_notifPanel = new QWidget;
    m_notifPanel->setVisible(false);
    m_notifPanel->setStyleSheet("background:#111827;border-bottom:1px solid rgba(255,255,255,0.08);");
    auto *notifLayout = new QVBoxLayout(m_notifPanel);
    notifLayout->setContentsMargins(16,8,16,8); notifLayout->setSpacing(4);

    auto *notifHeader = new QHBoxLayout;
    auto *notifTitle = new QLabel("🔔 알림 센터");
    notifTitle->setStyleSheet("font-size:12px;font-weight:bold;color:#90A4AE;");
    auto *clearBtn  = new QPushButton("전체 지우기");
    clearBtn->setStyleSheet("font-size:10px;padding:2px 8px;border-radius:5px;"
                             "background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);color:#90A4AE;");
    auto *closeNotif = new QPushButton("✕");
    closeNotif->setStyleSheet("background:none;border:none;color:#546E7A;font-size:12px;");
    connect(clearBtn,   &QPushButton::clicked, this, &MainWindow::clearNotifications);
    connect(closeNotif, &QPushButton::clicked, this, &MainWindow::toggleNotifPanel);
    notifHeader->addWidget(notifTitle);
    notifHeader->addStretch();
    notifHeader->addWidget(clearBtn);
    notifHeader->addWidget(closeNotif);
    notifLayout->addLayout(notifHeader);

    auto *notifScroll = new QScrollArea;
    notifScroll->setWidgetResizable(true);
    notifScroll->setMaximumHeight(180);
    notifScroll->setStyleSheet("QScrollArea{border:none;}");
    auto *notifContainer = new QWidget;
    m_notifListLayout = new QVBoxLayout(notifContainer);
    m_notifListLayout->setSpacing(3);
    m_notifListLayout->setContentsMargins(0,0,0,0);
    auto *emptyLbl = new QLabel("알림이 없습니다.");
    emptyLbl->setStyleSheet("color:#546E7A;font-size:11px;padding:8px;");
    emptyLbl->setObjectName("notif-empty");
    m_notifListLayout->addWidget(emptyLbl);
    m_notifListLayout->addStretch();
    notifScroll->setWidget(notifContainer);
    notifLayout->addWidget(notifScroll);

    // OpenRGB 연결 상태 → 헤더 dot 업데이트
    connect(m_deviceManager, &DeviceManager::openRGBConnected, this, [this](int n) {
        m_statusDot->setStyleSheet("background:#66BB6A;border-radius:5px;");
        m_statusText->setText(QString("OpenRGB 연결됨 — %1개 기기").arg(n));
        addNotification("success", QString("OpenRGB 연결됨 — %1개 기기 감지").arg(n));
    });
    connect(m_deviceManager, &DeviceManager::openRGBDisconnected, this, [this]() {
        m_statusDot->setStyleSheet("background:#EF5350;border-radius:5px;");
        m_statusText->setText("OpenRGB 연결 끊김");
        addNotification("warn", "OpenRGB 연결 끊김 — 재연결 시도 중...");
    });
}

void MainWindow::updateNavTitle(int idx)
{
    if (idx < 0 || idx >= NAV_TITLE_MAP.size()) return;
    m_headerTitle->setText(NAV_TITLE_MAP[idx].first);
    m_headerSubtitle->setText(NAV_TITLE_MAP[idx].second);
}

void MainWindow::toggleNotifPanel()
{
    m_notifOpen = !m_notifOpen;
    m_notifPanel->setVisible(m_notifOpen);
}

void MainWindow::addNotification(const QString &type, const QString &msg)
{
    // empty 레이블 제거
    if (auto *empty = m_notifPanel->findChild<QLabel*>("notif-empty"))
        empty->setVisible(false);

    QString icon = (type == "success") ? "✅" : (type == "warn") ? "⚠️" : "ℹ️";
    QString time = QTime::currentTime().toString("HH:mm:ss");
    auto *item = new QLabel(QString("%1 %2  <span style='color:#546E7A;font-size:10px;'>%3</span>")
                             .arg(icon).arg(msg).arg(time));
    item->setTextFormat(Qt::RichText);
    item->setStyleSheet("font-size:11px;color:#B0BEC5;padding:4px 6px;"
                        "background:rgba(255,255,255,0.03);border-radius:6px;");
    item->setWordWrap(true);

    // stretch 앞에 삽입
    int insertAt = m_notifListLayout->count() - 1;
    m_notifListLayout->insertWidget(qMax(0, insertAt), item);

    ++m_notifCount;
    m_notifBadge->setText(QString::number(m_notifCount));
    m_notifBadge->setVisible(true);
}

void MainWindow::clearNotifications()
{
    // stretch와 empty 빼고 모두 제거
    QList<QWidget*> toRemove;
    for (int i = 0; i < m_notifListLayout->count(); ++i) {
        auto *w = m_notifListLayout->itemAt(i)->widget();
        if (w && w->objectName() != "notif-empty") toRemove << w;
    }
    for (auto *w : toRemove) { m_notifListLayout->removeWidget(w); w->deleteLater(); }

    if (auto *empty = m_notifPanel->findChild<QLabel*>("notif-empty"))
        empty->setVisible(true);

    m_notifCount = 0;
    m_notifBadge->setVisible(false);
}

void MainWindow::addNavItem(const QString &icon, const QString &label, QWidget *view)
{
    QPushButton *btn = new QPushButton(icon + "  " + label);
    btn->setCheckable(true);
    btn->setFixedHeight(40);
    btn->setStyleSheet(R"(
        QPushButton {
            text-align: left;
            padding: 0 12px;
            border-radius: 8px;
            border: none;
            background: transparent;
            color: #90A4AE;
            font-size: 13px;
        }
        QPushButton:hover { background: rgba(79,195,247,0.08); color: #E0E0E0; }
        QPushButton:checked { background: rgba(79,195,247,0.15); color: #4FC3F7; font-weight: bold; }
    )");

    int idx = m_stack->addWidget(view);
    m_navBtns.append(btn);
    m_sidebarLayout->addWidget(btn);

    connect(btn, &QPushButton::clicked, this, [this, idx]() {
        onNavItemClicked(idx);
    });
}

void MainWindow::onNavItemClicked(int index)
{
    m_stack->setCurrentIndex(index);
    for (int i = 0; i < m_navBtns.size(); ++i)
        m_navBtns[i]->setChecked(i == index);
    updateNavTitle(index);
    if (m_notifOpen) toggleNotifPanel(); // 뷰 전환 시 알림 패널 닫기
}

// ── 트레이 ──────────────────────────────────────────────────
void MainWindow::setupTray()
{
    // 트레이 아이콘 (프로그래밍 방식으로 생성)
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(255, 60, 60, 200));  p.drawEllipse(0, 0, 40, 40);
    p.setBrush(QColor(60, 220, 60, 200));  p.drawEllipse(24, 0, 40, 40);
    p.setBrush(QColor(60, 120, 255, 200)); p.drawEllipse(12, 24, 40, 40);
    p.end();

    m_tray = new QSystemTrayIcon(QIcon(pm), this);
    m_trayMenu = new QMenu(this);

    auto *showAct = m_trayMenu->addAction("🌈 ColorDock 열기");
    m_trayMenu->addSeparator();
    auto *closeAct = m_trayMenu->addAction("X 버튼 → 트레이로");
    closeAct->setCheckable(true);
    closeAct->setChecked(m_closeToTray);
    m_trayMenu->addSeparator();
    auto *quitAct = m_trayMenu->addAction("종료");

    connect(showAct,  &QAction::triggered, this, &MainWindow::showWindow);
    connect(closeAct, &QAction::toggled, this, [this, closeAct](bool checked) {
        m_closeToTray = checked;
        AppSettings::instance().set("closeToTray", checked);
    });
    connect(quitAct,  &QAction::triggered, this, &MainWindow::quitApp);
    connect(m_tray,   &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

    m_tray->setContextMenu(m_trayMenu);
    m_tray->setToolTip("ColorDock RGB");
    m_tray->show();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger)
        showWindow();
}

void MainWindow::showWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::quitApp()
{
    m_tray->hide();
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_closeToTray && m_tray->isVisible()) {
        hide();
        m_tray->showMessage("ColorDock", "트레이에서 실행 중입니다.",
                             QSystemTrayIcon::Information, 2000);
        event->ignore();
    } else {
        quitApp();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && isMinimized()) {
        if (m_closeToTray) {
            hide();
            event->ignore();
            return;
        }
    }
    QMainWindow::changeEvent(event);
}

// ── 자동 업데이트 ────────────────────────────────────────────
void MainWindow::setupUpdateChecker()
{
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &MainWindow::onUpdateAvailable);
    // 30초 후 첫 체크, 이후 6시간마다
    QTimer::singleShot(30000, m_updateChecker, &UpdateChecker::check);
}

void MainWindow::toggleTheme()
{
    m_isDark = !m_isDark;
    applyTheme(m_isDark);
    AppSettings::instance().set("darkTheme", m_isDark);
}

void MainWindow::applyTheme(bool dark)
{
    m_themeBtn->setText(dark ? "🌙  다크 모드" : "☀️  라이트 모드");
    if (dark) {
        m_sidebar->setStyleSheet("background-color: #16213E;");
        qApp->setStyleSheet(
            "QWidget { background-color: #0F1623; color: #E0E0E0; }"
            "QGroupBox { border: 1px solid rgba(255,255,255,0.08); border-radius: 10px;"
            "            margin-top: 8px; padding-top: 8px; color: #90A4AE; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
            "QLineEdit, QComboBox, QSpinBox, QTimeEdit {"
            "    background: rgba(255,255,255,0.05); border: 1px solid rgba(255,255,255,0.12);"
            "    border-radius: 6px; padding: 4px 8px; color: #E0E0E0; }"
            "QScrollBar:vertical { background: transparent; width: 6px; }"
            "QScrollBar::handle:vertical { background: rgba(255,255,255,0.15); border-radius: 3px; }"
            "QPushButton { background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.12);"
            "              border-radius: 6px; padding: 5px 12px; color: #E0E0E0; }"
            "QPushButton:hover { background: rgba(255,255,255,0.1); }"
            "QListWidget { background: transparent; border: none; }"
            "QListWidget::item:selected { background: rgba(79,195,247,0.18); color: #4FC3F7; }"
        );
    } else {
        m_sidebar->setStyleSheet("background-color: #E8EAF6;");
        qApp->setStyleSheet(
            "QWidget { background-color: #F5F5F5; color: #212121; }"
            "QGroupBox { border: 1px solid #BDBDBD; border-radius: 10px;"
            "            margin-top: 8px; padding-top: 8px; color: #616161; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
            "QLineEdit, QComboBox, QSpinBox, QTimeEdit {"
            "    background: #FFFFFF; border: 1px solid #BDBDBD;"
            "    border-radius: 6px; padding: 4px 8px; color: #212121; }"
            "QScrollBar:vertical { background: #EEEEEE; width: 6px; }"
            "QScrollBar::handle:vertical { background: #BDBDBD; border-radius: 3px; }"
            "QPushButton { background: #EEEEEE; border: 1px solid #BDBDBD;"
            "              border-radius: 6px; padding: 5px 12px; color: #212121; }"
            "QPushButton:hover { background: #E0E0E0; }"
            "QListWidget { background: #FFFFFF; border: none; }"
            "QListWidget::item:selected { background: rgba(79,195,247,0.25); color: #0277BD; }"
        );
    }
}

void MainWindow::onUpdateAvailable(const QString &version, const QString &url)
{
    m_latestZipUrl = url;
    m_updateBannerText->setText(QString("🆕 v%1 업데이트 있음").arg(version));
    m_updateBanner->setVisible(true);
    addNotification("info", QString("v%1 업데이트가 있습니다.").arg(version));

    m_tray->showMessage(
        "ColorDock 업데이트",
        QString("v%1 업데이트가 있습니다.").arg(version),
        QSystemTrayIcon::Information, 8000
    );
    connect(m_tray, &QSystemTrayIcon::messageClicked, this, [url]() {
        QDesktopServices::openUrl(QUrl(url));
    });
}

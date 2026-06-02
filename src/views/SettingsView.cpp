#include "SettingsView.h"
#include "../utils/AppSettings.h"
#include "../utils/UpdateChecker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>

SettingsView::SettingsView(QWidget *parent) : QWidget(parent)
{
    setupUi();
    loadSettings();
}

void SettingsView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 헤더
    auto *title = new QLabel("⚙️  설정");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    mainLayout->addWidget(title);

    // OpenRGB 연결
    auto *orgbBox = new QGroupBox("🔌 OpenRGB 연결");
    auto *orgbForm = new QFormLayout(orgbBox);
    m_orgbHost = new QLineEdit;
    m_orgbHost->setPlaceholderText("127.0.0.1");
    m_orgbPort = new QSpinBox;
    m_orgbPort->setRange(1, 65535);
    m_orgbPort->setValue(6742);

    auto *saveOrgbBtn = new QPushButton("💾 저장");
    saveOrgbBtn->setStyleSheet("background:#0277BD;");
    connect(saveOrgbBtn, &QPushButton::clicked, this, &SettingsView::onSaveOpenRGB);

    auto *orgbRow = new QHBoxLayout;
    orgbRow->addWidget(m_orgbHost, 1);
    orgbRow->addWidget(new QLabel("포트:"));
    orgbRow->addWidget(m_orgbPort);
    orgbRow->addWidget(saveOrgbBtn);

    orgbForm->addRow("호스트:", orgbRow);
    mainLayout->addWidget(orgbBox);

    // 시작 동작
    auto *startBox = new QGroupBox("🚀 시작 동작");
    auto *startLayout = new QVBoxLayout(startBox);
    m_closeToTray    = new QCheckBox("창 닫기 시 트레이로 숨기기");
    m_startMinimized = new QCheckBox("시작 시 최소화 (트레이)");

    auto *effectRow = new QHBoxLayout;
    effectRow->addWidget(new QLabel("시작 시 효과:"));
    m_startupEffect = new QComboBox;
    m_startupEffect->addItem("🌈 무지개 순환", "rainbow");
    m_startupEffect->addItem("💫 브리딩",       "breathing");
    m_startupEffect->addItem("⚪ 단색 고정",   "static");
    m_startupEffect->addItem("🌊 컬러 웨이브", "wave");
    m_startupEffect->addItem("⬛ 끄기",        "disabled");
    effectRow->addWidget(m_startupEffect, 1);

    connect(m_closeToTray,    &QCheckBox::toggled, this, [](bool v) {
        AppSettings::instance().set("closeToTray", v);
    });
    connect(m_startMinimized, &QCheckBox::toggled, this, [](bool v) {
        AppSettings::instance().set("startMinimized", v);
    });
    connect(m_startupEffect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        AppSettings::instance().set("startupEffect", m_startupEffect->currentData().toString());
    });

    startLayout->addWidget(m_closeToTray);
    startLayout->addWidget(m_startMinimized);
    startLayout->addLayout(effectRow);
    mainLayout->addWidget(startBox);

    // 업데이트
    auto *updateBox = new QGroupBox("🔄 업데이트");
    auto *updateLayout = new QVBoxLayout(updateBox);
    m_autoUpdate = new QCheckBox("시작 시 자동으로 업데이트 확인");
    connect(m_autoUpdate, &QCheckBox::toggled, this, [](bool v) {
        AppSettings::instance().set("autoUpdate", v);
    });

    auto *updateBtnRow = new QHBoxLayout;
    auto *checkBtn = new QPushButton("🔍 지금 확인");
    connect(checkBtn, &QPushButton::clicked, this, &SettingsView::onCheckUpdate);
    m_updateStatusLabel = new QLabel;
    m_updateStatusLabel->setStyleSheet("color:#90A4AE;font-size:12px;");
    updateBtnRow->addWidget(checkBtn);
    updateBtnRow->addWidget(m_updateStatusLabel, 1);

    updateLayout->addWidget(m_autoUpdate);
    updateLayout->addLayout(updateBtnRow);
    mainLayout->addWidget(updateBox);

    // 앱 정보
    auto *infoBox = new QGroupBox("ℹ️ 앱 정보");
    auto *infoLayout = new QVBoxLayout(infoBox);
    m_versionLabel = new QLabel("ColorDock RGB  v1.0.0");
    m_versionLabel->setStyleSheet("font-weight:bold;");
    auto *repoLabel = new QLabel("<a href='https://github.com/impressionistfisherman/color-dock' style='color:#4FC3F7;'>GitHub 저장소</a>");
    repoLabel->setOpenExternalLinks(true);
    auto *qtLabel = new QLabel(QString("Qt %1 (MinGW)").arg(QT_VERSION_STR));
    qtLabel->setStyleSheet("color:#546E7A;font-size:11px;");

    infoLayout->addWidget(m_versionLabel);
    infoLayout->addWidget(repoLabel);
    infoLayout->addWidget(qtLabel);
    mainLayout->addWidget(infoBox);

    mainLayout->addStretch();
}

void SettingsView::loadSettings()
{
    auto &s = AppSettings::instance();
    m_orgbHost->setText(s.get("openrgbHost", "127.0.0.1").toString());
    m_orgbPort->setValue(s.get("openrgbPort", 6742).toInt());
    m_closeToTray->setChecked(s.get("closeToTray", true).toBool());
    m_startMinimized->setChecked(s.get("startMinimized", false).toBool());
    m_autoUpdate->setChecked(s.get("autoUpdate", true).toBool());

    QString startEff = s.get("startupEffect", "rainbow").toString();
    for (int i = 0; i < m_startupEffect->count(); ++i) {
        if (m_startupEffect->itemData(i).toString() == startEff) {
            m_startupEffect->setCurrentIndex(i);
            break;
        }
    }
}

void SettingsView::onSaveOpenRGB()
{
    auto &s = AppSettings::instance();
    s.set("openrgbHost", m_orgbHost->text().trimmed());
    s.set("openrgbPort", m_orgbPort->value());
    m_updateStatusLabel->setText("✅ OpenRGB 설정 저장됨. 재시작 시 적용됩니다.");
    m_updateStatusLabel->setStyleSheet("color:#66BB6A;font-size:12px;");
}

void SettingsView::onCheckUpdate()
{
    m_updateStatusLabel->setText("🔍 확인 중...");
    m_updateStatusLabel->setStyleSheet("color:#90A4AE;font-size:12px;");

    auto *checker = new UpdateChecker(this);
    connect(checker, &UpdateChecker::updateAvailable, this,
            [this](const QString &ver, const QString &url) {
        m_updateStatusLabel->setText(QString("🆕 v%1 업데이트 있음!").arg(ver));
        m_updateStatusLabel->setStyleSheet("color:#FFA726;font-size:12px;font-weight:bold;");
        QDesktopServices::openUrl(QUrl(url));
    });
    connect(checker, &UpdateChecker::noUpdate, this, [this]() {
        m_updateStatusLabel->setText("✅ 최신 버전입니다.");
        m_updateStatusLabel->setStyleSheet("color:#66BB6A;font-size:12px;");
    });
    checker->check();
}

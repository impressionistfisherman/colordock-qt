#include "SdkView.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QGroupBox>

// 50개 제조사 목록 {id, displayName}
const QList<QPair<QString,QString>> SdkView::SDK_BRANDS = {
    {"asus","ASUS"},{"msi","MSI"},{"gigabyte","Gigabyte"},{"asrock","ASRock"},{"biostar","Biostar"},
    {"evga","EVGA"},{"nzxt","NZXT"},{"nvidia","NVIDIA"},{"amd","AMD"},{"zotac","Zotac"},
    {"colorful","Colorful"},{"pny","PNY"},{"inno3d","Inno3D"},{"galax","Galax"},{"palit","Palit"},
    {"gainward","Gainward"},{"sapphire","Sapphire"},{"powercolor","PowerColor"},{"xfx","XFX"},
    {"samsung","Samsung"},{"skhynix","SK Hynix"},{"micron","Micron"},{"gskill","G.Skill"},
    {"corsair","Corsair"},{"kingston","Kingston"},{"teamgroup","TeamGroup"},{"adata","ADATA"},
    {"geil","GeIL"},{"klevv","KLEVV"},{"crucial","Crucial"},{"razer","Razer"},
    {"logitech","Logitech"},{"steelseries","SteelSeries"},{"roccat","ROCCAT"},{"hyperx","HyperX"},
    {"glorious","Glorious"},{"keychron","Keychron"},{"wooting","Wooting"},{"alienware","Alienware"},
    {"thermaltake","Thermaltake"},{"philips_hue","Philips Hue"},{"nanoleaf","Nanoleaf"},
    {"govee","Govee"},{"lifx","LIFX"},{"wled","WLED"},{"openrgb","OpenRGB"},
    {"lianli","Lian Li"},{"coolermaster","Cooler Master"},{"aula","AULA"},{"vgn","VGN"},
};

// ── SdkChip ───────────────────────────────────────────────────
SdkChip::SdkChip(const QString &brand, const QString &status, QWidget *parent)
    : QWidget(parent), m_brand(brand), m_status(status)
{
    setFixedSize(110, 38);
    setToolTip(QString("%1: %2").arg(brand).arg(status));
}

void SdkChip::setStatus(const QString &status)
{
    m_status = status;
    setToolTip(QString("%1: %2").arg(m_brand).arg(status));
    update();
}

void SdkChip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor dot, bg, border;
    if (m_status == "Connected") {
        dot    = QColor(0, 230, 118);
        bg     = QColor(0, 230, 118, 18);
        border = QColor(0, 230, 118, 60);
    } else {
        dot    = QColor(80, 100, 120);
        bg     = QColor(255, 255, 255, 5);
        border = QColor(255, 255, 255, 15);
    }

    // 배경
    p.setPen(QPen(border, 1));
    p.setBrush(bg);
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 8, 8);

    // 상태 dot
    p.setPen(Qt::NoPen);
    p.setBrush(dot);
    p.drawEllipse(QRect(8, height()/2 - 4, 8, 8));

    // 브랜드 이름
    p.setPen(m_status == "Connected" ? QColor(200,240,200) : QColor(100,120,140));
    p.setFont(QFont("Segoe UI", 8, m_status == "Connected" ? QFont::Bold : QFont::Normal));
    p.drawText(QRect(22, 0, width()-26, height()), Qt::AlignVCenter | Qt::AlignLeft, m_brand);
}

// ── SdkView ───────────────────────────────────────────────────
SdkView::SdkView(DeviceManager *dm, QWidget *parent)
    : QWidget(parent), m_dm(dm)
{
    setupUi();

    // OpenRGB 연결 시 openrgb 칩 업데이트
    connect(dm, &DeviceManager::openRGBConnected, this, [this](int) {
        updateStatus("openrgb", "Connected");
    });
    connect(dm, &DeviceManager::openRGBDisconnected, this, [this]() {
        updateStatus("openrgb", "Demo");
    });
    // 기기 변경 시 연결된 브랜드 업데이트
    connect(dm, &DeviceManager::devicesChanged, this, [this]() {
        int connected = 0;
        for (auto *chip : m_chips) {
            if (chip->toolTip().contains("Connected")) ++connected;
        }
        m_summaryLabel->setText(QString("✅ %1개 연결됨  /  %2개 지원")
            .arg(connected).arg(m_chips.size()));
    });
}

void SdkView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 헤더
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("🛠️  하드웨어 SDK 연동 상태");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    m_summaryLabel = new QLabel(QString("0개 연결됨  /  %1개 지원").arg(SDK_BRANDS.size()));
    m_summaryLabel->setStyleSheet("color:#90A4AE;font-size:12px;");
    header->addWidget(title);
    header->addStretch();
    header->addWidget(m_summaryLabel);
    mainLayout->addLayout(header);

    auto *desc = new QLabel(
        "연결된 기기는 초록색, Demo(미설치/미연결)는 회색으로 표시됩니다.\n"
        "OpenRGB를 서버 모드로 실행하면 메인보드·RAM·GPU가 자동 감지됩니다."
    );
    desc->setStyleSheet("color:#546E7A;font-size:11px;");
    mainLayout->addWidget(desc);

    // 칩 그리드
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border:none; }");

    auto *gridWidget = new QWidget;
    auto *grid = new QGridLayout(gridWidget);
    grid->setSpacing(8);
    grid->setContentsMargins(4, 4, 4, 4);

    int col = 0, row = 0;
    const int COLS = 6;
    for (const auto &[id, name] : SDK_BRANDS) {
        auto *chip = new SdkChip(name, "Demo");
        m_chips[id] = chip;
        grid->addWidget(chip, row, col);
        if (++col >= COLS) { col = 0; ++row; }
    }

    scroll->setWidget(gridWidget);
    mainLayout->addWidget(scroll, 1);
}

void SdkView::updateStatus(const QString &brand, const QString &status)
{
    if (m_chips.contains(brand)) {
        m_chips[brand]->setStatus(status);
    }
    // 요약 업데이트
    int connected = 0;
    for (auto *chip : m_chips) {
        if (chip->toolTip().contains("Connected")) ++connected;
    }
    m_summaryLabel->setText(QString("✅ %1개 연결됨  /  %2개 지원")
        .arg(connected).arg(m_chips.size()));
}

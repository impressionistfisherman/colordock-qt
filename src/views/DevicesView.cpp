#include "DevicesView.h"
#include "HidScannerDialog.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QGroupBox>
#include <QColorDialog>
#include <QPainter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>

// ── 카테고리 → 타입 매핑 ─────────────────────────────────────
static bool matchCategory(const QString &cat, const QString &type)
{
    if (cat == "all") return true;
    if (cat == "keyboard")    return type.contains("Keyboard", Qt::CaseInsensitive);
    if (cat == "mouse")       return type.contains("Mouse", Qt::CaseInsensitive);
    if (cat == "component")   return type.contains("Motherboard") || type.contains("GPU")
                                     || type.contains("RAM") || type.contains("Memory")
                                     || type.contains("Fan") || type.contains("Component");
    if (cat == "smart-light") return type.contains("Smart Light") || type.contains("LED Strip");
    if (cat == "custom")      return type.contains("Custom") || type.contains("Other");
    return false;
}

// ── GradientEditorDialog ─────────────────────────────────────
GradientEditorDialog::GradientEditorDialog(const QString &deviceId, int ledCount,
                                            const QList<QColor> &current, QWidget *parent)
    : QWidget(parent, Qt::Dialog), m_deviceId(deviceId)
{
    setWindowTitle("🎨 그라디언트 에디터 — " + deviceId);
    setMinimumSize(560, 340);
    m_colors = current;
    while (m_colors.size() < ledCount)
        m_colors << QColor(0,180,255);

    auto *mainLay = new QVBoxLayout(this);

    // 프리셋
    auto *presetRow = new QHBoxLayout;
    for (const QString &name : {"🌈 무지개","🔥 파이어","🌊 오션","⬛ 끄기"}) {
        auto *btn = new QPushButton(name);
        btn->setStyleSheet("background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.12);"
                           "border-radius:6px;padding:4px 10px;");
        connect(btn, &QPushButton::clicked, this, [this, name]() { onPreset(name); });
        presetRow->addWidget(btn);
    }
    presetRow->addStretch();
    mainLay->addLayout(presetRow);

    // 페인트 색상
    auto *paintRow = new QHBoxLayout;
    paintRow->addWidget(new QLabel("페인트 색상:"));
    auto *colorBtn = new QPushButton;
    colorBtn->setFixedSize(36, 28);
    colorBtn->setStyleSheet(QString("background:%1;border-radius:6px;border:1px solid rgba(255,255,255,0.2);")
                             .arg(m_paintColor.name()));
    connect(colorBtn, &QPushButton::clicked, this, [this, colorBtn]() {
        QColor c = QColorDialog::getColor(m_paintColor, this);
        if (c.isValid()) {
            m_paintColor = c;
            colorBtn->setStyleSheet(QString("background:%1;border-radius:6px;border:1px solid rgba(255,255,255,0.2);").arg(c.name()));
        }
    });
    paintRow->addWidget(colorBtn);
    paintRow->addStretch();
    mainLay->addLayout(paintRow);

    // LED 그리드
    auto *ledArea = new QScrollArea;
    ledArea->setWidgetResizable(true);
    ledArea->setStyleSheet("QScrollArea{border:none;background:#0D0D1A;}");
    auto *ledWidget = new QWidget;
    ledWidget->setStyleSheet("background:#0D0D1A;");
    auto *ledGrid = new QGridLayout(ledWidget);
    ledGrid->setSpacing(4);

    int cols = qMin(ledCount, 24);
    for (int i = 0; i < ledCount && i < m_colors.size(); ++i) {
        auto *btn = new QPushButton;
        btn->setFixedSize(22, 22);
        btn->setStyleSheet(QString("background:%1;border-radius:4px;border:1px solid rgba(255,255,255,0.15);")
                            .arg(m_colors[i].name()));
        m_ledBtns << btn;
        connect(btn, &QPushButton::clicked, this, [this, i]() { onLedClicked(i); });
        ledGrid->addWidget(btn, i / cols, i % cols);
    }
    ledArea->setWidget(ledWidget);
    mainLay->addWidget(ledArea, 1);

    // 버튼
    auto *btns = new QHBoxLayout;
    auto *applyBtn  = new QPushButton("✅ 적용");
    auto *cancelBtn = new QPushButton("취소");
    applyBtn->setStyleSheet("background:#0277BD;font-weight:bold;");
    connect(applyBtn,  &QPushButton::clicked, this, [this]() {
        emit applied(m_deviceId, m_colors);
        close();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QWidget::close);
    btns->addStretch();
    btns->addWidget(cancelBtn);
    btns->addWidget(applyBtn);
    mainLay->addLayout(btns);
}

void GradientEditorDialog::onLedClicked(int idx)
{
    m_colors[idx] = m_paintColor;
    m_ledBtns[idx]->setStyleSheet(
        QString("background:%1;border-radius:4px;border:1px solid rgba(255,255,255,0.15);")
        .arg(m_paintColor.name()));
}

void GradientEditorDialog::onPreset(const QString &name)
{
    int n = m_colors.size();
    for (int i = 0; i < n; ++i) {
        double t = (double)i / qMax(1, n-1);
        QColor c;
        if (name.contains("무지개"))
            c = QColor::fromHsvF(t * 300.0/360.0, 1.0, 1.0);
        else if (name.contains("파이어"))
            c = (t < 0.33) ? QColor(255,255,0) : (t < 0.66) ? QColor(255,130,0) : QColor(255,30,0);
        else if (name.contains("오션"))
            c = (t < 0.33) ? QColor(0,50,255)  : (t < 0.66) ? QColor(0,170,255) : QColor(0,255,230);
        else
            c = QColor(0,0,0);
        m_colors[i] = c;
        m_ledBtns[i]->setStyleSheet(
            QString("background:%1;border-radius:4px;border:1px solid rgba(255,255,255,0.15);")
            .arg(c.name()));
    }
}

// ── DeviceCard ────────────────────────────────────────────────
DeviceCard::DeviceCard(const RGBDevice &dev, QWidget *parent)
    : QWidget(parent), m_id(dev.id), m_ledCount(dev.ledCount), m_leds(dev.leds)
{
    setStyleSheet("background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.07);"
                  "border-radius:14px;");
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(6);

    // 타입 아이콘 매핑
    auto typeIcon = [](const QString &t) -> QString {
        if (t.contains("Keyboard"))   return "⌨️";
        if (t.contains("Mouse"))      return "🖱️";
        if (t.contains("Motherboard"))return "🖥️";
        if (t.contains("GPU"))        return "🎮";
        if (t.contains("RAM")||t.contains("Memory")) return "💾";
        if (t.contains("Fan"))        return "🌀";
        if (t.contains("Smart"))      return "💡";
        if (t.contains("Strip"))      return "💡";
        return "💻";
    };

    // 헤더
    auto *header = new QHBoxLayout;
    auto *iconLbl = new QLabel(typeIcon(dev.type));
    iconLbl->setStyleSheet("font-size:18px;");
    auto *nameLbl = new QLabel(dev.name);
    nameLbl->setStyleSheet("font-size:12px;font-weight:bold;color:#E0E0E0;");
    nameLbl->setWordWrap(true);
    auto *srcBadge = new QLabel(dev.openrgbSource ? "OpenRGB" : "SDK");
    srcBadge->setStyleSheet(dev.openrgbSource
        ? "font-size:9px;background:rgba(0,230,118,0.15);color:#66BB6A;border:1px solid rgba(0,230,118,0.3);border-radius:4px;padding:1px 5px;"
        : "font-size:9px;background:rgba(79,195,247,0.1);color:#4FC3F7;border:1px solid rgba(79,195,247,0.25);border-radius:4px;padding:1px 5px;");
    header->addWidget(iconLbl);
    header->addWidget(nameLbl, 1);
    header->addWidget(srcBadge);
    lay->addLayout(header);

    // 메타
    auto *meta = new QLabel(QString("제조사: %1  |  타입: %2  |  LED: %3개")
        .arg(dev.manufacturer).arg(dev.type).arg(dev.ledCount));
    meta->setStyleSheet("font-size:10px;color:#607D8B;");
    lay->addWidget(meta);

    // LED 컬러 바
    m_colorBar = new QLabel;
    m_colorBar->setFixedHeight(6);
    m_colorBar->setStyleSheet("border-radius:3px;");
    lay->addWidget(m_colorBar);
    if (!dev.leds.isEmpty()) setColor(dev.leds.first());

    // 버튼
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(6);

    auto *colorBtn = new QPushButton("색상");
    colorBtn->setStyleSheet("font-size:10px;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:6px;padding:3px 10px;");
    connect(colorBtn, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(m_leds.isEmpty() ? QColor(0,180,255) : m_leds.first(), this);
        if (c.isValid()) {
            setColor(c);
            emit colorChangeRequested(m_id, c);
        }
    });

    auto *gradBtn = new QPushButton("🎨 그라디언트");
    gradBtn->setStyleSheet("font-size:10px;background:rgba(79,195,247,0.08);border:1px solid rgba(79,195,247,0.2);border-radius:6px;padding:3px 10px;color:#4FC3F7;");
    connect(gradBtn, &QPushButton::clicked, this, [this]() {
        emit gradientRequested(m_id, m_ledCount, m_leds);
    });

    btnRow->addStretch();
    btnRow->addWidget(colorBtn);
    btnRow->addWidget(gradBtn);
    lay->addLayout(btnRow);
}

void DeviceCard::setColor(const QColor &c)
{
    if (m_leds.isEmpty()) m_leds << c;
    else m_leds[0] = c;
    m_colorBar->setStyleSheet(QString("border-radius:3px;background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                                       "stop:0 %1,stop:0.5 %2,stop:1 %1);")
        .arg(c.name()).arg(c.lighter(140).name()));
}

// ── DevicesView ───────────────────────────────────────────────
DevicesView::DevicesView(DeviceManager *dm, QWidget *parent)
    : QWidget(parent), m_dm(dm)
{
    setupUi();
    connect(dm, &DeviceManager::devicesChanged, this, &DevicesView::onDevicesChanged);
    connect(dm, &DeviceManager::openRGBConnected, this, [this](int n) {
        m_statusLabel->setText(QString("✅ OpenRGB 연결됨 — %1개 기기").arg(n));
        m_statusLabel->setStyleSheet("color:#66BB6A;font-weight:bold;font-size:11px;");
    });
    connect(dm, &DeviceManager::openRGBDisconnected, this, [this]() {
        m_statusLabel->setText("⚠ OpenRGB 연결 끊김");
        m_statusLabel->setStyleSheet("color:#FFA726;font-size:11px;");
    });
}

void DevicesView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // 헤더
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("💻  연결된 하드웨어 기기");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    m_countLabel  = new QLabel("0개 기기 연결됨");
    m_countLabel->setStyleSheet("color:#546E7A;font-size:11px;");
    auto *reconnectBtn = new QPushButton("🔄 재연결");
    reconnectBtn->setStyleSheet("background:rgba(79,195,247,0.1);border:1px solid rgba(79,195,247,0.25);"
                                "border-radius:8px;padding:4px 12px;color:#4FC3F7;font-size:11px;");
    connect(reconnectBtn, &QPushButton::clicked, m_dm, &DeviceManager::reconnectOpenRGB);

    auto *addHidBtn = new QPushButton("🔌 미지원 기기 추가");
    addHidBtn->setStyleSheet("background:rgba(0,180,255,0.1);border:1px solid rgba(0,180,255,0.25);"
                              "border-radius:8px;padding:4px 12px;color:#4FC3F7;font-size:11px;font-weight:bold;");
    connect(addHidBtn, &QPushButton::clicked, this, [this]() {
        auto *dlg = new HidScannerDialog(m_dm, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });

    header->addWidget(title);
    header->addStretch();
    header->addWidget(m_countLabel);
    header->addWidget(addHidBtn);
    header->addWidget(reconnectBtn);
    mainLayout->addLayout(header);

    // 검색 + 상태
    auto *searchRow = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("🔍 기기명, 제조사, 분류 키워드로 실시간 필터 검색...");
    m_searchEdit->setStyleSheet("border-radius:10px;padding:6px 12px;font-size:12px;"
                                "background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &DevicesView::onSearchChanged);
    m_statusLabel = new QLabel("🔄 OpenRGB 연결 중...");
    m_statusLabel->setStyleSheet("color:#90A4AE;font-size:11px;");
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(m_statusLabel);
    mainLayout->addLayout(searchRow);

    // 카테고리 필터 칩
    auto *filterRow = new QHBoxLayout;
    filterRow->setSpacing(6);
    struct FilterDef { QString id, label; };
    const QList<FilterDef> filters = {
        {"all",         "전체 기기"},
        {"keyboard",    "⌨️ 키보드"},
        {"mouse",       "🖱️ 마우스"},
        {"component",   "⚙️ PC 부품 (MB/GPU/RAM/팬)"},
        {"smart-light", "💡 스마트 조명"},
        {"custom",      "🔌 커스텀"},
    };
    for (const auto &f : filters) {
        auto *btn = new QPushButton(f.label);
        btn->setCheckable(true);
        btn->setChecked(f.id == "all");
        btn->setStyleSheet(R"(
            QPushButton { font-size:11px; padding:4px 10px; border-radius:14px;
                          border:1px solid rgba(255,255,255,0.12);
                          background:rgba(255,255,255,0.04); color:#90A4AE; }
            QPushButton:checked { background:rgba(79,195,247,0.15);
                                  border-color:rgba(79,195,247,0.4); color:#4FC3F7; }
            QPushButton:hover   { background:rgba(79,195,247,0.08); }
        )");
        connect(btn, &QPushButton::clicked, this, [this, f, btn]() {
            for (auto *b : m_filterBtns) b->setChecked(false);
            btn->setChecked(true);
            onCategoryFilter(f.id);
        });
        m_filterBtns << btn;
        filterRow->addWidget(btn);
    }
    filterRow->addStretch();
    mainLayout->addLayout(filterRow);

    // 카드 스크롤 영역
    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setStyleSheet("QScrollArea{border:none;}");
    m_cardsContainer = new QWidget;
    m_cardsLayout    = new QVBoxLayout(m_cardsContainer);
    m_cardsLayout->setSpacing(8);
    m_cardsLayout->setContentsMargins(0,0,0,0);
    m_cardsLayout->addStretch();
    m_scroll->setWidget(m_cardsContainer);
    mainLayout->addWidget(m_scroll, 1);
}

bool DevicesView::matchesFilter(const RGBDevice &dev) const
{
    if (!m_searchText.isEmpty()) {
        bool hit = dev.name.contains(m_searchText, Qt::CaseInsensitive)
                || dev.manufacturer.contains(m_searchText, Qt::CaseInsensitive)
                || dev.type.contains(m_searchText, Qt::CaseInsensitive);
        if (!hit) return false;
    }
    return matchCategory(m_activeCategory, dev.type);
}

void DevicesView::renderCards()
{
    // 기존 카드 제거
    QLayoutItem *item;
    while ((item = m_cardsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const auto &devs = m_dm->devices();
    int shown = 0;
    for (const auto &dev : devs) {
        if (!matchesFilter(dev)) continue;
        auto *card = new DeviceCard(dev);
        connect(card, &DeviceCard::colorChangeRequested, this, [this](const QString &id, const QColor &c) {
            m_dm->setDeviceColor(id, c);
        });
        connect(card, &DeviceCard::gradientRequested, this,
                [this](const QString &id, int n, const QList<QColor> &leds) {
            auto *dlg = new GradientEditorDialog(id, n, leds, this);
            connect(dlg, &GradientEditorDialog::applied, this,
                    [this](const QString &did, const QList<QColor> &colors) {
                // TODO: DeviceManager per-LED apply
                Q_UNUSED(did); Q_UNUSED(colors);
            });
            dlg->show();
        });
        m_cardsLayout->addWidget(card);
        ++shown;
    }

    if (shown == 0) {
        auto *empty = new QLabel(devs.isEmpty()
            ? "연결된 기기가 없습니다.\nOpenRGB를 서버 모드로 실행하면 메인보드·RAM·GPU가 자동 감지됩니다."
            : "검색 결과가 없습니다.");
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color:#546E7A;font-size:12px;padding:40px;");
        empty->setWordWrap(true);
        m_cardsLayout->addWidget(empty);
    }
    m_cardsLayout->addStretch();
    m_countLabel->setText(QString("%1개 기기 연결됨").arg(devs.size()));
}

void DevicesView::onDevicesChanged()     { renderCards(); }
void DevicesView::onSearchChanged(const QString &t) { m_searchText = t; renderCards(); }
void DevicesView::onCategoryFilter(const QString &c) { m_activeCategory = c; renderCards(); }

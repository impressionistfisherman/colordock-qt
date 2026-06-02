#include "EffectsView.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSlider>
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QtMath>

// ── ColorWheelWidget ─────────────────────────────────────────
ColorWheelWidget::ColorWheelWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(180, 180);
    setMaximumSize(220, 220);
    setCursor(Qt::CrossCursor);
}

void ColorWheelWidget::setColor(const QColor &c)
{
    m_color = c;
    update();
    emit colorChanged(c);
}

void ColorWheelWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int sz = qMin(width(), height());
    QRectF r((width()-sz)/2.0, (height()-sz)/2.0, sz, sz);
    r.adjust(4, 4, -4, -4);

    // 색상환 그리기
    QConicalGradient hue(r.center(), 0);
    for (int i = 0; i <= 360; i += 10)
        hue.setColorAt(i/360.0, QColor::fromHsvF(i/360.0, 1.0, 1.0));
    p.setBrush(hue);
    p.setPen(Qt::NoPen);
    p.drawEllipse(r);

    // 중앙 흰색 (채도 그라데이션)
    QRadialGradient sat(r.center(), r.width()/2);
    sat.setColorAt(0, QColor(255,255,255,255));
    sat.setColorAt(1, QColor(255,255,255,0));
    p.setBrush(sat);
    p.drawEllipse(r);

    // 테두리
    p.setPen(QPen(QColor(255,255,255,40), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(r);

    // 현재 색상 커서
    QPointF pos = colorToPos(m_color);
    p.setPen(QPen(Qt::white, 2));
    p.setBrush(m_color);
    p.drawEllipse(pos, 7, 7);
}

QPointF ColorWheelWidget::colorToPos(const QColor &c) const
{
    int sz = qMin(width(), height()) - 8;
    QPointF center(width()/2.0, height()/2.0);
    double r = sz/2.0 * c.saturationF();
    double a = c.hueF() * 2 * M_PI;
    return center + QPointF(r * cos(a), -r * sin(a));
}

QColor ColorWheelWidget::posToColor(const QPointF &p) const
{
    int sz = qMin(width(), height()) - 8;
    QPointF center(width()/2.0, height()/2.0);
    QPointF d = p - center;
    double r   = sqrt(d.x()*d.x() + d.y()*d.y());
    double maxR = sz/2.0;
    double sat = qMin(r / maxR, 1.0);
    double hue = fmod(atan2(-d.y(), d.x()) / (2*M_PI) + 1.0, 1.0);
    return QColor::fromHsvF(hue, sat, 1.0);
}

void ColorWheelWidget::mousePressEvent(QMouseEvent *e)  { setColor(posToColor(e->pos())); }
void ColorWheelWidget::mouseMoveEvent(QMouseEvent *e)   { if (e->buttons()) setColor(posToColor(e->pos())); }

// ── EffectsView ──────────────────────────────────────────────
EffectsView::EffectsView(DeviceManager *dm, QWidget *parent)
    : QWidget(parent), m_dm(dm)
{
    setupUi();
}

void EffectsView::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ── 좌측: 효과 선택 (10가지) ─────────────────────────────
    auto *leftBox = new QGroupBox("조명 효과 (Sync All)");
    auto *leftLayout = new QGridLayout(leftBox);
    leftLayout->setSpacing(8);

    struct EffectDef { QString icon, name, id; };
    const QList<EffectDef> effects = {
        {"⚪", "단색 고정",   "static"},
        {"🌈", "무지개 순환", "rainbow"},
        {"🫁", "브리딩 효과", "breathing"},
        {"⚡", "스트로브",    "strobe"},
        {"🌊", "컬러 웨이브", "wave"},
        {"🌡️", "하드웨어 센서","sensor"},
        {"🎵", "뮤직 싱크",   "audio"},
        {"🖥️", "화면 복제",   "ambient"},
        {"🎮", "게임 연동",   "game"},
        {"⬛", "조명 끄기",   "off"},
    };

    int row = 0, col = 0;
    for (const auto &e : effects) {
        auto *btn = makeEffectBtn(e.icon, e.name, e.id);
        leftLayout->addWidget(btn, row, col);
        if (++col >= 2) { col = 0; ++row; }
    }

    // ── 우측: 색상 컨트롤 ────────────────────────────────────
    auto *rightWidget = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setSpacing(12);

    // 색상환
    auto *colorBox = new QGroupBox("색상 선택");
    auto *colorLayout = new QVBoxLayout(colorBox);
    m_colorWheel = new ColorWheelWidget;
    colorLayout->addWidget(m_colorWheel, 0, Qt::AlignCenter);

    // HEX 입력
    auto *hexRow = new QHBoxLayout;
    hexRow->addWidget(new QLabel("HEX"));
    m_hexEdit = new QLineEdit("#00B4FF");
    m_hexEdit->setMaxLength(7);
    m_hexEdit->setFixedWidth(90);
    m_hexEdit->setStyleSheet("font-family:monospace;font-size:12px;letter-spacing:1px;");
    connect(m_hexEdit, &QLineEdit::editingFinished, this, &EffectsView::onHexEdited);
    hexRow->addWidget(m_hexEdit);
    hexRow->addStretch();
    colorLayout->addLayout(hexRow);

    // RGB 슬라이더 — RED
    {
        auto *row = new QHBoxLayout;
        auto *lbl = new QLabel("RED"); lbl->setFixedWidth(36);
        m_rSlider = new QSlider(Qt::Horizontal); m_rSlider->setRange(0,255); m_rSlider->setValue(0);
        m_rLabel  = new QLabel("0"); m_rLabel->setFixedWidth(28);
        row->addWidget(lbl); row->addWidget(m_rSlider,1); row->addWidget(m_rLabel);
        colorLayout->addLayout(row);
        connect(m_rSlider, &QSlider::valueChanged, this, &EffectsView::onSliderChanged);
    }
    // GREEN
    {
        auto *row = new QHBoxLayout;
        auto *lbl = new QLabel("GREEN"); lbl->setFixedWidth(36);
        m_gSlider = new QSlider(Qt::Horizontal); m_gSlider->setRange(0,255); m_gSlider->setValue(180);
        m_gLabel  = new QLabel("180"); m_gLabel->setFixedWidth(28);
        row->addWidget(lbl); row->addWidget(m_gSlider,1); row->addWidget(m_gLabel);
        colorLayout->addLayout(row);
        connect(m_gSlider, &QSlider::valueChanged, this, &EffectsView::onSliderChanged);
    }
    // BLUE
    {
        auto *row = new QHBoxLayout;
        auto *lbl = new QLabel("BLUE"); lbl->setFixedWidth(36);
        m_bSlider = new QSlider(Qt::Horizontal); m_bSlider->setRange(0,255); m_bSlider->setValue(255);
        m_bLabel  = new QLabel("255"); m_bLabel->setFixedWidth(28);
        row->addWidget(lbl); row->addWidget(m_bSlider,1); row->addWidget(m_bLabel);
        colorLayout->addLayout(row);
        connect(m_bSlider, &QSlider::valueChanged, this, &EffectsView::onSliderChanged);
    }

    // 빠른 색상 스와치 10개
    auto *swatchRow = new QHBoxLayout;
    const QList<QColor> swatches = {
        QColor(255,0,85), QColor(255,102,0), QColor(255,170,0),
        QColor(0,230,118), QColor(0,180,255), QColor(0,68,255),
        QColor(127,0,255), QColor(255,0,204), QColor(255,255,255), QColor(68,85,102)
    };
    for (const QColor &c : swatches) {
        auto *sw = new QPushButton;
        sw->setFixedSize(24, 24);
        sw->setStyleSheet(QString("background:%1;border-radius:12px;"
                                   "border:2px solid rgba(255,255,255,0.18);").arg(c.name()));
        connect(sw, &QPushButton::clicked, this, [this, c]() {
            syncColorUi(c);
            onColorChanged(c);
        });
        swatchRow->addWidget(sw);
        m_swatches << sw;
    }
    colorLayout->addLayout(swatchRow);
    rightLayout->addWidget(colorBox, 1);

    // 밝기
    auto *brightBox = new QGroupBox("밝기");
    auto *brightLayout = new QHBoxLayout(brightBox);
    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setRange(0, 100);
    m_brightnessSlider->setValue(100);
    m_brightnessLabel = new QLabel("100%");
    m_brightnessLabel->setFixedWidth(36);
    brightLayout->addWidget(m_brightnessSlider);
    brightLayout->addWidget(m_brightnessLabel);
    rightLayout->addWidget(brightBox);

    mainLayout->addWidget(leftBox, 1);
    mainLayout->addWidget(rightWidget, 1);

    connect(m_colorWheel, &ColorWheelWidget::colorChanged, this, [this](const QColor &c) {
        syncColorUi(c);
        onColorChanged(c);
    });
    connect(m_brightnessSlider, &QSlider::valueChanged, this, &EffectsView::onBrightnessChanged);

    updateButtonStates("static");
    syncColorUi(m_currentColor);
}

void EffectsView::syncColorUi(const QColor &c)
{
    m_syncing = true;
    m_colorWheel->setColor(c);
    m_hexEdit->setText(c.name().toUpper());
    m_rSlider->setValue(c.red());
    m_gSlider->setValue(c.green());
    m_bSlider->setValue(c.blue());
    m_rLabel->setText(QString::number(c.red()));
    m_gLabel->setText(QString::number(c.green()));
    m_bLabel->setText(QString::number(c.blue()));
    m_syncing = false;
}

void EffectsView::onHexEdited()
{
    if (m_syncing) return;
    QColor c(m_hexEdit->text().trimmed());
    if (!c.isValid()) return;
    m_currentColor = c;
    syncColorUi(c);
    m_dm->applyEffect(m_activeEffect, c);
}

void EffectsView::onSliderChanged()
{
    if (m_syncing) return;
    QColor c(m_rSlider->value(), m_gSlider->value(), m_bSlider->value());
    m_rLabel->setText(QString::number(c.red()));
    m_gLabel->setText(QString::number(c.green()));
    m_bLabel->setText(QString::number(c.blue()));
    m_hexEdit->setText(c.name().toUpper());
    m_currentColor = c;
    m_syncing = true;
    m_colorWheel->setColor(c);
    m_syncing = false;
    m_dm->applyEffect(m_activeEffect, c);
}

QPushButton* EffectsView::makeEffectBtn(const QString &icon, const QString &name, const QString &effectId)
{
    auto *btn = new QPushButton(icon + "  " + name);
    btn->setCheckable(true);
    btn->setMinimumHeight(52);
    m_effectBtns.append(btn);
    m_effectIds.append(effectId);

    connect(btn, &QPushButton::clicked, this, [this, effectId]() {
        onEffectSelected(effectId);
    });
    return btn;
}

void EffectsView::updateButtonStates(const QString &activeId)
{
    for (int i = 0; i < m_effectBtns.size(); ++i)
        m_effectBtns[i]->setChecked(m_effectIds[i] == activeId);
}

void EffectsView::onEffectSelected(const QString &effect)
{
    m_activeEffect = effect;
    updateButtonStates(effect);
    m_dm->applyEffect(effect, m_currentColor);
}

void EffectsView::onColorChanged(const QColor &color)
{
    m_currentColor = color;
    m_dm->applyEffect(m_activeEffect, color);
}

void EffectsView::onBrightnessChanged(int value)
{
    m_brightnessLabel->setText(QString("%1%").arg(value));
    QColor adjusted = m_currentColor;
    adjusted.setHsvF(adjusted.hueF(), adjusted.saturationF(), value / 100.0);
    onColorChanged(adjusted);
}

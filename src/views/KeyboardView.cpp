#include "KeyboardView.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QtMath>

static const int KEY_W = 42;
static const int KEY_H = 42;
static const int KEY_GAP = 4;

// 60키 레이아웃 정의
struct KeyDef { QString id, label; double w; };
static const QList<QList<KeyDef>> KEY_ROWS = {
    {{"esc","Esc",1},{"f1","F1",1},{"f2","F2",1},{"f3","F3",1},{"f4","F4",1},
     {"f5","F5",1},{"f6","F6",1},{"f7","F7",1},{"f8","F8",1},{"f9","F9",1},
     {"f10","F10",1},{"f11","F11",1},{"f12","F12",1}},
    {{"tilde","`",1},{"k1","1",1},{"k2","2",1},{"k3","3",1},{"k4","4",1},
     {"k5","5",1},{"k6","6",1},{"k7","7",1},{"k8","8",1},{"k9","9",1},
     {"k0","0",1},{"minus","-",1},{"equal","=",1},{"bksp","⌫",2}},
    {{"tab","Tab",1.5},{"Q","Q",1},{"W","W",1},{"E","E",1},{"R","R",1},
     {"T","T",1},{"Y","Y",1},{"U","U",1},{"I","I",1},{"O","O",1},
     {"P","P",1},{"lbr","[",1},{"rbr","]",1},{"bsl","\\",1.5}},
    {{"caps","Caps",1.75},{"A","A",1},{"S","S",1},{"D","D",1},{"F","F",1},
     {"G","G",1},{"H","H",1},{"J","J",1},{"K","K",1},{"L","L",1},
     {"semi",";",1},{"apos","'",1},{"enter","Enter",2.25}},
    {{"lshift","Shift",2.25},{"Z","Z",1},{"X","X",1},{"C","C",1},{"V","V",1},
     {"B","B",1},{"N","N",1},{"M","M",1},{"comma",",",1},{"dot",".",1},
     {"slash","/",1},{"rshift","Shift",2.75}},
    {{"lctrl","Ctrl",1.25},{"lwin","Win",1.25},{"lalt","Alt",1.25},
     {"space","",6.25},
     {"ralt","Alt",1.25},{"rwin","Win",1.25},{"rctrl","Ctrl",1.25}},
};

// ── KeyButton ────────────────────────────────────────────────
KeyButton::KeyButton(const QString &label, const QString &keyId, double widthUnits, QWidget *parent)
    : QWidget(parent), m_label(label), m_keyId(keyId)
{
    int w = qRound(widthUnits * (KEY_W + KEY_GAP)) - KEY_GAP;
    setFixedSize(w, KEY_H);
    setCursor(Qt::PointingHandCursor);
}

void KeyButton::setKeyColor(const QColor &c)
{
    m_color = c;
    update();
}

void KeyButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor bg = m_hovered ? m_color.lighter(130) : m_color;
    p.setBrush(bg);
    p.setPen(QPen(m_selected ? QColor(79,195,247) : QColor(255,255,255,40),
                  m_selected ? 2 : 1));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 5, 5);

    // 텍스트 색상 (배경에 따라 자동)
    double luma = 0.299*m_color.redF() + 0.587*m_color.greenF() + 0.114*m_color.blueF();
    p.setPen(luma > 0.35 ? Qt::black : Qt::white);
    p.setFont(QFont("Segoe UI", m_label.size() > 2 ? 7 : 9));
    p.drawText(rect(), Qt::AlignCenter, m_label);
}

void KeyButton::mousePressEvent(QMouseEvent *) { emit clicked(m_keyId); }
void KeyButton::enterEvent(QEnterEvent *)      { m_hovered = true;  update(); }
void KeyButton::leaveEvent(QEvent *)           { m_hovered = false; update(); }

// ── KeyboardView ─────────────────────────────────────────────
KeyboardView::KeyboardView(DeviceManager *dm, QWidget *parent)
    : QWidget(parent), m_dm(dm)
{
    setupUi();
}

void KeyboardView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 상단 툴바
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    // 기기 선택
    auto *deviceLabel = new QLabel("기기:");
    auto *deviceCombo = new QComboBox;
    deviceCombo->addItem("키보드 선택...");
    deviceCombo->setMinimumWidth(200);

    // 색상 선택 버튼
    auto *colorBtn = new QPushButton;
    colorBtn->setFixedSize(36, 36);
    colorBtn->setStyleSheet(QString("background:%1;border-radius:18px;border:2px solid rgba(255,255,255,0.3);").arg(m_selectedColor.name()));

    connect(colorBtn, &QPushButton::clicked, this, [this, colorBtn]() {
        QColor c = QColorDialog::getColor(m_selectedColor, this, "색상 선택");
        if (c.isValid()) {
            m_selectedColor = c;
            colorBtn->setStyleSheet(QString("background:%1;border-radius:18px;border:2px solid rgba(255,255,255,0.3);").arg(c.name()));
        }
    });

    // 프리셋 버튼
    auto *rainbowBtn = new QPushButton("🌈 레인보우");
    auto *fireBtn    = new QPushButton("🔥 파이어");
    auto *oceanBtn   = new QPushButton("🌊 오션");
    auto *fillBtn    = new QPushButton("전체 채우기");
    auto *applyBtn   = new QPushButton("✅ 기기 적용");
    applyBtn->setStyleSheet("background: #0288D1; font-weight: bold;");

    connect(rainbowBtn, &QPushButton::clicked, this, &KeyboardView::onPresetRainbow);
    connect(fireBtn,    &QPushButton::clicked, this, &KeyboardView::onPresetFire);
    connect(oceanBtn,   &QPushButton::clicked, this, &KeyboardView::onPresetOcean);
    connect(fillBtn,    &QPushButton::clicked, this, &KeyboardView::onFillAll);
    connect(applyBtn,   &QPushButton::clicked, this, &KeyboardView::applyToDevice);

    toolbar->addWidget(deviceLabel);
    toolbar->addWidget(deviceCombo);
    toolbar->addWidget(new QLabel("색상:"));
    toolbar->addWidget(colorBtn);
    toolbar->addSpacing(8);
    toolbar->addWidget(rainbowBtn);
    toolbar->addWidget(fireBtn);
    toolbar->addWidget(oceanBtn);
    toolbar->addSpacing(8);
    toolbar->addWidget(fillBtn);
    toolbar->addStretch();
    toolbar->addWidget(applyBtn);

    mainLayout->addLayout(toolbar);

    // 키보드 그리드
    auto *kbGroup = new QGroupBox("키별 색상");
    auto *kbLayout = new QVBoxLayout(kbGroup);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: none; }");

    m_keyboardWidget = new QWidget;
    m_keyboardWidget->setStyleSheet("background: #0D0D1A; border-radius: 10px;");
    buildKeyboard();

    scroll->setWidget(m_keyboardWidget);
    kbLayout->addWidget(scroll);
    mainLayout->addWidget(kbGroup, 1);
}

void KeyboardView::buildKeyboard()
{
    int totalHeight = 0;
    int maxWidth = 0;

    for (const auto &row : KEY_ROWS) {
        int rowWidth = 0;
        for (const auto &k : row)
            rowWidth += qRound(k.w * (KEY_W + KEY_GAP));
        maxWidth = qMax(maxWidth, rowWidth);
        totalHeight += KEY_H + KEY_GAP;
    }

    m_keyboardWidget->setMinimumSize(maxWidth + 24, totalHeight + 24);

    int y = 12;
    for (const auto &row : KEY_ROWS) {
        int x = 12;
        for (const auto &k : row) {
            int w = qRound(k.w * (KEY_W + KEY_GAP)) - KEY_GAP;
            auto *btn = new KeyButton(k.label, k.id, k.w, m_keyboardWidget);
            btn->move(x, y);
            m_keys[k.id] = btn;

            connect(btn, &KeyButton::clicked, this, &KeyboardView::onKeyClicked);
            x += w + KEY_GAP;
        }
        y += KEY_H + KEY_GAP;
    }
}

void KeyboardView::onKeyClicked(const QString &keyId)
{
    m_keyColors[keyId] = m_selectedColor;
    m_keys[keyId]->setKeyColor(m_selectedColor);
}

void KeyboardView::onPresetRainbow()
{
    QList<QString> keyList = m_keys.keys();
    for (int i = 0; i < keyList.size(); ++i) {
        double t = (double)i / qMax(1, keyList.size() - 1);
        QColor c = QColor::fromHsvF(t * 300.0 / 360.0, 1.0, 1.0);
        m_keyColors[keyList[i]] = c;
        m_keys[keyList[i]]->setKeyColor(c);
    }
}

void KeyboardView::onPresetFire()
{
    QList<QString> keyList = m_keys.keys();
    for (int i = 0; i < keyList.size(); ++i) {
        double t = (double)i / qMax(1, keyList.size() - 1);
        QColor c;
        if (t < 0.33)      c = QColor(255, 255, 0);
        else if (t < 0.66) c = QColor(255, 130, 0);
        else               c = QColor(255, 30, 0);
        m_keyColors[keyList[i]] = c;
        m_keys[keyList[i]]->setKeyColor(c);
    }
}

void KeyboardView::onPresetOcean()
{
    QList<QString> keyList = m_keys.keys();
    for (int i = 0; i < keyList.size(); ++i) {
        double t = (double)i / qMax(1, keyList.size() - 1);
        QColor c;
        if (t < 0.33)      c = QColor(0, 50, 255);
        else if (t < 0.66) c = QColor(0, 170, 255);
        else               c = QColor(0, 255, 230);
        m_keyColors[keyList[i]] = c;
        m_keys[keyList[i]]->setKeyColor(c);
    }
}

void KeyboardView::onFillAll()
{
    for (auto *btn : m_keys) {
        m_keyColors[btn->keyId()] = m_selectedColor;
        btn->setKeyColor(m_selectedColor);
    }
}

void KeyboardView::applyToDevice()
{
    if (m_selectedDeviceId.isEmpty()) return;
    QList<QColor> colors;
    for (auto *btn : m_keys)
        colors << btn->keyColor();
    m_dm->setDeviceLedColors(m_selectedDeviceId, colors);
}

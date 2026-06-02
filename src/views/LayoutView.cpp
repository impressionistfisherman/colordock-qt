#include "LayoutView.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QtMath>

static const int BASE_W = 110;
static const int BASE_H = 60;

// ── LayoutCanvas ──────────────────────────────────────────────
LayoutCanvas::LayoutCanvas(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(600, 400);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("background: #0A0D1A; border-radius: 10px;");
}

void LayoutCanvas::setDevices(const QList<LayoutDevice> &devices)
{
    m_devices = devices;
    m_selected = -1;
    update();
}

QRectF LayoutCanvas::deviceRect(const LayoutDevice &d) const
{
    double w = BASE_W * d.scale;
    double h = BASE_H * d.scale;
    return QRectF(d.pos.x() - w/2, d.pos.y() - h/2, w, h);
}

int LayoutCanvas::hitTest(const QPointF &p) const
{
    for (int i = m_devices.size()-1; i >= 0; --i) {
        if (deviceRect(m_devices[i]).contains(p)) return i;
    }
    return -1;
}

void LayoutCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 배경 그리드
    p.setPen(QPen(QColor(255,255,255,12), 1));
    for (int x = 0; x < width(); x += 40)
        p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += 40)
        p.drawLine(0, y, width(), y);

    // 힌트
    if (m_devices.isEmpty()) {
        p.setPen(QColor(80, 100, 120));
        p.setFont(QFont("Segoe UI", 11));
        p.drawText(rect(), Qt::AlignCenter, "기기를 클릭해 크기·회전·반전을 조정하세요.\n장치를 드래그해 실제 배치처럼 놓으세요.");
        return;
    }

    for (int i = 0; i < m_devices.size(); ++i) {
        const LayoutDevice &d = m_devices[i];
        QRectF r = deviceRect(d);

        p.save();
        p.translate(r.center());
        p.rotate(d.rotation);
        if (d.flipped) p.scale(-1, 1);
        p.translate(-r.center());

        // 기기 박스
        QColor bg = d.excluded ? QColor(40,40,50) : d.color.darker(200);
        QColor border = (i == m_selected) ? QColor(79,195,247) :
                        d.excluded         ? QColor(80,80,90)  :
                                             d.color.lighter(130);
        p.setBrush(bg);
        p.setPen(QPen(border, i == m_selected ? 2.5 : 1.5));
        p.drawRoundedRect(r, 8, 8);

        // 글로우 (선택 시)
        if (i == m_selected) {
            p.setPen(Qt::NoPen);
            QColor glow = QColor(79,195,247,40);
            p.setBrush(glow);
            p.drawRoundedRect(r.adjusted(-3,-3,3,3), 10, 10);
        }

        // 텍스트
        p.setPen(d.excluded ? QColor(80,80,90) : Qt::white);
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        p.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, d.name);

        // LED 바 (하단)
        if (!d.excluded) {
            QRectF ledBar(r.left()+6, r.bottom()-10, r.width()-12, 5);
            QLinearGradient grad(ledBar.left(), 0, ledBar.right(), 0);
            grad.setColorAt(0, d.color);
            grad.setColorAt(0.5, d.color.lighter(150));
            grad.setColorAt(1, d.color);
            p.setBrush(grad);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(ledBar, 2, 2);
        }

        p.restore();
    }
}

void LayoutCanvas::mousePressEvent(QMouseEvent *e)
{
    int idx = hitTest(e->position());
    if (idx != m_selected) {
        m_selected = idx;
        emit selectionChanged(idx);
        update();
    }
    if (idx >= 0) {
        m_dragging   = true;
        m_dragOffset = e->position() - m_devices[idx].pos;
    }
}

void LayoutCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && m_selected >= 0) {
        m_devices[m_selected].pos = e->position() - m_dragOffset;
        emit deviceMoved(m_selected, m_devices[m_selected].pos);
        update();
    }
}

void LayoutCanvas::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}

void LayoutCanvas::autoArrange()
{
    int cols = qMax(1, (int)sqrt((double)m_devices.size()));
    int cellW = qMax(BASE_W + 30, (width() - 40) / cols);
    int cellH = BASE_H + 50;

    for (int i = 0; i < m_devices.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        m_devices[i].pos = QPointF(30 + col * cellW + cellW/2,
                                    40 + row * cellH + cellH/2);
    }
    update();
}

void LayoutCanvas::setSelectedScale(double s)
{
    if (m_selected < 0) return;
    m_devices[m_selected].scale = s;
    update();
}

void LayoutCanvas::setSelectedRotation(int deg)
{
    if (m_selected < 0) return;
    m_devices[m_selected].rotation = deg;
    update();
}

void LayoutCanvas::flipSelected()
{
    if (m_selected < 0) return;
    m_devices[m_selected].flipped = !m_devices[m_selected].flipped;
    update();
}

void LayoutCanvas::resetSelected()
{
    if (m_selected < 0) return;
    m_devices[m_selected].scale    = 1.0;
    m_devices[m_selected].rotation = 0;
    m_devices[m_selected].flipped  = false;
    update();
}

void LayoutCanvas::toggleExcludeSelected()
{
    if (m_selected < 0) return;
    m_devices[m_selected].excluded = !m_devices[m_selected].excluded;
    update();
}

// ── LayoutView ───────────────────────────────────────────────
LayoutView::LayoutView(DeviceManager *dm, QWidget *parent)
    : QWidget(parent), m_dm(dm)
{
    setupUi();
    connect(dm, &DeviceManager::devicesChanged, this, &LayoutView::onDevicesChanged);
    onDevicesChanged();
}

void LayoutView::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── 좌측: 캔버스 ─────────────────────────────────────────
    auto *canvasWrap = new QWidget;
    canvasWrap->setStyleSheet("background:#0A0D1A;");
    auto *canvasLayout = new QVBoxLayout(canvasWrap);
    canvasLayout->setContentsMargins(12, 12, 12, 12);

    // 상단 툴바
    auto *toolbar = new QHBoxLayout;
    auto *title = new QLabel("🖥️  레이아웃 캔버스");
    title->setStyleSheet("font-size:14px;font-weight:bold;color:#4FC3F7;");
    auto *autoBtn = new QPushButton("실제 배치 정렬");
    autoBtn->setStyleSheet("background:rgba(79,195,247,0.12);border:1px solid rgba(79,195,247,0.3);border-radius:6px;color:#4FC3F7;padding:4px 12px;");
    connect(autoBtn, &QPushButton::clicked, this, &LayoutView::onAutoArrange);
    toolbar->addWidget(title);
    toolbar->addStretch();
    toolbar->addWidget(autoBtn);
    canvasLayout->addLayout(toolbar);

    m_canvas = new LayoutCanvas;
    connect(m_canvas, &LayoutCanvas::selectionChanged, this, &LayoutView::onSelectionChanged);
    canvasLayout->addWidget(m_canvas, 1);

    // 효과 속도/간격 슬라이더
    auto *effectRow = new QHBoxLayout;
    effectRow->addWidget(new QLabel("효과 속도"));
    m_speedSl = new QSlider(Qt::Horizontal);
    m_speedSl->setRange(1, 10); m_speedSl->setValue(4);
    effectRow->addWidget(m_speedSl, 1);
    effectRow->addSpacing(16);
    effectRow->addWidget(new QLabel("효과 간격"));
    m_spreadSl = new QSlider(Qt::Horizontal);
    m_spreadSl->setRange(1, 10); m_spreadSl->setValue(5);
    effectRow->addWidget(m_spreadSl, 1);
    canvasLayout->addLayout(effectRow);

    mainLayout->addWidget(canvasWrap, 1);

    // ── 우측: 컨트롤 패널 ────────────────────────────────────
    auto *ctrlPanel = new QWidget;
    ctrlPanel->setFixedWidth(220);
    ctrlPanel->setStyleSheet("background:#111827;border-left:1px solid rgba(255,255,255,0.06);");
    auto *ctrlLayout = new QVBoxLayout(ctrlPanel);
    ctrlLayout->setContentsMargins(12, 16, 12, 16);
    ctrlLayout->setSpacing(12);

    // 선택 장치
    auto *selBox = new QGroupBox("선택 장치");
    auto *selLayout = new QVBoxLayout(selBox);
    m_selectedLabel = new QLabel("장치를 선택하세요");
    m_selectedLabel->setStyleSheet("color:#90A4AE;font-size:11px;");
    m_selectedLabel->setWordWrap(true);
    selLayout->addWidget(m_selectedLabel);
    ctrlLayout->addWidget(selBox);

    // 위치
    auto *posBox = new QGroupBox("위치");
    auto *posLayout = new QHBoxLayout(posBox);
    posLayout->addWidget(new QLabel("X"));
    m_xSpin = new QSpinBox; m_xSpin->setRange(0, 2000); m_xSpin->setFixedWidth(70);
    posLayout->addWidget(m_xSpin);
    posLayout->addWidget(new QLabel("Y"));
    m_ySpin = new QSpinBox; m_ySpin->setRange(0, 2000); m_ySpin->setFixedWidth(70);
    posLayout->addWidget(m_ySpin);
    ctrlLayout->addWidget(posBox);

    connect(m_xSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        if (m_currentIdx < 0) return;
        QPointF p = m_canvas->devices()[m_currentIdx].pos;
        p.setX(v);
        m_canvas->devices(); // const ref — need to call canvas setters
    });

    // 크기
    auto *scaleBox = new QGroupBox("크기");
    auto *scaleLayout = new QHBoxLayout(scaleBox);
    m_scaleSl = new QSlider(Qt::Horizontal);
    m_scaleSl->setRange(50, 200); m_scaleSl->setValue(100);
    m_scaleLbl = new QLabel("100%");
    m_scaleLbl->setFixedWidth(38);
    scaleLayout->addWidget(m_scaleSl, 1);
    scaleLayout->addWidget(m_scaleLbl);
    connect(m_scaleSl, &QSlider::valueChanged, this, [this](int v) {
        m_scaleLbl->setText(QString("%1%").arg(v));
        m_canvas->setSelectedScale(v / 100.0);
    });
    ctrlLayout->addWidget(scaleBox);

    // 회전
    auto *rotBox = new QGroupBox("회전");
    auto *rotLayout = new QHBoxLayout(rotBox);
    m_rotSl = new QSlider(Qt::Horizontal);
    m_rotSl->setRange(0, 270); m_rotSl->setSingleStep(90); m_rotSl->setValue(0);
    m_rotLbl = new QLabel("0°");
    m_rotLbl->setFixedWidth(30);
    rotLayout->addWidget(m_rotSl, 1);
    rotLayout->addWidget(m_rotLbl);
    connect(m_rotSl, &QSlider::valueChanged, this, [this](int v) {
        int snapped = (v / 90) * 90;
        m_rotSl->setValue(snapped);
        m_rotLbl->setText(QString("%1°").arg(snapped));
        m_canvas->setSelectedRotation(snapped);
    });
    ctrlLayout->addWidget(rotBox);

    // 액션 버튼
    auto *actBox = new QGroupBox("장치 액션");
    auto *actLayout = new QVBoxLayout(actBox);
    auto *flipBtn    = new QPushButton("↔ 좌우 반전");
    auto *excludeBtn = new QPushButton("Effect 제외");
    auto *resetBtn   = new QPushButton("초기화");
    for (auto *b : {flipBtn, excludeBtn, resetBtn}) {
        b->setStyleSheet("background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:6px;padding:5px;");
        actLayout->addWidget(b);
    }
    connect(flipBtn,    &QPushButton::clicked, this, &LayoutView::onFlip);
    connect(excludeBtn, &QPushButton::clicked, this, &LayoutView::onToggleExclude);
    connect(resetBtn,   &QPushButton::clicked, this, &LayoutView::onReset);
    ctrlLayout->addWidget(actBox);

    ctrlLayout->addStretch();
    mainLayout->addWidget(ctrlPanel);
}

void LayoutView::onDevicesChanged()
{
    QList<LayoutDevice> ldevs;
    const auto &devs = m_dm->devices();
    for (const auto &d : devs) {
        LayoutDevice ld;
        ld.id    = d.id;
        ld.name  = d.name;
        ld.color = d.leds.isEmpty() ? QColor(0,180,255) : d.leds.first();
        ld.pos   = QPointF(100 + ldevs.size() * 140, 200);
        ldevs << ld;
    }
    m_canvas->setDevices(ldevs);
    if (!ldevs.isEmpty()) m_canvas->autoArrange();
}

void LayoutView::onSelectionChanged(int idx)
{
    m_currentIdx = idx;
    if (idx < 0) {
        m_selectedLabel->setText("장치를 선택하세요");
        m_scaleSl->setValue(100);
        m_rotSl->setValue(0);
    } else {
        const auto &d = m_canvas->devices()[idx];
        m_selectedLabel->setText(d.name);
        m_scaleSl->setValue(int(d.scale * 100));
        m_rotSl->setValue(d.rotation);
        m_xSpin->setValue(int(d.pos.x()));
        m_ySpin->setValue(int(d.pos.y()));
    }
}

void LayoutView::onAutoArrange() { m_canvas->autoArrange(); }
void LayoutView::onFlip()        { m_canvas->flipSelected(); }
void LayoutView::onReset()       { m_canvas->resetSelected(); m_scaleSl->setValue(100); m_rotSl->setValue(0); }
void LayoutView::onToggleExclude(){ m_canvas->toggleExcludeSelected(); }
void LayoutView::saveLayout()    {}
void LayoutView::loadLayout()    {}

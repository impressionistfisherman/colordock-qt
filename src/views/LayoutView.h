#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QColor>

class DeviceManager;
class QLabel;
class QSlider;
class QComboBox;
class QSpinBox;

struct LayoutDevice {
    QString id;
    QString name;
    QColor  color;
    QPointF pos;
    double  scale    = 1.0;
    int     rotation = 0;
    bool    flipped  = false;
    bool    excluded = false;
};

// ── 드래그 가능한 캔버스 ──────────────────────────────────────
class LayoutCanvas : public QWidget {
    Q_OBJECT
public:
    explicit LayoutCanvas(QWidget *parent = nullptr);

    void setDevices(const QList<LayoutDevice> &devices);
    const QList<LayoutDevice>& devices() const { return m_devices; }
    void autoArrange();
    void setSelectedScale(double s);
    void setSelectedRotation(int deg);
    void flipSelected();
    void resetSelected();
    void toggleExcludeSelected();

signals:
    void selectionChanged(int idx);          // -1 = none
    void deviceMoved(int idx, QPointF pos);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    int hitTest(const QPointF &p) const;
    QRectF deviceRect(const LayoutDevice &d) const;

    QList<LayoutDevice> m_devices;
    int    m_selected  = -1;
    bool   m_dragging  = false;
    QPointF m_dragOffset;
};

// ── LayoutView ───────────────────────────────────────────────
class LayoutView : public QWidget {
    Q_OBJECT
public:
    explicit LayoutView(DeviceManager *dm, QWidget *parent = nullptr);

public slots:
    void onDevicesChanged();

private slots:
    void onSelectionChanged(int idx);
    void onAutoArrange();
    void onFlip();
    void onReset();
    void onToggleExclude();

private:
    void setupUi();
    void saveLayout();
    void loadLayout();

    DeviceManager *m_dm;
    LayoutCanvas  *m_canvas;

    QLabel    *m_selectedLabel;
    QSpinBox  *m_xSpin;
    QSpinBox  *m_ySpin;
    QSlider   *m_scaleSl;
    QSlider   *m_rotSl;
    QLabel    *m_scaleLbl;
    QLabel    *m_rotLbl;
    QSlider   *m_speedSl;
    QSlider   *m_spreadSl;

    int m_currentIdx = -1;
};

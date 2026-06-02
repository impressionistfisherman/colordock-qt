#pragma once
#include <QWidget>
#include <QColor>
#include <QList>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>

class DeviceManager;
class ColorWheelWidget;

class EffectsView : public QWidget
{
    Q_OBJECT
public:
    explicit EffectsView(DeviceManager *dm, QWidget *parent = nullptr);

    QColor currentColor() const { return m_currentColor; }
    QString activeEffect() const { return m_activeEffect; }

private slots:
    void onEffectSelected(const QString &effect);
    void onColorChanged(const QColor &color);
    void onBrightnessChanged(int value);
    void onHexEdited();
    void onSliderChanged();

private:
    void setupUi();
    QPushButton* makeEffectBtn(const QString &icon, const QString &name, const QString &effectId);
    void updateButtonStates(const QString &activeId);
    void syncColorUi(const QColor &c);

    DeviceManager    *m_dm;
    QList<QPushButton*> m_effectBtns;
    QList<QString>    m_effectIds;
    ColorWheelWidget *m_colorWheel;

    QLabel    *m_brightnessLabel;
    QSlider   *m_brightnessSlider;
    QLineEdit *m_hexEdit;
    QSlider   *m_rSlider, *m_gSlider, *m_bSlider;
    QLabel    *m_rLabel,  *m_gLabel,  *m_bLabel;
    QList<QPushButton*> m_swatches;

    QColor  m_currentColor = QColor(0, 180, 255);
    QString m_activeEffect = "static";
    bool    m_syncing      = false;
};

// 색상환 위젯
class ColorWheelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ColorWheelWidget(QWidget *parent = nullptr);
    QColor color() const { return m_color; }
    void setColor(const QColor &c);

signals:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    QColor  m_color = QColor(0, 180, 255);
    QPointF colorToPos(const QColor &c) const;
    QColor  posToColor(const QPointF &p) const;
};

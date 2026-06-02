#pragma once
#include <QWidget>
#include <QMap>
#include <QColor>
#include <QScrollArea>

class DeviceManager;

// 단일 키 버튼
class KeyButton : public QWidget
{
    Q_OBJECT
public:
    KeyButton(const QString &label, const QString &keyId, double widthUnits, QWidget *parent = nullptr);
    void setKeyColor(const QColor &c);
    QColor keyColor() const { return m_color; }
    const QString& keyId() const { return m_keyId; }

signals:
    void clicked(const QString &keyId);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    QString m_label;
    QString m_keyId;
    QColor  m_color   = QColor(20, 20, 40);
    bool    m_hovered = false;
    bool    m_selected = false;
};

class KeyboardView : public QWidget
{
    Q_OBJECT
public:
    explicit KeyboardView(DeviceManager *dm, QWidget *parent = nullptr);

private slots:
    void onKeyClicked(const QString &keyId);
    void onPresetRainbow();
    void onPresetFire();
    void onPresetOcean();
    void onFillAll();
    void applyToDevice();

private:
    void setupUi();
    void buildKeyboard();

    DeviceManager *m_dm;
    QWidget       *m_keyboardWidget;
    QMap<QString, KeyButton*> m_keys;
    QMap<QString, QColor>     m_keyColors;
    QColor         m_selectedColor = QColor(0, 180, 255);
    QString        m_selectedDeviceId;
};

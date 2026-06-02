#pragma once
#include <QObject>
#include <QList>
#include <QTimer>
#include <QColor>

struct RGBDevice {
    QString id;
    QString name;
    QString manufacturer;
    QString type;          // "Motherboard","GPU","RAM","Keyboard","Mouse","Smart Light"
    int     ledCount = 1;
    QList<QColor> leds;
    bool    openrgbSource = false;
    int     openrgbIndex  = -1;
};

class OpenRGBClient;

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    void startDiscovery();
    void stopDiscovery();

    const QList<RGBDevice>& devices() const { return m_devices; }

    // 색상 적용
    void setDeviceColor(const QString &deviceId, const QColor &color);
    void setDeviceLedColors(const QString &deviceId, const QList<QColor> &colors);
    void setAllColor(const QColor &color);
    void applyEffect(const QString &effectName, const QColor &baseColor);
    void applyEffectToAll(const QString &effectName, const QColor &baseColor);

    // OpenRGB 재연결
    void reconnectOpenRGB();

    // 커스텀 기기 추가 (HID 스캐너에서 호출)
    void addCustomDevice(const RGBDevice &dev);

signals:
    void devicesChanged();
    void deviceColorChanged(const QString &deviceId, const QColor &color);
    void openRGBConnected(int deviceCount);
    void openRGBDisconnected();

private slots:
    void onDiscoveryTimer();
    void onOpenRGBDevicesLoaded(const QList<RGBDevice> &devices);

private:
    void discoverOpenRGB();
    void discoverNanoleaf();
    void mergeDevices(const QList<RGBDevice> &newDevices, const QString &source);

    QList<RGBDevice> m_devices;
    OpenRGBClient   *m_openrgb;
    QTimer          *m_discoveryTimer;
    QTimer          *m_effectTimer;
    QString          m_activeEffect;
    QColor           m_baseColor     = QColor(0, 180, 255);
    double           m_effectPhase   = 0.0;
};

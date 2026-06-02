#include "DeviceManager.h"
#include "OpenRGBClient.h"
#include <QDebug>
#include <QtMath>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    m_openrgb = new OpenRGBClient(this);

    connect(m_openrgb, &OpenRGBClient::devicesLoaded,
            this, &DeviceManager::onOpenRGBDevicesLoaded);
    connect(m_openrgb, &OpenRGBClient::connected, this, [this]() {
        emit openRGBConnected(m_devices.size());
    });
    connect(m_openrgb, &OpenRGBClient::disconnected, this, [this]() {
        // OpenRGB 기기 제거
        m_devices.removeIf([](const RGBDevice &d){ return d.openrgbSource; });
        emit devicesChanged();
        emit openRGBDisconnected();
    });

    // 효과 타이머
    m_effectTimer = new QTimer(this);
    m_effectTimer->setInterval(50); // 20fps
    connect(m_effectTimer, &QTimer::timeout, this, [this]() {
        if (m_activeEffect.isEmpty()) return;
        m_effectPhase += 0.02;
        if (m_effectPhase > 1.0) m_effectPhase -= 1.0;
        applyEffect(m_activeEffect, m_baseColor);
    });
}

DeviceManager::~DeviceManager()
{
    m_openrgb->disconnect();
}

void DeviceManager::onDiscoveryTimer()
{
    // 주기적 재발견 (현재는 OpenRGB 재시도가 자체적으로 처리함)
    if (!m_openrgb->isConnected())
        m_openrgb->connectToServer();
}

void DeviceManager::startDiscovery()
{
    qDebug() << "[DeviceManager] Starting discovery...";
    m_openrgb->connectToServer();
}

void DeviceManager::stopDiscovery()
{
    m_openrgb->disconnect();
}

void DeviceManager::reconnectOpenRGB()
{
    m_openrgb->disconnect();
    QTimer::singleShot(500, this, [this]() {
        m_openrgb->connectToServer();
    });
}

void DeviceManager::onOpenRGBDevicesLoaded(const QList<RGBDevice> &devices)
{
    // OpenRGB 기기만 교체
    m_devices.removeIf([](const RGBDevice &d){ return d.openrgbSource; });
    m_devices.append(devices);
    qDebug() << "[DeviceManager] Total devices:" << m_devices.size();
    emit devicesChanged();
}

void DeviceManager::setDeviceColor(const QString &deviceId, const QColor &color)
{
    for (RGBDevice &dev : m_devices) {
        if (dev.id == deviceId) {
            dev.leds.fill(color);
            if (dev.openrgbSource)
                m_openrgb->setDeviceColor(dev.openrgbIndex, color);
            emit deviceColorChanged(deviceId, color);
            return;
        }
    }
}

void DeviceManager::setDeviceLedColors(const QString &deviceId, const QList<QColor> &colors)
{
    for (RGBDevice &dev : m_devices) {
        if (dev.id == deviceId) {
            for (int i = 0; i < colors.size() && i < dev.leds.size(); ++i)
                dev.leds[i] = colors[i];
            if (dev.openrgbSource)
                m_openrgb->setDeviceColors(dev.openrgbIndex, colors);
            emit deviceColorChanged(deviceId, colors.isEmpty() ? Qt::black : colors.first());
            return;
        }
    }
}

void DeviceManager::addCustomDevice(const RGBDevice &dev)
{
    // 중복 확인
    for (const RGBDevice &d : m_devices)
        if (d.id == dev.id) return;
    m_devices << dev;
    emit devicesChanged();
}

void DeviceManager::applyEffectToAll(const QString &effectName, const QColor &baseColor)
{
    applyEffect(effectName, baseColor);
}

void DeviceManager::setAllColor(const QColor &color)
{
    m_activeEffect.clear();
    m_effectTimer->stop();
    m_baseColor = color;

    for (RGBDevice &dev : m_devices) {
        dev.leds.fill(color);
        if (dev.openrgbSource)
            m_openrgb->setDeviceColor(dev.openrgbIndex, color);
        emit deviceColorChanged(dev.id, color);
    }
}

void DeviceManager::applyEffect(const QString &effectName, const QColor &baseColor)
{
    if (effectName != m_activeEffect || baseColor != m_baseColor) {
        m_activeEffect = effectName;
        m_baseColor    = baseColor;
        m_effectPhase  = 0.0;
        if (effectName.isEmpty()) {
            m_effectTimer->stop();
            return;
        }
        m_effectTimer->start();
    }

    // 효과 계산 (20fps 타이머에서 호출)
    for (int i = 0; i < m_devices.size(); ++i) {
        RGBDevice &dev = m_devices[i];
        QColor c;

        if (effectName == "static") {
            c = baseColor;
        } else if (effectName == "rainbow") {
            double hue = fmod(m_effectPhase * 360.0 + i * 30.0, 360.0);
            c = QColor::fromHsvF(hue / 360.0, 1.0, 1.0);
        } else if (effectName == "breathing") {
            double v = (sin(m_effectPhase * 2 * M_PI) + 1.0) / 2.0;
            c = QColor(baseColor.red() * v, baseColor.green() * v, baseColor.blue() * v);
        } else if (effectName == "wave") {
            double offset = fmod(m_effectPhase * 360.0 + i * 45.0, 360.0);
            c = QColor::fromHsvF(offset / 360.0, 1.0, 1.0);
        } else {
            c = baseColor;
        }

        dev.leds.fill(c);
        if (dev.openrgbSource)
            m_openrgb->setDeviceColor(dev.openrgbIndex, c);
    }
}

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
    m_effectTimer->stop();
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
    // 효과 또는 색상이 바뀔 때만 페이즈 리셋 (타이머 콜백 재진입 시 리셋 방지)
    bool effectChanged = (effectName != m_activeEffect);
    bool colorChanged  = (baseColor  != m_baseColor);

    if (effectChanged || colorChanged) {
        m_activeEffect = effectName;
        m_baseColor    = baseColor;
        if (effectChanged) m_effectPhase = 0.0;   // 색상만 바뀌면 페이즈 유지
        if (effectName.isEmpty() || effectName == "off" || effectName == "disabled") {
            m_effectTimer->stop();
            // 조명 끄기
            for (RGBDevice &dev : m_devices) {
                dev.leds.fill(Qt::black);
                if (dev.openrgbSource)
                    m_openrgb->setDeviceColor(dev.openrgbIndex, Qt::black);
            }
            return;
        }
        if (effectName == "static") {
            m_effectTimer->stop(); // static은 타이머 불필요
        } else {
            m_effectTimer->start();
        }
    }

    // 효과 계산
    for (int i = 0; i < m_devices.size(); ++i) {
        RGBDevice &dev = m_devices[i];
        QColor c = baseColor;

        if (effectName == "static") {
            c = baseColor;
        } else if (effectName == "rainbow") {
            double hue = fmod(m_effectPhase * 360.0 + i * 30.0, 360.0);
            c = QColor::fromHsvF(hue / 360.0, 1.0, 1.0);
        } else if (effectName == "breathing") {
            double v = (sin(m_effectPhase * 2 * M_PI) + 1.0) / 2.0;
            c = QColor(int(baseColor.red()   * v),
                       int(baseColor.green() * v),
                       int(baseColor.blue()  * v));
        } else if (effectName == "wave") {
            double offset = fmod(m_effectPhase * 360.0 + i * 45.0, 360.0);
            c = QColor::fromHsvF(offset / 360.0, 1.0, 1.0);
        } else if (effectName == "strobe") {
            // 10Hz 스트로브: 페이즈 0~0.5 ON, 0.5~1.0 OFF
            c = (m_effectPhase < 0.5) ? baseColor : Qt::black;
        } else if (effectName == "fire") {
            // 저음 레드~오렌지~옐로우 그라디언트
            double t = fmod(m_effectPhase + i * 0.1, 1.0);
            if (t < 0.33)      c = QColor(255, int(t / 0.33 * 255), 0);
            else if (t < 0.66) c = QColor(255, 255, 0);
            else               c = QColor(255, int((1.0 - (t - 0.66) / 0.34) * 128), 0);
        } else if (effectName == "ocean") {
            double t = fmod(m_effectPhase + i * 0.08, 1.0);
            c = QColor::fromHsvF(0.55 + t * 0.1, 0.9, 0.8 + sin(t * M_PI) * 0.2);
        } else if (effectName == "sparkle") {
            // 랜덤 반짝임 (디바이스 인덱스 기반 의사난수)
            double r = fmod(sin((i + 1) * 127.1 + m_effectPhase * 311.7) * 43758.5, 1.0);
            c = (r > 0.85) ? baseColor : Qt::black;
        } else if (effectName == "sensor" || effectName == "audio" ||
                   effectName == "ambient" || effectName == "game") {
            c = baseColor; // 외부 데이터 연동 — 현재는 baseColor 유지
        }

        dev.leds.fill(c);
        if (dev.openrgbSource)
            m_openrgb->setDeviceColor(dev.openrgbIndex, c);
    }
}

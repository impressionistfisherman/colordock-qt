#include "OpenRGBClient.h"
#include <QDataStream>
#include <QDebug>

static const QByteArray MAGIC = "ORGB";

OpenRGBClient::OpenRGBClient(QObject *parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    m_retryTimer = new QTimer(this);
    m_retryTimer->setInterval(3000);

    connect(m_socket, &QTcpSocket::connected,    this, &OpenRGBClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &OpenRGBClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,    this, &OpenRGBClient::onReadyRead);
    connect(m_retryTimer, &QTimer::timeout,      this, &OpenRGBClient::onRetryTimer);
}

void OpenRGBClient::connectToServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_socket->connectToHost(host, port);
    m_retryTimer->start();
}

void OpenRGBClient::disconnect()
{
    m_retryTimer->stop();
    m_socket->disconnectFromHost();
}

bool OpenRGBClient::isConnected() const { return m_connected; }

void OpenRGBClient::onConnected()
{
    m_connected = true;
    m_retryTimer->stop();
    qDebug() << "[OpenRGB] Connected";
    setClientName();
    requestControllerCount();
    emit connected();
}

void OpenRGBClient::onDisconnected()
{
    m_connected = false;
    qDebug() << "[OpenRGB] Disconnected, retrying in 3s...";
    m_retryTimer->start();
    emit disconnected();
}

void OpenRGBClient::onRetryTimer()
{
    if (!m_connected)
        m_socket->connectToHost(m_host, m_port);
}

void OpenRGBClient::setClientName()
{
    QByteArray name = "ColorDock";
    sendPacket(0, SET_CLIENT_NAME, name);
}

void OpenRGBClient::requestControllerCount()
{
    sendPacket(0, REQUEST_CONTROLLER_COUNT);
}

void OpenRGBClient::requestControllerData(int index)
{
    QByteArray data(4, 0);
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << (quint32)index;
    sendPacket(index, REQUEST_CONTROLLER_DATA);
}

void OpenRGBClient::sendPacket(quint32 deviceId, quint32 type, const QByteArray &data)
{
    QByteArray packet;
    packet.append(MAGIC);

    QDataStream ds(&packet, QIODevice::Append);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << deviceId << type << (quint32)data.size();
    packet.append(data);

    m_socket->write(packet);
}

void OpenRGBClient::onReadyRead()
{
    m_readBuffer.append(m_socket->readAll());

    // 헤더 파싱 (16바이트: magic(4) + device_id(4) + type(4) + size(4))
    while (m_readBuffer.size() >= 16) {
        // 매직 바이트 탐색 - 1바이트씩 제거 대신 indexOf로 O(N) 처리
        if (m_readBuffer.left(4) != MAGIC) {
            int idx = m_readBuffer.indexOf(MAGIC, 1);
            if (idx < 0) { m_readBuffer.clear(); break; }
            m_readBuffer.remove(0, idx);
            continue;
        }

        QDataStream ds(m_readBuffer.mid(4, 12));
        ds.setByteOrder(QDataStream::LittleEndian);
        quint32 deviceId, type, dataSize;
        ds >> deviceId >> type >> dataSize;

        if ((quint32)m_readBuffer.size() < 16 + dataSize)
            break; // 아직 데이터 다 안 왔음

        QByteArray payload = m_readBuffer.mid(16, dataSize);
        m_readBuffer.remove(0, 16 + dataSize);

        // 패킷 처리
        if (type == REQUEST_CONTROLLER_COUNT) {
            QDataStream cd(payload);
            cd.setByteOrder(QDataStream::LittleEndian);
            quint32 count;
            cd >> count;
            m_pendingControllerCount = count;
            m_loadedControllers = 0;
            m_devices.clear();
            qDebug() << "[OpenRGB]" << count << "controllers found";
            for (quint32 i = 0; i < count; ++i)
                requestControllerData(i);
        }
        else if (type == REQUEST_CONTROLLER_DATA) {
            parseControllerData(deviceId, payload);
        }
    }
}

void OpenRGBClient::parseControllerData(int index, const QByteArray &data)
{
    if (data.size() < 6) return;

    QDataStream ds(data);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint32 dataSize;
    ds >> dataSize;

    quint16 nameLen;
    ds >> nameLen;

    if (nameLen == 0 || nameLen > 512) return;

    QByteArray nameBuf(nameLen, 0);
    ds.readRawData(nameBuf.data(), nameLen);
    QString name = QString::fromUtf8(nameBuf).trimmed().remove('\0');

    RGBDevice dev;
    dev.id            = QString("openrgb_%1").arg(index);
    dev.name          = name;
    dev.manufacturer  = "OpenRGB";
    dev.type          = "Component";
    dev.ledCount      = 1;
    dev.leds          << QColor(0, 180, 255);
    dev.openrgbSource = true;
    dev.openrgbIndex  = index;

    // 타입 추론
    QString nl = name.toLower();
    if (nl.contains("motherboard") || nl.contains("mb") || nl.contains("asus") ||
        nl.contains("msi") || nl.contains("gigabyte") || nl.contains("asrock"))
        dev.type = "Motherboard";
    else if (nl.contains("gpu") || nl.contains("graphics") || nl.contains("geforce") ||
             nl.contains("radeon") || nl.contains("rtx") || nl.contains("rx "))
        dev.type = "GPU";
    else if (nl.contains("ram") || nl.contains("memory") || nl.contains("ddr") ||
             nl.contains("corsair") || nl.contains("g.skill") || nl.contains("trident"))
        dev.type = "RAM";
    else if (nl.contains("keyboard"))
        dev.type = "Keyboard";
    else if (nl.contains("mouse"))
        dev.type = "Mouse";
    else if (nl.contains("fan") || nl.contains("cooler") || nl.contains("aio"))
        dev.type = "Fan/Cooling";

    m_devices.append(dev);
    ++m_loadedControllers;

    if (m_loadedControllers >= m_pendingControllerCount) {
        qDebug() << "[OpenRGB] All controllers loaded:" << m_devices.size();
        emit devicesLoaded(m_devices);
    }
}

void OpenRGBClient::setDeviceColor(int deviceIndex, const QColor &color)
{
    if (!m_connected) return;

    // UPDATE_LEDS: 헤더(2) + N * RGBW(4)
    // OpenRGB 프로토콜: quint16 count, then per-LED: R G B speed(0)
    const int LED_COUNT = 1;
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << (quint16)LED_COUNT;
    for (int i = 0; i < LED_COUNT; ++i) {
        ds << (quint8)color.red()
           << (quint8)color.green()
           << (quint8)color.blue()
           << (quint8)0;   // speed / padding
    }
    sendPacket(deviceIndex, UPDATE_LEDS, data);
}

void OpenRGBClient::setDeviceColors(int deviceIndex, const QList<QColor> &colors)
{
    if (!m_connected || colors.isEmpty()) return;

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << (quint16)0;                    // zone index (0 = all)
    ds << (quint16)colors.size();       // led count

    for (const QColor &c : colors) {
        ds << (quint8)c.red() << (quint8)c.green() << (quint8)c.blue();
        ds << (quint8)0;  // speed/padding
    }

    sendPacket(deviceIndex, UPDATE_ZONE_LEDS, data);
}

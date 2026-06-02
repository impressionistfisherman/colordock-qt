#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QList>
#include <QColor>
#include <QTimer>
#include "../hardware/DeviceManager.h"

// OpenRGB 네트워크 프로토콜 클라이언트
// TCP 포트 6742, 매직 헤더 "ORGB"
class OpenRGBClient : public QObject
{
    Q_OBJECT
public:
    explicit OpenRGBClient(QObject *parent = nullptr);

    void connectToServer(const QString &host = "127.0.0.1", quint16 port = 6742);
    void disconnect();
    bool isConnected() const;

    void setDeviceColor(int deviceIndex, const QColor &color);
    void setDeviceColors(int deviceIndex, const QList<QColor> &colors);

signals:
    void connected();
    void disconnected();
    void devicesLoaded(const QList<RGBDevice> &devices);
    void error(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onRetryTimer();

private:
    // OpenRGB 패킷 타입
    enum PacketType {
        REQUEST_CONTROLLER_COUNT = 0,
        REQUEST_CONTROLLER_DATA  = 1,
        SET_CLIENT_NAME          = 50,
        UPDATE_LEDS              = 1050,
        UPDATE_ZONE_LEDS         = 1051,
        UPDATE_SINGLE_LED        = 1052,
    };

    void sendPacket(quint32 deviceId, quint32 type, const QByteArray &data = {});
    void requestControllerCount();
    void requestControllerData(int index);
    void parseControllerData(int index, const QByteArray &data);

    void setClientName();

    QTcpSocket   *m_socket;
    QTimer       *m_retryTimer;
    QByteArray    m_readBuffer;
    int           m_pendingControllerCount = 0;
    int           m_loadedControllers      = 0;
    QList<RGBDevice> m_devices;
    QString       m_host = "127.0.0.1";
    quint16       m_port = 6742;
    bool          m_connected = false;
};

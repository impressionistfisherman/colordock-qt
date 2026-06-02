#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

public slots:
    void check();

signals:
    void updateAvailable(const QString &version, const QString &downloadUrl);
    void noUpdate();

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam;
    static const QString CURRENT_VERSION;
    static const QString API_URL;
};

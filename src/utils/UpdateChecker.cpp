#include "UpdateChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QDebug>

const QString UpdateChecker::CURRENT_VERSION = "1.0.0";
const QString UpdateChecker::API_URL =
    "https://api.github.com/repos/impressionistfisherman/color-dock/releases/latest";

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onReplyFinished);
}

void UpdateChecker::check()
{
    QUrl url{API_URL};
    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader, "ColorDock/" + CURRENT_VERSION);
    req.setRawHeader("Accept", "application/vnd.github.v3+json");
    m_nam->get(req);
}

void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[UpdateChecker] Error:" << reply->errorString();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj   = doc.object();

    QString tag = obj["tag_name"].toString().remove('v');
    QString url = QString();

    // installer URL 찾기
    for (const auto &asset : obj["assets"].toArray()) {
        QString name = asset.toObject()["name"].toString();
        if (name.endsWith(".exe") && name.contains("Setup")) {
            url = asset.toObject()["browser_download_url"].toString();
            break;
        }
    }

    auto current = QVersionNumber::fromString(CURRENT_VERSION);
    auto latest  = QVersionNumber::fromString(tag);

    if (latest > current && !url.isEmpty()) {
        qDebug() << "[UpdateChecker] Update available:" << tag;
        emit updateAvailable(tag, url);
    } else {
        emit noUpdate();
    }
}

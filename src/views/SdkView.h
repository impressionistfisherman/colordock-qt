#pragma once
#include <QWidget>
#include <QMap>

class DeviceManager;
class QLabel;
class QGridLayout;
class QGroupBox;

class SdkChip : public QWidget {
    Q_OBJECT
public:
    explicit SdkChip(const QString &brand, const QString &status, QWidget *parent = nullptr);
    void setStatus(const QString &status);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString m_brand;
    QString m_status; // "Connected" | "Demo" | "Error"
};

class SdkView : public QWidget {
    Q_OBJECT
public:
    explicit SdkView(DeviceManager *dm, QWidget *parent = nullptr);

    void updateStatus(const QString &brand, const QString &status);

private:
    void setupUi();

    DeviceManager *m_dm;
    QMap<QString, SdkChip*> m_chips;
    QLabel *m_summaryLabel;

    static const QList<QPair<QString,QString>> SDK_BRANDS; // {id, displayName}
};

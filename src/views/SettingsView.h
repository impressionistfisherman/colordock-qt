#pragma once
#include <QWidget>

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(QWidget *parent = nullptr);

private slots:
    void onSaveOpenRGB();
    void onCheckUpdate();

private:
    void setupUi();
    void loadSettings();

    QLineEdit  *m_orgbHost;
    QSpinBox   *m_orgbPort;
    QCheckBox  *m_closeToTray;
    QCheckBox  *m_startMinimized;
    QCheckBox  *m_autoUpdate;
    QComboBox  *m_startupEffect;
    QLabel     *m_updateStatusLabel;
    QLabel     *m_versionLabel;
};

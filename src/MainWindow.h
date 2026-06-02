#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QPropertyAnimation>

class EffectsView;
class DevicesView;
class KeyboardView;
class ScenesView;
class AudioView;
class MonitoringView;
class ProfilesView;
class SettingsView;
class LayoutView;
class ScheduleView;
class SdkView;
class DeviceManager;
class UpdateChecker;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onNavItemClicked(int index);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onUpdateAvailable(const QString &version, const QString &url);
    void showWindow();
    void quitApp();
    void toggleTheme();
    void toggleNotifPanel();
    void addNotification(const QString &type, const QString &msg);
    void clearNotifications();

private:
    void setupUi();
    void setupSidebar();
    void setupTray();
    void setupUpdateChecker();
    void setupHeader();
    void addNavItem(const QString &icon, const QString &label, QWidget *view);
    void updateNavTitle(int index);

    // UI
    QWidget        *m_central;
    QHBoxLayout    *m_mainLayout;
    QWidget        *m_sidebar;
    QVBoxLayout    *m_sidebarLayout;
    QStackedWidget *m_stack;
    QList<QPushButton*> m_navBtns;

    // Header
    QWidget        *m_headerWidget;
    QLabel         *m_headerTitle;
    QLabel         *m_headerSubtitle;
    QLabel         *m_statusDot;
    QLabel         *m_statusText;
    QWidget        *m_updateBanner;
    QLabel         *m_updateBannerText;
    QString         m_latestZipUrl;
    QPushButton    *m_notifBellBtn;
    QLabel         *m_notifBadge;
    QWidget        *m_notifPanel;
    QVBoxLayout    *m_notifListLayout;
    int             m_notifCount = 0;
    bool            m_notifOpen  = false;

    static const QList<QPair<QString,QString>> NAV_TITLES; // {title, subtitle}

    // Views
    EffectsView    *m_effectsView;
    DevicesView    *m_devicesView;
    KeyboardView   *m_keyboardView;
    ScenesView     *m_scenesView;
    AudioView      *m_audioView;
    MonitoringView *m_monitoringView;
    ProfilesView   *m_profilesView;
    SettingsView   *m_settingsView;
    LayoutView     *m_layoutView;
    ScheduleView   *m_scheduleView;
    SdkView        *m_sdkView;

    // System
    DeviceManager  *m_deviceManager;
    UpdateChecker  *m_updateChecker;

    // Tray
    QSystemTrayIcon *m_tray;
    QMenu           *m_trayMenu;
    bool             m_closeToTray = true;

    // Theme
    QPushButton     *m_themeBtn;
    bool             m_isDark = true;
    void             applyTheme(bool dark);
};

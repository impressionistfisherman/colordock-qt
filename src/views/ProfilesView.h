#pragma once
#include <QWidget>
#include <QColor>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;

struct Profile {
    QString id;
    QString name;
    QString icon;
    QString effect;
    QColor  color;
    int     brightness = 100;
};

class DeviceManager;

class ProfilesView : public QWidget {
    Q_OBJECT
public:
    explicit ProfilesView(DeviceManager *dm = nullptr, QWidget *parent = nullptr);

private slots:
    void onAddProfile();
    void onDeleteProfile();
    void onApplyProfile();
    void onSelectionChanged();

private:
    void setupUi();
    void loadProfiles();
    void saveProfiles();
    void refreshList();
    QString profilesFilePath() const;
    void openEditDialog(Profile *p);

    QListWidget  *m_list;
    QPushButton  *m_applyBtn;
    QPushButton  *m_deleteBtn;
    QLabel       *m_detailLabel;

    DeviceManager *m_dm = nullptr;
    QList<Profile> m_profiles;
};

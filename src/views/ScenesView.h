#pragma once
#include <QWidget>
#include <QJsonArray>
#include <QColor>
#include <QTimer>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QStackedWidget;
class QSpinBox;

struct Scene {
    QString id;
    QString name;
    QString icon;
    QString effect;
    QColor  color;
};

struct PlaylistItem {
    QString sceneId;
    int     durationSec = 5;
};

struct Playlist {
    QString id;
    QString name;
    QList<PlaylistItem> items;
    bool loop = true;
};

class DeviceManager;

class ScenesView : public QWidget {
    Q_OBJECT
public:
    explicit ScenesView(DeviceManager *dm = nullptr, QWidget *parent = nullptr);
    void setCurrentEffect(const QString &effect, const QColor &color);
    const QList<Scene>& scenes() const { return m_scenes; }

private slots:
    void onSaveScene();
    void onDeleteScene();
    void onApplyScene();
    void onSceneSelectionChanged();
    // 플레이리스트
    void onAddPlaylist();
    void onDeletePlaylist();
    void onPlayPlaylist();
    void onStopPlaylist();
    void onPlaylistTick();

private:
    void setupUi();
    void loadScenes();
    void saveScenes();
    void refreshSceneList();
    void refreshPlaylistList();
    void loadPlaylists();
    void savePlaylists();
    QString scenesFilePath() const;
    QString playlistsFilePath() const;
    void openPlaylistEditor(Playlist *p);

    // 씬 탭
    QListWidget  *m_sceneList;
    QLabel       *m_sceneDetailLabel;
    QPushButton  *m_applyBtn;
    QPushButton  *m_deleteBtn;
    QLineEdit    *m_nameEdit;
    QComboBox    *m_iconCombo;

    // 플레이리스트 탭
    QListWidget  *m_plList;
    QPushButton  *m_plDeleteBtn;
    QPushButton  *m_plPlayBtn;
    QPushButton  *m_plStopBtn;
    QLabel       *m_plStatusLabel;
    QLabel       *m_plProgressLabel;

    QStackedWidget *m_stack;

    DeviceManager  *m_dm = nullptr;
    QList<Scene>    m_scenes;
    QList<Playlist> m_playlists;
    QString         m_currentEffect = "static";
    QColor          m_currentColor  = QColor(0, 180, 255);

    // 플레이리스트 재생 상태
    QTimer *m_plTimer;
    int     m_plActiveIdx   = -1;
    int     m_plItemIdx     = -1;
    int     m_plElapsed     = 0;
};

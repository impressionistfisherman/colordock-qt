#pragma once
#include <QWidget>
#include <QList>
#include <QColor>

class DeviceManager;
struct RGBDevice;
class QLineEdit;
class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QButtonGroup;

// ── 그라디언트 에디터 다이얼로그 ─────────────────────────────
class GradientEditorDialog : public QWidget {
    Q_OBJECT
public:
    explicit GradientEditorDialog(const QString &deviceId, int ledCount,
                                   const QList<QColor> &current, QWidget *parent = nullptr);
    QList<QColor> result() const { return m_colors; }

signals:
    void applied(const QString &deviceId, const QList<QColor> &colors);

private slots:
    void onLedClicked(int idx);
    void onPreset(const QString &name);

private:
    void buildLeds();
    QString      m_deviceId;
    QList<QColor> m_colors;
    QList<QPushButton*> m_ledBtns;
    QColor       m_paintColor = QColor(0, 180, 255);
};

// ── 기기 카드 위젯 ────────────────────────────────────────────
class DeviceCard : public QWidget {
    Q_OBJECT
public:
    explicit DeviceCard(const RGBDevice &dev, QWidget *parent = nullptr);
    void setColor(const QColor &c);

signals:
    void gradientRequested(const QString &id, int ledCount, const QList<QColor> &leds);
    void colorChangeRequested(const QString &id, const QColor &c);

private:
    QString m_id;
    int     m_ledCount;
    QList<QColor> m_leds;
    QLabel *m_colorBar;
};

// ── DevicesView ───────────────────────────────────────────────
class DevicesView : public QWidget {
    Q_OBJECT
public:
    explicit DevicesView(DeviceManager *dm, QWidget *parent = nullptr);

private slots:
    void onDevicesChanged();
    void onSearchChanged(const QString &text);
    void onCategoryFilter(const QString &cat);

private:
    void setupUi();
    void renderCards();
    bool matchesFilter(const RGBDevice &dev) const;

    DeviceManager *m_dm;
    QLabel        *m_statusLabel;
    QLabel        *m_countLabel;
    QLineEdit     *m_searchEdit;
    QWidget       *m_cardsContainer;
    QVBoxLayout   *m_cardsLayout;
    QScrollArea   *m_scroll;

    QString m_activeCategory = "all";
    QString m_searchText;

    QList<QPushButton*> m_filterBtns;
};

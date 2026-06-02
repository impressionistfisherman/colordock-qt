#pragma once
#include <QDialog>
#include <QList>

class QTableWidget;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QListWidget;
class DeviceManager;

struct CustomDevice {
    QString id;
    QString name;
    QString manufacturer;
    QString vendorId;
    QString productId;
    QString type;
    int     ledCount = 12;
};

class HidScannerDialog : public QDialog {
    Q_OBJECT
public:
    explicit HidScannerDialog(DeviceManager *dm, QWidget *parent = nullptr);

private slots:
    void onScan();
    void onSelectScanned(int row, int col);
    void onRegister();
    void onDeleteCustom();

private:
    void setupUi();
    void loadCustomDevices();
    void saveCustomDevices();
    void refreshCustomList();
    QString customDevicesFilePath() const;

    DeviceManager *m_dm;
    QTableWidget  *m_scanTable;
    QListWidget   *m_customList;
    QLabel        *m_scanStatus;

    QLineEdit *m_nameEdit;
    QLineEdit *m_mfgEdit;
    QLineEdit *m_vidEdit;
    QLineEdit *m_pidEdit;
    QComboBox *m_typeCombo;
    QSpinBox  *m_ledCount;

    QList<CustomDevice> m_customDevices;
};

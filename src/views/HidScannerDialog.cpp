#include "HidScannerDialog.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QMessageBox>
#include <QSplitter>

HidScannerDialog::HidScannerDialog(DeviceManager *dm, QWidget *parent)
    : QDialog(parent), m_dm(dm)
{
    setWindowTitle("🔌 미지원 디바이스 수동 등록 (USB HID Scanner)");
    setMinimumSize(820, 620);
    setupUi();
    loadCustomDevices();
    refreshCustomList();
}

void HidScannerDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // 안내
    auto *desc = new QLabel(
        "ColorDock은 USB RAW HID 통신 표준 규격을 지원합니다. "
        "시스템에 연결된 USB HID 장치 목록을 조회하여 VID/PID를 자동으로 검색해 커스텀 장치로 추가하세요."
    );
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#90A4AE;font-size:11px;");
    mainLayout->addWidget(desc);

    auto *splitter = new QSplitter(Qt::Vertical);

    // ── 상단: USB HID 스캔 ──────────────────────────────────
    auto *scanGroup = new QGroupBox("실시간 USB HID 장치 스캔");
    auto *scanLayout = new QVBoxLayout(scanGroup);

    auto *scanBtnRow = new QHBoxLayout;
    auto *scanBtn = new QPushButton("🔍 실시간 USB HID 장치 스캔 시작");
    scanBtn->setStyleSheet("background:#0277BD;font-weight:bold;padding:8px 16px;border-radius:8px;");
    connect(scanBtn, &QPushButton::clicked, this, &HidScannerDialog::onScan);
    m_scanStatus = new QLabel("스캔 버튼을 클릭하여 USB HID 장치를 검색하세요.");
    m_scanStatus->setStyleSheet("color:#90A4AE;font-size:11px;");
    scanBtnRow->addWidget(scanBtn);
    scanBtnRow->addWidget(m_scanStatus, 1);
    scanLayout->addLayout(scanBtnRow);

    m_scanTable = new QTableWidget(0, 5);
    m_scanTable->setHorizontalHeaderLabels({"제품명 (Product String)", "제조사", "Vendor ID", "Product ID", "선택"});
    m_scanTable->horizontalHeader()->setStretchLastSection(false);
    m_scanTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_scanTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_scanTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_scanTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_scanTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_scanTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_scanTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_scanTable->setAlternatingRowColors(true);
    m_scanTable->setMinimumHeight(140);
    connect(m_scanTable, &QTableWidget::cellClicked, this, &HidScannerDialog::onSelectScanned);
    scanLayout->addWidget(m_scanTable);

    splitter->addWidget(scanGroup);

    // ── 하단: 수동 등록 ──────────────────────────────────────
    auto *regGroup = new QGroupBox("✨ 신규 디바이스 정보 등록");
    auto *regLayout = new QVBoxLayout(regGroup);

    auto *formGrid = new QGridLayout;
    formGrid->setSpacing(8);

    auto addField = [&](int r, int c, const QString &label, QWidget *w) {
        formGrid->addWidget(new QLabel(label), r, c*2);
        formGrid->addWidget(w, r, c*2+1);
    };

    m_nameEdit = new QLineEdit; m_nameEdit->setPlaceholderText("예: 독거미 Aula F75");
    m_mfgEdit  = new QLineEdit; m_mfgEdit->setPlaceholderText("예: Aula");
    m_vidEdit  = new QLineEdit; m_vidEdit->setPlaceholderText("예: 0x258A  (없으면 공백)");
    m_pidEdit  = new QLineEdit; m_pidEdit->setPlaceholderText("예: 0x0011  (없으면 공백)");

    m_typeCombo = new QComboBox;
    for (const QString &t : {"Keyboard","Mouse","Fan","LED Strip","Smart Light",
                              "AIO Cooler","Headset","Monitor","GPU","Memory","Other"})
        m_typeCombo->addItem(t);

    m_ledCount = new QSpinBox;
    m_ledCount->setRange(1, 512);
    m_ledCount->setValue(12);

    addField(0, 0, "기기 표시 이름",  m_nameEdit);
    addField(0, 1, "제조사 이름",     m_mfgEdit);
    addField(1, 0, "Vendor ID (선택)", m_vidEdit);
    addField(1, 1, "Product ID (선택)",m_pidEdit);
    addField(2, 0, "디바이스 분류",   m_typeCombo);
    addField(2, 1, "LED 개수",        m_ledCount);

    regLayout->addLayout(formGrid);

    auto *regBtn = new QPushButton("✅ 디바이스 등록 (재시작 없이 영구 저장)");
    regBtn->setStyleSheet("background:#2E7D32;font-weight:bold;padding:8px;border-radius:8px;");
    connect(regBtn, &QPushButton::clicked, this, &HidScannerDialog::onRegister);
    regLayout->addWidget(regBtn);

    // 저장된 커스텀 기기 목록
    auto *savedLabel = new QLabel("💾 저장된 커스텀 기기");
    savedLabel->setStyleSheet("font-weight:bold;color:#90A4AE;margin-top:4px;");
    regLayout->addWidget(savedLabel);

    auto *savedRow = new QHBoxLayout;
    m_customList = new QListWidget;
    m_customList->setMaximumHeight(100);
    m_customList->setStyleSheet("QListWidget{border:1px solid rgba(255,255,255,0.1);border-radius:6px;}");
    auto *delBtn = new QPushButton("🗑 삭제");
    delBtn->setStyleSheet("background:#B71C1C;min-width:60px;");
    connect(delBtn, &QPushButton::clicked, this, &HidScannerDialog::onDeleteCustom);
    savedRow->addWidget(m_customList, 1);
    savedRow->addWidget(delBtn, 0, Qt::AlignTop);
    regLayout->addLayout(savedRow);

    splitter->addWidget(regGroup);
    mainLayout->addWidget(splitter, 1);

    // 닫기
    auto *closeBtn = new QPushButton("닫기");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    mainLayout->addLayout(btnRow);
}

void HidScannerDialog::onScan()
{
    m_scanStatus->setText("🔄 스캔 중...");
    m_scanTable->setRowCount(0);

    QProcess proc;
    proc.start("powershell.exe", {
        "-NoProfile", "-Command",
        "Get-PnpDevice -Class HIDClass -Status OK | "
        "Select-Object FriendlyName, Manufacturer, DeviceID | "
        "ConvertTo-Csv -NoTypeInformation"
    });
    proc.waitForFinished(8000);

    QString out = proc.readAllStandardOutput();
    QStringList lines = out.split('\n', Qt::SkipEmptyParts);
    if (lines.size() < 2) {
        m_scanStatus->setText("⚠ 스캔 실패 또는 결과 없음.");
        return;
    }

    int count = 0;
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i].trimmed().remove('"');
        QStringList cols = line.split(',');
        if (cols.size() < 3) continue;

        QString name    = cols[0].trimmed();
        QString mfg     = cols[1].trimmed();
        QString devId   = cols[2].trimmed();

        // VID/PID 파싱 (예: HID\VID_258A&PID_0011...)
        QString vid, pid;
        QRegularExpression re("VID_([0-9A-Fa-f]+).*PID_([0-9A-Fa-f]+)");
        auto m = re.match(devId);
        if (m.hasMatch()) {
            vid = "0x" + m.captured(1).toUpper();
            pid = "0x" + m.captured(2).toUpper();
        }

        int row = m_scanTable->rowCount();
        m_scanTable->insertRow(row);
        m_scanTable->setItem(row, 0, new QTableWidgetItem(name.isEmpty() ? "(이름 없음)" : name));
        m_scanTable->setItem(row, 1, new QTableWidgetItem(mfg));
        m_scanTable->setItem(row, 2, new QTableWidgetItem(vid));
        m_scanTable->setItem(row, 3, new QTableWidgetItem(pid));
        auto *selBtn = new QPushButton("선택");
        selBtn->setStyleSheet("font-size:10px;padding:2px 8px;");
        connect(selBtn, &QPushButton::clicked, this, [this, row]() { onSelectScanned(row, 4); });
        m_scanTable->setCellWidget(row, 4, selBtn);
        ++count;
    }

    m_scanStatus->setText(QString("✅ %1개 HID 장치 검색됨. 행을 클릭하면 아래에 자동 입력됩니다.").arg(count));
}

void HidScannerDialog::onSelectScanned(int row, int)
{
    if (row < 0 || row >= m_scanTable->rowCount()) return;
    auto cell = [this, row](int c) {
        auto *item = m_scanTable->item(row, c);
        return item ? item->text() : QString();
    };
    m_nameEdit->setText(cell(0));
    m_mfgEdit->setText(cell(1));
    m_vidEdit->setText(cell(2));
    m_pidEdit->setText(cell(3));
}

void HidScannerDialog::onRegister()
{
    QString name = m_nameEdit->text().trimmed();
    QString mfg  = m_mfgEdit->text().trimmed();
    if (name.isEmpty() || mfg.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "기기 이름과 제조사는 필수입니다.");
        return;
    }

    CustomDevice dev;
    dev.id          = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    dev.name        = name;
    dev.manufacturer= mfg;
    dev.vendorId    = m_vidEdit->text().trimmed();
    dev.productId   = m_pidEdit->text().trimmed();
    dev.type        = m_typeCombo->currentText();
    dev.ledCount    = m_ledCount->value();
    m_customDevices << dev;
    saveCustomDevices();
    refreshCustomList();

    // DeviceManager에 즉시 추가
    if (m_dm) {
        RGBDevice rgbDev;
        rgbDev.id           = "custom_" + dev.id;
        rgbDev.name         = dev.name;
        rgbDev.manufacturer = dev.manufacturer;
        rgbDev.type         = dev.type;
        rgbDev.ledCount     = dev.ledCount;
        rgbDev.leds         = QList<QColor>(dev.ledCount, QColor(0,180,255));
        rgbDev.openrgbSource= false;
        // DeviceManager에 직접 추가하는 공개 슬롯
        m_dm->addCustomDevice(rgbDev);
    }

    m_nameEdit->clear(); m_mfgEdit->clear();
    m_vidEdit->clear();  m_pidEdit->clear();
    QMessageBox::information(this, "등록 완료",
        QString("'%1' 기기가 등록됐습니다. 기기 목록에 바로 표시됩니다.").arg(name));
}

void HidScannerDialog::onDeleteCustom()
{
    int idx = m_customList->currentRow();
    if (idx < 0 || idx >= m_customDevices.size()) return;
    m_customDevices.removeAt(idx);
    saveCustomDevices();
    refreshCustomList();
}

QString HidScannerDialog::customDevicesFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    return dir + "/custom_devices.json";
}

void HidScannerDialog::loadCustomDevices()
{
    QFile f(customDevicesFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    auto arr = QJsonDocument::fromJson(f.readAll()).array();
    m_customDevices.clear();
    for (const auto &v : arr) {
        auto o = v.toObject();
        CustomDevice d;
        d.id           = o["id"].toString();
        d.name         = o["name"].toString();
        d.manufacturer = o["manufacturer"].toString();
        d.vendorId     = o["vendorId"].toString();
        d.productId    = o["productId"].toString();
        d.type         = o["type"].toString("Other");
        d.ledCount     = o["ledCount"].toInt(12);
        m_customDevices << d;
    }
}

void HidScannerDialog::saveCustomDevices()
{
    QJsonArray arr;
    for (const CustomDevice &d : m_customDevices) {
        QJsonObject o;
        o["id"] = d.id; o["name"] = d.name;
        o["manufacturer"] = d.manufacturer;
        o["vendorId"] = d.vendorId; o["productId"] = d.productId;
        o["type"] = d.type; o["ledCount"] = d.ledCount;
        arr << o;
    }
    QFile f(customDevicesFilePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

void HidScannerDialog::refreshCustomList()
{
    m_customList->clear();
    for (const CustomDevice &d : m_customDevices) {
        m_customList->addItem(QString("%1  |  %2  |  VID:%3 PID:%4  |  LED %5개")
            .arg(d.name).arg(d.type).arg(d.vendorId).arg(d.productId).arg(d.ledCount));
    }
    if (m_customDevices.isEmpty())
        m_customList->addItem("저장된 커스텀 기기 없음");
}

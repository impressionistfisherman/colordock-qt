#include "ProfilesView.h"
#include "../hardware/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>
#include <QColorDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QMessageBox>

static const QStringList PROFILE_ICONS = {"🎮","💻","🌙","🔥","🎵","🏠","⚡","🌈","🧘","❄️"};
static const QStringList EFFECT_NAMES  = {"rainbow","breathing","static","wave","strobe","disabled"};
static const QStringList EFFECT_LABELS = {"🌈 무지개","💫 브리딩","⚪ 단색 고정","🌊 웨이브","⚡ 스트로브","⬛ 끄기"};

ProfilesView::ProfilesView(DeviceManager *dm, QWidget *parent) : QWidget(parent), m_dm(dm)
{
    setupUi();
    loadProfiles();
    refreshList();
}

void ProfilesView::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 헤더
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("👤  사용자 프로필");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#4FC3F7;");
    auto *addBtn = new QPushButton("+ 새 프로필");
    addBtn->setStyleSheet("background:#0277BD;font-weight:bold;");
    connect(addBtn, &QPushButton::clicked, this, &ProfilesView::onAddProfile);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(addBtn);
    mainLayout->addLayout(header);

    // 프로필 목록
    auto *listBox = new QGroupBox("저장된 프로필");
    auto *listLayout = new QVBoxLayout(listBox);

    m_list = new QListWidget;
    m_list->setStyleSheet(R"(
        QListWidget { border:none; }
        QListWidget::item { padding:10px; border-radius:6px; margin:2px 0; }
        QListWidget::item:hover { background:rgba(79,195,247,0.08); }
        QListWidget::item:selected { background:rgba(79,195,247,0.18); color:#4FC3F7; }
    )");
    connect(m_list, &QListWidget::itemSelectionChanged, this, &ProfilesView::onSelectionChanged);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &ProfilesView::onApplyProfile);
    listLayout->addWidget(m_list, 1);

    auto *btnRow = new QHBoxLayout;
    m_detailLabel = new QLabel;
    m_detailLabel->setStyleSheet("color:#90A4AE;font-size:12px;");
    m_applyBtn  = new QPushButton("▶ 적용");
    m_deleteBtn = new QPushButton("🗑 삭제");
    auto *editBtn  = new QPushButton("✏ 편집");

    m_applyBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    editBtn->setEnabled(false);
    m_applyBtn->setStyleSheet("background:#2E7D32;min-width:80px;");
    m_deleteBtn->setStyleSheet("background:#B71C1C;min-width:80px;");

    connect(m_applyBtn,  &QPushButton::clicked, this, &ProfilesView::onApplyProfile);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ProfilesView::onDeleteProfile);
    connect(editBtn, &QPushButton::clicked, this, [this, editBtn]() {
        int idx = m_list->currentRow();
        if (idx >= 0 && idx < m_profiles.size())
            openEditDialog(&m_profiles[idx]);
    });

    // editBtn도 selection 따라 켜기
    connect(m_list, &QListWidget::itemSelectionChanged, this, [editBtn, this]() {
        int idx = m_list->currentRow();
        editBtn->setEnabled(idx >= 0 && idx < m_profiles.size());
    });

    btnRow->addWidget(m_detailLabel, 1);
    btnRow->addWidget(editBtn);
    btnRow->addWidget(m_applyBtn);
    btnRow->addWidget(m_deleteBtn);
    listLayout->addLayout(btnRow);
    mainLayout->addWidget(listBox, 1);
}

QString ProfilesView::profilesFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    return dir + "/profiles.json";
}

void ProfilesView::loadProfiles()
{
    QFile f(profilesFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    auto arr = QJsonDocument::fromJson(f.readAll()).array();
    m_profiles.clear();
    for (const auto &v : arr) {
        auto o = v.toObject();
        Profile p;
        p.id         = o["id"].toString();
        p.name       = o["name"].toString();
        p.icon       = o["icon"].toString("🎮");
        p.effect     = o["effect"].toString("rainbow");
        p.color      = QColor(o["color"].toString("#00b4ff"));
        p.brightness = o["brightness"].toInt(100);
        m_profiles << p;
    }
}

void ProfilesView::saveProfiles()
{
    QJsonArray arr;
    for (const Profile &p : m_profiles) {
        QJsonObject o;
        o["id"]         = p.id;
        o["name"]       = p.name;
        o["icon"]       = p.icon;
        o["effect"]     = p.effect;
        o["color"]      = p.color.name();
        o["brightness"] = p.brightness;
        arr << o;
    }
    QFile f(profilesFilePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

void ProfilesView::refreshList()
{
    m_list->clear();
    for (const Profile &p : m_profiles) {
        QString text = QString("%1  %2\n   효과: %3  |  밝기: %4%  |  색상: %5")
            .arg(p.icon).arg(p.name).arg(p.effect).arg(p.brightness).arg(p.color.name());
        auto *item = new QListWidgetItem(text);
        QPixmap px(14, 14);
        px.fill(p.color);
        item->setIcon(QIcon(px));
        m_list->addItem(item);
    }
    if (m_profiles.isEmpty()) {
        m_list->addItem("저장된 프로필이 없습니다. '+ 새 프로필'을 눌러 추가하세요.");
        m_list->item(0)->setForeground(QColor(100, 130, 150));
        m_list->item(0)->setFlags(m_list->item(0)->flags() & ~Qt::ItemIsSelectable);
    }
}

void ProfilesView::openEditDialog(Profile *p)
{
    bool isNew = (p == nullptr);
    Profile tmp;
    if (isNew) {
        tmp.id         = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        tmp.icon       = "🎮";
        tmp.brightness = 100;
        tmp.color      = QColor(0, 180, 255);
        tmp.effect     = "rainbow";
        p = &tmp;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? "새 프로필" : "프로필 편집");
    dlg.setMinimumWidth(340);
    auto *form = new QFormLayout(&dlg);
    form->setSpacing(10);

    auto *iconCombo = new QComboBox;
    for (const QString &ic : PROFILE_ICONS) iconCombo->addItem(ic);
    iconCombo->setCurrentText(p->icon);

    auto *nameEdit = new QLineEdit(p->name);
    nameEdit->setPlaceholderText("프로필 이름");

    auto *effectCombo = new QComboBox;
    for (int i = 0; i < EFFECT_NAMES.size(); ++i)
        effectCombo->addItem(EFFECT_LABELS[i], EFFECT_NAMES[i]);
    effectCombo->setCurrentIndex(EFFECT_NAMES.indexOf(p->effect));

    auto *brightSlider = new QSlider(Qt::Horizontal);
    brightSlider->setRange(5, 100);
    brightSlider->setValue(p->brightness);
    auto *brightLabel = new QLabel(QString("%1%").arg(p->brightness));
    auto *brightRow = new QHBoxLayout;
    brightRow->addWidget(brightSlider);
    brightRow->addWidget(brightLabel);
    connect(brightSlider, &QSlider::valueChanged, this, [brightLabel](int v) {
        brightLabel->setText(QString("%1%").arg(v));
    });

    QColor selectedColor = p->color;
    auto *colorBtn = new QPushButton;
    colorBtn->setFixedSize(36, 36);
    colorBtn->setStyleSheet(QString("background:%1;border-radius:18px;border:2px solid rgba(255,255,255,0.3);").arg(selectedColor.name()));
    connect(colorBtn, &QPushButton::clicked, this, [&selectedColor, colorBtn, &dlg]() {
        QColor c = QColorDialog::getColor(selectedColor, &dlg, "색상 선택");
        if (c.isValid()) {
            selectedColor = c;
            colorBtn->setStyleSheet(QString("background:%1;border-radius:18px;border:2px solid rgba(255,255,255,0.3);").arg(c.name()));
        }
    });

    form->addRow("아이콘:", iconCombo);
    form->addRow("이름:", nameEdit);
    form->addRow("효과:", effectCombo);
    form->addRow("밝기:", brightRow);
    form->addRow("색상:", colorBtn);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted) return;

    p->icon       = iconCombo->currentText();
    p->name       = nameEdit->text().trimmed();
    p->effect     = effectCombo->currentData().toString();
    p->brightness = brightSlider->value();
    p->color      = selectedColor;
    if (p->name.isEmpty()) return;

    if (isNew) m_profiles << *p;
    saveProfiles();
    refreshList();
}

void ProfilesView::onAddProfile()
{
    openEditDialog(nullptr);
}

void ProfilesView::onDeleteProfile()
{
    int idx = m_list->currentRow();
    if (idx < 0 || idx >= m_profiles.size()) return;
    if (QMessageBox::question(this, "프로필 삭제",
        QString("'%1' 을 삭제합니까?").arg(m_profiles[idx].name))
        != QMessageBox::Yes) return;
    m_profiles.removeAt(idx);
    saveProfiles();
    refreshList();
    m_applyBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_detailLabel->clear();
}

void ProfilesView::onApplyProfile()
{
    int idx = m_list->currentRow();
    if (idx < 0 || idx >= m_profiles.size()) return;
    const Profile &p = m_profiles[idx];
    if (m_dm) {
        QColor adjusted = p.color;
        adjusted.setHsvF(adjusted.hueF(), adjusted.saturationF(), p.brightness / 100.0);
        m_dm->applyEffect(p.effect, adjusted);
    }
    m_detailLabel->setText(QString("✅ '%1' 적용됨").arg(p.name));
}

void ProfilesView::onSelectionChanged()
{
    int idx = m_list->currentRow();
    bool valid = (idx >= 0 && idx < m_profiles.size());
    m_applyBtn->setEnabled(valid);
    m_deleteBtn->setEnabled(valid);
    if (valid) {
        const Profile &p = m_profiles[idx];
        m_detailLabel->setText(QString("%1 %2  •  %3  •  밝기 %4%")
            .arg(p.icon).arg(p.name).arg(p.effect).arg(p.brightness));
    }
}

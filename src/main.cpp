#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ColorDock");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Changsik Noh");

    // Fusion 스타일 (다크 테마 적용 용이)
    app.setStyle(QStyleFactory::create("Fusion"));

    // 다크 팔레트
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor(26, 26, 46));
    darkPalette.setColor(QPalette::WindowText,       QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Base,             QColor(15, 15, 30));
    darkPalette.setColor(QPalette::AlternateBase,    QColor(22, 33, 62));
    darkPalette.setColor(QPalette::ToolTipBase,      QColor(26, 26, 46));
    darkPalette.setColor(QPalette::ToolTipText,      QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Text,             QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Button,           QColor(22, 33, 62));
    darkPalette.setColor(QPalette::ButtonText,       QColor(224, 224, 224));
    darkPalette.setColor(QPalette::BrightText,       Qt::red);
    darkPalette.setColor(QPalette::Link,             QColor(79, 195, 247));
    darkPalette.setColor(QPalette::Highlight,        QColor(0, 180, 255));
    darkPalette.setColor(QPalette::HighlightedText,  Qt::black);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text,       QColor(100, 100, 100));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(100, 100, 100));
    app.setPalette(darkPalette);

    app.setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #1A1A2E;
            color: #E0E0E0;
            font-family: 'Segoe UI', sans-serif;
            font-size: 13px;
        }
        QPushButton {
            background-color: #0F3460;
            color: #E0E0E0;
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 6px 14px;
        }
        QPushButton:hover { background-color: #1565C0; }
        QPushButton:pressed { background-color: #0D47A1; }
        QPushButton:checked { background-color: #0288D1; border-color: #4FC3F7; }
        QScrollBar:vertical {
            background: #16213E; width: 8px; border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #0F3460; border-radius: 4px; min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QLineEdit, QComboBox, QSpinBox {
            background-color: #16213E;
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 6px;
            padding: 5px 8px;
            color: #E0E0E0;
        }
        QLineEdit:focus, QComboBox:focus { border-color: #4FC3F7; }
        QLabel { color: #E0E0E0; }
        QGroupBox {
            border: 1px solid rgba(255,255,255,0.08);
            border-radius: 10px;
            margin-top: 8px;
            padding-top: 8px;
            color: #90CAF9;
            font-weight: bold;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; }
        QSlider::groove:horizontal {
            height: 4px; background: #16213E; border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #4FC3F7; width: 14px; height: 14px;
            border-radius: 7px; margin: -5px 0;
        }
        QSlider::sub-page:horizontal { background: #0288D1; border-radius: 2px; }
        QTabWidget::pane { border: 1px solid rgba(255,255,255,0.08); border-radius: 8px; }
        QTabBar::tab {
            background: #16213E; color: #90A4AE;
            padding: 6px 14px; border-radius: 6px 6px 0 0;
        }
        QTabBar::tab:selected { background: #0F3460; color: #E0E0E0; }
        QListWidget, QTreeWidget {
            background: #16213E;
            border: 1px solid rgba(255,255,255,0.08);
            border-radius: 8px;
        }
        QListWidget::item:selected { background: #0F3460; }
        QHeaderView::section {
            background: #16213E; color: #90A4AE;
            border: none; padding: 4px 8px;
        }
    )");

    MainWindow w;
    w.show();

    return app.exec();
}

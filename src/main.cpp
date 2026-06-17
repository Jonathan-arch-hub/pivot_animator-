#include <QApplication>
#include <QStyleFactory>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Pivot Animator Pro");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("PivotPro");

    // Dark theme
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor(30,30,40));
    darkPalette.setColor(QPalette::WindowText,       QColor(220,220,230));
    darkPalette.setColor(QPalette::Base,             QColor(22,22,32));
    darkPalette.setColor(QPalette::AlternateBase,    QColor(38,38,52));
    darkPalette.setColor(QPalette::ToolTipBase,      QColor(50,50,70));
    darkPalette.setColor(QPalette::ToolTipText,      QColor(220,220,230));
    darkPalette.setColor(QPalette::Text,             QColor(220,220,230));
    darkPalette.setColor(QPalette::Button,           QColor(45,45,60));
    darkPalette.setColor(QPalette::ButtonText,       QColor(220,220,230));
    darkPalette.setColor(QPalette::BrightText,       Qt::red);
    darkPalette.setColor(QPalette::Link,             QColor(80,140,255));
    darkPalette.setColor(QPalette::Highlight,        QColor(55,138,221));
    darkPalette.setColor(QPalette::HighlightedText,  Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text,       QColor(100,100,120));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(100,100,120));
    app.setPalette(darkPalette);

    app.setStyleSheet(R"(
        QToolBar { background: #1e1e28; border-bottom: 1px solid #3a3a52; spacing: 4px; padding: 4px; }
        QToolButton { background: #2d2d3e; border: 1px solid #3a3a52; border-radius: 5px;
                      color: #dcdce6; padding: 5px 10px; font-size: 12px; }
        QToolButton:hover { background: #3a3a52; }
        QToolButton:checked { background: #185FA5; border-color: #378ADD; color: white; }
        QDockWidget { titlebar-close-icon: none; color: #dcdce6; }
        QDockWidget::title { background: #1e1e28; padding: 5px; border-bottom: 1px solid #3a3a52; }
        QGroupBox { border: 1px solid #3a3a52; border-radius: 6px; margin-top: 8px; color: #9090aa; font-size: 11px; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; }
        QLabel { color: #dcdce6; }
        QSlider::groove:horizontal { height: 4px; background: #3a3a52; border-radius: 2px; }
        QSlider::handle:horizontal { width: 14px; height: 14px; background: #378ADD; border-radius: 7px; margin: -5px 0; }
        QSlider::sub-page:horizontal { background: #378ADD; border-radius: 2px; }
        QSpinBox, QDoubleSpinBox { background: #22222e; border: 1px solid #3a3a52; border-radius: 4px;
                                    color: #dcdce6; padding: 2px 4px; }
        QComboBox { background: #22222e; border: 1px solid #3a3a52; border-radius: 4px;
                    color: #dcdce6; padding: 2px 8px; }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView { background: #2d2d3e; color: #dcdce6; selection-background-color: #185FA5; }
        QListWidget { background: #16161e; border: none; color: #dcdce6; }
        QListWidget::item:selected { background: #185FA5; color: white; }
        QListWidget::item:hover { background: #2d2d3e; }
        QScrollBar:vertical { background: #16161e; width: 8px; }
        QScrollBar::handle:vertical { background: #3a3a52; border-radius: 4px; }
        QMenuBar { background: #1e1e28; color: #dcdce6; border-bottom: 1px solid #3a3a52; }
        QMenuBar::item:selected { background: #2d2d3e; }
        QMenu { background: #2d2d3e; border: 1px solid #3a3a52; color: #dcdce6; }
        QMenu::item:selected { background: #185FA5; }
        QStatusBar { background: #1e1e28; color: #9090aa; border-top: 1px solid #3a3a52; font-size: 11px; }
        QTextEdit { background: #12121a; color: #a9b7d0; font-family: monospace; border: none; }
        QTabWidget::pane { border: 1px solid #3a3a52; background: #1e1e28; }
        QTabBar::tab { background: #22222e; color: #9090aa; padding: 6px 14px; border: 1px solid #3a3a52;
                       border-bottom: none; border-radius: 4px 4px 0 0; }
        QTabBar::tab:selected { background: #1e1e28; color: #dcdce6; }
        QSplitter::handle { background: #3a3a52; }
        QPushButton { background: #2d2d3e; border: 1px solid #3a3a52; border-radius: 5px;
                      color: #dcdce6; padding: 5px 14px; }
        QPushButton:hover { background: #3a3a52; }
        QPushButton:pressed { background: #185FA5; }
        QLineEdit { background: #22222e; border: 1px solid #3a3a52; border-radius: 4px;
                    color: #dcdce6; padding: 3px 6px; }
        QCheckBox { color: #dcdce6; }
        QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #3a3a52;
                               border-radius: 3px; background: #22222e; }
        QCheckBox::indicator:checked { background: #378ADD; border-color: #378ADD; }
    )");

    MainWindow w;
    w.show();
    return app.exec();
}

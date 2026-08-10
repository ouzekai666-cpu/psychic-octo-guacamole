// =============================================================================
// main.cpp — Entry point for 渠沟检测标定
// =============================================================================
#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("\u6E20\u6C9F\u68C0\u6D4B\u6807\u5B9A"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setOrganizationName(QStringLiteral("IndustrialInspection"));

    // Global application style
    app.setStyleSheet(QStringLiteral(
        "QMainWindow { background-color: #F1F5F9; }"
        "QScrollArea { background-color: #F1F5F9; }"
    ));

    MainWindow window;
    window.show();

    return app.exec();
}

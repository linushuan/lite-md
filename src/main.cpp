#include <QApplication>
#include <QStringList>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("mded");
    app.setApplicationVersion("0.2.0");

    // Collect file paths from CLI arguments
    QStringList filesToOpen;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        filesToOpen.append(args.at(i));
    }

    MainWindow window(filesToOpen);
    window.show();

    return app.exec();
}

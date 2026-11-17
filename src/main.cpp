#include "ui/MainWindow.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::SimplifiedChineseScript, QLocale::China));

    QTranslator qtTranslator;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QString translationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
    const QString translationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
    if (qtTranslator.load("qt_zh_CN", translationsPath) ||
        qtTranslator.load("qt_zh_CN", app.applicationDirPath() + "/translations")) {
        app.installTranslator(&qtTranslator);
    }

    MainWindow window;
    window.show();
    return app.exec();
}

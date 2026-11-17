#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QSplitter>
#include <QToolBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>

// Offscreen checks for presentation state only; does not run model calculations.
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    MainWindow window;
    int failures = 0;
    const auto check = [&failures](bool passed, const char* name) {
        if (!passed) { std::fprintf(stderr, "FAIL: %s\n", name); ++failures; }
    };
    check(!window.isWindowModified(), "initial project is clean");
    auto* toolbar = window.findChild<QToolBar*>();
    check(toolbar && !toolbar->isMovable(), "fixed toolbar");
    check(window.findChild<QSplitter*>() != nullptr, "resizable result panels");
    int readonlyTables = 0;
    QTableWidget* inputTable = nullptr;
    for (auto* table : window.findChildren<QTableWidget*>()) {
        if (table->editTriggers() == QAbstractItemView::NoEditTriggers) ++readonlyTables;
        else inputTable = table;
    }
    check(readonlyTables == 2, "results are read-only");
    check(inputTable != nullptr, "input table exists");
    if (inputTable) {
        inputTable->item(0, 1)->setText("界面测试");
        check(window.isWindowModified(), "cell edit marks project dirty");
        check(!window.findChild<ConstraintPlotWidget*>()->isEnabled(), "stale plot is disabled");
        window.setWindowModified(false);
        auto* combo = qobject_cast<QComboBox*>(inputTable->cellWidget(0, 0));
        if (combo) combo->setCurrentIndex((combo->currentIndex() + 1) % combo->count());
        check(window.isWindowModified(), "type selection marks project dirty");
    }
    window.setWindowModified(false);
    for (auto* edit : window.findChildren<QLineEdit*>()) {
        if (!edit->validator()) continue;
        for (const QString& text : {QString("0"), QString("12.5"), QString("1.2e-4")}) {
            edit->setText(text);
            check(edit->hasAcceptableInput(), "ordinary numeric input accepted");
        }
        for (const QString& text : {QString(), QString("abc"), QString("1,25"), QString("1e")}) {
            edit->setText(text);
            check(!edit->hasAcceptableInput(), "invalid or incomplete numeric input rejected");
        }
        edit->setText("1.2e-4");
        QMetaObject::invokeMethod(edit, "textEdited", Q_ARG(QString, edit->text()));
        check(window.isWindowModified(), "text edit marks project dirty");
    }
    // Generic storage smoke check, not an end-to-end project save/load test.
    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary directory created");
    if (temporary.isValid()) {
        const QJsonObject sample{{"name", QString("中文测试：长度与时间")},
                                 {"length_m", 12.5}, {"duration_s", 90.0},
                                 {"small_number", 1.2e-4}};
        const QByteArray bytes = QJsonDocument(sample).toJson();
        const QString path = temporary.filePath("普通数据.json");
        QSaveFile output(path);
        const bool saved = output.open(QIODevice::WriteOnly)
            && output.write(bytes) == bytes.size() && output.commit();
        check(saved, "generic JSON saved atomically");
        QFile input(path);
        check(input.open(QIODevice::ReadOnly), "generic JSON reopened");
        const auto restored = QJsonDocument::fromJson(input.readAll()).object();
        check(restored == sample, "Chinese text and ordinary numbers round-trip");
        check(std::abs(restored["small_number"].toDouble() - 1.2e-4) < 1e-15,
              "small number retains precision");
    }
    std::printf("UI checks: %s\n", failures ? "FAILED" : "PASSED");
    return failures ? 1 : 0;
}

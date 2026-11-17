#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QSplitter>
#include <QToolBar>
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
        edit->setText("1.2e-4");
        check(edit->hasAcceptableInput(), "scientific notation accepted");
        QMetaObject::invokeMethod(edit, "textEdited", Q_ARG(QString, edit->text()));
        check(window.isWindowModified(), "text edit marks project dirty");
        break;
    }
    std::printf("UI checks: %s\n", failures ? "FAILED" : "PASSED");
    return failures ? 1 : 0;
}

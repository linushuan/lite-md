#include "mainwindow.h"
#include "editor/TabManager.h"
#include "editor/EditorWidget.h"
#include "editor/SessionManager.h"
#include "config/Settings.h"

#include <QCloseEvent>
#include <QDialog>
#include <QFileDialog>
#include <QFile>
#include <QShortcut>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

MainWindow::MainWindow(const QStringList &filesToOpen, QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("mded");

    // Load settings
    Settings settings = Settings::load();
    resize(settings.windowWidth, settings.windowHeight);

    // Create TabManager as central widget
    tabManager_ = new TabManager(this);
    setCentralWidget(tabManager_);

    // Status bar
    statusBar_ = new QStatusBar(this);
    setStatusBar(statusBar_);
    statusBar_->showMessage("Ready");

    // Connect signals
    connect(tabManager_, &TabManager::currentEditorChanged,
            this, &MainWindow::onCurrentEditorChanged);

    // Setup shortcuts
    setupShortcuts();

    // Restore session or open files from CLI
    if (!filesToOpen.isEmpty()) {
        for (const auto &path : filesToOpen) {
            tabManager_->openFile(path);
        }
    } else {
        restoreSession();
    }

    // Ensure at least one tab
    if (tabManager_->count() == 0) {
        tabManager_->addEmptyTab();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Batch-ask about all unsaved tabs
    if (tabManager_->hasUnsavedChanges()) {
        // Collect unsaved tabs
        QStringList unsavedFiles;
        QList<int> unsavedIndices;
        for (int i = 0; i < tabManager_->count(); ++i) {
            auto *editor = tabManager_->editorAt(i);
            if (editor && editor->isModified()) {
                unsavedIndices.append(i);
                QString name = editor->filePath().isEmpty()
                    ? QString("Untitled %1").arg(i + 1)
                    : editor->filePath();
                unsavedFiles.append(name);
            }
        }

        // Show a dialog with checkboxes for each unsaved file
        QDialog dialog(this);
        dialog.setWindowTitle("Unsaved Changes");
        auto *layout = new QVBoxLayout(&dialog);
        layout->addWidget(new QLabel("The following files have unsaved changes.\nSelect which to save:"));

        QList<QCheckBox *> checkboxes;
        for (const auto &name : unsavedFiles) {
            auto *cb = new QCheckBox(name, &dialog);
            cb->setChecked(true);
            layout->addWidget(cb);
            checkboxes.append(cb);
        }

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Discard | QDialogButtonBox::Cancel,
            &dialog);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons->button(QDialogButtonBox::Discard), &QPushButton::clicked,
                &dialog, &QDialog::reject);
        connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
                [&dialog]() { dialog.done(2); });

        int result = dialog.exec();
        if (result == 2) {  // Cancel
            event->ignore();
            return;
        }
        if (result == QDialog::Accepted) {
            for (int i = 0; i < checkboxes.size(); ++i) {
                if (checkboxes[i]->isChecked()) {
                    tabManager_->editorAt(unsavedIndices[i])->save();
                }
            }
        }
        // Discard: just close without saving
    }

    saveSession();
    event->accept();
}

void MainWindow::onCurrentEditorChanged(EditorWidget *editor)
{
    if (!editor) return;

    // Update status bar with cursor position, word count, etc.
    statusBar_->showMessage(
        QString("Line: %1  Col: %2  |  %3")
            .arg(editor->cursorLine())
            .arg(editor->cursorColumn())
            .arg(editor->filePath().isEmpty() ? "Untitled" : editor->filePath())
    );
}

void MainWindow::onNewTab()
{
    tabManager_->addEmptyTab();
}

void MainWindow::onOpenFile()
{
    QString path = QFileDialog::getOpenFileName(this, "Open Markdown File",
                                                 QString(), "Markdown (*.md *.markdown);;All Files (*)");
    if (!path.isEmpty()) {
        tabManager_->openFile(path);
    }
}

void MainWindow::onSave()
{
    auto *editor = tabManager_->currentEditor();
    if (editor) editor->save();
}

void MainWindow::onSaveAs()
{
    auto *editor = tabManager_->currentEditor();
    if (!editor) return;

    QString path = QFileDialog::getSaveFileName(this, "Save As",
                                                 QString(), "Markdown (*.md *.markdown);;All Files (*)");
    if (!path.isEmpty()) {
        editor->saveAs(path);
    }
}

void MainWindow::onCloseTab()
{
    tabManager_->closeCurrentTab();
}

void MainWindow::onNextTab()
{
    int next = (tabManager_->currentIndex() + 1) % tabManager_->count();
    tabManager_->setCurrentIndex(next);
}

void MainWindow::onPrevTab()
{
    int prev = (tabManager_->currentIndex() - 1 + tabManager_->count()) % tabManager_->count();
    tabManager_->setCurrentIndex(prev);
}

void MainWindow::onJumpToTab(int index)
{
    if (index < tabManager_->count()) {
        tabManager_->setCurrentIndex(index);
    } else if (tabManager_->count() > 0) {
        tabManager_->setCurrentIndex(tabManager_->count() - 1);
    }
}

void MainWindow::setupShortcuts()
{
    auto addShortcut = [this](const QKeySequence &key, auto slot) {
        auto *sc = new QShortcut(key, this);
        connect(sc, &QShortcut::activated, this, slot);
    };

    addShortcut(QKeySequence("Ctrl+T"),       &MainWindow::onNewTab);
    addShortcut(QKeySequence("Ctrl+O"),       &MainWindow::onOpenFile);
    addShortcut(QKeySequence("Ctrl+S"),       &MainWindow::onSave);
    addShortcut(QKeySequence("Ctrl+Shift+S"), &MainWindow::onSaveAs);
    addShortcut(QKeySequence("Ctrl+W"),       &MainWindow::onCloseTab);
    addShortcut(QKeySequence("Ctrl+Tab"),     &MainWindow::onNextTab);
    addShortcut(QKeySequence("Ctrl+Shift+Tab"), &MainWindow::onPrevTab);
    addShortcut(QKeySequence("Ctrl+Q"),       [this]() { close(); });

    // Ctrl+1..9
    for (int i = 1; i <= 9; ++i) {
        auto *sc = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i)), this);
        connect(sc, &QShortcut::activated, this, [this, i]() { onJumpToTab(i - 1); });
    }

    // Font size
    auto *zoomIn = new QShortcut(QKeySequence("Ctrl++"), this);
    connect(zoomIn, &QShortcut::activated, this, [this]() {
        auto *e = tabManager_->currentEditor();
        if (e) e->zoomIn();
    });
    auto *zoomOut = new QShortcut(QKeySequence("Ctrl+-"), this);
    connect(zoomOut, &QShortcut::activated, this, [this]() {
        auto *e = tabManager_->currentEditor();
        if (e) e->zoomOut();
    });
    auto *zoomReset = new QShortcut(QKeySequence("Ctrl+0"), this);
    connect(zoomReset, &QShortcut::activated, this, [this]() {
        auto *e = tabManager_->currentEditor();
        if (e) e->zoomReset();
    });

    // Focus mode, fullscreen
    auto *focus = new QShortcut(QKeySequence("F11"), this);
    connect(focus, &QShortcut::activated, this, [this]() {
        auto *e = tabManager_->currentEditor();
        if (e) e->toggleFocusMode();
    });
    auto *fullscreen = new QShortcut(QKeySequence("F12"), this);
    connect(fullscreen, &QShortcut::activated, this, [this]() {
        if (isFullScreen()) showNormal();
        else showFullScreen();
    });

    // Search
    auto *search = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(search, &QShortcut::activated, this, [this]() {
        auto *e = tabManager_->currentEditor();
        if (e) e->showSearchBar();
    });

    // Toggle line numbers
    auto *toggleLineNumbers = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(toggleLineNumbers, &QShortcut::activated, this, [this]() {
        auto *e = tabManager_->currentEditor();
        if (e) e->toggleLineNumbers();
    });

    // Toggle word wrap
    auto *toggleWordWrap = new QShortcut(QKeySequence("Ctrl+Shift+W"), this);
    connect(toggleWordWrap, &QShortcut::activated, this, [this]() {
        auto *e = tabManager_->currentEditor();
        if (e) e->toggleWordWrap();
    });
}

void MainWindow::restoreSession()
{
    Settings settings = Settings::load();
    if (!settings.restoreSession) return;

    auto session = SessionManager::load();
    for (const auto &path : session.openFiles) {
        if (QFile::exists(path)) {
            tabManager_->openFile(path);
        }
    }

    for (int i = 0; i < session.cursorLines.size() && i < tabManager_->count(); ++i) {
        auto *editor = tabManager_->editorAt(i);
        if (editor) {
            editor->setCursorLine(session.cursorLines[i]);
        }
    }

    if (session.activeIndex >= 0 && session.activeIndex < tabManager_->count()) {
        tabManager_->setCurrentIndex(session.activeIndex);
    }
}

void MainWindow::saveSession()
{
    SessionManager::Session session;
    session.openFiles = tabManager_->openFilePaths();
    session.activeIndex = tabManager_->currentIndex();

    for (int i = 0; i < tabManager_->count(); ++i) {
        auto *editor = tabManager_->editorAt(i);
        session.cursorLines.append(editor ? editor->cursorLine() : 0);
    }

    SessionManager::save(session);
}

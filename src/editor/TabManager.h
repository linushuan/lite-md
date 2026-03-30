#pragma once

#include <QWidget>
#include <QTabBar>
#include <QStackedWidget>
#include <QFileSystemWatcher>
#include <QToolButton>
#include <QStringList>

class EditorWidget;

class TabManager : public QWidget {
    Q_OBJECT
public:
    explicit TabManager(QWidget *parent = nullptr);

    // Open / create
    int  addEmptyTab();
    int  openFile(const QString &path);
    void closeTab(int index);
    void closeCurrentTab();

    // Access
    EditorWidget *currentEditor() const;
    EditorWidget *editorAt(int index) const;
    int           count() const;
    int           currentIndex() const;
    void          setCurrentIndex(int index);

    // State
    bool        hasUnsavedChanges() const;
    QStringList openFilePaths() const;

signals:
    void currentEditorChanged(EditorWidget *editor);
    void tabCountChanged(int count);

private slots:
    void onTabCloseRequested(int index);
    void onEditorModifiedChanged(bool modified);
    void onCurrentChanged(int index);
    void onFileWatcherTriggered(const QString &path);

private:
    QTabBar            *tabBar_;
    QStackedWidget     *stack_;
    QFileSystemWatcher *watcher_;
    QToolButton        *addTabBtn_;

    void updateTabTitle(int index);
    bool confirmClose(EditorWidget *editor);
    int  findTabByPath(const QString &path);
};

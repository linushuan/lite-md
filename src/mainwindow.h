#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QStatusBar>

class TabManager;
class EditorWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QStringList &filesToOpen = {}, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onCurrentEditorChanged(EditorWidget *editor);
    void onNewTab();
    void onOpenFile();
    void onSave();
    void onSaveAs();
    void onCloseTab();
    void onNextTab();
    void onPrevTab();
    void onJumpToTab(int index);

private:
    TabManager *tabManager_;
    QStatusBar *statusBar_;

    void setupShortcuts();
    void restoreSession();
    void saveSession();
};

#pragma once

#include <QPlainTextEdit>
#include <QString>

class LineNumberArea;
class MdHighlighter;
class QInputMethodEvent;

class MdEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit MdEditor(QWidget *parent = nullptr);

    void    loadFile(const QString &path);
    bool    saveFile(const QString &path = QString());
    QString currentFilePath() const;
    bool    isModified() const;

    // Line number area support
    int  lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void setFocusModeEnabled(bool enabled);
    bool isFocusModeEnabled() const;

    void setLineNumbersVisible(bool visible);
    bool lineNumbersVisible() const;

    void setWordWrapEnabled(bool enabled);
    bool isWordWrapEnabled() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;

signals:
    void fileChanged(const QString &path);
    void cursorPositionChanged(int line, int col);
    void wordCountChanged(int words, int chars);
    void modifiedChanged(bool modified);

private:
    LineNumberArea *lineNumberArea_;
    MdHighlighter  *highlighter_;
    QString         currentFile_;
    bool            focusModeEnabled_ = false;
    bool            lineNumbersVisible_ = true;

    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
};

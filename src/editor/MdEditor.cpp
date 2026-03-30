#include "MdEditor.h"
#include "LineNumberArea.h"
#include "highlight/MdHighlighter.h"
#include "config/Theme.h"

#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QScrollBar>
#include <QTextOption>

MdEditor::MdEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    // Initialize line number area
    lineNumberArea_ = new LineNumberArea(this);

    // Initialize highlighter with dark theme
    highlighter_ = new MdHighlighter(document(), Theme::darkDefault());

    // Connect signals for line number area
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &MdEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &MdEditor::updateLineNumberArea);

    // Current line highlight
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &MdEditor::highlightCurrentLine);

    // Forward modification changed signal
    connect(document(), &QTextDocument::modificationChanged,
            this, &MdEditor::modifiedChanged);

    // Initial setup
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Word wrap on by default
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    // Tab -> 4 spaces
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
}

void MdEditor::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    setPlainText(in.readAll());
    file.close();

    currentFile_ = path;
    document()->setModified(false);
    emit fileChanged(path);
}

bool MdEditor::saveFile(const QString &path)
{
    QString savePath = path.isEmpty() ? currentFile_ : path;
    if (savePath.isEmpty())
        return false;

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << toPlainText();
    file.close();

    currentFile_ = savePath;
    document()->setModified(false);
    emit fileChanged(savePath);
    return true;
}

QString MdEditor::currentFilePath() const
{
    return currentFile_;
}

bool MdEditor::isModified() const
{
    return document()->isModified();
}

int MdEditor::lineNumberAreaWidth() const
{
    if (!lineNumbersVisible_)
        return 0;

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void MdEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!lineNumbersVisible_)
        return;

    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), QColor("#1e1e2e"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#585b70"));
            painter.drawText(0, top, lineNumberArea_->width() - 2,
                           fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void MdEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(),
                                       lineNumberAreaWidth(), cr.height()));
}

void MdEditor::keyPressEvent(QKeyEvent *event)
{
    // Tab -> 4 spaces
    if (event->key() == Qt::Key_Tab) {
        insertPlainText("    ");
        return;
    }

    // Auto-indent on Enter
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QString currentLine = textCursor().block().text();
        int indent = 0;
        for (QChar c : currentLine) {
            if (c == ' ') indent++;
            else break;
        }
        QPlainTextEdit::keyPressEvent(event);
        if (indent > 0) {
            insertPlainText(QString(indent, ' '));
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void MdEditor::inputMethodEvent(QInputMethodEvent *event)
{
    // Pause highlighter during IME composition to avoid excessive rehighlights
    bool composing = !event->preeditString().isEmpty();
    highlighter_->setEnabled(!composing);
    QPlainTextEdit::inputMethodEvent(event);
    if (!composing) {
        highlighter_->rehighlightBlock(textCursor().block());
    }
}

void MdEditor::setFocusModeEnabled(bool enabled)
{
    if (focusModeEnabled_ == enabled)
        return;

    focusModeEnabled_ = enabled;
    highlightCurrentLine();
}

bool MdEditor::isFocusModeEnabled() const
{
    return focusModeEnabled_;
}

void MdEditor::setLineNumbersVisible(bool visible)
{
    if (lineNumbersVisible_ == visible)
        return;

    lineNumbersVisible_ = visible;
    lineNumberArea_->setVisible(visible);
    updateLineNumberAreaWidth(0);
    viewport()->update();
}

bool MdEditor::lineNumbersVisible() const
{
    return lineNumbersVisible_;
}

void MdEditor::setWordWrapEnabled(bool enabled)
{
    setWordWrapMode(enabled
        ? QTextOption::WrapAtWordBoundaryOrAnywhere
        : QTextOption::NoWrap);
}

bool MdEditor::isWordWrapEnabled() const
{
    return wordWrapMode() != QTextOption::NoWrap;
}

void MdEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void MdEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea_->scroll(0, dy);
    else
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void MdEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        if (focusModeEnabled_) {
            QTextCursor current = textCursor();
            const int currentBlock = current.blockNumber();

            QColor dimColor = palette().text().color();
            dimColor.setAlphaF(0.3);

            for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
                if (block.blockNumber() == currentBlock)
                    continue;

                QTextEdit::ExtraSelection dimSel;
                dimSel.format.setForeground(dimColor);
                dimSel.cursor = QTextCursor(block);
                dimSel.cursor.select(QTextCursor::LineUnderCursor);
                extraSelections.append(dimSel);
            }
        }

        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor("#2a2a3e"));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

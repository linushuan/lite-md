// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 linushuan

#include <QTest>
#include <QCoreApplication>
#include <QTextCursor>
#include <QTextBlock>
#include <QFontInfo>
#include <QPalette>
#include <QColor>
#include <QFocusEvent>
#include <QInputMethodEvent>
#include <QTemporaryFile>

#include "editor/MdEditor.h"
#include "config/Settings.h"

namespace {
int effectiveTabSize()
{
    int tabSize = Settings::load().tabSize;
    if (tabSize < 1) tabSize = 1;
    if (tabSize > 16) tabSize = 16;
    return tabSize;
}

QString spaces(int count)
{
    return QString(count, QLatin1Char(' '));
}
}

class TestMdEditor : public QObject {
    Q_OBJECT

private slots:
    void init()
    {
        editor_ = new MdEditor();
        editor_->resize(640, 360);
        editor_->show();
        editor_->setFocus();
    }

    void cleanup()
    {
        delete editor_;
        editor_ = nullptr;
    }

    void testOrderedListEnterAutoIncrement()
    {
        editor_->setPlainText("1. first");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("1. first"));
        QCOMPARE(lines.value(1), QString("2. "));
    }

    void testOrderedListEnterAfterNestedChildKeepsSiblingNumbering()
    {
        editor_->setPlainText("1. adfasdf\n    - asdfa\n2. asfsdf");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("1. adfasdf"));
        QCOMPARE(lines.value(1), QString("    - asdfa"));
        QCOMPARE(lines.value(2), QString("2. asfsdf"));
        QCOMPARE(lines.value(3), QString("3. "));
    }

    void testOrderedEmptyStarterEnterAutoIncrement()
    {
        editor_->setPlainText("1. ");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("1. "));
        QCOMPARE(lines.value(1), QString("2. "));
    }

    void testShiftEnterSkipsListAutocomplete()
    {
        editor_->setPlainText("- item");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return, Qt::ShiftModifier);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("- item"));
        QCOMPARE(lines.value(1), QString(""));
    }

    void testImePreeditEnterDoesNotTriggerListAutocomplete()
    {
        editor_->setPlainText("1. item");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        QTest::keyClick(editor_, Qt::Key_Return);
        QCOMPARE(editor_->toPlainText(), QString("1. item"));

        QInputMethodEvent commitEvent(QString(), {});
        commitEvent.setCommitString(QStringLiteral("中"));
        QCoreApplication::sendEvent(editor_, &commitEvent);

        QCOMPARE(editor_->toPlainText(), QString("1. item中"));
    }

    void testImePreeditTabDoesNotTriggerListIndent()
    {
        editor_->setPlainText("- ");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QInputMethodEvent preeditEvent(QStringLiteral("a"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        QTest::keyClick(editor_, Qt::Key_Tab);
        QCOMPARE(editor_->toPlainText(), QString("- "));

        QInputMethodEvent commitEvent(QString(), {});
        commitEvent.setCommitString(QStringLiteral("阿"));
        QCoreApplication::sendEvent(editor_, &commitEvent);

        QCOMPARE(editor_->toPlainText(), QString("- 阿"));
    }

    void testImeCancelPreeditResumesListAutocomplete()
    {
        editor_->setPlainText("1. item");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        // Simulate IME cancel (preedit cleared without commit text).
        QInputMethodEvent cancelEvent(QString(), {});
        QCoreApplication::sendEvent(editor_, &cancelEvent);

        QTest::keyClick(editor_, Qt::Key_Return);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("1. item"));
        QCOMPARE(lines.value(1), QString("2. "));
    }

    void testImePreeditOnEmptyLineNormalizesInheritedForeground()
    {
        editor_->clear();

        QTextCharFormat staleFmt;
        staleFmt.setForeground(Qt::red);
        editor_->mergeCurrentCharFormat(staleFmt);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        const QColor expected = editor_->palette().color(QPalette::Text);
        QCOMPARE(editor_->currentCharFormat().foreground().color(), expected);

        QInputMethodEvent cancelEvent(QString(), {});
        QCoreApplication::sendEvent(editor_, &cancelEvent);
    }

    void testImePreeditOnWhitespaceTabLineNormalizesInheritedForeground()
    {
        editor_->setPlainText(QStringLiteral(" \t  "));

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTextCharFormat staleFmt;
        staleFmt.setForeground(Qt::red);
        editor_->mergeCurrentCharFormat(staleFmt);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        const QColor expected = editor_->palette().color(QPalette::Text);
        QCOMPARE(editor_->currentCharFormat().foreground().color(), expected);

        QInputMethodEvent cancelEvent(QString(), {});
        QCoreApplication::sendEvent(editor_, &cancelEvent);
    }

    void testImePreeditAfterBackslashPrefixNormalizesInheritedForeground()
    {
        editor_->setPlainText(QStringLiteral(" \t\\"));

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTextCharFormat staleFmt;
        staleFmt.setForeground(Qt::red);
        editor_->mergeCurrentCharFormat(staleFmt);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        const QColor expected = editor_->palette().color(QPalette::Text);
        QCOMPARE(editor_->currentCharFormat().foreground().color(), expected);

        QInputMethodEvent cancelEvent(QString(), {});
        QCoreApplication::sendEvent(editor_, &cancelEvent);
    }

    void testImePreeditAfterCommittedTextNormalizesInheritedForeground()
    {
        editor_->setPlainText(QStringLiteral("abc"));

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTextCharFormat staleFmt;
        staleFmt.setForeground(Qt::red);
        editor_->mergeCurrentCharFormat(staleFmt);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        const QColor expected = editor_->palette().color(QPalette::Text);
        QCOMPARE(editor_->currentCharFormat().foreground().color(), expected);

        QInputMethodEvent cancelEvent(QString(), {});
        QCoreApplication::sendEvent(editor_, &cancelEvent);
    }

    void testFocusOutDuringImeCompositionRestoresEditorShortcuts()
    {
        editor_->setPlainText("plain text");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        QFocusEvent focusOutEvent(QEvent::FocusOut, Qt::ActiveWindowFocusReason);
        QCoreApplication::sendEvent(editor_, &focusOutEvent);

        editor_->setFocus();
        QCoreApplication::processEvents();

        const QString beforeEnter = editor_->toPlainText();
        QTest::keyClick(editor_, Qt::Key_Return);
        const QString afterEnter = editor_->toPlainText();

        QCOMPARE(beforeEnter, QString("plain text"));
        QCOMPARE(afterEnter, QString("plain text\n"));
    }

    void testLoadFileResetsImeCompositionState()
    {
        editor_->setPlainText("1. item");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QInputMethodEvent preeditEvent(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(editor_, &preeditEvent);

        QTemporaryFile file;
        QVERIFY(file.open());
        file.write("fresh");
        file.flush();

        editor_->loadFile(file.fileName());

        cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);
        QCOMPARE(editor_->toPlainText(), QString("fresh\n"));
    }

    void testSecondEnterOnEmptyUnorderedListExitsList()
    {
        editor_->setPlainText("- first");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);
        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("- first\n"));
    }

    void testShiftEnterOnEmptyUnorderedListCreatesPlainNewline()
    {
        editor_->setPlainText("- first");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);
        QTest::keyClick(editor_, Qt::Key_Return, Qt::ShiftModifier);

        QCOMPARE(editor_->toPlainText(), QString("- first\n- \n"));
    }

    void testShiftEnterOnEmptyUnorderedListExitsList()
    {
        testShiftEnterOnEmptyUnorderedListCreatesPlainNewline();
    }

    void testParenthesisAutopairAtLineEnd()
    {
        editor_->clear();

        QTest::keyClicks(editor_, "(");

        QCOMPARE(editor_->toPlainText(), QString("()"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 1);
    }

    void testParenthesisNoAutopairInMiddleOfText()
    {
        editor_->setPlainText("ab");

        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(1);
        editor_->setTextCursor(cursor);

        QTest::keyClicks(editor_, "(");

        QCOMPARE(editor_->toPlainText(), QString("a(b"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testTypingClosingParenSkipsExistingCloser()
    {
        editor_->setPlainText("()");

        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(1);
        editor_->setTextCursor(cursor);

        QTest::keyClicks(editor_, ")");

        QCOMPARE(editor_->toPlainText(), QString("()"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testBracketAutopairAtLineEnd_data()
    {
        QTest::addColumn<QString>("typed");
        QTest::addColumn<QString>("expected");

        QTest::newRow("square") << QString("[") << QString("[]");
        QTest::newRow("curly") << QString("{") << QString("{}");
    }

    void testBracketAutopairAtLineEnd()
    {
        QFETCH(QString, typed);
        QFETCH(QString, expected);

        editor_->clear();
        QTest::keyClicks(editor_, typed);

        QCOMPARE(editor_->toPlainText(), expected);
        QCOMPARE(editor_->textCursor().positionInBlock(), 1);
    }

    void testTypingClosingBracketSkipsExistingCloser_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<int>("cursorPos");
        QTest::addColumn<QString>("typed");

        QTest::newRow("square") << QString("[]") << 1 << QString("]");
        QTest::newRow("curly") << QString("{}") << 1 << QString("}");
    }

    void testTypingClosingBracketSkipsExistingCloser()
    {
        QFETCH(QString, line);
        QFETCH(int, cursorPos);
        QFETCH(QString, typed);

        editor_->setPlainText(line);

        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(cursorPos);
        editor_->setTextCursor(cursor);

        QTest::keyClicks(editor_, typed);

        QCOMPARE(editor_->toPlainText(), line);
        QCOMPARE(editor_->textCursor().positionInBlock(), cursorPos + 1);
    }

    void testAngleBracketAutopairAndSkipOverCloser()
    {
        editor_->clear();

        QTest::keyClicks(editor_, "<");
        QCOMPARE(editor_->toPlainText(), QString("<>"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 1);

        QTest::keyClicks(editor_, ">");
        QCOMPARE(editor_->toPlainText(), QString("<>"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testAngleBracketNoAutopairInMiddleOfText()
    {
        editor_->setPlainText("ab");

        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(1);
        editor_->setTextCursor(cursor);

        QTest::keyClicks(editor_, "<");

        QCOMPARE(editor_->toPlainText(), QString("a<b"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testDollarAutopairAndSkipOverCloser()
    {
        editor_->clear();

        QTest::keyClicks(editor_, "$");
        QCOMPARE(editor_->toPlainText(), QString("$$"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 1);

        QTest::keyClicks(editor_, "$");
        QCOMPARE(editor_->toPlainText(), QString("$$"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testBacktickAutopairAndTripleBacktickInput()
    {
        editor_->clear();

        QTest::keyClicks(editor_, "`");
        QCOMPARE(editor_->toPlainText(), QString("``"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 1);

        QTest::keyClicks(editor_, "`");
        QCOMPARE(editor_->toPlainText(), QString("``"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);

        QTest::keyClicks(editor_, "`");
        QCOMPARE(editor_->toPlainText(), QString("```"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 3);
    }

    void testBackspaceRemovesEmptyPairedDelimiters_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<int>("cursorPos");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<int>("expectedCursorPos");

        QTest::newRow("paren") << QString("()") << 1 << QString("") << 0;
        QTest::newRow("square") << QString("[]") << 1 << QString("") << 0;
        QTest::newRow("curly") << QString("{}") << 1 << QString("") << 0;
        QTest::newRow("angle") << QString("<>") << 1 << QString("") << 0;
        QTest::newRow("dollar") << QString("$$") << 1 << QString("") << 0;
        QTest::newRow("backtick") << QString("``") << 1 << QString("") << 0;
    }

    void testBackspaceRemovesEmptyPairedDelimiters()
    {
        QFETCH(QString, line);
        QFETCH(int, cursorPos);
        QFETCH(QString, expected);
        QFETCH(int, expectedCursorPos);

        editor_->setPlainText(line);

        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(cursorPos);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Backspace);

        QCOMPARE(editor_->toPlainText(), expected);
        QCOMPARE(editor_->textCursor().positionInBlock(), expectedCursorPos);
    }

    void testBackspaceInsideTripleBacktickRemovesSingleBacktick_data()
    {
        QTest::addColumn<int>("cursorPos");
        QTest::addColumn<int>("expectedCursorPos");

        QTest::newRow("between-first-second") << 1 << 0;
        QTest::newRow("between-second-third") << 2 << 1;
    }

    void testBackspaceInsideTripleBacktickRemovesSingleBacktick()
    {
        QFETCH(int, cursorPos);
        QFETCH(int, expectedCursorPos);

        editor_->setPlainText("```");

        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(cursorPos);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Backspace);

        QCOMPARE(editor_->toPlainText(), QString("``"));
        QCOMPARE(editor_->textCursor().positionInBlock(), expectedCursorPos);
    }

    void testBackspaceOnMultilineLatexFenceOpeningRemovesSingleDollar()
    {
        editor_->setPlainText("$$\n\n$$");

        QTextBlock opening = editor_->document()->findBlockByNumber(0);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(opening.position() + 1);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Backspace);

        QCOMPARE(editor_->toPlainText(), QString("$\n\n$$"));
        QCOMPARE(editor_->textCursor().blockNumber(), 0);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testLatexFenceEnterCreatesClosingFence()
    {
        editor_->setPlainText("$$");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("$$\n\n$$"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testCodeFenceEnterCreatesClosingFenceWithLanguage()
    {
        editor_->setPlainText("```python");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("```python\n\n```"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testTildeCodeFenceEnterCreatesMatchingClosingFence()
    {
        editor_->setPlainText("~~~python");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("~~~python\n\n~~~"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testEnterOnBacktickFenceInsideOpenTildeFenceDoesNotAutoClose()
    {
        editor_->setPlainText("~~~\n```");

        QTextBlock backtickLine = editor_->document()->findBlockByNumber(1);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(backtickLine.position() + qMax(0, backtickLine.length() - 1));
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("~~~\n```\n"));
        QCOMPARE(editor_->textCursor().blockNumber(), 2);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testLatexBeginEnvEnterCreatesClosingLine()
    {
        editor_->setPlainText("\\begin{equation}");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("\\begin{equation}\n\n\\end{equation}"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testLatexBeginEnvWithIndentAndStarInNameCreatesClosingLine()
    {
        editor_->setPlainText("  \\begin{align*}");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("  \\begin{align*}\n  \n  \\end{align*}"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testLatexBeginEnvWithTrailingContentDoesNotAutoClose()
    {
        editor_->setPlainText("\\begin{equation} x = 1");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("\\begin{equation} x = 1\n"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testLatexBeginEnvInBlockquoteKeepsPrefix()
    {
        editor_->setPlainText("> \\begin{align}");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("> \\begin{align}\n> \n> \\end{align}"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testEnterOnClosingCodeFenceDoesNotDuplicateFence()
    {
        editor_->setPlainText("```python\nline\n```");

        QTextBlock closing = editor_->document()->findBlockByNumber(2);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(closing.position() + qMax(0, closing.length() - 1));
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("```python\nline\n```\n"));
        QCOMPARE(editor_->textCursor().blockNumber(), 3);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testEnterOnClosingLatexFenceDoesNotDuplicateFence()
    {
        editor_->setPlainText("$$\nline\n$$");

        QTextBlock closing = editor_->document()->findBlockByNumber(2);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(closing.position() + qMax(0, closing.length() - 1));
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("$$\nline\n$$\n"));
        QCOMPARE(editor_->textCursor().blockNumber(), 3);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testBlockquoteEnterAutocompletesPrefix()
    {
        editor_->setPlainText("> quote");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("> quote\n> "));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testBlockquoteEnterKeepsCurrentIndentAndDepth()
    {
        editor_->setPlainText("   > > deep quote");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("   > > deep quote\n   > > "));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 7);
    }

    void testBlockquoteEnterAutocompletesCompactPrefix()
    {
        editor_->setPlainText(">>compact");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString(">>compact\n>> "));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 3);
    }

    void testSecondEnterOnEmptyBlockquoteExitsQuote()
    {
        editor_->setPlainText("> quote");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);
        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString("> quote\n"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testSecondEnterOnEmptyCompactBlockquoteExitsQuote()
    {
        editor_->setPlainText(">>compact");

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);
        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), QString(">>compact\n"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testEnterAfterHorizontalRuleDoesNotStartList_data()
    {
        QTest::addColumn<QString>("line");

        QTest::newRow("compact-stars") << QString("***");
        QTest::newRow("spaced-dashes") << QString("- - -");
        QTest::newRow("spaced-stars") << QString("* * *");
    }

    void testEnterAfterHorizontalRuleDoesNotStartList()
    {
        QFETCH(QString, line);

        editor_->setPlainText(line);

        QTextCursor cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Return);

        QCOMPARE(editor_->toPlainText(), line + QString("\n"));
        QCOMPARE(editor_->textCursor().blockNumber(), 1);
        QCOMPARE(editor_->textCursor().positionInBlock(), 0);
    }

    void testTabIndentOrderedListAndRecount()
    {
        const int tabSize = effectiveTabSize();
        editor_->setPlainText("1. alpha\n2. \n3. gamma");

        QTextBlock second = editor_->document()->findBlockByNumber(1);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(second.position());
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Tab);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("1. alpha"));
        QCOMPARE(lines.value(1), spaces(tabSize) + QString("1. "));
        QCOMPARE(lines.value(2), QString("2. gamma"));
    }

    void testBacktabOutdentOrderedListAndRecount()
    {
        const int tabSize = effectiveTabSize();
        editor_->setPlainText(QString("1. alpha\n") + spaces(tabSize) + QString("1. \n2. gamma"));

        QTextBlock second = editor_->document()->findBlockByNumber(1);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(second.position());
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Backtab);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("1. alpha"));
        QCOMPARE(lines.value(1), QString("2. "));
        QCOMPARE(lines.value(2), QString("3. gamma"));
    }

    void testTabIndentOrderedSubtreeRenumbersSiblings()
    {
        const int tabSize = effectiveTabSize();
        editor_->setPlainText("1. parent\n2. child-a\n3. child-b\n4. tail");

        QTextBlock second = editor_->document()->findBlockByNumber(1);
        QTextBlock third = editor_->document()->findBlockByNumber(2);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(second.position());
        cursor.setPosition(third.position() + qMax(0, third.length() - 1), QTextCursor::KeepAnchor);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Tab);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("1. parent"));
        QCOMPARE(lines.value(1), spaces(tabSize) + QString("1. child-a"));
        QCOMPARE(lines.value(2), spaces(tabSize) + QString("2. child-b"));
        QCOMPARE(lines.value(3), QString("2. tail"));
    }

    void testTabSelectionMovesListBlock()
    {
        const int tabSize = effectiveTabSize();
        editor_->setPlainText("- parent\n  child\n- peer");

        QTextBlock first = editor_->document()->findBlockByNumber(0);
        QTextBlock second = editor_->document()->findBlockByNumber(1);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(first.position());
        cursor.setPosition(second.position() + qMax(0, second.length() - 1), QTextCursor::KeepAnchor);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Tab);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), spaces(tabSize) + QString("- parent"));
        QCOMPARE(lines.value(1), spaces(tabSize + 2) + QString("child"));
        QCOMPARE(lines.value(2), QString("- peer"));
    }

    void testTabOnNonEmptyListDoesNotMoveSubtree()
    {
        const int tabSize = effectiveTabSize();
        editor_->setPlainText("- parent\n  child\n- peer");

        QTextBlock first = editor_->document()->findBlockByNumber(0);
        QTextCursor cursor(editor_->document());
        cursor.setPosition(first.position() + first.length() - 1);
        editor_->setTextCursor(cursor);

        QTest::keyClick(editor_, Qt::Key_Tab);

        const QStringList lines = editor_->toPlainText().split('\n');
        QCOMPARE(lines.value(0), QString("- parent") + spaces(tabSize));
        QCOMPARE(lines.value(1), QString("  child"));
        QCOMPARE(lines.value(2), QString("- peer"));
    }

    void testEditorUsesFixedPitchFont()
    {
        QFontInfo info(editor_->font());
        QVERIFY(info.fixedPitch());
    }

    void testEscapeBackslashPreventsParenAutopair()
    {
        editor_->clear();

        QTest::keyClicks(editor_, "\\");
        QTest::keyClicks(editor_, "(");

        QCOMPARE(editor_->toPlainText(), QString("\\("));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testEscapeBackslashPreventsAngleAutopair()
    {
        editor_->clear();

        QTest::keyClicks(editor_, "\\");
        QTest::keyClicks(editor_, "<");

        QCOMPARE(editor_->toPlainText(), QString("\\<"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testEscapeBackslashPreventsSkipOverExistingCloser()
    {
        editor_->setPlainText("\\)");

        QTextCursor cursor = editor_->textCursor();
        cursor.setPosition(1);
        editor_->setTextCursor(cursor);

        QTest::keyClicks(editor_, ")");

        QCOMPARE(editor_->toPlainText(), QString("\\))"));
        QCOMPARE(editor_->textCursor().positionInBlock(), 2);
    }

    void testWordCountCountsCjkCharactersIndividually()
    {
        editor_->setPlainText(QStringLiteral("你好世界"));
        QTest::qWait(220);
        QCOMPARE(editor_->wordCount(), 4);
        QCOMPARE(editor_->charCount(), 4);

        editor_->setPlainText(QStringLiteral("你好 world"));
        QTest::qWait(220);
        QCOMPARE(editor_->wordCount(), 3);
    }

    void testWhiteThemeKeepsImePreeditColorsReadable()
    {
        editor_->setThemeName("white");

        const QPalette pal = editor_->palette();
        const QColor base = pal.color(QPalette::Base);
        const QColor text = pal.color(QPalette::Text);

        QCOMPARE(pal.color(QPalette::BrightText), text);
        QCOMPARE(pal.color(QPalette::WindowText), text);
        QCOMPARE(pal.color(QPalette::ButtonText), text);
        QVERIFY(text != base);
    }

private:
    MdEditor *editor_ = nullptr;
};

QTEST_MAIN(TestMdEditor)
#include "test_md_editor.moc"

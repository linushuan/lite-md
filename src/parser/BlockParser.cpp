#include "BlockParser.h"
#include <QRegularExpression>

BlockType BlockParser::classify(const QString &text, ContextStack &ctx, QVector<BlockToken> &tokens)
{
    tokens.clear();

    // 1. Continue CodeFence
    if (ctx.inCode()) {
        if (matchCodeFenceEnd(text, ctx.top().fenceChar, ctx.top().fenceLen)) {
            tokens.append({0, text.length(), TokenType::CodeFenceMark});
            ctx.pop();
            return BlockType::CodeFenceEnd;
        }
        tokens.append({0, text.length(), TokenType::CodeFenceBody});
        return BlockType::CodeFenceBody;
    }

    // 2. Continue LatexDisplay
    if (ctx.topState() == BlockState::LatexDisplay) {
        if (text.trimmed() == "$$") {
            tokens.append({0, text.length(), TokenType::LatexDelimiter});
            ctx.pop();
            return BlockType::LatexDisplayEnd;
        }
        tokens.append({0, text.length(), TokenType::LatexMathBody});
        return BlockType::LatexDisplayBody;
    }

    // 3. Continue LatexEnv
    if (ctx.topState() == BlockState::LatexEnv) {
        QString endPattern = "\\end{" + ctx.top().envName + "}";
        if (text.contains(endPattern)) {
            tokens.append({0, text.length(), TokenType::LatexBeginEnd});
            ctx.pop();
            return BlockType::LatexEnvEnd;
        }
        tokens.append({0, text.length(), TokenType::LatexMathBody});
        return BlockType::LatexEnvBody;
    }

    // 4. ATX Heading
    int level, contentStart, contentEnd;
    if (matchATXHeading(text, level, contentStart, contentEnd)) {
        // # markers
        tokens.append({0, contentStart, TokenType::HeadingMarker});
        // Heading content
        TokenType headingType = static_cast<TokenType>(
            static_cast<int>(TokenType::HeadingH1) + level - 1);
        tokens.append({contentStart, contentEnd - contentStart, headingType});
        return BlockType::Heading;
    }

    // 5. CodeFence start
    QChar fenceChar;
    int fenceLen, indent;
    QString lang;
    if (matchCodeFenceStart(text, fenceChar, fenceLen, indent, lang)) {
        tokens.append({0, text.length(), TokenType::CodeFenceMark});
        if (!lang.isEmpty()) {
            // Find lang position and mark it
            int langStart = text.indexOf(lang);
            tokens.append({langStart, lang.length(), TokenType::CodeFenceLang});
        }
        ContextFrame frame;
        frame.state = BlockState::CodeFence;
        frame.fenceChar = fenceChar;
        frame.fenceLen = fenceLen;
        ctx.push(frame);
        return BlockType::CodeFenceStart;
    }

    // 6. $$ display LaTeX start
    if (text.trimmed() == "$$") {
        tokens.append({0, text.length(), TokenType::LatexDelimiter});
        ContextFrame frame;
        frame.state = BlockState::LatexDisplay;
        ctx.push(frame);
        return BlockType::LatexDisplayStart;
    }

    // 7. \begin{env} LaTeX env start
    static QRegularExpression beginRe(R"(\\begin\{(\w+)\})");
    auto beginMatch = beginRe.match(text);
    if (beginMatch.hasMatch()) {
        tokens.append({0, text.length(), TokenType::LatexBeginEnd});
        ContextFrame frame;
        frame.state = BlockState::LatexEnv;
        frame.envName = beginMatch.captured(1);
        ctx.push(frame);
        return BlockType::LatexEnvStart;
    }

    // 8. Table
    if (matchTable(text)) {
        // Tokenize pipe characters
        for (int i = 0; i < text.length(); ++i) {
            if (text[i] == '|') {
                tokens.append({i, 1, TokenType::TablePipe});
            }
        }
        return BlockType::Table;
    }

    // 9. Blockquote
    int contentOffset;
    if (matchBlockquote(text, contentOffset)) {
        tokens.append({0, contentOffset, TokenType::BlockquoteMark});
        tokens.append({contentOffset, text.length() - contentOffset, TokenType::BlockquoteBody});
        return BlockType::Blockquote;
    }

    // 10. Ordered list
    int listIndent;
    if (matchOrderedList(text, listIndent, contentOffset)) {
        tokens.append({0, contentOffset, TokenType::ListBullet});
        tokens.append({contentOffset, text.length() - contentOffset, TokenType::ListBody});
        return BlockType::ListItem;
    }

    // 11. Unordered list
    if (matchUnorderedList(text, listIndent, contentOffset)) {
        tokens.append({0, contentOffset, TokenType::ListBullet});
        tokens.append({contentOffset, text.length() - contentOffset, TokenType::ListBody});
        return BlockType::ListItem;
    }

    // 12. Horizontal rule
    if (matchHR(text)) {
        tokens.append({0, text.length(), TokenType::HR});
        return BlockType::HR;
    }

    // 13. Blank line
    if (text.trimmed().isEmpty()) {
        return BlockType::BlankLine;
    }

    // 14. Normal paragraph
    return BlockType::Normal;
}

bool BlockParser::isSetextH1Underline(const QString &nextLine)
{
    static QRegularExpression re(R"(^={3,}\s*$)");
    return re.match(nextLine).hasMatch();
}

bool BlockParser::isSetextH2Underline(const QString &nextLine)
{
    static QRegularExpression re(R"(^-{3,}\s*$)");
    return re.match(nextLine).hasMatch();
}

bool BlockParser::matchCodeFenceStart(const QString &text, QChar &fenceChar, int &fenceLen, int &indent, QString &lang)
{
    static QRegularExpression re(R"(^( {0,3})(`{3,}|~{3,})(\w*)\s*$)");
    auto m = re.match(text);
    if (!m.hasMatch()) return false;

    indent = m.captured(1).length();
    QString fence = m.captured(2);
    fenceChar = fence[0];
    fenceLen = fence.length();
    lang = m.captured(3);
    return true;
}

bool BlockParser::matchCodeFenceEnd(const QString &text, QChar fenceChar, int fenceLen)
{
    static QRegularExpression reBacktick(R"(^ {0,3}`{3,}\s*$)");
    static QRegularExpression reTilde(R"(^ {0,3}~{3,}\s*$)");

    const auto &re = (fenceChar == '`') ? reBacktick : reTilde;
    auto m = re.match(text);
    if (!m.hasMatch()) return false;

    // Count fence chars
    int count = 0;
    for (QChar c : text.trimmed()) {
        if (c == fenceChar) count++;
        else break;
    }
    return count >= fenceLen;
}

bool BlockParser::matchATXHeading(const QString &text, int &level, int &contentStart, int &contentEnd)
{
    static QRegularExpression re(R"(^ {0,3}(#{1,6})(\s+|$))");
    auto m = re.match(text);
    if (!m.hasMatch()) return false;

    level = m.captured(1).length();
    contentStart = m.capturedEnd(0);
    contentEnd = text.length();
    return true;
}

bool BlockParser::matchTable(const QString &text)
{
    return text.contains('|');
}

bool BlockParser::matchBlockquote(const QString &text, int &contentOffset)
{
    static QRegularExpression re(R"(^ {0,3}> ?)");
    auto m = re.match(text);
    if (!m.hasMatch()) return false;
    contentOffset = m.capturedEnd(0);
    return true;
}

bool BlockParser::matchOrderedList(const QString &text, int &indent, int &contentOffset)
{
    static QRegularExpression re(R"(^( {0,3})(\d{1,9})[.)](\s|$))");
    auto m = re.match(text);
    if (!m.hasMatch()) return false;
    indent = m.captured(1).length();
    contentOffset = m.capturedEnd(0);
    return true;
}

bool BlockParser::matchUnorderedList(const QString &text, int &indent, int &contentOffset)
{
    static QRegularExpression re(R"(^( {0,3})([-*+])(\s|$))");
    auto m = re.match(text);
    if (!m.hasMatch()) return false;

    // Make sure it's not an HR (e.g., "---" or "***")
    QString trimmed = text.trimmed();
    if (trimmed.length() >= 3) {
        bool allSame = true;
        QChar c = trimmed[0];
        for (int i = 1; i < trimmed.length(); ++i) {
            if (trimmed[i] != c && !trimmed[i].isSpace()) {
                allSame = false;
                break;
            }
        }
        if (allSame && (c == '-' || c == '*' || c == '_'))
            return false; // This is an HR, not a list
    }

    indent = m.captured(1).length();
    contentOffset = m.capturedEnd(0);
    return true;
}

bool BlockParser::matchHR(const QString &text)
{
    static QRegularExpression re(R"(^ {0,3}([-*_])(\s*\1){2,}\s*$)");
    return re.match(text).hasMatch();
}

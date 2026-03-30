#include "MdHighlighter.h"
#include "parser/BlockParser.h"
#include "parser/InlineParser.h"
#include "parser/LatexParser.h"

#include <QTextBlock>
#include <QTextBlockUserData>

// Store ContextStack in QTextBlockUserData
class ContextBlockData : public QTextBlockUserData {
public:
    ContextBlockData(const ContextStack &ctx) : ctx_(ctx) {}
    ContextStack ctx_;
};

MdHighlighter::MdHighlighter(QTextDocument *parent, const Theme &theme)
    : QSyntaxHighlighter(parent)
    , theme_(theme)
{
    buildFormats();
}

void MdHighlighter::setTheme(const Theme &theme)
{
    theme_ = theme;
    buildFormats();
    rehighlight();
}

void MdHighlighter::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

void MdHighlighter::highlightBlock(const QString &text)
{
    if (!enabled_) return;

    // 1. Restore context from previous block
    ContextStack ctx = restoreContext();

    // Check for setext heading (lookahead)
    bool isSetextH1 = false, isSetextH2 = false;
    if (ctx.topState() == BlockState::Normal) {
        QTextBlock nextBlock = currentBlock().next();
        if (nextBlock.isValid()) {
            QString nextLine = nextBlock.text();
            isSetextH1 = BlockParser::isSetextH1Underline(nextLine);
            isSetextH2 = BlockParser::isSetextH2Underline(nextLine);
        }
    }

    // 2. Classify the block
    QVector<BlockToken> blockTokens;
    BlockType blockType = BlockParser::classify(text, ctx, blockTokens);

    // Override for setext heading
    if (isSetextH1 && blockType == BlockType::Normal) {
        blockTokens.clear();
        blockTokens.append({0, text.length(), TokenType::HeadingH1});
        blockType = BlockType::Heading;
    } else if (isSetextH2 && blockType == BlockType::Normal) {
        blockTokens.clear();
        blockTokens.append({0, text.length(), TokenType::HeadingH2});
        blockType = BlockType::Heading;
    }
    // Mark setext underline itself
    if (BlockParser::isSetextH1Underline(text) || BlockParser::isSetextH2Underline(text)) {
        if (currentBlock().previous().isValid()) {
            blockTokens.clear();
            blockTokens.append({0, text.length(), TokenType::SetextMarker});
        }
    }

    // 3. Apply block token formats
    for (const auto &token : blockTokens) {
        if (formats_.contains(token.type)) {
            setFormat(token.start, token.length, formats_[token.type]);
        }
    }

    // 4. Run inline parser if not in code fence
    if (blockType != BlockType::CodeFenceBody &&
        blockType != BlockType::CodeFenceStart &&
        blockType != BlockType::CodeFenceEnd) {

        int contentOffset = 0;
        // Adjust offset for blockquote/list prefix
        for (const auto &token : blockTokens) {
            if (token.type == TokenType::BlockquoteMark ||
                token.type == TokenType::ListBullet) {
                contentOffset = token.start + token.length;
            }
        }

        if (blockType == BlockType::LatexDisplayBody ||
            blockType == BlockType::LatexEnvBody) {
            // Only run LaTeX parser for math bodies
            QVector<InlineToken> latexTokens;
            LatexParser::parseLatexBody(text, contentOffset, text.length() - contentOffset, latexTokens);
            for (const auto &token : latexTokens) {
                if (formats_.contains(token.type))
                    setFormat(token.start, token.length, formats_[token.type]);
            }
        } else if (blockType != BlockType::LatexDisplayStart &&
                   blockType != BlockType::LatexDisplayEnd &&
                   blockType != BlockType::LatexEnvStart &&
                   blockType != BlockType::LatexEnvEnd) {
            // Run full inline parser
            QVector<InlineToken> inlineTokens;
            InlineParser::parse(text, contentOffset, ctx, inlineTokens);
            for (const auto &token : inlineTokens) {
                if (formats_.contains(token.type))
                    setFormat(token.start, token.length, formats_[token.type]);
            }
        }
    }

    // 5. Save context
    saveContext(ctx);
    setCurrentBlockState(static_cast<int>(ctx.topState()));
}

ContextStack MdHighlighter::restoreContext() const
{
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid()) {
        auto *data = dynamic_cast<ContextBlockData *>(prevBlock.userData());
        if (data) return data->ctx_;
    }
    return ContextStack();
}

void MdHighlighter::saveContext(const ContextStack &ctx)
{
    setCurrentBlockUserData(new ContextBlockData(ctx));
}

void MdHighlighter::buildFormats()
{
    formats_.clear();

    auto makeFormat = [](const QColor &fg, bool bold = false, bool italic = false, const QColor &bg = QColor()) {
        QTextCharFormat fmt;
        fmt.setForeground(fg);
        if (bold) fmt.setFontWeight(QFont::Bold);
        if (italic) fmt.setFontItalic(true);
        if (bg.isValid()) fmt.setBackground(bg);
        return fmt;
    };

    // Headings — with font size scaling
    int baseSize = 14;
    auto headingFmt = [&](const QColor &color, double scale) {
        QTextCharFormat fmt;
        fmt.setForeground(color);
        fmt.setFontWeight(QFont::Bold);
        fmt.setFontPointSize(baseSize * scale);
        return fmt;
    };

    formats_[TokenType::HeadingH1] = headingFmt(theme_.heading[0], 1.5);
    formats_[TokenType::HeadingH2] = headingFmt(theme_.heading[1], 1.3);
    formats_[TokenType::HeadingH3] = headingFmt(theme_.heading[2], 1.15);
    formats_[TokenType::HeadingH4] = headingFmt(theme_.heading[3], 1.05);
    formats_[TokenType::HeadingH5] = headingFmt(theme_.heading[4], 1.0);
    QTextCharFormat h6 = headingFmt(theme_.heading[5], 1.0);
    h6.setFontItalic(true);
    formats_[TokenType::HeadingH6] = h6;

    formats_[TokenType::HeadingMarker] = makeFormat(theme_.markerFg);
    formats_[TokenType::SetextMarker]  = makeFormat(theme_.markerFg);

    // Code
    formats_[TokenType::CodeFenceMark] = makeFormat(theme_.codeFenceFg);
    formats_[TokenType::CodeFenceLang] = makeFormat(theme_.codeFenceLangFg);
    formats_[TokenType::CodeFenceBody] = makeFormat(theme_.codeFenceFg, false, false, theme_.codeFenceBg);
    formats_[TokenType::InlineCode]    = makeFormat(theme_.codeInlineFg, false, false, theme_.codeInlineBg);
    formats_[TokenType::InlineCodeMark] = makeFormat(theme_.markerFg);

    // Blockquote
    formats_[TokenType::BlockquoteMark] = makeFormat(theme_.blockquoteBorderFg);
    formats_[TokenType::BlockquoteBody] = makeFormat(theme_.blockquoteFg);

    // List
    formats_[TokenType::ListBullet] = makeFormat(theme_.listBulletFg, true);
    formats_[TokenType::ListBody]   = makeFormat(theme_.foreground);

    // Table
    formats_[TokenType::TablePipe]      = makeFormat(theme_.tablePipeFg);
    formats_[TokenType::TableHeader]    = makeFormat(theme_.foreground, true);
    formats_[TokenType::TableSeparator] = makeFormat(theme_.tablePipeFg);
    formats_[TokenType::TableCell]      = makeFormat(theme_.foreground);

    // HR
    formats_[TokenType::HR] = makeFormat(theme_.hrFg);

    // Inline
    formats_[TokenType::Bold]          = makeFormat(theme_.boldFg, true);
    formats_[TokenType::BoldMarker]    = makeFormat(theme_.markerFg, true);
    formats_[TokenType::Italic]        = makeFormat(theme_.italicFg, false, true);
    formats_[TokenType::ItalicMarker]  = makeFormat(theme_.markerFg, false, true);
    formats_[TokenType::BoldItalic]    = makeFormat(theme_.boldFg, true, true);
    formats_[TokenType::BoldItalicMarker] = makeFormat(theme_.markerFg, true, true);
    formats_[TokenType::Strikethrough] = makeFormat(theme_.strikeFg);
    formats_[TokenType::StrikeMarker]  = makeFormat(theme_.markerFg);

    formats_[TokenType::LinkText]    = makeFormat(theme_.linkTextFg);
    formats_[TokenType::LinkUrl]     = makeFormat(theme_.linkUrlFg);
    formats_[TokenType::LinkBracket] = makeFormat(theme_.markerFg);
    formats_[TokenType::ImageAlt]    = makeFormat(theme_.imageFg);
    formats_[TokenType::ImageUrl]    = makeFormat(theme_.linkUrlFg);
    formats_[TokenType::ImageBracket] = makeFormat(theme_.markerFg);

    formats_[TokenType::HardBreakSpace]     = makeFormat(theme_.hardBreakFg);
    formats_[TokenType::HardBreakBackslash] = makeFormat(theme_.hardBreakFg);
    formats_[TokenType::Escape] = makeFormat(theme_.markerFg);

    // LaTeX
    formats_[TokenType::LatexDelimiter] = makeFormat(theme_.latexDelimiterFg);
    formats_[TokenType::LatexCommand]   = makeFormat(theme_.latexCommandFg);
    formats_[TokenType::LatexBrace]     = makeFormat(theme_.latexBraceFg);
    formats_[TokenType::LatexMathBody]  = makeFormat(theme_.latexMathBodyFg);
    formats_[TokenType::LatexEnvName]   = makeFormat(theme_.latexEnvNameFg);
    formats_[TokenType::LatexBeginEnd]  = makeFormat(theme_.latexCommandFg);

    // Meta
    formats_[TokenType::Marker] = makeFormat(theme_.markerFg);
}

// Unused methods — the block-specific highlight methods are handled via
// the generic token-based approach in highlightBlock().
void MdHighlighter::highlightHeading(const QString &, int) {}
void MdHighlighter::highlightCodeFence(const QString &, ContextStack &) {}
void MdHighlighter::highlightInline(const QString &, int, const ContextStack &) {}
void MdHighlighter::highlightLatex(const QString &, int, ContextStack &) {}
void MdHighlighter::highlightTable(const QString &, const ContextStack &) {}
void MdHighlighter::highlightBlockquote(const QString &, ContextStack &) {}
void MdHighlighter::highlightListItem(const QString &, const ContextStack &) {}

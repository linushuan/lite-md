#include "Theme.h"
#include "util/TomlParser.h"

Theme Theme::darkDefault()
{
    Theme t;
    t.name = "dark";

    t.background      = QColor("#1e1e2e");
    t.foreground      = QColor("#cdd6f4");
    t.currentLineBg   = QColor("#2a2a3e");
    t.lineNumberFg    = QColor("#585b70");
    t.lineNumberBg    = QColor("#1e1e2e");
    t.selectionBg     = QColor("#45475a");
    t.selectionFg     = QColor("#cdd6f4");
    t.cursorColor     = QColor("#f5c2e7");
    t.searchHighlightBg = QColor("#f9e2af44");

    t.heading[0] = QColor("#cba6f7");
    t.heading[1] = QColor("#89b4fa");
    t.heading[2] = QColor("#74c7ec");
    t.heading[3] = QColor("#a6e3a1");
    t.heading[4] = QColor("#f9e2af");
    t.heading[5] = QColor("#fab387");

    t.codeInlineFg    = QColor("#f38ba8");
    t.codeInlineBg    = QColor("#2a1a1a");
    t.codeFenceFg     = QColor("#cdd6f4");
    t.codeFenceBg     = QColor("#181825");
    t.codeFenceLangFg = QColor("#a6e3a1");

    t.blockquoteFg       = QColor("#6c7086");
    t.blockquoteBorderFg = QColor("#585b70");
    t.listBulletFg       = QColor("#89b4fa");
    t.linkTextFg         = QColor("#89b4fa");
    t.linkUrlFg          = QColor("#585b70");
    t.imageFg            = QColor("#a6e3a1");
    t.boldFg             = QColor("#cdd6f4");
    t.italicFg           = QColor("#cdd6f4");
    t.strikeFg           = QColor("#585b70");
    t.tablePipeFg        = QColor("#45475a");
    t.hrFg               = QColor("#45475a");
    t.markerFg           = QColor("#45475a");
    t.hardBreakFg        = QColor("#6c7086");

    t.latexDelimiterFg = QColor("#94e2d5");
    t.latexCommandFg   = QColor("#89dceb");
    t.latexBraceFg     = QColor("#74c7ec");
    t.latexMathBodyFg  = QColor("#cdd6f4");
    t.latexEnvNameFg   = QColor("#94e2d5");

    return t;
}

Theme Theme::lightDefault()
{
    Theme t;
    t.name = "light";

    t.background      = QColor("#eff1f5");
    t.foreground      = QColor("#4c4f69");
    t.currentLineBg   = QColor("#e6e9f0");
    t.lineNumberFg    = QColor("#acaec4");
    t.lineNumberBg    = QColor("#eff1f5");
    t.selectionBg     = QColor("#acaec4");
    t.selectionFg     = QColor("#4c4f69");
    t.cursorColor     = QColor("#8839ef");
    t.searchHighlightBg = QColor("#df8e1d44");

    t.heading[0] = QColor("#8839ef");
    t.heading[1] = QColor("#1e66f5");
    t.heading[2] = QColor("#04a5e5");
    t.heading[3] = QColor("#40a02b");
    t.heading[4] = QColor("#df8e1d");
    t.heading[5] = QColor("#fe640b");

    t.codeInlineFg    = QColor("#d20f39");
    t.codeInlineBg    = QColor("#f0e0e0");
    t.codeFenceFg     = QColor("#4c4f69");
    t.codeFenceBg     = QColor("#e6e9f0");
    t.codeFenceLangFg = QColor("#40a02b");

    t.blockquoteFg       = QColor("#9ca0b0");
    t.blockquoteBorderFg = QColor("#acaec4");
    t.listBulletFg       = QColor("#1e66f5");
    t.linkTextFg         = QColor("#1e66f5");
    t.linkUrlFg          = QColor("#acaec4");
    t.imageFg            = QColor("#40a02b");
    t.boldFg             = QColor("#4c4f69");
    t.italicFg           = QColor("#4c4f69");
    t.strikeFg           = QColor("#acaec4");
    t.tablePipeFg        = QColor("#ccd0da");
    t.hrFg               = QColor("#ccd0da");
    t.markerFg           = QColor("#ccd0da");
    t.hardBreakFg        = QColor("#9ca0b0");

    t.latexDelimiterFg = QColor("#179299");
    t.latexCommandFg   = QColor("#04a5e5");
    t.latexBraceFg     = QColor("#209fb5");
    t.latexMathBodyFg  = QColor("#4c4f69");
    t.latexEnvNameFg   = QColor("#179299");

    return t;
}

Theme Theme::fromToml(const QString &path)
{
    Theme fallback = darkDefault();

    TomlParser toml(path);
    if (!toml.isValid()) return fallback;

    Theme t;
    t.name = toml.getString("", "name", fallback.name);

    auto color = [&](const QString &section, const QString &key, const QColor &def) -> QColor {
        QString val = toml.getString(section, key);
        if (val.isEmpty()) return def;
        QColor c(val);
        return c.isValid() ? c : def;
    };

    t.background      = color("", "background", fallback.background);
    t.foreground      = color("", "foreground", fallback.foreground);
    t.currentLineBg   = color("", "currentLineBg", fallback.currentLineBg);
    t.lineNumberFg    = color("", "lineNumberFg", fallback.lineNumberFg);
    t.lineNumberBg    = color("", "lineNumberBg", fallback.lineNumberBg);
    t.selectionBg     = color("", "selectionBg", fallback.selectionBg);
    t.cursorColor     = color("", "cursorColor", fallback.cursorColor);
    t.searchHighlightBg = color("", "searchHighlightBg", fallback.searchHighlightBg);

    t.heading[0] = color("heading", "h1", fallback.heading[0]);
    t.heading[1] = color("heading", "h2", fallback.heading[1]);
    t.heading[2] = color("heading", "h3", fallback.heading[2]);
    t.heading[3] = color("heading", "h4", fallback.heading[3]);
    t.heading[4] = color("heading", "h5", fallback.heading[4]);
    t.heading[5] = color("heading", "h6", fallback.heading[5]);

    t.codeInlineFg    = color("code", "inlineFg", fallback.codeInlineFg);
    t.codeInlineBg    = color("code", "inlineBg", fallback.codeInlineBg);
    t.codeFenceFg     = color("code", "fenceFg", fallback.codeFenceFg);
    t.codeFenceBg     = color("code", "fenceBg", fallback.codeFenceBg);
    t.codeFenceLangFg = color("code", "langFg", fallback.codeFenceLangFg);

    t.blockquoteFg       = color("block", "blockquoteFg", fallback.blockquoteFg);
    t.blockquoteBorderFg = color("block", "blockquoteBorder", fallback.blockquoteBorderFg);
    t.listBulletFg       = color("block", "listBullet", fallback.listBulletFg);
    t.linkTextFg         = color("block", "linkText", fallback.linkTextFg);
    t.linkUrlFg          = color("block", "linkUrl", fallback.linkUrlFg);
    t.imageFg            = color("block", "image", fallback.imageFg);
    t.boldFg             = color("block", "bold", fallback.boldFg);
    t.italicFg           = color("block", "italic", fallback.italicFg);
    t.strikeFg           = color("block", "strike", fallback.strikeFg);
    t.tablePipeFg        = color("block", "tablePipe", fallback.tablePipeFg);
    t.hrFg               = color("block", "hr", fallback.hrFg);
    t.markerFg           = color("block", "marker", fallback.markerFg);
    t.hardBreakFg        = color("block", "hardBreak", fallback.hardBreakFg);

    t.latexDelimiterFg = color("latex", "delimiter", fallback.latexDelimiterFg);
    t.latexCommandFg   = color("latex", "command", fallback.latexCommandFg);
    t.latexBraceFg     = color("latex", "brace", fallback.latexBraceFg);
    t.latexMathBodyFg  = color("latex", "mathBody", fallback.latexMathBodyFg);
    t.latexEnvNameFg   = color("latex", "envName", fallback.latexEnvNameFg);

    return t;
}

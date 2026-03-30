#pragma once

#include <QChar>

namespace CjkUtil {

// Returns true if this character can serve as an inline token boundary
bool isBoundary(QChar c);

// Returns true if this is a CJK character
bool isCjk(QChar c);

// Returns true if this is a blank line (including fullwidth spaces)
bool isBlankLine(const QString &text);

} // namespace CjkUtil

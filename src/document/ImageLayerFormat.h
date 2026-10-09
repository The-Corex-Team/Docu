#pragma once

#include <QTextFormat>

namespace ImageLayerFormat {

inline constexpr int IsLayeredImage = QTextFormat::UserProperty + 1;
inline constexpr int SourceWidth = QTextFormat::UserProperty + 2;
inline constexpr int SourceHeight = QTextFormat::UserProperty + 3;
inline constexpr int PositionX = QTextFormat::UserProperty + 4;
inline constexpr int PositionY = QTextFormat::UserProperty + 5;

} // namespace ImageLayerFormat

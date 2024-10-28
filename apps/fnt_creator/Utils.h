#pragma once

#include "Common.h"

std::string stringFormat(const char* format, ...);

int nextPOT(int x);

int getMinWidth(const FntPage& page, const GenerateConfig& config);

int calculateHeight(const FntPage& page, const GenerateConfig& config, int maxWidth);

std::vector<char32_t> collectCodepoints(const PageConfig& config);

// 获取字符的度量信息
GlyphInfo getGlyphInfo(SkFont font, char32_t codepoint);

SkFontStyle getFontStyle(bool isBold, bool isItalic);

SkColor stringToSkColor(const std::string hex);

std::string rgbaToHex(int r, int g, int b, int a);

SkPaint createPaint(const std::string& color, const std::string& blend_mode);

void setPaintShader(SkPaint& paint, const TextEffect& config, SkScalar x, SkScalar y, SkScalar w, SkScalar h);

std::vector<SkPaint> createShadowPaints(const std::vector<TextShadow>& configs);
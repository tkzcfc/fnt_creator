#pragma once

#include <include/core/SkCanvas.h>
#include <include/core/SkBitmap.h>
#include <include/core/SkPaint.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkFont.h>
#include <include/codec/SkCodec.h>
#include <include/core/SkStream.h>
#include <include/core/SkFontMetrics.h>
#include <include/encode/SkPngEncoder.h>
#include <include/core/SkFontMgr.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include "ajson.hpp"

struct PageConfig
{
    std::string text;
    std::vector<uint32_t> chars;
    std::vector<std::string> fonts;
};
AJSON(PageConfig, text, chars, fonts);

struct Position
{
    float x;
    float y;
};
AJSON(Position, x, y);

// ���Խ������
struct LinearGradient
{
    // ��ʼλ��
    Position begin;
    // ����λ��
    Position end;
    // ��ɫ������������
    std::vector<std::string> colors;
    // ��ɫ�ֲ�������Ĭ�����Էֲ���
    std::vector<float> pos;
};
AJSON(LinearGradient, begin, end, colors, pos);

struct TextEffect
{
    // ��Ч����
    // linear_gradient ���Խ���
    std::string effect_type;

    // ���Խ������
    LinearGradient linear_gradient;
};
AJSON(TextEffect, effect_type, linear_gradient);

struct TextShadow
{
    int offsetx;
    int offsety;
    float blur_radius;
    std::string color;
    std::string blend_mode;
    TextEffect effect;
};
AJSON(TextShadow, offsetx, offsety, blur_radius, color, blend_mode, effect);

struct TextStyle
{
    // ���ִ�С
    int font_size;
    // ������ɫ
    std::string color;
    // ���ֻ��ģʽ
    std::string blend_mode;

    // ������Ӱ
    std::vector<TextShadow> shadows;
    // ������Ч
    TextEffect effect;
    // ������ɫ
    std::string background_color;
    // �Ƿ�Ӵ�
    bool is_bold;
    // �Ƿ���б
    bool is_italic;
    // ��߿���
    int outline_thickness;
    // ��߻���ʱ����
    float outline_thickness_render_scale;
    // �����ɫ
    std::string outline_color;
    // ��߻��ģʽ
    std::string outline_blend_mode;
    // �����Ч
    TextEffect outline_effect;
    // �����Ӱ
    std::vector<TextShadow> outline_shadows;
};
AJSON(TextStyle, 
    font_size,
    color,
    blend_mode,
    shadows,
    effect,
    background_color,
    is_bold,
    is_italic,
    outline_thickness,
    outline_thickness_render_scale,
    outline_color,
    outline_blend_mode,
    outline_effect,
    outline_shadows
);

struct GenerateConfig
{
    // ʹ��gpu��Ⱦ
    bool use_gpu;
    // ����ļ�
    std::string output_file;
    // �־�
    int spacing_horiz;
    // �о�
    int spacing_vert;

    // ����ͼƬʱ���������뵥������֮��X��ļ��
    int spacing_glyph_x;
    // ����ͼƬʱ���������뵥������֮��Y��ļ��
    int spacing_glyph_y;

    // �������� xadvance ���Ӿ���
    int glyph_padding_xadvance;

    // �������־��붥�����
    int glyph_padding_up;
    // �������־���ײ����
    int glyph_padding_down;
    // �������־�����߼��
    int glyph_padding_left;
    // �������־����ұ߼��
    int glyph_padding_right;

    // ���ɵ�ͼƬ���붥�����
    int padding_up;
    // ���ɵ�ͼƬ����ײ����
    int padding_down;
    // ���ɵ�ͼƬ������߼��
    int padding_left;
    // ���ɵ�ͼƬ�����ұ߼��
    int padding_right;
    // ����ͼƬ�Ŀ����Ƿ���2��n�η�
    bool is_NPOT;
    // �Ƿ���ȫ��������
    bool is_fully_wrapped_mode;
    // ���ͼƬ������
    int max_width;

    // �Ƿ����debug��Ϣ
    bool is_draw_debug;

    // ������ʽ
    TextStyle text_style;

    std::vector<PageConfig> pages;
};
AJSON(GenerateConfig,
    use_gpu,
    output_file,
    spacing_horiz,
    spacing_vert,
    spacing_glyph_x,
    spacing_glyph_y,
    glyph_padding_xadvance,
    glyph_padding_up,
    glyph_padding_down,
    glyph_padding_left,
    glyph_padding_right,
    padding_up,
    padding_right,
    padding_down,
    padding_left,
    is_NPOT,
    is_fully_wrapped_mode,
    max_width,
    is_draw_debug,
    text_style,
    pages
)

struct GlyphInfo
{
    char32_t codepoint;
    SkFont font;

    int x;
    int y;
    // �ַ�ʵ��ռ�ÿ���
    int width;
    // �ַ�ʵ��ռ�ø߶�
    int height;
    // �����ַ�ʱ��ƫ����
    int xoffset;
    int yoffset;
    // ����ڻ�������ַ�����Ҫ�ƶ��ľ��롣
    int xadvance;
    // �ַ����ڵ�����ҳ���ţ����ڶ�ҳ��֧�֣�
    int page;
    // ָ����ɫͨ�������� RGBA ��ʽ�е� 15 ��������ͨ����
    int chnl;
};

struct FntPage
{
    int width;
    int height;
    std::vector<GlyphInfo> glyphs;
    std::string fileName;
};

struct FntInfo
{
    int fontSize;
    bool isBold;
    bool isItalic;
    bool useUnicode;
    int scaleH;
    bool useSmoothing;
    int aa;
    int paddingUp;
    int paddingRight;
    int paddingDown;
    int paddingLeft;
    int spacingHoriz;
    int spacingVert;
    int outlineThickness;

    int commonLineHeight;
    int base;

    std::vector<FntPage> pages;
};

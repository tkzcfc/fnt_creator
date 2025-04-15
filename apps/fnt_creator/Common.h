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
    PageConfig()
    {
        fixed_width_alignment = false;
        text = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!№;%:?*()_+-=.,/|\"'@#$^&{}[] ";
    }
	// 是否固定宽度对齐(按最大字符宽度)
    bool fixed_width_alignment;
    std::string text;
    std::vector<uint32_t> chars;
    std::vector<std::string> fonts;
};
AJSON(PageConfig, fixed_width_alignment, text, chars, fonts);

struct Position
{
    Position()
    {
        x = y = 0.0f;
    }
    Position(float _x, float _y)
    {
        x = _x;
        y = _y;
    }
    float x;
    float y;
};
AJSON(Position, x, y);

// 线性渐变参数
struct LinearGradient
{
    LinearGradient()
    {
        begin = Position(0.5f, 1.0f);
        end = Position(0.5f, 0.0f);
    }
    // 起始位置
    Position begin;
    // 结束位置
    Position end;
    // 颜色（至少两个）
    std::vector<std::string> colors;
    // 颜色分布比例（默认线性分布）
    std::vector<float> pos;
};
AJSON(LinearGradient, begin, end, colors, pos);

struct TextEffect
{
    TextEffect()
    {
        effect_type = "none";
    }
    // 特效类型
    // linear_gradient 线性渐变
    std::string effect_type;

    // 线性渐变参数
    LinearGradient linear_gradient;
};
AJSON(TextEffect, effect_type, linear_gradient);

struct TextShadow
{
    TextShadow()
    {
        offsetx = 0;
        offsety = 0;
        blur_radius = 0.0f;
        color = "#ffffffff";
    }
    int offsetx;
    int offsety;
    float blur_radius;
    std::string color;
    std::string blend_mode;
    //TextEffect effect;  // 阴影暂时不支持effect
};
AJSON(TextShadow, offsetx, offsety, blur_radius, color, blend_mode);

struct TextStyle
{
    TextStyle()
    {
        font_size = 30;
        color = "#000000ff";
        is_bold = false;
        is_italic = false;
        outline_thickness = 0;
        outline_thickness_render_scale = 2.0f;
        outline_color = "#ffffffff";
    }

    // 文字大小
    int font_size;
    // 文字颜色
    std::string color;
    // 文字混合模式
    std::string blend_mode;

    // 文字阴影
    std::vector<TextShadow> shadows;
    // 文字特效
    TextEffect effect;
    // 背景颜色
    std::string background_color;
    // 是否加粗
    bool is_bold;
    // 是否倾斜
    bool is_italic;
    // 描边宽度
    int outline_thickness;
    // 描边绘制时缩放
    float outline_thickness_render_scale;
    // 描边颜色
    std::string outline_color;
    // 描边混合模式
    std::string outline_blend_mode;
    // 描边特效
    TextEffect outline_effect;
    // 描边阴影
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
    GenerateConfig()
    {
        use_gpu = true;
        spacing_horiz = 1;
        spacing_vert = 1;
        spacing_glyph_x = 1;
        spacing_glyph_y = 1;
        line_height_padding_adcance = 0;
        glyph_padding_xadvance = 0;
        glyph_padding_yadvance = 0;
        glyph_padding_up = 0;
        glyph_padding_down = 0;
        glyph_padding_left = 0;
        glyph_padding_right = 0;
        padding_up = 0;
        padding_down = 0;
        padding_left = 0;
        padding_right = 0;
        is_NPOT = true;
        is_fully_wrapped_mode = true;
        max_width = 4096;

        is_draw_debug = false;
        is_debug_draw_glyph_all_area = true;
        is_debug_draw_glyph_outline_thickness_area = false;
        is_debug_draw_glyph_raw_area = false;
        is_debug_draw_glyph_real_area = true;
        color_debug_draw_glyph_all_area = "#00FF00FF";
        color_debug_draw_glyph_outline_thickness_area = "#00FFFFFF";
        color_debug_draw_glyph_raw_area = "#FF0000FF";
        color_debug_draw_glyph_real_area = "#0000FFFF";
    }
    // 使用gpu渲染
    bool use_gpu;
    // 输出文件
    std::string output_file;
    // 字距
    int spacing_horiz;
    // 行距
    int spacing_vert;

    // 生成图片时单个文字与单个文字之间X轴的间距
    int spacing_glyph_x;
    // 生成图片时单个文字与单个文字之间Y轴的间距
    int spacing_glyph_y;

    // 行高调整
    int line_height_padding_adcance;

    // 单个文字 xadvance 增加距离
    int glyph_padding_xadvance;
    // 单个文字 yadvance 增加距离
    int glyph_padding_yadvance;

    // 单个文字距离顶部间距
    int glyph_padding_up;
    // 单个文字距离底部间距
    int glyph_padding_down;
    // 单个文字距离左边间距
    int glyph_padding_left;
    // 单个文字距离右边间距
    int glyph_padding_right;

    // 生成的图片距离顶部间距
    int padding_up;
    // 生成的图片距离底部间距
    int padding_down;
    // 生成的图片距离左边间距
    int padding_left;
    // 生成的图片距离右边间距
    int padding_right;
    // 生成图片的宽高是否是2的n次方
    bool is_NPOT;
    // 是否完全包裹字体
    bool is_fully_wrapped_mode;
    // 输出图片最大宽度
    int max_width;


    // 字体样式
    TextStyle text_style;

    std::vector<PageConfig> pages;


    // 是否绘制debug信息
    bool is_draw_debug;
    bool is_debug_draw_glyph_all_area;
    bool is_debug_draw_glyph_outline_thickness_area;
    bool is_debug_draw_glyph_raw_area;
    bool is_debug_draw_glyph_real_area;
    std::string color_debug_draw_glyph_all_area;
    std::string color_debug_draw_glyph_outline_thickness_area;
    std::string color_debug_draw_glyph_raw_area;
    std::string color_debug_draw_glyph_real_area;

};
AJSON(GenerateConfig,
    use_gpu,
    output_file,
    spacing_horiz,
    spacing_vert,
    spacing_glyph_x,
    spacing_glyph_y,
    line_height_padding_adcance,
    glyph_padding_xadvance,
    glyph_padding_yadvance,
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
    // 字符实际占用宽度
    int width;
    // 字符实际占用高度
    int height;
    // 绘制字符时的偏移量
    int xoffset;
    int yoffset;
    // 光标在绘制完该字符后需要移动的距离。
    int xadvance;
    // 字符所在的纹理页面编号（用于多页面支持）
    int page;
    // 指定颜色通道（比如 RGBA 格式中的 15 代表所有通道）
    int chnl;


    // 单个文字距离顶部间距(>=0)
    int padding_up;
    // 单个文字距离底部间距(>=0)
    int padding_down;
    // 单个文字距离左边间距(>=0)
    int padding_left;
    // 单个文字距离右边间距(>=0)
    int padding_right;
};

struct FntPage
{
    // 是否固定宽度对齐(按最大字符宽度)
    bool fixed_width_alignment;
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


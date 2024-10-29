#include "Utils.h"
#include <sstream>
#include <iomanip>
#include "include/effects/SkGradientShader.h"
#include "include/effects/SkImageFilters.h"
#include "tinyutf8.h"

std::string stringFormat(const char* format, ...)
{
    std::string ret;

    va_list args;
    va_start(args, format);

    char tmp[256];
    int r = _vsnprintf_s(tmp, 255, 256, format, args);

    if (r > 0)
    {
        ret = tmp;
    }
    else
    {
        int n = 512;
        std::string str;
        str.resize(n);

        while ((r = _vsnprintf_s(&str[0], n, n, format, args)) < 0)
        {
            n *= 2;
            str.resize(n);
        }

        ret = str.c_str();
    }

    va_end(args);

    return ret;
}

int nextPOT(int x)
{
    --x;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return ++x;
}

int getMinWidth(const FntPage& page, const GenerateConfig& config)
{
    // 字符增加的宽高
    int glyhMargin = config.text_style.outline_thickness;
    int width = 0;
    for (auto&& glyphInfo : page.glyphs)
    {
        if (glyphInfo.width > width)
            width = glyphInfo.width;
    }
    return width + config.padding_left + config.padding_right + config.glyph_padding_left + config.glyph_padding_right + glyhMargin * 2;
}

int calculateHeight(const FntPage& page, const GenerateConfig& config, int maxWidth)
{
    // 字符增加的宽高
    int glyhMargin = config.text_style.outline_thickness;
    int x = config.padding_left;
    int y = config.padding_up;
    int maxHeight = 0;

    for (auto&& glyphInfo : page.glyphs)
    {
        // 字符宽高（字符宽高+描边大小）
        int glyphWidth = glyphInfo.width + glyhMargin * 2;
        int glyphHeight = glyphInfo.height + glyhMargin * 2;

        // 字符实际宽高（包括字符预留距离）
        int glyphRealWidth = glyphWidth + config.glyph_padding_left + config.glyph_padding_right;
        int glyphRealHeight = glyphHeight + config.glyph_padding_up + config.glyph_padding_down;

        auto maxHeightBack = maxHeight;
        maxHeight = std::max(maxHeight, glyphRealHeight);

        if (x + glyphRealWidth + config.padding_right >= maxWidth)
        {
            y += maxHeightBack;
            y += config.spacing_glyph_y;
            x = config.padding_left;
            maxHeight = glyphRealHeight;
        }
        x += glyphRealWidth;
        x += config.spacing_glyph_x;
    }

    y += maxHeight;

    if (config.is_NPOT)
    {
        // 2的n次方之后底部留白小于配置的留白距离
        if (nextPOT(y) - y < config.padding_down)
        {
            // 增加高度
            y = y + config.padding_down;
        }
        // 对齐
        return nextPOT(y);
    }
    else
    {
        return y + config.padding_down;
    }
}

std::vector<char32_t> collectCodepoints(const PageConfig& config)
{
    std::set<char32_t> charSet;
    std::vector<char32_t> codepoints;
    tiny_utf8::string utf8_text = config.text;

    codepoints.reserve(utf8_text.size() + config.chars.size());

    for (auto c : config.chars)
    {
        char32_t codepoint = (char32_t)c;
        if (charSet.count(codepoint) == 0)
        {
            charSet.insert(codepoint);
            codepoints.push_back(codepoint);
        }
    }

    for_each(utf8_text.begin(), utf8_text.end(), [&](char32_t codepoint) 
    {
        if (charSet.count(codepoint) == 0)
        {
            charSet.insert(codepoint);
            codepoints.push_back(codepoint);
        }
    });

    return codepoints;
}

// 获取字符的度量信息
GlyphInfo getGlyphInfo(SkFont font, char32_t codepoint)
{
    SkRect bounds;
    auto width = font.measureText(&codepoint, sizeof(codepoint), SkTextEncoding::kUTF32, &bounds);

    GlyphInfo glyphInfo;
    memset(&glyphInfo, 0, sizeof(glyphInfo));
    glyphInfo.codepoint = codepoint;
    glyphInfo.font = font;
    glyphInfo.x = 0;
    glyphInfo.y = 0;
    glyphInfo.width = (int)std::ceilf(bounds.width());
    glyphInfo.height = (int)std::ceilf(bounds.height());
    glyphInfo.xoffset = (int)std::ceilf(bounds.left());   // 左侧偏移量
    glyphInfo.yoffset = (int)std::ceilf(bounds.bottom()); // 基线到字符顶部的偏移
    glyphInfo.xadvance = (int)std::ceilf(width);
    glyphInfo.page = 0;
    glyphInfo.chnl = 15;   // 使用所有通道

    return glyphInfo;
}

SkFontStyle getFontStyle(bool isBold, bool isItalic)
{
    if (isBold && isItalic)
        return SkFontStyle::BoldItalic();
    if (isBold)
        return SkFontStyle::Bold();
    if (isItalic)
        return SkFontStyle::Italic();
    return SkFontStyle::Normal();
}

// 解析十六进制字符串为整数
int hexToInt(const std::string& hex, bool* ok)
{
    int value;
    std::istringstream iss(hex);
    iss >> std::hex >> value;
    if (iss.fail()) 
    {
        if (ok)
            *ok = false;
        return 0;
    }
    return value;
}

static std::unordered_map<std::string, SkColor> colorMap;

SkColor stringToSkColor(const std::string hex)
{
    //auto it = colorMap.find(hex);
    //if (it != colorMap.end())
    //    return it->second;

    if (hex.empty() || hex[0] != '#')
    {
        std::cerr << "invalid color: " << hex << std::endl;
        //colorMap.insert(std::make_pair(hex, SK_ColorBLACK));
        return SK_ColorBLACK;
    }
    std::string hex_color = hex;
    while (hex_color.size() < 9)
    {
        hex_color.append("F");
    }

    bool ok = true;
    int r = hexToInt(hex_color.substr(1, 2), &ok);  // Red
    int g = hexToInt(hex_color.substr(3, 2), &ok);  // Green
    int b = hexToInt(hex_color.substr(5, 2), &ok);  // Blue
    int a = hexToInt(hex_color.substr(7, 2), &ok);  // Alpha

    if (!ok)
    {
        std::cerr << "invalid color: " << hex << std::endl;
    }

    auto color = SkColorSetARGB((U8CPU)a, (U8CPU)r, (U8CPU)g, (U8CPU)b);
    //colorMap.insert(std::make_pair(hex, color));
    return color;
}

// 将 RGBA 转换为 #RRGGBBAA 格式的字符串
std::string rgbaToHex(int r, int g, int b, int a) {
    std::stringstream ss;

    // 设置输出为16进制，且宽度为2位，不足补0
    ss << "#"
        << std::hex << std::setw(2) << std::setfill('0') << (r & 0xFF)
        << std::setw(2) << std::setfill('0') << (g & 0xFF)
        << std::setw(2) << std::setfill('0') << (b & 0xFF)
        << std::setw(2) << std::setfill('0') << (a & 0xFF);

    return ss.str();
}


struct NameModePair
{
    const char* name;
    SkBlendMode mode;
};
// 混合模式映射
static NameModePair ModeMap[] = {
    { "Clear", SkBlendMode::kClear },
    { "Src", SkBlendMode::kSrc },
    { "Dst", SkBlendMode::kDst },
    { "SrcOver", SkBlendMode::kSrcOver },
    { "DstOver", SkBlendMode::kDstOver },
    { "SrcIn", SkBlendMode::kSrcIn },
    { "DstIn", SkBlendMode::kDstIn },
    { "SrcOut", SkBlendMode::kSrcOut },
    { "DstOut", SkBlendMode::kDstOut },
    { "SrcATop", SkBlendMode::kSrcATop },
    { "DstATop", SkBlendMode::kDstATop },
    { "Xor", SkBlendMode::kXor },
    { "Plus", SkBlendMode::kPlus },
    { "Modulate", SkBlendMode::kModulate },
    { "Screen", SkBlendMode::kScreen },
    { "Overlay", SkBlendMode::kOverlay },
    { "Darken", SkBlendMode::kDarken },
    { "Lighten", SkBlendMode::kLighten },
    { "ColorDodge", SkBlendMode::kColorDodge },
    { "ColorBurn", SkBlendMode::kColorBurn },
    { "HardLight", SkBlendMode::kHardLight },
    { "SoftLight", SkBlendMode::kSoftLight },
    { "Difference", SkBlendMode::kDifference },
    { "Exclusion", SkBlendMode::kExclusion },
    { "Multiply", SkBlendMode::kMultiply },
    { "Hue", SkBlendMode::kHue },
    { "Saturation", SkBlendMode::kSaturation },
    { "Color", SkBlendMode::kColor },
    { "Luminosity", SkBlendMode::kLuminosity },
    { "LastCoeffMode", SkBlendMode::kLastCoeffMode },
    { "LastSeparableMode", SkBlendMode::kLastSeparableMode },
    { "LastMode", SkBlendMode::kLastMode },
};

SkPaint createPaint(const std::string& color, const std::string& blend_mode)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(stringToSkColor(color));

    // 混合模式设置
    if (!blend_mode.empty())
    {
        for (auto i = 0; i < sizeof(ModeMap) / sizeof(ModeMap[0]); ++i)
        {
            if (blend_mode == ModeMap[i].name)
            {
                paint.setBlendMode(ModeMap[i].mode);
                break;
            }
        }
    }


    return paint;
}

void setPaintShader(SkPaint& paint, const TextEffect& effect, SkScalar x, SkScalar y, SkScalar w, SkScalar h)
{
    if (effect.effect_type == "linear_gradient")
    {
        auto& params = effect.linear_gradient;

        SkPoint pts[2]{ SkPoint::Make(x + w * params.begin.x, y + h * params.begin.y), SkPoint::Make(x + w * params.end.x, y + h * params.end.y) };

        std::vector<SkColor> colors;
        colors.reserve(params.colors.capacity());

        for (auto& color : params.colors)
            colors.push_back(stringToSkColor(color));

        // 至少两种颜色
        while (colors.size() < 2)
        {
            colors.push_back(SK_ColorBLACK);
        }

        if (params.pos.size() < 2)
        {
            auto shader = SkGradientShader::MakeLinear(pts, &colors[0], nullptr, (int)colors.size(), SkTileMode::kClamp);
            paint.setShader(shader);
        }
        else
        {
            auto shader = SkGradientShader::MakeLinear(pts, &colors[0], &params.pos[0], (int)std::min(colors.size(), params.pos.size()), SkTileMode::kClamp);
            paint.setShader(shader);
        }
    }
}

SkPaint createShadowPaint(const TextShadow& config)
{
    SkPaint paint = createPaint(config.color, config.blend_mode);
    // 创建外发光效果的滤镜
    auto filter = SkImageFilters::DropShadow(
        config.offsetx, config.offsety,         // X和Y方向的位移为0（居中发光）
        config.blur_radius, config.blur_radius, // 模糊半径（水平和垂直）
        stringToSkColor(config.color),          // 发光颜色
        nullptr                                 // 父滤镜为空
    );
    paint.setImageFilter(filter);
    return paint;
}

std::vector<SkPaint> createShadowPaints(const std::vector<TextShadow>& configs)
{
    if (configs.empty())
        return {};

    std::vector<SkPaint> paints;

    for (auto& config : configs)
    {
        paints.push_back(createShadowPaint(config));
    }
    return paints;
}
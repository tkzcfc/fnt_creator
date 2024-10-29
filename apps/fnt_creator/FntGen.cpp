#include "FntGen.h"
#include "Utils.h"
#include "Clock.h"

#define NOMINMAX

#define SK_GANESH
#define SK_GL
#include "include/core/SkSurface.h"
#include "include/core/Skcanvas.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/gl/GrGLAssembleInterface.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <assert.h>

// is_fully_wrapped_mode == false时
// 开启之后 底部基准线以实际文字对齐，关闭则以文字描边对齐
#define BASE_IN_REAL_TEXT_BOTTOM 1

FntGen::FntGen()
    : m_maxOffsetY(0)
    , m_isEditorMode(false)
    , m_editorShowPageIndex(0)
{
    m_pageRenderOpenglData.texture_id = 0;
    m_pageRenderOpenglData.width = 0;
    m_pageRenderOpenglData.height = 0;

    auto interface = GrGLMakeNativeInterface();
    if (!interface) 
    {
        //backup plan. see https://gist.github.com/ad8e/dd150b775ae6aa4d5cf1a092e4713add?permalink_comment_id=4680136#gistcomment-4680136
        interface = GrGLMakeAssembledInterface(nullptr, [](void*, const char name[]) -> GrGLFuncPtr { return glfwGetProcAddress(name); });
    }
    if (interface)
        m_context = GrDirectContext::MakeGL(interface);
    else
        m_context = nullptr;
}

FntGen::~FntGen()
{
}

bool FntGen::run(const GenerateConfig& config)
{
    m_outFileName = config.output_file;
    if (m_outFileName.size() > 4 && _stricmp(m_outFileName.substr(m_outFileName.length() - 4).c_str(), ".fnt") == 0)
        m_outFileName = m_outFileName.substr(0, m_outFileName.length() - 4);

    m_fntInfo.fontSize = config.text_style.font_size;
    m_fntInfo.isBold = config.text_style.is_bold;
    m_fntInfo.isItalic = config.text_style.is_italic;
    m_fntInfo.useUnicode = true;
    m_fntInfo.scaleH = 100;
    m_fntInfo.useSmoothing = true;
    m_fntInfo.aa = 2;
    m_fntInfo.paddingUp = 0;
    m_fntInfo.paddingRight = 0;
    m_fntInfo.paddingDown = 0;
    m_fntInfo.paddingLeft = 0;
    m_fntInfo.spacingHoriz = config.spacing_horiz;
    m_fntInfo.spacingVert = config.spacing_vert;
    m_fntInfo.outlineThickness = config.text_style.outline_thickness;
    m_fntInfo.commonLineHeight = config.text_style.font_size + config.text_style.outline_thickness * 2 + config.glyph_padding_up + config.glyph_padding_down + config.line_height_padding_adcance;
    m_fntInfo.base = config.text_style.font_size;
    m_fntInfo.pages.clear();

    do
    {
        Clock clock;

        // 匹配字体
        matchFont(config);

        clock.update();
        printf("match font time: %.2fs(%dms)\n", clock.getDeltaTimeInSecs(), (int)clock.getDeltaTime());

        // 生成图片
        if (!draw(config))
            break;

        clock.update();
        printf("draw text time: %.2fs(%dms)\n", clock.getDeltaTimeInSecs(), (int)clock.getDeltaTime());

        // 保存字体
        if (!saveFont(config))
            break;

        clock.update();
        printf("save font time: %.2fs(%dms)\n", clock.getDeltaTimeInSecs(), (int)clock.getDeltaTime());

        return true;
    } while (false);

    return false;
}

void FntGen::matchFont(const GenerateConfig& config)
{
    // 获取系统默认的字体管理器
    sk_sp<SkFontMgr> fontMgr = SkFontMgr::RefDefault();
    for (size_t i = 0; i < config.pages.size(); ++i)
    {
        if (m_isEditorMode && m_editorShowPageIndex != i)
        {
            m_fntInfo.pages.push_back(FntPage{
                .width = 0,
                .height = 0,
                .glyphs = {},
                .fileName = "",
            });
            continue;
        }
        auto& pageCfg = config.pages[i];

        std::vector<GlyphInfo> glyphs;
        for (auto& codepoint : collectCodepoints(pageCfg))
        {
            sk_sp<SkTypeface> typeface;
            for (auto& font : pageCfg.fonts)
            {
                if (!font.empty())
                {
                    typeface = SkTypeface::MakeFromFile(font.c_str());
                    if (!typeface)
                    {
                        typeface = SkTypeface::MakeFromName(font.c_str(), getFontStyle(config.text_style.is_bold, config.text_style.is_italic));
                    }
                }

                if (typeface->unicharToGlyph((SkUnichar)codepoint) == 0)
                {
                    typeface = nullptr;
                }

                if (typeface)
                    break;
            }

            if (!typeface)
            {
                typeface = fontMgr->matchFamilyStyleCharacter(nullptr, getFontStyle(config.text_style.is_bold, config.text_style.is_italic), nullptr, 0, codepoint);

                if (!typeface)
                {
                    if (codepoint != char32_t(10))
                    {
                        std::cerr << "No matching font found for character: " << int(codepoint) << std::endl;
                    }
                }
            }

            if (typeface)
            {
                SkFont font;
                font.setSize(config.text_style.font_size);
                font.setTypeface(typeface);

                auto glyph = getGlyphInfo(font, codepoint);
                glyph.page = (int)i;
                glyphs.push_back(glyph);
            }
        }

        if (!glyphs.empty())
        {
            m_fntInfo.pages.push_back(FntPage{
                .width = 0,
                .height = 0,
                .glyphs = glyphs,
                .fileName = "",
                });
        }
    }
}


bool FntGen::draw(const GenerateConfig& config)
{
    m_maxOffsetY = 0;
    for (size_t pageIndex = 0; pageIndex < m_fntInfo.pages.size(); ++pageIndex)
    {
        if (m_isEditorMode && m_editorShowPageIndex != pageIndex)
            continue;

        auto& page = m_fntInfo.pages[pageIndex];

        initPageData(config, page, pageIndex);
        if (!drawPage(config, page))
            return false;
    }
    return true;
}

void FntGen::initPageData(const GenerateConfig& config, FntPage& page, int pageIndex)
{
    if (m_fntInfo.pages.size() > 1)
        page.fileName = stringFormat("%s%d.png", m_outFileName.c_str(), pageIndex);
    else
        page.fileName = stringFormat("%s.png", m_outFileName.c_str());

    // 计算最适合的最大宽度
    int maxWidth = 0;
    for (int x = 2;;)
    {
        auto max = x;
        if (config.is_NPOT)
        {
            max = nextPOT(max);
        }
        if (max > getMinWidth(page, config))
        {
            if (max > config.max_width)
            {
                maxWidth = max;
                break;
            }

            if (calculateHeight(page, config, max) <= max)
            {
                maxWidth = max;
                break;
            }
        }

        x *= 2;
    }
    if (maxWidth > config.max_width)
        maxWidth = config.max_width;

    // 计算本页宽高
    page.width = maxWidth;
    page.height = calculateHeight(page, config, maxWidth);
}

bool FntGen::drawPage(const GenerateConfig& config, FntPage& page)
{
    bool useGPU = supportGPU();

    if (useGPU && config.use_gpu)
    {
        // 查询最大纹理尺寸
        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        if (page.width > maxTextureSize || page.height > maxTextureSize)
        {
            printf("The required size(%dx%d) exceeds the maximum texture size supported by the device : %dx%d, using CPU rendering", page.width, page.height, maxTextureSize, maxTextureSize);
            useGPU = false;
        }
    }
    else
    {
        useGPU = false;
    }

    // 使用GPU渲染
    if (useGPU)
    {
        std::cout << "Rendering with GPU (Opengl)" << std::endl;

        // 重置 OpenGL 状态
        m_context->resetContext(kAll_GrBackendState);


        GLuint fbo = 0;
        GLuint texture = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // 清理各种缓冲区
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);  // 颜色缓冲区清为透明黑色
        glClearDepth(1.0f);                    // 深度缓冲区清为1.0（最远）
        glClearStencil(0);                     // 模板缓冲区清为0
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // 创建用于离屏渲染的纹理
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, page.width, page.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 将纹理附加到 FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            printf("Failed to create framebuffer.\n");
            return false;
        }

        GrGLFramebufferInfo framebufferInfo;
        framebufferInfo.fFBOID = fbo; // assume default framebuffer
        // We are always using OpenGL and we use RGBA8 internal format for both RGBA and BGRA configs in OpenGL.
        //(replace line below with this one to enable correct color spaces) framebufferInfo.fFormat = GL_SRGB8_ALPHA8;
        framebufferInfo.fFormat = GL_RGBA8;

        SkColorType colorType = kRGBA_8888_SkColorType;
        GrBackendRenderTarget backendRenderTarget = GrBackendRenderTarget(page.width, page.height,
            0, // sample count
            0, // stencil bits
            framebufferInfo);
        auto surface = SkSurfaces::WrapBackendRenderTarget(m_context.get(), backendRenderTarget, kTopLeft_GrSurfaceOrigin, colorType, nullptr, nullptr);
        assert(surface != nullptr);

        auto canvas = surface->getCanvas();
        drawGlyphs(config, page, canvas);
        surface->flushAndSubmit();

        if (m_isEditorMode)
        {
            m_pageRenderOpenglData.texture_id = texture;
            m_pageRenderOpenglData.width = page.width;
            m_pageRenderOpenglData.height = page.height;
            glDeleteFramebuffers(1, &fbo);
        }
        else
        {
            SkImageInfo imageInfo = SkImageInfo::Make(page.width, page.height, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
            SkBitmap bitmap;
            bitmap.allocPixels(imageInfo, imageInfo.minRowBytes());

            if (!surface->readPixels(bitmap, 0, 0)) 
            {
                std::cerr << "readPixels failed" << std::endl;
                return false;
            }

            glDeleteTextures(1, &texture);
            texture = 0;
            glDeleteFramebuffers(1, &fbo);
            return saveBitmapToFile(page.fileName, bitmap);
        }
    }
    else
    {
        std::cout << "Rendering with CPU" << std::endl;

        SkImageInfo imageInfo = SkImageInfo::Make(page.width, page.height, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        SkBitmap bitmap;
        bitmap.allocPixels(imageInfo, imageInfo.minRowBytes());

        SkCanvas cv(bitmap);
        drawGlyphs(config, page, &cv);

        if (m_isEditorMode)
        {
            uint8_t* pixels = static_cast<uint8_t*>(bitmap.getPixels());
            if (!pixels)
            {
                std::cerr << "Failed to get pixels from SkBitmap!" << std::endl;
                return false;
            }

            m_pageRenderOpenglData.texture_id = 0;
            m_pageRenderOpenglData.width = page.width;
            m_pageRenderOpenglData.height = page.height;

            size_t dataSize = page.width * page.height * 4;
            m_pageRenderOpenglData.pixels.resize(dataSize);
            std::memcpy(&m_pageRenderOpenglData.pixels[0], pixels, dataSize);
        }
        else
        {
            return saveBitmapToFile(page.fileName, bitmap);
        }
    }
    return true;
}

bool FntGen::saveBitmapToFile(const std::string& filename, SkBitmap& bitmap)
{
    SkFILEWStream file(filename.c_str());
    if (!file.isValid())
    {
        std::cerr << "failed to open file: " << filename;
        return false;
    }
    if (!SkPngEncoder::Encode(&file, bitmap.pixmap(), {}))
    {
        std::cerr << "failed to write file: " << filename;
        return false;
    }
    return true;
}

void FntGen::drawGlyphs(const GenerateConfig& config, FntPage& page, SkCanvas* canvas)
{
    canvas->clear(stringToSkColor(config.text_style.background_color));

    // 描边阴影画笔
    std::vector<SkPaint> outlineShadowPaints = createShadowPaints(config.text_style.outline_shadows);
    // 文字描边画笔
    SkPaint outlinePaint = createPaint(config.text_style.outline_color, config.text_style.outline_blend_mode);
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(config.text_style.outline_thickness <= 0 ? 1 : config.text_style.outline_thickness * config.text_style.outline_thickness_render_scale);

    // 描边阴影画笔
    std::vector<SkPaint> textShadowPaints = createShadowPaints(config.text_style.shadows);
    // 文字画笔
    SkPaint textPaint = createPaint(config.text_style.color, config.text_style.blend_mode);

    // 调试画笔
    SkPaint debugPaint;
    debugPaint.setAntiAlias(true);                // 开启抗锯齿
    debugPaint.setStyle(SkPaint::kStroke_Style);  // 设置为描边模式
    debugPaint.setStrokeWidth(1);                 // 设置描边宽度

    // 字符增加的宽高
    int glyhMargin = config.text_style.outline_thickness;

    int x = config.padding_left;
    int y = config.padding_up;
    int maxHeight = 0;
    for (auto& glyphInfo : page.glyphs)
    {
        // 字符宽高（字符宽高+描边大小）
        int glyphWidth = glyphInfo.width + glyhMargin * 2;
        int glyphHeight = glyphInfo.height + glyhMargin * 2;

        // 字符实际宽高（包括字符预留距离）
        int glyphRealWidth = glyphWidth + config.glyph_padding_left + config.glyph_padding_right;
        int glyphRealHeight = glyphHeight + config.glyph_padding_up + config.glyph_padding_down;

        auto maxHeightBack = maxHeight;
        maxHeight = std::max(maxHeight, glyphRealHeight);

        if (x + glyphRealWidth + config.padding_right >= page.width)
        {
            y += maxHeightBack;
            y += config.spacing_glyph_y;
            x = config.padding_left;
            maxHeight = glyphRealHeight;
        }
        glyphInfo.x = x;
        glyphInfo.y = y;

        //printf("x:%d,y:%d, w:%d,h:%d  xoffset:%d,yoffset:%d  xadvance:%d\n", glyphInfo.x, glyphInfo.y, glyphInfo.width, glyphInfo.height, glyphInfo.xoffset, glyphInfo.yoffset, glyphInfo.xadvance);

        // 绘制相关逻辑
        {
            SkScalar drawx = x + glyhMargin + config.glyph_padding_left - glyphInfo.xoffset;
            SkScalar drawy = y + glyhMargin + config.glyph_padding_up + glyphInfo.height - glyphInfo.yoffset;

            SkScalar w = (SkScalar)glyphInfo.width;
            SkScalar h = (SkScalar)glyphInfo.height;

            // 描边绘制
            if (config.text_style.outline_thickness > 0)
            {
                // 描边阴影
                for (size_t index = 0; index < outlineShadowPaints.size(); ++index)
                {
                    auto& shadowPaint = outlineShadowPaints[index];
                    //setPaintShader(shadowPaint, config.text_style.outline_shadows[index].effect, drawx, drawy - h, w, h);
                    canvas->drawSimpleText(&glyphInfo.codepoint, sizeof(glyphInfo.codepoint), SkTextEncoding::kUTF32, drawx, drawy, glyphInfo.font, shadowPaint);
                }

                setPaintShader(outlinePaint, config.text_style.outline_effect, drawx, drawy - h, w, h);
                canvas->drawSimpleText(&glyphInfo.codepoint, sizeof(glyphInfo.codepoint), SkTextEncoding::kUTF32, drawx, drawy, glyphInfo.font, outlinePaint);
            }

            // 文字阴影
            for (size_t index = 0; index < textShadowPaints.size(); ++index)
            {
                auto& shadowPaint = textShadowPaints[index];
                //setPaintShader(shadowPaint, config.text_style.shadows[index].effect, drawx, drawy - h, w, h);
                canvas->drawSimpleText(&glyphInfo.codepoint, sizeof(glyphInfo.codepoint), SkTextEncoding::kUTF32, drawx, drawy, glyphInfo.font, shadowPaint);
            }

            // 文字绘制
            setPaintShader(textPaint, config.text_style.effect, drawx, drawy - h, w, h);
            canvas->drawSimpleText(&glyphInfo.codepoint, sizeof(glyphInfo.codepoint), SkTextEncoding::kUTF32, drawx, drawy, glyphInfo.font, textPaint);

            if (config.is_draw_debug)
            {
                // 绘制字符全部区域
                if (config.glyph_padding_left != 0 || config.glyph_padding_right != 0 || config.glyph_padding_up != 0 || config.glyph_padding_down != 0)
                {
                    // 绘制字符不带描边区域
                    SkRect rect = SkRect::MakeXYWH(x, y, glyphRealWidth, glyphRealHeight);
                    debugPaint.setColor(SK_ColorGREEN);
                    canvas->drawRect(rect, debugPaint);
                }

                // 绘制字符+描边区域
                if (config.text_style.outline_thickness > 0)
                {
                    SkRect rect = SkRect::MakeXYWH(x + config.glyph_padding_left, y + config.glyph_padding_up, glyphWidth, glyphHeight);
                    debugPaint.setColor(SK_ColorCYAN);
                    canvas->drawRect(rect, debugPaint);
                }

                // 绘制字符不带描边区域
                SkRect rect = SkRect::MakeXYWH(x + config.glyph_padding_left + glyhMargin, y + config.glyph_padding_up + glyhMargin, glyphInfo.width, glyphInfo.height);
                debugPaint.setColor(SK_ColorMAGENTA);
                canvas->drawRect(rect, debugPaint);
            }
        }

        x += glyphRealWidth;
        x += config.spacing_glyph_x;

        glyphInfo.yoffset += config.glyph_padding_yadvance;
        glyphInfo.yoffset += config.line_height_padding_adcance;

        if (config.is_fully_wrapped_mode)
        {
            m_maxOffsetY = std::max(m_maxOffsetY, glyphInfo.yoffset);
        }
        else
        {
#if BASE_IN_REAL_TEXT_BOTTOM
            m_maxOffsetY = std::max(m_maxOffsetY, glyphInfo.yoffset + glyhMargin);
#else
            m_maxOffsetY = std::max(m_maxOffsetY, glyphInfo.yoffset);
#endif
        }

        // 偏移值计算
        int diffVal = int(glyphInfo.font.getSize() - glyphInfo.height);
        glyphInfo.yoffset = glyphInfo.yoffset + diffVal;

        int leftSpace = std::max(glyphInfo.xadvance - glyphInfo.width, 0);

        // 字符描边距离
        glyphInfo.width = glyphRealWidth;
        glyphInfo.height = glyphRealHeight;
        glyphInfo.xoffset = glyphInfo.xoffset - (leftSpace / 2);
#if BASE_IN_REAL_TEXT_BOTTOM
        if (!config.is_fully_wrapped_mode)
            glyphInfo.yoffset = glyphInfo.yoffset + glyhMargin;
#endif

        // 外部文件自定义增加距离
        //glyphInfo.xadvance = glyphInfo.xadvance + glyhMargin * 2 + config.glyph_padding_left + config.glyph_padding_right;
        glyphInfo.xadvance += config.glyph_padding_xadvance;
    }

    canvas->flush();
}

bool FntGen::saveFont(const GenerateConfig& config)
{
    if (m_isEditorMode)
        return true;

    FILE* f;
    errno_t e = fopen_s(&f, stringFormat("%s.fnt", m_outFileName.c_str()).c_str(), "wb");
    if (e != 0 || f == 0)
    {
        std::cerr << "failed to open file: " << m_outFileName;
        return false;
    }

    int outWidth = 0;
    int outHeight = 0;
    int numPages = m_fntInfo.pages.size();
    int fourChnlPacked = 0;
    int alphaChnl = 1;
    int redChnl = 0;
    int greenChnl = 0;
    int blueChnl = 0;

    for (auto& page : m_fntInfo.pages)
    {
        if (outWidth < page.width)
            outWidth = page.width;
        if (outHeight < page.height)
            outHeight = page.height;
    }

    if (config.is_fully_wrapped_mode)
    {
        m_fntInfo.commonLineHeight += m_maxOffsetY;
    }

    fprintf(f, "info face=\"arial\" size=%d bold=%d italic=%d charset=\"%s\" unicode=%d stretchH=%d smooth=%d aa=%d padding=%d,%d,%d,%d spacing=%d,%d outline=%d\r\n", m_fntInfo.fontSize, m_fntInfo.isBold, m_fntInfo.isItalic, m_fntInfo.useUnicode ? "" : "ANSI", m_fntInfo.useUnicode, m_fntInfo.scaleH, m_fntInfo.useSmoothing, m_fntInfo.aa, m_fntInfo.paddingUp, m_fntInfo.paddingRight, m_fntInfo.paddingDown, m_fntInfo.paddingLeft, m_fntInfo.spacingHoriz, m_fntInfo.spacingVert, m_fntInfo.outlineThickness);
    fprintf(f, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d packed=%d alphaChnl=%d redChnl=%d greenChnl=%d blueChnl=%d\r\n", m_fntInfo.commonLineHeight, m_fntInfo.base, outWidth, outHeight, int(numPages), fourChnlPacked, alphaChnl, redChnl, greenChnl, blueChnl);

    for (size_t n = 0; n < numPages; n++)
    {
        auto&& page = m_fntInfo.pages[n];
        fprintf(f, "page id=%d file=\"%s\"\r\n", (int)n, page.fileName.c_str());

        fprintf(f, "chars count=%d\r\n", (int)page.glyphs.size());
        for (auto&& glyphInfo : page.glyphs)
        {
            fprintf(f, "char id=%lld   x=%d     y=%d     width=%d    height=%d    xoffset=%d     yoffset=%d     xadvance=%d    page=%d  chnl=%d\r\n", (long long)glyphInfo.codepoint, glyphInfo.x, glyphInfo.y, glyphInfo.width, glyphInfo.height, glyphInfo.xoffset, glyphInfo.yoffset, glyphInfo.xadvance, glyphInfo.page, glyphInfo.chnl);
        }
    }

    fclose(f);
    return true;
}
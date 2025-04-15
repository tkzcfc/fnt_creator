#include "Editor.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include "Utils.h"


static const char* vertexShaderCode = R"(
#version 330 core

uniform mat4 u_MVPMatrix;

layout (location = 0) in vec4 a_position;
layout (location = 1) in vec2 a_texCoord;

out vec2 v_texCoord;

void main() {
    v_texCoord = a_texCoord;
    
    gl_Position = u_MVPMatrix * a_position;
    //gl_Position = a_position;
}
)";

static const char* fragmentShaderCode = R"(
#version 330 core
out vec4 FragColor;

in vec2 v_texCoord;

uniform sampler2D u_tex0;

void main() {
    FragColor = texture(u_tex0, v_texCoord);
}
)";

Editor::Editor()
{
    m_img_shader = nullptr;
    m_texture = nullptr;
    m_needReRender = true;
    m_fntRenderBusy = false;
}

Editor::~Editor()
{
}

int Editor::run(const GenerateConfig& config)
{
    m_config = config;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
    //GLFWwindow* window = glfwCreateWindow(videoMode->width * 0.6f, videoMode->height * 0.8f, "fnt_creator", NULL, NULL);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "fnt_creator", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetWindowSizeLimits(window, 200, 150, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    if (!init())
    {
        std::cerr << "Failed to initialize" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 获取窗口内容的缩放系数
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    xscale = std::min(xscale, 1.0f);
    yscale = std::min(yscale, 1.0f);

    // Dear ImGui
    const char* glsl_version = "#version 330";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        m_screenSize.x = (float)display_w;
        m_screenSize.y = (float)display_h;

        // 设计分辨率
        glm::vec2 designResolutionSize = m_screenSize;
#if 0
        // 模拟设计分辨率和实际屏幕不一样大小
        designResolutionSize.x *= 2.0f;
        designResolutionSize.y *= 2.0f;
#else
        designResolutionSize.x /= xscale;
        designResolutionSize.y /= yscale;
#endif

        float scaleX = (float)m_screenSize.x / designResolutionSize.x;
        float scaleY = (float)m_screenSize.y / designResolutionSize.y;
        // SHOW_ALL
        scaleX = scaleY = std::min(scaleX, scaleY);

        float viewPortW = designResolutionSize.x * scaleX;
        float viewPortH = designResolutionSize.y * scaleY;
        float viewPortX = (m_screenSize.x - viewPortW) / 2.0f;
        float viewPortY = (m_screenSize.y - viewPortH) / 2.0f;

        glViewport(viewPortX, viewPortY, viewPortW, viewPortH);

        m_winSize = designResolutionSize;

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        openglDraw();

        // Dear ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        imguiDraw();

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        fntRender();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    m_gen = nullptr;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

bool Editor::init()
{
    do
    {
        m_img_shader = std::make_unique<Shader>();
        if (!m_img_shader->init(vertexShaderCode, fragmentShaderCode))
            break;

        m_texture = std::make_unique<Texture>();
        m_texture->initWithFile("loading.png");

        float vertices[] = {
            // 位置 (aPos)   // 纹理坐标 (aTexCoord)
            -0.5f,  0.5f,    0.0f, 1.0f,
            -0.5f, -0.5f,    0.0f, 0.0f,
             0.5f, -0.5f,    1.0f, 0.0f,
             0.5f,  0.5f,    1.0f, 1.0f
        };
        unsigned int indices[] = {
            0, 1, 2,
            0, 2, 3
        };

        unsigned int VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // 绑定并设置 VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        // 绑定并设置 EBO（索引缓冲对象）
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        //glVertexAttribPointer：
        //    第一个参数：着色器中 layout(location = n) 的位置。
        //    第二个参数：每个属性的数据个数（vec2 是 2）。
        //    第三个参数：数据类型（GL_FLOAT）。
        //    第四个参数：是否需要归一化（这里填 GL_FALSE）。
        //    第五个参数：步长（每个顶点的字节大小）。
        //    第六个参数：数据偏移量（在顶点数据数组中的起始位置）。
        // a_position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // a_texCoord
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        m_VAO = VAO;
        m_VBO = VBO;

        m_gen = std::make_unique<FntGen>();
        m_gen->setEditorMode(true);

        return true;
    } while (false);

    return false;
}

void Editor::fntRender()
{
    if (m_fntRenderBusy)
    {
        if (m_fntRenderTask.valid())
        {
            if (m_fntRenderTask.ready())
            {
                if (m_fntRenderTask.get() && m_gen->getPageRenderOpenglData().width > 0)
                {
                    auto& data = m_gen->getPageRenderOpenglData();
                    m_texture->initWithData(data.width, data.height, (unsigned char*) &data.pixels[0], true);
                    m_texture->setPremultipliedAlpha(false);
                }
                m_fntRenderBusy = false;
            }
        }
        else
        {
            m_fntRenderBusy = false;
        }
    }

    if (m_fntRenderBusy || !m_needReRender)
        return;

    m_needReRender = false;

    if (m_gen->supportGPU() && m_config.use_gpu)
    {
        if (m_gen->run(m_config))
        {
            auto& data = m_gen->getPageRenderOpenglData();
            m_texture->initWithTextureId(data.texture_id, data.width, data.height);
            m_texture->setPremultipliedAlpha(true);
            m_gen->clearPageRenderOpenglData();
        }
    }
    else
    {
        m_fntRenderBusy = true;
        m_fntRenderTask = async::spawn([this] {
            auto config = m_config;
            return m_gen->run(config);
        });
    }
}

void Editor::openglDraw()
{
    glm::vec2 size = m_winSize;
    float zeye = m_winSize.y / 1.154700538379252f;

    // projectionMatrix
    glm::mat4 matrixPerspective = glm::perspective(60.0f, size.x / size.y, 0.5f, zeye + size.y / 2.0f);
    glm::mat4 projectionMatrix = matrixPerspective;

    glm::vec3 eye(size.x / 2.0f, size.y / 2.0f, zeye), center(size.x / 2.0f, size.y / 2.0f, 0.0f), up(0.0f, 1.0f, 0.0f);
    glm::mat4 viewMatrix = glm::lookAt(eye, center, up);

    if (m_texture && m_texture->texture_id() > 0)
    {
        m_img_shader->use();

        float w = (float)m_texture->width();
        float h = (float)m_texture->height();

        float x = m_winSize.x * 0.5f - w * 0.5f;
        float y = m_winSize.y * 0.5f - h * 0.5f;
        float z = 0.0f;

        glm::mat4 modelMatrix = glm::translate(glm::vec3(x, y, z));
        //modelMatrix = glm::scale_slow(modelMatrix, glm::vec3(1.0f, -1.0f, 1.0f));

        
        glm::mat4 MVP = projectionMatrix * viewMatrix * modelMatrix;
        m_img_shader->setMat4("u_MVPMatrix", MVP);
        
        // 激活纹理单元 0，并绑定纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture->texture_id());
        m_img_shader->setInt("u_tex0", 0);

        // uv y翻转过
        float updatedVertices[] = {
            0.0f,  h,  0.0f, 0.0f,  // 左上
            0.0f, 0.0f,  0.0f, 1.0f,  // 左下
            w, 0.0f,  1.0f, 1.0f,  // 右下
            w, h,  1.0f, 0.0f   // 右上
        };
        // 绑定 VBO 并更新数据
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(updatedVertices), updatedVertices);

        glEnable(GL_BLEND);
        glBlendFunc(m_texture->hasPremultipliedAlpha() ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDisable(GL_BLEND);
    }
}

void Editor::imguiDraw()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + PAD, viewport->WorkPos.y + PAD), ImGuiCond_Always, ImVec2());
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("#Simple overlay", nullptr, window_flags))
    {
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Separator();

        ImGui::Text("Status:");
        ImGui::SameLine();
        if (m_fntRenderBusy || m_needReRender)
            ImGui::TextColored(ImVec4(0.8f, 0, 0, 1.0f), "Rendering");
        else
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0, 1.0f), "Free");
    }
    ImGui::End();



    bool refresh = false;

    if(ImGui::Begin("General"))
    {
        if(m_texture)
            ImGui::Text("%d x %d", m_texture->width(), m_texture->height());

        if (ImGui::Button("save config to file"))
            ajson::save_to_file(m_config, "fnt_creator_edit_config.json");

        refresh |= ImGui::Checkbox("use_gpu", &m_config.use_gpu);

        char szbuf[2048] = { 0 };
        strcpy_s(szbuf, m_config.output_file.c_str());
        if (ImGui::InputText("output_file", szbuf, sizeof(szbuf)))
            m_config.output_file = szbuf;


        if (ImGui::CollapsingHeader("spacing", ImGuiTreeNodeFlags_DefaultOpen))
        {
            refresh |= ImGui::InputInt("spacing_horiz", &m_config.spacing_horiz);
            refresh |= ImGui::InputInt("spacing_vert", &m_config.spacing_vert);
            refresh |= ImGui::InputInt("spacing_glyph_x", &m_config.spacing_glyph_x);
            refresh |= ImGui::InputInt("spacing_glyph_y", &m_config.spacing_glyph_y);
            refresh |= ImGui::InputInt("line_height_padding_adcance", &m_config.line_height_padding_adcance);
        }

        if (ImGui::CollapsingHeader("glyph", ImGuiTreeNodeFlags_DefaultOpen))
        {
            refresh |= ImGui::InputInt("glyph_padding_yadvance", &m_config.glyph_padding_yadvance);
            refresh |= ImGui::InputInt("glyph_padding_xadvance", &m_config.glyph_padding_xadvance);
            refresh |= ImGui::InputInt("glyph_padding_up", &m_config.glyph_padding_up);
            refresh |= ImGui::InputInt("glyph_padding_down", &m_config.glyph_padding_down);
            refresh |= ImGui::InputInt("glyph_padding_left", &m_config.glyph_padding_left);
            refresh |= ImGui::InputInt("glyph_padding_right", &m_config.glyph_padding_right);
        }

        if (ImGui::CollapsingHeader("texture", ImGuiTreeNodeFlags_DefaultOpen))
        {
            refresh |= ImGui::InputInt("padding_up", &m_config.padding_up);
            refresh |= ImGui::InputInt("padding_down", &m_config.padding_down);
            refresh |= ImGui::InputInt("padding_left", &m_config.padding_left);
            refresh |= ImGui::InputInt("padding_right", &m_config.padding_right);
            refresh |= ImGui::InputInt("max_width", &m_config.max_width);

            refresh |= ImGui::Checkbox("is_NPOT", &m_config.is_NPOT);
            refresh |= ImGui::Checkbox("is_fully_wrapped_mode", &m_config.is_fully_wrapped_mode);
        }

        if (ImGui::CollapsingHeader("debug", ImGuiTreeNodeFlags_DefaultOpen))
        {
            refresh |= ImGui::Checkbox("is_draw_debug", &m_config.is_draw_debug);
            if (m_config.is_draw_debug)
            {
                refresh |= ImGui::Checkbox("is_debug_draw_glyph_all_area", &m_config.is_debug_draw_glyph_all_area);
                refresh |= imguiColor("color_debug_draw_glyph_all_area", &m_config.color_debug_draw_glyph_all_area);
                ImGui::Separator();

                refresh |= ImGui::Checkbox("is_debug_draw_glyph_outline_thickness_area", &m_config.is_debug_draw_glyph_outline_thickness_area);
                refresh |= imguiColor("color_debug_draw_glyph_outline_thickness_area", &m_config.color_debug_draw_glyph_outline_thickness_area);
                ImGui::Separator();

                refresh |= ImGui::Checkbox("is_debug_draw_glyph_raw_area", &m_config.is_debug_draw_glyph_raw_area);
                refresh |= imguiColor("color_debug_draw_glyph_raw_area", &m_config.color_debug_draw_glyph_raw_area);
                ImGui::Separator();

                refresh |= ImGui::Checkbox("is_debug_draw_glyph_real_area", &m_config.is_debug_draw_glyph_real_area);
                refresh |= imguiColor("color_debug_draw_glyph_real_area", &m_config.color_debug_draw_glyph_real_area);
            }
        }

        ImGui::End();
    }

    if(ImGui::Begin("Text Style"))
    {
        auto& style = m_config.text_style;

        refresh |= ImGui::SliderInt("font_size", &style.font_size, 10, 100);
        refresh |= imguiBlendMode("blend_mode", &style.blend_mode);
        refresh |= imguiColor("background_color", &style.background_color);
        refresh |= ImGui::Checkbox("is_bold", &style.is_bold);
        refresh |= ImGui::Checkbox("is_italic", &style.is_italic);
        refresh |= imguiColor("color", &style.color);
        refresh |= imguiTextEffect("effect", style.effect);
        refresh |= imguiTextShadows("shadows", style.shadows);
        ImGui::Separator();

        refresh |= ImGui::SliderInt("outline_thickness", &style.outline_thickness, 0, 20);
        refresh |= ImGui::SliderFloat("outline_thickness_render_scale", &style.outline_thickness_render_scale, 0.5, 3.0f);
        refresh |= imguiColor("outline_color", &style.outline_color);
        refresh |= imguiBlendMode("outline_blend_mode", &style.outline_blend_mode);
        refresh |= imguiTextEffect("outline_effect", style.outline_effect);
        refresh |= imguiTextShadows("outline_shadows", style.outline_shadows);
        ImGui::Separator();


        ImGui::End();
    }

    if (refresh)
    {
        m_needReRender = true;
    }

    //ImGui::ShowDemoWindow();
}

bool Editor::imguiColor(const char* label, std::string* color)
{
    SkColor skColor = stringToSkColor(*color);

    float vcolor[] = { SkColorGetR(skColor) / 255.0f, SkColorGetG(skColor) / 255.0f, SkColorGetB(skColor) / 255.0f, SkColorGetA(skColor) / 255.0f };
    if (ImGui::ColorEdit4(label, vcolor))
    {
        *color = rgbaToHex(vcolor[0] * 255, vcolor[1] * 255, vcolor[2] * 255, vcolor[3] * 255);
        return true;
    }
    return false;
}

bool Editor::imguiBlendMode(const char* label, std::string* model)
{
    const char* arrBlendMode[] = { "Clear", "Src", "Dst", "SrcOver", "DstOver", "SrcIn", "DstIn", "SrcOut", "DstOut", "SrcATop", "DstATop", "Xor", "Plus", "Modulate", "Screen", "Overlay", "Darken", "Lighten", "ColorDodge", "ColorBurn", "HardLight", "SoftLight", "Difference", "Exclusion", "Multiply", "Hue", "Saturation", "Color", "Luminosity", "LastCoeffMode", "LastSeparableMode", "LastMode", "Default"};
    
    int index = 32;
    for (int i = 0; i < 32; ++i)
    {
        if (*model == arrBlendMode[i])
        {
            index = i;
            break;
        }
    }

    if (ImGui::Combo(label, &index, arrBlendMode, IM_ARRAYSIZE(arrBlendMode)))
    {
        *model = arrBlendMode[index];
        return true;
    }
    return false;
}

bool Editor::imguiTextEffect(const char* label, TextEffect& effect)
{
    bool refresh = false;
    if (ImGui::TreeNode(label))
    {
        const char* arrEffectType[] = { "none", "linear_gradient" };

        int index = 0;
        if (effect.effect_type == "linear_gradient")
        {
            index = 1;
        }

        if (ImGui::Combo("linear_gradient", &index, arrEffectType, IM_ARRAYSIZE(arrEffectType)))
        {
            effect.effect_type = arrEffectType[index];
            refresh = true;
        }

        if (index == 1)
        {
            if (effect.linear_gradient.colors.size() < 2)
            {
                refresh = true;
                while ((effect.linear_gradient.colors.size() < 2))
                {
                    effect.linear_gradient.colors.push_back("#ffffffff");
                    effect.linear_gradient.pos.clear();
                }
            }

            // begin
            float tmpV2[2] = { 0.0f };
            tmpV2[0] = effect.linear_gradient.begin.x;
            tmpV2[1] = effect.linear_gradient.begin.y;
            if (ImGui::SliderFloat2("begin", tmpV2, 0.0f, 1.0f))
            {
                refresh = true;
                effect.linear_gradient.begin.x = tmpV2[0];
                effect.linear_gradient.begin.y = tmpV2[1];
            }
            // end
            tmpV2[0] = effect.linear_gradient.end.x;
            tmpV2[1] = effect.linear_gradient.end.y;
            if (ImGui::SliderFloat2("end", tmpV2, 0.0f, 1.0f))
            {
                refresh = true;
                effect.linear_gradient.end.x = tmpV2[0];
                effect.linear_gradient.end.y = tmpV2[1];
            }

            int remove_index = -1;
            for (size_t i = 0; i < effect.linear_gradient.colors.size(); ++i)
            {
                ImGui::PushID(i);
                refresh |= imguiColor("color", &effect.linear_gradient.colors[i]);
                if (effect.linear_gradient.colors.size() > 2)
                {
                    ImGui::SameLine();

                    SkColor skColor = stringToSkColor(effect.linear_gradient.colors[i]);
                    float r = SkColorGetR(skColor) / 255.0f;
                    float g = SkColorGetG(skColor) / 255.0f;
                    float b = SkColorGetB(skColor) / 255.0f;

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r, g, b, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r, g, b, 0.8f));
                    if (ImGui::Button("remove"))
                    {
                        remove_index = (int)i;
                    }
                    ImGui::PopStyleColor(3);
                }
                ImGui::PopID();
            }
            if (remove_index >= 0)
            {
                refresh = true;
                effect.linear_gradient.colors.erase(effect.linear_gradient.colors.begin() + (size_t)remove_index);
                effect.linear_gradient.pos.clear();
            }

            if (ImGui::Button("add color"))
            {
                refresh = true;
                effect.linear_gradient.colors.push_back("#ffffffff");
                effect.linear_gradient.pos.clear();
            }
        }
        ImGui::TreePop();
    }

    return refresh;
}

bool Editor::imguiTextShadows(const char* label, std::vector<TextShadow>& shadows)
{
    bool refresh = false;
    ImGui::PushID(label);

    if (ImGui::Button("add shadow"))
    {
        shadows.push_back(TextShadow());
        refresh = true;
    }

    if (ImGui::BeginTabBar(label))
    {
        int remove_index = -1;
        size_t index = 0;
        for (auto& shadow : shadows)
        {
            index++;
            auto name = stringFormat("shadow_%d", (int)index);
            if (ImGui::BeginTabItem(name.c_str()))
            {
                ImGui::PushID(index);
                ImGui::Text(name.c_str());
                ImGui::SameLine();
                if (ImGui::Button("remove"))
                {
                    remove_index = int(index - 1);
                }

                refresh |= ImGui::DragInt("offsetx", &shadow.offsetx);
                refresh |= ImGui::DragInt("offsety", &shadow.offsety);
                refresh |= ImGui::DragFloat("blur_radius", &shadow.blur_radius);
                refresh |= imguiColor("color", &shadow.color);
                refresh |= imguiBlendMode("blend_mode", &shadow.blend_mode);
                //refresh |= imguiTextEffect("effect", shadow.effect);

                ImGui::PopID();
                ImGui::EndTabItem();
            }
        }

        if (remove_index != -1)
        {
            shadows.erase(shadows.begin() + remove_index);
            refresh = true;
        }

        ImGui::EndTabBar();
    }
    ImGui::PopID();
    return refresh;
}
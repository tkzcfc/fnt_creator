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

    // ��ȡ�������ݵ�����ϵ��
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

        // ��Ʒֱ���
        glm::vec2 designResolutionSize = m_screenSize;
#if 0
        // ģ����Ʒֱ��ʺ�ʵ����Ļ��һ����С
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
    m_gen = std::make_unique<FntGen>();
    m_gen->setEditorMode(true);

    do
    {
        m_img_shader = std::make_unique<Shader>();
        if (!m_img_shader->init(vertexShaderCode, fragmentShaderCode))
            break;

        m_texture = std::make_unique<Texture>();
        m_texture->initWithFile("loading.png");

        float vertices[] = {
            // λ�� (aPos)   // �������� (aTexCoord)
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

        // �󶨲����� VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        // �󶨲����� EBO�������������
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        //glVertexAttribPointer��
        //    ��һ����������ɫ���� layout(location = n) ��λ�á�
        //    �ڶ���������ÿ�����Ե����ݸ�����vec2 �� 2����
        //    �������������������ͣ�GL_FLOAT����
        //    ���ĸ��������Ƿ���Ҫ��һ���������� GL_FALSE����
        //    �����������������ÿ��������ֽڴ�С����
        //    ����������������ƫ�������ڶ������������е���ʼλ�ã���
        // a_position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // a_texCoord
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        m_VAO = VAO;
        m_VBO = VBO;

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
    fntRender();

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
        
        // ����������Ԫ 0����������
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture->texture_id());
        m_img_shader->setInt("u_tex0", 0);

        // uv y��ת��
        float updatedVertices[] = {
            0.0f,  h,  0.0f, 0.0f,  // ����
            0.0f, 0.0f,  0.0f, 1.0f,  // ����
            w, 0.0f,  1.0f, 1.0f,  // ����
            w, h,  1.0f, 0.0f   // ����
        };
        // �� VBO ����������
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
        refresh |= ImGui::Checkbox("use_gpu", &m_config.use_gpu);
        refresh |= ImGui::Checkbox("is_draw_debug", &m_config.is_draw_debug);

        char szbuf[2048] = { 0 };
        strcpy_s(szbuf, m_config.output_file.c_str());
        if (ImGui::InputText("output_file", szbuf, sizeof(szbuf)))
            m_config.output_file = szbuf;


        if (ImGui::TreeNode("spacing"))
        {
            refresh |= ImGui::InputInt("spacing_horiz", &m_config.spacing_horiz);
            refresh |= ImGui::InputInt("spacing_vert", &m_config.spacing_vert);
            refresh |= ImGui::InputInt("spacing_glyph_x", &m_config.spacing_glyph_x);
            refresh |= ImGui::InputInt("spacing_glyph_y", &m_config.spacing_glyph_y);

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("glyph"))
        {
            refresh |= ImGui::InputInt("glyph_padding_xadvance", &m_config.glyph_padding_xadvance);
            refresh |= ImGui::InputInt("glyph_padding_up", &m_config.glyph_padding_up);
            refresh |= ImGui::InputInt("glyph_padding_down", &m_config.glyph_padding_down);
            refresh |= ImGui::InputInt("glyph_padding_left", &m_config.glyph_padding_left);
            refresh |= ImGui::InputInt("glyph_padding_right", &m_config.glyph_padding_right);

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("texture"))
        {
            refresh |= ImGui::InputInt("padding_up", &m_config.padding_up);
            refresh |= ImGui::InputInt("padding_down", &m_config.padding_down);
            refresh |= ImGui::InputInt("padding_left", &m_config.padding_left);
            refresh |= ImGui::InputInt("padding_right", &m_config.padding_right);
            refresh |= ImGui::InputInt("max_width", &m_config.max_width);

            refresh |= ImGui::Checkbox("is_NPOT", &m_config.is_NPOT);
            refresh |= ImGui::Checkbox("is_fully_wrapped_mode", &m_config.is_fully_wrapped_mode);

            ImGui::TreePop();
        }

        ImGui::End();
    }

    if(ImGui::Begin("Text Style"))
    {
        auto& style = m_config.text_style;

        refresh |= ImGui::InputInt("font_size", &style.font_size);
        refresh |= imguiColor("color", &style.color);
        refresh |= imguiBlendMode("blend_mode", &style.blend_mode);

        refresh |= imguiColor("background_color", &style.background_color);
        refresh |= ImGui::Checkbox("is_bold", &style.is_bold);
        refresh |= ImGui::Checkbox("is_italic", &style.is_italic);
        refresh |= ImGui::InputInt("outline_thickness", &style.outline_thickness);
        refresh |= ImGui::DragFloat("outline_thickness_render_scale", &style.outline_thickness_render_scale, 0.05, 0.5, 3.0f);
        refresh |= imguiColor("outline_color", &style.outline_color);
        refresh |= imguiBlendMode("outline_blend_mode", &style.outline_blend_mode);

        //blend_mode



        //refresh |= ImGui::InputInt("padding_down", &m_config.padding_down);
        //refresh |= ImGui::InputInt("padding_left", &m_config.padding_left);
        //refresh |= ImGui::InputInt("padding_right", &m_config.padding_right);
        //refresh |= ImGui::InputInt("max_width", &m_config.max_width);

        //refresh |= ImGui::Checkbox("is_NPOT", &m_config.is_NPOT);
        //refresh |= ImGui::Checkbox("is_fully_wrapped_mode", &m_config.is_fully_wrapped_mode);

        ImGui::End();
    }

    if (refresh)
    {
        m_needReRender = true;
    }

    ImGui::ShowDemoWindow();
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

bool Editor::imguiTextEffect(TextEffect& effect)
{
    const char* arrEffectType[] = { "none", "linear_gradient" };

    int index = 0;
    if (effect.effect_type == "linear_gradient")
    {
        index = 1;
    }

    bool refresh = false;

    if (ImGui::Combo("linear_gradient", &index, arrEffectType, IM_ARRAYSIZE(arrEffectType)))
    {
        effect.effect_type = arrEffectType[index];
        refresh = true;
    }

    if (index == 1)
    {
        ImGui::InputFloat2("begin", &effect.linear_gradient.begin.x);
        ImGui::InputFloat2("end", &effect.linear_gradient.end.x);
    }

    return refresh;
}
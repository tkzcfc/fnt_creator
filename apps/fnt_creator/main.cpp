#include <fstream>
#include <sstream>
#include <iostream>
#include "FntGen.h"
#include "Editor.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

const char* templateConfigStr = R"(
{
    "output_file": "out.fnt",

    "spacing_horiz": 0,
    "spacing_vert": 0,

    "spacing_glyph_x": 1,
    "spacing_glyph_y": 1,

    "glyph_padding_xadvance": 10,
    "glyph_padding_left": 20,
    "glyph_padding_right": 20,
    "glyph_padding_up": 20,
    "glyph_padding_down": 20,

    "padding_left": 0,
    "padding_right": 0,
    "padding_up": 0,
    "padding_down": 0,
    "is_NPOT": false,
    "is_fully_wrapped_mode": true,
    "max_width": 2048,

    "is_draw_debug": false,

    "text_style": {
        "font_size": 40,
        "color": "#FFFFFFFF",
        "background_color": "",
        "is_italic": false,
        "outline_thickness": 5,
        "outline_thickness_render_scale": 2,
        "outline_color": "#ffffffff",
        "outline_effect": {
            "effect_type": "linear_gradient",
            "linear_gradient": {
                "begin": {"x":0, "y": 0},
                "end": {"x": 1, "y": 1},
                "colors": ["#8F1FFFFF", "#FF00FFFF"],
                "pos": [0, 1]
            }
        },
        "outline_shadows": [
            { "offsetx": 0 , "offsety": 0 , "blur_radius": 5,"color": "#FF00FFFF" },
            { "offsetx": 0 , "offsety": 0 , "blur_radius": 10,"color": "#FF00FFFF" },
            { "offsetx": 0 , "offsety": 0 , "blur_radius": 15,"color": "#FF00FFFF" },

            { "offsetx": 0 , "offsety": 0 , "blur_radius": 5,"color": "#FF00FFFF" },
            { "offsetx": 0 , "offsety": 0 , "blur_radius": 10,"color": "#FF00FFFF" },
            { "offsetx": 0 , "offsety": 0 , "blur_radius": 15,"color": "#FF00FFFF" },

            { "offsetx": 0 , "offsety": 0 , "blur_radius": 5,"color": "#FF00FFFF" },
            { "offsetx": 0 , "offsety": 0 , "blur_radius": 10,"color": "#FF00FFFF" },
            { "offsetx": 0 , "offsety": 0 , "blur_radius": 15,"color": "#FF00FFFF" }
        ]
    },

    "pages": [
        {
            "text": "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=  ",
            "fonts": [
            ]
        }
    ]
}
)";

GenerateConfig readConfig(const std::string& filename)
{
    GenerateConfig config;
    try
    {
        if (filename.empty())
            ajson::load_from_buff(config, templateConfigStr);
        else
            ajson::load_from_file(config, filename.c_str());


        //std::ifstream cfgFile;
        //cfgFile.open(filename);
        //if (!cfgFile.is_open())
        //{
        //    std::cerr << "open config file failed";
        //    std::exit(-1);
        //}

        //std::stringstream buffer;
        //buffer << cfgFile.rdbuf();
        //cfgFile.close();

        //printf(">>>>>>>>>>>>>>>>>>>>>>>\n%s\n\n", buffer.str().c_str());
    }
    catch (const std::exception& e)
    {
        std::cerr << "load json exception: " << e.what();
        std::exit(-1);
    }

    if (config.max_width <= 0)
        config.max_width = 2048;

    config.spacing_glyph_x = std::max(config.spacing_glyph_x, 0);
    config.spacing_glyph_y = std::max(config.spacing_glyph_y, 0);

    config.padding_left = std::max(config.padding_left, 0);
    config.padding_right = std::max(config.padding_right, 0);
    config.padding_up = std::max(config.padding_up, 0);
    config.padding_down = std::max(config.padding_down, 0);

    // 描边宽度
    config.text_style.outline_thickness = std::max(config.text_style.outline_thickness, 0);
    if (config.text_style.background_color.empty())
        config.text_style.background_color = "#FFFFFF00";

    if (config.text_style.outline_thickness_render_scale < 0.001f)
        config.text_style.outline_thickness_render_scale = 2.0f;

    return config;
}

int main(int argc, char* const argv[]) 
{
    bool showGUI = false;
    std::string configFileName = "config.json";
    if (argc > 1)
    {
        configFileName = argv[1];
        if (argc > 2 && strcmp(argv[2], "--gui") == 0)
            showGUI = true;
    }
    else
    {
        showGUI = true;
    }

    auto config = readConfig(configFileName);
    if (showGUI)
    {
        Editor editor;
        return editor.run(config);
    }
    else
    {
        glfwInit();    
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        GLFWwindow* window = glfwCreateWindow(1, 1, "", nullptr, nullptr);
        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "Failed to initialize GLAD" << std::endl;
        }

        bool ok = false;
        {
            FntGen gen;
            ok = gen.run(config);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}
#pragma once

#include <memory>
#include "Texture.h"
#include "Shader.h"
#include "Common.h"
#include "FntGen.h"
#include "async++.h"

class Editor
{
public:
	Editor();

	~Editor();

	int run(const GenerateConfig& config);

private:

	bool init();

	void imguiDraw();

	void fntRender();

	void openglDraw();

	bool imguiColor(const char* label, std::string* color);

	bool imguiBlendMode(const char* label, std::string* model);

	bool imguiTextEffect(TextEffect& effect);

private:

	bool m_needReRender;
	bool m_fntRenderBusy;
	async::task<bool> m_fntRenderTask;

	GenerateConfig m_config;
	std::unique_ptr<FntGen> m_gen;

	// ����ʵ�ʿ��(ʵ�ʷֱ���)
	glm::vec2 m_screenSize;
	// �߼����(��Ʒֱ���)
	glm::vec2 m_winSize;

	std::unique_ptr<Shader> m_img_shader;
	std::unique_ptr<Texture> m_texture;
	GLuint m_VBO;
	GLuint m_VAO;
};


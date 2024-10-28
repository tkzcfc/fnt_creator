#pragma once

#include <glad/glad.h>
#include "stb_image.h"
#include <iostream>

class Texture
{
public:

	Texture()
		: m_texture_id(0)
        , m_width(0)
        , m_height(0)
        , m_premultipliedAlpha(false)
	{}

    ~Texture()
    {
        if(m_texture_id != 0)
            glDeleteTextures(1, &m_texture_id);
    }

	bool initWithFile(const char* path)
	{
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data) 
        {
            auto ok = initWithData(width, height, data, nrChannels == 4);
            stbi_image_free(data);
            return ok;
        }
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
	}

    bool initWithData(int width, int height, unsigned char* data, bool hasAlpha)
    {
        m_width = width;
        m_height = height;

        if (m_texture_id == 0)
        {
            glGenTextures(1, &m_texture_id);
            glBindTexture(GL_TEXTURE_2D, m_texture_id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        GLenum format = hasAlpha ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        return true;
    }

    bool initWithTextureId(GLuint texture, int width, int height)
    {
        if (texture != m_texture_id && texture != 0 && width > 0 && height > 0)
        {
            glDeleteTextures(1, &m_texture_id);
            m_texture_id = texture;
            m_width = width;
            m_height = height;
            return true;
        }
        return false;
    }

    bool hasPremultipliedAlpha() const { return m_premultipliedAlpha; }
    void setPremultipliedAlpha(bool premultipliedAlpha) { m_premultipliedAlpha = premultipliedAlpha; }

    GLuint texture_id() { return m_texture_id; }
    int width() { return m_width; }
    int height() { return m_height; }

private:
    bool m_premultipliedAlpha;
    int m_width;
    int m_height;
	GLuint m_texture_id;
};

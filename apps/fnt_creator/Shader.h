#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

#include "glm.hpp"
#include "ext.hpp"


class Shader
{
public:
    GLuint ID;

    Shader()
    {
        ID = 0;
    }

    bool initWithFile(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
            return false;
        }
        
        return init(vertexCode.c_str(), fragmentCode.c_str());
    }

    bool init(const char* vShaderCode, const char* fShaderCode)
    {
        unsigned int vertex, fragment;

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        if (!checkCompileErrors(vertex, "VERTEX"))
            return false;

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        if (!checkCompileErrors(fragment, "FRAGMENT"))
            return false;

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        if (!checkCompileErrors(ID, "PROGRAM"))
            return false;

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        return true;
    }

    ~Shader()
    {
        if(ID != 0)
            glDeleteProgram(ID);
    }

    void use()
    {
        glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value)
    {
        glUniform1i(getUniformLocationCache(name), (int)value);
    }

    void setInt(const std::string& name, int value)
    {
        glUniform1i(getUniformLocationCache(name), value);
    }

    void setFloat(const std::string& name, float value)
    {
        glUniform1f(getUniformLocationCache(name), value);
    }

    void setMat4(const std::string& name, glm::mat4 value)
    {
        glUniformMatrix4fv(getUniformLocationCache(name), 1, GL_FALSE, glm::value_ptr(value));
    }
    void setVec2(const std::string& name, glm::vec2 vec2)
    {
        glUniform2f(getUniformLocationCache(name), vec2.x, vec2.y);
    }

    void setVec3(const std::string& name, float f0, float f1, float f2)
    {
        glUniform3f(getUniformLocationCache(name), f0, f1, f2);
    }
    void setVec3(const std::string& name, glm::vec3 vec3)
    {
        glUniform3f(getUniformLocationCache(name), vec3.x, vec3.y, vec3.z);
    }

private:

    bool checkCompileErrors(unsigned int shader, std::string type)
    {
        int success = 0;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        return success != 0;
    }

    GLint getUniformLocationCache(const std::string& name)
    {
        auto it = m_uniformLocationCache.find(name);
        if (it == m_uniformLocationCache.end())
        {
            auto location = glGetUniformLocation(ID, name.c_str());
            m_uniformLocationCache[name] = location;
            return location;
        }
        return it->second;
    }

private:

    std::unordered_map<std::string, GLint> m_uniformLocationCache;
};

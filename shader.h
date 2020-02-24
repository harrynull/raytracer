#pragma once
#include <GL/glew.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>

// copy-pasted from NEWorld
class Shader {
public:
    explicit Shader(GLenum stage) : mShader(glCreateShader(stage)) {}

    void compile(const std::string& text) {
        auto array = text.c_str();
        glShaderSource(mShader, 1, &array, nullptr);
        glCompileShader(mShader);
        int status{};
        glGetShaderiv(mShader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            int logLen = 0;
            glGetShaderiv(mShader, GL_INFO_LOG_LENGTH, &logLen);
            std::string log(static_cast<size_t>(logLen + 1), '\0');
            glGetShaderInfoLog(mShader, logLen + 1, &logLen, log.data());
            std::cerr << "Could not compile shader, error " << status << ": " << log;
            throw std::runtime_error(log);
        }
    }

    ~Shader() {
        if (mShader) {
            glDeleteShader(mShader);
        }
    }

    [[nodiscard]] GLuint native() const noexcept { return mShader; }
private:
    GLuint mShader{};
};

inline std::string loadFile(std::string path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

inline GLuint createProgram(Shader& vert, Shader& frag)
{
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, vert.native());
    glAttachShader(ProgramID, frag.native());
    glLinkProgram(ProgramID);
    return ProgramID;
}

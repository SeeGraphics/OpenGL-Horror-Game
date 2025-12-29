#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
    unsigned int ID;

    // Constructor
    Shader(const char* vertexPath, const char* fragmentPath);

    // Activate the shader
    void use();

    // Uniform utility functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    // Utility function for checking shader compilation/linking errors
    void checkCompileErrors(unsigned int shader, const std::string& type);
};

#endif 

#include "GLShader.h"
#ifdef _WIN32
#include <GL/glew.h>
#include <GL/wglew.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#endif

#include <fstream>
// <iostream> is no longer needed here as debug outputs are removed

bool GLShader::LoadVertexShader(const char* filename)
{
	// 1. Load the file content
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	fin.seekg(0, std::ios::end);
	uint32_t length = (uint32_t)fin.tellg();
	fin.seekg(0, std::ios::beg);
	char* buffer = new char[length + 1];
	buffer[length] = '\0';
	fin.read(buffer, length);
	fin.close();
	
	// 2. Create the shader object
	m_VertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_VertexShader, 1, &buffer, nullptr);
	// 3. Compile the shader
	glCompileShader(m_VertexShader);
	// 4. Clean up buffer
	delete[] buffer;
	
	// 5. Check compilation status
    GLint compiled;
    glGetShaderiv(m_VertexShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        glDeleteShader(m_VertexShader); // Delete invalid shader
        m_VertexShader = 0; // Set to 0 to indicate failure
        return false;
    }
	return true;
}

bool GLShader::LoadGeometryShader(const char* filename)
{
	// 1. Load the file content
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	fin.seekg(0, std::ios::end);
	uint32_t length = (uint32_t)fin.tellg();
	fin.seekg(0, std::ios::beg);
	char* buffer = new char[length + 1];
	buffer[length] = '\0';
	fin.read(buffer, length);
	fin.close();

	// 2. Create the shader object
	m_GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(m_GeometryShader, 1, &buffer, nullptr);
	// 3. Compile the shader
	glCompileShader(m_GeometryShader);
	// 4. Clean up buffer
	delete[] buffer;

    // 5. Check compilation status
    GLint compiled;
    glGetShaderiv(m_GeometryShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        glDeleteShader(m_GeometryShader); // Delete invalid shader
        m_GeometryShader = 0; // Set to 0 to indicate failure
        return false;
    }
	return true;
}

bool GLShader::LoadFragmentShader(const char* filename)
{
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	fin.seekg(0, std::ios::end);
	uint32_t length = (uint32_t)fin.tellg();
	fin.seekg(0, std::ios::beg);
	char* buffer = new char[length + 1];
	buffer[length] = '\0';
	fin.read(buffer, length);
	fin.close();
	
	// 2. Create the shader object
	m_FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_FragmentShader, 1, &buffer, nullptr);
	// 3. Compile the shader
	glCompileShader(m_FragmentShader);
	// 4. Clean up buffer
	delete[] buffer;
	
	// 5. Check compilation status
    GLint compiled;
    glGetShaderiv(m_FragmentShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        glDeleteShader(m_FragmentShader); // Delete invalid shader
        m_FragmentShader = 0; // Set to 0 to indicate failure
        return false;
    }
	return true;
}

bool GLShader::Create()
{
	m_Program = glCreateProgram();
	glAttachShader(m_Program, m_VertexShader);
	if (m_GeometryShader) // Check if geometry shader was loaded
		glAttachShader(m_Program, m_GeometryShader);
	glAttachShader(m_Program, m_FragmentShader);
	glLinkProgram(m_Program);

	// Check program linking status
	GLint linked = 0;
	glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		glDeleteProgram(m_Program); // Delete the invalid program
		m_Program = 0; // Set to 0 to indicate failure
		return false;
	}

    // Clean up individual shaders after successful linking
    // They are now part of the program and no longer needed independently
    glDetachShader(m_Program, m_VertexShader);
    glDeleteShader(m_VertexShader);
    m_VertexShader = 0; // Reset to 0 after deletion

    if (m_GeometryShader) { // Only if geometry shader was attached
        glDetachShader(m_Program, m_GeometryShader);
        glDeleteShader(m_GeometryShader);
        m_GeometryShader = 0; // Reset to 0 after deletion
    }

    glDetachShader(m_Program, m_FragmentShader);
    glDeleteShader(m_FragmentShader);
    m_FragmentShader = 0; // Reset to 0 after deletion

	return true;
}

void GLShader::Destroy()
{
    // Only delete the program if it's valid (not 0)
    if (m_Program != 0) {
	    glDeleteProgram(m_Program);
    }
    // Individual shaders should have been deleted by Create() on success,
    // or by LoadShader functions on compilation failure.
    // Resetting IDs to 0 for safety.
    m_Program = 0;
    m_VertexShader = 0;
    m_GeometryShader = 0;
    m_FragmentShader = 0;
}

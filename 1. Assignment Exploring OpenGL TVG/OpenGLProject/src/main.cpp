#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

glm::vec3 cameraPos(0.0f, 1.0f, 5.0f);
float rotationSpeed = 0.5f;
float lookCam[3] = {0.0f,0.0f,0.0f};
std::string textLoc[4] = {
    "../models/mtl_chr1032_00_face_diffuse.png",
    "../models/mtl_bdy1032_00_0_diffuse.png",
    "../models/mtl_chr1032_00_eye_diffuse.png",
    "../models/mtl_chr1032_00_hair_diffuse.png"
};

// Vertex structure
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

// Material structure
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

// Mesh structure
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;
};

// Window dimensions
const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
std::string readShaderFile(const std::string& filePath);
GLuint compileShader(const std::string& source, GLenum shaderType);
GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);

bool loadOBJ(const std::string& filePath, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << filePath << std::endl;
        return false;
    }
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        }
        else if (type == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(glm::normalize(glm::vec3(x, y, z)));
        }
        else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            texCoords.push_back(glm::vec2(u, v));
        }
        else if (type == "f") {
            struct FaceVertex {
                int posIndex;
                int texIndex;
                int normIndex;
            };

            std::vector<FaceVertex> faceData;
            std::string vertex;
            
            while (iss >> vertex) {
                int posIndex = 0;
                int texIndex = 0;
                int normIndex = 0;

                sscanf(vertex.c_str(), "%d/%d/%d", &posIndex, &texIndex, &normIndex);

                posIndex--;
                texIndex--;
                normIndex--;

                faceData.push_back({posIndex, texIndex, normIndex});
            }
            
            // Triangulate quads and handle polygons
            for (size_t i = 1; i < faceData.size() - 1; i++) {
                for (int j = 0; j < 3; j++) {
                    int idx = (j == 0) ? 0 : (j == 1) ? i : i + 1;
                    int posIdx = faceData[idx].posIndex;
                    int texIdx = faceData[idx].texIndex;
                    int normIdx = faceData[idx].normIndex;
                    
                    if (posIdx >= 0 && posIdx < (int)positions.size()) {
                        Vertex v;
                        v.position = positions[posIdx];
                        v.normal = (normIdx >= 0 && normIdx < (int)normals.size()) 
                            ? normals[normIdx] 
                            : glm::vec3(0.0f, 1.0f, 0.0f);
                        v.texCoords = (texIdx >= 0 && texIdx < texCoords.size())
                            ? texCoords[texIdx]
                            : glm::vec2(0.0f);
                        
                        vertices.push_back(v);
                        indices.push_back(indices.size());
                    }
                }
            }
        }
    }

    file.close();
    std::cout << "Loaded OBJ: " << vertices.size() << " vertices, " << indices.size() << " indices" << std::endl;
    return true;
}

void setupMesh(Mesh& mesh)
{
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), &mesh.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, texCoords)
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

GLuint loadTexture(const char* path)
{
    GLuint textureID;

    glGenTextures(1, &textureID);

    int width, height, nrChannels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = GL_RGB;

        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            width,
            height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            data
        );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        std::cout << "Texture loaded successfully!\n";
    }
    else
    {
        std::cout << "Failed to load texture!\n";
    }

    stbi_image_free(data);

    return textureID;
}

int main()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL - Exploring Shaders", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // Main loop
    // Load shader program
    GLuint shaderProgram = createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");

    // Load OBJ model
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    if (!loadOBJ("../models/model.obj", vertices, indices)) {
        std::cerr << "Failed to load model" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Create mesh
    Mesh mesh;
    mesh.vertices = vertices;
    mesh.indices = indices;
    setupMesh(mesh);

    std::string textLoc[4] = {
        "../models/mtl_chr1032_00_face_diffuse.png",
        "../models/mtl_bdy1032_00_0_diffuse.png",
        "../models/mtl_chr1032_00_eye_diffuse.png",
        "../models/mtl_chr1032_00_hair_diffuse.png"
    };

    GLuint textures[4];

    glGenTextures(4, textures);

    for (int i = 0; i < 4; i++)
    {
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_set_flip_vertically_on_load(true);

        int texWidth, texHeight, texChannels;

        unsigned char* data = stbi_load(
            textLoc[i].c_str(),
            &texWidth,
            &texHeight,
            &texChannels,
            0
        );

        if (data)
        {
            GLenum format = GL_RGB;

            if (texChannels == 4)
                format = GL_RGBA;

            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                format,
                texWidth,
                texHeight,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data
            );

            glGenerateMipmap(GL_TEXTURE_2D);

            std::cout << "Loaded texture: " << textLoc[i] << std::endl;
        }
        else
        {
            std::cout << "Failed texture: " << textLoc[i] << std::endl;
        }

        stbi_image_free(data);
    }

    // Light position
    glm::vec3 lightPos(2.0f, 3.0f, 4.0f);
    glm::vec3 viewPos(5.0f, 1.0f, 1.0f);

    // Material properties
    Material material;
    material.ambient = glm::vec3(0.3f, 0.3f, 0.3f);    // Stronger ambient
    material.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);    // Full white diffuse
    material.specular = glm::vec3(0.5f, 0.5f, 0.5f);   // Reduced specular
    material.shininess = 16.0f;     

    // Main rendering loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Set up matrices
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(15.0f));
        model = glm::rotate(model, glm::radians(00.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // Rotate on X axis
        model = glm::rotate(
            model,
            (float)glfwGetTime() * rotationSpeed,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 view = glm::lookAt(
            cameraPos,                          // Camera position
            glm::vec3(lookCam[0], lookCam[1], lookCam[2]),    // Look at origin (where model is)
            glm::vec3(2.0f, 0.0f, 2.0f)     // Up vector
        );

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

        // Pass matrices to shader
        GLuint modelLoc = glGetUniformLocation(shaderProgram, "uModel");
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "uView");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "uProjection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Pass material properties to shader
        GLuint ambientLoc = glGetUniformLocation(shaderProgram, "uAmbient");
        GLuint diffuseLoc = glGetUniformLocation(shaderProgram, "uDiffuse");
        GLuint specularLoc = glGetUniformLocation(shaderProgram, "uSpecular");
        GLuint shininessLoc = glGetUniformLocation(shaderProgram, "uShininess");
        GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "uObjectColor");

        glUniform3f(ambientLoc, material.ambient.x, material.ambient.y, material.ambient.z);
        glUniform3f(diffuseLoc, material.diffuse.x, material.diffuse.y, material.diffuse.z);
        glUniform3f(specularLoc, material.specular.x, material.specular.y, material.specular.z);
        glUniform1f(shininessLoc, material.shininess);
        glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);

        // Pass lighting uniforms
        GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "uLightPos");
        GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");

        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(viewPosLoc, viewPos.x, viewPos.y, viewPos.z);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);

        GLuint textureLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(textureLoc, 0);

        // Render mesh
        glBindVertexArray(mesh.VAO);
        glActiveTexture(GL_TEXTURE0);

        int currentTexture = 0;

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
            currentTexture = 0;

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
            currentTexture = 1;

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
            currentTexture = 2;

        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
            currentTexture = 3;

        glBindTexture(GL_TEXTURE_2D, textures[currentTexture]);

        glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &mesh.VBO);
    glDeleteBuffers(1, &mesh.EBO);
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    extern glm::vec3 cameraPos;
    extern float rotationSpeed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 0.01f;

    // Camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos.z -= speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos.z += speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos.x -= speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos.x += speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos.y -= speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos.y += speed;

    // Camera movement
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        lookCam[0] -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        lookCam[0] += 0.01f;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        lookCam[1] -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        lookCam[1] += 0.01f;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        lookCam[2] -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        lookCam[2] += 0.01f;

    // Rotation controls
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        rotationSpeed = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        rotationSpeed = 2.0f;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        rotationSpeed = 0.2f;
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
        rotationSpeed = 0.5f;
}

std::string readShaderFile(const std::string& filePath)
{
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }
    catch (std::ifstream::failure e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << filePath << std::endl;
        return "";
    }
}

GLuint compileShader(const std::string& source, GLenum shaderType)
{
    GLuint shader = glCreateShader(shaderType);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);

    GLuint vertexShader = compileShader(vertexCode, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}
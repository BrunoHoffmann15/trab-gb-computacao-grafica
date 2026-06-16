#ifndef ENV_H
#define ENV_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

using namespace std;

class Mesh {
    public:
        // OpenGL buffers
        GLuint VAO;
        GLuint VBO;
        GLuint texID;
        int nVertices;

        // Transformações
        glm::vec3 position; // Posição do mesh no espaço
        glm::vec3 rotation; // Rotação do mesh em torno dos eixos X, Y e Z (em graus)
        glm::vec3 scale;  // Escala do mesh em relação ao ponto de origem 

        // Cor do objeto
        glm::vec3 color;

        // Luz
        glm::vec3 ka; // Coeficiente de reflexão ambiente
        glm::vec3 kd; // Coeficiente de reflexão difusa
        glm::vec3 ks; // Coeficiente de reflexão especular
        float q; // Exponente de brilho para reflexão especular

        // Textura
        string texturePath; // Caminho para a textura do mesh

        bool isAnimated = false; // Novo atributo na classe Mesh

        // Carrega um arquivo OBJ simples (sem texturas, apenas vértices, normais e cores) e preenche os buffers do mesh
        void loadObj(string filePath);

        // Carrega um arquivo .MTL para obter as informações de material (ka, kd, ks, q) e cor do mesh
        void loadMtl(string filePath);

        // Carrega texturas
        void loadTexture();

        // Construtor para inicializar os atributos do mesh
        Mesh(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
            this->position = position;
            this->rotation = rotation;
            this->scale = scale;
        };

};

class Light {
    public:
        glm::vec3 position;
        glm::vec3 color;

};

struct CameraConfig {
    glm::vec3 position;
    float yaw;
    float pitch;
    float fov;
};

class Env {
public:
    // Vetor com os meshes a serem renderizados
    vector<Mesh> meshes;

    // Luz
    Light light;

    // Carregar o ambiente a partir de um arquivo JSON
    void loadEnvironment(string envPath);

    void loadModelWithAssimp(string objPath, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, bool isAnimated);

    CameraConfig cameraConfig;
};

#endif
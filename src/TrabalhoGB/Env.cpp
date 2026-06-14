#include "Env.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <json.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

using json = nlohmann::json;

void Mesh::loadTexture()
{
    // Carregar a imagem usando stb_image
    int nrChannels, imgWidth, imgHeight;
    unsigned char* data = stbi_load(this->texturePath.c_str(), &imgWidth, &imgHeight, &nrChannels, 0);
    if (!data) {
        throw runtime_error("Failed to load texture: " + this->texturePath);
    }

    // Gerar e configurar a textura OpenGL
    glGenTextures(1, &this->texID);
    glBindTexture(GL_TEXTURE_2D, this->texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determinar o formato da textura com base no número de canais
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    // Carregar os dados da textura para a GPU
    glTexImage2D(GL_TEXTURE_2D, 0, format, imgWidth, imgHeight, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Liberar os dados da imagem da memória
    stbi_image_free(data);
}

void Mesh::loadObj(string filePath)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);

    std::ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open()) 
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePath << std::endl;
		exit(EXIT_FAILURE);
	}

    std::string line;
    while (std::getline(arqEntrada, line)) 
	{
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v") 
		{
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } 
        else if (word == "vt") 
		{
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } 
        else if (word == "vn") 
		{
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } 
        else if (word == "f")
		 {
            while (ssline >> word) 
			{
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                
                vBuffer.push_back(color.r); // Cor R
                vBuffer.push_back(color.g); // Cor G
                vBuffer.push_back(color.b); // Cor B
                
                // Trava de segurança para texturas
                if (!texCoords.empty()) {
                    vBuffer.push_back(texCoords[ti].s); 
                    vBuffer.push_back(texCoords[ti].t); 
                } else {
                    vBuffer.push_back(0.0f); 
                    vBuffer.push_back(0.0f); 
                }

                vBuffer.push_back(normals[ni].x);   // Normal X
                vBuffer.push_back(normals[ni].y);   // Normal Y
                vBuffer.push_back(normals[ni].z);   // Normal Z
            }
        }
    }

    arqEntrada.close();

    std::cout << "Gerando o buffer de geometria..." << std::endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Única declaração de Stride (11 floats agora)
    GLsizei stride = 11 * sizeof(GLfloat);

    // 0: Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // 1: Cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // 2: Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // 3: Coordenada de Textura
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	this->VAO = VAO;
	this->VBO = VBO;
	this->nVertices = vBuffer.size() / 11;
}

void Mesh::loadMtl(string filePath)
{
    std::ifstream arqEntrada(filePath.c_str());

    if (!arqEntrada.is_open()) 
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePath << std::endl;
		exit(EXIT_FAILURE);
	}

    std::string line;
    while (std::getline(arqEntrada, line)) 
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "Ka") 
        {
            ssline >> this->ka.r >> this->ka.g >> this->ka.b;
        } 
        else if (word == "Kd") 
        {
            ssline >> this->kd.r >> this->kd.g >> this->kd.b;
        } 
        else if (word == "Ks") 
        {
            ssline >> this->ks.r >> this->ks.g >> this->ks.b;
        } 
        else if (word == "Ns") 
        {
            ssline >> this->q;
        } else if (word == "map_Kd") 
        {
            ssline >> this->texturePath;
        }
    }

    arqEntrada.close();

}

void Env::loadEnvironment(string envPath)
{
    ifstream file(envPath);

    if (!file.is_open())
    {
        throw runtime_error(
            "Nao foi possivel abrir o arquivo: " + envPath
        );
    }

    json envJson;
    file >> envJson;

    meshes.clear();

    // Carrega os meshes
    for (const auto& obj : envJson["objects"])
    {
        string objPath = obj["objPath"];
        string mtlPath = obj["mtlPath"];


        glm::vec3 position = glm::vec3(obj["position"][0], obj["position"][1], obj["position"][2]);
        glm::vec3 rotation = glm::vec3(obj["rotation"][0], obj["rotation"][1], obj["rotation"][2]);
        glm::vec3 scale    = glm::vec3(obj["scale"][0], obj["scale"][1], obj["scale"][2]);

        Mesh mesh(position, rotation, scale);

        mesh.loadObj(objPath);
        mesh.loadMtl(mtlPath);
        mesh.loadTexture();

        meshes.push_back(mesh);
    }

    // Carrega a luz
    const auto& lightJson = envJson["light"];

    light.position = glm::vec3(lightJson["position"][0], lightJson["position"][1], lightJson["position"][2]);
    light.color    = glm::vec3(lightJson["color"][0], lightJson["color"][1], lightJson["color"][2]);
}
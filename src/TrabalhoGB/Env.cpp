#include "Env.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <json.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
        throw runtime_error("Nao foi possivel abrir o arquivo: " + envPath);
    }

    json envJson;
    file >> envJson;

    meshes.clear();

    // Carrega os meshes
    for (const auto& obj : envJson["objects"])
    {
        string objPath = obj["objPath"];

        glm::vec3 position = glm::vec3(obj["position"][0], obj["position"][1], obj["position"][2]);
        glm::vec3 rotation = glm::vec3(obj["rotation"][0], obj["rotation"][1], obj["rotation"][2]);
        glm::vec3 scale    = glm::vec3(obj["scale"][0], obj["scale"][1], obj["scale"][2]);

        bool isAnimated = false;
        if (obj.contains("isAnimated")) {
            isAnimated = obj["isAnimated"];
        }

        loadModelWithAssimp(objPath, position, rotation, scale, isAnimated);
        // ---------------------------------
    }

    // Carrega a luz
    const auto& lightJson = envJson["light"];

    light.position = glm::vec3(lightJson["position"][0], lightJson["position"][1], lightJson["position"][2]);
    light.color    = glm::vec3(lightJson["color"][0], lightJson["color"][1], lightJson["color"][2]);

    // Carrega a câmera
    const auto& camJson = envJson["camera"];
    cameraConfig.position = glm::vec3(camJson["position"][0], camJson["position"][1], camJson["position"][2]);
    cameraConfig.yaw = camJson["yaw"];
    cameraConfig.pitch = camJson["pitch"];
    cameraConfig.fov = camJson["fov"];
}

void Env::loadModelWithAssimp(string objPath, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, bool isAnimated) 
{
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(objPath, 
        aiProcess_Triangulate | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        std::cerr << "ERRO::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    // Descobre o diretório base do arquivo .obj
    // Procura a última barra (funciona para Linux/Mac '/' ou Windows '\')
    std::string directory = objPath.substr(0, objPath.find_last_of("/\\"));

    // O arquivo .obj do Assimp fica guardado na variável 'scene'.
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) 
    {
        aiMesh* aimesh = scene->mMeshes[i];
        
        // Criamos o nosso Mesh da aplicação
        Mesh myMesh(pos, rot, scale);
        
        std::vector<GLfloat> vBuffer;

        myMesh.isAnimated = isAnimated;

        // 1. Extraindo Vértices, Normais e Coordenadas de Textura
        for (unsigned int j = 0; j < aimesh->mNumVertices; j++) 
        {
            // Posição
            vBuffer.push_back(aimesh->mVertices[j].x);
            vBuffer.push_back(aimesh->mVertices[j].y);
            vBuffer.push_back(aimesh->mVertices[j].z);

            // Cor base (branco por padrão)
            vBuffer.push_back(1.0f); 
            vBuffer.push_back(1.0f); 
            vBuffer.push_back(1.0f);

            // Coordenadas de Textura 
            if (aimesh->mTextureCoords[0]) {
                vBuffer.push_back(aimesh->mTextureCoords[0][j].x);
                vBuffer.push_back(aimesh->mTextureCoords[0][j].y);
            } else {
                vBuffer.push_back(0.0f);
                vBuffer.push_back(0.0f);
            }

            // Normais
            vBuffer.push_back(aimesh->mNormals[j].x);
            vBuffer.push_back(aimesh->mNormals[j].y);
            vBuffer.push_back(aimesh->mNormals[j].z);
        }

        // 2. Extraindo os Materiais (ka, kd, ks) específicos DESTE grupo
        if (aimesh->mMaterialIndex >= 0) 
        {
            aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];
            aiColor3D color(0.f, 0.f, 0.f);
            
            // Ambiente (Ka)
            material->Get(AI_MATKEY_COLOR_AMBIENT, color);
            myMesh.ka = glm::vec3(color.r, color.g, color.b);

            // Difuso (Kd)
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            myMesh.kd = glm::vec3(color.r, color.g, color.b);

            // Especular (Ks)
            material->Get(AI_MATKEY_COLOR_SPECULAR, color);
            myMesh.ks = glm::vec3(color.r, color.g, color.b);
            
            // Brilho (Ns / Shininess)
            float shininess;
            material->Get(AI_MATKEY_SHININESS, shininess);
            myMesh.q = shininess > 0.0f ? shininess : 32.0f; // Previne brilho zero

            // Extraindo o caminho da textura difusa (se houver)
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString str;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
                
                // O caminho final é a pasta do OBJ + / + o caminho que está no MTL
                // (O str.C_Str() do Kenney já costuma vir como "textures/nome_da_textura.png")
                myMesh.texturePath = directory + "/" + str.C_Str(); 
                
                std::cout << "Textura carregada em: " << myMesh.texturePath << std::endl;
            }
        }

        // 3. Geração do VAO/VBO 
        GLuint VBO, VAO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
        
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        
        GLsizei stride = 11 * sizeof(GLfloat);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(8 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(3);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        myMesh.VAO = VAO;
        myMesh.VBO = VBO;
        myMesh.nVertices = vBuffer.size() / 11;
        
        // Se houver textura, já manda carregar
        if(!myMesh.texturePath.empty()) {
            myMesh.loadTexture();
        }

        // Adiciona este sub-grupo pronto na lista de renderização do ambiente
        this->meshes.push_back(myMesh);
    }
}
#include "Env.h"
#include <fstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <json.hpp>

using namespace std;

using json = nlohmann::json;

void Mesh::loadObj(string filePath)
{
    // TODO: Adicionar controle do obj aqui.
}

void Mesh::loadMtl(string filePath)
{
    // TODO: Adicionar controle do mtl aqui.
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

        meshes.push_back(mesh);
    }

    // Carrega a luz
    const auto& lightJson = envJson["light"];

    light.position = glm::vec3(lightJson["position"][0], lightJson["position"][1], lightJson["position"][2]);
    light.color    = glm::vec3(lightJson["color"][0], lightJson["color"][1], lightJson["color"][2]);
}
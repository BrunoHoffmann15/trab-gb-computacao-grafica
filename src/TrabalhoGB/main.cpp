/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 * e https://antongerdelan.net/opengl/
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 03/03/2026
 */

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Camera
#include "Camera.h"

// Estrutura Mesh para controle de objeto;
struct Mesh 
{
    GLuint VAO;
	GLuint VBO;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale; 
	int nVertices;
	glm::vec3 color;
};

// Estrutura Light para controle da luz.
struct Light {
	glm::vec3 position;
	glm::vec3 color;
	float ka, kd, ks;
};


// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
Mesh loadSimpleOBJ(string filePATH);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 600, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"glsl(#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoord;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

out vec4 finalColor;
out vec3 fragPos;
out vec3 scaledNormal;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    finalColor = vec4(color, 1.0);
    fragPos = vec3(model * vec4(position, 1.0)); 
    scaledNormal = mat3(transpose(inverse(model))) * normal;
}
)glsl";

// Código fonte do Fragment Shader (em GLSL)
const GLchar* fragmentShaderSource = R"glsl(#version 450
in vec4 finalColor;
in vec3 fragPos;
in vec3 scaledNormal;

uniform bool isWireframe;
uniform vec3 wireColor;
uniform vec3 objectColor;

// Propriedades da superfície/material
uniform float ka;
uniform float kd;
uniform float ks, q;

// Propriedades da fonte de luz
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 cameraPos;

out vec4 color;

void main()
{
    // Parcela da luz ambiente
    vec3 ambient = ka * lightColor;

    // Parcela da reflexão difusa
    vec3 N = normalize(scaledNormal);
    vec3 L = normalize(lightPos - fragPos);
    float diff = max(dot(N,L),0.0);
    vec3 diffuse = kd * diff * lightColor;

    //Coeficiente de reflexão especular
    vec3 R = normalize(reflect(-L,N));
    vec3 V = normalize(cameraPos - vec3(fragPos));
    float spec = max(dot(R,V),0.0);
    spec = pow(spec,q);
    vec3 specular = ks * spec * lightColor;

    // Mistura a cor do vértice (branco) com a cor do objeto (C++)
    vec4 actualColor = finalColor * vec4(objectColor, 1.0);

    // Define qual será a cor base (preto se for wireframe, cor real se for sólido)
    vec4 baseColor;
    if (isWireframe) {
        baseColor = vec4(wireColor, 1.0);
    } else {
        baseColor = actualColor;
    }

    // Aplica a iluminação na cor escolhida
    color = (vec4(ambient, 1.0) + vec4(diffuse, 1.0)) * baseColor + vec4(specular, 1.0);
}
)glsl";


// Definindo variáveis globais para controle de transformações.
bool axisX=true, axisY=false, axisZ=false, rotateEnabled = true, scale = false, translade = false, perspective = true;
bool moveLight = false, changeKa = false, changeKd = false, changeKs = false, changeQ = false;
int active_mesh = 0; //mesh selecionado para transformação (0 ou 1)
bool showWireframe = false;

//Instanciação da Camera
Camera camera(glm::vec3(0.0, 0.0, -5.0), glm::vec3(0.0,1.0,0.0),90.0,0.0);
float deltaTime = 0.0;
float lastFrame = 0.0; 


// Posição inicial no centro da tela
float lastX = WIDTH / 2.0f;  // 300.0f
float lastY = HEIGHT / 2.0f; // 300.0f
bool firstMouse = true;

void cameraHandler(GLFWwindow* window, Camera &camera, float deltaTime);

void applyLightChange(GLFWwindow* window, Light &light, bool shouldGoUp);

// Definição da função de renderização dos meshes.
void renderMeshes(std::vector<Mesh> &meshes, GLuint shaderID);

// Definição das funções de transformação.
void applyTransform(Mesh &mesh, bool shouldGoUp);

void rotateMesh(Mesh &mesh, bool clockwise);

void scaleMesh(Mesh &mesh, bool up);

void transladeMesh(Mesh &mesh, bool up);

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho GA - Bel Cogo e Bruno Hoffmann", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);


	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	glUseProgram(shaderID);

    // Matriz de modelo - Transformações nos objetos
	glm::mat4 model = glm::mat4(1); //matriz identidade;
  	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    //-----------------
    // Matriz de projeção
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),(float)WIDTH/(float)HEIGHT,0.1f,100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Matriz de view
    glm::mat4 view = glm::lookAt(glm::vec3(0,0,-3), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
    glEnable(GL_DEPTH_TEST);

	// Definição dos objetos e seus atributos.
	std::vector<Mesh> meshes;

	Mesh m1 = loadSimpleOBJ("../assets/Suzanne.obj");
	m1.position = glm::vec3(-1.0, 0.0, 0.0);
	m1.rotation = glm::vec3(0.0, 180.0, 0.0);
	m1.scale = glm::vec3(0.5, 0.5, 0.5);
	m1.color = glm::vec3(1.0, 0.0, 0.0); // Red

	meshes.push_back(m1);

	Mesh m2 = loadSimpleOBJ("../assets/bunny.obj");
	m2.position = glm::vec3(0.8, 0.0, 0.0);
	m2.rotation = glm::vec3(0.0, 180.0, 0.0);
	m2.scale = glm::vec3(0.7, 0.7, 0.7);
	m2.color = glm::vec3(0.5, 0.5, 1.0); // Violet

	meshes.push_back(m2);

	// Mandando as infos de iluminação para o shader
	float ka = 0.2, kd = 0.5, ks = 0.5, q = 10.0;

	Light light;
	light.position = glm::vec3(-0.5, 1.0, 0.0);
	light.color = glm::vec3(1.0, 1.0, 1.0);
	light.ka = ka;
	light.kd = kd;
	light.ks = ks;

	// Método para captura do movimento do mouse.
	glfwSetCursorPosCallback(window, mouse_callback);

	// Método para travar o cursor no centro da janela e não mostrar o cursor.
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Controle do tempo entre frames
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Essas funções estão depreciadas, só vão funcionar se usarmos o 
        // OpenGL em modo "Compability" -- eu as deixo pra facilitar a visualização
		glLineWidth(1.0);
		glPointSize(1.0);

		// Configuração da projeção.
		if (perspective) {
			projection = glm::perspective(glm::radians(45.0f),(float)WIDTH/(float)HEIGHT,0.1f,100.0f);
		} else {
			projection = glm::ortho(-3.0, 3.0, -3.0, 3.0, 0.1, 100.0);
		}
		
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		// Configuração da movimentação da câmera via teclado (IKJL).
		cameraHandler(window, camera, deltaTime);

		// Configurações das transformação dos objetos e da luz.
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			applyTransform(meshes[active_mesh], true);
			applyLightChange(window, light, true);
		}

		// Configurações das transformação dos objetos e da luz.
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			applyTransform(meshes[active_mesh], false);
			applyLightChange(window, light, false);
		}

		// Mandando a posição da luz para o shader.
		glUniform3f(glGetUniformLocation(shaderID, "lightPos"),light.position.x,light.position.y,light.position.z);
		glUniform3f(glGetUniformLocation(shaderID, "lightColor"),light.color.x,light.color.y,light.color.z);

		// Mandando variáveis de iluminação para o shader.
		glUniform1f(glGetUniformLocation(shaderID, "ka"),light.ka);
		glUniform1f(glGetUniformLocation(shaderID, "kd"),light.kd);
		glUniform1f(glGetUniformLocation(shaderID, "ks"),light.ks);
		glUniform1f(glGetUniformLocation(shaderID, "q"),q);

    
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		
		// Atualização da matriz de view de acordo com as mudanças que ela sofreu via input
		// de mouse e/ou teclado
		view = camera.getViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// Chamada de Desenho - DRAWCALL
		// Primeiro: renderizar solido
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform1i(glGetUniformLocation(shaderID, "isWireframe"), GL_FALSE);
		renderMeshes(meshes, shaderID);

		// 2. Se a opção estiver ativa, desenha as linhas por cima
		if (showWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(-1.0, -1.0); 
			
			glUniform1i(glGetUniformLocation(shaderID, "isWireframe"), GL_TRUE);
			glUniform3f(glGetUniformLocation(shaderID, "wireColor"), 0.0f, 0.0f, 0.0f);
			renderMeshes(meshes, shaderID);
			
			glDisable(GL_POLYGON_OFFSET_LINE);
		}

			// Troca os buffers da tela
			glfwSwapBuffers(window);
	}


	// Desaloca o VAO do buffer.
	for (auto& mesh : meshes) 
	{
		glDeleteVertexArrays(1, &mesh.VAO);
	}

	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

void cameraHandler(GLFWwindow* window, Camera &camera, float deltaTime)
{
	// Configuração da movimentação da câmera via teclado (IKJL).
	if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD",deltaTime);
	if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD",deltaTime);
	if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT",deltaTime);
	if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT",deltaTime);

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.processMouseMovement(xoffset, yoffset, true);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	// Covertendo os valores de posição do mouse para float.
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

	// Configuração da movimentação da câmera via mouse.
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

	// Atualizando a última posição do mouse para o próximo cálculo de offset.
    lastX = xpos;
    lastY = ypos;

    // Processa o movimento do mouse para atualizar a orientação da câmera.
    camera.processMouseMovement(xoffset, yoffset);
}

void applyLightChange(GLFWwindow* window, Light &light, bool shouldGoUp)
{
	float delta = 0.1f * (shouldGoUp ? 1 : -1);

	// Movimenta a luz no sentido positivo ou negativo do eixo selecionado (X, Y ou Z)
	if (moveLight) {
		if (axisX)
			light.position.x += delta;
		if (axisY)
			light.position.y += delta;
		if (axisZ)
			light.position.z += delta;
	}

	// Aumenta ou diminui os coeficientes de iluminação (ka, kd, ks) dependendo do valor de "shouldGoUp"
	if (changeKa) {
		light.ka += delta * 0.1f;
		if (light.ka < 0.0f) light.ka = 0.0f; // Evita valores negativos
	}

	if (changeKd) {
		light.kd += delta * 0.1f;
		if (light.kd < 0.0f) light.kd = 0.0f; // Evita valores negativos
	}

	if (changeKs) {
		light.ks += delta * 0.1f;
		if (light.ks < 0.0f) light.ks = 0.0f; // Evita valores negativos
	}
}

void applyTransform(Mesh &mesh, bool shouldGoUp)
{
	if (rotateEnabled) { // Caso de rotação.
		rotateMesh(mesh, shouldGoUp);
	} else if (scale) { // Caso de escalar.
		scaleMesh(mesh, shouldGoUp);
	} else if (translade) { // Caso de translação.
		transladeMesh(mesh, shouldGoUp);
	}
}

// Translada o mesh no sentido positivo ou negativo do eixo selecionado (X, Y ou Z)
void transladeMesh(Mesh &mesh, bool up)
{
	float delta = 0.01f * (up ? 1 : -1);

	if (axisX)
		mesh.position.x += delta;
	if (axisY)
		mesh.position.y += delta;
	if (axisZ)
		mesh.position.z += delta;
}

// Rotaciona o mesh em torno do eixo selecionado (X, Y ou Z) e no sentido horário ou anti-horário
void rotateMesh(Mesh &mesh, bool clockwise)
{
    float delta = clockwise ? -1.0f : 1.0f;

	if (axisX)
		mesh.rotation.x += delta;
	if (axisY)
		mesh.rotation.y += delta;
	if (axisZ)
		mesh.rotation.z += delta;
}

// Escala o mesh para mais próximo ou mais distante do ponto de origem, dependendo do valor de "up"
void scaleMesh(Mesh &mesh, bool up)
{
	float delta = up ? 0.01f : -0.01f;

	if (axisX)
		mesh.scale.x += delta;
	if (axisY)
		mesh.scale.y += delta;
	if (axisZ)
		mesh.scale.z += delta;
}

// Renderiza os meshes com base no shader e nas transformações aplicadas a ele.
void renderMeshes(std::vector<Mesh> &meshes, GLuint shaderID)
{
	for (auto& mesh : meshes) 
	{
		glm::mat4 modelMesh = glm::mat4(1);
		
		// Aplicar transformações do mesh
		modelMesh = glm::translate(modelMesh, mesh.position);
		modelMesh = glm::rotate(modelMesh, glm::radians(mesh.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		modelMesh = glm::rotate(modelMesh, glm::radians(mesh.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMesh = glm::rotate(modelMesh, glm::radians(mesh.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		modelMesh = glm::scale(modelMesh, mesh.scale);
		
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(modelMesh));
		glUniform3f(glGetUniformLocation(shaderID, "objectColor"), mesh.color.x, mesh.color.y, mesh.color.z);
		
		glBindVertexArray(mesh.VAO);
		glDrawArrays(GL_TRIANGLES, 0, mesh.nVertices);
		glBindVertexArray(0);
	}
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		perspective = !perspective;
	}

	// Modifica a opção a ser realizada com as setas (transformação, movimento da luz, aumento de k, d, ks ou q)
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		scale = true;
		rotateEnabled = translade = moveLight = changeKa = changeKd = changeKs = changeQ= false;
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		rotateEnabled = true;
		translade = scale = moveLight = changeKa = changeKd = changeKs = changeQ = false;
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		scale = moveLight = rotateEnabled = changeKa = changeKd = changeKs = changeQ = false;
		translade = true;
	}
	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
	{
		scale = rotateEnabled = translade = changeKa = changeKd = changeKs = changeQ = false;
		moveLight = true;
	}
	if (key == GLFW_KEY_5 && action == GLFW_PRESS)
	{
		scale = rotateEnabled = translade = moveLight = changeKd = changeKs = changeQ = false;
		changeKa = true;
	}
	if (key == GLFW_KEY_6 && action == GLFW_PRESS)
	{
		scale = rotateEnabled = translade = moveLight = changeKa = changeKs = changeQ = false;
		changeKd = true;
	}
	if (key == GLFW_KEY_7 && action == GLFW_PRESS)
	{
		scale = rotateEnabled = translade = moveLight = changeKa = changeKd = changeQ = false;
		changeKs = true;
	}

	// Modifica os eixos a serem considerados para as transformações e para o movimento da luz.
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		axisX = true;
		axisY = false;
		axisZ = false;
	}
	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		axisX = false;
		axisY = true;
		axisZ = false;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		axisX = false;
		axisY = false;
		axisZ = true;
	}
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		axisX = true;
		axisY = true;
		axisZ = true;
	}

	// Muda o mesh ativo para transformação (tecla N) - só tem 2 meshes, então alterna entre 0 e 1
	if (key == GLFW_KEY_N && action == GLFW_PRESS) {
		active_mesh = (active_mesh + 1) % 2;
	}

	// Ativa ou desativa a visualização em modo wireframe (tecla O).
	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		showWireframe = !showWireframe;
	}
}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Função para carregar um arquivo .obj simples;
// Cria um Mesh a partir do arquivo, gerando o VAO e VBO correspondentes, e retorna esse Mesh para ser renderizado posteriormente.
Mesh loadSimpleOBJ(string filePATH)
 {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) 
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
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

	Mesh mesh;

	mesh.VAO = VAO;
	mesh.VBO = VBO;
	mesh.nVertices = vBuffer.size() / 11;

    return mesh;
}

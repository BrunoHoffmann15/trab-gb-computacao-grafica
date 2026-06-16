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

// Ambiente
#include "Env.h"

// Camera
#include "Camera.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 600, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"glsl(#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

out vec4 finalColor;
out vec3 fragPos;
out vec3 scaledNormal;
out vec2 texcoord;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    finalColor = vec4(color, 1.0);
    fragPos = vec3(model * vec4(position, 1.0)); 
    scaledNormal = mat3(transpose(inverse(model))) * normal;
	texcoord = vec2(texc.s, 1.0 - texc.t);
}
)glsl";

// Código fonte do Fragment Shader (em GLSL)
const GLchar* fragmentShaderSource = R"glsl(#version 450
in vec4 finalColor;
in vec3 fragPos;
in vec3 scaledNormal;
in vec2 texcoord;

uniform sampler2D texBuffer;

uniform bool isWireframe;
uniform vec3 wireColor;
uniform vec3 objectColor;

// Propriedades da superfície/material
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float q;

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
	vec4 texColor = texture(texBuffer, texcoord);

    // Aplica a iluminação na cor escolhida
    color = (vec4(ambient, 1.0) + vec4(diffuse, 1.0)) * texColor + vec4(specular, 1.0);
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

int total_meshes = 1;

void cameraHandler(GLFWwindow* window, Camera &camera, float deltaTime);

void applyLightChange(GLFWwindow* window, Light &light, bool shouldGoUp);

// Definição da função de renderização dos meshes.
void renderMeshes(std::vector<Mesh> &meshes, GLuint shaderID);

// Definição das funções de transformação.
void applyTransform(Mesh &mesh, bool shouldGoUp);

void rotateMesh(Mesh &mesh, bool clockwise);

void scaleMesh(Mesh &mesh, bool up);

void transladeMesh(Mesh &mesh, bool up);

void applyReflectorsChange(GLFWwindow* window, Mesh &mesh, bool shouldGoUp);

glm::vec3 calculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 p = uuu * p0; 
    p += 3 * uu * t * p1;   
    p += 3 * u * tt * p2;   
    p += ttt * p3;          
    return p;
}

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho GB - Bel Cogo e Bruno Hoffmann", nullptr, nullptr);
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


	Env env;

    env.loadEnvironment("../assets/env.json");
    total_meshes = env.meshes.size();

    //Sobrescreve a câmera hardcoded com a câmera do JSON --
    camera = Camera(env.cameraConfig.position, glm::vec3(0.0,1.0,0.0), env.cameraConfig.yaw, env.cameraConfig.pitch);

    // Sobrescreve a matriz de projeção com o FOV (frustum) que veio do JSON
    projection = glm::perspective(glm::radians(env.cameraConfig.fov), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

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
			applyTransform(env.meshes[active_mesh], true);
			applyLightChange(window, env.light, true);
			applyReflectorsChange(window, env.meshes[active_mesh], true);
		}

		// Configurações das transformação dos objetos e da luz.
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			applyTransform(env.meshes[active_mesh], false);
			applyLightChange(window, env.light, false);
			applyReflectorsChange(window, env.meshes[active_mesh], false);
		}

		// Mandando a posição da luz para o shader.
		glUniform3f(glGetUniformLocation(shaderID, "lightPos"),env.light.position.x,env.light.position.y,env.light.position.z);
		glUniform3f(glGetUniformLocation(shaderID, "lightColor"),env.light.color.x,env.light.color.y,env.light.color.z);

    
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		
		// Atualização da matriz de view de acordo com as mudanças que ela sofreu via input
		// de mouse e/ou teclado
		view = camera.getViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// Chamada de Desenho - DRAWCALL
		// Primeiro: renderizar solido
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform1i(glGetUniformLocation(shaderID, "isWireframe"), GL_FALSE);
    float time = glfwGetTime();

    for (auto& mesh : env.meshes) {
        if (mesh.isAnimated) {
            // Pontos de controlo da curva
            glm::vec3 p0(-3.0f, 0.0f, 0.0f);
            glm::vec3 p1(-1.0f, 3.0f, -2.0f);
            glm::vec3 p2(1.0f, -3.0f, 2.0f);
            glm::vec3 p3(3.0f, 0.0f, 0.0f);

            // O valor 't' deve oscilar entre 0 e 1. Usamos sin() para um movimento de vai e vem.
            float t = (sin(time) + 1.0f) / 2.0f; 
            
            mesh.position = calculateBezierPoint(t, p0, p1, p2, p3);
        }
    }
		renderMeshes(env.meshes, shaderID);

		// 2. Se a opção estiver ativa, desenha as linhas por cima
		if (showWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(-1.0, -1.0); 
			
			glUniform1i(glGetUniformLocation(shaderID, "isWireframe"), GL_TRUE);
			glUniform3f(glGetUniformLocation(shaderID, "wireColor"), 0.0f, 0.0f, 0.0f);
			renderMeshes(env.meshes, shaderID);
			
			glDisable(GL_POLYGON_OFFSET_LINE);
		}

			// Troca os buffers da tela
			glfwSwapBuffers(window);
	}


	// Desaloca o VAO do buffer.
	for (auto& mesh : env.meshes) 
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
}

void applyReflectorsChange(GLFWwindow* window, Mesh &mesh, bool shouldGoUp)
{
	float delta = 1.0f * (shouldGoUp ? 1 : -1);

	// Aumenta ou diminui os coeficientes de iluminação (ka, kd, ks) dependendo do valor de "shouldGoUp"
	if (changeKa) {
		mesh.ka += delta * 0.1f;
		if (mesh.ka.b < 0.0f) {
			mesh.ka.r = 0.0f;
			mesh.ka.g = 0.0f;
			mesh.ka.b = 0.0f;
		}
	}

	if (changeKd) {
		mesh.kd += delta * 0.1f;
		if (mesh.kd.b < 0.0f) {
			mesh.kd.r = 0.0f;
			mesh.kd.g = 0.0f;
			mesh.kd.b = 0.0f;
		}
	}

	if (changeKs) {
		mesh.ks += delta * 0.1f;
		if (mesh.ks.b < 0.0f) {
			mesh.ks.r = 0.0f;
			mesh.ks.g = 0.0f;
			mesh.ks.b = 0.0f;
		}
	}

	if (changeQ) {
		mesh.q += delta;
		if (mesh.q < 1.0f) mesh.q = 1.0f; // Evita valores menores que 1
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
		glUniform3f(glGetUniformLocation(shaderID, "ka"), mesh.ka.x, mesh.ka.y, mesh.ka.z);
		glUniform3f(glGetUniformLocation(shaderID, "kd"), mesh.kd.x, mesh.kd.y, mesh.kd.z);
		glUniform3f(glGetUniformLocation(shaderID, "ks"), mesh.ks.x, mesh.ks.y, mesh.ks.z);
		glUniform1f(glGetUniformLocation(shaderID, "q"), mesh.q);
		
		glBindVertexArray(mesh.VAO);
		glDrawArrays(GL_TRIANGLES, 0, mesh.nVertices);
		glBindVertexArray(0);

        // Habilita a textura
		glActiveTexture(GL_TEXTURE0);
		// Associa a textura da Suzzane ao buffer de textura 0
		glBindTexture(GL_TEXTURE_2D, mesh.texID);
		// Envia a informação de textura para o shader
		glUniform1i(glGetUniformLocation(shaderID, "texBuffer"), 0);

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
    active_mesh = (active_mesh + 1) % total_meshes;
}

	// Ativa ou desativa a visualização em modo wireframe (tecla O).
	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		showWireframe = !showWireframe;
	}
}


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
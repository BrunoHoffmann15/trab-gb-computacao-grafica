# Trabalho Prático do Grau B - Computação Gráfica
Leitor e Visualizador de Cenas 3D com OpenGL Moderna

Este projeto é um visualizador de cenas tridimensionais (Diorama) desenvolvido como avaliação do Grau B da disciplina de Computação Gráfica. 

O programa permite carregar múltiplos modelos 3D (.obj) dinamicamente através de um arquivo de configuração JSON, aplicar iluminação de Phong, texturas, e interagir livremente com os objetos e a câmera no espaço cênico.

# Componentes do Grupo

Bel Cogo
Bruno Hoffmann

# Dependências e Pré-requisitos

O projeto utiliza o CMake para gerenciar o processo de build. As bibliotecas matemáticas e de janelas (GLFW, GLM) e o carregador de imagens (stb_image) são baixados e configurados automaticamente durante a compilação via FetchContent.

No entanto, você precisará ter instalado no seu ambiente de desenvolvimento:
Compilador C++ (com suporte a C++17).
CMake (versão 3.10 ou superior).
Assimp (Open Asset Import Library): Utilizado para a leitura avançada de geometrias complexas e extração hierárquica de materiais.
GLAD: Gerenciador de ponteiros da OpenGL.Instalando o Assimp (Linux - Ubuntu/Debian)

Para compilar o projeto, a biblioteca de desenvolvimento do Assimp precisa estar presente no sistema operacional.

Execute o seguinte comando no terminal:
```sudo apt-get install libassimp-dev```


## Configurando a GLAD
Certifique-se de que os arquivos da GLAD (API: gl Version 4.6, Profile: Core) estão posicionados na seguinte estrutura dentro do projeto antes de compilar:
glad.c na pasta common/glad.h e khrplatform.h na pasta include/glad/

# Estrutura de Diretórios

/raiz_do_projeto
├── /assets                 # Texturas, arquivos .obj, .mtl e o env.json
│   └── env.json            # Arquivo de configuração que dita a montagem da cena
├── /common
│   └── glad.c
├── /include
│   └── /glad
│       ├── glad.h
│       └── khrplatform.h
├── /src
│   └── /TrabalhoGB         # Código fonte da engine gráfica
│       ├── main.cpp
│       ├── Env.cpp
│       ├── Env.h
│       ├── Camera.cpp
│       └── Camera.h
└── CMakeLists.txt

# Instruções de Compilação

Abra o terminal na raiz do projeto e execute os comandos abaixo para gerar os arquivos de build e realizar a compilação utilizando múltiplas threads:

```
# 1. Gera os arquivos de build na pasta /build
cmake -S . -B build

# 2. Compila o executável principal
cmake --build build -j 4
```

# Execução e Exemplo de Uso

Após a compilação ser concluída com sucesso, o executável TrabalhoGB estará disponível. Para rodar a aplicação e visualizar o diorama, execute a partir da raiz:
```./build/TrabalhoGB```

Nota: A arquitetura do projeto busca o arquivo estrutural da cena no caminho relativo ../assets/env.json. Para garantir que as texturas relativas e o JSON sejam encontrados, recomenda-se rodar o executável mantendo o diretório base intacto.

# Controles da Aplicação

## Navegabilidade (Câmera):

### Mouse: Movimente o cursor para alterar a orientação de visão (Pitch/Yaw).

- W, A, S, D: Navegação em primeira pessoa pelo espaço tridimensional.

- P: Alterna entre as matrizes de projeção Perspectiva e Ortográfica.

- O: Alterna o modo de renderização para Wireframe (linhas estruturais ocultando as faces).


## Seleção e Geometria:

- N: Alterna o objeto ativo na cena para receber transformações.

## Setas Direcionais (CIMA / BAIXO): Aplica a transformação matemática selecionada ao objeto ativo.
- 1: Modo de Transformação: Escala.
- 2: Modo de Transformação: Rotação.
- 3: Modo de Transformação: Translação.
- X, Y, Z, C: Define qual eixo cartesiano sofrerá a transformação (X, Y, Z isolados ou Todos simultaneamente com C).


## Iluminação e Materiais de Phong:
- 4: Modo de Transformação da Fonte de Luz (movimenta a luz pelo cenário usando as setas).
- 5: Ajustar o Coeficiente de Reflexão Ambiente ($k_a$).
- 6: Ajustar o Coeficiente de Reflexão Difusa ($k_d$).
- 7: Ajustar o Coeficiente de Reflexão Especular ($k_s$).
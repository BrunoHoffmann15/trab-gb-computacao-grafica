# TODO

## Status atual
- Existem implementações de leitura de OBJ simples, câmera navegável, iluminação Phong, e transformações de objetos.
- A cena atual é limitada a dois objetos carregados diretamente em `src/TrabalhoGB/main.cpp`.
- O código ainda não tem suporte a `.mtl`, grupos de malha, animações por curva paramétrica, nem arquivo de configuração de cena.

## Faltando implementar

1. Suporte real a múltiplos arquivos `.obj` via arquivo de configuração
   - Definir formato de arquivo de cena (`JSON`, `XML` ou texto simples)
   - Carregar objetos dinamicamente com: nome do arquivo, transformações iniciais e animações
   - Instanciar objetos com base na definição do arquivo de cena

2. Suporte a grupos de malha em OBJ
   - Parse de `g`, `o` e `usemtl` no arquivo `.obj`
   - Criar e renderizar cada grupo separadamente

3. Uso de materiais do `.mtl`
   - Parse de `mtllib`, `newmtl`, `Ka`, `Kd`, `Ks`
   - Aplicar `ka`, `kd`, `ks` de cada material no shader de iluminação
   - Associar material correto a cada objeto/grupo

4. Suporte a texturas reais
   - Carregar imagens de textura
   - Aplicar `sampler2D` no fragment shader
   - Usar coordenadas `vt` lidas do `.obj`
   - Usar `map_Kd` ou índice de textura do `.mtl`

5. Construir cena com múltiplos objetos
   - Criar cena com mais de 2 objetos
   - Preferencialmente chegar a um mínimo recomendado de 15 objetos (podendo repetir modelos)

6. Implementar animação de objeto por curva paramétrica
   - Trajetória por Bézier, Catmull-Rom ou curva similar
   - Pelo menos um objeto deve se mover usando animação paramétrica
   - Suportar definição dessa animação no arquivo de cena

7. Configuração completa de cena
   - Arquivo de cena define objetos, transformações iniciais, animações paramétricas, luzes, câmera inicial e frustum
   - Carregar posições e orientação de câmera a partir do arquivo de configuração
   - Carregar informação da(s) fonte(s) de luz do arquivo de cena

## Observações
- A entrada atual está hardcoded em `src/TrabalhoGB/main.cpp`.
- A definição única de shaders também está hardcoded no código.
- Não há ainda suporte a seleção de objeto via mouse, apenas alternância por teclado (`N`).

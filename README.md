# Guia de Compilação e Execução: BRKGA para o Problema de Roteamento de Ônibus Escolar

## 1. Estrutura esperada do projeto

A estrutura básica do projeto deve estar organizada aproximadamente assim:

```txt
ic-proe-2025/
│
├── CMakeLists.txt
├── config.conf
├── instance.cpp
├── instance.hpp
│
├── data/
│   ├── Porthcawl.bus
│   ├── Suffolk.bus
│   ├── Cardiff.bus
│   └── ...
│
├── build/
│
└── src/
    ├── decoder/
    │   ├── PROEDecoder.cpp
    │   └── PROEDecoder.hpp
    │
    ├── main/
    │   └── main.cpp
    │
    └── brkga_mp_ipr_cpp/
```

O arquivo `CMakeLists.txt` deve ficar na raiz do projeto, junto de `instance.cpp` e `instance.hpp`.

---

## 2. Pré-requisitos

Antes de compilar, verifique se você tem instalado:

- CMake;
- compilador C++ compatível com C++20;
- MinGW/MSYS2 no Windows;
- biblioteca BRKGA-MP-IPR disponível dentro do projeto.

No terminal PowerShell, você pode verificar se o CMake está instalado com:

```powershell
cmake --version
```

E verificar o compilador com:

```powershell
g++ --version
```

ou:

```powershell
c++ --version
```

---

## 3. Arquivo `CMakeLists.txt`

O arquivo `CMakeLists.txt` deve conter algo neste formato:

```cmake
cmake_minimum_required(VERSION 3.16)

project(ic_proe_2025 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(proe_brkga
    instance.cpp
    src/decoder/PROEDecoder.cpp
    src/main/main.cpp
)

target_include_directories(proe_brkga PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder
    ${CMAKE_CURRENT_SOURCE_DIR}/src/brkga_mp_ipr_cpp/examples/tsp/src/single_obj/brkga_mp_ipr
)
```

Atenção: o último caminho deve apontar para a pasta onde está o arquivo:

```txt
brkga_mp_ipr.hpp
```

Caso o arquivo esteja em outro local, encontre-o com:

```powershell
Get-ChildItem -Path . -Recurse -Filter brkga_mp_ipr.hpp
```

Depois ajuste o caminho dentro de `target_include_directories`.

---

## 4. Arquivo de configuração `config.conf`

Na raiz do projeto, crie o arquivo:

```txt
config.conf
```

com o seguinte conteúdo:

```txt
population_size 100
elite_percentage 0.20
mutants_percentage 0.10
num_elite_parents 1
total_parents 2
bias_type LOGINVERSE
num_independent_populations 1

pr_number_pairs 0
pr_minimum_distance 0.15
pr_type DIRECT
pr_selection BESTSOLUTION
pr_distance_function_type HAMMING
alpha_block_size 1.0
pr_percentage 1.0

exchange_interval 100
num_exchange_individuals 2
reset_interval 0

ipr_interval 0
shake_interval 0
shaking_intensity_upper_bound 0.0
shaking_intensity_lower_bound 0.0
shaking_type SWAP

stall_offset 100
maximum_running_time 60.0
```

Esse arquivo define os parâmetros do BRKGA, como tamanho da população, fração de elite, mutantes, critérios de parada e configurações do path relinking.

---

## 5. Compilando o projeto

Abra o PowerShell na raiz do projeto:

```powershell
cd "C:\Users\Guilherme Moreira\Documents\GitHub\ic-proe-2025"
```

Depois gere os arquivos de build com CMake:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
```

Em seguida, compile:

```powershell
cmake --build build
```

Se tudo estiver correto, o terminal deve mostrar algo parecido com:

```txt
[100%] Linking CXX executable proe_brkga.exe
[100%] Built target proe_brkga
```

O executável será criado em:

```txt
build/proe_brkga.exe
```

---

## 6. Limpando e recompilando do zero

Se quiser limpar a pasta de build e compilar novamente, use:

```powershell
Remove-Item -Recurse -Force build
```

Depois rode novamente:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Se o PowerShell disser que a pasta `build` não existe, ignore e continue com os comandos de CMake.

---

## 7. Executando o programa

A execução segue o formato:

```powershell
.\build\proe_brkga.exe <arquivo_instancia> <arquivo_configuracao> [semente] [threads]
```

Exemplo com a instância `Porthcawl.bus`:

```powershell
.\build\proe_brkga.exe .\data\Porthcawl.bus .\config.conf 123 1
```

Onde:

```txt
.\build\proe_brkga.exe    executável compilado
.\data\Porthcawl.bus      instância do PROE
.\config.conf             arquivo de parâmetros do BRKGA
123                       semente aleatória
1                         número de threads
```

---

## 8. Exemplo de execução

Comando:

```powershell
.\build\proe_brkga.exe .\data\Porthcawl.bus .\config.conf 123 1
```

Saída esperada:

```txt
Lendo arquivo de configuracao do BRKGA...
  Arquivo: .\config.conf

Lendo instancia do PROE...
  Arquivo: .\data\Porthcawl.bus

Instancia carregada com sucesso:
  Formato: .bus da literatura
  Numero de paradas candidatas: 152
  Numero de enderecos: 42
  Capacidade dos veiculos: 40
  Escola: Ysgol y Ferch o'r Sger
  Distancia minima de elegibilidade: 3.200
  Distancia maxima de caminhada: 1.600
  Semente utilizada: 123
  Numero de threads: 1

Criando decodificador do PROE...
  Cada gene do cromossomo representa a prioridade de uma parada candidata.
  O decodificador seleciona paradas, aloca estudantes e constroi rotas.

Executando o BRKGA...

* Iteracao 1 | melhor fitness = ...
* Iteracao 5 | melhor fitness = ...

Melhor solucao:
Distancia total: 25.483 km
Numero de rotas: 2
Fitness: 25.483
Penalidades aplicadas: 0.000

Rota 1:
Escola -> Parada 73 -> Parada 55 -> Parada 40 -> Escola
Carga: 37 estudantes
Distancia: 13.210 km
Tempo estimado: 1840.000 segundos

Rota 2:
Escola -> Parada 127 -> Parada 143 -> Parada 1 -> Escola
Carga: 29 estudantes
Distancia: 12.273 km
Tempo estimado: 1710.000 segundos
```

Os valores exatos podem mudar dependendo da instância, da semente e dos parâmetros do BRKGA.

---

## 9. Testando outras instâncias

Para testar a instância `Suffolk.bus`:

```powershell
.\build\proe_brkga.exe .\data\Suffolk.bus .\config.conf 123 1
```

Para testar a instância `Cardiff.bus`:

```powershell
.\build\proe_brkga.exe .\data\Cardiff.bus .\config.conf 123 1
```

Para testar com outra semente:

```powershell
.\build\proe_brkga.exe .\data\Porthcawl.bus .\config.conf 456 1
```

Para testar com mais threads:

```powershell
.\build\proe_brkga.exe .\data\Porthcawl.bus .\config.conf 123 4
```

---

## 10. O que o programa faz

O programa implementa uma metaheurística baseada em BRKGA para o Problema de Roteamento de Ônibus Escolar.

O fluxo geral é:

```txt
1. Lê a instância .bus.
2. Lê os parâmetros do BRKGA.
3. Cria um cromossomo com uma random key por parada candidata.
4. Usa o BRKGA para evoluir populações de cromossomos.
5. O decoder transforma cada cromossomo em uma solução do PROE.
6. O decoder seleciona paradas.
7. O decoder aloca estudantes às paradas selecionadas.
8. O decoder constrói rotas respeitando a capacidade dos veículos.
9. A solução é avaliada pela distância total percorrida.
10. O programa imprime a melhor solução encontrada.
```

A função objetivo principal é:

```txt
minimizar a distância total percorrida
```

Penalidades são aplicadas apenas em casos de inviabilidade, como:

```txt
- estudante/endereço sem atendimento;
- parada com demanda superior à capacidade do veículo;
- violação de tempo máximo, caso essa restrição seja ativada futuramente.
```

---

## 11. Problemas comuns

### Erro: `does not appear to contain CMakeLists.txt`

Esse erro acontece quando o `CMakeLists.txt` não está na pasta onde o comando está sendo executado.

Verifique se você está na raiz do projeto:

```powershell
pwd
```

A raiz correta deve ser algo como:

```txt
C:\Users\Guilherme Moreira\Documents\GitHub\ic-proe-2025
```

E o arquivo deve estar em:

```txt
ic-proe-2025/CMakeLists.txt
```

---

### Erro: `brkga_mp_ipr.hpp: No such file or directory`

Esse erro significa que o CMake não está encontrando o cabeçalho da biblioteca BRKGA.

Procure o arquivo com:

```powershell
Get-ChildItem -Path . -Recurse -Filter brkga_mp_ipr.hpp
```

Depois ajuste o caminho no `CMakeLists.txt`:

```cmake
target_include_directories(proe_brkga PRIVATE
    ...
    caminho/para/a/pasta/que/contem/brkga_mp_ipr.hpp
)
```

---

### Erro de parâmetros no `config.conf`

Se aparecer erro relacionado a algum parâmetro ausente, como:

```txt
Parameter 'pr_distance_function_type' is required
```

verifique se o arquivo `config.conf` contém todos os parâmetros listados neste guia.

---

### O programa mostra apenas `Usage`

Se você executar apenas:

```powershell
.\build\proe_brkga.exe
```

o programa mostrará:

```txt
Uso:
  proe_brkga.exe <arquivo_instancia> <arquivo_configuracao> [semente] [threads]
```

Isso significa que faltaram argumentos.

Execute com a instância e o arquivo de configuração:

```powershell
.\build\proe_brkga.exe .\data\Porthcawl.bus .\config.conf 123 1
```

---

## 12. Comandos principais resumidos

Compilar:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Executar:

```powershell
.\build\proe_brkga.exe .\data\Porthcawl.bus .\config.conf 123 1
```

Limpar e recompilar:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

---

## 13. Interpretação da saída

A saída final apresenta:

```txt
Melhor solucao:
Distancia total: valor em km
Numero de rotas: quantidade de rotas/ônibus usados
Fitness: valor da função objetivo
Penalidades aplicadas: penalidades por inviabilidade
```

Depois, cada rota é exibida no formato:

```txt
Rota k:
Escola -> Parada a -> Parada b -> ... -> Escola
Carga: número de estudantes
Distancia: distância total da rota
Tempo estimado: tempo total da rota em segundos
```

A rota sempre começa e termina na escola.

---

## 14. Observação metodológica

A implementação atual corresponde a uma metaheurística populacional baseada em BRKGA com decodificador construtivo.

O BRKGA realiza a busca global por boas prioridades de paradas. O decoder, por sua vez, transforma cada cromossomo em uma solução do PROE por meio de:

```txt
- seleção de paradas;
- alocação de estudantes;
- construção de rotas capacitadas.
```

Portanto, o método completo pode ser descrito como:

```txt
BRKGA + decoder construtivo para o PROE
```

ou:

```txt
metaheurística evolutiva com representação por random keys aplicada ao Problema de Roteamento de Ônibus Escolar.
```

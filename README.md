# ic-proe-2025
Avaliação e proposta de métodos de otimização para roteamento de ônibus escolar.

ic-proe-2025/
├── README.md                       # Apresentação, instruções, escopo do projeto
├── LICENSE                         # Licença (MIT, GPL, etc)
├── .gitignore                      # Estilo: Python, C++, dados, modelo
│
├── data/                           # Conjunto completo de dados do projeto
│   ├── raw/                        # Dados brutos originais
│   ├── processed/                  # Dados tratados, prontos para modelagem
│   └── external/                   # Dados externos (ex: benchmark, público)
│
├── notebooks/                      # Jupyter Notebooks para exploração, EDA e protótipos ML
│   ├── 01_exploratory.ipynb
│   ├── 02_lstm_training.ipynb
│   └── 03_results_analysis.ipynb
│
├── src/
│   ├── ml/                         # Deep Learning Python (PyTorch, TensorFlow)
│   │   ├── preprocess.py           # Limpeza, feature engineering
│   │   ├── train_lstm.py           # Treinamento, avaliação LSTM
│   │   ├── predict_demand.py       # Inferência demanda por parada/hora
│   │   └── utils.py                # Funções utilitárias, métricas
│   │
│   ├── cpp/                        # Otimização em C++, núcleo BRKGA/SBRP
│   │   ├── include/                # Headers (.h/.hpp) públicos para biblioteca
│   │   ├── src/                    # Implementações (.cpp)
│   │   ├── decoder/                # Decodificadores BRKGA customizados para cada aplicação
│   │   ├── main.cpp                # Executável principal
│   │   └── CMakeLists.txt          # Script de build modernizado
│   │
│   └── integration/                # Scripts ponte Python↔C++ (ex: geração CSV, chamada sistema)
│       ├── ml_to_cpp.py            # Gera arquivo de previsão ML para otimização
│       └── cpp_to_ml.py            # Exporta resultados do C++ para análise Python
│
├── models/                         # Modelos treinados e checkpoints (.pt/.pth/.onnx)
│   ├── lstm_school_demand.pt
│   ├── scaler_school_demand.pkl
│   ├── brkga_best_solutions.json
│
├── results/                        # Relatórios, gráficos, tabelas-exportadas, análises
│   ├── figures/                    # Imagens, gráficos
│   ├── tables/                     # Tabelas resumo, comparações
│   └── logs/                       # Logs de execução, outputs experimentais
│
├── tests/                          # Testes unitários, integração, experimentação
│   ├── test_preprocessing.py
│   ├── test_lstm.py
│   ├── test_brkga.cpp
│   └── test_integration.py
│
├── docs/                           # Documentação adicional do projeto
│   ├── usage_guide.md
│   ├── architecture.md
│   ├── literature_review.md
│   └── api_reference.md
│
├── requirements.txt                # Dependências Python (PyTorch, pandas, etc)
├── environment.yml                 # Ambiente Conda (alternativa/expansão)
├── setup.py                        # Instalação módulos Python (opcional)
│
├── scripts/                        # Shell scripts úteis (ex: inicialização, deployment)
│   ├── download_data.sh
│   └── run_full_pipeline.sh
│
└── Makefile                        # Build, testes, rotinas automatizadas (C++, Python)

Recomendações Práticas e Justificativas
Separação clara Python/C++: Diretórios independentes para ML (Python) e Otimização (C++) evitam conflitos, facilitam CI/CD e builds automatizados.​

docs/: Documentação em Markdown, com arquitetura, literatura e guias de uso transparente para onboarding de novos pesquisadores.​

notebooks/: Facilita pesquisa exploratória, provas de conceito, comparações entre métodos.​

integration/: Scripts para conversão, importação/exportação, permitindo workflow reproducível entre ML e BRKGA.​

tests/: Foco no rigor científico e confiabilidade do pipeline porque integra componentes com linguagens distintas, diminuindo riscos de inconsistências.​

results/: Tabelas e gráficos de performance, logs replicáveis para uso em papers e relatórios.​

models/: Versionamento dos checkpoints garante reprodutibilidade dos experimentos ML.​

CMakeLists.txt e Makefile: Automatização do build C++, simplificando setup em novas máquinas e integração com CI (Travis, GitHub Actions).​

.gitignore bem configurado: Ignorar dados confidenciais, logs, checkpoints de modelos, arquivos temporários e caches.​
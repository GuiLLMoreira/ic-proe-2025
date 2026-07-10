#include "../../instance.hpp"
#include "../decoder/PROEDecoder.hpp"
#include "brkga_mp_ipr.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace BRKGA;

namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
// Classe para duplicar a saida do cout:
// tudo que aparece no PowerShell tambem sera salvo no arquivo de log.
// -----------------------------------------------------------------------------
class TeeBuffer : public std::streambuf {
public:
    TeeBuffer(std::streambuf* buffer1, std::streambuf* buffer2):
        buffer1(buffer1),
        buffer2(buffer2)
    {}

protected:
    int overflow(int c) override {
        if(c == EOF) {
            return !EOF;
        }

        const int r1 = buffer1->sputc(static_cast<char>(c));
        const int r2 = buffer2->sputc(static_cast<char>(c));

        if(r1 == EOF || r2 == EOF) {
            return EOF;
        }

        return c;
    }

    int sync() override {
        const int r1 = buffer1->pubsync();
        const int r2 = buffer2->pubsync();

        return (r1 == 0 && r2 == 0) ? 0 : -1;
    }

private:
    std::streambuf* buffer1;
    std::streambuf* buffer2;
};

// -----------------------------------------------------------------------------
// Gera data no formato DIAMESANO.
// Exemplo: 08052026
// -----------------------------------------------------------------------------
string get_current_date_stamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};

#ifdef _WIN32
    localtime_s(&local_time, &now_time);
#else
    localtime_r(&now_time, &local_time);
#endif

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%d%m%Y");

    return oss.str();
}

// -----------------------------------------------------------------------------
// Funcoes auxiliares
// -----------------------------------------------------------------------------
string dot_escape(const string& text) {
    string result;

    for(char c : text) {
        if(c == '"') {
            result += "\\\"";
        }
        else if(c == '\\') {
            result += "\\\\";
        }
        else {
            result += c;
        }
    }

    return result;
}

string build_route_sequence(const PROEDecoder::Route& route) {
    ostringstream oss;

    oss << "Escola";

    for(unsigned stop : route.stops) {
        oss << " -> Parada " << stop;
    }

    oss << " -> Escola";

    return oss.str();
}

string format_decimal(double value, int precision = 3) {
    ostringstream oss;
    oss << fixed << setprecision(precision) << value;

    string text = oss.str();
    replace(text.begin(), text.end(), '.', ',');

    return text;
}

string format_yes_no(bool value) {
    return value ? "sim" : "nao";
}

size_t find_best_route_index(const vector<PROEDecoder::Route>& routes) {
    if(routes.empty()) {
        return numeric_limits<size_t>::max();
    }

    const double eps = 1e-9;
    size_t best_index = 0;

    for(size_t i = 1; i < routes.size(); ++i) {
        const auto& candidate = routes[i];
        const auto& current_best = routes[best_index];

        const bool shorter_distance =
            candidate.distance + eps < current_best.distance;

        const bool same_distance_shorter_time =
            abs(candidate.distance - current_best.distance) <= eps &&
            candidate.time + eps < current_best.time;

        if(shorter_distance || same_distance_shorter_time) {
            best_index = i;
        }
    }

    return best_index;
}

void export_route_table_csv(
    const PROEDecoder::Solution& solution,
    double algorithm_time,
    double wall_time,
    const fs::path& output_file
) {
    ofstream file(output_file);

    if(!file) {
        throw runtime_error("Nao foi possivel criar a tabela de rotas: " + output_file.string());
    }

    const size_t best_route_index = find_best_route_index(solution.routes);

    file << "rota;"
         << "melhor_rota_da_solucao;"
         << "sequencia;"
         << "numero_paradas;"
         << "carga_estudantes;"
         << "distancia_km;"
         << "tempo_rota_segundos;"
         << "tempo_execucao_algoritmo_seg;"
         << "tempo_parede_execucao_seg\n";

    for(size_t r = 0; r < solution.routes.size(); ++r) {
        const auto& route = solution.routes[r];

        file << (r + 1) << ";"
             << format_yes_no(r == best_route_index) << ";"
             << "\"" << build_route_sequence(route) << "\"" << ";"
             << route.stops.size() << ";"
             << route.load << ";"
             << format_decimal(route.distance) << ";"
             << format_decimal(route.time) << ";"
             << format_decimal(algorithm_time) << ";"
             << format_decimal(wall_time) << "\n";
    }
}

void export_solution_graph_dot(
    const PROEDecoder::Solution& solution,
    const Instance& instance,
    const fs::path& output_file
) {
    ofstream file(output_file);

    if(!file) {
        throw runtime_error("Nao foi possivel criar o arquivo DOT do grafo: " + output_file.string());
    }

    file << fixed << setprecision(8);

    file << "digraph SolucaoPROE {\n";
    file << "  graph [layout=neato, overlap=false, splines=true];\n";
    file << "  node [shape=circle, fontsize=10];\n";
    file << "  edge [fontsize=9];\n\n";

    // No arquivo .bus, Point.x = latitude e Point.y = longitude.
    // Para visualizacao, usamos pos=\"longitude,latitude!\".
    file << "  Escola ["
         << "label=\"Escola\\n" << dot_escape(instance.school_name) << "\", "
         << "shape=box, "
         << "pos=\"" << instance.school.y << "," << instance.school.x << "!\""
         << "];\n\n";

    set<unsigned> used_stops;

    for(const auto& route : solution.routes) {
        for(unsigned stop : route.stops) {
            used_stops.insert(stop);
        }
    }

    for(unsigned stop : used_stops) {
        const auto& s = instance.stops[stop];

        // No .bus, indice 0 = escola.
        // Portanto, a parada interna stop corresponde ao indice original stop + 1.
        const unsigned original_index = stop + 1;

        file << "  P" << stop << " ["
             << "label=\"Parada " << stop
             << "\\nOrig. " << original_index
             << "\\nDem. " << solution.demand_by_stop[stop]
             << "\", "
             << "pos=\"" << s.coord.y << "," << s.coord.x << "!\""
             << "];\n";
    }

    file << "\n";

    for(size_t r = 0; r < solution.routes.size(); ++r) {
        const auto& route = solution.routes[r];

        if(route.stops.empty()) {
            continue;
        }

        file << "  // Rota " << (r + 1) << "\n";

        file << "  Escola -> P" << route.stops.front()
             << " [label=\"R" << (r + 1) << "\"];\n";

        for(size_t i = 1; i < route.stops.size(); ++i) {
            file << "  P" << route.stops[i - 1]
                 << " -> P" << route.stops[i]
                 << " [label=\"R" << (r + 1) << "\"];\n";
        }

        file << "  P" << route.stops.back()
             << " -> Escola"
             << " [label=\"R" << (r + 1) << "\"];\n\n";
    }

    file << "}\n";
}

void export_solution_graph_png(
    const PROEDecoder::Solution& solution,
    const Instance& instance,
    const fs::path& dot_file,
    const fs::path& png_file
) {
    export_solution_graph_dot(solution, instance, dot_file);

    // Usa Graphviz/neato para converter DOT em PNG.
    // O Graphviz precisa estar instalado e adicionado ao PATH do Windows.
    const string command =
        "neato -Tpng \"" + dot_file.string() + "\" -o \"" + png_file.string() + "\"";

    const int result = std::system(command.c_str());

    if(result != 0) {
        throw runtime_error(
            "Nao foi possivel gerar o PNG do grafo. "
            "Verifique se o Graphviz esta instalado e se o comando 'neato' esta no PATH. "
            "Comando executado: " + command
        );
    }
}

int main(int argc, char* argv[]) {
    ofstream log_file;
    streambuf* original_cout_buffer = cout.rdbuf();
    TeeBuffer* tee_buffer = nullptr;

    try {
        if(argc < 3) {
            cerr << "Uso:\n"
                 << "  " << argv[0]
                 << " <arquivo_instancia> <arquivo_configuracao> [semente] [threads]\n\n";

            cerr << "Exemplo:\n"
                 << "  " << argv[0]
                 << " .\\data\\Porthcawl.bus .\\config.conf 123 1\n";

            return 1;
        }

        const string instance_file = argv[1];
        const string config_file   = argv[2];

        const unsigned seed =
            argc >= 4 ? static_cast<unsigned>(stoul(argv[3])) : 123u;

        const unsigned num_threads =
            argc >= 5 ? static_cast<unsigned>(stoul(argv[4])) : 1u;

        // ---------------------------------------------------------------------
        // Pastas de resultados ja padronizadas no repositorio
        // ---------------------------------------------------------------------
        const fs::path results_dir = "results";
        const fs::path figures_dir = results_dir / "figures";
        const fs::path logs_dir    = results_dir / "logs";
        const fs::path tables_dir  = results_dir / "tables";

        fs::create_directories(figures_dir);
        fs::create_directories(logs_dir);
        fs::create_directories(tables_dir);

        const string instance_name = fs::path(instance_file).stem().string();
        const string date_stamp = get_current_date_stamp();

        // Padrao:
        // DIAMESANOnomeinstancia_figure.png
        // DIAMESANOnomeinstancia_logs.txt
        // DIAMESANOnomeinstancia_table.csv
        const string prefix = date_stamp + instance_name;

        const fs::path graph_png_path = figures_dir / (prefix + "_figure.png");
        const fs::path graph_dot_path = figures_dir / (prefix + "_figure.dot");
        const fs::path log_path       = logs_dir    / (prefix + "_logs.txt");
        const fs::path table_path     = tables_dir  / (prefix + "_table.csv");

        // ---------------------------------------------------------------------
        // Ativa o log em arquivo duplicando o cout
        // ---------------------------------------------------------------------
        log_file.open(log_path);

        if(!log_file) {
            throw runtime_error("Nao foi possivel criar o arquivo de log: " + log_path.string());
        }

        tee_buffer = new TeeBuffer(original_cout_buffer, log_file.rdbuf());
        cout.rdbuf(tee_buffer);

        cout << fixed << setprecision(3);

        cout << "Arquivos de saida desta execucao:\n";
        cout << "  Log da execucao: " << log_path.string() << "\n";
        cout << "  Tabela de rotas: " << table_path.string() << "\n";
        cout << "  Grafo da melhor solucao: " << graph_png_path.string() << "\n\n";

        // ----------------------------
        // Ler configuração do BRKGA
        // ----------------------------
        cout << "Lendo arquivo de configuracao do BRKGA...\n";
        cout << "  Arquivo: " << config_file << "\n\n";

        auto [brkga_params, control_params] =
            BRKGA::readConfiguration(config_file);

        // ----------------------------
        // Ler instância PROE
        // ----------------------------
        cout << "Lendo instancia do PROE...\n";
        cout << "  Arquivo: " << instance_file << "\n\n";

        Instance instance(instance_file);

        const unsigned n =
            static_cast<unsigned>(instance.stops.size());

        if(n == 0) {
            throw runtime_error("A instancia nao possui paradas candidatas.");
        }

        cout << "Instancia carregada com sucesso:\n";
        cout << "  Formato: "
             << (instance.is_bus_format ? ".bus da literatura" : "formato sintetico simples")
             << "\n";

        cout << "  Numero de paradas candidatas: " << n << "\n";
        cout << "  Numero de enderecos: " << instance.addresses.size() << "\n";
        cout << "  Capacidade dos veiculos: " << instance.vehicle_capacity << "\n";

        if(instance.is_bus_format) {
            cout << "  Escola: " << instance.school_name << "\n";
            cout << "  Distancia minima de elegibilidade: "
                 << instance.min_eligibility_distance << "\n";
            cout << "  Distancia maxima de caminhada: "
                 << instance.max_walking_distance << "\n";
        }

        cout << "  Semente utilizada: " << seed << "\n";
        cout << "  Numero de threads: " << num_threads << "\n\n";

        // ----------------------------
        // Criar decoder
        // ----------------------------
        cout << "Criando decodificador do PROE...\n";
        cout << "  Cada gene do cromossomo representa a prioridade de uma parada candidata.\n";
        cout << "  O decodificador seleciona paradas, aloca estudantes e constroi rotas.\n\n";

        PROEDecoder decoder(instance);

        const unsigned chromosome_size = n;

        if(chromosome_size <= 1) {
            throw runtime_error("O tamanho do cromossomo deve ser maior que um.");
        }

        cout << "Configuracao do cromossomo:\n";
        cout << "  Tamanho do cromossomo: " << chromosome_size << "\n";
        cout << "  Interpretacao: uma random key por parada candidata.\n\n";

        // ----------------------------
        // Criar e rodar BRKGA
        // ----------------------------
        cout << "Inicializando BRKGA-MP-IPR...\n";
        cout << "  Objetivo: minimizar a distancia total percorrida.\n";
        cout << "  Penalidades sao aplicadas apenas para violacoes de viabilidade.\n\n";

        BRKGA::BRKGA_MP_IPR<PROEDecoder> algorithm(
            decoder,
            BRKGA::Sense::MINIMIZE,
            seed,
            chromosome_size,
            brkga_params,
            num_threads,
            true
        );

        algorithm.addNewSolutionObserver(
            [](const BRKGA::AlgorithmStatus& status) {
                cout << "* Iteracao " << status.current_iteration
                     << " | melhor fitness = " << status.best_fitness
                     << " | tempo = " << status.current_time << "s"
                     << "\n";

                return true;
            }
        );

        cout << "Executando o BRKGA...\n\n";

        const auto wall_start = chrono::steady_clock::now();

        const auto final_status =
            algorithm.run(control_params, &cout);

        const auto wall_end = chrono::steady_clock::now();

        const double wall_time =
            chrono::duration<double>(wall_end - wall_start).count();

        cout << "\nStatus final do algoritmo:\n";
        cout << final_status << "\n";

        cout << "Melhor fitness encontrado: "
             << final_status.best_fitness << "\n\n";

        // ----------------------------
        // Reconstruir melhor solução
        // ----------------------------
        cout << "Reconstruindo a melhor solucao encontrada pelo decodificador...\n\n";

        const PROEDecoder::Solution best_solution =
            decoder.decodeSolution(final_status.best_chromosome);

        const size_t best_route_index =
            find_best_route_index(best_solution.routes);

        // ----------------------------
        // Imprimir solução em português
        // ----------------------------
        cout << "Melhor solucao:\n";
        cout << "Distancia total: "
             << best_solution.total_distance << " km\n";

        cout << "Numero de rotas: "
             << best_solution.number_routes << "\n";

        cout << "Fitness: "
             << best_solution.fitness << "\n";

        cout << "Penalidades aplicadas: "
             << best_solution.total_penalty << "\n\n";

        for(size_t r = 0; r < best_solution.routes.size(); ++r) {
            const auto& route = best_solution.routes[r];

            cout << "Rota " << (r + 1) << ":\n";

            cout << "Escola";

            for(unsigned stop : route.stops) {
                cout << " -> Parada " << stop;
            }

            cout << " -> Escola\n";

            cout << "Carga: "
                 << route.load << " estudantes\n";

            cout << "Distancia: "
                 << route.distance << " km\n";

            cout << "Tempo estimado: "
                 << route.time << " segundos\n";

            cout << "\n";
        }

        if(best_route_index != numeric_limits<size_t>::max()) {
            const auto& best_route = best_solution.routes[best_route_index];

            cout << "Melhor rota especifica "
                 << "(menor distancia; desempate por menor tempo):\n";
            cout << "  Rota: " << (best_route_index + 1) << "\n";
            cout << "  Distancia: " << best_route.distance << " km\n";
            cout << "  Tempo estimado: " << best_route.time << " segundos\n";
            cout << "  Sequencia: " << build_route_sequence(best_route) << "\n\n";
        }

        // ----------------------------
        // Ranking completo das paradas
        // ----------------------------
        vector<pair<double, unsigned>> ranking(n);

        for(unsigned i = 0; i < n; ++i) {
            ranking[i] = make_pair(final_status.best_chromosome[i], i);
        }

        sort(ranking.begin(), ranking.end());

        cout << "Ordem de prioridade das paradas no melhor cromossomo:\n";

        for(const auto& item : ranking) {
            cout << item.second << " ";
        }

        cout << "\n\n";

        // ----------------------------
        // Exportações padronizadas
        // ----------------------------
        cout << "Exportando arquivos padronizados da melhor solucao...\n";

        export_route_table_csv(
            best_solution,
            final_status.current_time.count(),
            wall_time,
            table_path
        );
        cout << "  Tabela de rotas salva em: " << table_path.string() << "\n";

        export_solution_graph_png(best_solution, instance, graph_dot_path, graph_png_path);
        cout << "  Grafo da solucao salvo em PNG: " << graph_png_path.string() << "\n";

        cout << "  Log da execucao salvo em: " << log_path.string() << "\n\n";

        cout << "Execucao finalizada com sucesso.\n";

        // Restaura cout antes de encerrar.
        cout.rdbuf(original_cout_buffer);
        delete tee_buffer;
        tee_buffer = nullptr;

        return 0;
    }
    catch(const exception& e) {
        cout.rdbuf(original_cout_buffer);

        if(tee_buffer != nullptr) {
            delete tee_buffer;
            tee_buffer = nullptr;
        }

        cerr << "\nERRO: " << e.what() << "\n";
        return 1;
    }
}















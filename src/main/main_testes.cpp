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
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;
using namespace BRKGA;

namespace fs = std::filesystem;

// ============================================================================
// Estrutura para armazenar o resultado de uma execução
// ============================================================================

struct RunResult {
    string instance_name;
    string instance_file;

    unsigned global_execution_id{};
    unsigned execution_id{};
    unsigned seed{};
    unsigned threads{};

    double best_fitness{};
    double total_distance{};
    double total_penalty{};

    int number_routes{};
    int selected_stops{};
    int total_load{};

    double total_route_time{};
    double average_route_time{};
    double max_route_time{};

    unsigned current_iteration{};
    unsigned last_update_iteration{};
    double algorithm_time{};
    double wall_time{};
};

struct RouteRecord {
    string instance_name;
    string instance_file;

    unsigned global_execution_id{};
    unsigned execution_id{};
    unsigned seed{};
    unsigned threads{};
    unsigned route_id{};

    bool best_route_in_execution{};
    bool best_route_overall{};
    bool from_global_best_solution{};

    string sequence;
    size_t number_stops{};
    int load{};

    double distance{};
    double route_time{};
    double total_route_time_in_execution{};
    double algorithm_time{};
    double wall_time{};
};

// ============================================================================
// Data no formato DIAMESANO
// ============================================================================

string get_current_date_stamp() {
    const auto now = chrono::system_clock::now();
    const time_t now_time = chrono::system_clock::to_time_t(now);

    tm local_time{};

#ifdef _WIN32
    localtime_s(&local_time, &now_time);
#else
    localtime_r(&now_time, &local_time);
#endif

    ostringstream oss;
    oss << put_time(&local_time, "%d%m%Y");

    return oss.str();
}

// ============================================================================
// Geração de seeds aleatórias sem repetição
// ============================================================================

vector<unsigned> generate_random_seeds(
    unsigned total_seeds,
    unsigned master_seed
) {
    mt19937 rng(master_seed);

    uniform_int_distribution<unsigned> distribution(
        1u,
        numeric_limits<unsigned>::max()
    );

    unordered_set<unsigned> used_seeds;
    vector<unsigned> seeds;

    seeds.reserve(total_seeds);

    while(seeds.size() < total_seeds) {
        const unsigned seed = distribution(rng);

        if(used_seeds.insert(seed).second) {
            seeds.push_back(seed);
        }
    }

    return seeds;
}

// ============================================================================
// Funções auxiliares para exportação
// ============================================================================

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

string format_decimal(double value, int precision = 6) {
    ostringstream oss;
    oss << fixed << setprecision(precision) << value;

    string text = oss.str();
    replace(text.begin(), text.end(), '.', ',');

    return text;
}

string format_yes_no(bool value) {
    return value ? "sim" : "nao";
}

double read_maximum_running_time_seconds(const string& config_file) {
    ifstream file(config_file);

    if(!file) {
        throw runtime_error("Nao foi possivel ler o arquivo de configuracao: " + config_file);
    }

    string key;

    while(file >> key) {
        if(!key.empty() && key[0] == '#') {
            string ignored_line;
            getline(file, ignored_line);
            continue;
        }

        if(key == "maximum_running_time") {
            double value = 0.0;
            file >> value;
            return value;
        }

        string ignored_value;
        getline(file, ignored_value);
    }

    return 0.0;
}

string build_running_time_filename_tag(double maximum_running_time) {
    ostringstream oss;
    oss << fixed << setprecision(3) << maximum_running_time;

    string value = oss.str();

    while(value.size() > 1 && value.back() == '0') {
        value.pop_back();
    }

    if(!value.empty() && value.back() == '.') {
        value.pop_back();
    }

    replace(value.begin(), value.end(), '.', 'p');

    return "maximum_run_time_" + value + "s";
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

size_t find_best_route_record_index(const vector<RouteRecord>& routes) {
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
            candidate.route_time + eps < current_best.route_time;

        if(shorter_distance || same_distance_shorter_time) {
            best_index = i;
        }
    }

    return best_index;
}

void export_solution_graph_dot(
    const PROEDecoder::Solution& solution,
    const Instance& instance,
    const fs::path& output_file
) {
    ofstream file(output_file);

    if(!file) {
        throw runtime_error("Nao foi possivel criar o arquivo DOT: " + output_file.string());
    }

    file << fixed << setprecision(8);

    file << "digraph SolucaoPROE {\n";
    file << "  graph [layout=neato, overlap=false, splines=true];\n";
    file << "  node [shape=circle, fontsize=10];\n";
    file << "  edge [fontsize=9];\n\n";

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

        // No .bus, o índice 0 é a escola.
        // Portanto, a parada interna stop corresponde ao índice original stop + 1.
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
             << " -> Escola [label=\"R" << (r + 1) << "\"];\n\n";
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

    const string command =
        "neato -Tpng \"" + dot_file.string() + "\" -o \"" + png_file.string() + "\"";

    const int result = system(command.c_str());

    if(result != 0) {
        throw runtime_error(
            "Nao foi possivel gerar o PNG do grafo. "
            "Verifique se o Graphviz esta instalado e se o comando 'neato' esta no PATH."
        );
    }
}

// ============================================================================
// Exportação das tabelas
// ============================================================================

void export_detailed_results_csv(
    const vector<RunResult>& results,
    const fs::path& output_file
) {
    ofstream file(output_file);

    if(!file) {
        throw runtime_error("Nao foi possivel criar a tabela detalhada: " + output_file.string());
    }

    file << "instancia;"
         << "arquivo;"
         << "execucao_global;"
         << "execucao;"
         << "seed;"
         << "threads;"
         << "funcao_objetivo;"
         << "distancia_total_km;"
         << "penalidade_total;"
         << "numero_rotas;"
         << "paradas_selecionadas;"
         << "carga_total_estudantes;"
         << "tempo_total_rotas_seg;"
         << "tempo_medio_rota_seg;"
         << "maior_tempo_rota_seg;"
         << "iteracao_final;"
         << "iteracao_ultima_melhoria;"
         << "tempo_algoritmo_seg;"
         << "tempo_parede_seg\n";

    for(const auto& r : results) {
        file << r.instance_name << ";"
             << r.instance_file << ";"
             << r.global_execution_id << ";"
             << r.execution_id << ";"
             << r.seed << ";"
             << r.threads << ";"
             << format_decimal(r.best_fitness) << ";"
             << format_decimal(r.total_distance) << ";"
             << format_decimal(r.total_penalty) << ";"
             << r.number_routes << ";"
             << r.selected_stops << ";"
             << r.total_load << ";"
             << format_decimal(r.total_route_time) << ";"
             << format_decimal(r.average_route_time) << ";"
             << format_decimal(r.max_route_time) << ";"
             << r.current_iteration << ";"
             << r.last_update_iteration << ";"
             << format_decimal(r.algorithm_time) << ";"
             << format_decimal(r.wall_time) << "\n";
    }
}

double mean(const vector<double>& values) {
    if(values.empty()) {
        return 0.0;
    }

    const double sum = accumulate(values.begin(), values.end(), 0.0);
    return sum / static_cast<double>(values.size());
}

double standard_deviation(const vector<double>& values) {
    if(values.size() <= 1) {
        return 0.0;
    }

    const double avg = mean(values);

    double acc = 0.0;

    for(double v : values) {
        const double diff = v - avg;
        acc += diff * diff;
    }

    return sqrt(acc / static_cast<double>(values.size() - 1));
}

void export_summary_by_instance_csv(
    const vector<RunResult>& results,
    const fs::path& output_file
) {
    ofstream file(output_file);

    if(!file) {
        throw runtime_error("Nao foi possivel criar a tabela resumo: " + output_file.string());
    }

    file << "instancia;"
         << "numero_execucoes;"
         << "melhor_funcao_objetivo;"
         << "media_funcao_objetivo;"
         << "pior_funcao_objetivo;"
         << "desvio_padrao_funcao_objetivo;"
         << "media_numero_rotas;"
         << "media_paradas_selecionadas;"
         << "media_tempo_algoritmo_seg;"
         << "media_tempo_total_rotas_seg;"
         << "media_maior_tempo_rota_seg;"
         << "melhor_seed\n";

    set<string> instance_names;

    for(const auto& r : results) {
        instance_names.insert(r.instance_name);
    }

    for(const auto& name : instance_names) {
        vector<double> objectives;
        vector<double> routes;
        vector<double> selected_stops;
        vector<double> algorithm_times;
        vector<double> total_route_times;
        vector<double> max_route_times;

        double best_objective = numeric_limits<double>::infinity();
        double worst_objective = -numeric_limits<double>::infinity();
        unsigned best_seed = 0;

        for(const auto& r : results) {
            if(r.instance_name != name) {
                continue;
            }

            objectives.push_back(r.best_fitness);
            routes.push_back(static_cast<double>(r.number_routes));
            selected_stops.push_back(static_cast<double>(r.selected_stops));
            algorithm_times.push_back(r.algorithm_time);
            total_route_times.push_back(r.total_route_time);
            max_route_times.push_back(r.max_route_time);

            if(r.best_fitness < best_objective) {
                best_objective = r.best_fitness;
                best_seed = r.seed;
            }

            if(r.best_fitness > worst_objective) {
                worst_objective = r.best_fitness;
            }
        }

        file << name << ";"
             << objectives.size() << ";"
             << format_decimal(best_objective) << ";"
             << format_decimal(mean(objectives)) << ";"
             << format_decimal(worst_objective) << ";"
             << format_decimal(standard_deviation(objectives)) << ";"
             << format_decimal(mean(routes)) << ";"
             << format_decimal(mean(selected_stops)) << ";"
             << format_decimal(mean(algorithm_times)) << ";"
             << format_decimal(mean(total_route_times)) << ";"
             << format_decimal(mean(max_route_times)) << ";"
             << best_seed << "\n";
    }
}

void export_routes_csv(
    const vector<RouteRecord>& routes,
    const fs::path& output_file
) {
    ofstream file(output_file);

    if(!file) {
        throw runtime_error("Nao foi possivel criar a tabela de rotas: " + output_file.string());
    }

    file << "instancia;"
         << "arquivo;"
         << "execucao_global;"
         << "execucao;"
         << "seed;"
         << "threads;"
         << "rota;"
         << "melhor_rota_da_execucao;"
         << "melhor_rota_geral;"
         << "rota_da_melhor_solucao_global;"
         << "sequencia;"
         << "numero_paradas;"
         << "carga_estudantes;"
         << "distancia_km;"
         << "tempo_rota_segundos;"
         << "tempo_total_rotas_execucao_seg;"
         << "tempo_execucao_algoritmo_seg;"
         << "tempo_parede_execucao_seg\n";

    for(const auto& route : routes) {
        file << route.instance_name << ";"
             << route.instance_file << ";"
             << route.global_execution_id << ";"
             << route.execution_id << ";"
             << route.seed << ";"
             << route.threads << ";"
             << route.route_id << ";"
             << format_yes_no(route.best_route_in_execution) << ";"
             << format_yes_no(route.best_route_overall) << ";"
             << format_yes_no(route.from_global_best_solution) << ";"
             << "\"" << route.sequence << "\"" << ";"
             << route.number_stops << ";"
             << route.load << ";"
             << format_decimal(route.distance) << ";"
             << format_decimal(route.route_time) << ";"
             << format_decimal(route.total_route_time_in_execution) << ";"
             << format_decimal(route.algorithm_time) << ";"
             << format_decimal(route.wall_time) << "\n";
    }
}

// ============================================================================
// Listagem das instâncias .bus
// ============================================================================

vector<fs::path> list_bus_instances(const fs::path& data_dir) {
    vector<fs::path> instances;

    if(!fs::exists(data_dir)) {
        throw runtime_error("Diretorio de instancias nao encontrado: " + data_dir.string());
    }

    for(const auto& entry : fs::directory_iterator(data_dir)) {
        if(entry.is_regular_file() && entry.path().extension() == ".bus") {
            instances.push_back(entry.path());
        }
    }

    sort(instances.begin(), instances.end());

    return instances;
}

// ============================================================================
// Main experimental
// ============================================================================

int main(int argc, char* argv[]) {
    try {
        if(argc < 2) {
            cerr << "Uso:\n"
                 << "  " << argv[0]
                 << " <arquivo_configuracao> [execucoes_por_instancia] [master_seed] [threads] [diretorio_instancias]\n\n";

            cerr << "Exemplo:\n"
                 << "  " << argv[0]
                 << " .\\config.conf 10 123 1 .\\data\n\n";

            cerr << "Observacao:\n"
                 << "  master_seed nao e a seed usada diretamente no BRKGA.\n"
                 << "  Ela e usada para gerar seeds aleatorias, sem repeticao, para cada execucao.\n";

            return 1;
        }

        const string config_file = argv[1];

        const unsigned runs_per_instance =
            argc >= 3 ? static_cast<unsigned>(stoul(argv[2])) : 10u;

        const unsigned master_seed =
            argc >= 4 ? static_cast<unsigned>(stoul(argv[3])) : 123u;

        const unsigned num_threads =
            argc >= 5 ? static_cast<unsigned>(stoul(argv[4])) : 1u;

        const fs::path data_dir =
            argc >= 6 ? fs::path(argv[5]) : fs::path("data");

        const string date_stamp = get_current_date_stamp();
        const double maximum_running_time =
            read_maximum_running_time_seconds(config_file);
        const string running_time_tag =
            build_running_time_filename_tag(maximum_running_time);

        const fs::path results_dir = "results";
        const fs::path figures_dir = results_dir / "figures";
        const fs::path logs_dir    = results_dir / "logs";
        const fs::path tables_dir  = results_dir / "tables";

        fs::create_directories(figures_dir);
        fs::create_directories(logs_dir);
        fs::create_directories(tables_dir);

        const fs::path detailed_table_path =
            tables_dir / (date_stamp + "experimentos_" + running_time_tag + "_detalhados_table.csv");

        const fs::path summary_table_path =
            tables_dir / (date_stamp + "experimentos_" + running_time_tag + "_resumo_table.csv");

        const fs::path all_routes_table_path =
            tables_dir / (date_stamp + "experimentos_" + running_time_tag + "_todas_rotas_table.csv");

        const fs::path best_routes_table_path =
            tables_dir / (date_stamp + "experimentos_" + running_time_tag + "_melhor_solucao_rotas_table.csv");

        const fs::path experiment_log_path =
            logs_dir / (date_stamp + "experimentos_" + running_time_tag + "_logs.txt");

        const fs::path best_graph_dot_path =
            figures_dir / (date_stamp + "experimentos_" + running_time_tag + "_melhor_solucao_figure.dot");

        const fs::path best_graph_png_path =
            figures_dir / (date_stamp + "experimentos_" + running_time_tag + "_melhor_solucao_figure.png");

        ofstream experiment_log(experiment_log_path);

        if(!experiment_log) {
            throw runtime_error("Nao foi possivel criar o log dos experimentos.");
        }

        auto log = [&](const string& message) {
            cout << message;
            experiment_log << message;
        };

        cout << fixed << setprecision(3);
        experiment_log << fixed << setprecision(3);

        log("Iniciando bateria experimental do PROE com BRKGA.\n\n");

        log("Configuracao geral:\n");
        log("  Arquivo de configuracao: " + config_file + "\n");
        log("  maximum_running_time: " + format_decimal(maximum_running_time, 1) + " s\n");
        log("  Identificador nos arquivos de saida: " + running_time_tag + "\n");
        log("  Diretorio de instancias: " + data_dir.string() + "\n");
        log("  Execucoes por instancia: " + to_string(runs_per_instance) + "\n");
        log("  Master seed para geracao das seeds aleatorias: " + to_string(master_seed) + "\n");
        log("  Seeds das execucoes serao geradas aleatoriamente, sem repeticao.\n");
        log("  Threads: " + to_string(num_threads) + "\n\n");

        vector<fs::path> instance_files = list_bus_instances(data_dir);

        if(instance_files.empty()) {
            throw runtime_error("Nenhuma instancia .bus foi encontrada em: " + data_dir.string());
        }

        log("Instancias encontradas:\n");

        for(const auto& file : instance_files) {
            log("  - " + file.string() + "\n");
        }

        log("\n");

        const unsigned total_executions =
            static_cast<unsigned>(instance_files.size()) * runs_per_instance;

        const vector<unsigned> random_seeds =
            generate_random_seeds(total_executions, master_seed);

        unsigned seed_index = 0;

        log("Total de execucoes planejadas: " + to_string(total_executions) + "\n");
        log("Seeds aleatorias geradas com sucesso.\n\n");

        vector<RunResult> all_results;
        vector<RouteRecord> all_route_records;

        double global_best_fitness = numeric_limits<double>::infinity();
        string global_best_instance_name;
        unsigned global_best_seed = 0;
        RunResult global_best_result;

        PROEDecoder::Solution global_best_solution;
        unique_ptr<Instance> global_best_instance;

        unsigned global_execution_counter = 0;

        for(size_t i = 0; i < instance_files.size(); ++i) {
            const fs::path instance_path = instance_files[i];
            const string instance_name = instance_path.stem().string();

            log("============================================================\n");
            log("Instancia: " + instance_name + "\n");
            log("Arquivo: " + instance_path.string() + "\n");
            log("============================================================\n\n");

            for(unsigned run = 0; run < runs_per_instance; ++run) {
                const unsigned seed = random_seeds[seed_index++];

                global_execution_counter++;

                ostringstream start_msg;
                start_msg << "Execucao global " << global_execution_counter
                          << "/" << total_executions
                          << " | instancia " << instance_name
                          << " | execucao " << (run + 1)
                          << "/" << runs_per_instance
                          << " | seed aleatoria = " << seed
                          << " ...\n";

                log(start_msg.str());

                const auto wall_start = chrono::steady_clock::now();

                auto [brkga_params, control_params] =
                    BRKGA::readConfiguration(config_file);

                Instance instance(instance_path.string());

                const unsigned chromosome_size =
                    static_cast<unsigned>(instance.stops.size());

                if(chromosome_size <= 1) {
                    throw runtime_error(
                        "Instancia com tamanho de cromossomo invalido: " + instance_name
                    );
                }

                PROEDecoder decoder(instance);

                BRKGA::BRKGA_MP_IPR<PROEDecoder> algorithm(
                    decoder,
                    BRKGA::Sense::MINIMIZE,
                    seed,
                    chromosome_size,
                    brkga_params,
                    num_threads,
                    true
                );

                const auto final_status =
                    algorithm.run(control_params);

                const PROEDecoder::Solution solution =
                    decoder.decodeSolution(final_status.best_chromosome);

                const auto wall_end = chrono::steady_clock::now();

                const double wall_time =
                    chrono::duration<double>(wall_end - wall_start).count();

                RunResult result;
                result.instance_name = instance_name;
                result.instance_file = instance_path.string();
                result.global_execution_id = global_execution_counter;
                result.execution_id = run + 1;
                result.seed = seed;
                result.threads = num_threads;

                result.best_fitness = final_status.best_fitness;
                result.total_distance = solution.total_distance;
                result.total_penalty = solution.total_penalty;
                result.number_routes = solution.number_routes;

                result.selected_stops = 0;
                result.total_load = 0;
                result.total_route_time = 0.0;
                result.max_route_time = 0.0;

                for(bool selected : solution.selected_stops) {
                    if(selected) {
                        result.selected_stops++;
                    }
                }

                for(const auto& route : solution.routes) {
                    result.total_load += route.load;
                    result.total_route_time += route.time;
                    result.max_route_time = max(result.max_route_time, route.time);
                }

                if(solution.number_routes > 0) {
                    result.average_route_time =
                        result.total_route_time / static_cast<double>(solution.number_routes);
                }

                result.current_iteration =
                    static_cast<unsigned>(final_status.current_iteration);

                result.last_update_iteration =
                    static_cast<unsigned>(final_status.last_update_iteration);

                result.algorithm_time = final_status.current_time.count();
                result.wall_time = wall_time;

                const size_t best_route_index =
                    find_best_route_index(solution.routes);

                for(size_t route_index = 0; route_index < solution.routes.size(); ++route_index) {
                    const auto& route = solution.routes[route_index];

                    RouteRecord route_record;
                    route_record.instance_name = result.instance_name;
                    route_record.instance_file = result.instance_file;
                    route_record.global_execution_id = result.global_execution_id;
                    route_record.execution_id = result.execution_id;
                    route_record.seed = result.seed;
                    route_record.threads = result.threads;
                    route_record.route_id = static_cast<unsigned>(route_index + 1);
                    route_record.best_route_in_execution =
                        route_index == best_route_index;
                    route_record.sequence = build_route_sequence(route);
                    route_record.number_stops = route.stops.size();
                    route_record.load = route.load;
                    route_record.distance = route.distance;
                    route_record.route_time = route.time;
                    route_record.total_route_time_in_execution =
                        result.total_route_time;
                    route_record.algorithm_time = result.algorithm_time;
                    route_record.wall_time = result.wall_time;

                    all_route_records.push_back(route_record);
                }

                all_results.push_back(result);

                ostringstream end_msg;
                end_msg << "  Resultado: FO = " << result.best_fitness
                        << " | distancia = " << result.total_distance << " km"
                        << " | rotas = " << result.number_routes
                        << " | paradas selecionadas = " << result.selected_stops
                        << " | tempo algoritmo = " << result.algorithm_time << " s"
                        << " | tempo total das rotas = " << result.total_route_time << " s"
                        << " | maior tempo de rota = " << result.max_route_time << " s"
                        << "\n\n";

                log(end_msg.str());

                if(result.best_fitness < global_best_fitness) {
                    global_best_fitness = result.best_fitness;
                    global_best_instance_name = instance_name;
                    global_best_seed = seed;
                    global_best_result = result;
                    global_best_solution = solution;
                    global_best_instance = make_unique<Instance>(instance_path.string());
                }
            }
        }

        log("============================================================\n");
        log("Exportando resultados consolidados.\n");
        log("============================================================\n\n");

        if(global_best_instance == nullptr) {
            throw runtime_error("Nenhuma melhor solucao global foi armazenada.");
        }

        for(auto& route_record : all_route_records) {
            route_record.from_global_best_solution =
                route_record.global_execution_id == global_best_result.global_execution_id;
        }

        const size_t best_route_record_index =
            find_best_route_record_index(all_route_records);

        if(best_route_record_index != numeric_limits<size_t>::max()) {
            all_route_records[best_route_record_index].best_route_overall = true;
        }

        vector<RouteRecord> global_best_route_records;

        for(const auto& route_record : all_route_records) {
            if(route_record.from_global_best_solution) {
                global_best_route_records.push_back(route_record);
            }
        }

        export_detailed_results_csv(all_results, detailed_table_path);
        export_summary_by_instance_csv(all_results, summary_table_path);
        export_routes_csv(all_route_records, all_routes_table_path);
        export_routes_csv(global_best_route_records, best_routes_table_path);

        export_solution_graph_png(
            global_best_solution,
            *global_best_instance,
            best_graph_dot_path,
            best_graph_png_path
        );

        log("Tabela detalhada salva em: " + detailed_table_path.string() + "\n");
        log("Tabela resumo por instancia salva em: " + summary_table_path.string() + "\n");
        log("Tabela com todas as rotas salva em: " + all_routes_table_path.string() + "\n");
        log("Tabela de rotas da melhor solucao salva em: " + best_routes_table_path.string() + "\n");
        log("Grafo da melhor solucao salvo em: " + best_graph_png_path.string() + "\n");
        log("Log dos experimentos salvo em: " + experiment_log_path.string() + "\n\n");

        ostringstream best_msg;
        best_msg << "Melhor solucao global:\n"
                 << "  Instancia: " << global_best_instance_name << "\n"
                 << "  Execucao global: " << global_best_result.global_execution_id << "\n"
                 << "  Execucao na instancia: " << global_best_result.execution_id << "\n"
                 << "  Seed aleatoria: " << global_best_seed << "\n"
                 << "  Funcao objetivo: " << format_decimal(global_best_solution.fitness) << "\n"
                 << "  Distancia total: " << format_decimal(global_best_solution.total_distance) << " km\n"
                 << "  Numero de rotas: " << global_best_solution.number_routes << "\n"
                 << "  Penalidade: " << format_decimal(global_best_solution.total_penalty) << "\n\n";

        log(best_msg.str());

        if(best_route_record_index != numeric_limits<size_t>::max()) {
            const auto& best_route = all_route_records[best_route_record_index];

            ostringstream best_route_msg;
            best_route_msg << "Melhor rota especifica "
                           << "(menor distancia; desempate por menor tempo):\n"
                           << "  Instancia: " << best_route.instance_name << "\n"
                           << "  Execucao global: " << best_route.global_execution_id << "\n"
                           << "  Execucao na instancia: " << best_route.execution_id << "\n"
                           << "  Seed aleatoria: " << best_route.seed << "\n"
                           << "  Rota: " << best_route.route_id << "\n"
                           << "  Distancia: " << format_decimal(best_route.distance) << " km\n"
                           << "  Tempo da rota: " << format_decimal(best_route.route_time) << " s\n"
                           << "  Sequencia: " << best_route.sequence << "\n\n";

            log(best_route_msg.str());
        }

        log("Bateria experimental finalizada com sucesso.\n");

        return 0;
    }
    catch(const exception& e) {
        cerr << "\nERRO: " << e.what() << "\n";
        return 1;
    }
}

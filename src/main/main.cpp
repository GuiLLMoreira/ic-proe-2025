#include "../../instance.hpp"
#include "../decoder/PROEDecoder.hpp"
#include "brkga_mp_ipr.hpp"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace BRKGA;

int main(int argc, char* argv[]) {
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

        cout << fixed << setprecision(3);

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

        const auto final_status =
            algorithm.run(control_params, &cout);

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

        if(best_solution.total_penalty > 0.0) {
            cout << "Penalidades aplicadas: "
                 << best_solution.total_penalty << "\n";
        }
        else {
            cout << "Penalidades aplicadas: 0.000\n";
        }

        cout << "\n";

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

        cout << "\n";

        return 0;
    }
    catch(const exception& e) {
        cerr << "\nERRO: " << e.what() << "\n";
        return 1;
    }
}















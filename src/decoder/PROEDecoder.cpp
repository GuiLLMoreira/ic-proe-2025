#include "PROEDecoder.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

PROEDecoder::PROEDecoder(const Instance& instance):
    instance(instance)
{}

double PROEDecoder::decode(vector<double>& chromosome, bool /*rewrite*/) {
    if(chromosome.size() != instance.stops.size()) {
        throw runtime_error("O tamanho do cromossomo deve ser igual ao número de paradas candidatas.");
    }

    Solution solution = decodeSolution(chromosome);
    return solution.fitness;
}

PROEDecoder::Solution PROEDecoder::decodeSolution(const vector<double>& chromosome) const {
    if(chromosome.size() != instance.stops.size()) {
        throw runtime_error("O tamanho do cromossomo deve ser igual ao número de paradas candidatas.");
    }

    if(instance.is_bus_format) {
        return decode_bus_instance(chromosome);
    }

    return decode_simple_instance(chromosome);
}

PROEDecoder::Solution PROEDecoder::decode_simple_instance(const vector<double>& chromosome) const {
    const unsigned n = static_cast<unsigned>(instance.stops.size());

    Solution solution;
    solution.selected_stops.assign(n, false);
    solution.demand_by_stop.assign(n, 0);

    vector<pair<double, unsigned>> ranking;
    ranking.reserve(n);

    for(unsigned i = 0; i < n; ++i) {
        ranking.emplace_back(chromosome[i], i);
    }

    sort(ranking.begin(), ranking.end());

    vector<unsigned> ordered_stops;
    ordered_stops.reserve(n);

    for(const auto& item : ranking) {
        const unsigned stop = item.second;

        if(instance.stops[stop].demand > 0) {
            solution.selected_stops[stop] = true;
            solution.demand_by_stop[stop] = instance.stops[stop].demand;
            ordered_stops.push_back(stop);
        }
    }

    build_routes(ordered_stops, solution.demand_by_stop, solution);

    solution.fitness = solution.total_distance + solution.total_penalty;

    return solution;
}

PROEDecoder::Solution PROEDecoder::decode_bus_instance(const vector<double>& chromosome) const {
    const unsigned num_stops = static_cast<unsigned>(instance.stops.size());
    const unsigned num_addresses = static_cast<unsigned>(instance.addresses.size());

    Solution solution;
    solution.selected_stops.assign(num_stops, false);
    solution.demand_by_stop.assign(num_stops, 0);

    // ------------------------------------------------------------------
    // 1. Seleção de paradas
    //
    // Para cada endereço, verifica se ele já está coberto por alguma
    // parada selecionada. Caso não esteja, abre a parada viável de menor
    // chave BRKGA.
    // ------------------------------------------------------------------
    for(unsigned a = 0; a < num_addresses; ++a) {
        bool already_covered = false;

        for(const auto& walk : instance.feasible_walks_by_address[a]) {
            if(walk.stop_index == 0) {
                continue;
            }

            const unsigned local_stop = walk.stop_index - 1;

            if(local_stop < num_stops && solution.selected_stops[local_stop]) {
                already_covered = true;
                break;
            }
        }

        if(already_covered) {
            continue;
        }

        double best_key = numeric_limits<double>::infinity();
        int best_stop = -1;

        for(const auto& walk : instance.feasible_walks_by_address[a]) {
            if(walk.stop_index == 0) {
                continue;
            }

            const unsigned local_stop = walk.stop_index - 1;

            if(local_stop >= num_stops) {
                continue;
            }

            if(chromosome[local_stop] < best_key) {
                best_key = chromosome[local_stop];
                best_stop = static_cast<int>(local_stop);
            }
        }

        if(best_stop >= 0) {
            solution.selected_stops[best_stop] = true;
        }
        else {
            solution.total_penalty +=
                instance.penalty_infeasible * instance.addresses[a].students;
        }
    }

    // ------------------------------------------------------------------
    // 2. Alocação dos estudantes
    //
    // Cada endereço é alocado à parada selecionada mais próxima dentre
    // suas caminhadas viáveis.
    // ------------------------------------------------------------------
    for(unsigned a = 0; a < num_addresses; ++a) {
        double best_walk_distance = numeric_limits<double>::infinity();
        int best_stop = -1;

        for(const auto& walk : instance.feasible_walks_by_address[a]) {
            if(walk.stop_index == 0) {
                continue;
            }

            const unsigned local_stop = walk.stop_index - 1;

            if(local_stop >= num_stops) {
                continue;
            }

            if(!solution.selected_stops[local_stop]) {
                continue;
            }

            if(walk.distance < best_walk_distance) {
                best_walk_distance = walk.distance;
                best_stop = static_cast<int>(local_stop);
            }
        }

        if(best_stop >= 0) {
            solution.demand_by_stop[best_stop] += instance.addresses[a].students;
        }
        else {
            solution.total_penalty +=
                instance.penalty_infeasible * instance.addresses[a].students;
        }
    }

    // ------------------------------------------------------------------
    // 3. Ordenação das paradas selecionadas
    //
    // A ordem de visita é definida pela ordenação crescente das random keys.
    // ------------------------------------------------------------------
    vector<pair<double, unsigned>> ranking;

    for(unsigned s = 0; s < num_stops; ++s) {
        if(solution.selected_stops[s] && solution.demand_by_stop[s] > 0) {
            ranking.emplace_back(chromosome[s], s);
        }
    }

    sort(ranking.begin(), ranking.end());

    vector<unsigned> ordered_selected_stops;
    ordered_selected_stops.reserve(ranking.size());

    for(const auto& item : ranking) {
        ordered_selected_stops.push_back(item.second);
    }

    // ------------------------------------------------------------------
    // 4. Construção das rotas
    // ------------------------------------------------------------------
    build_routes(ordered_selected_stops, solution.demand_by_stop, solution);

    solution.fitness = solution.total_distance + solution.total_penalty;

    return solution;
}

void PROEDecoder::build_routes(
    const vector<unsigned>& ordered_selected_stops,
    const vector<int>& demand_by_stop,
    Solution& solution
) const {
    if(ordered_selected_stops.empty()) {
        return;
    }

    Route current_route;

    bool route_open = false;
    unsigned previous_stop = 0;

    auto close_route = [&]() {
        if(route_open) {
            current_route.distance += instance.distance_from_stop_to_school(previous_stop);
            current_route.time += instance.time_from_stop_to_school(previous_stop);

            if(instance.max_route_time > 0.0 &&
               current_route.time > instance.max_route_time) {
                solution.total_penalty +=
                    instance.penalty_time *
                    (current_route.time - instance.max_route_time);
            }

            solution.total_distance += current_route.distance;
            solution.routes.push_back(current_route);
            solution.number_routes++;

            current_route = Route{};
            route_open = false;
        }
    };

    for(unsigned stop : ordered_selected_stops) {
        const int demand = demand_by_stop[stop];

        if(demand <= 0) {
            continue;
        }

        // Caso extremo: uma única parada possui mais alunos que a capacidade.
        // A rota é mantida, mas a solução é penalizada.
        if(demand > instance.vehicle_capacity) {
            solution.total_penalty +=
                instance.penalty_capacity *
                static_cast<double>(demand - instance.vehicle_capacity);
        }

        // Se a próxima parada não couber na rota atual, fecha a rota
        // e retorna para a escola.
        if(route_open && current_route.load + demand > instance.vehicle_capacity) {
            close_route();
        }

        // Abre uma nova rota saindo da escola.
        if(!route_open) {
            current_route = Route{};

            current_route.distance += instance.distance_from_school_to_stop(stop);
            current_route.time += instance.time_from_school_to_stop(stop);

            route_open = true;
        }
        else {
            current_route.distance += instance.distance_between_stops(previous_stop, stop);
            current_route.time += instance.time_between_stops(previous_stop, stop);
        }

        current_route.stops.push_back(stop);
        current_route.load += demand;

        previous_stop = stop;
    }

    close_route();
}
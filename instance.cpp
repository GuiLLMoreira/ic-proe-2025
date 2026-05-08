#include "instance.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {
    vector<string> split_csv(const string& line) {
        vector<string> tokens;
        string token;
        stringstream ss(line);

        while(getline(ss, token, ',')) {
            while(!token.empty() && (token.front() == ' ' || token.front() == '\t')) {
                token.erase(token.begin());
            }

            while(!token.empty() &&
                  (token.back() == ' ' || token.back() == '\t' || token.back() == '\r')) {
                token.pop_back();
            }

            tokens.push_back(token);
        }

        return tokens;
    }

    bool looks_like_bus_file(const string& first_line) {
        auto tokens = split_csv(first_line);

        if(tokens.size() < 6) {
            return false;
        }

        // No formato .bus, o quarto campo costuma ser K ou M.
        return tokens[3] == "K" || tokens[3] == "M";
    }
}

Instance::Instance(const string& filename):
    school(),
    school_name(),
    stops(),
    addresses(),
    feasible_walks_by_address(),
    driving_distance(),
    driving_time(),
    vehicle_capacity(40),
    penalty_infeasible(1e6),
    penalty_capacity(1e5),
    max_route_time(0.0),
    penalty_time(1e3),
    min_eligibility_distance(0.0),
    max_walking_distance(0.0),
    is_bus_format(false)
{
    ifstream file(filename);
    if(!file) {
        throw runtime_error("Cannot open instance file: " + filename);
    }

    string first_line;
    getline(file, first_line);
    file.close();

    if(looks_like_bus_file(first_line)) {
        is_bus_format = true;
        load_bus_format(filename);
    }
    else {
        is_bus_format = false;
        load_simple_format(filename);
    }
}

void Instance::load_simple_format(const string& filename) {
    ifstream file(filename, ios::in);
    if(!file) {
        throw runtime_error("Cannot open simple instance file: " + filename);
    }

    unsigned num_stops{};
    file >> num_stops;

    file >> school.x >> school.y;

    stops.clear();
    stops.resize(num_stops);

    for(unsigned i = 0; i < num_stops; ++i) {
        file >> stops[i].coord.x
             >> stops[i].coord.y
             >> stops[i].demand;

        stops[i].name = "stop_" + to_string(i);
    }

    // Parâmetros opcionais no formato simples:
    //
    // vehicle_capacity penalty_capacity penalty_infeasible max_route_time penalty_time
    //
    // Se nem todos estiverem presentes, mantém os valores padrão.
    if(file >> vehicle_capacity) {
        if(file >> penalty_capacity) {
            if(file >> penalty_infeasible) {
                if(file >> max_route_time) {
                    file >> penalty_time;
                }
            }
        }
    }

    addresses.clear();
    feasible_walks_by_address.clear();
    driving_distance.clear();
    driving_time.clear();
}

void Instance::load_bus_format(const string& filename) {
    ifstream file(filename, ios::in);
    if(!file) {
        throw runtime_error("Cannot open .bus instance file: " + filename);
    }

    string line;

    // -------------------------
    // Primeira linha
    // -------------------------
    getline(file, line);
    auto header = split_csv(line);

    if(header.size() < 6) {
        throw runtime_error("Invalid .bus header.");
    }

    const unsigned num_locations = static_cast<unsigned>(stoul(header[0])); // inclui escola
    const unsigned num_addresses = static_cast<unsigned>(stoul(header[1]));
    const unsigned num_walks     = static_cast<unsigned>(stoul(header[2]));

    min_eligibility_distance = stod(header[4]);
    max_walking_distance     = stod(header[5]);

    if(num_locations <= 1) {
        throw runtime_error(".bus instance must have at least one school and one stop.");
    }

    // -------------------------
    // Linha da escola
    // -------------------------
    getline(file, line);
    auto school_tokens = split_csv(line);

    if(school_tokens.size() < 4 || school_tokens[0] != "s") {
        throw runtime_error("Invalid school line in .bus file.");
    }

    school.x = stod(school_tokens[1]); // latitude
    school.y = stod(school_tokens[2]); // longitude
    school_name = school_tokens[3];

    // -------------------------
    // Linhas das paradas
    // -------------------------
    stops.clear();
    stops.resize(num_locations - 1);

    for(unsigned i = 0; i < num_locations - 1; ++i) {
        getline(file, line);
        auto tokens = split_csv(line);

        if(tokens.size() < 4 || tokens[0] != "s") {
            throw runtime_error("Invalid stop line in .bus file.");
        }

        stops[i].coord.x = stod(tokens[1]);
        stops[i].coord.y = stod(tokens[2]);
        stops[i].demand = 0;
        stops[i].name = tokens[3];
    }

    // -------------------------
    // Linhas dos endereços
    // -------------------------
    addresses.clear();
    addresses.resize(num_addresses);

    for(unsigned i = 0; i < num_addresses; ++i) {
        getline(file, line);
        auto tokens = split_csv(line);

        if(tokens.size() < 5 || tokens[0] != "a") {
            throw runtime_error("Invalid address line in .bus file.");
        }

        addresses[i].coord.x = stod(tokens[1]);
        addresses[i].coord.y = stod(tokens[2]);
        addresses[i].students = stoi(tokens[3]);
        addresses[i].name = tokens[4];
    }

    // -------------------------
    // Matriz de distâncias e tempos dirigidos
    // -------------------------
    const double INF = numeric_limits<double>::infinity();

    driving_distance.assign(
        num_locations,
        vector<double>(num_locations, INF)
    );

    driving_time.assign(
        num_locations,
        vector<double>(num_locations, INF)
    );

    const unsigned total_distance_lines = num_locations * num_locations;

    for(unsigned k = 0; k < total_distance_lines; ++k) {
        getline(file, line);
        auto tokens = split_csv(line);

        if(tokens.size() < 5 || tokens[0] != "d") {
            throw runtime_error("Invalid driving distance line in .bus file.");
        }

        const unsigned from = static_cast<unsigned>(stoul(tokens[1]));
        const unsigned to   = static_cast<unsigned>(stoul(tokens[2]));

        const double dist = stod(tokens[3]);
        const double time = stod(tokens[4]);

        if(from >= num_locations || to >= num_locations) {
            throw runtime_error("Driving distance index out of range.");
        }

        driving_distance[from][to] = dist;
        driving_time[from][to] = time;
    }

    // -------------------------
    // Caminhadas viáveis
    // -------------------------
    feasible_walks_by_address.clear();
    feasible_walks_by_address.resize(num_addresses);

    for(unsigned k = 0; k < num_walks; ++k) {
        getline(file, line);
        auto tokens = split_csv(line);

        if(tokens.size() < 5 || tokens[0] != "w") {
            throw runtime_error("Invalid walking line in .bus file.");
        }

        Walk w;
        w.address_index = static_cast<unsigned>(stoul(tokens[1]));
        w.stop_index    = static_cast<unsigned>(stoul(tokens[2]));
        w.distance      = stod(tokens[3]);
        w.time          = stod(tokens[4]);

        if(w.address_index >= num_addresses) {
            throw runtime_error("Walk address index out of range.");
        }

        if(w.stop_index >= num_locations) {
            throw runtime_error("Walk stop index out of range.");
        }

        feasible_walks_by_address[w.address_index].push_back(w);
    }

    // Parâmetros padrão para as instâncias reais.
    // A função objetivo principal continuará sendo a distância total percorrida.
    vehicle_capacity = 40;
    penalty_capacity = 1e5;
    penalty_infeasible = 1e6;

    // Por padrão, não aplica restrição de tempo máximo.
    // A matriz de tempos já é carregada para extensão futura.
    max_route_time = 0.0;
    penalty_time = 1e3;
}

double Instance::euclidean_distance(const Point& a, const Point& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;

    return sqrt(dx * dx + dy * dy);
}

double Instance::distance_from_school_to_stop(unsigned stop_local_index) const {
    if(stop_local_index >= stops.size()) {
        throw out_of_range("stop_local_index out of range.");
    }

    if(is_bus_format) {
        return driving_distance[0][stop_local_index + 1];
    }

    return euclidean_distance(school, stops[stop_local_index].coord);
}

double Instance::distance_from_stop_to_school(unsigned stop_local_index) const {
    if(stop_local_index >= stops.size()) {
        throw out_of_range("stop_local_index out of range.");
    }

    if(is_bus_format) {
        return driving_distance[stop_local_index + 1][0];
    }

    return euclidean_distance(stops[stop_local_index].coord, school);
}

double Instance::distance_between_stops(unsigned from_local_index, unsigned to_local_index) const {
    if(from_local_index >= stops.size() || to_local_index >= stops.size()) {
        throw out_of_range("stop index out of range.");
    }

    if(is_bus_format) {
        return driving_distance[from_local_index + 1][to_local_index + 1];
    }

    return euclidean_distance(stops[from_local_index].coord, stops[to_local_index].coord);
}

double Instance::time_from_school_to_stop(unsigned stop_local_index) const {
    if(stop_local_index >= stops.size()) {
        throw out_of_range("stop_local_index out of range.");
    }

    if(is_bus_format) {
        return driving_time[0][stop_local_index + 1];
    }

    // No formato simples, não há tempo.
    // Usa distância como aproximação neutra.
    return distance_from_school_to_stop(stop_local_index);
}

double Instance::time_from_stop_to_school(unsigned stop_local_index) const {
    if(stop_local_index >= stops.size()) {
        throw out_of_range("stop_local_index out of range.");
    }

    if(is_bus_format) {
        return driving_time[stop_local_index + 1][0];
    }

    return distance_from_stop_to_school(stop_local_index);
}

double Instance::time_between_stops(unsigned from_local_index, unsigned to_local_index) const {
    if(from_local_index >= stops.size() || to_local_index >= stops.size()) {
        throw out_of_range("stop index out of range.");
    }

    if(is_bus_format) {
        return driving_time[from_local_index + 1][to_local_index + 1];
    }

    return distance_between_stops(from_local_index, to_local_index);
}

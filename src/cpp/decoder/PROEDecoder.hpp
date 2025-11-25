#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>

// ---------------------
// Estruturas da instância
// ---------------------

struct Point {
    double x{}, y{};
};

struct Stop {
    Point coord;
    int demand{};
};

struct PROEInstance {
    Point school;
    std::vector<Stop> stops;
    int vehicle_capacity{40};
    double penalty_capacity{1e5};
    double penalty_open_route{1e3};
};

// ---------------------
// Decoder PROE / SBRP
// ---------------------

class PROEDecoder {
public:
    explicit PROEDecoder(const PROEInstance& inst) : inst_(inst) {
        if (inst_.stops.empty())
            throw std::runtime_error("PROEInstance: no stops.");
        if (inst_.vehicle_capacity <= 0)
            throw std::runtime_error("Vehicle capacity must be > 0.");
    }

    double decode(const std::vector<double>& c, bool /*write_back*/ = false) {
        const std::size_t n = inst_.stops.size();

        if (c.size() != n) {
            std::cerr << "[PROEDecoder] ERRO: chromosome.size() = "
                      << c.size() << " != n_stops = " << n << "\n";
            throw std::runtime_error("Chromosome size must equal number of stops.");
        }

        // 1) ordena índices das paradas pelas random keys
        order_.resize(n);
        for (std::size_t i = 0; i < n; ++i) order_[i] = static_cast<unsigned>(i);

        std::sort(order_.begin(), order_.end(),
                  [&](unsigned a, unsigned b){ return c[a] < c[b]; });

        auto dist = [](const Point& a, const Point& b) {
            const double dx = a.x - b.x;
            const double dy = a.y - b.y;
            return std::sqrt(dx*dx + dy*dy);
        };

        double total_cost = 0.0;
        int load = 0;
        Point last = inst_.school;
        bool route_open = false;

        // 2) constrói rotas sequenciais respeitando capacidade
        for (unsigned idx : order_) {
            const Stop& s = inst_.stops[idx];

            if (!route_open) {
                route_open = true;
                last = inst_.school;
                load = 0;
            }

            // se estourar, fecha rota e abre nova
            if (load + s.demand > inst_.vehicle_capacity) {
                total_cost += dist(last, inst_.school);
                if (load == 0)
                    total_cost += inst_.penalty_open_route; // rota degenerada

                route_open = true;
                last = inst_.school;
                load = 0;
            }

            total_cost += dist(last, s.coord);
            last = s.coord;
            load += s.demand;
        }

        // 3) fecha última rota
        if (route_open) {
            total_cost += dist(last, inst_.school);
            if (load == 0)
                total_cost += inst_.penalty_open_route;
        }

        // 4) segurança extra: se algo sobrar acima da capacidade
        if (load > inst_.vehicle_capacity) {
            total_cost += (load - inst_.vehicle_capacity) * inst_.penalty_capacity;
        }

        return total_cost;
    }

private:
    const PROEInstance& inst_;
    std::vector<unsigned> order_;
};


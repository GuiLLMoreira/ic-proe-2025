#ifndef PROE_DECODER_HPP
#define PROE_DECODER_HPP

#include "../../instance.hpp"

#include <vector>

class PROEDecoder {
public:
    struct Route {
        std::vector<unsigned> stops;
        int load{};
        double distance{};
        double time{};
    };

    struct Solution {
        double total_distance{};
        double total_penalty{};
        double fitness{};
        int number_routes{};

        std::vector<Route> routes;
        std::vector<bool> selected_stops;
        std::vector<int> demand_by_stop;
    };

public:
    explicit PROEDecoder(const Instance& instance);

    double decode(std::vector<double>& chromosome, bool rewrite);

    Solution decodeSolution(const std::vector<double>& chromosome) const;

private:
    const Instance& instance;

    Solution decode_simple_instance(const std::vector<double>& chromosome) const;
    Solution decode_bus_instance(const std::vector<double>& chromosome) const;

    void build_routes(
        const std::vector<unsigned>& ordered_selected_stops,
        const std::vector<int>& demand_by_stop,
        Solution& solution
    ) const;
};

#endif

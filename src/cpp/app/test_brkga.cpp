#include <iostream>
#include <clocale>
#include "brkga_mp_ipr.hpp"
#include "../decoder/PROEDecoder.hpp"

// Mini-instância de teste (8 paradas em cruz)
static PROEInstance build_tiny_instance() {
    PROEInstance I;
    I.school = {0.0, 0.0};
    I.stops = {
        {{ 1.0,  0.0}, 1}, {{ 2.0,  0.0}, 1},
        {{-1.0,  0.0}, 1}, {{-2.0,  0.0}, 1},
        {{ 0.0,  1.0}, 1}, {{ 0.0,  2.0}, 1},
        {{ 0.0, -1.0}, 1}, {{ 0.0, -2.0}, 1}
    };
    I.vehicle_capacity   = 3;
    I.penalty_capacity   = 1e6;
    I.penalty_open_route = 1e2;
    return I;
}

int main() {
    std::setlocale(LC_ALL, "C");

    // 1) Instância
    PROEInstance inst = build_tiny_instance();
    const unsigned CHR = static_cast<unsigned>(inst.stops.size());

    // 2) Decoder
    PROEDecoder decoder(inst);

    // 3) Parâmetros BRKGA
    BRKGA::BrkgaParams params;
    params.population_size             = 400;
    params.elite_percentage            = 0.30;
    params.mutants_percentage          = 0.05;
    params.num_elite_parents           = 1;
    params.total_parents               = 2;
    params.bias_type                   = BRKGA::BiasFunctionType::LOGINVERSE;
    params.num_independent_populations = 1;

    // >>> parâmetros específicos da sua versão do BRKGA_MP_IPR
    params.alpha_block_size = 1.0;   // > 0.0
    params.pr_percentage    = 1.0;   // ∈ (0,1], ex: 1.0 ou 0.5

    std::cout << "Pop=" << params.population_size
              << " | elite%=" << params.elite_percentage
              << " | elite_set=" << int(params.population_size * params.elite_percentage)
              << " | elite_parents=" << params.num_elite_parents
              << " | total_parents=" << params.total_parents
              << " | pops=" << params.num_independent_populations
              << " | CHR=" << CHR
              << std::endl;

    std::cout << "Antes de criar o BRKGA\n";

    BRKGA::BRKGA_MP_IPR<PROEDecoder> brkga(
        decoder,
        BRKGA::Sense::MINIMIZE,
        params.num_independent_populations,
        CHR,
        params,
        42u,
        false
    );

    std::cout << "BRKGA criado com sucesso!\n";

    try {
        double best = brkga.getBestFitness();
        std::cout << "Fitness inicial: " << best << '\n';

        for (unsigned g = 0; g < 100; ++g) {
            brkga.evolve();
            double cur = brkga.getBestFitness();
            if (cur < best) best = cur;

            if ((g + 1) % 10 == 0) {
                std::cout << "Geracao " << (g + 1)
                          << " | Melhor = " << best << '\n';
            }
        }

        std::cout << "\nFinalizado. Melhor valor encontrado: " << best << '\n';

        const auto& bestChrom = brkga.getBestChromosome();
        std::cout << "Melhor cromossomo (random keys): ";
        for (double g : bestChrom) std::cout << g << ' ';
        std::cout << '\n';

    } catch (const std::exception& e) {
        std::cerr << "Erro durante execucao: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}













#include <iostream>       // entrada/saída padrão (std::cout, std::cerr)
#include <vector>         // std::vector usado para cromossomos e coleções
#include <cmath>          // funções matemáticas (não usado explicitamente aqui, mas comumente incluído)
#include <random>         // geradores aleatórios (pode ser usado dentro da biblioteca BRKGA)
#include <clocale>        // setlocale para configurar localidade (separador decimal, etc.)
#include "brkga_mp_ipr.hpp" // cabeçalho da implementação da biblioteca BRKGA usada (implementação própria/externa)

class QuadraticDecoder {
public:
    // Método que decodifica um cromossomo (vetor de genes) para calcular o fitness.
    // Aqui assume-se que o cromossomo tem tamanho 2 e o problema é minimizar x^2
    double decode(const std::vector<double>& c, bool /*write_back*/ = false) {
        if (c.size() != 2) throw std::runtime_error("Chromosome size must be 2");
        // converte o gene [0,1] para o intervalo [-5,5]
        double x = c[0] * 10.0 - 5.0;
        // retorna o valor objetivo x^2 (problema de minimização)
        return x * x;
    }
};

int main() {
    std::setlocale(LC_ALL, "C"); // configura a localidade para "C" (garante separador decimal "." em impressões)

    constexpr unsigned CHR = 2; // const expressando o tamanho do cromossomo (número de genes)
    static_assert(CHR >= 2, "Chromosome size must be >= 2"); // verificação em tempo de compilação: CHR >= 2

    QuadraticDecoder decoder; // instancia o decodificador (função objetivo)

    BRKGA::BrkgaParams params; // estrutura de parâmetros da biblioteca BRKGA
    params.population_size = 400;               // tamanho da população total
    params.elite_percentage = 0.30;             // porcentagem de elite (30% da população será considerada elite)
    params.mutants_percentage = 0.05;           // porcentagem de mutantes introduzidos por geração (5%)
    params.num_elite_parents = 1;               // número de pais da elite usados na recombinação (por indivíduo filho)
    params.total_parents = 2;                   // número total de pais usados na recombinação (mínimo exigido é 2)
    params.bias_type = BRKGA::BiasFunctionType::LOGINVERSE; // função de bias para seleção entre pais (loginverse)
    params.num_independent_populations = 1;     // número de populações independentes (multi-população)
    params.num_independent_populations = 1;     // (linha repetida; é redundante mas inofensiva)
    params.alpha_block_size = 1.0; // <-- adicione esta linha (parâmetro específico da implementação usada)
    params.pr_percentage = 1.0; // ou outro valor entre 0.0 e 1.0, por exemplo 0.5 (parâmetro específico: prob. de reintrodução / repair)

    // imprime informações resumidas dos parâmetros configurados
    std::cout << "Pop=" << params.population_size
              << " | elite%=" << params.elite_percentage
              << " | elite_set=" << int(params.population_size * params.elite_percentage)
              << " | elite_parents=" << params.num_elite_parents
              << " | total_parents=" << params.total_parents
              << " | pops=" << params.num_independent_populations
              << " | CHR=" << CHR
              << std::endl;

    std::cout << "Antes de criar o BRKGA" << std::endl;

    // cria o objeto BRKGA com template do tipo de decodificador
    BRKGA::BRKGA_MP_IPR<QuadraticDecoder> brkga(
        decoder,                               // decodificador (avalia cromossomos)
        BRKGA::Sense::MINIMIZE,                // objetivo: minimizar a função fitness
        params.num_independent_populations,    // número de populações independentes
        CHR,                                   // comprimento do cromossomo (número de genes)
        params,                                // parâmetros configurados acima
        42u,                                   // semente do gerador aleatório (seed)
        false                                  // flag adicional (provavelmente verbose ou similar) — depende da API
    );

    std::cout << "BRKGA criado com sucesso!" << std::endl;
    brkga.evolve(); // Inicializa a população e executa uma primeira evolução (gera população inicial)

    // 🔹 Diagnóstico correto: usar métodos públicos
    std::cout << "Numero de populacoes: " << params.num_independent_populations << std::endl;

    try {
        // obtém o melhor fitness atual (após inicialização) — método público da classe BRKGA
        double best = brkga.getBestFitness();
        // obtém o cromossomo que produziu esse melhor fitness
        const auto& bestChrom = brkga.getBestChromosome();
        std::cout << "Fitness inicial: " << best << std::endl;
        std::cout << "Cromossomo inicial: ";
        for (auto g : bestChrom) std::cout << g << " "; // imprime cada gene do cromossomo
        std::cout << "\n";

        // loop que executa 50 evoluções (gerações) adicionais
        for (unsigned g = 0; g < 50; ++g) {
            brkga.evolve(); // gera a próxima geração (selecao, recombinacao, mutacao, substituicao)
            double cur = brkga.getBestFitness(); // obtém o melhor fitness da geração atual
            if (cur < best) best = cur;          // atualiza o melhor global se encontramos melhor valor (menor no caso)
            if ((g + 1) % 10 == 0)               // a cada 10 gerações, imprime um status
                std::cout << "Geracao " << (g + 1) << " | Melhor = " << best << '\n';
        }

        std::cout << "\nFinalizado. Melhor valor encontrado: " << best << '\n';
        const auto& best_final = brkga.getBestChromosome(); // obtém o melhor cromossomo final
        std::cout << "Melhor cromossomo: ";
        for (auto g : best_final) std::cout << g << " "; // imprime genes do melhor cromossomo encontrado
        std::cout << "\n";
    }
    catch (const std::exception& e) {
        // tratamento de exceções: imprime mensagem de erro e retorna código de falha
        std::cerr << "Erro durante execucao: " << e.what() << std::endl;
        return 1;
    }

    return 0; // término bem-sucedido do programa
}












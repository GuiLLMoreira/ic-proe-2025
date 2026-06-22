#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <string>
#include <vector>

class Instance {
public:
    // ---------------------
    // Estruturas básicas
    // ---------------------

    struct Point {
        double x{};
        double y{};
    };

    struct Stop {
        Point coord;
        int demand{};
        std::string name;
    };

    struct Address {
        Point coord;
        int students{};
        std::string name;
    };

    struct Walk {
        unsigned address_index{};
        unsigned stop_index{};      // índice original da instância .bus: 0 = escola, 1..|V1|-1 = paradas
        double distance{};
        double time{};
    };

public:
    // Constructor: detecta automaticamente formato simples ou .bus
    explicit Instance(const std::string& filename);

    // ---------------------
    // Dados principais
    // ---------------------

    Point school;
    std::string school_name;

    // No formato .bus, stops NÃO inclui a escola.
    // stops[i] corresponde ao índice original i + 1.
    std::vector<Stop> stops;

    // Endereços dos estudantes.
    // Cada endereço pode representar 1 ou mais estudantes.
    std::vector<Address> addresses;

    // Caminhadas viáveis endereço -> parada.
    // Vêm diretamente das linhas "w" do arquivo .bus.
    std::vector<std::vector<Walk>> feasible_walks_by_address;

    // Matriz de distância dirigida.
    // Índice 0 = escola.
    // Índice i + 1 = stops[i].
    std::vector<std::vector<double>> driving_distance;

    // Matriz de tempo dirigido em segundos.
    // Mantida para futuras restrições de tempo máximo de permanência.
    std::vector<std::vector<double>> driving_time;

    // ---------------------
    // Parâmetros operacionais
    // ---------------------

    int vehicle_capacity{40};

    // Penalidade para inviabilidades fortes, como aluno sem parada viável.
    double penalty_infeasible{1e6};

    // Penalidade para excesso de capacidade, se uma única parada ultrapassar C.
    double penalty_capacity{1e5};

    // Tempo máximo de rota em segundos.
    // Valor <= 0 significa que essa restrição não será aplicada.
    double max_route_time{0.0};

    // Penalidade por violação do tempo máximo de rota.
    double penalty_time{1e3};

    double min_eligibility_distance{};
    double max_walking_distance{};

    bool is_bus_format{false};

    // ---------------------
    // Funções auxiliares de distância
    // ---------------------

    double distance_from_school_to_stop(unsigned stop_local_index) const;
    double distance_from_stop_to_school(unsigned stop_local_index) const;
    double distance_between_stops(unsigned from_local_index, unsigned to_local_index) const;

    // ---------------------
    // Funções auxiliares de tempo
    // ---------------------

    double time_from_school_to_stop(unsigned stop_local_index) const;
    double time_from_stop_to_school(unsigned stop_local_index) const;
    double time_between_stops(unsigned from_local_index, unsigned to_local_index) const;

private:
    void load_simple_format(const std::string& filename);
    void load_bus_format(const std::string& filename);

    static double euclidean_distance(const Point& a, const Point& b);
};

#endif

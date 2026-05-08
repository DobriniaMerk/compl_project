#include "optimizer.hpp"
#include <cmath>
#include <iostream>
#include <ostream>
#include <vector>

struct AS : ACO {
  float evaporation_rate;
  float a, b;
  AS(std::vector<std::vector<float>> distance_matrix, int ant_num, int max_iter, float evaporation_rate,
     float a, float b)
      : ACO(distance_matrix, ant_num, max_iter), evaporation_rate(evaporation_rate),
        a(a), b(b) {}

  void update_choice_info() {
    for (int i = 0; i < dim; ++i) {
      for (int j = i + 1; j < dim; ++j) {
        choice_info[i][j] = choice_info[j][i] =
            std::pow(pheromone[i][j], a) * std::pow(1 / dist[i][j], b);
      }
    }
  }

  void init_pheromone(int ant_num) {
    // nearest neighbour solution
    Ant nearest_neighbour{dim};
    nearest_neighbour.visit(0);
    for (int n = 0; n < dim - 1; ++n) {
      int i = 0;
      while (
          nearest_neighbour.visited[nn_list[nearest_neighbour.current()][i]]) {
        ++i;
      }
      visit(nearest_neighbour, nn_list[nearest_neighbour.current()][i]);
    }
    visit(nearest_neighbour, nearest_neighbour.path[0]);

    for (int i = 0; i < dim; ++i) {
      for (int j = i + 1; j < dim; ++j) {
        pheromone[i][j] = pheromone[j][i] =
            static_cast<float>(ant_num) / nearest_neighbour.path_len;
      }
    }

    best = nearest_neighbour;

    update_choice_info();
  }

  void update_pheromone() {
    for (auto& row : pheromone) {
      for (float& p : row) {
        p *= (1 - evaporation_rate);
      }
    }

    for (Ant& a : ants) {
      float p = 1.0 / static_cast<float>(a.path_len);
      for (int i = 0; i < dim; ++i) {
        pheromone[a.path[i]][a.path[i + 1]] += p;
      }
    }

    update_choice_info();
  }

  void update_best() {
    int best_ind = -1;
    for (int i = 0; i < ants.size(); ++i) {
      if (ants[i].path_len < best.path_len) {
        best_ind = i;
      }
    }

    if (best_ind != -1) {
      best = ants[best_ind];
    }
  }

  void visit(Ant& ant, int city) {
    ant.path_len += dist[ant.current()][city];
    ant.visit(city);
  }

  void step_ant(Ant& ant) {
    int current = ant.current();

    std::vector<float> prob(dim, 0);
    float sum = 0;
    for (int i = 0; i < dim; ++i) {
      if (!ant.visited[i]) {
        prob[i] = choice_info[current][i];
        sum += prob[i];
      }
    }

    float r = rng.real(0, sum);
    sum = prob[0];
    int j = 0;
    while (sum < r) {
      ++j;
      sum += prob[j];
    }

    visit(ant, j);
  }

  std::vector<int> solve() {
    calculate_nn_list();
    init_pheromone(ant_num);

    for (int iter = 0; iter < max_iter; ++iter) {
      ants = std::vector<Ant>(ant_num, Ant(dim));
      // Start in random cities
      for (Ant& a : ants) {
        a.visit(rng.integer(0, dim - 1));
      }

      for (int i = 0; i < dim - 1; ++i) {
        for (Ant& a : ants) {
          step_ant(a);
        }
      }

      // Close the cycle
      for (Ant& a : ants) {
        visit(a, a.path[0]);
      }

      update_best();
      update_pheromone();
    }

    return best.path;
  }
};

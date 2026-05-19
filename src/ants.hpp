#include "optimizer.hpp"
#include <atomic>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include <chrono>

std::atomic<bool> stop_requested = false;

void request_stop(int) {
  stop_requested = true;
}

struct Metrics {
  int iter = 0;
  double elapsed_s = 0.0;
  float global_best_len = 0;
  float iter_best_len = 0;
  float iter_worst_len = 0;
  float iter_mean_len = 0;
};

// Наследование задумавалось как красивый способ все обернуть и переиспользовать.
// Вместо этого вышла каша. Похоже и правда надо было писать на си.
// Выстрелил себе в ногу получается, мы же за это плюсы любим. (-:
struct AS : ACO {
  float evaporation_rate;
  float a, b;
  bool elitist;
  float elitist_weight;

  std::vector<Metrics> history;

  std::vector<float> prob;

  AS(std::vector<std::vector<float>> distance_matrix, int ant_num, int max_iter, float evaporation_rate,
     float a, float b, bool elitist = false, float elitist_weight=-1)
      : ACO(distance_matrix, ant_num, max_iter), evaporation_rate(evaporation_rate),
        a(a), b(b), elitist(elitist), elitist_weight(elitist_weight < 0 ? dim : elitist_weight), prob(dim, 0) {}

  std::vector<int> bestpath() const {
    return best.path;
  }

  float bestlen() const {
    return best.path_len;
  }

  void update_choice_info() {
    for (int i = 0; i < dim; ++i) {
      for (int j = i + 1; j < dim; ++j) {
        choice_info[i][j] = choice_info[j][i] =
            std::pow(pheromone[i][j], a) * std::pow(1 / dist[i][j], b);
      }
    }
  }

  Ant do_nearest_neighbour() {
    // nearest neighbour solution
    Ant nearest_neighbour{dim};
    nearest_neighbour.visit(0);
    for (int n = 0; n < dim - 1; ++n) {
      int i = 0;
      while (nearest_neighbour.visited[nn_list[nearest_neighbour.current()][i]]) {
        ++i;
      }
      visit(nearest_neighbour, nn_list[nearest_neighbour.current()][i]);
    }
    visit(nearest_neighbour, nearest_neighbour.path[0]);

    return nearest_neighbour;
  }

  virtual void init_pheromone(int ant_num) {
    best = do_nearest_neighbour();

    for (int i = 0; i < dim; ++i) {
      for (int j = i + 1; j < dim; ++j) {
        pheromone[i][j] = pheromone[j][i] =
            static_cast<float>(ant_num) / best.path_len;
      }
    }
  }

  void deposit_pheromone(const Ant& ant, float mult = 1.) {
    float p = mult / ant.path_len;
    for (int i = 0; i < dim; ++i) {
      pheromone[ant.path[i]][ant.path[i + 1]] += p;
    }
  }

  virtual void update_pheromone() {
    for (auto& row : pheromone) {
      for (float& p : row) {
        p *= (1 - evaporation_rate);
      }
    }

    for (const Ant& a : ants) {
      deposit_pheromone(a);
    }
  }

  const Ant& best_ant() {
    int best_ind = 0;
    for (size_t i = 1; i < ants.size(); ++i) {
      if (ants[i].path_len < ants[best_ind].path_len) {
        best_ind = i;
      }
    }

    return ants[best_ind];
  }

  void visit(Ant& ant, int city) {
    ant.path_len += dist[ant.current()][city];
    ant.visit(city);
  }

  void step_ant(Ant& ant) {
    int current = ant.current();

    memset(&prob[0], 0, sizeof(prob[0]) * prob.size()); // for optimization

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

  void historize(int iter, decltype(std::chrono::steady_clock::now()) t0) {
    float best_len = ants[0].path_len;
    float worst_len = best_len;
    float sum = 0;
    for (const Ant& a : ants) {
      if (a.path_len < best_len)
        best_len = a.path_len;
      if (a.path_len > worst_len)
        worst_len = a.path_len;
      sum += a.path_len;
    }
    auto t = std::chrono::steady_clock::now();
    history.push_back({iter, std::chrono::duration<double>(t - t0).count(),
                       best.path_len, best_len, worst_len,
                       sum / static_cast<float>(ant_num)});
  }

  void solve(bool progress, int every) override {
    calculate_nn_list();
    init_pheromone(ant_num);

    history = {};
    auto t0 = std::chrono::steady_clock::now();

    for (int iter = 0; iter < max_iter; ++iter) {
      if (stop_requested) {
        return;
      }

      update_choice_info();

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

      const Ant& it_best = best_ant();
      if (it_best.path_len < best.path_len) {
        best = it_best;
      }

      update_pheromone();

      if (elitist) {
        deposit_pheromone(best, elitist_weight);
      }

      historize(iter, t0);

      if (progress && (iter + 1) % every == 0) {
        std::cout << "Iteration " << iter + 1 << ", running for "
                  << history.back().elapsed_s << " best route so far has len "
                  << best.path_len << std::endl;
      }
    }
  }
};

struct MMAS : AS {
  float tau_max;
  float tau_min;
  float p_best;

  MMAS(std::vector<std::vector<float>> distance_matrix, int ant_num,
       int max_iter, float evaporation_rate, float a, float b, float p_best)
  : AS(distance_matrix, ant_num, max_iter, evaporation_rate, a, b, false,-1),
  p_best(p_best) {}

  void init_pheromone(int ant_num) override {
    best = do_nearest_neighbour();

    tau_max = 1.f / (evaporation_rate * best.path_len);

    float p_dec = std::pow(p_best, 1.f / static_cast<float>(dim));
    tau_min = tau_max * (1.f - p_dec) / ((dim / 2.f - 1.f) * p_dec);

    if (tau_min > tau_max) {
      tau_min = tau_max;
    }

    for (int i = 0; i < dim; ++i) {
      for (int j = i + 1; j < dim; ++j) {
        pheromone[i][j] = pheromone[j][i] = tau_max;
      }
    }
  }

  void update_pheromone() override {
    for (auto& row : pheromone) {
      for (float& p : row) {
        p *= (1.f - evaporation_rate);
      }
    }

    if (rng.integer(0, 10) == 0) {
      deposit_pheromone(best);
    } else {
      deposit_pheromone(best_ant());
    }

    for (auto& row : pheromone) {
      for (float& p : row) {
        if (p < tau_min) p = tau_min;
        if (p > tau_max) p = tau_max;
      }
    }
  }
};

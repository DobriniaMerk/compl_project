#include "load_tsplib.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

struct RNG {
  std::random_device rand_dev;
  std::mt19937 rng;
  RNG() : rng(rand_dev()) {}

  int integer(int from, int to) {
    return std::uniform_int_distribution<int>(from, to)(rng);
  }

  float real(float from, float to) {
    return std::uniform_real_distribution<float>(from, to)(rng);
  }
};

struct ACO {
  struct Ant {
    float path_len = 0;
    std::vector<int> path;
    int pathhead = 0;
    std::vector<bool> visited;

    Ant(int dim) : path(dim + 1, -1), visited(dim, false) {}

    void visit(int n) {
      visited[n] = true;
      path[pathhead] = n;
      ++pathhead;
    }

    int current() { return path[pathhead - 1]; }
  };

  int dim;
  std::vector<std::vector<float>> dist;
  std::vector<std::vector<int>> nn_list;
  std::vector<std::vector<float>> pheromone;
  std::vector<std::vector<float>> choice_info;
  std::vector<Ant> ants;

  int ant_num;
  int max_iter;

  Ant best;

  RNG rng;

  ACO(std::vector<std::vector<float>> distance_matrix, int ant_num, int max_iter)
  : dim(distance_matrix.size()), dist(distance_matrix),
        nn_list(dim, std::vector<int>()),
        pheromone(dim, std::vector<float>(dim)),
        choice_info(dim, std::vector<float>(dim)), ants(dim, Ant(dim)),
        ant_num(ant_num), max_iter(max_iter), best(dim) {}

  virtual std::vector<int> solve() = 0;

  void calculate_nn_list(int trim = -1) {
    if (trim == -1)
      trim = dim;

    for (int i = 0; i < dim; ++i) {
      nn_list[i] = std::vector<int>(dim);
      std::iota(nn_list[i].begin(), nn_list[i].end(), 0);
      std::sort(nn_list[i].begin(), nn_list[i].end(),
                [this, i](int a, int b) { return dist[i][a] < dist[i][b]; });

      nn_list[i].resize(trim);
    }
  }
};

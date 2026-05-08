#include "as.hpp"
#include "load_tsplib.hpp"
#include "load_data.hpp"
#include <iostream>
#include <ostream>

std::vector<std::vector<float>> distance_matrix(std::vector<std::pair<float, float>>& points) {
  int dimension = points.size();

  std::vector<std::vector<float>> distance_matrix(dimension, std::vector<float>(dimension, 0));
  for (int i = 0; i < dimension; ++i) {
    for (int j = i + 1; j < dimension; ++j) {
      float dx = std::abs(points[i].first - points[j].first);
      float dy = std::abs(points[i].second - points[j].second);
      distance_matrix[i][j] = distance_matrix[j][i] =
          std::sqrt(dx * dx + dy * dy);
    }
  }

  return distance_matrix;
}

int main() {
  std::string base_path = "tsp-visualizer-master/tsp-visualizer-master/";
  auto points = load_problem(base_path + "tsp_200_2");
  AS as(distance_matrix(points), 51, 500, 0.5, 1., 2.);
  std::vector<int> ans = as.solve();

  for (int i : ans) {
    std::cout << i << ' ';
  }
  std::cout << std::endl;
  std::cout << as.best.path_len << std::endl;
}

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

std::vector<std::pair<float, float>> load_problem(std::string filename) {
  std::ifstream file(filename);
  int dim;
  file >> dim;

  std::vector<std::pair<float, float>> ret(dim);
  for (int i = 0; i < dim; ++i) {
    file >> ret[i].first >> ret[i].second;
  }

  return ret;
}

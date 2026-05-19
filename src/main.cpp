#include "ants.hpp"
#include "load_tsplib.hpp"
#include "load_data.hpp"
#include <csignal>
#include <iostream>
#include <ostream>
#include <vector>

void metrics_to_csv(const std::vector<Metrics>& history, const std::string& filename) {
  std::ofstream out(filename);
  out << "iter,elapsed_s,global_best_len,iter_best_len,iter_mean_len,iter_worst_len\n";
  for (const auto& m : history) {
    out << m.iter << ','
        << m.elapsed_s << ','
        << m.global_best_len << ','
        << m.iter_best_len << ','
        << m.iter_mean_len << ','
        << m.iter_worst_len << '\n';
  }
}

int main(int argc, char** argv) {
  if (argc < 9) {
    std::cerr << "Usage:\n"
              << argv[0]
              << " <problem_file> AS <ant_num> <max_iter>"
                 " <evaporation_rate> <a> <b> <metrics_csv>"
                 " <report_each_n_iters>"
              << '\n'
              << argv[0]
              << " <problem_file> EAS <ant_num> <max_iter>"
                 " <evaporation_rate> <a> <b> <elitist_mult>"
                 " <metrics_csv> <report_each_n_iters>"
              << '\n'
              << argv[0]
              << " <problem_file> MMAS <ant_num> <max_iter>"
                 " <evaporation_rate> <a> <b> <p_best>"
                 " <metrics_csv> <report_each_n_iters>"
              << '\n';

    return 1;
  }

  signal(SIGINT, request_stop);
  signal(SIGTERM, request_stop);

  std::string problem_file = argv[1];
  int ant_num = std::stoi(argv[3]);
  int max_iter = std::stoi(argv[4]);
  float evaporation_rate = std::stof(argv[5]);
  float a = std::stof(argv[6]);
  float b = std::stof(argv[7]);

  std::string metrics_file;
  int report_every;

  float param;

  switch (argv[2][0]) {
  case 'a':
  case 'A':
    metrics_file = argv[8];
    report_every = std::stoi(argv[9]);
    break;
  case 'e':
  case 'E':
  case 'm':
  case 'M':
    param = std::stof(argv[8]);
    metrics_file = argv[9];
    report_every = std::stoi(argv[10]);
    break;
  }


  auto points = load_problem(problem_file);
  AS* solver;

  switch (argv[2][0]) {
  case 'a':
  case 'A':
    solver = new AS(distance_matrix(points), ant_num, max_iter, evaporation_rate, a, b);
    break;
  case 'e':
  case 'E':
    solver = new AS(distance_matrix(points), ant_num, max_iter, evaporation_rate, a, b, true, param);
    break;
  case 'm':
  case 'M':
    solver = new MMAS(distance_matrix(points), ant_num, max_iter, evaporation_rate, a, b, param);
    break;
  }

  solver->solve(true, report_every);
  std::vector<int> ans = solver->bestpath();

  metrics_to_csv(solver->history, metrics_file);

  for (int i : ans) {
    std::cout << i << ' ';
  }
  std::cout << std::endl;
  std::cout << solver->bestlen() << std::endl;
}

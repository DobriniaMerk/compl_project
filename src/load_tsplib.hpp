#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

struct Problem {
  std::string name;
  std::string type;
  std::string comment;
  int dimension;
  std::string edge_weight_type;
  std::string display_data_type;

  std::vector<std::pair<float, float>> points;

  Problem(std::string filename) {
    std::ifstream file(filename);

    std::string field;
    while (file >> field, field != "NODE_COORD_SECTION") {
      if (field.ends_with(':')) {
        field.pop_back();
      } else {
        // got space separated colon
        std::string devnull;
        file >> devnull;
      }

      if (field == "NAME") {
        std::getline(file, name);
      } else
      if (field == "COMMENT") {
        std::getline(file, comment);
      } else
      if (field == "TYPE") {
        file >> type;
      } else
      if (field == "DIMENSION") {
        file >> dimension;
      } else
      if (field == "EDGE_WEIGHT_TYPE") {
        file >> edge_weight_type;
      } else
      if (field == "DISPLAY_DATA_TYPE") {
        file >> display_data_type;
      } else {
        std::string data;
        std::getline(file, data);
        std::cout << "Uncnown header " << field << ": " << data << std::endl;
      }
    }

    points = std::vector<std::pair<float, float>>(dimension);
    for (int i = 0; i < dimension; ++i) {
      int ind;
      file >> ind >> points[i].first >> points[i].second;
    }
  }
};

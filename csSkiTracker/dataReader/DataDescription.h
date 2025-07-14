#pragma once

#include <Eigen/Core>
#include <vector>

namespace csSkiTracker {
  namespace dataReader {

  struct MatchingPoints {
    Eigen::Matrix2Xd imgPoints_view1;
    Eigen::Matrix2Xd imgPoints_view2;
    Eigen::Matrix3Xd worldPoints;

    MatchingPoints();
    MatchingPoints(int n);
  };

  struct CalibPoints {
    Eigen::Matrix<double, 3, 4> projMat;
    Eigen::Matrix2Xd imgPoints;
    Eigen::ArrayXi worldPointsId;

    CalibPoints(int n);
    CalibPoints();
  };

  struct FrameData {
    MatchingPoints matchingPoints;
    CalibPoints view2;
  };

  struct ImageSize {
    int width;
    int height;
  };

  struct ProblemData {

    ImageSize view1_imgSize;
    ImageSize view2_imgSize;
    Eigen::Matrix3Xd worldPoints;

    CalibPoints calib_view1;

    std::vector<FrameData> frames;

    ProblemData();

  private:
  };
  }
}
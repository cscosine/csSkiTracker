#pragma once
#include <Eigen/Core>

#include "ProjectionMatrixEstimate.h"

class Triangulate {

public:
  static Eigen::Vector3d triangulateLinear(const ProjectionMatrix& P1, const Eigen::Vector2d& i1, const ProjectionMatrix& P2,
                                           const Eigen::Vector2d& i2);
  static Eigen::Vector3d triangulateNonLinear(const ProjectionMatrix& P1, const Eigen::Vector2d& i1, const ProjectionMatrix& P2,
                                              const Eigen::Vector2d& i2, const Eigen::Vector3d& guess);
};

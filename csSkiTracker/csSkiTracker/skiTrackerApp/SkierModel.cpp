#include "SkierModel.h"

#include "csLie/SO3.hpp"

SkierModel::SkierModel() {}

SkierModel::~SkierModel() {}

void SkierModel::setFromPoints(const Eigen::Matrix3Xd& points) {
  assert(points.cols() == Labels::END);
  _bodyPoints = points;

  // create a ref frame
  _pose.translation() = hipsMiddlePoints();
  Eigen::Vector3d z = (shouldersMiddlePoints() - hipsMiddlePoints()).normalized();
  Eigen::Vector3d y = (bodyPoint(Labels::LeftHip) - bodyPoint(Labels::RightHip)).normalized();
  Eigen::Vector3d x = -z.cross(y);
  z = x.cross(y);

  _pose.linear().col(0) = x;
  _pose.linear().col(1) = y;
  _pose.linear().col(2) = z;
}

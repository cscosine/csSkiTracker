#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "CameraPosePointNodeBase.h"

class PointNode : public CameraPosePointNodeBase {

  Eigen::Vector3d _point;

public:
  PointNode();

  PointNode(const Eigen::Vector3d& point);
  virtual ~PointNode();

  const Eigen::Vector3d& point() const {
    return _point;
  }
  void setPoint(const Eigen::Vector3d& v) {
    _point = v;
  }
};

class PointXYNode : public CameraPosePointNodeBase {
  Eigen::Isometry3d _T_W_planeOrigin;
  Eigen::Vector2d _pointXY;

public:
  PointXYNode();

  PointXYNode(const Eigen::Isometry3d& T_W_planeOrigin, const Eigen::Vector2d& pointXY);

  const Eigen::Vector2d& point() const {
    return _pointXY;
  }
  Eigen::Vector3d point3d() const {
    return Eigen::Vector3d(_pointXY.x(), _pointXY.y(), 0);
  }
  void setPoint(const Eigen::Vector2d& v) {
    _pointXY = v;
  }

  const Eigen::Isometry3d& T_W_planeOrigin() const {
    return _T_W_planeOrigin;
  }
};

#pragma once
#include <Eigen/Core>

#include "csLie/SE3.hpp"

class SkierModel {

  Eigen::Matrix3Xd _bodyPoints;
  csLie::SE3d _pose;

public:
  SkierModel();
  virtual ~SkierModel();

  enum Labels {
    RightFootHead,
    RightFootRearBottom,
    RightAnkle,
    RightKnee,
    RightHip,
    RightShoulder,
    RightElbow,
    RightHand,
    LeftFootHead,
    LeftFootRearBottom,
    LeftAnkle,
    LeftKnee,
    LeftHip,
    LeftShoulder,
    LeftElbow,
    LeftHand,
    Head,
    RightPoleTip,
    LeftPoleTip,
    RightSkiHead,
    RightSkiTail,
    LeftSkiHead,
    LeftSkiTail,
    END
  };

  bool valid() const {
    return _bodyPoints.cols() == Labels::END;
  }

  void setFromPoints(const Eigen::Matrix3Xd& points);

  Eigen::Vector3d hipsMiddlePoints() const {
    return (_bodyPoints.col(LeftHip) + _bodyPoints.col(RightHip)) * 0.5;
  }

  Eigen::Vector3d shouldersMiddlePoints() const {
    return (_bodyPoints.col(LeftShoulder) + _bodyPoints.col(RightShoulder)) * 0.5;
  }

  Eigen::Vector3d bodyPoint(Labels l) const {
    return _bodyPoints.col(l);
  }

  const Eigen::Matrix3Xd& bodyPoints() const {
    return _bodyPoints;
  }

  const csLie::SE3d& pose() const {
    return _pose;
  }
};

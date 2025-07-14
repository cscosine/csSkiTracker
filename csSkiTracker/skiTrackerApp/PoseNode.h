#pragma once
#include "csLie/SE3_impl.hpp"

#include "CameraPoseNodeBase.h"

class PoseNode : public CameraPoseNodeBase {
  csLie::SE3d _pose;
public:

  PoseNode();
  PoseNode(const csLie::SE3d& pose);
  virtual ~PoseNode();

  const csLie::SE3d& pose() const {
    return _pose;
  }
  void setPose(const csLie::SE3d& pose);


  template<typename Derived>
  void oplus(const Eigen::MatrixBase<Derived>& oplus) {
    _pose = csLie::se3Exp(csLie::se3Log(csLie::se3Exp(csLie::SE3Algd(oplus)) * _pose));
  }

};

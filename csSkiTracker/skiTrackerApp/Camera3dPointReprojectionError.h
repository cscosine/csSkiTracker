#pragma once
#include "CameraCalibSection.h"
#include "csNelson/EdgeSectionBase.h"
#include "csNelson/EdgeBinary.h"

class Camera3dPointReprojectionError : public CameraCalibSection::EdgeBinary< Camera3dPointReprojectionError> {
  //inputs
  Eigen::Matrix3Xd _worldPoints;
  Eigen::Matrix2Xd _imgMeasPoints;

  // hessian storage
  Eigen::MatrixXd H_camParams;
  Eigen::VectorXd b_camParams;

  Eigen::Matrix<double, Eigen::Dynamic, 6> H_camParams_pose;

  Eigen::Matrix<double, 6, 6> H_pose;
  Eigen::Matrix<double, 6, 1> b_pose;


public:
  Camera3dPointReprojectionError(
    const Eigen::Matrix3Xd& worldPoints,
    const Eigen::Matrix2Xd& imgMeasPoints
  );
  virtual ~Camera3dPointReprojectionError();

  void update(bool hessians) override;

  template<class Derived1, class Derived2>
  void updateH11Block(Eigen::MatrixBase<Derived1>& H, Eigen::MatrixBase<Derived2>& b) {
    H.noalias() += H_camParams;
    b.noalias() += b_camParams;
  }
  template<class Derived>
  void updateH12Block(Eigen::MatrixBase<Derived>& H, bool transpose) {
    assert(transpose == false);
    H.noalias() += H_camParams_pose;
  }
  template<class Derived1, class Derived2>
  void updateH22Block(Eigen::MatrixBase<Derived1>& H, Eigen::MatrixBase<Derived2>& b) {
    H.noalias() += H_pose;
    b.noalias() += b_pose;
  }

};
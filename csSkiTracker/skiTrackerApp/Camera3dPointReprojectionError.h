#pragma once
#include "CameraCalibSection.h"
#include <csNelson/EdgeNary_impl.hpp>
#include <csNelson/EdgeSectionBase_impl.hpp>

class Camera3dPointReprojectionError : public CameraCalibSection::EdgeNary<Camera3dPointReprojectionError, 3> {
  Eigen::Vector2d _imgMeasPoint;

  // hessian storage
  Eigen::MatrixXd H_camParams;
  Eigen::VectorXd b_camParams;

  Eigen::Matrix<double, 6, 6> H_pose;
  Eigen::Matrix<double, 6, 1> b_pose;

  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> H_point;
  Eigen::Matrix<double, Eigen::Dynamic, 1> b_point;

  Eigen::Matrix<double, Eigen::Dynamic, 6> H_camParams_pose;
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> H_camParams_point;
  Eigen::Matrix<double, 6, Eigen::Dynamic> H_pose_point;

public:
  Camera3dPointReprojectionError(const Eigen::Vector2d& imgMeasPoint);
  virtual ~Camera3dPointReprojectionError();

  void update(bool hessians) override;

  template <class Derived1, class Derived2>
  void updateHBlock(int i, Eigen::MatrixBase<Derived1>& H, Eigen::MatrixBase<Derived2>& b) {
    if (i == 0) {
      H.noalias() += H_camParams;
      b.noalias() += b_camParams;
    } else if (i == 1) {
      H.noalias() += H_pose;
      b.noalias() += b_pose;
    } else if (i == 2) {
      H.noalias() += H_point;
      b.noalias() += b_point;
    } else
      assert(false);
  }
  template <class Derived>
  void updateHBlock(int i, int j, Eigen::MatrixBase<Derived>& H, bool transpose) {
    if (i == 0 && j == 1) {
      if (!transpose) {
        H.noalias() += H_camParams_pose;
      } else {
        H.noalias() += H_camParams_pose.transpose();
      }
    } else if (i == 0 && j == 2) {
      if (!transpose) {
        H.noalias() += H_camParams_point;
      } else {
        H.noalias() += H_camParams_point.transpose();
      }
    } else if (i == 1 && j == 2) {
      if (!transpose) {
        H.noalias() += H_pose_point;
      } else {
        H.noalias() += H_pose_point.transpose();
      }
    } else
      assert(false);
  }
};

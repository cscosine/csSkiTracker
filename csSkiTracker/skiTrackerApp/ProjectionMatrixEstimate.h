#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "csCamera/Camera.h"

using ProjectionMatrix = Eigen::Matrix<double, 3, 4>;

class ProjectionMatrixEstimate {

public:
  static ProjectionMatrix estimateProjMatrix(const Eigen::Matrix2Xd& imgPoints, const Eigen::Matrix3Xd& worldPoints);

  static void decomposeProjMatrix(const ProjectionMatrix& P, std::array<Eigen::Isometry3d, 4>& Rts, csCamera::Camerad& camera);

  static ProjectionMatrix createP(const Eigen::Matrix3d& K, const Eigen::Isometry3d& Rt);
  static ProjectionMatrix createP(const csCamera::Camerad& K, const Eigen::Isometry3d& Rt);

  static Eigen::Matrix2Xd proj3DPoints2img(const ProjectionMatrix& P, const Eigen::Matrix3Xd& worldPoints);
  static Eigen::Vector2d proj3DPoints2img(const ProjectionMatrix& P, const Eigen::Vector3d& worldPoint,
                                          Eigen::Matrix<double, 2, 3>& d_e_d_worldPoint);

  static Eigen::Isometry3d selectBestRt(const std::array<Eigen::Isometry3d, 4>& Rts, const csCamera::Camerad& camera,
                                        const Eigen::Matrix2Xd& imgPoints, const Eigen::Matrix3Xd& worldPoints);

  static void estimateCameraAndRt(const Eigen::Matrix2Xd& imgPoints, const Eigen::Matrix3Xd& worldPoints, csCamera::Camerad& camera,
                                  Eigen::Isometry3d& Rt);
};

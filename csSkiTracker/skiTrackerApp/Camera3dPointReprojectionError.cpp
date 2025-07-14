#include "Camera3dPointReprojectionError.h"

#include "csNelson/EdgeBinary.hpp"
#include "csNelson/EdgeSectionBase.hpp"

#include "csCamera/Camera.hpp"
#include "csCamera/CameraDistortionModel.hpp"
#include "csCamera/Utils.hpp"

Camera3dPointReprojectionError::Camera3dPointReprojectionError(const Eigen::Matrix3Xd& worldPoints,
                                                               const Eigen::Matrix2Xd& imgMeasPoints)
    : _worldPoints(worldPoints)
    , _imgMeasPoints(imgMeasPoints) {}
Camera3dPointReprojectionError::~Camera3dPointReprojectionError() {}

void Camera3dPointReprojectionError::update(bool hessians) {
#ifndef NDEBUG
  const CameraNode& camera = dynamic_cast<const CameraNode&>(this->parameter_1());
  const PoseNode& pose = dynamic_cast<const PoseNode&>(this->parameter_2());
#else
  const CameraNode& camera = static_cast<const CameraNode&>(this->parameter_1());
  const PoseNode& pose = static_cast<const PoseNode&>(this->parameter_2());
#endif

  if (!hessians) {
    Eigen::Matrix3Xd cameraPoints = pose.pose() * _worldPoints;
    Eigen::Matrix2Xd pz1 = csCamera::Utils::points3D_to_z1(cameraPoints);
    Eigen::Matrix2Xd pz1d = camera.distModel().distort(pz1);
    Eigen::Matrix2Xd repr = camera.camera().pointsZ1_to_image(pz1d);
    Eigen::Matrix2Xd err = repr - _imgMeasPoints;
    this->setChi2(err.squaredNorm());
  } else {
    // resize hessians (camera params is variable)
    this->H_camParams.setZero(camera.numParams(), camera.numParams());
    this->H_camParams_pose.setZero(camera.numParams(), 6);
    this->H_pose.setZero();

    this->b_camParams.setZero(camera.numParams());
    this->b_pose.setZero();

    Eigen::Matrix3Xd cameraPoints = pose.pose() * _worldPoints;
    double chi2 = 0;
    for (int i = 0; i < _worldPoints.cols(); i++) {
      // local points
      Eigen::Matrix<double, 3, 6> d_pcam_d_g0 = csLie::se3JacobianLeftZeroPerturbationPointTransformation(cameraPoints.col(i));
      Eigen::Vector3d pcam = cameraPoints.col(i);

      // z=1
      Eigen::Matrix<double, 2, 3> d_pz1_d_pcam;
      Eigen::Vector2d pz1 = csCamera::Utils::points3D_to_z1_jacob(pcam, d_pz1_d_pcam);

      // z=1 distorted
      Eigen::Matrix2d d_pz1d_d_pz1;
      Eigen::Matrix<double, 2, 3> d_pz1d_d_k123, d_pz1d_d_k456;
      Eigen::Matrix2d d_pz1d_d_p12;
      Eigen::Matrix<double, 2, 4> d_pz1d_d_s1234;
      Eigen::Vector2d pz1d =
          camera.distModel().distort_jacobians(pz1, d_pz1d_d_pz1, d_pz1d_d_k123, d_pz1d_d_k456, d_pz1d_d_p12, d_pz1d_d_s1234);

      // reproj
      Eigen::Matrix<double, 2, 4> d_repr_d_camparams;
      Eigen::Matrix2d d_repr_d_pz1d;
      Eigen::Vector2d repr = camera.camera().pointsZ1_to_image_jacobian(pz1d, d_repr_d_camparams, d_repr_d_pz1d);

      // error
      Eigen::Vector2d err = repr - _imgMeasPoints.col(i);

      // final jacobians
      Eigen::Matrix<double, 2, 6> d_repr_d_g0 = d_repr_d_pz1d * d_pz1d_d_pz1 * d_pz1_d_pcam * d_pcam_d_g0;

      Eigen::Matrix<double, 2, 3> d_repr_d_k123 = d_repr_d_pz1d * d_pz1d_d_k123;
      Eigen::Matrix<double, 2, 3> d_repr_d_k456 = d_repr_d_pz1d * d_pz1d_d_k456;
      Eigen::Matrix<double, 2, 2> d_repr_d_p12 = d_repr_d_pz1d * d_pz1d_d_p12;
      Eigen::Matrix<double, 2, 4> d_repr_d_s1234 = d_repr_d_pz1d * d_pz1d_d_s1234;

      Eigen::Matrix<double, 2, Eigen::Dynamic> d_repr_d_camParams;
      d_repr_d_camParams.setZero(2, camera.numParams());

      int np = 0;
      if (camera.focalEstimation() == CameraNode::FocalEstimation::Both) {
        d_repr_d_camParams.block<2, 2>(0, np) = d_repr_d_camparams.leftCols<2>();
        np += 2;
      } else if (camera.focalEstimation() == CameraNode::FocalEstimation::FixRatio) {
        d_repr_d_camParams(0, np) = d_repr_d_camparams(0, 0);
        d_repr_d_camParams(1, np) = d_repr_d_camparams(1, 1);
        np++;
      } else if (camera.focalEstimation() == CameraNode::FocalEstimation::Fixed) {
        // nothing
      } else {
        assert(false && "how the hell do you ended up here?");
      }

      if (!camera.fixCenter()[0])
        d_repr_d_camParams.col(np++) = d_repr_d_camparams.col(2);
      if (!camera.fixCenter()[1])
        d_repr_d_camParams.col(np++) = d_repr_d_camparams.col(3);

      if (!camera.fixKs()[0])
        d_repr_d_camParams.col(np++) = d_repr_d_k123.col(0);
      if (!camera.fixKs()[1])
        d_repr_d_camParams.col(np++) = d_repr_d_k123.col(1);
      if (!camera.fixKs()[2])
        d_repr_d_camParams.col(np++) = d_repr_d_k123.col(2);
      if (!camera.fixKs()[3])
        d_repr_d_camParams.col(np++) = d_repr_d_k456.col(0);
      if (!camera.fixKs()[4])
        d_repr_d_camParams.col(np++) = d_repr_d_k456.col(1);
      if (!camera.fixKs()[5])
        d_repr_d_camParams.col(np++) = d_repr_d_k456.col(2);

      if (!camera.fixPs()[0])
        d_repr_d_camParams.col(np++) = d_repr_d_p12.col(0);
      if (!camera.fixPs()[1])
        d_repr_d_camParams.col(np++) = d_repr_d_p12.col(1);

      if (!camera.fixSs()[0])
        d_repr_d_camParams.col(np++) = d_repr_d_s1234.col(0);
      if (!camera.fixSs()[1])
        d_repr_d_camParams.col(np++) = d_repr_d_s1234.col(1);
      if (!camera.fixSs()[2])
        d_repr_d_camParams.col(np++) = d_repr_d_s1234.col(2);
      if (!camera.fixSs()[3])
        d_repr_d_camParams.col(np++) = d_repr_d_s1234.col(3);

      assert(np == camera.numParams());

      chi2 += err.squaredNorm();

      // hessian
      this->H_pose += d_repr_d_g0.transpose() * d_repr_d_g0;
      this->b_pose += d_repr_d_g0.transpose() * err;

      this->H_camParams += d_repr_d_camParams.transpose() * d_repr_d_camParams;
      this->b_camParams += d_repr_d_camParams.transpose() * err;

      this->H_camParams_pose += d_repr_d_camParams.transpose() * d_repr_d_g0;
    }

    this->setChi2(chi2);
  }
}

#include "ProjectionMatrixEstimate.h"

#include <csCamera/Camera_impl.hpp>

#include <Eigen/Dense>
#include <array>
#include <iostream>

// #define DEBUGME_EST
#define DEBUGME_EST_COUT \
  if (false)             \
  std::cout

// #define DEBUGME_DECOMP
#define DEBUGME_DECOMP_COUT \
  if (false)                \
  std::cout

ProjectionMatrix ProjectionMatrixEstimate::estimateProjMatrix(const Eigen::Matrix2Xd& imgPoints, const Eigen::Matrix3Xd& worldPoints) {
  assert(imgPoints.cols() == worldPoints.cols());
  using MatSolve = Eigen::Matrix<double, Eigen::Dynamic, 12>;
  using VecSolve = Eigen::Matrix<double, 12, 1>;

  int n = imgPoints.cols();
  MatSolve A(n * 2, 12);
  A.setZero();
  for (int i = 0; i < n; i++) {
    // first equation
    A(2 * i, 0) = worldPoints.col(i).x();
    A(2 * i, 1) = worldPoints.col(i).y();
    A(2 * i, 2) = worldPoints.col(i).z();
    A(2 * i, 3) = 1;
    A(2 * i, 8) = -worldPoints.col(i).x() * imgPoints.col(i).x();
    A(2 * i, 9) = -worldPoints.col(i).y() * imgPoints.col(i).x();
    A(2 * i, 10) = -worldPoints.col(i).z() * imgPoints.col(i).x();
    A(2 * i, 11) = -imgPoints.col(i).x();
    // second equation
    A(2 * i + 1, 4) = worldPoints.col(i).x();
    A(2 * i + 1, 5) = worldPoints.col(i).y();
    A(2 * i + 1, 6) = worldPoints.col(i).z();
    A(2 * i + 1, 7) = 1;
    A(2 * i + 1, 8) = -worldPoints.col(i).x() * imgPoints.col(i).y();
    A(2 * i + 1, 9) = -worldPoints.col(i).y() * imgPoints.col(i).y();
    A(2 * i + 1, 10) = -worldPoints.col(i).z() * imgPoints.col(i).y();
    A(2 * i + 1, 11) = -imgPoints.col(i).y();
  }

  Eigen::JacobiSVD<MatSolve> svd(A, Eigen::ComputeFullV);

  VecSolve vSol = svd.matrixV().col(11);

  ProjectionMatrix ret;
  ret.row(0) = vSol.segment<4>(0);
  ret.row(1) = vSol.segment<4>(4);
  ret.row(2) = vSol.segment<4>(8);

#ifdef DEBUGME_EST

  Eigen::Matrix2Xd uv = proj3DPoints2img(ret, worldPoints);

  Eigen::Matrix2Xd err = uv - imgPoints;
  DEBUGME_EST_COUT << "uv = " << std::endl << uv << std::endl;
  DEBUGME_EST_COUT << "imgPoints = " << std::endl << imgPoints << std::endl;
  DEBUGME_EST_COUT << "err = " << std::endl << err << std::endl;

  double mc = err.cwiseAbs().maxCoeff();
  DEBUGME_EST_COUT << "max err " << mc << std::endl;
#endif

  return ret;
}

void ProjectionMatrixEstimate::decomposeProjMatrix(const ProjectionMatrix& P, std::array<Eigen::Isometry3d, 4>& Rts,
                                                   csCamera::Camerad& camera) {
  const Eigen::Matrix3d A = P.block<3, 3>(0, 0);
  double rhoPlus = 1.0 / A.row(2).norm();
  double rhoMinus = -rhoPlus;

  double rho2 = rhoPlus * rhoPlus;

  camera.setCxCy(rho2 * A.row(0).dot(A.row(2)), rho2 * A.row(1).dot(A.row(2)));

  Eigen::Vector3d A0_cross_A2 = A.row(0).cross(A.row(2));
  Eigen::Vector3d A1_cross_A2 = A.row(1).cross(A.row(2));

  double A0_cross_A2_norm = A0_cross_A2.norm();
  double A1_cross_A2_norm = A1_cross_A2.norm();

  double cos_th = (A0_cross_A2).dot(A1_cross_A2) / (A0_cross_A2_norm * A1_cross_A2_norm);

  // skew not supported
  // camera.th = std::acos(cos_th);
  // double sin_th = std::sin(camera.th);
  // camera.setFxFy(rho2 * A0_cross_A2_norm * sin_th, rho2 * A1_cross_A2_norm * sin_th);
  camera.setFxFy(rho2 * A0_cross_A2_norm, rho2 * A1_cross_A2_norm);

  Eigen::Vector3d r1 = A1_cross_A2 / A1_cross_A2_norm;
  Eigen::Vector3d r3_plus = A.row(2) * rhoPlus;
  Eigen::Vector3d r3_minus = A.row(2) * rhoMinus;

  Eigen::Vector3d r2_plus = r3_plus.cross(r1);
  Eigen::Vector3d r2_minus = r3_minus.cross(r1);

  Eigen::Matrix3d K = camera.K();

  Eigen::Vector3d t_plus = rhoPlus * K.inverse() * P.col(3);
  Eigen::Vector3d t_minus = rhoMinus * K.inverse() * P.col(3);

  Eigen::Matrix3d Rp;
  Rp.row(0) = r1;
  Rp.row(1) = r2_plus;
  Rp.row(2) = r3_plus;

  assert(Rp.determinant() > 0);

  Eigen::Matrix3d Rm;
  Rm.row(0) = r1;
  Rm.row(1) = r2_minus;
  Rm.row(2) = r3_minus;

  assert(Rm.determinant() > 0);

  // prepare four hypothesis
  for (auto Rt : Rts)
    Rt.setIdentity();

  Rts[0].linear() = Rp;
  Rts[0].translation() = t_plus;

  Rts[1].linear() = Rp;
  Rts[1].translation() = t_minus;

  Rts[2].linear() = Rm;
  Rts[2].translation() = t_plus;

  Rts[3].linear() = Rm;
  Rts[3].translation() = t_minus;

#ifdef DEBUGME_DECOMP
  for (auto Rt : Rt_hypotesis) {
    ProjectionMatrix Ptmp = createP(K, Rt);
    Ptmp /= rhoPlus;
    DEBUGME_DECOMP_COUT << Ptmp << std::endl << std::endl;
  }
#endif
}

Eigen::Vector2d ProjectionMatrixEstimate::proj3DPoints2img(const ProjectionMatrix& P, const Eigen::Vector3d& worldPoint,
                                                           Eigen::Matrix<double, 2, 3>& d_e_d_worldPoint) {

  Eigen::Vector3d uvw = P.block<3, 3>(0, 0) * worldPoint + P.col(3);

  Eigen::Vector2d uv(2, uvw.cols());
  uv.row(0) = uvw.row(0).array() / uvw.row(2).array();
  uv.row(1) = uvw.row(1).array() / uvw.row(2).array();

  const double w = uvw(2);
  const double w2 = w * w;
  d_e_d_worldPoint.setZero();
  d_e_d_worldPoint(0, 0) = 1 / w;
  d_e_d_worldPoint(1, 1) = 1 / w;
  d_e_d_worldPoint(0, 2) = -uvw(0) / (w2);
  d_e_d_worldPoint(1, 2) = -uvw(1) / (w2);

  d_e_d_worldPoint = d_e_d_worldPoint * P.block<3, 3>(0, 0);

  return uv;
}

Eigen::Matrix2Xd ProjectionMatrixEstimate::proj3DPoints2img(const ProjectionMatrix& P, const Eigen::Matrix3Xd& worldPoints) {
  Eigen::Matrix4Xd w(4, worldPoints.cols());
  w.row(3).setConstant(1);
  w.topRows<3>() = worldPoints;

  Eigen::Matrix3Xd uvw = P * w;

  Eigen::Matrix2Xd uv(2, uvw.cols());
  uv.row(0) = uvw.row(0).array() / uvw.row(2).array();
  uv.row(1) = uvw.row(1).array() / uvw.row(2).array();

  return uv;
}

ProjectionMatrix ProjectionMatrixEstimate::createP(const Eigen::Matrix3d& K, const Eigen::Isometry3d& Rt) {
  ProjectionMatrix ret;
  ret.block<3, 3>(0, 0) = K * Rt.linear();
  ret.col(3) = K * Rt.translation();
  return ret;
}

ProjectionMatrix ProjectionMatrixEstimate::createP(const csCamera::Camerad& camera, const Eigen::Isometry3d& Rt) {
  return createP(camera.K(), Rt);
}

Eigen::Isometry3d ProjectionMatrixEstimate::selectBestRt(const std::array<Eigen::Isometry3d, 4>& Rts, const csCamera::Camerad& camera,
                                                         const Eigen::Matrix2Xd& imgPoints, const Eigen::Matrix3Xd& worldPoints) {
  double minErr = Eigen::NumTraits<double>::highest();
  Eigen::Isometry3d ret;

  Eigen::Matrix3d K = camera.K();

  for (const auto& Rt : Rts) {
    ProjectionMatrix P = createP(K, Rt);

    Eigen::Matrix<double, 4, Eigen::Dynamic> wH(4, worldPoints.cols());
    wH.topRows<3>() = worldPoints;
    wH.row(3).setConstant(1);
    Eigen::Matrix3Xd p_repH = P * wH;

    Eigen::Matrix2Xd p_rep(2, p_repH.cols());
    p_rep.row(0) = p_repH.row(0).array() / p_repH.row(2).array();
    p_rep.row(1) = p_repH.row(1).array() / p_repH.row(2).array();

    Eigen::Matrix2Xd err = p_rep - imgPoints;
    double totErr = err.cwiseAbs2().sum();
    if (totErr < minErr) {
      minErr = totErr;
      ret = Rt;
    }
  }

  return ret;
}

void ProjectionMatrixEstimate::estimateCameraAndRt(const Eigen::Matrix2Xd& imgPoints, const Eigen::Matrix3Xd& worldPoints,
                                                   csCamera::Camerad& camera, Eigen::Isometry3d& Rt) {
  auto P = estimateProjMatrix(imgPoints, worldPoints);

  std::array<Eigen::Isometry3d, 4> Rts;
  ProjectionMatrixEstimate::decomposeProjMatrix(P, Rts, camera);

  Rt = ProjectionMatrixEstimate::selectBestRt(Rts, camera, imgPoints, worldPoints);
}

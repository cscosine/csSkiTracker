#include "ProblemDescription.h"

#include <Eigen/Geometry>
#include <array>

#include <iostream>

#include "Triangulate.h"

#include "Camera3dPointReprojectionError.h"
#include "CameraCalibSection.h"
#include "CameraNode.hpp"
#include "PoseNode.h"
#include "csCamera/Camera.hpp"
#include "csCamera/CameraDistortionModel.hpp"
#include "csNelson/GaussNewton.hpp"
#include "csNelson/LevenbergMarquardt.hpp"
#include "csNelson/SingleSection.hpp"

#define DEBUGME_CLOSEST \
  if (false)            \
  std::cout
#define DEBUGME_VIEWRAYS \
  if (false)             \
  std::cout
#define DEBUGME \
  if (true)     \
  std::cout <<

Eigen::Vector2d CameraDistModel::undistImagePoint(const Eigen::Vector2d& imgPoint) const {
  Eigen::Vector2d pz1 = this->cameraModel.pointsImage_to_Z1(imgPoint);
  Eigen::Vector2d pz1_und = this->distModel.undistortSinglePoint(pz1);
  Eigen::Vector2d pimg = this->cameraModel.pointsZ1_to_image(pz1_und);
  return pimg;
}

Eigen::Matrix2Xd ImgCalibPoints::reprojErr() const {
  return reprojPoints - imgPoints;
}

std::vector<int> ImgCalibPoints::indexesVec() const {
  std::vector<int> ret(this->indexes.size());
  for (int i = 0; i < indexes.size(); i++) {
    ret[i] = indexes(i);
  }
  return ret;
}

void ImgCalibPoints::reprojErrMeanMax(double& mean, double& max) const {
  Eigen::Matrix2Xd err = reprojErr();
  Eigen::VectorXd dist = err.colwise().norm();
  mean = dist.mean();
  max = (dist.array() - mean).cwiseAbs().maxCoeff();
}

void ImgCalibPoints::reprojErr2Vec(std::vector<double>& ex, std::vector<double>& ey) const {
  auto err = this->reprojErr();
  ex.resize(err.cols());
  ey.resize(err.cols());
  for (int i = 0; i < ex.size(); i++) {
    ex[i] = err.col(i).x();
    ey[i] = err.col(i).y();
  }
}

Eigen::Matrix3Xd ImgCalibPoints::viewRaysZ1(const CameraDistModel& camera) const {
  Eigen::Matrix3Xd ret(3, imgPoints.cols());
  // Eigen::Matrix3Xd Kinv = camera.K().inverse();

  for (int i = 0; i < ret.cols(); i++) {
    Eigen::Vector3d p(imgPoints.col(i).x(), imgPoints.col(i).y(), 1);

    Eigen::Vector2d pz1d = camera.cameraModel.pointsImage_to_Z1(imgPoints.col(i));
    Eigen::Vector2d pz1 = camera.distModel.undistortSinglePoint(pz1d);

    ret.col(i).head<2>() = pz1;
    ret.col(i)(2) = 1.0;
  }

  return ret;
}

Eigen::Matrix3Xd ImgCalibPoints::viewRaysUnit(const CameraDistModel& camera) const {
  Eigen::Matrix3Xd ret = viewRaysZ1(camera);
  DEBUGME_VIEWRAYS << "ret " << ret.transpose() << std::endl;
  Eigen::Matrix3Xd ret2 = ret.colwise().normalized();
  DEBUGME_VIEWRAYS << "ret2 " << ret2.transpose() << std::endl;
  return ret2;
}

//---------------------------------------------------------------------------------------------------------

Problem::Problem(int nMovingCameras, const Eigen::Matrix3Xd& calibWorldPoints)
    : _T_W_wrt_view2(nMovingCameras, std::make_pair(false, Eigen::Isometry3d::Identity()))
    , _view2CamPar(nMovingCameras)
    , _view2CalibPoints(nMovingCameras)
    , _calibWorldPoints(calibWorldPoints)
    , _framesMeas(nMovingCameras)
    , pole_tol(.2) {
  this->identifyPoles();
}

Problem::~Problem() {}

Eigen::Matrix3Xd Problem::recoverPoints(const Eigen::ArrayXi& idx, const Eigen::Matrix3Xd& source) {
  Eigen::Matrix3Xd ret(3, idx.size());
  for (int i = 0; i < ret.cols(); i++) {
    ret.col(i) = source.col(idx(i));
  }
  return ret;
}

Eigen::Matrix3Xd Problem::computeClosest3DPoint(const Eigen::Isometry3d& T_W_wrt_C, const Eigen::Matrix3Xd& wp,
                                                const Eigen::Matrix3Xd& unitView_C) {
  Eigen::Matrix3Xd cp = T_W_wrt_C * wp;

  Eigen::Matrix3Xd ret(3, unitView_C.cols());

  for (int i = 0; i < unitView_C.cols(); i++) {
    Eigen::Vector3d uw = cp.col(i);
    double w = uw.norm();
    uw.normalize();
    DEBUGME_CLOSEST << "uw " << uw.transpose() << std::endl;
    DEBUGME_CLOSEST << "w " << w << std::endl;

    DEBUGME_CLOSEST << "unitView_C.col(i) " << unitView_C.col(i) << std::endl;
    double dot = (uw.transpose() * unitView_C.col(i));
    DEBUGME_CLOSEST << "dot " << dot << std::endl;
    double scale = w / dot;
    DEBUGME_CLOSEST << scale << std::endl;
    ret.col(i) = scale * unitView_C.col(i);
    DEBUGME_CLOSEST << ret.col(i) << std::endl;
  }

  return ret;
}

void Problem::initView1(const Eigen::Matrix2Xd& imgPoints, const Eigen::ArrayXi& worldPointIndexes, Eigen::Vector2i imgSize) {
  this->_view1CalibPoints.imgPoints = imgPoints;
  this->_view1CalibPoints.indexes = worldPointIndexes;
  Eigen::Matrix3Xd worldPoints = recoverPoints(worldPointIndexes, _calibWorldPoints);

  this->_view1CamPar.cameraModel.setImageSize(imgSize.x(), imgSize.y());
  ProjectionMatrixEstimate::estimateCameraAndRt(imgPoints, worldPoints, this->_view1CamPar.cameraModel, this->_T_W_wrt_view1);

  this->_view1CalibPoints.reprojPoints =
      this->_view1CamPar.cameraModel.points3D_to_image(this->_T_W_wrt_view1 * worldPoints, this->_view1CamPar.distModel);
  this->_view1CalibPoints.p3d_wrt_cam_closest =
      computeClosest3DPoint(_T_W_wrt_view1, worldPoints, _view1CalibPoints.viewRaysUnit(_view1CamPar));
}

void Problem::initView2(int i, const Eigen::Matrix2Xd& imgPoints, const Eigen::ArrayXi& worldPointIndexes, Eigen::Vector2i imgSize) {
  this->_view2CalibPoints[i].imgPoints = imgPoints;
  this->_view2CalibPoints[i].indexes = worldPointIndexes;
  Eigen::Matrix3Xd worldPoints = recoverPoints(worldPointIndexes, _calibWorldPoints);

  this->_view2CamPar[i].cameraModel.setImageSize(imgSize.x(), imgSize.y());
  if (imgPoints.cols() >= 6) {
    this->_T_W_wrt_view2[i].first = true;
    ProjectionMatrixEstimate::estimateCameraAndRt(imgPoints, worldPoints, this->_view2CamPar[i].cameraModel,
                                                  this->_T_W_wrt_view2[i].second);
    _validViewsIndexes.push_back(i);

    this->_view2CalibPoints[i].reprojPoints = this->_view2CamPar[i].cameraModel.points3D_to_image(
        this->_T_W_wrt_view2[i].second * worldPoints, this->_view2CamPar[i].distModel);
    this->_view2CalibPoints[i].p3d_wrt_cam_closest =
        computeClosest3DPoint(this->_T_W_wrt_view2[i].second, worldPoints, _view2CalibPoints[i].viewRaysUnit(_view2CamPar[i]));

  } else {
    this->_T_W_wrt_view2[i].first = false;
    this->_view2CalibPoints[i].reprojPoints.resize(2, 0);
    this->_view2CalibPoints[i].p3d_wrt_cam_closest.resize(3, 0);
  }
}

void Problem::setImgMeasPoints(int i, const Eigen::Matrix2Xd& view1_meas, const Eigen::Matrix2Xd& view2_meas,
                               const Eigen::VectorXi& indexes) {
  this->_framesMeas[i].resize(view1_meas.cols());
  this->_framesMeas[i].view1Points = view1_meas;
  this->_framesMeas[i].view2Points = view2_meas;
  this->_framesMeas[i].indexes = indexes;

  for (int k = 0; k < this->_framesMeas[i].p3d_est.cols(); k++) {
    Eigen::Vector2d v1_und = this->view1CamPar().undistImagePoint(this->_framesMeas[i].view1Points.col(k));
    Eigen::Vector2d v2_und = this->view2CamPar(i).undistImagePoint(this->_framesMeas[i].view2Points.col(k));

    this->_framesMeas[i].p3d_est.col(k) = Triangulate::triangulateLinear(
        ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
        ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und);

    this->_framesMeas[i].p3d_est.col(k) = Triangulate::triangulateNonLinear(
        ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
        ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und,
        this->_framesMeas[i].p3d_est.col(k));
  }

  this->_framesMeas[i].view1Points_repr =
      this->_view1CamPar.cameraModel.points3D_to_image(this->_T_W_wrt_view1 * this->_framesMeas[i].p3d_est);
  this->_framesMeas[i].view2Points_repr =
      this->_view2CamPar[i].cameraModel.points3D_to_image(this->_T_W_wrt_view2[i].second * this->_framesMeas[i].p3d_est);
}

void Problem::recomputeImgMeasTriangulation() {
  for (int i = 0; i < this->_framesMeas.size(); i++) {
    for (int k = 0; k < this->_framesMeas[i].p3d_est.cols(); k++) {
      Eigen::Vector2d v1_und = this->view1CamPar().undistImagePoint(this->_framesMeas[i].view1Points.col(k));
      Eigen::Vector2d v2_und = this->view2CamPar(i).undistImagePoint(this->_framesMeas[i].view2Points.col(k));

      this->_framesMeas[i].p3d_est.col(k) = Triangulate::triangulateLinear(
          ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
          ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und);

      this->_framesMeas[i].p3d_est.col(k) = Triangulate::triangulateNonLinear(
          ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
          ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und,
          this->_framesMeas[i].p3d_est.col(k));
    }

    this->_framesMeas[i].view1Points_repr =
        this->_view1CamPar.cameraModel.points3D_to_image(this->_T_W_wrt_view1 * this->_framesMeas[i].p3d_est);
    this->_framesMeas[i].view2Points_repr =
        this->view2CamPar(i).cameraModel.points3D_to_image(this->_T_W_wrt_view2[i].second * this->_framesMeas[i].p3d_est);
  }
}

void Problem::identifyPoles() {
  _polesPointPairs.resize(3, _calibWorldPoints.cols());
  Eigen::VectorXf toBeUsed(_calibWorldPoints.cols());
  toBeUsed.setConstant(1);
  int poles = 0;
  for (int i = 0; i < _calibWorldPoints.cols(); i++) {
    if (toBeUsed(i) == 0)
      continue;
    toBeUsed(i) = 0;
    Eigen::Vector3d sourcePoint = _calibWorldPoints.col(i);
    // this can be optimized, but points are so few that is not worth
    for (int j = i + 1; j < _calibWorldPoints.cols(); j++) {
      if (toBeUsed(j) == 0)
        continue;
      Eigen::Vector3d destPoint = _calibWorldPoints.col(j);
      Eigen::Vector3d dist = destPoint - sourcePoint;
      if (std::abs(dist.x()) < pole_tol && std::abs(dist.z()) < pole_tol) {
        // found!
        toBeUsed(j) = 1;
        _polesPointPairs.col(2 * poles) = sourcePoint;
        _polesPointPairs.col(2 * poles + 1) = destPoint;
        poles++;
      }
    }
  }

  _polesPointPairs.conservativeResize(3, poles * 2);
}

std::vector<double> Problem::collectView2Cx() const {
  std::vector<double> ret;
  ret.reserve(this->numViews());
  for (int i = 0; i < this->numViews(); i++) {
    if (this->T_W_wrt_view2(i).first) {
      ret.push_back(this->view2CamPar(i).cameraModel.center().x());
    }
  }
  return ret;
}

std::vector<double> Problem::collectView2Cy() const {
  std::vector<double> ret;
  ret.reserve(this->numViews());
  for (int i = 0; i < this->numViews(); i++) {
    if (this->T_W_wrt_view2(i).first) {
      ret.push_back(this->view2CamPar(i).cameraModel.center().y());
    }
  }
  return ret;
}

std::vector<double> Problem::collectView2Fx() const {
  std::vector<double> ret;
  ret.reserve(this->numViews());
  for (int i = 0; i < this->numViews(); i++) {
    if (this->T_W_wrt_view2(i).first) {
      ret.push_back(this->view2CamPar(i).cameraModel.fx());
    }
  }
  return ret;
}

std::vector<double> Problem::collectView2Fy() const {
  std::vector<double> ret;
  ret.reserve(this->numViews());
  for (int i = 0; i < this->numViews(); i++) {
    if (this->T_W_wrt_view2(i).first) {
      ret.push_back(this->view2CamPar(i).cameraModel.fy());
    }
  }
  return ret;
}

std::vector<Eigen::Isometry3d> Problem::collect_T_W_wrt_view2() const {
  std::vector<Eigen::Isometry3d> ret;
  ret.reserve(this->numViews());
  for (int i = 0; i < this->numViews(); i++) {
    if (this->T_W_wrt_view2(i).first) {
      ret.push_back(this->T_W_wrt_view2(i).second);
    }
  }
  return ret;
}

double median(const std::vector<double>& valuesIn) {
  std::vector<double> values = valuesIn;
  std::sort(values.begin(), values.end());
  assert(values.size() > 0);
  if (values.size() % 2 == 1) {
    return values[(values.size() - 1) / 2];
  } else {
    return (values[values.size() / 2 - 1] + values[values.size() / 2]) * 0.5;
  }
}

Eigen::Vector2d Problem::view2fxfyMedian() const {
  Eigen::Vector2d ret(median(this->collectView2Fx()), median(this->collectView2Fy()));
  return ret;
}

Eigen::Vector2d Problem::view2cxcyMedian() const {
  Eigen::Vector2d ret(median(this->collectView2Cx()), median(this->collectView2Cy()));
  return ret;
}

void Problem::recomputeCameraParamsPoses() {
  // full
  // std::array<bool, 6> k_flags_fixed = { false, false, false, false, false, false };
  // std::array<bool, 2> p_flags_fixed = { false, false };
  // std::array<bool, 4> s_flags_fixed = { false, false, false, false };

  // standard
  std::array<bool, 6> k_flags_fixed = {false, false, false, true, true, true};
  std::array<bool, 2> p_flags_fixed = {false, false};
  std::array<bool, 4> s_flags_fixed = {true, true, true, true};

  // advanced
  // std::array<bool, 6> k_flags_fixed = { false, false, false, true, true, true };
  // std::array<bool, 2> p_flags_fixed = { false, false };
  // std::array<bool, 4> s_flags_fixed = { false, false,false, false };

  // no dist
  // std::array<bool, 6> k_flags = { true, true, true, true, true, true };
  // std::array<bool, 2> p_flags = { true, true };
  // std::array<bool, 4> s_flags_fixed = { true, true ,true, true };

  CameraNode camera1(this->view1CamPar().cameraModel, this->view1CamPar().distModel, CameraNode::FocalEstimation::Both,
                     {false, false}, // center
                     k_flags_fixed, p_flags_fixed,
                     s_flags_fixed // s1234
  );

  auto view2fxfyMedian = this->view2fxfyMedian();
  auto view2cxcyMedian = this->view2cxcyMedian();
  CameraNode camera2(this->uniqueCamParamsView2()
                         ? this->view2CamPar(0).cameraModel
                         : csCamera::Camerad(view2fxfyMedian.x(), view2fxfyMedian.y(), view2cxcyMedian.x(), view2cxcyMedian.y(),
                                             this->view2CamPar(0).cameraModel.w(), this->view2CamPar(0).cameraModel.h()),
                     this->uniqueCamParamsView2() ? this->view2CamPar(0).distModel
                                                  : csCamera::CameraDistortionModeld(0, 0, 0,   // k123
                                                                                     0, 0,      // p12
                                                                                     0, 0, 0,   // k456
                                                                                     0, 0, 0, 0 // s1234
                                                                                     ),
                     CameraNode::FocalEstimation::Both, {false, false}, // center
                     k_flags_fixed, p_flags_fixed,
                     s_flags_fixed // s1234
  );

  CameraCalibSection section = CameraCalibSection(camera1, camera2, this->T_W_wrt_view1(), this->collect_T_W_wrt_view2());
  // enable this line to optimize only view 1 (debugging...)
  // CameraCalibSection section = CameraCalibSection(camera1, camera2, this->T_W_wrt_view1(), {});

  // add repr error of first camera
  section.addEdge(section.camera1ParId(), section.camera1PoseId(),
                  new Camera3dPointReprojectionError(recoverPoints(this->view1CalibPoints().indexes, _calibWorldPoints),
                                                     this->view1CalibPoints().imgPoints));

  // add repr error of other views
  if (section.numView2Poses() > 0) {
    for (int i = 0; i < this->numViews(); i++) {
      if (this->T_W_wrt_view2(i).first) {
        section.addEdge(section.camera2ParId(), section.camera2PoseId(i),
                        new Camera3dPointReprojectionError(recoverPoints(this->view2CalibPoints(i).indexes, _calibWorldPoints),
                                                           this->view2CalibPoints(i).imgPoints));
      }
    }
  }

  section.structureReady();

  csNelson::LevenbergMarquardt<
      typename csNelson::SolverTraits<csNelson::solverCholeskyDense>::Solver<typename CameraCalibSection::Hessian::Traits>>
      lm;

  lm.settings().epsBVector = 1e-6;
  lm.settings().epsChi2 = 1e-6;
  lm.settings().epsIncSquare = 1e-6;
  lm.settings().maxNumIt = 100;
  lm.settings().minNumIt = 3;
  // gn.settings().absLambda = 100000.;

  auto tc = lm.solve(section);

  DEBUGME "--- recomputeCameraParamsPoses ---" << std::endl << lm.stats().toString() << std::endl << std::endl;

  // update view1
  _view1CamPar.cameraModel = section.view1Camera();
  _view1CamPar.distModel = section.view1CameraDistModel();
  _view2CamPar = {{section.view2Camera(), section.view2CameraDistModel()}}; // unique
  _T_W_wrt_view1 = section.view1Pose();
  int si = 0;
  if (section.numView2Poses() > 0) {
    for (int i = 0; i < this->numViews(); i++) {
      if (this->T_W_wrt_view2(i).first) {
        this->_T_W_wrt_view2[i].second = section.view2Pose(i);

        Eigen::Matrix3Xd worldPoints = recoverPoints(this->_view2CalibPoints[i].indexes, _calibWorldPoints);

        this->_view2CalibPoints[i].reprojPoints = this->_view2CamPar[0].cameraModel.points3D_to_image(
            this->_T_W_wrt_view2[i].second * worldPoints, this->_view2CamPar[0].distModel);
        this->_view2CalibPoints[i].p3d_wrt_cam_closest =
            computeClosest3DPoint(this->_T_W_wrt_view2[i].second, worldPoints, _view2CalibPoints[i].viewRaysUnit(_view2CamPar[0]));
      }
      si++;
    }
  }

  // update points repr
  Eigen::Matrix3Xd worldPoints = recoverPoints(this->_view1CalibPoints.indexes, _calibWorldPoints);
  this->_view1CalibPoints.reprojPoints =
      this->_view1CamPar.cameraModel.points3D_to_image(this->_T_W_wrt_view1 * worldPoints, this->_view1CamPar.distModel);
  this->_view1CalibPoints.p3d_wrt_cam_closest =
      computeClosest3DPoint(_T_W_wrt_view1, worldPoints, _view1CalibPoints.viewRaysUnit(_view1CamPar));

  // update triangulated points
  this->recomputeImgMeasTriangulation();
}

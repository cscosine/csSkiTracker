#include "ProblemDescription.h"

#include <Eigen/Geometry>
#include <array>

#include <iostream>

#include <QChar>
#include <QDir>

#include "Triangulate.h"

#include "Camera3dPointReprojectionError.h"
#include "CameraCalibSection.h"
#include "CameraNode.hpp"
#include "PoseNode.h"
#include <csCamera/CameraDistortionModel_impl.hpp>
#include <csCamera/Camera_impl.hpp>
#include <csNelson/GaussNewton_impl.hpp>
#include <csNelson/LevenbergMarquardt_impl.hpp>
#include <csNelson/SingleSection_impl.hpp>

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

Problem::Problem(int nMovingCameras, const Eigen::Matrix3Xd& calibWorldPoints, const QString& f1, int s1, const QString& f2, int s2)
    : _T_W_wrt_view2(nMovingCameras, std::make_pair(false, Eigen::Isometry3d::Identity()))
    , _view2CamPar(nMovingCameras)
    , _view2CalibPoints(nMovingCameras)
    , _calibWorldPoints(calibWorldPoints)
    , _framesMeas(nMovingCameras)
    , _framesSkierPoints(nMovingCameras)
    , _framesSkierModel(nMovingCameras)
    , pole_tol(.2)
    , _folderPathView1(f1)
    , _s1(s1)
    , _folderPathView2(f2)
    , _s2(s2) {
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

    this->_view2CalibPoints[i].reprojPoints = this->view2CamPar(i).cameraModel.points3D_to_image(
        this->_T_W_wrt_view2[i].second * worldPoints, this->view2CamPar(i).distModel);
    this->_view2CalibPoints[i].p3d_wrt_cam_closest =
        computeClosest3DPoint(this->_T_W_wrt_view2[i].second, worldPoints, _view2CalibPoints[i].viewRaysUnit(view2CamPar(i)));

  } else {
    this->_T_W_wrt_view2[i].first = false;
    this->_view2CalibPoints[i].reprojPoints.resize(2, 0);
    this->_view2CalibPoints[i].p3d_wrt_cam_closest.resize(3, 0);
  }
}

void Problem::setImgSkierPoints(int i, const Eigen::Matrix2Xd& view1_meas, const Eigen::Matrix2Xd& view2_meas) {
  this->_framesSkierPoints[i].resize(view1_meas.cols());
  this->_framesSkierPoints[i].view1Points = view1_meas;
  this->_framesSkierPoints[i].view2Points = view2_meas;

  for (int k = 0; k < this->_framesSkierPoints[i].p3d_est.cols(); k++) {
    Eigen::Vector2d v1_und = this->view1CamPar().undistImagePoint(this->_framesSkierPoints[i].view1Points.col(k));
    Eigen::Vector2d v2_und = this->view2CamPar(i).undistImagePoint(this->_framesSkierPoints[i].view2Points.col(k));

    this->_framesSkierPoints[i].p3d_est.col(k) = Triangulate::triangulateLinear(
        ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
        ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und);

    this->_framesSkierPoints[i].p3d_est.col(k) = Triangulate::triangulateNonLinear(
        ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
        ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und,
        this->_framesSkierPoints[i].p3d_est.col(k));
  }

  this->_framesSkierPoints[i].view1Points_repr = this->_view1CamPar.cameraModel.points3D_to_image(
      this->_T_W_wrt_view1 * this->_framesSkierPoints[i].p3d_est, this->_view1CamPar.distModel);
  this->_framesSkierPoints[i].view2Points_repr = this->view2CamPar(i).cameraModel.points3D_to_image(
      this->_T_W_wrt_view2[i].second * this->_framesSkierPoints[i].p3d_est, this->view2CamPar(i).distModel);

  this->_framesSkierModel[i].setFromPoints(this->_framesSkierPoints[i].p3d_est);
}

void Problem::updateSkierModelFromPoints() {
  for (int i = 0; i < this->_framesSkierPoints.size(); i++) {
    this->_framesSkierModel[i].setFromPoints(this->_framesSkierPoints[i].p3d_est);
  }
}

void Problem::recomputeImgSkierReprojErrors() {
  for (int i = 0; i < this->_framesSkierPoints.size(); i++) {
    this->_framesSkierPoints[i].view1Points_repr = this->_view1CamPar.cameraModel.points3D_to_image(
        this->_T_W_wrt_view1 * this->_framesSkierPoints[i].p3d_est, this->_view1CamPar.distModel);
    this->_framesSkierPoints[i].view2Points_repr = this->view2CamPar(i).cameraModel.points3D_to_image(
        this->_T_W_wrt_view2[i].second * this->_framesSkierPoints[i].p3d_est, this->view2CamPar(i).distModel);
  }
}

void Problem::recomputeImgSkierTriangulation() {
  for (int i = 0; i < this->_framesSkierPoints.size(); i++) {
    for (int k = 0; k < this->_framesSkierPoints[i].p3d_est.cols(); k++) {
      Eigen::Vector2d v1_und = this->view1CamPar().undistImagePoint(this->_framesSkierPoints[i].view1Points.col(k));
      Eigen::Vector2d v2_und = this->view2CamPar(i).undistImagePoint(this->_framesSkierPoints[i].view2Points.col(k));

      this->_framesSkierPoints[i].p3d_est.col(k) = Triangulate::triangulateLinear(
          ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
          ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und);

      this->_framesSkierPoints[i].p3d_est.col(k) = Triangulate::triangulateNonLinear(
          ProjectionMatrixEstimate::createP(this->view1CamPar().cameraModel, this->T_W_wrt_view1()), v1_und,
          ProjectionMatrixEstimate::createP(this->view2CamPar(i).cameraModel, this->T_W_wrt_view2(i).second), v2_und,
          this->_framesSkierPoints[i].p3d_est.col(k));
    }

    this->_framesSkierPoints[i].view1Points_repr = this->_view1CamPar.cameraModel.points3D_to_image(
        this->_T_W_wrt_view1 * this->_framesSkierPoints[i].p3d_est, this->_view1CamPar.distModel);
    this->_framesSkierPoints[i].view2Points_repr = this->view2CamPar(i).cameraModel.points3D_to_image(
        this->_T_W_wrt_view2[i].second * this->_framesSkierPoints[i].p3d_est, this->view2CamPar(i).distModel);

    this->_framesSkierModel[i].setFromPoints(this->_framesSkierPoints[i].p3d_est);
  }
}

void Problem::setImgMatchingPoints(int i, const Eigen::Matrix2Xd& view1_meas, const Eigen::Matrix2Xd& view2_meas) {
  this->_framesMeas[i].resize(view1_meas.cols());
  this->_framesMeas[i].view1Points = view1_meas;
  this->_framesMeas[i].view2Points = view2_meas;

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

  this->_framesMeas[i].view1Points_repr = this->_view1CamPar.cameraModel.points3D_to_image(
      this->_T_W_wrt_view1 * this->_framesMeas[i].p3d_est, this->_view1CamPar.distModel);
  this->_framesMeas[i].view2Points_repr = this->view2CamPar(i).cameraModel.points3D_to_image(
      this->_T_W_wrt_view2[i].second * this->_framesMeas[i].p3d_est, this->view2CamPar(i).distModel);
}

void Problem::recomputeImgMatchingTriangulation() {
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

    this->_framesMeas[i].view1Points_repr = this->_view1CamPar.cameraModel.points3D_to_image(
        this->_T_W_wrt_view1 * this->_framesMeas[i].p3d_est, this->_view1CamPar.distModel);
    this->_framesMeas[i].view2Points_repr = this->view2CamPar(i).cameraModel.points3D_to_image(
        this->_T_W_wrt_view2[i].second * this->_framesMeas[i].p3d_est, this->view2CamPar(i).distModel);
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

Eigen::VectorXi Problem::indexesCalibrationPointsVisibleMultipleTimes(int minN) const {
  Eigen::VectorXi countView1 = Eigen::VectorXi::Zero(_calibWorldPoints.cols());
  Eigen::VectorXi countView2 = Eigen::VectorXi::Zero(_calibWorldPoints.cols());

  for (int i = 0; i < this->view1CalibPoints().indexes.size(); i++) {
    countView1(this->view1CalibPoints().indexes(i))++;
  }

  for (int vi = 0; vi < this->numValidViews(); vi++) {
    int v = _validViewsIndexes[vi];
    for (int i = 0; i < this->view2CalibPoints(v).indexes.size(); i++) {
      countView2(this->view2CalibPoints(v).indexes(i))++;
    }
  }

  Eigen::VectorXi common = ((countView1 + countView2).array() > minN).select(Eigen::VectorXi::Ones(_calibWorldPoints.cols()), 0);
  Eigen::VectorXi pointEstimateIndexes(common.sum());
  int count = 0;
  for (int i = 0; i < common.size(); i++) {
    if (common(i))
      pointEstimateIndexes(count++) = i;
  }
  assert(count == pointEstimateIndexes.size());

  // enable for debug
  if (false) {
    // print common points
    Eigen::Matrix<int, Eigen::Dynamic, 4> tmp(countView1.size(), 4);
    tmp.col(0).setLinSpaced(tmp.col(0).size(), 0, tmp.col(0).size() - 1);
    tmp.col(1) = countView1;
    tmp.col(2) = countView2;
    tmp.col(3) = common;
    std::cout << "#Point #V1 #V2" << std::endl << tmp << std::endl;
  }

  return pointEstimateIndexes;
}

Eigen::VectorXi Problem::applyMask2CalibPoints(const Eigen::VectorXi& sel) {
  auto mask = this->selectedVectorToBoolMask(sel);

  Eigen::VectorXi oldCalibId2NewCalibId(_calibWorldPoints.cols());
  oldCalibId2NewCalibId.setConstant(-1);
  int c = 0;
  for (int i = 0; i < oldCalibId2NewCalibId.size(); i++) {
    if (mask(i)) {
      oldCalibId2NewCalibId(i) = c++;
    }
  }

  // recompute calib world points
  Eigen::Matrix3Xd newCalibWorldPoints(3, c);
  for (int i = 0; i < sel.size(); i++) {
    newCalibWorldPoints.col(i) = _calibWorldPoints.col(sel(i));
  }
  _calibWorldPoints = newCalibWorldPoints;

  // view1 calib points
  ImgCalibPoints newView1CalibPoints = _view1CalibPoints;
  c = 0;
  for (int i = 0; i < _view1CalibPoints.indexes.size(); i++) {
    if (mask(_view1CalibPoints.indexes(i))) {
      newView1CalibPoints.imgPoints.col(c) = _view1CalibPoints.imgPoints.col(i);
      newView1CalibPoints.indexes(c) = oldCalibId2NewCalibId(_view1CalibPoints.indexes(i));
      newView1CalibPoints.reprojPoints.col(c) = _view1CalibPoints.reprojPoints.col(i);
      newView1CalibPoints.p3d_wrt_cam_closest.col(c) = _view1CalibPoints.p3d_wrt_cam_closest.col(i);
      assert(newView1CalibPoints.indexes(c) != -1);
      c++;
    }
  }
  newView1CalibPoints.imgPoints.conservativeResize(2, c);
  newView1CalibPoints.indexes.conservativeResize(c);
  newView1CalibPoints.reprojPoints.conservativeResize(2, c);
  newView1CalibPoints.p3d_wrt_cam_closest.conservativeResize(3, c);
  _view1CalibPoints = newView1CalibPoints;

  // view2 calib points
  for (int j = 0; j < _view2CalibPoints.size(); j++) {
    ImgCalibPoints newView2jCalibPoints = _view2CalibPoints[j];
    c = 0;
    for (int i = 0; i < _view2CalibPoints[j].indexes.size(); i++) {
      if (mask(_view2CalibPoints[j].indexes(i))) {
        newView2jCalibPoints.imgPoints.col(c) = _view2CalibPoints[j].imgPoints.col(i);
        newView2jCalibPoints.indexes(c) = oldCalibId2NewCalibId(_view2CalibPoints[j].indexes(i));
        newView2jCalibPoints.reprojPoints.col(c) = _view2CalibPoints[j].reprojPoints.col(i);
        newView2jCalibPoints.p3d_wrt_cam_closest.col(c) = _view2CalibPoints[j].p3d_wrt_cam_closest.col(i);
        assert(newView2jCalibPoints.indexes(c) != -1);
        c++;
      }
    }
    newView2jCalibPoints.imgPoints.conservativeResize(2, c);
    newView2jCalibPoints.indexes.conservativeResize(c);
    newView2jCalibPoints.reprojPoints.conservativeResize(2, c);
    newView2jCalibPoints.p3d_wrt_cam_closest.conservativeResize(3, c);
    _view2CalibPoints[j] = newView2jCalibPoints;
  }

  // recompute poles
  this->identifyPoles();

  return oldCalibId2NewCalibId;
}

Eigen::Matrix<bool, Eigen::Dynamic, 1> Problem::selectedVectorToBoolMask(const Eigen::VectorXi& sel) const {
  Eigen::Matrix<bool, Eigen::Dynamic, 1> selectedBin = Eigen::Matrix<bool, Eigen::Dynamic, 1>(_calibWorldPoints.cols());
  selectedBin.setConstant(false);
  for (int i = 0; i < sel.size(); i++) {
    selectedBin(sel(i)) = true;
  }

  return selectedBin;
}

Eigen::VectorXi Problem::indexCalibrationPointsNotSelected(const Eigen::VectorXi& sel) const {
  auto selectedBin = selectedVectorToBoolMask(sel);

  Eigen::VectorXi ret(_calibWorldPoints.cols() - sel.size());
  int c = 0;
  for (int i = 0; i < selectedBin.size(); i++) {
    if (selectedBin(i) == 0) {
      ret(c++) = i;
    }
  }
  assert(c == ret.size());
  return ret;
}

Eigen::VectorXi Problem::indexesCalibrationPointsVisibleFromBoth() const {
  Eigen::VectorXi countView1 = Eigen::VectorXi::Zero(_calibWorldPoints.cols());
  Eigen::VectorXi countView2 = Eigen::VectorXi::Zero(_calibWorldPoints.cols());

  for (int i = 0; i < this->view1CalibPoints().indexes.size(); i++) {
    countView1(this->view1CalibPoints().indexes(i))++;
  }

  for (int vi = 0; vi < this->numValidViews(); vi++) {
    int v = _validViewsIndexes[vi];
    for (int i = 0; i < this->view2CalibPoints(v).indexes.size(); i++) {
      countView2(this->view2CalibPoints(v).indexes(i))++;
    }
  }
  Eigen::VectorXi common =
      (countView1.array() > 0 && countView2.array() > 0).select(Eigen::VectorXi::Ones(_calibWorldPoints.cols()), 0);

  Eigen::VectorXi pointEstimateIndexes(common.sum());
  int count = 0;
  for (int i = 0; i < common.size(); i++) {
    if (common(i))
      pointEstimateIndexes(count++) = i;
  }
  assert(count == pointEstimateIndexes.size());

  // enable for debug
  if (false) {
    // print common points
    Eigen::Matrix<int, Eigen::Dynamic, 4> tmp(countView1.size(), 4);
    tmp.col(0).setLinSpaced(tmp.col(0).size(), 0, tmp.col(0).size() - 1);
    tmp.col(1) = countView1;
    tmp.col(2) = countView2;
    tmp.col(3) = common;
    std::cout << "#Point #V1 #V2" << std::endl << tmp << std::endl;
  }

  return pointEstimateIndexes;
}

// full
// static const std::array<bool, 6> k_flags_fixed = { false, false, false, false, false, false };
// static const std::array<bool, 2> p_flags_fixed = { false, false };
// static const std::array<bool, 4> s_flags_fixed = { false, false, false, false };

// standard
static const std::array<bool, 6> k_flags_fixed = {false, false, false, true, true, true};
static const std::array<bool, 2> p_flags_fixed = {false, false};
static const std::array<bool, 4> s_flags_fixed = {true, true, true, true};

// advanced
// static const std::array<bool, 6> k_flags_fixed = { false, false, false, true, true, true };
// static const std::array<bool, 2> p_flags_fixed = { false, false };
// static const std::array<bool, 4> s_flags_fixed = { false, false,false, false };

// no dist
// static const std::array<bool, 6> k_flags = { true, true, true, true, true, true };
// static const std::array<bool, 2> p_flags = { true, true };
// static const std::array<bool, 4> s_flags_fixed = { true, true ,true, true };

Eigen::Vector3i Problem::recomputeCameraParamsPosesPoints3Fixed(int minViews, Eigen::Vector3i index3FixedPoints,
                                                                bool includeSkierPoints) {
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

  Eigen::VectorXi movablePointsIndexesAll;

  if (minViews <= 1) {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleFromBoth();
  } else {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleMultipleTimes(minViews);
  }

  // remove unused points!!
  Eigen::VectorXi oldCalibId2NewCalibId = this->applyMask2CalibPoints(movablePointsIndexesAll);

  // rename the fixed points
  bool ok = true;
  for (int j = 0; j < index3FixedPoints.size(); j++) {
    index3FixedPoints(j) = oldCalibId2NewCalibId(index3FixedPoints(j));
    if (index3FixedPoints(j) == -1) {
      ok = false;
    }
  }
  if (!ok)
    return index3FixedPoints;

  // and now get them again, they have changed ids
  if (minViews <= 1) {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleFromBoth();
  } else {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleMultipleTimes(minViews);
  }

  std::cout << movablePointsIndexesAll.transpose() << std::endl;

  Eigen::VectorXi movablePointsIndexes(movablePointsIndexesAll.size() - index3FixedPoints.size());
  int c = 0;
  for (int i = 0; i < movablePointsIndexesAll.size(); i++) {
    bool found = false;
    int s = movablePointsIndexesAll(i);
    for (int j = 0; j < index3FixedPoints.size(); j++) {
      if (s == index3FixedPoints(j)) {
        found = true;
        break;
      }
    }
    if (!found) {
      movablePointsIndexes(c++) = s;
    }
  }
  assert(c == movablePointsIndexes.size());
  std::cout << "fixedPointsIndexes   : " << index3FixedPoints.transpose() << std::endl;
  std::cout << "movablePointsIndexes : " << movablePointsIndexes.transpose() << std::endl;

  Eigen::VectorXi origPointIndex2MovableIndexMap(_calibWorldPoints.cols());
  origPointIndex2MovableIndexMap.setConstant(-1);
  for (int i = 0; i < movablePointsIndexes.size(); i++) {
    origPointIndex2MovableIndexMap(movablePointsIndexes(i)) = i;
  }
  for (int i = 0; i < index3FixedPoints.size(); i++) {
    origPointIndex2MovableIndexMap(index3FixedPoints(i)) = -i - 2;
  }

  assert(index3FixedPoints.size() >= 3);

  Eigen::Matrix3Xd movablePoints = recoverPoints(movablePointsIndexes, _calibWorldPoints);
  Eigen::Matrix3Xd fixedPoints = recoverPoints(index3FixedPoints, _calibWorldPoints);

  std::vector<Eigen::Matrix3Xd> skierPoints;
  if (includeSkierPoints) {
    skierPoints.resize(this->_framesSkierPoints.size());
    for (int i = 0; i < this->_framesSkierPoints.size(); i++) {
      skierPoints[i] = this->_framesSkierPoints[i].p3d_est;
    }
  }

  CameraCalibSection section = CameraCalibSection(camera1, camera2, this->T_W_wrt_view1(), this->collect_T_W_wrt_view2(),
                                                  movablePoints, fixedPoints, skierPoints);

  // add repr error of first camera
  for (int i = 0; i < this->view1CalibPoints().imgPoints.cols(); i++) {
    int pointId = origPointIndex2MovableIndexMap(this->view1CalibPoints().indexes(i));
    if (pointId >= 0) {
      section.addEdge(
          {
              section.camera1ParId(),
              section.camera1PoseId(),
              section.pointId(pointId),
          },
          new Camera3dPointReprojectionError(this->view1CalibPoints().imgPoints.col(i)));
    } else if (pointId <= -2) {
      pointId = -pointId - 2;
      csNelson::NodeId nodeId = csNelson::NodeId(pointId, csNelson::NodeType::Fixed);

      section.addEdge({section.camera1ParId(), section.camera1PoseId(), nodeId},
                      new Camera3dPointReprojectionError(this->view1CalibPoints().imgPoints.col(i)));
    }
  }

  // add repr error of other views
  if (section.numView2Poses() > 0) {
    for (int i = 0; i < this->numViews(); i++) {
      if (this->T_W_wrt_view2(i).first) {
        for (int j = 0; j < this->view2CalibPoints(i).imgPoints.cols(); j++) {
          int pointId = origPointIndex2MovableIndexMap(this->view2CalibPoints(i).indexes(j));
          if (pointId >= 0) {
            section.addEdge(
                {
                    section.camera2ParId(),
                    section.camera2PoseId(i),
                    section.pointId(pointId),
                },
                new Camera3dPointReprojectionError(this->view2CalibPoints(i).imgPoints.col(j)));
          } else if (pointId <= -2) {
            pointId = -pointId - 2;
            csNelson::NodeId nodeId = csNelson::NodeId(pointId, csNelson::NodeType::Fixed);
            section.addEdge({section.camera2ParId(), section.camera2PoseId(i), nodeId},
                            new Camera3dPointReprojectionError(this->view2CalibPoints(i).imgPoints.col(j)));
          }
        }
      }
    }
  }

  // add repr error of skiers
  if (includeSkierPoints) {

    for (int i = 0; i < _framesSkierPoints.size(); i++) {
      for (int j = 0; j < _framesSkierPoints[i].p3d_est.cols(); j++) {
        auto pIndex = section.skierPointId(i, j);
        // fixed view
        section.addEdge({section.camera1ParId(), section.camera1PoseId(), pIndex},
                        new Camera3dPointReprojectionError(_framesSkierPoints[i].view1Points.col(j)));
        // movable view
        section.addEdge({section.camera2ParId(), section.camera2PoseId(i), pIndex},
                        new Camera3dPointReprojectionError(_framesSkierPoints[i].view2Points.col(j)));
      }
    }
  }

  section.permuteAMD();
  section.structureReady();

  section.settings().edgeEvalParallelSettings.setNumThreadsMax();
  section.settings().hessianUpdateParallelSettings.setNumThreadsMax();

  csNelson::LevenbergMarquardt<typename csNelson::SolverTraits<csNelson::solverCholeskySparse>::Solver<
      typename CameraCalibSection::Hessian::Traits, csNelson::choleskyNaturalOrdering>>
      lm;
  // csNelson::LevenbergMarquardt<typename csNelson::SolverTraits<csNelson::solverCholeskyDense>::Solver<typename
  // CameraCalibSection::Hessian::Traits>>  lm;

  lm.settings().epsBVector = 1e-6;
  lm.settings().epsChi2 = 1e-6;
  lm.settings().epsIncSquare = 1e-6;
  lm.settings().maxNumIt = 500;
  lm.settings().minNumIt = 3;

  // lm.settings().maxNumSubIt = 10;
  // gn.settings().absLambda = 100000.;

  auto t0 = std::chrono::steady_clock::now();
  auto tc = lm.solve(section);
  auto t1 = std::chrono::steady_clock::now();

  DEBUGME "--- recomputeCameraParamsPosesPoints3Fixed ---"
      << std::endl
      << "- termination: " << csNelson::LevenbergMarquardtUtils::toString(tc) << std::endl
      << lm.stats().toString() << "TIME " << std::chrono::duration<double>(t1 - t0).count() << std::endl
      << std::endl;

  // update world points
  for (int i = 0; i < movablePointsIndexes.size(); i++) {
    int id = movablePointsIndexes(i);
    _calibWorldPoints.col(id) = section.point(i);
  }

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

        this->_view2CalibPoints[i].reprojPoints = this->view2CamPar(i).cameraModel.points3D_to_image(
            this->_T_W_wrt_view2[i].second * worldPoints, this->view2CamPar(i).distModel);
        this->_view2CalibPoints[i].p3d_wrt_cam_closest =
            computeClosest3DPoint(this->_T_W_wrt_view2[i].second, worldPoints, _view2CalibPoints[i].viewRaysUnit(_view2CamPar[0]));
      }
      si++;
    }
  }

  // update skier points
  {
    for (int i = 0; i < this->_framesSkierPoints.size(); i++) {
      for (int k = 0; k < this->_framesSkierPoints[i].p3d_est.cols(); k++) {
        this->_framesSkierPoints[i].p3d_est.col(k) = section.skierPoint(i, k);
      }
    }
  }

  // update points repr
  Eigen::Matrix3Xd worldPoints = recoverPoints(this->_view1CalibPoints.indexes, _calibWorldPoints);
  this->_view1CalibPoints.reprojPoints =
      this->_view1CamPar.cameraModel.points3D_to_image(this->_T_W_wrt_view1 * worldPoints, this->_view1CamPar.distModel);
  this->_view1CalibPoints.p3d_wrt_cam_closest =
      computeClosest3DPoint(_T_W_wrt_view1, worldPoints, _view1CalibPoints.viewRaysUnit(_view1CamPar));

  // update poles
  this->identifyPoles();

  // update triangulated points
  this->recomputeImgMatchingTriangulation();
  if (!includeSkierPoints) {
    this->recomputeImgSkierTriangulation();
  } else {
    this->recomputeImgSkierReprojErrors();
    this->updateSkierModelFromPoints();
  }

  return index3FixedPoints;
}

Eigen::Vector3i Problem::recomputeCameraParamsPosesPoints2Fixed(int minViews, Eigen::Vector3i index3FixedPoints,
                                                                bool includeSkierPoints) {

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

  Eigen::VectorXi movablePointsIndexesAll;

  if (minViews <= 1) {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleFromBoth();
  } else {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleMultipleTimes(minViews);
  }

  // remove unused points!!
  Eigen::VectorXi oldCalibId2NewCalibId = this->applyMask2CalibPoints(movablePointsIndexesAll);

  // rename the fixed points
  bool ok = true;
  for (int j = 0; j < index3FixedPoints.size(); j++) {
    index3FixedPoints(j) = oldCalibId2NewCalibId(index3FixedPoints(j));
    if (index3FixedPoints(j) == -1) {
      ok = false;
    }
  }
  if (!ok)
    return index3FixedPoints;

  // and now get them again, they have changed ids
  if (minViews <= 1) {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleFromBoth();
  } else {
    movablePointsIndexesAll = this->indexesCalibrationPointsVisibleMultipleTimes(minViews);
  }

  std::cout << movablePointsIndexesAll.transpose() << std::endl;

  Eigen::VectorXi movablePointsIndexes(movablePointsIndexesAll.size() - index3FixedPoints.size());
  int c = 0;
  for (int i = 0; i < movablePointsIndexesAll.size(); i++) {
    bool found = false;
    int s = movablePointsIndexesAll(i);
    for (int j = 0; j < index3FixedPoints.size(); j++) {
      if (s == index3FixedPoints(j)) {
        found = true;
        break;
      }
    }
    if (!found) {
      movablePointsIndexes(c++) = s;
    }
  }
  assert(c == movablePointsIndexes.size());
  std::cout << "fixedPointsIndexes   : " << index3FixedPoints.transpose() << std::endl;
  std::cout << "movablePointsIndexes : " << movablePointsIndexes.transpose() << std::endl;

  Eigen::VectorXi origPointIndex2MovableIndexMap(_calibWorldPoints.cols());
  origPointIndex2MovableIndexMap.setConstant(-1);
  for (int i = 0; i < movablePointsIndexes.size(); i++) {
    origPointIndex2MovableIndexMap(movablePointsIndexes(i)) = i;
  }
  for (int i = 0; i < index3FixedPoints.size(); i++) {
    origPointIndex2MovableIndexMap(index3FixedPoints(i)) = -i - 2;
  }

  assert(index3FixedPoints.size() >= 3);

  Eigen::Matrix3Xd movablePoints = recoverPoints(movablePointsIndexes, _calibWorldPoints);
  Eigen::Matrix3Xd fixedPoints = recoverPoints(index3FixedPoints, _calibWorldPoints);

  // compute the "special fixed points", i.e., the third one will be added as a 2d point laying on a plane
  Eigen::Vector2d p3_xy;
  Eigen::Isometry3d T_p012;
  {
    Eigen::Vector3d x_axis = fixedPoints.col(1) - fixedPoints.col(0);
    x_axis.normalize();
    Eigen::Vector3d y1_axis = fixedPoints.col(2) - fixedPoints.col(0);
    y1_axis.normalize();
    Eigen::Vector3d z_axis = x_axis.cross(y1_axis);
    z_axis.normalize();
    Eigen::Vector3d y_axis = z_axis.cross(x_axis);

    T_p012 = Eigen::Isometry3d::Identity();
    T_p012.translation() = fixedPoints.col(0);
    T_p012.linear().col(0) = x_axis;
    T_p012.linear().col(1) = y_axis;
    T_p012.linear().col(2) = z_axis;

    Eigen::Vector3d p3_xyz = T_p012.inverse() * fixedPoints.col(2);
    p3_xy = p3_xyz.head<2>();
  }

  CameraCalibSection section =
      CameraCalibSection(camera1, camera2, this->T_W_wrt_view1(), this->collect_T_W_wrt_view2(), movablePoints,
                         fixedPoints.leftCols(2), p3_xy, T_p012, std::vector<Eigen::Matrix3Xd>());

  // add repr error of first camera
  for (int i = 0; i < this->view1CalibPoints().imgPoints.cols(); i++) {
    int pointId = origPointIndex2MovableIndexMap(this->view1CalibPoints().indexes(i));
    if (pointId >= 0) {
      section.addEdge(
          {
              section.camera1ParId(),
              section.camera1PoseId(),
              section.pointId(pointId),
          },
          new Camera3dPointReprojectionError(this->view1CalibPoints().imgPoints.col(i)));
    } else if (pointId <= -2) {
      pointId = -pointId - 2;
      csNelson::NodeId nodeId;
      if (pointId == 2) {
        // is not fixed, is the xy point
        nodeId = section.pointXYId();
      } else {
        nodeId = csNelson::NodeId(pointId, csNelson::NodeType::Fixed);
      }

      section.addEdge({section.camera1ParId(), section.camera1PoseId(), nodeId},
                      new Camera3dPointReprojectionError(this->view1CalibPoints().imgPoints.col(i)));
    }
  }

  // add repr error of other views
  if (section.numView2Poses() > 0) {
    for (int i = 0; i < this->numViews(); i++) {
      if (this->T_W_wrt_view2(i).first) {
        for (int j = 0; j < this->view2CalibPoints(i).imgPoints.cols(); j++) {
          int pointId = origPointIndex2MovableIndexMap(this->view2CalibPoints(i).indexes(j));
          if (pointId >= 0) {
            section.addEdge(
                {
                    section.camera2ParId(),
                    section.camera2PoseId(i),
                    section.pointId(pointId),
                },
                new Camera3dPointReprojectionError(this->view2CalibPoints(i).imgPoints.col(j)));
          } else if (pointId <= -2) {
            pointId = -pointId - 2;
            csNelson::NodeId nodeId;
            if (pointId == 2) {
              // is not fixed, is the xy point
              nodeId = section.pointXYId();
            } else {
              nodeId = csNelson::NodeId(pointId, csNelson::NodeType::Fixed);
            }
            section.addEdge({section.camera2ParId(), section.camera2PoseId(i), nodeId},
                            new Camera3dPointReprojectionError(this->view2CalibPoints(i).imgPoints.col(j)));
          }
        }
      }
    }
  }

  section.permuteAMD();
  section.structureReady();

  section.settings().edgeEvalParallelSettings.setNumThreadsMax();
  section.settings().hessianUpdateParallelSettings.setNumThreadsMax();

  csNelson::LevenbergMarquardt<typename csNelson::SolverTraits<csNelson::solverCholeskySparse>::Solver<
      typename CameraCalibSection::Hessian::Traits, csNelson::choleskyNaturalOrdering>>
      lm;
  // csNelson::LevenbergMarquardt<typename csNelson::SolverTraits<csNelson::solverCholeskyDense>::Solver<typename
  // CameraCalibSection::Hessian::Traits>>  lm;

  lm.settings().epsBVector = 1e-6;
  lm.settings().epsChi2 = 1e-6;
  lm.settings().epsIncSquare = 1e-6;
  lm.settings().maxNumIt = 500;
  lm.settings().minNumIt = 3;

  // lm.settings().maxNumSubIt = 10;
  // gn.settings().absLambda = 100000.;

  auto t0 = std::chrono::steady_clock::now();
  auto tc = lm.solve(section);
  auto t1 = std::chrono::steady_clock::now();

  DEBUGME "--- recomputeCameraParamsPosesPoints2Fixed ---"
      << std::endl
      << "- termination: " << csNelson::LevenbergMarquardtUtils::toString(tc) << std::endl
      << lm.stats().toString() << "TIME " << std::chrono::duration<double>(t1 - t0).count() << std::endl
      << std::endl;

  // update world points
  for (int i = 0; i < movablePointsIndexes.size(); i++) {
    int id = movablePointsIndexes(i);
    _calibWorldPoints.col(id) = section.point(i);
  }
  {
    // the xy point
    int id = index3FixedPoints(2);
    Eigen::Vector2d pxy = section.pointXY();
    Eigen::Vector3d pxyz(pxy.x(), pxy.y(), 0);
    _calibWorldPoints.col(id) = section.pointXYRefFrame() * pxyz;
  }

  // update view1
  _view1CamPar.cameraModel = section.view1Camera();
  _view1CamPar.distModel = section.view1CameraDistModel();
  // update view2
  _view2CamPar = {{section.view2Camera(), section.view2CameraDistModel()}}; // unique
  _T_W_wrt_view1 = section.view1Pose();
  int si = 0;
  if (section.numView2Poses() > 0) {
    for (int i = 0; i < this->numViews(); i++) {
      if (this->T_W_wrt_view2(i).first) {
        this->_T_W_wrt_view2[i].second = section.view2Pose(i);

        Eigen::Matrix3Xd worldPoints = recoverPoints(this->_view2CalibPoints[i].indexes, _calibWorldPoints);

        this->_view2CalibPoints[i].reprojPoints = this->view2CamPar(i).cameraModel.points3D_to_image(
            this->_T_W_wrt_view2[i].second * worldPoints, this->view2CamPar(i).distModel);
        this->_view2CalibPoints[i].p3d_wrt_cam_closest =
            computeClosest3DPoint(this->_T_W_wrt_view2[i].second, worldPoints, _view2CalibPoints[i].viewRaysUnit(_view2CamPar[0]));
      }
      si++;
    }
  }

  // update skier points
  {
    std::cerr << "TODO update skier points" << std::endl;
  }

  // update points repr
  Eigen::Matrix3Xd worldPoints = recoverPoints(this->_view1CalibPoints.indexes, _calibWorldPoints);
  this->_view1CalibPoints.reprojPoints =
      this->_view1CamPar.cameraModel.points3D_to_image(this->_T_W_wrt_view1 * worldPoints, this->_view1CamPar.distModel);
  this->_view1CalibPoints.p3d_wrt_cam_closest =
      computeClosest3DPoint(_T_W_wrt_view1, worldPoints, _view1CalibPoints.viewRaysUnit(_view1CamPar));

  // update poles
  this->identifyPoles();

  // update triangulated points
  this->recomputeImgMatchingTriangulation();
  if (!includeSkierPoints) {
    this->recomputeImgSkierTriangulation();
  } else {
    this->recomputeImgSkierReprojErrors();
    this->updateSkierModelFromPoints();
  }

  return index3FixedPoints;
}

void Problem::recomputeCameraParamsPoses() {

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

  CameraCalibSection section = CameraCalibSection(camera1, camera2, this->T_W_wrt_view1(), this->collect_T_W_wrt_view2(),
                                                  Eigen::Matrix3Xd(), _calibWorldPoints, std::vector<Eigen::Matrix3Xd>());
  // enable this line to optimize only view 1 (debugging...)
  // CameraCalibSection section = CameraCalibSection(camera1, camera2, this->T_W_wrt_view1(), {});

  // add repr error of first camera
  for (int i = 0; i < this->view1CalibPoints().imgPoints.cols(); i++) {
    section.addEdge(
        {
            section.camera1ParId(),
            section.camera1PoseId(),
            csNelson::NodeId(this->view1CalibPoints().indexes(i), csNelson::NodeType::Fixed),
        },
        new Camera3dPointReprojectionError(this->view1CalibPoints().imgPoints.col(i)));
  }

  // add repr error of other views
  if (section.numView2Poses() > 0) {
    for (int i = 0; i < this->numViews(); i++) {
      if (this->T_W_wrt_view2(i).first) {
        for (int j = 0; j < this->view2CalibPoints(i).imgPoints.cols(); j++) {
          section.addEdge(
              {
                  section.camera2ParId(),
                  section.camera2PoseId(i),
                  csNelson::NodeId(this->view2CalibPoints(i).indexes(j), csNelson::NodeType::Fixed),
              },
              new Camera3dPointReprojectionError(this->view2CalibPoints(i).imgPoints.col(j)));
        }
      }
    }
  }

  section.permuteAMD();
  section.structureReady();

  section.settings().edgeEvalParallelSettings.setNumThreadsMax();
  section.settings().hessianUpdateParallelSettings.setNumThreadsMax();

  csNelson::LevenbergMarquardt<typename csNelson::SolverTraits<csNelson::solverCholeskySparse>::Solver<
      typename CameraCalibSection::Hessian::Traits, csNelson::choleskyNaturalOrdering>>
      lm;
  // csNelson::LevenbergMarquardt<typename csNelson::SolverTraits<csNelson::solverCholeskyDense>::Solver<typename
  // CameraCalibSection::Hessian::Traits>>  lm;

  lm.settings().epsBVector = 1e-6;
  lm.settings().epsChi2 = 1e-6;
  lm.settings().epsIncSquare = 1e-6;
  lm.settings().maxNumIt = 100;
  lm.settings().minNumIt = 3;
  // gn.settings().absLambda = 100000.;

  auto t0 = std::chrono::steady_clock::now();
  auto tc = lm.solve(section);
  auto t1 = std::chrono::steady_clock::now();

  DEBUGME "--- recomputeCameraParamsPoses ---" << "- termination: " << csNelson::LevenbergMarquardtUtils::toString(tc) << std::endl
                                               << lm.stats().toString() << "TIME " << std::chrono::duration<double>(t1 - t0).count()
                                               << std::endl
                                               << std::endl;

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

        this->_view2CalibPoints[i].reprojPoints = this->view2CamPar(i).cameraModel.points3D_to_image(
            this->_T_W_wrt_view2[i].second * worldPoints, this->view2CamPar(i).distModel);
        this->_view2CalibPoints[i].p3d_wrt_cam_closest = computeClosest3DPoint(
            this->_T_W_wrt_view2[i].second, worldPoints, _view2CalibPoints[i].viewRaysUnit(this->view2CamPar(i)));
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
  this->recomputeImgMatchingTriangulation();
  this->recomputeImgSkierTriangulation();
}

QImage Problem::imageView1(int i) const {
  QString imgName = QDir(_folderPathView1).absoluteFilePath(QString("%1.jpg").arg(i + this->_s1, 4, 10, QChar('0')));
  // std::cout << "IMG1 " << imgName.toStdString() << std::endl;
  QImage img;
  bool b = img.load(imgName);
  if (b == false)
    img = QImage();
  return img;
}

QImage Problem::imageView2(int i) const {
  QString imgName = QDir(_folderPathView2).absoluteFilePath(QString("%1.jpg").arg(i + this->_s2, 4, 10, QChar('0')));
  // std::cout << "IMG2 " << imgName.toStdString() << std::endl;
  QImage img;
  bool b = img.load(imgName);
  if (b == false)
    img = QImage();
  return img;
}

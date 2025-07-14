#pragma once
#include "ProjectionMatrixEstimate.h"

#include "csCamera/Camera.hpp"
#include "csCamera/CameraDistortionModel.hpp"

#include "SkierModel.h"
#include <Eigen/Dense>

#include <QImage>
#include <QString>

struct CameraDistModel {
  csCamera::Camerad cameraModel;
  csCamera::CameraDistortionModeld distModel;

  Eigen::Vector2d undistImagePoint(const Eigen::Vector2d& imgPoint) const;
};

struct ImgCalibPoints {
  Eigen::Matrix2Xd imgPoints;
  Eigen::ArrayXi indexes;
  Eigen::Matrix2Xd reprojPoints;

  Eigen::Matrix3Xd p3d_wrt_cam_closest;

  Eigen::Matrix2Xd reprojErr() const;
  Eigen::Matrix3Xd viewRaysZ1(const CameraDistModel& camera) const;
  Eigen::Matrix3Xd viewRaysUnit(const CameraDistModel& camera) const;
  void reprojErrMeanMax(double& mean, double& max) const;
  void reprojErr2Vec(std::vector<double>& ex, std::vector<double>& ey) const;
  std::vector<int> indexesVec() const;
};

struct FrameMeasPoints {
  Eigen::Matrix2Xd view1Points;
  Eigen::Matrix2Xd view1Points_repr;
  Eigen::Matrix2Xd view2Points;
  Eigen::Matrix2Xd view2Points_repr;

  Eigen::Matrix3Xd p3d_est;

  void resize(int n) {
    view1Points.setZero(2, n);
    view2Points.setZero(2, n);
    view1Points_repr.setZero(2, n);
    view2Points_repr.setZero(2, n);
    p3d_est.setZero(3, n);
  }
};

class Problem {
  double pole_tol;

  void identifyPoles();

  CameraDistModel _view1CamPar;
  Eigen::Isometry3d _T_W_wrt_view1;

  std::vector<CameraDistModel>
      _view2CamPar; // note:can be unique or one per pose (at init), use _view2CamPar(i) method, that will return proper one
  std::vector<std::pair<bool, Eigen::Isometry3d>> _T_W_wrt_view2;
  std::vector<FrameMeasPoints> _framesMeas;
  std::vector<FrameMeasPoints> _framesSkierPoints;
  std::vector<SkierModel> _framesSkierModel;
  std::vector<int> _validViewsIndexes;

  Eigen::Matrix3Xd _calibWorldPoints;
  Eigen::Matrix3Xd _polesPointPairs;

  ImgCalibPoints _view1CalibPoints;
  std::vector<ImgCalibPoints> _view2CalibPoints;

  QString _folderPathView1, _folderPathView2;
  int _s1, _s2;

public:
  Problem(int nMovingCameras, const Eigen::Matrix3Xd& calibWorldPoints, const QString& f1, int s1, const QString& f2, int s2);
  virtual ~Problem();

  static Eigen::Matrix3Xd recoverPoints(const Eigen::ArrayXi& idx, const Eigen::Matrix3Xd& source);
  static Eigen::Matrix3Xd computeClosest3DPoint(const Eigen::Isometry3d& T_W_wrt_C, const Eigen::Matrix3Xd& wp,
                                                const Eigen::Matrix3Xd& unitView_C);

  void initView1(const Eigen::Matrix2Xd& imgPoints, const Eigen::ArrayXi& worldPointIndexes, Eigen::Vector2i imgSize);
  void initView2(int i, const Eigen::Matrix2Xd& imgPoints, const Eigen::ArrayXi& worldPointIndexes, Eigen::Vector2i imgSize);
  void setImgMatchingPoints(int i, const Eigen::Matrix2Xd& view1_meas, const Eigen::Matrix2Xd& view2_meas);
  void setImgSkierPoints(int i, const Eigen::Matrix2Xd& view1_meas, const Eigen::Matrix2Xd& view2_meas);
  void recomputeImgMatchingTriangulation();
  void recomputeImgSkierTriangulation();
  void recomputeImgSkierReprojErrors();
  void updateSkierModelFromPoints();

  int numViews() const {
    return _T_W_wrt_view2.size();
  }

  Eigen::VectorXi indexesCalibrationPointsVisibleMultipleTimes(int minN) const;
  Eigen::VectorXi indexesCalibrationPointsVisibleFromBoth() const;
  Eigen::VectorXi indexCalibrationPointsNotSelected(const Eigen::VectorXi& sel) const;
  Eigen::Matrix<bool, Eigen::Dynamic, 1> selectedVectorToBoolMask(const Eigen::VectorXi& sel) const;

  // return oldCalibId2NewCalibId
  Eigen::VectorXi applyMask2CalibPoints(const Eigen::VectorXi& sel);

  int numValidViews() const {
    return int(_validViewsIndexes.size());
  }

  const CameraDistModel& view1CamPar() const {
    return _view1CamPar;
  }

  Eigen::Isometry3d T_W_wrt_view1() const {
    return _T_W_wrt_view1;
  }

  const CameraDistModel& view2CamPar(int i) const {
    if (uniqueCamParamsView2()) {
      return _view2CamPar[0];
    } else
      return _view2CamPar[i];
  }

  Eigen::Vector2d view2fxfyMedian() const;
  Eigen::Vector2d view2cxcyMedian() const;

  bool uniqueCamParamsView2() const {
    return _view2CamPar.size() == 1;
  }

  // first: valid, second: isometry
  std::pair<bool, Eigen::Isometry3d> T_W_wrt_view2(int i) const {
    return _T_W_wrt_view2[i];
  }

  Eigen::Matrix3Xd calibWorldPoints() const {
    return _calibWorldPoints;
  }
  Eigen::Matrix3Xd view1CalibPoints3D() const {
    return Problem::recoverPoints(view1CalibPoints().indexes, calibWorldPoints());
  }
  Eigen::Matrix3Xd view2CalibPoints3D(int n) const {
    return Problem::recoverPoints(view2CalibPoints(n).indexes, calibWorldPoints());
  }

  Eigen::Matrix3Xd polesPointPairs() const {
    return _polesPointPairs;
  }

  const ImgCalibPoints& view1CalibPoints() const {
    return _view1CalibPoints;
  }

  const ImgCalibPoints& view2CalibPoints(int i) const {
    return _view2CalibPoints[i];
  }

  const std::vector<int>& validViewsIndexes() const {
    return _validViewsIndexes;
  }

  const FrameMeasPoints& framesMeas(int k) const {
    return _framesMeas[k];
  }

  const FrameMeasPoints& framesSkierMeas(int k) const {
    return _framesSkierPoints[k];
  }

  QImage imageView1(int i) const;
  QImage imageView2(int i) const;

  const SkierModel& skierModel(int k) const {
    return _framesSkierModel[k];
  }

  std::vector<double> collectView2Cx() const;
  std::vector<double> collectView2Cy() const;
  std::vector<double> collectView2Fx() const;
  std::vector<double> collectView2Fy() const;

  std::vector<Eigen::Isometry3d> collect_T_W_wrt_view2() const;

  void recomputeCameraParamsPoses();

  // return the three fixed points new indexes
  Eigen::Vector3i recomputeCameraParamsPosesPoints2Fixed(int minViews, Eigen::Vector3i index3FixedPoints,
                                                         bool includeSkierPoints); // 1: visible from both, >=2 min num of views
  Eigen::Vector3i recomputeCameraParamsPosesPoints3Fixed(int minViews, Eigen::Vector3i index3FixedPoints,
                                                         bool includeSkierPoints); // 1: visible from both, >=2 min num of views
};

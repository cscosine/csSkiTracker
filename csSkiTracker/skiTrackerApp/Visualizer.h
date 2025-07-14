#pragma once
#include "csVisOpenGL/Visualizer.h"
#include "csVisOpenGL/UniformPointRenderer.h"
#include "csVisOpenGL/UniformLineRenderer.h"
#include "csVisOpenGL/PainterAxes.h"
#include "csVisOpenGL/PainterGrid.h"
#include "csVisOpenGL/BackgroundRenderer.h"
#include "csVisOpenGL/PainterCameras.h"

#include <Eigen/Geometry>

class Visualizer : public csVisOpenGL::Visualizer
{
  csVisOpenGL::UniformPointRenderer worldPointsRenderer;
  csVisOpenGL::UniformLineRenderer polesLineRenderer;
  csVisOpenGL::PainterCameras camerasMoving, cameraFixed, cameraMoving;
  csVisOpenGL::PainterAxes camerasMovingAxes, cameraFixedAxis, cameraMovingAxis;

  csVisOpenGL::UniformPointRenderer fixCameraWorldPointsRenderer;
  csVisOpenGL::UniformLineRenderer  fixCameraWorldLinesRenderer, fixCameraWorldErrRenderer;

  csVisOpenGL::UniformPointRenderer movCameraWorldPointsRenderer;
  csVisOpenGL::UniformLineRenderer  movCameraWorldLinesRenderer, movCameraWorldErrRenderer;

  csVisOpenGL::UniformPointRenderer reconstructedPointsRenderer;

  csVisOpenGL::PainterAxes axes;
  csVisOpenGL::PainterGrid grid;

  csVisOpenGL::BackgroundRenderer bkgRenderer;

  Eigen::Isometry3f _T_ski_wrt_vis;

  QSize _widgetSize;

  QImage _fixImg, _movImg;
  float _fixImgScale, _movImgScale;

  Eigen::Matrix2Xf fixImgMeasPoints, fixImgReprojPoints;
  Eigen::ArrayXi fixIndexes;

  Eigen::Matrix2Xf movImgMeasPoints, movImgReprojPoints;
  Eigen::ArrayXi movIndexes;

  Eigen::Matrix3Xf worldPoints;
  Eigen::ArrayXi   worldPointsIdxs;

  Eigen::Matrix2Xf corrImgMeasPointsV1, corrImgReprPointsV1;
  Eigen::Matrix2Xf corrImgMeasPointsV2, corrImgReprPointsV2;
  Eigen::ArrayXi corrImgMeasIndexes;

  float computeImgScale(const QImage& source, QImage& dest);

public:
  // visualization flags
  bool showLabels3D, showLabelsMov, showLabelsFix, showLabelsCorr;
  bool showMeas3D, showMeasMov, showMeasFix, showMeasCorr;
  bool showReprMov, showReprFix, showReprCorr;
  bool showErr3D, showErrFix, showErrMov, showErrCorr;
  bool showViewRaysFix, showViewRaysMov;
  bool show3DCalibPoints, show3DCalibPoles;
  bool showCorr3D;

public:
  Visualizer();
  virtual ~Visualizer();

  void initialize(csVisOpenGL::ShaderFactory* shaderFactory) override;
  void paintBackground(const csVisOpenGL::Camera& camera) override;
  void paint(const csVisOpenGL::Camera& camera) override;
  void paintQt(const csVisOpenGL::Camera& camera, QPainter& painter);

  void setWorldPoints(const Eigen::Matrix3Xf& wp, const Eigen::Matrix3Xf& polesLines, const Eigen::ArrayXi& idxs);

  void setFixCameraWorldPoints(const Eigen::Isometry3f& T_C_wrt_W, const Eigen::Matrix3Xf& camPoints, const Eigen::Matrix3Xf& worldPoints);
  void setMovCameraWorldPoints(const Eigen::Isometry3f& T_C_wrt_W, const Eigen::Matrix3Xf& camPoints, const Eigen::Matrix3Xf& worldPoints);
  void setReconstructedPoints(const Eigen::Matrix3Xf& points);
  void setCameraPoses(const Eigen::Isometry3f& fixCam, const std::vector<Eigen::Isometry3f>& T_C_wrt_W);

  void setFixCameraImgPoints(const Eigen::Matrix2Xf& measPoints, const Eigen::Matrix2Xf& reprojPoints, const Eigen::ArrayXi& idxs);
  void setMovCameraImgPoints(const Eigen::Matrix2Xf& measPoints, const Eigen::Matrix2Xf& reprojPoints, const Eigen::ArrayXi& idxs);

  void setFixCameraImg(const QImage& img);
  void setMovCameraImg(const QImage& img);
  void setMovCameraPose(const Eigen::Isometry3f& T_C_wrt_W);

  void setCorrespondences(const Eigen::Matrix2Xf& measV1, const Eigen::Matrix2Xf& reprV1, const Eigen::Matrix2Xf& measV2, const Eigen::Matrix2Xf& reprV2, const Eigen::ArrayXi& ids);

  void setWidgetSize(int w, int h);
};

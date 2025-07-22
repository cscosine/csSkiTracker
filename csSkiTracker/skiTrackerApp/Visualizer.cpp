#include "Visualizer.h"
#include <csVisOpenGL/Colors.hpp>
#include <iostream>

#include <csVisOpenGL/Camera.hpp>
#include <csVisOpenGL/Colors.hpp>

#include "SkierModel.h"

#include <math.h>

Visualizer::Visualizer()
    : csVisOpenGL::VisualizerInterface()
    , _widgetSize(-1, -1)
    , showLabels3D(true)
    , showLabelsMov(true)
    , showLabelsFix(true)
    , showLabelsCorr(true)
    , showMeas3D(true)
    , showMeasMov(true)
    , showMeasFix(true)
    , showMeasCorr(true)
    , showReprMov(true)
    , showReprFix(true)
    , showReprCorr(true)
    , showErr3D(true)
    , showErrFix(true)
    , showErrMov(true)
    , showErrCorr(true)
    , showViewRaysFix(true)
    , showViewRaysMov(true)
    , show3DCalibPoints(true)
    , show3DCalibPoles(true)
    , showCorr3D(true) {}
Visualizer::~Visualizer() {}

void Visualizer::initialize(csVisOpenGL::ShaderFactory* shaderFactory,
                            std::shared_ptr<QOpenGLExtraFunctions> const& glExtraFunctions) {

  // swap y and z
  _T_ski_wrt_vis.setIdentity();
  _T_ski_wrt_vis.linear().col(0) = Eigen::Vector3f(1, 0, 0);
  _T_ski_wrt_vis.linear().col(1) = Eigen::Vector3f(0, 0, 1);
  _T_ski_wrt_vis.linear().col(2) = Eigen::Vector3f(0, -1, 0);

  bkgRenderer.initialize(shaderFactory, glExtraFunctions);

  worldPointsRenderer.initialize(shaderFactory, glExtraFunctions);
  worldPointsRenderer.setPointSize(3);
  worldPointsRenderer.setSmoothPoints(true);
  worldPointsRenderer.setUniformColor(csVisOpenGL::Color::deepOrange);
  worldPointsRenderer.setPoints(Eigen::Matrix3Xf());

  fixCameraWorldPointsRenderer.initialize(shaderFactory, glExtraFunctions);
  fixCameraWorldPointsRenderer.setPointSize(3);
  fixCameraWorldPointsRenderer.setSmoothPoints(true);
  fixCameraWorldPointsRenderer.setUniformColor(csVisOpenGL::ColorLight::red);
  fixCameraWorldPointsRenderer.setPoints(Eigen::Matrix3Xf());

  fixCameraWorldLinesRenderer.initialize(shaderFactory, glExtraFunctions);
  fixCameraWorldLinesRenderer.setLineWidth(2);
  fixCameraWorldLinesRenderer.setUniformColor(csVisOpenGL::ColorDark::lime);
  fixCameraWorldLinesRenderer.setLines(Eigen::Matrix3Xf());

  fixCameraWorldErrRenderer.initialize(shaderFactory, glExtraFunctions);
  fixCameraWorldErrRenderer.setLineWidth(4);
  fixCameraWorldErrRenderer.setUniformColor(csVisOpenGL::ColorDark::red);
  fixCameraWorldErrRenderer.setLines(Eigen::Matrix3Xf());

  movCameraWorldPointsRenderer.initialize(shaderFactory, glExtraFunctions);
  movCameraWorldPointsRenderer.setPointSize(3);
  movCameraWorldPointsRenderer.setSmoothPoints(true);
  movCameraWorldPointsRenderer.setUniformColor(csVisOpenGL::ColorLight::red);
  movCameraWorldPointsRenderer.setPoints(Eigen::Matrix3Xf());

  movCameraWorldLinesRenderer.initialize(shaderFactory, glExtraFunctions);
  movCameraWorldLinesRenderer.setLineWidth(2);
  movCameraWorldLinesRenderer.setUniformColor(csVisOpenGL::ColorDark::lime);
  movCameraWorldLinesRenderer.setLines(Eigen::Matrix3Xf());

  movCameraWorldErrRenderer.initialize(shaderFactory, glExtraFunctions);
  movCameraWorldErrRenderer.setLineWidth(4);
  movCameraWorldErrRenderer.setUniformColor(csVisOpenGL::ColorDark::red);
  movCameraWorldErrRenderer.setLines(Eigen::Matrix3Xf());

  reconstructedPointsRenderer.initialize(shaderFactory, glExtraFunctions);
  reconstructedPointsRenderer.setPointSize(3);
  reconstructedPointsRenderer.setSmoothPoints(true);
  reconstructedPointsRenderer.setUniformColor(csVisOpenGL::Color::blue);
  reconstructedPointsRenderer.setPoints(Eigen::Matrix3Xf());

  skierModelRenderer.initialize(shaderFactory, glExtraFunctions);
  skierModelRenderer.setLineWidth(3);
  skierModelRenderer.setUniformColor(csVisOpenGL::Color::white);
  skierModelRenderer.setLines(Eigen::Matrix3Xf(), Eigen::Matrix3Xf());

  skierPoseRenderer.initialize(shaderFactory, glExtraFunctions);
  skierPoseRenderer.setLineWidth(3);

  skierHeadRenderer.initialize(shaderFactory, glExtraFunctions);
  skierHeadRenderer.setUniformColor(csVisOpenGL::Color::pink);

  polesLineRenderer.initialize(shaderFactory, glExtraFunctions);
  polesLineRenderer.setLineWidth(2);
  polesLineRenderer.setUniformColor(csVisOpenGL::Color::orange);
  polesLineRenderer.setLines(Eigen::Matrix3Xf());

  axes.initialize(shaderFactory, glExtraFunctions);
  axes.setPose(Eigen::Isometry3f::Identity(), 1.0f);
  grid.initialize(shaderFactory, glExtraFunctions);

  cameraFixed.initialize(shaderFactory, glExtraFunctions);
  cameraFixed.setUniformColor(csVisOpenGL::Color::red);

  cameraMoving.initialize(shaderFactory, glExtraFunctions);
  cameraMoving.setUniformColor(csVisOpenGL::Color::blue);

  camerasMoving.initialize(shaderFactory, glExtraFunctions);
  camerasMoving.setUniformColor(csVisOpenGL::Color::green);

  camerasMovingAxes.initialize(shaderFactory, glExtraFunctions);
  cameraFixedAxis.initialize(shaderFactory, glExtraFunctions);
  cameraMovingAxis.initialize(shaderFactory, glExtraFunctions);

  _fixImgScale = _movImgScale = -1;
}

float Visualizer::computeImgScale(const QImage& source, QImage& dest) {
  assert(_widgetSize.height() > 0);
  assert(_widgetSize.width() > 0);

  int max_height = _widgetSize.height() / 3;
  int max_width = _widgetSize.width() / 3;

  if (source.width() > max_width || source.height() > max_height) {
    float scaleW = float(source.width()) / float(max_width);
    float scaleH = float(source.height()) / float(max_height);
    if (scaleW > scaleH) {
      dest = source.scaledToWidth(max_width);
      return 1.0 / scaleW;
    } else {
      dest = source.scaledToHeight(max_height);
      return 1.0 / scaleH;
    }
  } else {
    dest = source;
    return 1.0;
  }
}

class PainterSaveRestore {
  QPainter& painter;

public:
  PainterSaveRestore(PainterSaveRestore& p)
      : painter(p()) {
    painter.save();
  }

  PainterSaveRestore(QPainter& p)
      : painter(p) {
    painter.save();
  }
  ~PainterSaveRestore() {
    painter.restore();
  }

  QPainter& operator()() {
    return painter;
  }
};

void drawCross(const Eigen::Vector2f& point, PainterSaveRestore painter, const QColor& color, float scale, int offset, int crossSize) {
  auto pen = painter().pen();
  pen.setColor(color);
  painter().setPen(pen);

  painter().drawLine(QPointF(offset + scale * point.x() - crossSize, scale * point.y()),
                     QPointF(offset + scale * point.x() + crossSize, scale * point.y()));
  painter().drawLine(QPointF(offset + scale * point.x(), scale * point.y() - crossSize),
                     QPointF(offset + scale * point.x(), scale * point.y() + crossSize));
}

void drawLine(const Eigen::Vector2f& p1, const Eigen::Vector2f& p2, PainterSaveRestore painter, const QColor& color, float scale,
              int offset) {
  auto pen = painter().pen();
  pen.setColor(color);
  pen.setWidth(3);
  painter().setPen(pen);

  painter().drawLine(QPointF(offset + scale * p1.x(), scale * p1.y()), QPointF(offset + scale * p2.x(), scale * p2.y()));
}

void drawCorrespondances(const Eigen::Matrix2Xf& meas, const Eigen::Matrix2Xf& reproj, PainterSaveRestore painter, float scale,
                         int offset, int crossSize, bool flagMeas, bool flagRepr, bool flagErr) {

  if (flagMeas && meas.cols() > 0) {
    for (int i = 0; i < meas.cols(); i++) {
      drawCross(meas.col(i), painter, Qt::blue, scale, offset, crossSize);
    }
  }
  if (flagRepr && reproj.cols() > 0) {
    for (int i = 0; i < reproj.cols(); i++) {
      drawCross(reproj.col(i), painter, Qt::green, scale, offset, crossSize);
    }
  }
  if (flagErr && reproj.cols() > 0 && meas.cols() > 0) {
    for (int i = 0; i < meas.cols(); i++) {
      drawLine(reproj.col(i), meas.col(i), painter, Qt::red, scale, offset);
    }
  }
}

void drawText(const Eigen::Matrix2Xf& meas, const Eigen::ArrayXi& idx, PainterSaveRestore painter, float scale, int offset,
              Eigen::Vector2i d) {
  auto pen = painter().pen();
  pen.setColor(Qt::black);
  painter().setPen(pen);

  for (int i = 0; i < meas.cols(); i++) {
    painter().drawText(QPoint(offset + scale * meas.col(i).x() + d.x(), scale * meas.col(i).y() + d.y()), QString("%1").arg(idx(i)));
  }
}

void Visualizer::paintQt(const csVisOpenGL::Camera& camera, QPainter& painter) {
  // world points
  if (showLabels3D) {
    auto pen = painter.pen();
    pen.setColor(Qt::yellow);
    painter.setPen(pen);
    for (int i = 0; i < worldPoints.cols(); i++) {
      auto sp = camera.worldToScreen(worldPoints.col(i), true);
      painter.drawText(QPoint(sp.x(), sp.y()), QString("%1").arg(worldPointsIdxs(i)));
    }
  }

  // reconstructed points
  if (showLabels3D) {
    auto pen = painter.pen();
    pen.setColor(Qt::yellow);
    painter.setPen(pen);
    for (int i = 0; i < reconstructedPoints.cols(); i++) {
      auto sp = camera.worldToScreen(reconstructedPoints.col(i), true);
      painter.drawText(QPoint(sp.x(), sp.y()), QString("%1").arg(i));
    }
  }

  // skier points
  if (showLabels3D) {
    auto pen = painter.pen();
    pen.setColor(Qt::yellow);
    painter.setPen(pen);
    for (int i = 0; i < this->skierPoints.cols(); i++) {
      auto sp = camera.worldToScreen(this->skierPoints.col(i), true);
      painter.drawText(QPoint(sp.x(), sp.y()), QString("%1").arg(i));
    }
  }

  // images
  int movImgOffset = _widgetSize.width() - _movImg.width(); // -1;
  if (!_fixImg.isNull()) {
    painter.drawImage(QPoint(0, 0), _fixImg);
  }
  if (!_movImg.isNull()) {
    painter.drawImage(QPoint(movImgOffset, 0), _movImg);
  }

  static const int crossSize = 5;
  drawCorrespondances(fixImgMeasPoints, fixImgReprojPoints, painter, _fixImgScale, 0, crossSize, showMeasFix, showReprFix, showErrFix);
  if (showLabelsFix)
    drawText(fixImgMeasPoints, fixIndexes, painter, _fixImgScale, 0, Eigen::Vector2i(crossSize, crossSize));

  drawCorrespondances(movImgMeasPoints, movImgReprojPoints, painter, _movImgScale, movImgOffset, crossSize, showMeasMov, showReprMov,
                      showErrMov);
  if (showLabelsMov)
    drawText(movImgMeasPoints, movIndexes, painter, _movImgScale, movImgOffset, Eigen::Vector2i(crossSize, crossSize));

  //------------------------------------------
  // matching
  drawCorrespondances(corrImgMeasPointsV1, corrImgReprPointsV1, painter, _fixImgScale, 0, crossSize, showMeasCorr, showReprCorr,
                      showErrCorr);
  drawCorrespondances(corrImgMeasPointsV2, corrImgReprPointsV2, painter, _movImgScale, movImgOffset, crossSize, showMeasCorr,
                      showReprCorr, showErrCorr);
  if (showLabelsCorr) {
    Eigen::VectorXi indexes = Eigen::VectorXi::LinSpaced(corrImgMeasPointsV1.cols(), 0, corrImgMeasPointsV1.cols() - 1);
    drawText(corrImgMeasPointsV1, indexes, painter, _fixImgScale, 0, Eigen::Vector2i(crossSize, crossSize));
    drawText(corrImgMeasPointsV2, indexes, painter, _movImgScale, movImgOffset, Eigen::Vector2i(crossSize, crossSize));
  }

  //------------------------------------------
  // skier
  drawCorrespondances(skierImgMeasPointsV1, skierImgReprPointsV1, painter, _fixImgScale, 0, crossSize, showMeasCorr, showReprCorr,
                      showErrCorr);
  drawCorrespondances(skierImgMeasPointsV2, skierImgReprPointsV2, painter, _movImgScale, movImgOffset, crossSize, showMeasCorr,
                      showReprCorr, showErrCorr);
  if (showLabelsCorr) {
    Eigen::VectorXi indexes = Eigen::VectorXi::LinSpaced(skierImgMeasPointsV1.cols(), 0, skierImgMeasPointsV1.cols() - 1);
    drawText(skierImgMeasPointsV1, indexes, painter, _fixImgScale, 0, Eigen::Vector2i(crossSize, crossSize));
    drawText(skierImgMeasPointsV2, indexes, painter, _movImgScale, movImgOffset, Eigen::Vector2i(crossSize, crossSize));
  }
}

void Visualizer::setFixCameraImg(const QImage& img) {

  _fixImgScale = computeImgScale(img, _fixImg);
}

void Visualizer::setMovCameraImg(const QImage& img) {
  _movImgScale = computeImgScale(img, _movImg);
}

void Visualizer::setMovCameraImgPoints(const Eigen::Matrix2Xf& measPoints, const Eigen::Matrix2Xf& reprojPoints,
                                       const Eigen::ArrayXi& idxs) {
  this->movImgMeasPoints = measPoints;
  this->movImgReprojPoints = reprojPoints;
  this->movIndexes = idxs;
}

void Visualizer::setReconstructedPoints(const Eigen::Matrix3Xf& points) {
  // store to show ids
  this->reconstructedPoints = _T_ski_wrt_vis * points;

  this->reconstructedPointsRenderer.setPoints(this->reconstructedPoints);
}

void Visualizer::setSkierModel(const SkierModel& model) {
  this->skierPoints = _T_ski_wrt_vis * model.bodyPoints().cast<float>();

  const int nLinesSide = 10;
  const int nLinesConnections = 2;
  const int nLinesHead = 2;

  const int nPoints = 2 * (2 * nLinesSide + nLinesConnections + nLinesHead);

  Eigen::Matrix3Xf lines(3, nPoints), colors(3, nPoints);
  int count = 0;
  int cstart = 0;
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightFootHead).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightFootRearBottom).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightFootRearBottom).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightAnkle).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightAnkle).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightFootHead).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::red.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightAnkle).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightKnee).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightKnee).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightHip).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::amber.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightHip).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightShoulder).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::yellow.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightShoulder).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightElbow).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightElbow).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightHand).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::cyan.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightHand).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightPoleTip).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::green.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightSkiHead).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightSkiTail).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::orange.head<3>();
  cstart = count;

  //-------------------------------------------------------------
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftFootHead).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftFootRearBottom).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftFootRearBottom).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftAnkle).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftAnkle).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftFootHead).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::red.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftAnkle).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftKnee).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftKnee).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftHip).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::amber.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftHip).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftShoulder).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::yellow.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftShoulder).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftElbow).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftElbow).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftHand).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::cyan.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftHand).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftPoleTip).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::green.head<3>();
  cstart = count;

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftSkiHead).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftSkiTail).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::orange.head<3>();
  cstart = count;

  //-------------------------------------------------------------
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightHip).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftHip).cast<float>();

  lines.col(count++) = model.bodyPoint(SkierModel::Labels::RightShoulder).cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::LeftShoulder).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::yellow.head<3>();
  cstart = count;

  //-------------------------------------------------------------
  lines.col(count++) = model.shouldersMiddlePoints().cast<float>();
  lines.col(count++) = model.bodyPoint(SkierModel::Labels::Head).cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::pink.head<3>();
  cstart = count;

  lines.col(count++) = model.shouldersMiddlePoints().cast<float>();
  lines.col(count++) = model.hipsMiddlePoints().cast<float>();

  colors.middleCols(cstart, count - cstart).colwise() = csVisOpenGL::Color::yellow.head<3>();
  cstart = count;

  assert(count == nPoints);

  this->skierModelRenderer.setLines(_T_ski_wrt_vis * lines, colors);
  this->skierPoseRenderer.setPose(_T_ski_wrt_vis * model.pose().cast<float>(), 0.3);

  Eigen::Isometry3f poseHead = Eigen::Isometry3f::Identity();
  poseHead.translation() = _T_ski_wrt_vis * model.bodyPoint(SkierModel::Labels::Head).cast<float>();
  this->skierHeadRenderer.setCovariances(poseHead, Eigen::Matrix3d::Identity() * 0.1 * 0.1, 1.0);
}

void Visualizer::setCorrespondences(const Eigen::Matrix2Xf& measV1, const Eigen::Matrix2Xf& reprV1, const Eigen::Matrix2Xf& measV2,
                                    const Eigen::Matrix2Xf& reprV2) {
  this->corrImgMeasPointsV1 = measV1;
  this->corrImgReprPointsV1 = reprV1;

  this->corrImgMeasPointsV2 = measV2;
  this->corrImgReprPointsV2 = reprV2;
}

void Visualizer::setSkierCorrespondences(const Eigen::Matrix2Xf& measV1, const Eigen::Matrix2Xf& reprV1,
                                         const Eigen::Matrix2Xf& measV2, const Eigen::Matrix2Xf& reprV2) {
  this->skierImgMeasPointsV1 = measV1;
  this->skierImgReprPointsV1 = reprV1;

  this->skierImgMeasPointsV2 = measV2;
  this->skierImgReprPointsV2 = reprV2;
}

void Visualizer::setFixCameraImgPoints(const Eigen::Matrix2Xf& measPoints, const Eigen::Matrix2Xf& reprojPoints,
                                       const Eigen::ArrayXi& idxs) {
  this->fixImgMeasPoints = measPoints;
  this->fixImgReprojPoints = reprojPoints;
  this->fixIndexes = idxs;
}

void Visualizer::setFixCameraWorldPoints(const Eigen::Isometry3f& T_C_wrt_W, const Eigen::Matrix3Xf& camPoints,
                                         const Eigen::Matrix3Xf& worldPoints) {
  Eigen::Matrix3Xf destPoints = T_C_wrt_W * camPoints;
  Eigen::Matrix3Xf destLines(3, 2 * destPoints.cols());
  Eigen::Matrix3Xf errLines(3, 2 * destPoints.cols());
  for (int i = 0; i < destPoints.cols(); i++) {
    destLines.col(2 * i) = T_C_wrt_W.translation();
    destLines.col(2 * i + 1) = destPoints.col(i);

    errLines.col(2 * i) = destPoints.col(i);
    errLines.col(2 * i + 1) = worldPoints.col(i);
  }

  this->fixCameraWorldPointsRenderer.setPoints(_T_ski_wrt_vis * destPoints);
  this->fixCameraWorldLinesRenderer.setLines(_T_ski_wrt_vis * destLines);
  this->fixCameraWorldErrRenderer.setLines(_T_ski_wrt_vis * errLines);
}

void Visualizer::setMovCameraWorldPoints(const Eigen::Isometry3f& T_C_wrt_W, const Eigen::Matrix3Xf& camPoints,
                                         const Eigen::Matrix3Xf& worldPoints) {
  Eigen::Matrix3Xf destPoints = T_C_wrt_W * camPoints;
  Eigen::Matrix3Xf destLines(3, 2 * destPoints.cols());
  Eigen::Matrix3Xf errLines(3, 2 * destPoints.cols());
  for (int i = 0; i < destPoints.cols(); i++) {
    destLines.col(2 * i) = T_C_wrt_W.translation();
    destLines.col(2 * i + 1) = destPoints.col(i);

    errLines.col(2 * i) = destPoints.col(i);
    errLines.col(2 * i + 1) = worldPoints.col(i);
  }

  this->movCameraWorldPointsRenderer.setPoints(_T_ski_wrt_vis * destPoints);
  this->movCameraWorldLinesRenderer.setLines(_T_ski_wrt_vis * destLines);
  this->movCameraWorldErrRenderer.setLines(_T_ski_wrt_vis * errLines);
}

void Visualizer::paintBackground(const csVisOpenGL::Camera& camera) {
  bkgRenderer.draw(camera);
}

void Visualizer::paint(const csVisOpenGL::Camera& camera) {

  if (show3DCalibPoints)
    worldPointsRenderer.draw(camera);
  if (show3DCalibPoles)
    polesLineRenderer.draw(camera);
  axes.draw(camera);
  grid.draw(camera);

  if (showMeas3D)
    fixCameraWorldPointsRenderer.draw(camera);
  if (showViewRaysFix)
    fixCameraWorldLinesRenderer.draw(camera);
  if (showErr3D)
    fixCameraWorldErrRenderer.draw(camera);

  if (showMeas3D)
    movCameraWorldPointsRenderer.draw(camera);
  if (showViewRaysMov)
    movCameraWorldLinesRenderer.draw(camera);
  if (showErr3D)
    movCameraWorldErrRenderer.draw(camera);
  if (showCorr3D)
    reconstructedPointsRenderer.draw(camera);
  skierModelRenderer.draw(camera);
  skierPoseRenderer.draw(camera);
  skierHeadRenderer.draw(camera);

  camerasMoving.draw(camera);
  camerasMovingAxes.draw(camera);
  cameraFixed.draw(camera);
  cameraFixedAxis.draw(camera);

  cameraMoving.draw(camera);
  cameraMovingAxis.draw(camera);
}

void Visualizer::setWorldPoints(const Eigen::Matrix3Xf& wp, const Eigen::Matrix3Xf& polesLines, const Eigen::ArrayXi& idxs) {
  this->worldPoints = _T_ski_wrt_vis * wp;
  worldPointsRenderer.setPoints(this->worldPoints);
  polesLineRenderer.setLines(_T_ski_wrt_vis * polesLines);

  // store to show ids
  this->worldPointsIdxs = idxs;
}

void Visualizer::setMovCameraPose(const Eigen::Isometry3f& T_C_wrt_W) {
  cameraMoving.setPose(_T_ski_wrt_vis * T_C_wrt_W, 2, 1, 3);
  cameraMovingAxis.setPose(_T_ski_wrt_vis * T_C_wrt_W, 3.0);
}

void Visualizer::setCameraPoses(const Eigen::Isometry3f& fixCam, const std::vector<Eigen::Isometry3f>& T_C_wrt_W) {

  std::vector<Eigen::Isometry3f> T_C_wrt_W_trans = T_C_wrt_W;
  for (int i = 0; i < T_C_wrt_W_trans.size(); i++) {
    T_C_wrt_W_trans[i] = _T_ski_wrt_vis * T_C_wrt_W_trans[i];
  }

  cameraFixed.setPose(_T_ski_wrt_vis * fixCam, 2, 1, 3);
  cameraFixedAxis.setPose(_T_ski_wrt_vis * fixCam, 3.0);

  camerasMoving.setPoses(T_C_wrt_W_trans, 0.5, 0.25, 3.0 / 4.0);
  camerasMovingAxes.setPoses(T_C_wrt_W_trans, 3.0 / 4.0);
}

void Visualizer::setWidgetSize(int w, int h) {
  this->_widgetSize.setWidth(w);
  this->_widgetSize.setHeight(h);
}

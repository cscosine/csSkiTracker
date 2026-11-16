#include "Window.h"

#include <QLabel>

#include "ui_Window.h"

#include <csVisOpenGL/OrbitCameraController.hpp>

#include "ProjectionMatrixEstimate.h"

#include "CommandLineOptions.h"
#include "ProblemDescription.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

#include <iostream>

Window::Window(Problem& p, const CommandLineOptions& cmd, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::Window())
    , problem(p)
    , vis(std::make_shared<Visualizer>())
    , m_cameraController(std::make_shared<csVisOpenGL::OrbitCameraController>())
    , _initialized(false) {
  ui->setupUi(this);

  m_cameraController->setRadius(10);
  m_cameraController->setMinRadius(0.001);

  ui->preview->connectCameraControllerSignals(m_cameraController->getCameraControllerSignals());
  ui->preview->connectSlotsInterface(*m_cameraController);

  ui->preview->addVisualizer(vis);

  this->showMaximized();

  vis->setWidgetSize(ui->preview->width(), ui->preview->height());

  vis->setWorldPoints(problem.calibWorldPoints().cast<float>(), problem.polesPointPairs().cast<float>(),
                      Eigen::ArrayXi::LinSpaced(problem.calibWorldPoints().cols(), 0, problem.calibWorldPoints().cols() - 1));

  std::vector<Eigen::Isometry3f> T_c_wrt_W;
  auto c2 = problem.collect_T_W_wrt_view2();
  for (int i = 0; i < c2.size(); i++) {
    T_c_wrt_W.push_back(c2[i].inverse().cast<float>());
  }

  vis->setCameraPoses(problem.T_W_wrt_view1().inverse().cast<float>(), T_c_wrt_W);

  vis->setFixCameraImgPoints(problem.view1CalibPoints().imgPoints.cast<float>(), problem.view1CalibPoints().reprojPoints.cast<float>(),
                             problem.view1CalibPoints().indexes);

  // calib points of fix camera
  vis->setFixCameraWorldPoints(problem.T_W_wrt_view1().inverse().cast<float>(),
                               problem.view1CalibPoints().p3d_wrt_cam_closest.cast<float>(),
                               problem.view1CalibPoints3D().cast<float>());

  ui->horizontalSlider->blockSignals(true);
  ui->horizontalSlider->setMinimum(0);
  ui->horizontalSlider->setValue(0);
  ui->horizontalSlider->setMaximum(problem.numValidViews() - 1);
  ui->horizontalSlider->blockSignals(false);

  ui->spinBoxMinNumViews->setValue(2);

  ui->lineEditFixedPointsIds->setText(cmd.fixedPointsString.c_str());

  ui->checkBoxEstimateSkier->setChecked(true);

  this->on_horizontalSlider_valueChanged(ui->horizontalSlider->value());

  this->on_checkBoxShowLabels3D_clicked();
  this->on_checkBoxShowLabelsFix_clicked();
  this->on_checkBoxShowLabelsMov_clicked();
  this->on_checkBoxShowLabelsCorr_clicked();

  this->on_checkBoxShowMeas3D_clicked();
  this->on_checkBoxShowMeasFix_clicked();
  this->on_checkBoxShowMeasMov_clicked();
  this->on_checkBoxShowMeasCorr_clicked();

  this->on_checkBoxShowReprFix_clicked();
  this->on_checkBoxShowReprMov_clicked();
  this->on_checkBoxShowReprCorr_clicked();

  this->on_checkBoxShowErr3D_clicked();
  this->on_checkBoxShowErrFix_clicked();
  this->on_checkBoxShowErrMov_clicked();
  this->on_checkBoxShowErrCorr_clicked();

  this->on_checkBoxShowViewRaysFix_clicked();
  this->on_checkBoxShowViewRaysMov_clicked();

  this->on_checkBoxShow3DCalibPoints_clicked();
  this->on_checkBoxShow3DPoles_clicked();
  this->on_checkBoxShow3DCorr_clicked();

  _initialized = true;

  ui->pushButtonTest->setVisible(false);
}

Window::~Window() {}

void Window::on_horizontalSlider_valueChanged(int n) {

  {
    QImage fixImg;

    fixImg = problem.imageView1(n);
    if (fixImg.isNull()) {
      fixImg = QImage(problem.view1CamPar().cameraModel.w(), problem.view1CamPar().cameraModel.h(), QImage::Format::Format_RGB888);
      fixImg.fill(Qt::white);
    }

    vis->setFixCameraImg(fixImg);
  }
  {
    QImage movImg;

    movImg = problem.imageView2(n);
    if (movImg.isNull()) {
      movImg = QImage(problem.view1CamPar().cameraModel.w(), problem.view1CamPar().cameraModel.h(), QImage::Format::Format_RGB888);
      movImg.fill(Qt::white);
    }

    vis->setMovCameraImg(movImg);
  }

  vis->setMovCameraImgPoints(problem.view2CalibPoints(n).imgPoints.cast<float>(),
                             problem.view2CalibPoints(n).reprojPoints.cast<float>(), problem.view2CalibPoints(n).indexes);

  // calib points of mov camera
  vis->setMovCameraWorldPoints(problem.T_W_wrt_view2(n).second.inverse().cast<float>(),
                               problem.view2CalibPoints(n).p3d_wrt_cam_closest.cast<float>(),
                               problem.view2CalibPoints3D(n).cast<float>());
  vis->setMovCameraPose(problem.T_W_wrt_view2(n).second.inverse().cast<float>());

  // correspondences
  const auto& f = problem.framesMeas(n);
  vis->setCorrespondences(f.view1Points.cast<float>(), f.view1Points_repr.cast<float>(), f.view2Points.cast<float>(),
                          f.view2Points_repr.cast<float>());

  vis->setReconstructedPoints(f.p3d_est.cast<float>());

  const auto& s = problem.skierModel(n);
  vis->setSkierModel(s);

  const auto& fs = problem.framesSkierMeas(n);
  vis->setSkierCorrespondences(fs.view1Points.cast<float>(), fs.view1Points_repr.cast<float>(), fs.view2Points.cast<float>(),
                               fs.view2Points_repr.cast<float>());

  // autoplay
  QTimer::singleShot(50, this, [this] {
    if (ui->pushButtonPlay->isChecked()) {
      if (ui->horizontalSlider->value() < ui->horizontalSlider->maximum()) {
        ui->horizontalSlider->setValue(ui->horizontalSlider->value() + 1);
      } else {
        ui->horizontalSlider->setValue(0);
      }
    }
  });
}

bool Window::gui2fixedPoints(Eigen::Vector3i& fixedPointsIds, QString& errmsg) const {
  bool ok = true;

  QString sfixed = ui->lineEditFixedPointsIds->text();
  auto ss = sfixed.split(",", Qt::KeepEmptyParts);
  if (ss.size() != 3) {
    errmsg = "Provide three point indexes comma separated (e.g., 0,26,30)";
    ok = false;
  }
  if (ok) {
    for (int i = 0; i < fixedPointsIds.size(); i++) {
      fixedPointsIds(i) = ss[i].toInt(&ok);
      if (!ok) {
        errmsg = QString("Point %1 expected int number, got %2").arg(i + 1).arg(ss[i]);
        break;
      } else {
        if (fixedPointsIds(i) < 0 || fixedPointsIds(i) >= this->problem.calibWorldPoints().cols()) {
          errmsg = QString("Point %2 index invalid").arg(i + 1).arg(ss[i]);
          ok = false;
        }
      }
    }
  }
  if (ok) {
    // check not repeated
    for (int i = 0; i < fixedPointsIds.size() - 1; i++) {
      for (int j = i + 1; j < fixedPointsIds.size(); j++) {
        if (fixedPointsIds(i) == fixedPointsIds(j)) {
          errmsg = QString("Point %1 equal %2").arg(i + 1).arg(j + 1);
          ok = false;
          break;
        }
      }
    }
  }
  return ok;
}

void Window::fixedPoints2gui(const Eigen::Vector3i& newFixedPointsIds) {
  QString newText = QString("%1, %2, %3").arg(newFixedPointsIds(0)).arg(newFixedPointsIds(1)).arg(newFixedPointsIds(2));
  ui->lineEditFixedPointsIds->setText(newText);
}

void Window::on_pushButtonNonLinearRefinePoints3Fix_clicked() {
  // recover fixed points
  Eigen::Vector3i fixedPointsIds;
  QString errmsg;
  bool ok = gui2fixedPoints(fixedPointsIds, errmsg);
  if (!ok) {
    QMessageBox::critical(this, "Error", errmsg);
    return;
  }

  Eigen::Vector3i newFixedPointsIds = problem.recomputeCameraParamsPosesPoints3Fixed(ui->spinBoxMinNumViews->value(), fixedPointsIds,
                                                                                     ui->checkBoxEstimateSkier->isChecked());
  if (!(newFixedPointsIds.array() >= 0).all()) {
    // some point selected is not valid
    QMessageBox::critical(this, "Error", "Some index of points selected is not valid, change it");
  }
  fixedPoints2gui(newFixedPointsIds);

  vis->setWorldPoints(problem.calibWorldPoints().cast<float>(), problem.polesPointPairs().cast<float>(),
                      Eigen::ArrayXi::LinSpaced(problem.calibWorldPoints().cols(), 0, problem.calibWorldPoints().cols() - 1));

  std::vector<Eigen::Isometry3f> T_c_wrt_W;
  auto c2 = problem.collect_T_W_wrt_view2();
  for (int i = 0; i < c2.size(); i++) {
    T_c_wrt_W.push_back(c2[i].inverse().cast<float>());
  }

  vis->setCameraPoses(problem.T_W_wrt_view1().inverse().cast<float>(), T_c_wrt_W);
  vis->setFixCameraImgPoints(problem.view1CalibPoints().imgPoints.cast<float>(), problem.view1CalibPoints().reprojPoints.cast<float>(),
                             problem.view1CalibPoints().indexes);
  vis->setFixCameraWorldPoints(problem.T_W_wrt_view1().inverse().cast<float>(),
                               problem.view1CalibPoints().p3d_wrt_cam_closest.cast<float>(),
                               problem.view1CalibPoints3D().cast<float>());

  on_horizontalSlider_valueChanged(ui->horizontalSlider->value());
}

void Window::on_pushButtonNonLinearRefinePoints2Fix_clicked() {
  // recover fixed points
  Eigen::Vector3i fixedPointsIds;
  QString errmsg;
  bool ok = gui2fixedPoints(fixedPointsIds, errmsg);
  if (!ok) {
    QMessageBox::critical(this, "Error", errmsg);
    return;
  }

  Eigen::Vector3i newFixedPointsIds = problem.recomputeCameraParamsPosesPoints2Fixed(ui->spinBoxMinNumViews->value(), fixedPointsIds,
                                                                                     ui->checkBoxEstimateSkier->isChecked());
  if (!(newFixedPointsIds.array() >= 0).all()) {
    // some point selected is not valid
    QMessageBox::critical(this, "Error", "Some index of points selected is not valid, change it");
  }
  fixedPoints2gui(newFixedPointsIds);

  vis->setWorldPoints(problem.calibWorldPoints().cast<float>(), problem.polesPointPairs().cast<float>(),
                      Eigen::ArrayXi::LinSpaced(problem.calibWorldPoints().cols(), 0, problem.calibWorldPoints().cols() - 1));

  std::vector<Eigen::Isometry3f> T_c_wrt_W;
  auto c2 = problem.collect_T_W_wrt_view2();
  for (int i = 0; i < c2.size(); i++) {
    T_c_wrt_W.push_back(c2[i].inverse().cast<float>());
  }

  vis->setCameraPoses(problem.T_W_wrt_view1().inverse().cast<float>(), T_c_wrt_W);
  vis->setFixCameraImgPoints(problem.view1CalibPoints().imgPoints.cast<float>(), problem.view1CalibPoints().reprojPoints.cast<float>(),
                             problem.view1CalibPoints().indexes);
  vis->setFixCameraWorldPoints(problem.T_W_wrt_view1().inverse().cast<float>(),
                               problem.view1CalibPoints().p3d_wrt_cam_closest.cast<float>(),
                               problem.view1CalibPoints3D().cast<float>());

  on_horizontalSlider_valueChanged(ui->horizontalSlider->value());
}

void Window::on_pushButtonNonLinearRefine_clicked() {
  problem.recomputeCameraParamsPoses();

  std::vector<Eigen::Isometry3f> T_c_wrt_W;
  auto c2 = problem.collect_T_W_wrt_view2();
  for (int i = 0; i < c2.size(); i++) {
    T_c_wrt_W.push_back(c2[i].inverse().cast<float>());
  }

  vis->setCameraPoses(problem.T_W_wrt_view1().inverse().cast<float>(), T_c_wrt_W);
  vis->setFixCameraImgPoints(problem.view1CalibPoints().imgPoints.cast<float>(), problem.view1CalibPoints().reprojPoints.cast<float>(),
                             problem.view1CalibPoints().indexes);
  vis->setFixCameraWorldPoints(problem.T_W_wrt_view1().inverse().cast<float>(),
                               problem.view1CalibPoints().p3d_wrt_cam_closest.cast<float>(),
                               problem.view1CalibPoints3D().cast<float>());

  on_horizontalSlider_valueChanged(ui->horizontalSlider->value());
}

void Window::on_pushButtonSaveSkierPoints_clicked() {
  QString filename = QFileDialog::getSaveFileName(this, "Save Skier Points", "", "*.stxt");
  if (!filename.isEmpty()) {
    QFile fi(filename);
    bool b = fi.open(QIODevice::WriteOnly);
    if (b) {
      QTextStream tt(&fi);
      tt << "# u1 v1 u2 v2 x y z" << Qt::endl;
      for (int i = 0; i < this->problem.numValidViews(); i++) {
        tt << "#FRAME " << i << Qt::endl;
        for (int k = 0; k < this->problem.framesMeas(i).view1Points.cols(); k++) {
          const auto& f = this->problem.framesMeas(i);
          tt << f.view1Points.col(k).x() << " " << f.view1Points.col(k).y() << " " << f.view2Points.col(k).x() << " "
             << f.view2Points.col(k).y() << " " << f.p3d_est.col(k).x() << " " << f.p3d_est.col(k).y() << " " << f.p3d_est.col(k).z()
             << " " << Qt::endl;
        }
      }
    }
  }
}

void Window::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  vis->setWidgetSize(ui->preview->width(), ui->preview->height());

  if (_initialized) {
    this->on_horizontalSlider_valueChanged(ui->horizontalSlider->value());
  }
}

void Window::on_checkBoxShowLabels3D_clicked() {
  vis->showLabels3D = ui->checkBoxShowLabels3D->isChecked();
}
void Window::on_checkBoxShowLabelsFix_clicked() {
  vis->showLabelsFix = ui->checkBoxShowLabelsFix->isChecked();
}
void Window::on_checkBoxShowLabelsMov_clicked() {
  vis->showLabelsMov = ui->checkBoxShowLabelsMov->isChecked();
}
void Window::on_checkBoxShowLabelsCorr_clicked() {
  vis->showLabelsCorr = ui->checkBoxShowLabelsCorr->isChecked();
}

void Window::on_checkBoxShowMeas3D_clicked() {
  vis->showMeas3D = ui->checkBoxShowMeas3D->isChecked();
}
void Window::on_checkBoxShowMeasFix_clicked() {
  vis->showMeasFix = ui->checkBoxShowMeasFix->isChecked();
}
void Window::on_checkBoxShowMeasMov_clicked() {
  vis->showMeasMov = ui->checkBoxShowMeasMov->isChecked();
}
void Window::on_checkBoxShowMeasCorr_clicked() {
  vis->showMeasCorr = ui->checkBoxShowMeasCorr->isChecked();
}

void Window::on_checkBoxShowReprFix_clicked() {
  vis->showReprFix = ui->checkBoxShowReprFix->isChecked();
}
void Window::on_checkBoxShowReprMov_clicked() {
  vis->showReprMov = ui->checkBoxShowReprMov->isChecked();
}
void Window::on_checkBoxShowReprCorr_clicked() {
  vis->showReprCorr = ui->checkBoxShowReprCorr->isChecked();
}

void Window::on_checkBoxShowErr3D_clicked() {
  vis->showErr3D = ui->checkBoxShowErr3D->isChecked();
}
void Window::on_checkBoxShowErrFix_clicked() {
  vis->showErrFix = ui->checkBoxShowErrFix->isChecked();
}
void Window::on_checkBoxShowErrMov_clicked() {
  vis->showErrMov = ui->checkBoxShowErrMov->isChecked();
}
void Window::on_checkBoxShowErrCorr_clicked() {
  vis->showErrCorr = ui->checkBoxShowErrCorr->isChecked();
}

void Window::on_checkBoxShowViewRaysFix_clicked() {
  vis->showViewRaysFix = ui->checkBoxShowViewRaysFix->isChecked();
}
void Window::on_checkBoxShowViewRaysMov_clicked() {
  vis->showViewRaysMov = ui->checkBoxShowViewRaysMov->isChecked();
}

void Window::on_checkBoxShow3DCalibPoints_clicked() {
  vis->show3DCalibPoints = ui->checkBoxShow3DCalibPoints->isChecked();
}
void Window::on_checkBoxShow3DPoles_clicked() {
  vis->show3DCalibPoles = ui->checkBoxShow3DPoles->isChecked();
}
void Window::on_checkBoxShow3DCorr_clicked() {
  vis->showCorr3D = ui->checkBoxShow3DCorr->isChecked();
}

void Window::on_pushButtonPlay_clicked() {
  if (ui->pushButtonPlay->isChecked()) {
    this->on_horizontalSlider_valueChanged(ui->horizontalSlider->value());
  } else {
  }
}

void Window::on_pushButtonTest_clicked() {
  // use me!
  // recover fixed points
  Eigen::Vector3i fixedPointsIds;
  QString errmsg;
  bool ok = gui2fixedPoints(fixedPointsIds, errmsg);
  if (!ok) {
    QMessageBox::critical(this, "Error", errmsg);
    return;
  }

  Eigen::Matrix3Xd points = problem.recoverPoints(fixedPointsIds, problem.calibWorldPoints());
  Eigen::Vector3d x_axis = points.col(1) - points.col(0);
  x_axis.normalize();
  Eigen::Vector3d y1_axis = points.col(2) - points.col(0);
  y1_axis.normalize();
  Eigen::Vector3d z_axis = x_axis.cross(y1_axis);
  z_axis.normalize();
  Eigen::Vector3d y_axis = z_axis.cross(x_axis);

  Eigen::Isometry3d T_p012 = Eigen::Isometry3d::Identity();
  T_p012.translation() = points.col(0);
  T_p012.linear().col(0) = x_axis;
  T_p012.linear().col(1) = y_axis;
  T_p012.linear().col(2) = z_axis;

  std::cout << points << std::endl << std::endl;
  std::cout << T_p012.matrix() << std::endl << std::endl;
  std::cout << T_p012.inverse() * points << std::endl;
}

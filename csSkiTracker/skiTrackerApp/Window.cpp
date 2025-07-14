#include "Window.h"

#include <QLabel>

#include "ui_Window.h"

#include "csVisOpenGL/OrbitCameraController.h"

#include "ProjectionMatrixEstimate.h"

#include "ProblemDescription.h"

#include <QTimer>

Window::Window(Problem& p, QWidget* parent)
  : QMainWindow(parent),
  ui(new Ui::Window()),
  problem(p),
  vis(new Visualizer()),
  _initialized(false)
{
  ui->setupUi(this);

  vis->setWidgetSize(ui->preview->width(), ui->preview->height());

  this->showMaximized();

  ui->preview->addVisualizer(vis.get());
  auto controller = static_cast<csVisOpenGL::OrbitCameraController*>(ui->preview->getCameraController());
  controller->setRadius(10);
  controller->setMinRadius(0.001);

  vis->setWorldPoints(problem.calibWorldPoints().cast<float>(), problem.polesPointPairs().cast<float>(), Eigen::ArrayXi::LinSpaced(problem.calibWorldPoints().cols(), 0, problem.calibWorldPoints().cols() - 1));

  std::vector<Eigen::Isometry3f> T_c_wrt_W;
  auto c2 = problem.collect_T_W_wrt_view2();
  for (int i = 0; i < c2.size(); i++) {
    T_c_wrt_W.push_back(c2[i].inverse().cast<float>());
  }

  vis->setCameraPoses(problem.T_W_wrt_view1().inverse().cast<float>(), T_c_wrt_W);

  ui->horizontalSlider->blockSignals(true);
  ui->horizontalSlider->setMinimum(0);
  ui->horizontalSlider->setValue(0);
  ui->horizontalSlider->setMaximum(problem.numValidViews() - 1);
  ui->horizontalSlider->blockSignals(false);


  vis->setFixCameraImgPoints(problem.view1CalibPoints().imgPoints.cast<float>(), problem.view1CalibPoints().reprojPoints.cast<float>(), problem.view1CalibPoints().indexes);

  // calib points of fix camera
  vis->setFixCameraWorldPoints(problem.T_W_wrt_view1().inverse().cast<float>(), problem.view1CalibPoints().p3d_wrt_cam_closest.cast<float>(), problem.view1CalibPoints3D().cast<float>());

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
}

Window::~Window()
{

}

void Window::on_horizontalSlider_valueChanged(int n) {

  {
    QImage fixImg(problem.view1CamPar().cameraModel.w(), problem.view1CamPar().cameraModel.h(), QImage::Format::Format_RGB888);
    fixImg.fill(Qt::white);

    vis->setFixCameraImg(fixImg);
  }
  {
    QImage movImg(problem.view1CamPar().cameraModel.w(), problem.view1CamPar().cameraModel.h(), QImage::Format::Format_RGB888);
    movImg.fill(Qt::white);

    vis->setMovCameraImg(movImg);
  }

  vis->setMovCameraImgPoints(problem.view2CalibPoints(n).imgPoints.cast<float>(), problem.view2CalibPoints(n).reprojPoints.cast<float>(), problem.view2CalibPoints(n).indexes);


  // calib points of mov camera
  vis->setMovCameraWorldPoints(problem.T_W_wrt_view2(n).second.inverse().cast<float>(), problem.view2CalibPoints(n).p3d_wrt_cam_closest.cast<float>(), problem.view2CalibPoints3D(n).cast<float>());
  vis->setMovCameraPose(problem.T_W_wrt_view2(n).second.inverse().cast<float>());

  // correspondences
  const auto& f = problem.framesMeas(n);
  vis->setCorrespondences(f.view1Points.cast<float>(), f.view1Points_repr.cast<float>(), f.view2Points.cast<float>(), f.view2Points_repr.cast<float>(), f.indexes);
  
  vis->setReconstructedPoints(f.p3d_est.cast<float>());

  // autoplay
  QTimer::singleShot(50, this, [this] {
    if (ui->pushButtonPlay->isChecked()) {
      if (ui->horizontalSlider->value() < ui->horizontalSlider->maximum()) {
        ui->horizontalSlider->setValue(ui->horizontalSlider->value() + 1);
      }
      else {
        ui->horizontalSlider->setValue(0);
      }
    }
    });


}

void Window::on_pushButtonNonLinearRefine_clicked() {
  problem.recomputeCameraParamsPoses();

  std::vector<Eigen::Isometry3f> T_c_wrt_W;
  auto c2 = problem.collect_T_W_wrt_view2();
  for (int i = 0; i < c2.size(); i++) {
    T_c_wrt_W.push_back(c2[i].inverse().cast<float>());
  }

  vis->setCameraPoses(problem.T_W_wrt_view1().inverse().cast<float>(), T_c_wrt_W);
  vis->setFixCameraImgPoints(problem.view1CalibPoints().imgPoints.cast<float>(), problem.view1CalibPoints().reprojPoints.cast<float>(), problem.view1CalibPoints().indexes);
  vis->setFixCameraWorldPoints(problem.T_W_wrt_view1().inverse().cast<float>(), problem.view1CalibPoints().p3d_wrt_cam_closest.cast<float>(), problem.view1CalibPoints3D().cast<float>());

  on_horizontalSlider_valueChanged(ui->horizontalSlider->value());
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
  }
  else {

  }
}
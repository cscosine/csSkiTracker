#pragma once
#include "Visualizer.h"
#include <QMainWindow>
#include "csSkiTracker/dataReader/DataDescription.h"

namespace Ui {
  class Window;
}

class Problem;

class Window : public QMainWindow
{
  Q_OBJECT;

  std::unique_ptr<Ui::Window> ui;
  std::shared_ptr<const csSkiTracker::dataReader::ProblemData> _data;

  std::unique_ptr<Visualizer> vis;

  Problem& problem;

  bool _initialized;

  void saveAndShow();

public:
  Window(Problem& p, QWidget* parent = 0);
  virtual ~Window();

  void resizeEvent(QResizeEvent* event) override;

public slots:
  void on_pushButtonPlotCameraParams_clicked();
  void on_pushButtonPlotReprojErr_clicked();
  void on_pushButtonPlotReprojErrV1_clicked();
  void on_pushButtonReprojErrSummary_clicked();

  void on_horizontalSlider_valueChanged(int n);

  void on_pushButtonNonLinearRefine_clicked();

  void on_pushButtonPlay_clicked();

  void on_checkBoxShowLabels3D_clicked();
  void on_checkBoxShowLabelsFix_clicked();
  void on_checkBoxShowLabelsMov_clicked();
  void on_checkBoxShowLabelsCorr_clicked();

  void on_checkBoxShowMeas3D_clicked();
  void on_checkBoxShowMeasFix_clicked();
  void on_checkBoxShowMeasMov_clicked();
  void on_checkBoxShowMeasCorr_clicked();

  void on_checkBoxShowReprFix_clicked();
  void on_checkBoxShowReprMov_clicked();
  void on_checkBoxShowReprCorr_clicked();

  void on_checkBoxShowErr3D_clicked();
  void on_checkBoxShowErrFix_clicked();
  void on_checkBoxShowErrMov_clicked();
  void on_checkBoxShowErrCorr_clicked();

  void on_checkBoxShowViewRaysFix_clicked();
  void on_checkBoxShowViewRaysMov_clicked();

  void on_checkBoxShow3DCalibPoints_clicked();
  void on_checkBoxShow3DPoles_clicked();
  void on_checkBoxShow3DCorr_clicked();
};
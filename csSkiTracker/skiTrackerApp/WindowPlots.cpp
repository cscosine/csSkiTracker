// #include "matplotlibcpp/matplotlibcpp.h"

#include "Window.h"
#include "ui_Window.h"

#include "ProblemDescription.h"

#include <QDir>
#include <QImage>
#include <QTemporaryFile>

// namespace plt = matplotlibcpp;

void Window::on_pushButtonPlotReprojErrV1_clicked() {
  /*
    plt::figure_size(this->size().width(), this->size().height());

    plt::suptitle("Reprojection Errors - Static View");

    {
      std::vector<double> ex, ey;
      std::vector<int> indexes;
      std::vector<double> indexes_d;
      //----------------------------------------------------------------------------
      plt::subplot(3, 1, 1);

      plt::title("Reprojection Error X-Y");

      problem.view1CalibPoints().reprojErr2Vec(ex, ey);
      indexes = problem.view1CalibPoints().indexesVec();
      indexes_d.insert(indexes_d.end(), indexes.begin(), indexes.end());

      plt::plot(ex, ey, "+");
      for (int i = 0; i < ex.size(); i++) {
        plt::text(ex[i], ey[i], QString("%1").arg(indexes[i]).toStdString());

      }
      plt::axis("equal");
      plt::xlabel("x [px]");
      plt::ylabel("y [px]");
      plt::grid(true);

      //----------------------------------------------------------------------------
      plt::subplot(3, 1, 2);

      plt::title("Reprojection Error per Id");
      plt::named_plot("x", indexes_d, ex, "r+");
      plt::named_plot("y", indexes_d, ey, "g+");
      plt::xlabel("Frame Id");
      plt::ylabel("err [px]");
      plt::legend();
      plt::grid(true);

      //----------------------------------------------------------------------------
      plt::subplot(3, 1, 3);

      // recompute 3d points
      Eigen::VectorXd error3d = (problem.T_W_wrt_view1().inverse() * problem.view1CalibPoints().p3d_wrt_cam_closest -
    problem.view1CalibPoints3D()).colwise().norm(); std::vector<double> e3d(error3d.size()); for (int i = 0; i < e3d.size(); i++) {
        e3d[i] = error3d(i);
      }

      plt::title("3D distance from point");
      plt::named_plot("x", indexes_d, e3d, "r+");
      plt::xlabel("Frame Id");
      plt::ylabel("err [m]");
      plt::grid(true);

    }

    plt::draw();

    saveAndShow();
  */
}
void Window::on_pushButtonPlotReprojErr_clicked() {
  /*
    int view2Num = ui->horizontalSlider->value();

    plt::figure_size(this->size().width(), this->size().height());

    plt::suptitle(QString("View 2 [%1] - Reproj Err").arg(view2Num).toStdString());

    {
      std::vector<double> ex, ey;
      std::vector<int> indexes;
      std::vector<double> indexes_d;

      //----------------------------------------------------------------------------
      plt::subplot(3, 1, 1);

      plt::title("Reprojection Error X-Y");

      problem.view2CalibPoints(view2Num).reprojErr2Vec(ex, ey);
      indexes = problem.view2CalibPoints(view2Num).indexesVec();
      indexes_d.insert(indexes_d.end(), indexes.begin(), indexes.end());

      plt::plot(ex, ey, "+");
      for (int i = 0; i < ex.size(); i++) {
        plt::text(ex[i], ey[i], QString("%1").arg(indexes[i]).toStdString());
      }
      plt::xlabel("x [px]");
      plt::ylabel("y [px]");

      plt::axis("equal");
      plt::grid(true);

      //----------------------------------------------------------------------------
      plt::subplot(3, 1, 2);

      plt::title("Reprojection Error per Id");
      plt::named_plot("x", indexes_d, ex, "r+");
      plt::named_plot("y", indexes_d, ey, "g+");
      plt::xlabel("Frame Id");
      plt::ylabel("err [px]");
      plt::legend();
      plt::grid(true);

      //----------------------------------------------------------------------------
      plt::subplot(3, 1, 3);

      // recompute 3d points
      Eigen::VectorXd error3d = (problem.T_W_wrt_view2(view2Num).second.inverse() *
    problem.view2CalibPoints(view2Num).p3d_wrt_cam_closest - problem.view2CalibPoints3D(view2Num)).colwise().norm();
      std::vector<double> e3d(error3d.size());
      for (int i = 0; i < e3d.size(); i++) {
        e3d[i] = error3d(i);
      }

      plt::title("3D distance from point");
      plt::named_plot("x", indexes_d, e3d, "r+");
      plt::xlabel("Frame Id");
      plt::ylabel("err [m]");
      plt::grid(true);

    }
    plt::draw();

    saveAndShow();
  */
}

void Window::on_pushButtonPlotCameraParams_clicked() {
  /*
  auto view2idx = problem.validViewsIndexes();
  std::vector<double> view2idx_d;
  view2idx_d.insert(view2idx_d.begin(), view2idx.begin(), view2idx.end());
  auto view2cx = problem.collectView2Cx();
  auto view2cy = problem.collectView2Cy();
  auto view2fx = problem.collectView2Fx();
  auto view2fy = problem.collectView2Fy();

  plt::figure_size(this->size().width(), this->size().height());

  plt::suptitle("Camera Params Distribution");

  plt::subplot(2, 2, 1);
  plt::title("Camera Pricipal Point");
  plt::named_plot("Fixed Camera", std::vector<double>{problem.view1CamPar().cameraModel.cx()},
  std::vector<double>{problem.view1CamPar().cameraModel.cy()}, "r+"); plt::named_plot("Moving Camera", view2cx, view2cy, "g+"); for
  (int i = 0; i < view2cx.size(); i++) { plt::text(view2cx[i], view2cy[i], QString("%1").arg(view2idx[i]).toStdString());
  }
  plt::plot(std::vector<int>{0, problem.view1CamPar().cameraModel.w(), problem.view1CamPar().cameraModel.w(), 0, 0},
    std::vector<int>{0, 0, problem.view1CamPar().cameraModel.h(), problem.view1CamPar().cameraModel.h(), 0},
    "b"
  );
  plt::named_plot("Theorical",
    std::vector<double>{problem.view1CamPar().cameraModel.w() / 2.0},
    std::vector<double>{problem.view1CamPar().cameraModel.h() / 2.0},
    "bx"
  );
  plt::xlabel("x [px]");
  plt::ylabel("y [px]");
  plt::legend();
  plt::grid(true);

  // add view1 at first
  view2idx_d.insert(view2idx_d.begin(), -1); // view1

  view2cx.insert(view2cx.begin(), problem.view1CamPar().cameraModel.cx());
  view2cy.insert(view2cy.begin(), problem.view1CamPar().cameraModel.cy());

  view2fx.insert(view2fx.begin(), problem.view1CamPar().cameraModel.fx());
  view2fy.insert(view2fy.begin(), problem.view1CamPar().cameraModel.fy());

  // plot c distributions
  plt::subplot(2, 2, 2);
  plt::title("Camera Pricipal Point Distribution");
  plt::named_plot("cx", view2idx_d, view2cx, "r+");
  plt::named_plot("cy", view2idx_d, view2cy, "g+");
  plt::xlabel("Frame ID");
  plt::ylabel("position [px]");
  plt::legend();
  plt::grid(true);

  // plot f distributions
  plt::subplot(2, 2, 3);
  plt::title("Camera Focal Length Distribution");
  plt::named_plot("fx", view2idx_d, view2fx, "r+");
  plt::named_plot("fy", view2idx_d, view2fy, "g+");
  plt::xlabel("Frame ID");
  plt::ylabel("focal length [px]");
  plt::legend();
  plt::grid(true);

  std::vector<double> fx_fy(view2fx.size());
  for (int i = 0; i < fx_fy.size(); i++) fx_fy[i] = view2fx[i] / view2fy[i];


  // aspect ratio
  plt::subplot(2, 2, 4);
  plt::title("fx/fy (aspect ratio)");
  plt::plot(view2idx_d, fx_fy, "g+");
  plt::xlabel("Frame ID");
  plt::ylabel("Aspect Ratio [-]");
  plt::grid(true);

  saveAndShow();
*/
}

void Window::on_pushButtonReprojErrSummary_clicked() {
  /*
    plt::figure_size(this->size().width(), this->size().height());


    {
      plt::subplot(2, 1, 1);
      plt::title("Reprojection Errors");

      std::vector<double> e_mean(problem.numValidViews() + 1), e_errMax(problem.numValidViews() + 1);
      std::vector<double> indexes;
      indexes.push_back(-1);
      auto idx = problem.validViewsIndexes();
      indexes.insert(indexes.end(), idx.begin(), idx.end());

      problem.view1CalibPoints().reprojErrMeanMax(e_mean[0], e_errMax[0]);
      for (int i = 0; i < idx.size(); i++) {
        problem.view2CalibPoints(idx[i]).reprojErrMeanMax(e_mean[i + 1], e_errMax[i + 1]);
      }


      plt::errorbar(indexes, e_mean, e_errMax, { {"fmt","r*"} });
      plt::xlabel("Frame Id");
      plt::ylabel("Error [px]");

      plt::grid(true);
    }
    {
      plt::subplot(2, 1, 2);
      plt::title("3D Errors");

      std::vector<double> e_mean(problem.numValidViews() + 1), e_errMax(problem.numValidViews() + 1);
      std::vector<double> indexes;
      indexes.push_back(-1);
      auto idx = problem.validViewsIndexes();
      indexes.insert(indexes.end(), idx.begin(), idx.end());

      {
        Eigen::VectorXd dist = (problem.T_W_wrt_view1().inverse() * problem.view1CalibPoints().p3d_wrt_cam_closest -
    problem.view1CalibPoints3D()).colwise().norm(); e_mean[0] = dist.mean(); e_errMax[0] = (dist.array() -
    e_mean[0]).cwiseAbs().maxCoeff();
      }
      for (int i = 0; i < idx.size(); i++) {
        Eigen::VectorXd dist = (problem.T_W_wrt_view2(i).second.inverse() * problem.view2CalibPoints(i).p3d_wrt_cam_closest -
    problem.view2CalibPoints3D(i)).colwise().norm(); e_mean[i+1] = dist.mean(); e_errMax[i+1] = (dist.array() -
    e_mean[i+1]).cwiseAbs().maxCoeff();
      }


      plt::errorbar(indexes, e_mean, e_errMax, { {"fmt","r*"} });
      plt::xlabel("Frame Id");
      plt::ylabel("Error [m]");

      plt::grid(true);
    }

    plt::draw();

    saveAndShow();
    */
}

void Window::saveAndShow() {
  /*
  QString filename =
  QDir::temp().absoluteFilePath(QString("%1-%2-plot.svg").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationPid()));
  std::cout << filename.toStdString() << std::endl;
  plt::save(filename.toStdString());

  QImage img(filename);
  if (!img.isNull()) {
    QLabel* test = new QLabel();
    test->setPixmap(QPixmap::fromImage(img));
    test->show();
  }
  else {
    std::cerr << "FAILED TO LOAD" << std::endl;
  }
*/
}

#include "Window.h"
#include <QApplication>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QTimer>

#include <QFileDialog>

#include <cstdlib>
#include <iostream>

#include "csSkiTracker/dataReader/DataReader.h"

#include "CommandLineOptions.h"

#include "ProblemDescription.h"

// --smoke: start on the given data file, then after a while check that every
// OpenGL view got its context and end (exit code 0), else exit code 1; for the
// automatic runs of the deployed application (CI, a machine without the
// build). The process ends right after the check (std::_Exit, the output
// flushed): no teardown, which the software OpenGL of Mesa on Windows can hang
// at the unload of its threads, and nothing left to check.
static void checkAndQuit(const Window &window) {
  const auto views = window.findChildren<QOpenGLWidget *>();
  for (const QOpenGLWidget *view : views) {
    if (!view->isValid() || view->context() == nullptr) {
      std::cerr << "smoke: failed, no OpenGL context for "
                << view->objectName().toStdString() << std::endl;
      std::_Exit(1);
    }
  }
  const QSurfaceFormat format =
      views.isEmpty() ? QSurfaceFormat() : views.front()->format();
  std::cout << "smoke: ok, platform "
            << QApplication::platformName().toStdString() << ", "
            << views.size() << " OpenGL view(s), OpenGL "
            << format.majorVersion() << "." << format.minorVersion()
            << std::endl;
  std::_Exit(0);
}

int main(int argc, char *argv[]) {
  Q_INIT_RESOURCE(vis);

  QApplication app(argc, argv);

  CommandLineOptions cmd(argc, argv);

  QString filename = cmd.filename.c_str();
  if (filename.size() == 0) {
    if (cmd.smoke) {
      std::cerr << "smoke: failed, no data file given" << std::endl;
      return 1;
    }
    QString filter = "Ski Tracker Data (*.std)";
    filename = QFileDialog::getOpenFileName(
        nullptr, "Load Ski Tracker Data File", "", filter, &filter);
  }

  csSkiTracker::dataReader::DataReader reader;
  auto d = reader.readFromFile(filename);
  if (d == nullptr) {
    if (cmd.smoke) {
      std::cerr << "smoke: failed, error reading file "
                << filename.toStdString() << ": "
                << reader.error().toStdString() << std::endl;
      return 1;
    }
    QMessageBox::critical(nullptr, "Error",
                          "Error reading file: " + reader.error());
    return 0;
  }

  QFileInfo qi(filename);
  QString f1 = qi.dir().absoluteFilePath(
      qi.completeBaseName() + QDir::separator() + "imgs" + QDir::separator() +
      "deinterlaced_fro");
  QString f2 = qi.dir().absoluteFilePath(
      qi.completeBaseName() + QDir::separator() + "imgs" + QDir::separator() +
      "deinterlaced_lat");

  Problem p = Problem(d->frames.size(), d->worldPoints, f1, cmd.startFrame1, f2,
                      cmd.startFrame2);

  // initialize views, i.e. compute camera pose and f
  p.initView1(d->calib_view1.imgPoints, d->calib_view1.worldPointsId,
              Eigen::Vector2i(d->view1_imgSize.width, d->view1_imgSize.height));
  for (int i = 0; i < d->frames.size(); i++) {
    p.initView2(
        i, d->frames[i].view2.imgPoints, d->frames[i].view2.worldPointsId,
        Eigen::Vector2i(d->view2_imgSize.width, d->view2_imgSize.height));

    p.setImgMatchingPoints(i, d->frames[i].matchingPoints.imgPoints_view1,
                           d->frames[i].matchingPoints.imgPoints_view2);

    p.setImgSkierPoints(i, d->frames[i].skierPoints.imgPoints_view1,
                        d->frames[i].skierPoints.imgPoints_view2);
  }

  Window window(p, cmd, nullptr);
  if (cmd.smoke) {
    QTimer::singleShot(2000, &app, [&window] { checkAndQuit(window); });
  }
  return app.exec();
}

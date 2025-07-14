#include "Window.h"
#include <QApplication>
#include <QMessageBox>

#include <QFileDialog>

#include "csSkiTracker/dataReader/DataReader.h"

#include "CommandLineOptions.h"

#include "ProblemDescription.h"

int main(int argc, char* argv[]) {
  Q_INIT_RESOURCE(vis);

  QApplication app(argc, argv);

  CommandLineOptions cmd(argc, argv);

  QString filename = cmd.filename.c_str();
  if (filename.size() == 0) {
    QString filter = "Ski Tracker Data (*.std)";
    filename = QFileDialog::getOpenFileName(nullptr, "Load Ski Tracker Data File", "", filter, &filter);
  }

  csSkiTracker::dataReader::DataReader reader;
  auto d = reader.readFromFile(filename);
  if (d == nullptr) {
    QMessageBox::critical(nullptr, "Error", "Error reading file: " + reader.error());
  } else {

    QFileInfo qi(filename);
    QString f1 =
        qi.dir().absoluteFilePath(qi.completeBaseName() + QDir::separator() + "imgs" + QDir::separator() + "deinterlaced_fro");
    QString f2 =
        qi.dir().absoluteFilePath(qi.completeBaseName() + QDir::separator() + "imgs" + QDir::separator() + "deinterlaced_lat");

    Problem p = Problem(d->frames.size(), d->worldPoints, f1, cmd.startFrame1, f2, cmd.startFrame2);

    // initialize views, i.e. compute camera pose and f
    p.initView1(d->calib_view1.imgPoints, d->calib_view1.worldPointsId,
                Eigen::Vector2i(d->view1_imgSize.width, d->view1_imgSize.height));
    for (int i = 0; i < d->frames.size(); i++) {
      p.initView2(i, d->frames[i].view2.imgPoints, d->frames[i].view2.worldPointsId,
                  Eigen::Vector2i(d->view2_imgSize.width, d->view2_imgSize.height));

      p.setImgMatchingPoints(i, d->frames[i].matchingPoints.imgPoints_view1, d->frames[i].matchingPoints.imgPoints_view2);

      p.setImgSkierPoints(i, d->frames[i].skierPoints.imgPoints_view1, d->frames[i].skierPoints.imgPoints_view2);
    }

    Window window(p, cmd, nullptr);
    app.exec();
  }

  return 0;
}

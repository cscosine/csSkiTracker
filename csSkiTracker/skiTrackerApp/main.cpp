#include <QApplication>
#include <QMessageBox>
#include "Window.h"

#include <QFileDialog>

#include "csSkiTracker/dataReader/DataReader.h"

#include "CommandLineOptions.h"

#include "ProblemDescription.h"

int main(int argc, char * argv[])
{
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
  }
  else {

    Problem p = Problem(d->frames.size(), d->worldPoints);

    // initialize views, i.e. compute camera pose and f
    p.initView1(d->calib_view1.imgPoints, d->calib_view1.worldPointsId, Eigen::Vector2i(d->view1_imgSize.width, d->view1_imgSize.height));
    for (int i = 0; i < d->frames.size(); i++) {
      p.initView2(i, d->frames[i].view2.imgPoints, d->frames[i].view2.worldPointsId, Eigen::Vector2i(d->view2_imgSize.width, d->view2_imgSize.height));

      // todo: read indexes of points of the skeleton
      Eigen::ArrayXi indexes(d->frames[i].matchingPoints.imgPoints_view1.cols());
      for (int k = 0; k < indexes.size(); k++)indexes[k] = k;
      p.setImgMeasPoints(i, d->frames[i].matchingPoints.imgPoints_view1, d->frames[i].matchingPoints.imgPoints_view2, indexes);
    }

    Window window(p, nullptr);
    app.exec();
  }
 
  

  return 0;
}

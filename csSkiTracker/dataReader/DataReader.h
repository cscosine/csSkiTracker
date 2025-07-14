#pragma once
#include <Eigen/Core>
#include <QString>
#include <memory>
#include <vector>

#include "DataDescription.h"

class QFile;

namespace csSkiTracker {
namespace dataReader {

class DataReader {

  static const QString header;
  static const QString version;

  static const QString view1_img_size_h;
  static const QString view2_img_size_h;

  static const QString world_points_h;
  static const QString frame_h;

  static const QString matching_pts_h;
  static const QString matching_pts_view1_h;
  static const QString matching_pts_view2_h;
  static const QString matching_pts_world_h;

  static const QString calib_pts_view1_h;
  static const QString calib_pts_view2_h;

  static const QString calib_pts_projMatrix_h;
  static const QString calib_pts_imgPoints_h;
  static const QString calib_pts_worldPointsId_h;

  QString _error;

  static QString getLine(QFile& file, int& lineCount);
  static bool parseNameValuePair(const QString& input, const QString& name, int& v, QString& error);
  static bool parseName2ValuesPair(const QString& input, const QString& name, int& v1, int& v2, QString& error);

  static bool readCalibSection(CalibPoints& cp, const QString& header, QFile& file, int& lineCount, QString& error,
                               const Eigen::Matrix3Xd& allWorldPoints);

  template <class T>
  static bool readMatrix(Eigen::Ref<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>> dest, QFile& file, int& lineCount,
                         QString& error);

public:
  DataReader();
  virtual ~DataReader();

  std::shared_ptr<const ProblemData> readFromFile(const QString& filename);
  const QString& error() const {
    return _error;
  }
};

} // namespace dataReader
} // namespace csSkiTracker

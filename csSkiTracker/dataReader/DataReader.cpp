#include "DataReader.h"

#include <QFile>

#include <iostream>


namespace csSkiTracker {
    namespace dataReader {

  const QString DataReader::header = "#SKI_TRACKER_DATA_INPUT";
  const QString DataReader::version = "v1.0";

  const QString DataReader::view1_img_size_h = "#VIEW1_IMG_SIZE";
  const QString DataReader::view2_img_size_h = "#VIEW2_IMG_SIZE";

  const QString DataReader::world_points_h = "#WORLD_POINTS";
  const QString DataReader::frame_h = "#FRAME";

  const QString DataReader::matching_pts_h = "#MATCHING_PTS";
  const QString DataReader::matching_pts_view1_h = "#VIEW_1";
  const QString DataReader::matching_pts_view2_h = "#VIEW_2";
  const QString DataReader::matching_pts_world_h = "#WORLD";

  const QString DataReader::calib_pts_view1_h = "#CALIB_PTS_VIEW_1";
  const QString DataReader::calib_pts_view2_h = "#CALIB_PTS_VIEW_2";

  const QString DataReader::calib_pts_projMatrix_h = "#PROJ_MATRIX";
  const QString DataReader::calib_pts_imgPoints_h = "#IMG_POINTS";
  const QString DataReader::calib_pts_worldPointsId_h = "#WORLD_POINTS_ID";


  DataReader::DataReader() {

  }

  DataReader::~DataReader() {

  }

  QString DataReader::getLine(QFile& file, int& lineCount) {
    while (!file.atEnd() && file.error() == QFile::NoError) {
      QString ret = QString(file.readLine()).simplified();
      lineCount++;
      if (ret.size() != 0) return ret;
    }
    return "";
  }

  bool DataReader::parseName2ValuesPair(const QString& input, const QString& name, int& v1, int& v2, QString& error) {
    auto s = input.split(' ');

    if (s.size() != 3) {
      error = "expected 3 fields";
      return false;
    }

    if (s[0].compare(name) != 0) {
      error = QString("expected %1, got %2").arg(name).arg(s[0]);
      return false;
    }

    bool ok;
    v1 = s[1].toInt(&ok);
    if (!ok) {
      error = QString("conversion to int of %1 failed").arg(s[1]);
    }
    v2 = s[2].toInt(&ok);
    if (!ok) {
      error = QString("conversion to int of %1 failed").arg(s[2]);
    }

    return ok;
  }

  bool DataReader::parseNameValuePair(const QString& input, const QString& name, int& v, QString& error) {
    auto s = input.split(' ');

    if (s.size() != 2) {
      error = "expected 2 fields";
      return false;
    }

    if (s[0].compare(name) != 0) {
      error = QString("expected %1, got %2").arg(name).arg(s[0]);
      return false;
    }

    bool ok;
    v = s[1].toInt(&ok);
    if (!ok) {
      error = QString("conversion to int of %1 failed").arg(s[1]);
    }

    return ok;
  }

  template<class T>
  bool DataReader::readMatrix(Eigen::Ref<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>> dest, QFile& file, int& lineCount, QString& error) {
    int n = dest.cols();
    int m = dest.rows();
    if (m == 0) {
      return true; //dont read line, is empty, will be skipped
    }
    for (int i = 0; i < n; i++) {
      QString line = getLine(file, lineCount);
      auto vs = line.split(" ");
      if (vs.size() != m) {
        error = QString("line %1: Expected %2 values, got %3").arg(lineCount).arg(m).arg(vs.size());
        return false;
      }
      for (int j = 0; j < m; j++) {
        bool ok;
        dest(j, i) = vs[j].toDouble(&ok);
        if (!ok) {
          error = QString("conversion to double of %1 failed").arg(vs[j]);
          return false;
        }
      }
    }
    return true;
  }


  bool DataReader::readCalibSection(CalibPoints& cp, const QString& header, QFile& file, int& lineCount, QString& _error, const Eigen::Matrix3Xd & allWorldPoints) {
    int nCalib;
    bool ok = true;
    QString line;
    if (ok) {
      QString err;
      line = getLine(file, lineCount);
      ok = parseNameValuePair(line, header, nCalib, err);
      if (!ok) {
        _error = QString("line %1: %2").arg(lineCount).arg(err);
      }
      else {
        cp = CalibPoints(nCalib);
      }
    }
    // check proj matrix header
    if (ok) {
      line = getLine(file, lineCount);
      if (line.compare(calib_pts_projMatrix_h) != 0) {
        _error = QString("line %1: wrong header, expected %2, got %3").arg(lineCount).arg(calib_pts_projMatrix_h).arg(line);
        ok = false;
      }
    }
    // read proj matrix
    if (ok) {
      // trick, readMatrix read a transposed matrix really....
      Eigen::Matrix<double, 4, 3> pT;
      ok = readMatrix<double>(pT, file, lineCount, _error);
      if (ok) {
        cp.projMat = pT.transpose();
        //std::cout << cp.projMat << std::endl;
      }
    }

    // check img points
    if (ok) {
      line = getLine(file, lineCount);
      if (line.compare(calib_pts_imgPoints_h) != 0) {
        _error = QString("line %1: wrong header, expected %2, got %3").arg(lineCount).arg(calib_pts_imgPoints_h).arg(line);
        ok = false;
      }
    }
    // read img points matrix
    if (ok) {
      ok = readMatrix<double>(cp.imgPoints, file, lineCount, _error);
    }

    if (ok) {
      line = getLine(file, lineCount);
      if (line.compare(calib_pts_worldPointsId_h) != 0) {
        _error = QString("line %1: wrong header, expected %2, got %3").arg(lineCount).arg(calib_pts_worldPointsId_h).arg(line);
        ok = false;
      }
    }
    // read world matching ids
    if (ok) {      
      ok = readMatrix<int>(cp.worldPointsId, file, lineCount, _error);
    }

    return ok;
  }

  std::shared_ptr<const ProblemData> DataReader::readFromFile(const QString& filename) {
    _error = "";

    int lineCount = 0;

    QFile file;
    std::shared_ptr<ProblemData> ret = std::make_shared<ProblemData>();

    file.setFileName(filename);
    bool ok = file.open(QIODevice::ReadOnly);
    if (!ok) {
      _error = "Error Opening " + filename;
    }

    // look for header
    if (ok) {
      QString header_line = getLine(file, lineCount);
      if (header_line.compare(DataReader::header + " " + DataReader::version) != 0) {
        _error = QString("line %1: wrong header, expected %2, got %3").arg(lineCount).arg(DataReader::header).arg(header);
        ok = false;
      }
    }

    // look for image sizes
    if (ok) {
      QString line = getLine(file, lineCount);
      int nw, nh;
      QString err;
      ok = parseName2ValuesPair(line, view1_img_size_h, nw, nh, err);
      if (!ok) {
        _error = QString("line %1: %2").arg(lineCount).arg(err);
      }
      else {
        ret->view1_imgSize.width = nw;
        ret->view1_imgSize.height = nh;
      }
    }
    if (ok) {
      QString line = getLine(file, lineCount);
      int nw, nh;
      QString err;
      ok = parseName2ValuesPair(line, view2_img_size_h, nw, nh, err);
      if (!ok) {
        _error = QString("line %1: %2").arg(lineCount).arg(err);
      }
      else {
        ret->view2_imgSize.width = nw;
        ret->view2_imgSize.height = nh;
      }
    }

    // look for world points
    if (ok) {
      QString line = getLine(file, lineCount);
      int nw;
      QString err;
      ok = parseNameValuePair(line, world_points_h, nw, err);
      if (!ok) {
        _error = QString("line %1: %2").arg(lineCount).arg(err);
      }
      else {
        ret->worldPoints.resize(3, nw);
      }
    }

    // read world points
    if (ok) {
      ok = readMatrix<double>(ret->worldPoints, file, lineCount, _error);
      // convert to meters
      ret->worldPoints = ret->worldPoints / 1000.0;
    }

    // read calib_pts
    if (ok) {
      ok = readCalibSection(ret->calib_view1, calib_pts_view1_h, file, lineCount, _error, ret->worldPoints);
    }


    // read frames
    int expFrameNum = 1;
    bool readFrameOk = true;
    QString line;
    do {

      line = getLine(file, lineCount);
      if (line.size() == 0) {
        // not an error, no more frames!
        readFrameOk = false;
      }
      // I have a frame to read
      if (readFrameOk) {
        FrameData frame_data;
        {
          int nFrame;
          QString err;
          ok = parseNameValuePair(line, frame_h, nFrame, err);
          if (!ok) {
            _error = QString("line %1: %2").arg(lineCount).arg(err);
          }
          // check expected frame number
          if (ok) {
            if (nFrame != expFrameNum) {
              ok = false;
              _error = QString("line %1: got frame %2, expected %3").arg(lineCount).arg(nFrame).arg(expFrameNum);
            }
            else {
              expFrameNum++;
            }
          }
        }
        //-----------------------------------------------
        // read matching pts
        {
          int nMatches;
          MatchingPoints& mp = frame_data.matchingPoints;
          if (ok) {
            QString err;
            line = getLine(file, lineCount);
            ok = parseNameValuePair(line, matching_pts_h, nMatches, err);
            if (!ok) {
              _error = QString("line %1: %2").arg(lineCount).arg(err);
            }
            else {
              mp = MatchingPoints(nMatches);
            }
          }
          // check view1 header
          if (ok) {
            line = getLine(file, lineCount);
            if (line.compare(matching_pts_view1_h) != 0) {
              _error = QString("line %1: wrong header, expected %2, got %3").arg(lineCount).arg(matching_pts_view1_h).arg(line);
              ok = false;
            }
          }
          // read view1
          if (ok) {
            ok = readMatrix<double>(mp.imgPoints_view1, file, lineCount, _error);
          }
          // check view2 header
          if (ok) {
            line = getLine(file, lineCount);
            if (line.compare(matching_pts_view2_h) != 0) {
              _error = QString("line %1: wrong header, expected %2, got %3").arg(lineCount).arg(matching_pts_view2_h).arg(line);
              ok = false;
            }
          }
          // read view2
          if (ok) {
            ok = readMatrix<double>(mp.imgPoints_view2, file, lineCount, _error);
          }
          // check world header
          if (ok) {
            line = getLine(file, lineCount);
            if (line.compare(matching_pts_world_h) != 0) {
              _error = QString("line %1: wrong header, expected %2, got %3").arg(lineCount).arg(matching_pts_world_h).arg(line);
              ok = false;
            }
          }
          // read world
          if (ok) {
            ok = readMatrix<double>(mp.worldPoints, file, lineCount, _error);
            // convert to meters
            mp.worldPoints = mp.worldPoints / 1000.0;
          }

        }
        //-----------------------------------------------
        if (ok) {
          ok = readCalibSection(frame_data.view2, calib_pts_view2_h, file, lineCount, _error, ret->worldPoints);
        }
        if (ok) {
          ret->frames.push_back(frame_data);
        }

        if (!ok) {
          readFrameOk = false;
        }

      } //end if on frame
    } while (readFrameOk);

    if (ok) {
      assert(ret != nullptr);
      return ret;
    }
    else {
      return nullptr;
    }
  }

  template bool DataReader::readMatrix<double>(Eigen::Ref<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>  dest, QFile& file, int& lineCount, QString& error);
  template bool DataReader::readMatrix<int>(Eigen::Ref<Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>>  dest, QFile& file, int& lineCount, QString& error);

}
}
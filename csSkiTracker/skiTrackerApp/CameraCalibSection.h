#pragma once
#include "csNelson/SingleSection.h"

#include "CameraNode.h"
#include "CameraPosePointNodeBase.h"
#include "PointNode.h"
#include "PoseNode.h"

#include <vector>

class CameraCalibSection : public csNelson::SingleSection<CameraCalibSection, CameraPosePointNodeBase, csBlockMatrix::BlockCoeffSparse,
                                                          double, csBlockMatrix::Variable> {

  std::array<CameraNode, 2> _cameras, _camerasBck;
  PoseNode _view1Pose, _view1PoseBck;
  std::vector<PoseNode> _view2Poses, _view2PosesBck;

  bool _hasPointXY;
  PointXYNode _pointXY, _pointXYBck;
  std::vector<PointNode> _points, _pointsBck;
  std::vector<PointNode> _fixedPoints;

  std::vector<std::vector<PointNode>> _skierPoints, _skierPointsBck;
  std::vector<int> _skierPointsStarts;
  std::vector<std::pair<int, int>> _skierPointsLinear2ij;

  std::vector<int> _parametersSize;

  using SingleSectionBase = csNelson::SingleSection<CameraCalibSection, CameraPosePointNodeBase, csBlockMatrix::BlockCoeffSparse,
                                                    double, csBlockMatrix::Variable>;

  void init(const std::vector<csLie::SE3d>& view2Poses, const Eigen::Matrix3Xd& points, const Eigen::Matrix3Xd& fixedPoints,
            const std::vector<Eigen::Matrix3Xd>& skierPoints);

public:
  CameraCalibSection(const CameraNode& camera1, const CameraNode& camera2, const csLie::SE3d& view1Pose,
                     const std::vector<csLie::SE3d>& view2Poses, const Eigen::Matrix3Xd& points, const Eigen::Matrix3Xd& fixedPoints,
                     const Eigen::Vector2d& p3_xy, const Eigen::Isometry3d& T_p012, const std::vector<Eigen::Matrix3Xd>& skierPoints);
  CameraCalibSection(const CameraNode& camera1, const CameraNode& camera2, const csLie::SE3d& view1Pose,
                     const std::vector<csLie::SE3d>& view2Poses, const Eigen::Matrix3Xd& points, const Eigen::Matrix3Xd& fixedPoints,
                     const std::vector<Eigen::Matrix3Xd>& skierPoints);
  virtual ~CameraCalibSection();

  void backupSolution() {
    _camerasBck = _cameras;
    _view1PoseBck = _view1Pose;
    _view2PosesBck = _view2Poses;
    _pointsBck = _points;
    _pointXYBck = _pointXY;
    _skierPointsBck = _skierPoints;
  }

  void rollbackSolution() {
    _cameras = _camerasBck;
    _view1Pose = _view1PoseBck;
    _view2Poses = _view2PosesBck;
    _points = _pointsBck;
    _pointXY = _pointXYBck;
    _skierPoints = _skierPointsBck;
  }

  const std::vector<int>& parameterSize() const override {
    return _parametersSize;
  }

  int numFixedParameters() const override {
    return _fixedPoints.size();
  }

  void oplus(const typename SingleSectionBase::HessianVecType& inc) {
    int ns = 0;
    _cameras[0].oplus(inc.segment(this->user2internalIndexes()(ns++)));

    if (_view2Poses.size() > 0) {
      _cameras[1].oplus(inc.segment(this->user2internalIndexes()(ns++)));
    }

    _view1Pose.oplus(inc.segment(this->user2internalIndexes()(ns++)));
    for (int i = 0; i < _view2Poses.size(); i++) {
      _view2Poses[i].oplus(inc.segment(this->user2internalIndexes()(ns++)));
    }

    if (_hasPointXY)
      _pointXY.setPoint(_pointXY.point() + inc.segment(this->user2internalIndexes()(ns++)));

    for (int i = 0; i < _points.size(); i++) {
      _points[i].setPoint(_points[i].point() + inc.segment(this->user2internalIndexes()(ns++)));
    }

    for (int i = 0; i < _skierPoints.size(); i++) {
      for (int j = 0; j < _skierPoints[i].size(); j++) {
        _skierPoints[i][j].setPoint(_skierPoints[i][j].point() + inc.segment(this->user2internalIndexes()(ns++)));
      }
    }

    assert(ns == inc.numSegments());
  }

  csNelson::NodeId camera1ParId() const {
    return csNelson::NodeId(0);
  }
  csNelson::NodeId camera2ParId() const {
    if (_view2Poses.size() > 0) {
      return csNelson::NodeId(1);
    } else {
      assert(false);
      return csNelson::NodeId(-1);
    }
  }
  csNelson::NodeId camera1PoseId() const {
    if (_view2Poses.size() > 0) {
      return csNelson::NodeId(2);
    } else {
      return csNelson::NodeId(1);
    }
  }
  csNelson::NodeId camera2PoseId(int i) const {
    return csNelson::NodeId(2 + (_view2Poses.size() > 0 ? 1 : 0) + i);
  }

  csNelson::NodeId pointXYId() const {
    assert(_hasPointXY);
    return csNelson::NodeId(2 + (_view2Poses.size() > 0 ? 1 : 0) + _view2Poses.size());
  }
  csNelson::NodeId pointId(int i) const {
    return csNelson::NodeId(2 + (_view2Poses.size() > 0 ? 1 : 0) + _view2Poses.size() + (_hasPointXY ? 1 : 0) + i);
  }

  csNelson::NodeId skierPointId(int i, int j) const {
    int start = 2 + (_view2Poses.size() > 0 ? 1 : 0) + _view2Poses.size() + (_hasPointXY ? 1 : 0) + _points.size();
    return csNelson::NodeId(start + _skierPointsStarts[i] + j);
  }

  const csCamera::Camerad& view1Camera() const {
    return _cameras[0].camera();
  }
  const csCamera::Camerad& view2Camera() const {
    return _cameras[1].camera();
  }
  const csCamera::CameraDistortionModeld& view1CameraDistModel() const {
    return _cameras[0].distModel();
  }
  const csCamera::CameraDistortionModeld& view2CameraDistModel() const {
    return _cameras[1].distModel();
  }
  const csLie::SE3d& view1Pose() const {
    return _view1Pose.pose();
  }
  int numView2Poses() const {
    return _view2Poses.size();
  }
  const csLie::SE3d& view2Pose(int i) const {
    return _view2Poses[i].pose();
  }

  const Eigen::Vector2d& pointXY() const {
    return _pointXY.point();
  }
  const Eigen::Isometry3d& pointXYRefFrame() const {
    return _pointXY.T_W_planeOrigin();
  }

  const Eigen::Vector3d& point(int i) const {
    return _points[i].point();
  }
  const Eigen::Vector3d& fixedPoint(int i) const {
    return _fixedPoints[i].point();
  }

  const Eigen::Vector3d& skierPoint(int i, int j) const {
    return _skierPoints[i][j].point();
  }

  virtual const CameraPosePointNodeBase& parameter(csNelson::NodeId i) const override {
    if (i.isVariable()) {
      if (i.id() == 0) {
        return _cameras[0];
      } else if (i.id() == 1) {
        if (_view2Poses.size() > 0) {
          return _cameras[1];
        } else {
          return _view1Pose;
        }
      } else if (i.id() == 2) {
        return _view1Pose;
      } else {
        int id = i.id() - 3;
        if (id < _view2Poses.size()) {
          return _view2Poses[i.id() - 3];
        } else {
          // a point
          int id = i.id() - 3 - _view2Poses.size();
          if (_hasPointXY) {
            if (id == 0)
              return _pointXY;
            id = id - 1;
            if (id < _points.size()) {
              return _points[id];
            } else {
              id = id - _points.size();
              return _skierPoints[_skierPointsLinear2ij[id].first][_skierPointsLinear2ij[id].second];
            }

          } else {
            if (id < _points.size()) {
              return _points[id];
            } else {
              id = id - _points.size();
              return _skierPoints[_skierPointsLinear2ij[id].first][_skierPointsLinear2ij[id].second];
            }
          }
        }
      }
    } else {
      return _fixedPoints[i.id()];
    }
  }

  virtual CameraPosePointNodeBase& parameter(csNelson::NodeId i) override {
    if (i.isVariable()) {
      if (i.id() == 0) {
        return _cameras[0];
      } else if (i.id() == 1) {
        if (_view2Poses.size() > 0) {
          return _cameras[1];
        } else {
          return _view1Pose;
        }
      } else if (i.id() == 2) {
        return _view1Pose;
      } else {
        int id = i.id() - 3;
        if (id < _view2Poses.size()) {
          return _view2Poses[i.id() - 3];
        } else {
          int id = i.id() - 3 - _view2Poses.size();
          if (_hasPointXY) {
            if (id == 0)
              return _pointXY;
            id = id - 1;
            if (id < _points.size()) {
              return _points[id];
            } else {
              id = id - _points.size();
              return _skierPoints[_skierPointsLinear2ij[id].first][_skierPointsLinear2ij[id].second];
            }

          } else {
            if (id < _points.size()) {
              return _points[id];
            } else {
              id = id - _points.size();
              return _skierPoints[_skierPointsLinear2ij[id].first][_skierPointsLinear2ij[id].second];
            }
          }
        }
      }
    } else {
      return _fixedPoints[i.id()];
    }
  }
};

#include "CameraCalibSection.h"

#include "CameraNode.hpp"
#include "PoseNode.h"

#include <csNelson/SingleSection_impl.hpp>

#include <cassert>

CameraCalibSection::CameraCalibSection(const CameraNode& camera1, const CameraNode& camera2, const csLie::SE3d& view1Pose,
                                       const std::vector<csLie::SE3d>& view2Poses, const Eigen::Matrix3Xd& points,
                                       const Eigen::Matrix3Xd& fixedPoints, const Eigen::Vector2d& p3_xy,
                                       const Eigen::Isometry3d& T_p012, const std::vector<Eigen::Matrix3Xd>& skierPoints)
    : _cameras({camera1, camera2})
    , _camerasBck({camera1, camera2})
    , // need init
    _view1Pose(PoseNode(view1Pose))
    , _pointXY(T_p012, p3_xy)
    , _hasPointXY(true) {
  assert(fixedPoints.cols() == 2);

  this->init(view2Poses, points, fixedPoints, skierPoints);
}

CameraCalibSection::CameraCalibSection(const CameraNode& camera1, const CameraNode& camera2, const csLie::SE3d& view1Pose,
                                       const std::vector<csLie::SE3d>& view2Poses, const Eigen::Matrix3Xd& points,
                                       const Eigen::Matrix3Xd& fixedPoints, const std::vector<Eigen::Matrix3Xd>& skierPoints)
    : _cameras({camera1, camera2})
    , _camerasBck({camera1, camera2})
    , // need init
    _view1Pose(PoseNode(view1Pose))
    , _hasPointXY(false) {
  this->init(view2Poses, points, fixedPoints, skierPoints);
}

void CameraCalibSection::init(const std::vector<csLie::SE3d>& view2Poses, const Eigen::Matrix3Xd& points,
                              const Eigen::Matrix3Xd& fixedPoints, const std::vector<Eigen::Matrix3Xd>& skierPoints) {

  {
    _skierPointsStarts.resize(skierPoints.size() + 1); // last is total count
    int nSkierPoints = 0;
    for (int i = 0; i < skierPoints.size(); i++) {
      _skierPointsStarts[i] = nSkierPoints;
      nSkierPoints += skierPoints[i].cols();
    }
    _skierPointsStarts.back() = nSkierPoints;

    _skierPointsLinear2ij.resize(nSkierPoints);
    nSkierPoints = 0;
    for (int i = 0; i < skierPoints.size(); i++) {
      for (int j = 0; j < skierPoints[i].cols(); j++) {
        _skierPointsLinear2ij[nSkierPoints++] = std::make_pair(i, j);
      }
    }
  }

  _view2Poses.resize(view2Poses.size());
  _points.resize(points.cols());
  _fixedPoints.resize(fixedPoints.cols());
  _skierPoints.resize(skierPoints.size());

  _parametersSize.resize(2 +                             // 1 camera params + 1 camera pose (fixed)
                         (view2Poses.size() > 0 ? 1 : 0) // 1 camera params (moving)
                         + view2Poses.size()             // moving camera poses
                         + points.cols()                 // 3d calib points to estimate
                         + (_hasPointXY ? 1 : 0)         // additional calib points on plane
                         + _skierPointsStarts.back()     // skier points along all frames
  );

  for (int i = 0; i < _view2Poses.size(); i++) {
    _view2Poses[i].setPose(view2Poses[i]);
  }
  for (int i = 0; i < points.cols(); i++) {
    _points[i].setPoint(points.col(i));
  }
  for (int i = 0; i < fixedPoints.cols(); i++) {
    _fixedPoints[i].setPoint(fixedPoints.col(i));
  }
  for (int i = 0; i < _skierPoints.size(); i++) {
    _skierPoints[i].resize(skierPoints[i].cols());
    for (int j = 0; j < _skierPoints[i].size(); j++) {
      _skierPoints[i][j].setPoint(skierPoints[i].col(j));
    }
  }

  int p = 0;
  _parametersSize[p++] = _cameras[0].numParams(); // first camera intrinsics
  if (view2Poses.size() > 0) {
    _parametersSize[p++] = _cameras[1].numParams(); // second camera intrinsics
  }
  _parametersSize[p++] = 6; // first camera pose
  for (int i = 0; i < _view2Poses.size(); i++) {
    _parametersSize[p++] = 6; // seconds cameras poses
  }
  // xy point
  if (_hasPointXY)
    _parametersSize[p++] = 2;
  // 3d points
  for (int i = 0; i < _points.size(); i++) {
    _parametersSize[p++] = 3;
  }
  // skier points
  for (int i = 0; i < _skierPoints.size(); i++) {
    for (int j = 0; j < _skierPoints[i].size(); j++) {
      _parametersSize[p++] = 3;
    }
  }
  assert(p == _parametersSize.size());

  // ok, no further operation on parameters foreseen
  this->parametersReady();
}

CameraCalibSection::~CameraCalibSection() {}

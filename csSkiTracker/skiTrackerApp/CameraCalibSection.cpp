#include "CameraCalibSection.h"

#include "CameraNode.hpp"
#include "PoseNode.h"

#include "csNelson/SingleSection.hpp"

CameraCalibSection::CameraCalibSection(const CameraNode& camera1, const CameraNode& camera2, const csLie::SE3d& view1Pose,
                                       const std::vector<csLie::SE3d>& view2Poses)
    : _cameras({camera1, camera2})
    , _camerasBck({camera1, camera2})
    , // need init
    _view1Pose(PoseNode(view1Pose))
    , _view2Poses(view2Poses.size())
    , _parametersSize(2 + (view2Poses.size() > 0 ? 1 : 0) + view2Poses.size()) {
  for (int i = 0; i < _view2Poses.size(); i++) {
    _view2Poses[i].setPose(view2Poses[i]);
  }

  int p = 0;
  _parametersSize[p++] = _cameras[0].numParams();
  if (view2Poses.size() > 0) {
    _parametersSize[p++] = _cameras[1].numParams();
  }
  _parametersSize[p++] = 6;
  for (int i = 0; i < _view2Poses.size(); i++) {
    _parametersSize[p++] = 6;
  }

  // ok, no further operation on parameters foreseen
  this->parametersReady();
}

CameraCalibSection::~CameraCalibSection() {}

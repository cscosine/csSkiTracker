#include "CameraNode.h"
#include "CameraNode.hpp"

#include "csCamera/Camera.hpp"
#include "csCamera/CameraDistortionModel.hpp"

CameraNode::CameraNode(const csCamera::Camerad& camera, const csCamera::CameraDistortionModeld& distModel,
                       FocalEstimation focalEstimation, const std::array<bool, 2>& fixCenter, const std::array<bool, 6>& fixKs,
                       const std::array<bool, 2>& fixPs, const std::array<bool, 4>& fixSs)
    : CameraPoseNodeBase(CameraPoseNodeBaseType::Camera)
    , _camera(camera)
    , _distModel(distModel)
    , _focalEstimation(focalEstimation)
    , _fixCenter(fixCenter)
    , _fixKs(fixKs)
    , _fixPs(fixPs)
    , _fixSs(fixSs)
    , _numParams(0) {
  this->_aspectRatio = _camera.fx() / _camera.fy();

  if (_focalEstimation == FocalEstimation::FixRatio) {
    _numParams++;
  } else if (_focalEstimation == FocalEstimation::Both) {
    _numParams += 2;
  }

  for (auto b : _fixCenter) {
    if (!b) {
      _numParams++;
    }
  }
  for (auto b : _fixKs) {
    if (!b) {
      _numParams++;
    }
  }
  for (auto b : _fixPs) {
    if (!b) {
      _numParams++;
    }
  }
  for (auto b : _fixSs) {
    if (!b) {
      _numParams++;
    }
  }
}

CameraNode::~CameraNode() {}

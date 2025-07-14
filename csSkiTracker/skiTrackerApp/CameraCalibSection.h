#pragma once
#include "csNelson/SingleSection.h"

#include "CameraPoseNodeBase.h"
#include "PoseNode.h"
#include "CameraNode.h"

#include <vector>

class CameraCalibSection : public csNelson::SingleSection<CameraCalibSection, CameraPoseNodeBase, csBlockMatrix::BlockCoeffSparse, double, csBlockMatrix::Variable> {

  std::array<CameraNode, 2> _cameras, _camerasBck;
  PoseNode _view1Pose, _view1PoseBck;
  std::vector<PoseNode> _view2Poses, _view2PosesBck;

  std::vector<int> _parametersSize;

  using SingleSectionBase = csNelson::SingleSection<CameraCalibSection, CameraPoseNodeBase, csBlockMatrix::BlockCoeffSparse, double, csBlockMatrix::Variable>;

public:
  CameraCalibSection(
    const CameraNode& camera1,
    const CameraNode& camera2,
    const csLie::SE3d& view1Pose,
    const std::vector<csLie::SE3d>& view2Poses
  );
  virtual ~CameraCalibSection();

  void backupSolution() {
    _camerasBck = _cameras;
    _view1PoseBck = _view1Pose;
    _view2PosesBck = _view2Poses;
  }

  void rollbackSolution() {
    _cameras = _camerasBck;
    _view1Pose = _view1PoseBck;
    _view2Poses = _view2PosesBck;
  }


  const std::vector<int>& parameterSize() const override {
    return _parametersSize;
  }

  void oplus(const typename SingleSectionBase::HessianVecType& inc) {
    int ns = 0;
    _cameras[0].oplus(inc.segment(ns++));

    if (_view2Poses.size() > 0) {
      _cameras[1].oplus(inc.segment(ns++));
    }

    _view1Pose.oplus(inc.segment(ns++));
    for (int i = 0; i < _view2Poses.size(); i++) {
      _view2Poses[i].oplus(inc.segment(ns++));
    }
    assert(ns == inc.numSegments());
  }

  csNelson::NodeId camera1ParId() const { return csNelson::NodeId(0); }
  csNelson::NodeId camera2ParId() const { 
    if (_view2Poses.size() > 0) {
      return csNelson::NodeId(1);
    }
    else {
      assert(false);
      return csNelson::NodeId(-1);
    }
  }
  csNelson::NodeId camera1PoseId() const { 
    if (_view2Poses.size() > 0) {
      return csNelson::NodeId(2);
    }
    else {
      return csNelson::NodeId(1);
    }
  }
  csNelson::NodeId camera2PoseId(int i) const { 
    return csNelson::NodeId(2 + (_view2Poses.size() > 0 ? 1 : 0) + i);
  }

  const csCamera::Camerad& view1Camera() const { return _cameras[0].camera(); }
  const csCamera::Camerad& view2Camera() const { return _cameras[1].camera(); }
  const csCamera::CameraDistortionModeld& view1CameraDistModel() const { return _cameras[0].distModel(); }
  const csCamera::CameraDistortionModeld& view2CameraDistModel() const { return _cameras[1].distModel(); }
  const csLie::SE3d& view1Pose() const { return _view1Pose.pose(); }
  int numView2Poses() const { return _view2Poses.size(); }
  const csLie::SE3d& view2Pose(int i) const { return _view2Poses[i].pose(); }

  virtual const CameraPoseNodeBase& parameter(csNelson::NodeId i) const override {
    assert(i.isVariable()); 
    if (i.id() == 0) { return _cameras[0]; }
    else if (i.id() == 1) { 
      if (_view2Poses.size() > 0) {
        return _cameras[1];
      }
      else {
        return _view1Pose;
      }
    }
    else if (i.id() == 2) { 
      return _view1Pose; 
    }
    else { return _view2Poses[i.id() - 3]; }
  }
  virtual CameraPoseNodeBase& parameter(csNelson::NodeId i) override {
    assert(i.isVariable());
    if (i.id() == 0) { return _cameras[0]; }
    else if (i.id() == 1) {
      if (_view2Poses.size() > 0) {
        return _cameras[1];
      }
      else {
        return _view1Pose;
      }
    }
    else if (i.id() == 2) {
      return _view1Pose;
    }
    else { return _view2Poses[i.id() - 3]; }
  }


};
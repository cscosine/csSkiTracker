#pragma once
#include "CameraNode.h"

template<typename Derived>
void CameraNode::oplus(const Eigen::MatrixBase<Derived>& oplus) {
  assert(oplus.size() == _numParams);

  int np = 0;

  // -- focal
  double new_fx = _camera.fx();
  double new_fy = _camera.fy();
  if (_focalEstimation == FocalEstimation::FixRatio) {
    new_fx += oplus(np++);
    new_fy = new_fx / _aspectRatio;
  }
  else if (_focalEstimation == FocalEstimation::Both) {
    new_fx += oplus(np++);
    new_fy += oplus(np++);
    this->_aspectRatio = new_fx / new_fy;
  }
  else {
    // nothing
  }

  // -- center
  double new_cx = _camera.cx() + (_fixCenter[0] ? 0 : oplus(np++));
  double new_cy = _camera.cy() + (_fixCenter[1] ? 0 : oplus(np++));

  double newK1 = _distModel.k1() + (_fixKs[0] ? 0 : oplus(np++));
  double newK2 = _distModel.k2() + (_fixKs[1] ? 0 : oplus(np++));
  double newK3 = _distModel.k3() + (_fixKs[2] ? 0 : oplus(np++));
  double newK4 = _distModel.k4() + (_fixKs[3] ? 0 : oplus(np++));
  double newK5 = _distModel.k5() + (_fixKs[4] ? 0 : oplus(np++));
  double newK6 = _distModel.k6() + (_fixKs[5] ? 0 : oplus(np++));

  double newP1 = _distModel.p1() + (_fixPs[0] ? 0 : oplus(np++));
  double newP2 = _distModel.p2() + (_fixPs[1] ? 0 : oplus(np++));

  double newS1 = _distModel.s1x() + (_fixSs[0] ? 0 : oplus(np++));
  double newS2 = _distModel.s2x() + (_fixSs[1] ? 0 : oplus(np++));
  double newS3 = _distModel.s3y() + (_fixSs[2] ? 0 : oplus(np++));
  double newS4 = _distModel.s4y() + (_fixSs[3] ? 0 : oplus(np++));

  csCamera::Camerad newCamera = csCamera::Camerad(new_fx, new_fy, new_cx, new_cy, _camera.w(), _camera.h());
  csCamera::CameraDistortionModeld newDistModel = csCamera::CameraDistortionModeld(newK1, newK2, newK3, newP1, newP2, newK4, newK5, newK6, newS1, newS2, newS3, newS4);

  this->_camera = newCamera;
  this->_distModel = newDistModel;

  assert(_numParams == np);

}

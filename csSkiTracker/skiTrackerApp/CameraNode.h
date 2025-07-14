#pragma once

#include "csCamera/Camera.h"
#include "csCamera/CameraDistortionModel.h"

#include "CameraPoseNodeBase.h"

#include <array>

class CameraNode : public CameraPoseNodeBase {
public:
  enum class FocalEstimation { Fixed, FixRatio, Both };

private:
  csCamera::Camerad _camera;
  csCamera::CameraDistortionModeld _distModel;
  double _aspectRatio;

  FocalEstimation _focalEstimation;
  std::array<bool, 2> _fixCenter;
  std::array<bool, 6> _fixKs;
  std::array<bool, 2> _fixPs;
  std::array<bool, 4> _fixSs;

  int _numParams;

public:
  CameraNode(const csCamera::Camerad& camera, const csCamera::CameraDistortionModeld& distModel, FocalEstimation focalEstimation,
             const std::array<bool, 2>& fixCenter, const std::array<bool, 6>& fixKs, const std::array<bool, 2>& fixPs,
             const std::array<bool, 4>& fixSs);
  virtual ~CameraNode();

  double aspectRatio() const {
    return _aspectRatio;
  }
  int numParams() const {
    return _numParams;
  }

  const csCamera::Camerad& camera() const {
    return _camera;
  }
  const csCamera::CameraDistortionModeld& distModel() const {
    return _distModel;
  }

  FocalEstimation focalEstimation() const {
    return _focalEstimation;
  }
  const std::array<bool, 2>& fixCenter() const {
    return _fixCenter;
  }
  const std::array<bool, 6>& fixKs() const {
    return _fixKs;
  }
  const std::array<bool, 2>& fixPs() const {
    return _fixPs;
  }
  const std::array<bool, 4>& fixSs() const {
    return _fixSs;
  }

  template <typename Derived>
  void oplus(const Eigen::MatrixBase<Derived>& oplus);
};

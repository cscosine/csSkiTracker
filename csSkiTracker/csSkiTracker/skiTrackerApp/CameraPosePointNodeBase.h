#pragma once

enum class CameraPosePointNodeBaseType { Camera, Pose, Point, PointXY };

class CameraPosePointNodeBase {
private:
  CameraPosePointNodeBaseType _type;

public:
  CameraPosePointNodeBase(CameraPosePointNodeBaseType type);
  virtual ~CameraPosePointNodeBase();

  CameraPosePointNodeBaseType type() const {
    return _type;
  }
};

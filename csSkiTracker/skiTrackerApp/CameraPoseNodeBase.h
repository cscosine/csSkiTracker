#pragma once

enum class CameraPoseNodeBaseType { Camera, Pose };

class CameraPoseNodeBase {
private:
  CameraPoseNodeBaseType _type;

public:
  CameraPoseNodeBase(CameraPoseNodeBaseType type)
      : _type(type) {}
  virtual ~CameraPoseNodeBase() {}

  CameraPoseNodeBase type() const {
    return _type;
  }
};

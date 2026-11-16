#include "PoseNode.h"

PoseNode::PoseNode()
    : CameraPosePointNodeBase(CameraPosePointNodeBaseType::Pose)
    , _pose(csLie::SE3d::Identity()) {}
PoseNode::PoseNode(const csLie::SE3d& pose)
    : CameraPosePointNodeBase(CameraPosePointNodeBaseType::Pose)
    , _pose(pose) {}

PoseNode::~PoseNode() {}

void PoseNode::setPose(const csLie::SE3d& pose) {
  this->_pose = pose;
}

#include "PoseNode.h"

PoseNode::PoseNode() 
  : 
  CameraPoseNodeBase(CameraPoseNodeBaseType::Pose),
  _pose(csLie::SE3d::Identity())
{

}
PoseNode::PoseNode(const csLie::SE3d& pose) 
  : 
  CameraPoseNodeBase(CameraPoseNodeBaseType::Pose),
  _pose(pose) 
{

}

PoseNode::~PoseNode() {

}


void PoseNode::setPose(const csLie::SE3d& pose) {
  this->_pose = pose;
}
#include "PointNode.h"

PointNode::PointNode()
    : CameraPosePointNodeBase(CameraPosePointNodeBaseType::Point) {}

PointNode::PointNode(const Eigen::Vector3d& point)
    : CameraPosePointNodeBase(CameraPosePointNodeBaseType::Point)
    , _point(point) {}

PointNode::~PointNode() {}

//-------------------------------------------------

PointXYNode::PointXYNode()
    : CameraPosePointNodeBase(CameraPosePointNodeBaseType::PointXY) {}

PointXYNode::PointXYNode(const Eigen::Isometry3d& T_W_planeOrigin, const Eigen::Vector2d& pointXY)
    : CameraPosePointNodeBase(CameraPosePointNodeBaseType::PointXY)
    , _T_W_planeOrigin(T_W_planeOrigin)
    , _pointXY(pointXY) {}

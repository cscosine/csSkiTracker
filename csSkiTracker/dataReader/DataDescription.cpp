#include "DataDescription.h"

namespace csSkiTracker {
namespace dataReader {

MatchingPoints::MatchingPoints() {}

MatchingPoints::MatchingPoints(int n)
    : imgPoints_view1(2, n)
    , imgPoints_view2(2, n)
    , worldPoints(3, n) {}

CalibPoints::CalibPoints() {}

CalibPoints::CalibPoints(int n)
    : imgPoints(2, n)
    , worldPointsId(n) {}

ProblemData::ProblemData() {}
} // namespace dataReader
} // namespace csSkiTracker

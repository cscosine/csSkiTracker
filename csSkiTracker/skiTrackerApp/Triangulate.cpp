#include "Triangulate.h"

#include "csBlockMatrix/DenseMatrixBlock.hpp"
#include "csNelson/EdgeUnary.hpp"
#include "csNelson/GaussNewton.hpp"
#include "csNelson/SingleSection.hpp"

#include <iostream>

#define DEBUGME \
  if (false)    \
  std::cout <<

Eigen::Vector3d Triangulate::triangulateLinear(const ProjectionMatrix& P1, const Eigen::Vector2d& i1, const ProjectionMatrix& P2,
                                               const Eigen::Vector2d& i2) {

  Eigen::Matrix4d A;

  // first point
  A.row(0) = (P1.row(2) * i1.x() - P1.row(0));
  A.row(1) = (P1.row(2) * i1.y() - P1.row(1));

  // second point
  A.row(2) = (P2.row(2) * i2.x() - P2.row(0));
  A.row(3) = (P2.row(2) * i2.y() - P2.row(1));

  Eigen::JacobiSVD<Eigen::Matrix4d> svd(A, Eigen::ComputeFullV);

  Eigen::Vector4d P = svd.matrixV().col(3);

  P = (P / P.w()).eval();

  return P.head<3>();
}

class SinglePointSection
    : public csNelson::SingleSection<SinglePointSection, Eigen::Vector3d, csBlockMatrix::BlockDense, double, 3, 1> {
  Eigen::Vector3d _point;
  using SingleSection = csNelson::SingleSection<SinglePointSection, Eigen::Vector3d, csBlockMatrix::BlockDense, double, 3, 1>;

public:
  SinglePointSection(const Eigen::Vector3d& point)
      : _point(point) {
    this->parametersReady();
  }

  virtual const Eigen::Vector3d& parameter(csNelson::NodeId i) const {
    assert(i.id() == 0);
    assert(i.type() == csNelson::NodeType::Variable);
    return _point;
  }
  virtual Eigen::Vector3d& parameter(csNelson::NodeId i) {
    assert(i.id() == 0);
    assert(i.type() == csNelson::NodeType::Variable);
    return _point;
  }

  void oplus(const typename SingleSection::HessianVecType& inc) {
    _point += inc.segment(0);
  }

  Eigen::Vector3d point() const {
    return _point;
  }
};

class ReprojErr : public SinglePointSection::EdgeUnary<ReprojErr> {
  Eigen::Vector2d imgPoint;
  ProjectionMatrix Pmat;

  // jacobian
  Eigen::Matrix<double, 2, 3> jacobian;
  Eigen::Vector2d error;

public:
  ReprojErr(const Eigen::Vector2d& imgPoint, const ProjectionMatrix& Pmat)
      : imgPoint(imgPoint)
      , Pmat(Pmat) {}

  virtual ~ReprojErr() {}

  void update(bool hessians) {
    if (hessians) {
      Eigen::Vector2d repr = ProjectionMatrixEstimate::proj3DPoints2img(Pmat, this->parameter(), jacobian);
      error = repr - imgPoint;
    } else {
      Eigen::Vector2d repr = ProjectionMatrixEstimate::proj3DPoints2img(Pmat, this->parameter());
      error = repr - imgPoint;
    }

    this->setChi2(error.transpose() * error);
  }

  template <class Derived1, class Derived2>
  void updateHBlock(Eigen::MatrixBase<Derived1>& H, Eigen::MatrixBase<Derived2>& b) {
    H.noalias() += jacobian.transpose() * jacobian;
    b.noalias() += jacobian.transpose() * error;
  }
};

Eigen::Vector3d Triangulate::triangulateNonLinear(const ProjectionMatrix& P1, const Eigen::Vector2d& i1, const ProjectionMatrix& P2,
                                                  const Eigen::Vector2d& i2, const Eigen::Vector3d& guess) {

  SinglePointSection optProb(guess);
  optProb.addEdge(0, new ReprojErr(i1, P1));
  optProb.addEdge(0, new ReprojErr(i2, P2));

  optProb.structureReady();

  optProb.update(true);
  double chi2 = optProb.hessian().chi2();
  csNelson::GaussNewton<
      typename csNelson::SolverTraits<csNelson::solverCholeskyDense>::Solver<typename SinglePointSection::Hessian::Traits>>
      gn;

  gn.settings().epsBVector = 1e-6;
  gn.settings().epsChi2 = 1e-6;
  gn.settings().epsIncSquare = 1e-6;
  gn.settings().maxNumIt = 20;
  gn.settings().minNumIt = 3;

  auto tc = gn.solve(optProb);

  DEBUGME "--- triangulate ---" << std::endl << gn.stats().toString() << std::endl << std::endl;

  optProb.update(true);
  chi2 = optProb.hessian().chi2();

  return optProb.point();
}

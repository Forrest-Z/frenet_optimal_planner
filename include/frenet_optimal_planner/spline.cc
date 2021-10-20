/** spline.cc
 * 
 * Copyright (C) 2019 SS47816 & Advanced Robotics Center, National University of Singapore & Micron Technology
 * 
 * Class for constructing and solving 2D Splines
*/

#include "spline.h"

namespace fop
{

/****************************************************************************
 * Spline
 ****************************************************************************/
Spline::Spline(const std::vector<double>& x, const std::vector<double>& y)
{
  x_ = x;
  y_ = y;

  num_x_ = x.size();

  // compute the difference between x coordinates
  std::vector<double> h;
  for (int i = 0; i < x.size() - 1; i++)
  {
    h.push_back(x.at(i + 1) - x.at(i));
  }

  // calculate coefficient a
  a_ = y;

  // calculate coefficient c
  Eigen::MatrixXd A = calculateMatrixA(h);
  Eigen::MatrixXd B = calculateMatrixB(h);

  Eigen::MatrixXd Ai = A.inverse();
  Eigen::MatrixXd C = Ai * B;

  for (int i = 0; i < C.size(); i++)
  {
    c_.push_back(C.data()[i]);
  }

  // calculate coefficient b and d
  for (int i = 0; i < num_x_ - 1; i++)
  {
    d_.push_back((c_.at(i + 1) - c_.at(i)) / (3.0 * h.at(i)));
    b_.push_back((a_.at(i + 1) - a_.at(i)) / h.at(i) - h.at(i) * (c_.at(i + 1) + 2.0 * c_.at(i)) / 3.0);
  }
};

double Spline::calculatePoint(double t)
{
  if (t < x_.at(0))
  {
    // std::cout << "condition 1" << std::endl;
    return 0.0;
  }
  else if (t > x_.back())
  {
    // std::cout << "condition 2" << std::endl;
    return 0.0;
  }

  const int i = searchIndex(t);
  const double dx = t - x_.at(i);
  const double result = a_.at(i) + b_.at(i) * dx + c_.at(i) * dx * dx + d_.at(i) * dx * dx * dx;

  return result;
}

double Spline::calculateFirstDerivative(double t)
{
  if (t < x_.at(0))
  {
    return 0.0;
  }
  else if (t > x_.back())
  {
    return 0.0;
  }

  const int i = searchIndex(t);
  const double dx = t - x_.at(i);
  const double result = b_.at(i) + 2.0 * c_.at(i) * dx + 3.0 * d_.at(i) * dx * dx;

  return result;
}

double Spline::calculateSecondDerivative(double t)
{
  if (t < x_.at(0))
  {
    return 0.0;
  }
  else if (t > x_.back())
  {
    return 0.0;
  }

  const int i = searchIndex(t);
  const double dx = t - x_.at(i);
  const double result = 2.0 * c_.at(i) + 6.0 * d_.at(i) * dx;

  return result;
}

Eigen::MatrixXd Spline::calculateMatrixA(std::vector<double> h)
{
  Eigen::MatrixXd A = Eigen::MatrixXd(num_x_, num_x_);
  if (num_x_ == 5)
  {
    A << 1.0, 0.0, 0.0, 0.0, 0.0, h[0], 2.0 * (h[0] + h[1]), h[1], 0.0, 0.0, 0.0, h[1], 2.0 * (h[1] + h[2]), h[2], 0.0,
        0.0, 0.0, h[2], 2.0 * (h[2] + h[3]), h[3], 0.0, 0.0, 0.0, 0.0, 1.0;
  }

  return A;
}

Eigen::MatrixXd Spline::calculateMatrixB(std::vector<double> h)
{
  Eigen::MatrixXd B = Eigen::MatrixXd(num_x_, 1);
  if (num_x_ == 5)
  {
    B << 0.0, 3.0 * (a_[2] - a_[1]) / h[1] - 3.0 * (a_[1] - a_[0]) / h[0],
        3.0 * (a_[3] - a_[2]) / h[2] - 3.0 * (a_[2] - a_[1]) / h[1],
        3.0 * (a_[4] - a_[3]) / h[3] - 3.0 * (a_[3] - a_[2]) / h[2], 0.0;
  }

  return B;
}

int Spline::searchIndex(double x)
{
  int index = 0;
  for (int i = 0; i < x_.size(); i++)
  {
    if (fop::cmp_doubles_greater_or_equal(x, x_.at(i)))
    {
      index = i;
    }
  }

  return index;
}


/****************************************************************************
 * 2D Spline
 ****************************************************************************/
Spline2D::Spline2D(const Map& ref_wps)
{
  s_ = calculate_s(ref_wps);
  sx_ = Spline(s_, ref_wps.x);
  sy_ = Spline(s_, ref_wps.y);
}

VehicleState Spline2D::calculatePosition(double s)
{
  VehicleState state;
  state.x = sx_.calculatePoint(s);
  state.y = sy_.calculatePoint(s);
  return state;
}

double Spline2D::calculateCurvature(double s)
{
  const double dx = sx_.calculateFirstDerivative(s);
  const double ddx = sx_.calculateSecondDerivative(s);
  const double dy = sy_.calculateFirstDerivative(s);
  const double ddy = sy_.calculateSecondDerivative(s);
  const double k = (ddy * dx - ddx * dy) / (dx * dx + dy * dy);

  return k;
}

double Spline2D::calculateYaw(double s)
{
  const double dx = sx_.calculateFirstDerivative(s);
  const double dy = sy_.calculateFirstDerivative(s);
  return atan2(dy, dx);
}

Spline2D::ResultType Spline2D::calculateSplineCourse(const Map& ref_wps, double ds = 0.1)
{
  Spline2D spline2d = Spline2D(ref_wps);

  ResultType result;
  for (double s = 0.0; s < spline2d.s_.back(); s += ds)
  {
    VehicleState state = spline2d.calculatePosition(s);
    result.rx.push_back(state.x);
    result.ry.push_back(state.y);
    result.ryaw.push_back(spline2d.calculateYaw(s));
    result.rk.push_back(spline2d.calculateCurvature(s));
  }

  return result;
}

std::vector<double> Spline2D::calculate_s(const Map& ref_wps)
{
  // store the difference along x and y axis in dx and dy variables
  std::vector<double> dx;
  std::vector<double> dy;
  for (int i = 0; i < ref_wps.x.size() - 1; i++)
  {
    dx.push_back(ref_wps.x.at(i + 1) - ref_wps.x.at(i));
    dy.push_back(ref_wps.y.at(i + 1) - ref_wps.y.at(i));
  }

  // compute the euclidean distances between reference waypoints
  std::vector<double> ds;
  for (int i = 0; i < dx.size(); i++)
  {
    ds.push_back(sqrt(dx.at(i) * dx.at(i) + dy.at(i) * dy.at(i)));
  }

  // compute the cumulative distances from the starting point
  std::vector<double> s;
  s.push_back(0.0);
  for (int i = 0; i < ds.size(); i++)
  {
    s.push_back(s.at(i) + ds.at(i));
  }

  return s;
};

}  // namespace fop
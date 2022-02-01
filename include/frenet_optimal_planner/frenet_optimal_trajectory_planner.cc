/* frenet_optimal_trajectory_planner.cc

  Copyright (C) 2019 SS47816 & Advanced Robotics Center, National University of Singapore & Micron Technology

  Implementation of Optimal trajectory planning in Frenet Coordinate Algorithm
  Using the algorithm described in this paper, https://ieeexplore.ieee.org/document/5509799
*/

#include "frenet_optimal_trajectory_planner.h"

namespace fop
{

FrenetOptimalTrajectoryPlanner::TestResult::TestResult() : count(0)
{
  this->numbers = std::vector<size_t>(5, size_t(0));
  this->numbers_min = std::vector<size_t>(5, size_t(100000));
  this->numbers_max = std::vector<size_t>(5, size_t(0));
  this->total_numbers = std::vector<size_t>(5, size_t(0));

  this->time = std::vector<double>(6, double(0));
  this->time_min = std::vector<double>(6, double(100000));
  this->time_max = std::vector<double>(6, double(0));
  this->total_time = std::vector<double>(6, double(0));

  this->numbers.shrink_to_fit();
  this->numbers_min.shrink_to_fit();
  this->numbers_max.shrink_to_fit();
  this->total_numbers.shrink_to_fit();

  this->time.shrink_to_fit();
  this->time_min.shrink_to_fit();
  this->time_max.shrink_to_fit();
  this->total_time.shrink_to_fit();
}

FrenetOptimalTrajectoryPlanner::TestResult::TestResult(const int length) : length(length), count(0)
{
  this->numbers = std::vector<size_t>(length, size_t(0));
  this->numbers_min = std::vector<size_t>(length, size_t(100000));
  this->numbers_max = std::vector<size_t>(length, size_t(0));
  this->total_numbers = std::vector<size_t>(length, size_t(0));

  this->time = std::vector<double>(length+1, double(0));
  this->time_min = std::vector<double>(length+1, double(100000));
  this->time_max = std::vector<double>(length+1, double(0));
  this->total_time = std::vector<double>(length+1, double(0));

  this->numbers.shrink_to_fit();
  this->numbers_min.shrink_to_fit();
  this->numbers_max.shrink_to_fit();
  this->total_numbers.shrink_to_fit();

  this->time.shrink_to_fit();
  this->time_min.shrink_to_fit();
  this->time_max.shrink_to_fit();
  this->total_time.shrink_to_fit();
}

void FrenetOptimalTrajectoryPlanner::TestResult::updateCount(const std::vector<size_t> numbers, const std::vector<std::chrono::_V2::system_clock::time_point> timestamps)
{
  if (numbers.size() != this->length || timestamps.size() != this->length+1)
  {
    std::cout << "Recorded TestResult for this planning iteration is invalid" << std::endl;
    return;
  }
  
  this->count++;

  // Update the numbers for the current iteration
  this->numbers = numbers;
  for (size_t i = 0; i < this->length; ++i)
  {
    this->numbers_min[i] = std::min(this->numbers_min[i], numbers[i]);
    this->numbers_max[i] = std::max(this->numbers_max[i], numbers[i]);
  }
  // Add the current numbers to total numbers
  std::transform(this->total_numbers.begin(), this->total_numbers.end(), numbers.begin(), this->total_numbers.begin(), std::plus<size_t>());
  
  // Calculate the elapsed_time for the current iteration, in [ms]
  for (int i = 0; i < timestamps.size() - 1; i++)
  {
    const std::chrono::duration<double, std::milli> elapsed_time = timestamps[i+1] - timestamps[i];
    this->time[i] = elapsed_time.count();
  }
  const std::chrono::duration<double, std::milli> elapsed_time = timestamps.back() - timestamps.front();
  this->time[this->length] = elapsed_time.count();

  // Update the time for the current iteration
  for (size_t i = 0; i < this->length; ++i)
  {
    this->time_min[i] = std::min(this->time_min[i], this->time[i]);
    this->time_max[i] = std::max(this->time_max[i], this->time[i]);
  }

  // Add the current elapsed_time to total time, in [ms]
  std::transform(this->total_time.begin(), this->total_time.end(), this->time.begin(), this->total_time.begin(), std::plus<double>());
}

void FrenetOptimalTrajectoryPlanner::TestResult::printSummary()
{
  // Print Summary for this iteration
  std::cout << " " << std::endl;
  std::cout << "Summary: This Planning Iteration (iteration no." << this->count << ")" << std::endl;
  std::cout << "Step 1 : Predicted               " << this->numbers[0] << " Trajectories in " << this->time[0] << " ms" << std::endl;
  std::cout << "Step 2 : Generated               " << this->numbers[1] << " End States   in " << this->time[1] << " ms" << std::endl;
  std::cout << "Step 3 : Generated & Evaluated   " << this->numbers[2] << " Trajectories in " << this->time[2] << " ms" << std::endl;
  std::cout << "Step 4 : Validated               " << this->numbers[3] << " Trajectories in " << this->time[3] << " ms" << std::endl;
  std::cout << "Step 5 : Checked Collisions for  " << this->numbers[4] << " PolygonPairs in " << this->time[4] << " ms" << std::endl;
  std::cout << "Total  : Planning Took           " << this->time[5] << " ms (or " << 1000/this->time[5] << " Hz)" << std::endl;

  // Print Summary for Best Case performance
  std::cout << " " << std::endl;
  std::cout << "Summary: Best Case Performance  (" << this->count << " iterations so far)" << std::endl;
  std::cout << "Step 1 : Predicted               " << this->numbers_min[0]/this->count << " Trajectories in " << this->time_min[0] << " ms" << std::endl;
  std::cout << "Step 2 : Generated               " << this->numbers_min[1]/this->count << " End States   in " << this->time_min[1] << " ms" << std::endl;
  std::cout << "Step 3 : Generated & Evaluated   " << this->numbers_min[2]/this->count << " Trajectories in " << this->time_min[2] << " ms" << std::endl;
  std::cout << "Step 4 : Validated               " << this->numbers_min[3]/this->count << " Trajectories in " << this->time_min[3] << " ms" << std::endl;
  std::cout << "Step 5 : Checked Collisions for  " << this->numbers_min[4]/this->count << " PolygonPairs in " << this->time_min[4] << " ms" << std::endl;
  std::cout << "Total  : Planning Took           " << this->time_min[5] << " ms (or " << 1000/this->time_min[5] << " Hz)" << std::endl;

  // Print Summary for Worst Case performance
  std::cout << " " << std::endl;
  std::cout << "Summary: Worst Case Performance (" << this->count << " iterations so far)" << std::endl;
  std::cout << "Step 1 : Predicted               " << this->numbers_max[0]/this->count << " Trajectories in " << this->time_max[0] << " ms" << std::endl;
  std::cout << "Step 2 : Generated               " << this->numbers_max[1]/this->count << " End States   in " << this->time_max[1] << " ms" << std::endl;
  std::cout << "Step 3 : Generated & Evaluated   " << this->numbers_max[2]/this->count << " Trajectories in " << this->time_max[2] << " ms" << std::endl;
  std::cout << "Step 4 : Validated               " << this->numbers_max[3]/this->count << " Trajectories in " << this->time_max[3] << " ms" << std::endl;
  std::cout << "Step 5 : Checked Collisions for  " << this->numbers_max[4]/this->count << " PolygonPairs in " << this->time_max[4] << " ms" << std::endl;
  std::cout << "Total  : Planning Took           " << this->time_max[5] << " ms (or " << 1000/this->time_max[5] << " Hz)" << std::endl;

  // Print Summary for average performance
  std::cout << " " << std::endl;
  std::cout << "Summary: Average Performance (" << this->count << " iterations so far)" << std::endl;
  std::cout << "Step 1 : Predicted               " << this->total_numbers[0]/this->count << " Trajectories in " << this->total_time[0]/this->count << " ms" << std::endl;
  std::cout << "Step 2 : Generated               " << this->total_numbers[1]/this->count << " End States   in " << this->total_time[1]/this->count << " ms" << std::endl;
  std::cout << "Step 3 : Generated & Evaluated   " << this->total_numbers[2]/this->count << " Trajectories in " << this->total_time[2]/this->count << " ms" << std::endl;
  std::cout << "Step 4 : Validated               " << this->total_numbers[3]/this->count << " Trajectories in " << this->total_time[3]/this->count << " ms" << std::endl;
  std::cout << "Step 5 : Checked Collisions for  " << this->total_numbers[4]/this->count << " PolygonPairs in " << this->total_time[4]/this->count << " ms" << std::endl;
  std::cout << "Total  : Planning Took           " << this->total_time[5]/this->count << " ms (or " << 1000/(this->total_time[5]/this->count) << " Hz)" << std::endl;
}

FrenetOptimalTrajectoryPlanner::FrenetOptimalTrajectoryPlanner()
{
  this->settings_ = Setting();
  this->test_result_ = TestResult(5);
}

FrenetOptimalTrajectoryPlanner::FrenetOptimalTrajectoryPlanner(const Setting& settings)
{
  this->settings_ = settings;
  this->test_result_ = TestResult(5);
}

void FrenetOptimalTrajectoryPlanner::updateSettings(const Setting& settings)
{
  this->settings_ = settings;
}

std::pair<Path, Spline2D> FrenetOptimalTrajectoryPlanner::generateReferenceCurve(const Lane& lane)
{
  Path ref_path = Path();
  auto cubic_spline = Spline2D(lane);

  std::vector<double> s;
  for (double i = 0; i < cubic_spline.s_.back(); i += 0.1)
  {
    s.emplace_back(i);
  }

  for (int i = 0; i < s.size(); i++)
  {
    VehicleState state = cubic_spline.calculatePosition(s[i]);
    ref_path.x.emplace_back(state.x);
    ref_path.y.emplace_back(state.y);
    ref_path.yaw.emplace_back(cubic_spline.calculateYaw(s[i]));
  }

  return std::pair<Path, Spline2D>{ref_path, cubic_spline};
}

std::vector<FrenetPath> 
FrenetOptimalTrajectoryPlanner::frenetOptimalPlanning(Spline2D& cubic_spline, const FrenetState& start_state, const int lane_id,
                                                      const double left_width, const double right_width, const double current_speed, 
                                                      const autoware_msgs::DetectedObjectArray& obstacles, const bool check_collision, const bool use_async)
{
  // Initialize a series of results to be recorded
  std::vector<size_t> numbers;
  std::vector<std::chrono::_V2::system_clock::time_point> timestamps;
  timestamps.emplace_back(std::chrono::high_resolution_clock::now());

  /* --------------------------------- Construction Zone -------------------------------- */
  
  // Initialize start state and obstacle trajectories
  start_state_ = start_state;
  const auto obstacle_trajs = predictTrajectories(obstacles);
  numbers.emplace_back(obstacle_trajs.size());
  timestamps.emplace_back(std::chrono::high_resolution_clock::now());
  
  // Sample all the end states in 3 dimensions, [d, v, t] and form the 3d traj candidate array
  auto result = sampleEndStates(lane_id, left_width, right_width, current_speed);
  auto trajs_3d = std::move(result.first);
  auto best_idx = std::move(result.second);
  numbers.emplace_back(trajs_3d.size()*trajs_3d[0].size()*trajs_3d[0][0].size());
  timestamps.emplace_back(std::chrono::high_resolution_clock::now());

  // ################################ Search Process #####################################
  size_t num_iter = 0;
  size_t num_trajs_generated = 0;
  bool converged = false;
  while (!converged)
  {
    // std::cout << "FOP: Search iteration " << num_iter << " convergence: " << converged << std::endl;
    // std::cout << "FOP: Current idx " << best_idx(0) << best_idx(1) << best_idx(2) << std::endl;

    // Perform a search for the real best trajectory using gradient descent
    converged = findNextBest(trajs_3d, best_idx, num_trajs_generated);
    num_iter++;
  }
  std::cout << "FOP: Search Done in " << num_iter << " iterations" << std::endl;
  numbers.emplace_back(num_trajs_generated);
  timestamps.emplace_back(std::chrono::high_resolution_clock::now());

  // ################################ Validation Process #####################################
  size_t num_trajs_validated = 0;
  size_t num_collision_checks = 0;
  FrenetPath best_traj = FrenetPath();
  bool best_traj_found = false;
  while (!best_traj_found && !candidate_trajs_.empty())
  {
    num_trajs_validated++;
    auto candidate_traj = candidate_trajs_.top();
    candidate_trajs_.pop();
    
    // Convert to the global frame
    convertToGlobalFrame(candidate_traj, cubic_spline);
    // Check for constraints
    bool is_safe = checkConstraints(candidate_traj);
    if (!is_safe)
    {
      continue;
    }
    else
    {
      // Check for collisions
      if (check_collision)
      {
        is_safe = checkCollisions(candidate_traj, obstacle_trajs, obstacles, use_async, num_collision_checks);
      }
      else
      {
        std::cout << "Collision Checking Skipped" << std::endl;
        is_safe = true;
      }
    }
    
    if (is_safe)
    {
      best_traj_found = true;
      best_traj = std::move(candidate_traj);
      std::cout << "FOP: Best Traj Found" << std::endl;
    }
  }
  numbers.emplace_back(num_trajs_validated);
  timestamps.emplace_back(std::chrono::high_resolution_clock::now());
  numbers.emplace_back(num_collision_checks);
  timestamps.emplace_back(std::chrono::high_resolution_clock::now());
  test_result_.updateCount(numbers, timestamps);
  test_result_.printSummary();
  /* --------------------------------- Construction Zone -------------------------------- */

  if (best_traj_found)
  {
    return std::vector<FrenetPath>{1, best_traj};
  }
  else
  {
    return std::vector<FrenetPath>{};
  }
}

std::pair<std::vector<std::vector<std::vector<FrenetPath>>>, Eigen::Vector3i> 
FrenetOptimalTrajectoryPlanner::sampleEndStates(const int lane_id, const double left_bound, const double right_bound, const double current_speed)
{
  // list of frenet end states sampled
  std::vector<std::vector<std::vector<FrenetPath>>> trajs_3d;

  double min_cost = std::numeric_limits<double>::max();
  Eigen::Vector3i idx;
  
  // Sampling on the lateral direction
  const double delta_width = (left_bound - settings_.center_offset)/((settings_.num_width - 1)/2);
  for (int i = 0; i < settings_.num_width; i++)  // left being positive
  {
    std::vector<std::vector<FrenetPath>> trajs_2d;
    const double d = right_bound + i*delta_width;
    const double lat_norm = std::max(std::pow(left_bound - settings_.center_offset, 2), std::pow(right_bound - settings_.center_offset, 2));
    const double lat_cost = std::pow(d - settings_.center_offset, 2)/lat_norm;

    // Sampling on the longitudial direction
    const double delta_v = (settings_.highest_speed - settings_.lowest_speed)/(settings_.num_speed - 1);
    for (int j = 0; j < settings_.num_speed; j++)
    {
      std::vector<FrenetPath> trajs_1d;
      const double v = settings_.lowest_speed + j*delta_v;
      const double speed_cost = pow(settings_.highest_speed - v, 2) + 0.5*pow(current_speed - v, 2);

      // Sampling on the time dimension
      const double delta_t = (settings_.max_t - settings_.min_t)/(settings_.num_t - 1);
      for (int k = 0; k < settings_.num_t; k++)
      {
        FrenetState end_state;
        // end time
        end_state.T = settings_.min_t + k*delta_t;
        // end longitudial state [s, s_d, s_dd]
        end_state.s = 0.0;  // TBD later by polynomial
        end_state.s_d = v;
        end_state.s_dd = 0.0;
        // end lateral state [d, d_d, d_dd]
        end_state.d = d;
        end_state.d_d = 0.0;
        end_state.d_dd = 0.0;

        // Planning Horizon cost (encourage longer planning horizon)
        const double time_cost = (1 - end_state.T/settings_.max_t);
        
        // fixed cost terms
        const double fix_cost = settings_.k_lat * settings_.k_diff*lat_cost 
                              + settings_.k_lon * (settings_.k_time*time_cost + settings_.k_diff*speed_cost);
        // estimated huristic cost terms
        const double hur_cost = settings_.k_lat * settings_.k_diff*pow(start_state_.d - end_state.d, 2);
        // total estimated cost
        const double est_cost = fix_cost + hur_cost;

        // find the index of the traj with the lowest estimated cost
        if (est_cost < min_cost)
        {
          min_cost = est_cost;
          idx(0) = i;
          idx(1) = j;
          idx(2) = k;
        }

        trajs_1d.emplace_back(FrenetPath(lane_id, end_state, fix_cost, hur_cost));
      }

      trajs_2d.emplace_back(trajs_1d);
    }

    trajs_3d.emplace_back(trajs_2d);
  }

  return std::pair<std::vector<std::vector<std::vector<FrenetPath>>>, Eigen::Vector3i>(std::move(trajs_3d), idx);
}

bool FrenetOptimalTrajectoryPlanner::findNextBest(std::vector<std::vector<std::vector<FrenetPath>>>& trajs, Eigen::Vector3i& idx, size_t& num_traj)
{
  if (trajs[idx(0)][idx(1)][idx(2)].is_used)
  {
    // std::cout << "FNB: At Current idx " << idx(0) << idx(1) << idx(2) << " converged" << std::endl;
    return true; // converged
  }
  else
  {
    // std::cout << "FNB: At Current idx " << idx(0) << idx(1) << idx(2) << " not converged" << std::endl;
    trajs[idx(0)][idx(1)][idx(2)].is_used = true; // label this traj as searched
    const auto gradients = findGradients(trajs, idx, num_traj);

    int grad_dim = 0;
    double max_grad = gradients(0);
    for (int i = 1; i < 3; i++)
    {
      if (std::abs(gradients(i)) > std::abs(max_grad))
      {
        grad_dim = i;
        max_grad = gradients(i);
      }
    }

    idx(grad_dim) += max_grad > 0? -1 : 1; // move in the max gradient direction, towards lower cost
    
    return false; // not converged
  }
}

Eigen::Vector3d FrenetOptimalTrajectoryPlanner::findGradients(std::vector<std::vector<std::vector<FrenetPath>>>& trajs, const Eigen::Vector3i& idx, size_t& num_traj)
{
  const Eigen::Vector3i sizes = {int(trajs.size()), int(trajs[0].size()), int(trajs[0][0].size())};
  const Eigen::Vector3i directions = findDirection(sizes, idx);

  // Center sample location which we want to find the gradient
  const double cost_center = getTrajAndRealCost(trajs, idx, num_traj);

  // Compute the gradients at each direction
  Eigen::Vector3d gradients;
  for (int dim = 0; dim < 3; dim++)
  {
    Eigen::Vector3i next_idx = idx;
    next_idx(dim) += directions(dim);
    if (directions(dim) >= 0) // the right side has neighbor
    {
      gradients(dim) = getTrajAndRealCost(trajs, next_idx, num_traj) - cost_center;
      if (gradients(dim) >= 0 && idx(dim) == 0) // the right neighbor has higher cost and there is no neighbor on the left side
      {
        gradients(dim) = 0.0; // set the gradient to zero
      }
    }
    else // the right side has no neighbor, calculate gradient using the left neighbor
    {
      gradients(dim) = cost_center - getTrajAndRealCost(trajs, next_idx, num_traj);
      if (gradients(dim) <= 0 && idx(dim) == sizes(dim)-1) // the left neighbor has higher cost and there is no neighbor on the right side
      {
        gradients(dim) = 0.0; // set the gradient to zero
      }
    }
  }

  return gradients;
}

Eigen::Vector3i FrenetOptimalTrajectoryPlanner::findDirection(const Eigen::Vector3i& sizes, const Eigen::Vector3i& idx)
{
  Eigen::Vector3i directions;
  for (int dim = 0; dim < 3; dim++)
  {
    if (idx(dim) >= sizes(dim)-1)
    {
      directions(dim) = -1;
    }
    else
    {
      directions(dim) = +1;
    }
  }

  return directions;
}

double FrenetOptimalTrajectoryPlanner::getTrajAndRealCost(std::vector<std::vector<std::vector<FrenetPath>>>& trajs, const Eigen::Vector3i& idx, size_t& num_traj)
{
  const int i = idx(0);  // width dimension
  const int j = idx(1);  // speed dimension
  const int k = idx(2);  // time  dimension
  
  if (trajs[i][j][k].is_generated)
  {
    return trajs[i][j][k].final_cost;
  }
  else
  {
    num_traj++;
    trajs[i][j][k].is_generated = true;
    
    // calculate the costs
    double jerk_s = 0.0;
    double jerk_d = 0.0;

    // generate lateral quintic polynomial
    QuinticPolynomial lateral_quintic_poly = QuinticPolynomial(start_state_, trajs[i][j][k].end_state);

    // store the this lateral trajectory into traj
    for (double t = 0.0; t <= trajs[i][j][k].end_state.T; t += settings_.tick_t)
    {
      trajs[i][j][k].t.emplace_back(t);
      trajs[i][j][k].d.emplace_back(lateral_quintic_poly.calculatePoint(t));
      trajs[i][j][k].d_d.emplace_back(lateral_quintic_poly.calculateFirstDerivative(t));
      trajs[i][j][k].d_dd.emplace_back(lateral_quintic_poly.calculateSecondDerivative(t));
      trajs[i][j][k].d_ddd.emplace_back(lateral_quintic_poly.calculateThirdDerivative(t));
      jerk_d += std::pow(trajs[i][j][k].d_ddd.back(), 2);
    }

    // generate longitudinal quartic polynomial
    QuarticPolynomial longitudinal_quartic_poly = QuarticPolynomial(start_state_, trajs[i][j][k].end_state);

    // store the this longitudinal trajectory into traj
    for (double t = 0.0; t <= trajs[i][j][k].end_state.T; t += settings_.tick_t)
    {
      trajs[i][j][k].s.emplace_back(longitudinal_quartic_poly.calculatePoint(t));
      trajs[i][j][k].s_d.emplace_back(longitudinal_quartic_poly.calculateFirstDerivative(t));
      trajs[i][j][k].s_dd.emplace_back(longitudinal_quartic_poly.calculateSecondDerivative(t));
      trajs[i][j][k].s_ddd.emplace_back(longitudinal_quartic_poly.calculateThirdDerivative(t));
      jerk_s += std::pow(trajs[i][j][k].s_ddd.back(), 2);
    }
    
    trajs[i][j][k].dyn_cost = settings_.k_jerk * (settings_.k_lon * jerk_s + settings_.k_lat * jerk_d);
    trajs[i][j][k].final_cost = trajs[i][j][k].fix_cost + trajs[i][j][k].dyn_cost;

    candidate_trajs_.push(trajs[i][j][k]);

    return trajs[i][j][k].final_cost;
  }
}

void FrenetOptimalTrajectoryPlanner::convertToGlobalFrame(FrenetPath& traj, Spline2D& cubic_spline)
{
  // calculate global positions
  for (int j = 0; j < traj.s.size(); j++)
  {
    VehicleState state = cubic_spline.calculatePosition(traj.s[j]);
    double i_yaw = cubic_spline.calculateYaw(traj.s[j]);
    const double di = traj.d[j];
    const double frenet_x = state.x + di * cos(i_yaw + M_PI / 2.0);
    const double frenet_y = state.y + di * sin(i_yaw + M_PI / 2.0);
    if (!isLegal(frenet_x) || !isLegal(frenet_y))
    {
      break;
    }
    else
    {
      traj.x.emplace_back(frenet_x);
      traj.y.emplace_back(frenet_y);
    }
  }
  // calculate yaw and ds
  for (int j = 0; j < traj.x.size() - 1; j++)
  {
    const double dx = traj.x[j+1] - traj.x[j];
    const double dy = traj.y[j+1] - traj.y[j];
    traj.yaw.emplace_back(atan2(dy, dx));
    traj.ds.emplace_back(sqrt(dx * dx + dy * dy));
  }

  traj.yaw.emplace_back(traj.yaw.back());
  traj.ds.emplace_back(traj.ds.back());

  // calculate curvature
  for (int j = 0; j < traj.yaw.size() - 1; j++)
  {
    double yaw_diff = unifyAngleRange(traj.yaw[j+1] - traj.yaw[j]);
    traj.c.emplace_back(yaw_diff / traj.ds[j]);
  }
}

/**
 * @brief Checks whether frenet paths are safe to execute based on constraints 
 * @param traj the trajectory to be checked
 * @return true if trajectory satisfies constraints
 */
bool FrenetOptimalTrajectoryPlanner::checkConstraints(FrenetPath& traj)
{
  bool passed = true;
  for (int i = 0; i < traj.c.size(); i++)
  {
    if (!std::isnormal(traj.x[i]) || !std::isnormal(traj.y[i]))
    {
      passed = false;
      // std::cout << "Condition 0: Contains ilegal values" << std::endl;
      break;
    }
    else if (traj.s_d[i] > settings_.max_speed)
    {
      passed = false;
      // std::cout << "Condition 1: Exceeded Max Speed" << std::endl;
      break;
    }
    else if (traj.s_dd[i] > settings_.max_accel || traj.s_dd[i] < settings_.max_decel)
    {
      passed = false;
      // std::cout << "Condition 2: Exceeded Max Acceleration" << std::endl;
      break;
    }
    else if (std::abs(traj.c[i]) > settings_.max_curvature)
    {
      passed = false;
      // std::cout << "Exceeded max curvature = " << settings_.max_curvature
      //           << ". Curr curvature = " << (traj.c[i]) << std::endl;
      break;
    }
  }

  traj.constraint_passed = passed;
  return passed;
}

std::vector<Path> FrenetOptimalTrajectoryPlanner::predictTrajectories(const autoware_msgs::DetectedObjectArray& obstacles)
{
  std::vector<Path> obstacle_trajs;

  for (const auto& obstacle : obstacles.objects)
  {
    Path obstacle_traj;

    obstacle_traj.x.push_back(obstacle.pose.position.x);
    obstacle_traj.y.push_back(obstacle.pose.position.y);
    tf2::Quaternion q_tf2(obstacle.pose.orientation.x, obstacle.pose.orientation.y,
                          obstacle.pose.orientation.z, obstacle.pose.orientation.w);
    tf2::Matrix3x3 m(q_tf2.normalize());
    double roll, pitch, yaw;
    m.getRPY(roll, pitch, yaw);
    obstacle_traj.yaw.push_back(yaw);
    const double v = magnitude(obstacle.velocity.linear.x, obstacle.velocity.linear.y, obstacle.velocity.linear.z);
    obstacle_traj.v.push_back(v);
    
    const int steps = settings_.max_t/settings_.tick_t;
    for (size_t i = 0; i < steps; i++)
    {
      obstacle_traj.x.push_back(obstacle_traj.x.back() + v*settings_.tick_t*std::cos(yaw));
      obstacle_traj.x.push_back(obstacle_traj.y.back() + v*settings_.tick_t*std::sin(yaw));
      obstacle_traj.yaw.push_back(yaw);
      obstacle_traj.v.push_back(v);
    }

    obstacle_trajs.emplace_back(obstacle_traj);
  }

  return obstacle_trajs;
}

bool FrenetOptimalTrajectoryPlanner::checkCollisions(FrenetPath& ego_traj, const std::vector<Path>& obstacle_trajs, 
                                                     const autoware_msgs::DetectedObjectArray& obstacles, 
                                                     const bool use_async, size_t& num_checks)
{
  if (use_async)
  {
    std::future<std::pair<bool, int>> collision_check = std::async(std::launch::async, &FrenetOptimalTrajectoryPlanner::checkTrajCollision, this, 
                                                                   ego_traj, obstacle_trajs, obstacles, settings_.safety_margin_lon, settings_.safety_margin_lat);
    const auto result = collision_check.get();
    ego_traj.collision_passed = result.first;
    num_checks += result.second;
  }
  else
  {
    const auto result = checkTrajCollision(ego_traj, obstacle_trajs, obstacles, settings_.safety_margin_lon, settings_.safety_margin_lat);
    ego_traj.collision_passed = result.first;
    num_checks += result.second;
  }

  return ego_traj.collision_passed;
}

/**
 * @brief Check for collisions at each point along a frenet path
 *
 * @param ego_traj the path to check
 * @param obstacles obstacles to check against for collision
 * @param margin collision margin in [m]
 * @return false if there is a collision along the path. Otherwise, true
 */
std::pair<bool, int> FrenetOptimalTrajectoryPlanner::checkTrajCollision(const FrenetPath& ego_traj, const std::vector<Path>& obstacle_trajs,
                                                                        const autoware_msgs::DetectedObjectArray& obstacles,
                                                                        const double margin_lon, const double margin_lat)
{
  int num_checks = 0;
  geometry_msgs::Polygon ego_rect, obstacle_rect;
  for (int i = 0; i < obstacles.objects.size(); i++)
  {
    const int num_steps = ego_traj.x.size();
    // Check for collisions between ego and obstacle trajectories
    for (int j = 0; j < num_steps; j++)
    {
      num_checks++;
      const double vehicle_center_x = ego_traj.x[j] + Vehicle::Lr() * cos(ego_traj.yaw[j]);
      const double vehicle_center_y = ego_traj.y[j] + Vehicle::Lr() * sin(ego_traj.yaw[j]);

      ego_rect = sat_collision_checker_.construct_rectangle(vehicle_center_x, vehicle_center_y, ego_traj.yaw[j], 
                                                            settings_.vehicle_length, settings_.vehicle_width, 0.0, 0.0);
      obstacle_rect = sat_collision_checker_.construct_rectangle(obstacle_trajs[i].x[j], obstacle_trajs[i].y[j], obstacle_trajs[i].yaw[j], 
                                                                 obstacles.objects[i].dimensions.x, obstacles.objects[i].dimensions.y, margin_lon, margin_lat);

      if (sat_collision_checker_.check_collision(ego_rect, obstacle_rect))
      {
        return std::pair<bool, int>{false, num_checks};
      }
    }
  }

  return std::pair<bool, int>{true, num_checks};
}

}  // namespace fop
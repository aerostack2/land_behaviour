/*!*******************************************************************************************
 *  \file       land_base.hpp
 *  \brief      Base class for land plugins header
 *  \authors    Miguel Fernández Cortizas
 *              Pedro Arias Pérez
 *              David Pérez Saura
 *              Rafael Pérez Seguí
 *
 *  \copyright  Copyright (c) 2022 Universidad Politécnica de Madrid
 *              All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************************/

#ifndef LAND_BASE_HPP
#define LAND_BASE_HPP

#include <rclcpp_action/rclcpp_action.hpp>

#include "as2_behavior/behavior_server.hpp"
#include "as2_core/names/actions.hpp"
#include "as2_core/names/topics.hpp"
#include "as2_core/node.hpp"
#include "as2_core/utils/frame_utils.hpp"
#include "as2_core/utils/tf_utils.hpp"
#include "as2_msgs/action/land.hpp"
#include "as2_msgs/msg/platform_info.hpp"
#include "as2_msgs/msg/platform_status.hpp"
#include "motion_reference_handlers/hover_motion.hpp"

#include <Eigen/Dense>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>

namespace land_base {

class LandBase {
public:
  using GoalHandleLand = rclcpp_action::ServerGoalHandle<as2_msgs::action::Land>;

  LandBase(){};
  virtual ~LandBase(){};

  void initialize(as2::Node *node_ptr, const std::shared_ptr<as2::tf::TfHandler> tf_handler) {
    node_ptr_             = node_ptr;
    hover_motion_handler_ = std::make_shared<as2::motionReferenceHandlers::HoverMotion>(node_ptr_);
    this->ownInit();
  }

  virtual void state_callback(geometry_msgs::msg::PoseStamped &pose_msg,
                              geometry_msgs::msg::TwistStamped &twist_msg) {
    actual_pose_ = pose_msg;

    feedback_.actual_land_height = actual_pose_.pose.position.z;
    feedback_.actual_land_speed  = twist_msg.twist.linear.z;

    localization_received_ = true;
    return;
  }

  virtual bool on_deactivate(const std::shared_ptr<std::string> &message) = 0;
  virtual bool on_pause(const std::shared_ptr<std::string> &message)      = 0;
  virtual bool on_resume(const std::shared_ptr<std::string> &message)     = 0;

  virtual bool on_activate(std::shared_ptr<const as2_msgs::action::Land::Goal> goal) {
    if (!localization_received_) {
      RCLCPP_ERROR(node_ptr_->get_logger(), "Behavior reject, there is no localization");
      return false;
    }

    if (own_activate(goal)) {
      goal_ = *goal;
      return true;
    }
    return false;
  }

  virtual bool on_modify(std::shared_ptr<const as2_msgs::action::Land::Goal> goal) {
    if (!localization_received_) {
      RCLCPP_ERROR(node_ptr_->get_logger(), "Behavior reject, there is no localization");
      return false;
    }

    if (own_activate(goal)) {
      goal_ = *goal;
      return true;
    }
    return false;
  }

  virtual as2_behavior::ExecutionStatus on_run(
      const std::shared_ptr<const as2_msgs::action::Land::Goal> goal,
      std::shared_ptr<as2_msgs::action::Land::Feedback> &feedback_msg,
      std::shared_ptr<as2_msgs::action::Land::Result> &result_msg) {
    as2_behavior::ExecutionStatus status = own_run();

    feedback_msg = std::make_shared<as2_msgs::action::Land::Feedback>(feedback_);
    result_msg   = std::make_shared<as2_msgs::action::Land::Result>(result_);
    return status;
  }

  virtual void on_excution_end(const as2_behavior::ExecutionStatus &state) {
    localization_received_ = false;
    own_execution_end(state);
    return;
  }

protected:
  virtual void ownInit(){};

  virtual bool own_activate(std::shared_ptr<const as2_msgs::action::Land::Goal> goal) {
    return true;
  }

  virtual as2_behavior::ExecutionStatus own_run() = 0;

  virtual bool own_modify(std::shared_ptr<const as2_msgs::action::Land::Goal> goal) { return true; }

  virtual void own_execution_end(const as2_behavior::ExecutionStatus &state) = 0;

  inline void sendHover() {
    hover_motion_handler_->sendHover();
    return;
  };

protected:
  as2::Node *node_ptr_;

  as2_msgs::action::Land::Goal goal_;
  as2_msgs::action::Land::Feedback feedback_;
  as2_msgs::action::Land::Result result_;

  geometry_msgs::msg::PoseStamped actual_pose_;
  bool localization_received_ = false;

  std::shared_ptr<as2::motionReferenceHandlers::HoverMotion> hover_motion_handler_ = nullptr;
};  // class LandBase
}  // namespace land_base
#endif  // LAND_BASE_HPP

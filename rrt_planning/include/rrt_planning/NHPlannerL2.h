#ifndef INCLUDE_NHPLANNERL2_H_
#define INCLUDE_NHPLANNERL2_H_

#include <ros/ros.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <costmap_2d/costmap_2d.h>
#include <nav_core/base_global_planner.h>
#include <geometry_msgs/PoseStamped.h>

#include <Eigen/Dense>

#include "rrt_planning/distance/Distance.h"
#include "rrt_planning/extenders/ExtenderFactory.h"
#include "rrt_planning/visualization/Visualizer.h"
#include "rrt_planning/sampling/position/SamplingPositionFactory.h"
#include "rrt_planning/sampling/angle/SamplingAngleFactory.h"
#include "rrt_planning/nh/CornerIndex.h"
#include "rrt_planning/nh/OpenList.h"
#include "rrt_planning/map/SGMap.h"
#include "rrt_planning/AbstractPlanner.h"


namespace rrt_planning
{

class NHPlannerL2 : public AbstractPlanner
{
public:
    NHPlannerL2();
    NHPlannerL2(std::string name, costmap_2d::Costmap2DROS* costmap_ros);
    NHPlannerL2(std::string name, costmap_2d::Costmap2DROS* costmap_ros, std::chrono::duration<double> t);

    void initialize(std::string name, costmap_2d::Costmap2DROS* costmap_ros) override;
    bool makePlan(const geometry_msgs::PoseStamped& start,
                  const geometry_msgs::PoseStamped& goal,
                  std::vector<geometry_msgs::PoseStamped>& plan) override;

    virtual ~NHPlannerL2();

private:
    Node* reach(Node* current, const Eigen::VectorXd& xCorner);
    bool isReached(const Eigen::VectorXd& x0, const Eigen::VectorXd& xTarget);

    Eigen::VectorXd convertPose(const geometry_msgs::PoseStamped& pose);

    void publishPlan(std::vector<Eigen::VectorXd>& path, std::vector<geometry_msgs::PoseStamped>& plan,
                     const ros::Time& stamp);

    void addOpen(Node* node, const Action& action, Distance& distance);
    void addSubgoal(Node* node, const Action& action, Distance& distance);
    std::vector<Action> findAction(Node* node, const Action& action, Distance& distance, std::vector<Triangle>& triangles);
    std::vector<Eigen::VectorXd> retrievePath(Node* node);

    void sampleCorner(const Eigen::VectorXd& corner, bool cw);
    double sampleAngle(double theta);

    void addGlobal(const Eigen::VectorXd& node, const Eigen::VectorXd& action, const Eigen::VectorXd& parent);
    bool insideGlobal(const Eigen::VectorXd& p, bool subgoal);
    Triangle createTriangle(const Action& a, const Eigen::VectorXd& n);

private:
    Map* rosmap;
    SGMap* map;
    Distance* l2dis;
    Distance* l2thetadis;
    Distance* thetadis;

    Action target;
    OpenList open;
    std::map<Eigen::VectorXd, Node*, rrt_planning::CmpReached> reached;
    std::map<Eigen::Vector2d, std::vector<Eigen::VectorXd>, CmpReached> corner_samples;
    std::vector<Triangle> global_closed;

    double deltaX;
    double deltaTheta;
    int count;
    int k;
    int k_ancestors;

    ExtenderFactory extenderFactory;
    Visualizer visualizer;
    SamplingAngleFactory angleFactory;
    SamplingPositionFactory positionFactory;

};

}



#endif /* INCLUDE_NHPLANNERL2_H_ */

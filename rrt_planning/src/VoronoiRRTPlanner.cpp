#include <pluginlib/class_list_macros.h>
#include <visualization_msgs/Marker.h>

#include "rrt_planning/VoronoiRRTPlanner.h"

#include "rrt_planning/extenders/MotionPrimitivesExtender.h"
#include "rrt_planning/map/ROSMap.h"
#include "rrt_planning/kinematics_models/DifferentialDrive.h"
#include "rrt_planning/utils/RandomGenerator.h"
#include "rrt_planning/rrt/RRT.h"

using namespace Eigen;
using namespace voronoi_planner;

//Register the planner
PLUGINLIB_EXPORT_CLASS(rrt_planning::VoronoiRRTPlanner, nav_core::BaseGlobalPlanner)

using namespace std;

namespace rrt_planning
{
    VoronoiRRTPlanner::VoronoiRRTPlanner(){

        K = 0;
        deltaX = 0;
        laneWidth = 0;
        greedy = 0;
        deltaTheta = 0;

        map = nullptr;
        distance = nullptr;

        voronoiPlanner = new VoronoiPlanner();

    }

    VoronoiRRTPlanner::VoronoiRRTPlanner(std::string name, costmap_2d::Costmap2DROS* costmap_ros){

        initialize(name, costmap_ros);
        voronoiPlanner = new VoronoiPlanner();
    }

    void VoronoiRRTPlanner::initialize(std::string name, costmap_2d::Costmap2DROS* costmap_ros){

        voronoiPlanner->initialize(name, costmap_ros);

        map = new ROSMap(costmap_ros);
        distance = new L2ThetaDistance();

        //Parameters from ros parameters server
        ros::NodeHandle private_nh("~/" + name);

        private_nh.param("iterations", K, 30000);
        private_nh.param("deltaX", deltaX, 0.5);
        private_nh.param("laneWidth", laneWidth, 2.0);
        private_nh.param("greedy", greedy, 0.1);
        private_nh.param("deltaTheta", deltaTheta, M_PI/4);

        extenderFactory.initialize(private_nh, *map, *distance);
        visualizer.initialize(private_nh);
    }

    bool VoronoiRRTPlanner::makePlan(const geometry_msgs::PoseStamped& start,
                                const geometry_msgs::PoseStamped& goal,
                                std::vector<geometry_msgs::PoseStamped>& plan){

        visualizer.clean();

        //Retrieve Voronoi plan
        vector<geometry_msgs::PoseStamped> voronoiPlan;

        if(!voronoiPlanner->makePlan(start, goal, voronoiPlan)){
            ROS_INFO("Impossible to compute the Voronoi plan");
            return false;
        }

        visualizer.displayPlan(voronoiPlan);

        //Compute VoronoiRRT plan
        Distance& distance = *this->distance;

        VectorXd&& x0 = convertPose(start);
        VectorXd&& xGoal = convertPose(goal);

        RRT rrt(distance, x0);

        ROS_INFO("Voronoi-RRT started");


        for(unsigned int i = 0; i < K; i++){

            VectorXd xRand;

            if(RandomGenerator::sampleEvent(greedy)){
                xRand = xGoal;
            }
            else{
                xRand = extenderFactory.getKinematicModel().sampleOnLane(voronoiPlan,
                                                                         laneWidth,
                                                                         deltaTheta);
            }

            visualizer.addPoint(xRand);

            auto* node = rrt.searchNearestNode(xRand);

            VectorXd xNew;

            if(newState(xRand, node->x, xNew)){
                rrt.addNode(node, xNew);

                visualizer.addSegment(node->x, xNew);

                if(distance(xNew, xGoal) < deltaX){
                    auto&& path = rrt.getPathToLastNode();
                    publishPlan(path, plan, start.header.stamp);

                    visualizer.displayPlan(plan);
                    visualizer.flush();

                    ROS_INFO("Plan found");

                    return true;
                }
            }

        }

        visualizer.flush();

        ROS_WARN_STREAM("Failed to found a plan in " << K << " RRT iterations");
        return false;
    }

    bool VoronoiRRTPlanner::newState(const Eigen::VectorXd& xRand,
                                     const Eigen::VectorXd& xNear,
                                     Eigen::VectorXd& xNew){

        return extenderFactory.getExtender().compute(xNear, xRand,
                                                     xNew);
    }

    VectorXd VoronoiRRTPlanner::convertPose(const geometry_msgs::PoseStamped& msg){

        auto& q_ros = msg.pose.orientation;
        auto& t_ros = msg.pose.position;

        Quaterniond q(q_ros.w, q_ros.x, q_ros.y, q_ros.z);

        Vector3d theta = q.matrix().eulerAngles(0, 1, 2);

        VectorXd x(3);
        x << t_ros.x, t_ros.y, theta(2);

        return x;
    }

    void VoronoiRRTPlanner::publishPlan(std::vector<Eigen::VectorXd>& path,
                                        std::vector<geometry_msgs::PoseStamped>& plan,
                                        const ros::Time& stamp){

        for(auto x : path){
            geometry_msgs::PoseStamped msg;

            msg.header.stamp = stamp;
            msg.header.frame_id = "map";

            msg.pose.position.x = x(0);
            msg.pose.position.y = x(1);
            msg.pose.position.z = 0;

            Matrix3d m;
            m = AngleAxisd(x(2), Vector3d::UnitZ())
                * AngleAxisd(0, Vector3d::UnitY())
                * AngleAxisd(0, Vector3d::UnitX());

            Quaterniond q(m);

            msg.pose.orientation.x = q.x();
            msg.pose.orientation.y = q.y();
            msg.pose.orientation.z = q.z();
            msg.pose.orientation.w = q.w();

            plan.push_back(msg);
        }
    }

    VoronoiRRTPlanner::~VoronoiRRTPlanner(){

        if(distance){
            delete distance;
        }

        if(map){
            delete map;
        }

        if(voronoiPlanner){
            delete voronoiPlanner;
        }
    }
}

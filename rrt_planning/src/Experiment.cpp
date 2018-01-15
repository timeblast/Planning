#include <fstream>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

#include <tf/transform_listener.h>
#include <nav_core/base_global_planner.h>
#include <geometry_msgs/PoseStamped.h>

#include "rrt_planning/AbstractPlanner.h"
#include "rrt_planning/NHPlanner.h"
#include "rrt_planning/ForwardNHPlanner.h"
#include "rrt_planning/RRTPlanner.h"
#include "rrt_planning/RRTStarPlanner.h"
#include "rrt_planning/ThetaStarRRTPlanner.h"
#include "rrt_planning/VoronoiRRTPlanner.h"


using namespace rrt_planning;
using namespace std;

AbstractPlanner* getPlanner(const string& name, costmap_2d::Costmap2DROS* costmap_ros, const string& t);
bool parse(const std::string& conf, geometry_msgs::PoseStamped& start_pose,
				 geometry_msgs::PoseStamped& goal_pose);
void save(const std::string& filename, const std::string& conf, double t,
				double l, std::vector<geometry_msgs::PoseStamped>& plan);


int main(int argc, char** argv)
{
	ROS_FATAL_STREAM("Starting");
	string planner_name = argv[1];
	string map = argv[2];
	string conf = argv[3];
	string row = argv[4];
	string deadline = argv[5];
	string dir = argv[6];
	string node_name = planner_name + "_" + map + "_" + row;
	ROS_FATAL_STREAM("node_name: " << node_name);

	ros::init(argc, argv, node_name);
	ros::NodeHandle private_nh("~/");

	//Costmap inizialization magics
        tf::TransformListener tf_(ros::Duration(10));
        costmap_2d::Costmap2DROS* costmap_ros = new costmap_2d::Costmap2DROS("global_costmap", tf_);
        costmap_ros->pause();

	ROS_FATAL_STREAM("Costmap loaded");

	//Convert start and goal
	geometry_msgs::PoseStamped start_pose, goal_pose;
	std::vector<geometry_msgs::PoseStamped> plan;
        bool valid = parse(conf, start_pose, goal_pose);

        if(!valid)
        {
            std::ofstream f;
            f.open(dir+node_name + string(".log"));
            f << "INVALID CONFIGURATION: " << conf << "\n";
            f.close();
            return 0;
        }

	//Launch planner
	AbstractPlanner* planner = getPlanner(planner_name, costmap_ros, deadline);
	ROS_FATAL_STREAM("Started planner: " + planner_name);
	bool result = planner->makePlan(start_pose, goal_pose, plan);
	if(result)
	{
		save(dir + node_name, conf, planner->getElapsedTime(), planner->getPathLength(), plan);
	}
	else
	{
		std::ofstream f;
		f.open(dir+node_name + string(".log"));
		f << "configuration: " << conf << "\n";
		f << "NO_PATH_FOUND_WITHIN_DEADLINE(" << deadline << "s)" << "\n";
		f.close();
	}

	private_nh.deleteParam("");
	ROS_FATAL_STREAM("Plan found: " << result);

	return 0;
}

bool parse(const string& conf, geometry_msgs::PoseStamped& start_pose, geometry_msgs::PoseStamped& goal_pose)
{
	stringstream s(conf);
	string segment;
	vector<string> seglist;
	while(getline(s, segment, '_'))
	{
	   seglist.push_back(segment);
	}

        if(seglist.size() != 8)
            return false;

	start_pose.pose.position.x = stod(seglist[0]);
	start_pose.pose.position.y = stod(seglist[1]);
	start_pose.pose.position.z = 0;
	start_pose.pose.orientation.x = 0;
	start_pose.pose.orientation.y = 0;
	start_pose.pose.orientation.z = stod(seglist[2]);
	start_pose.pose.orientation.w = stod(seglist[3]);

	goal_pose.pose.position.x = stod(seglist[4]);
	goal_pose.pose.position.y = stod(seglist[5]);
	goal_pose.pose.position.z = 0;
	goal_pose.pose.orientation.x = 0;
	goal_pose.pose.orientation.y = 0;
	goal_pose.pose.orientation.z = stod(seglist[6]);
	goal_pose.pose.orientation.w = stod(seglist[7]);

        start_pose.header.frame_id = std::string("/map");
        goal_pose.header.frame_id = std::string("/map");

        return true;
}

void save(const std::string& filename, const std::string& conf, double t, double l, std::vector<geometry_msgs::PoseStamped>& plan)
{
	std::ofstream f;
	f.open(filename + string(".log"));
	std::string d = string("_");

	//Write configuration
	f << "configuration: "<< conf << "\n";

	//Write path length
	f << "length: " << l << "\n";

	//Write execution Time
	f << "time: "<<t << "\n";

	//Write path points
	for(auto p : plan)
	{
		f << p.pose.position.x << d << p.pose.position.y << d
		  << p.pose.orientation.z << d << p.pose.orientation.w << "\n";
	}

	f.close();
}


AbstractPlanner* getPlanner(const string& name, costmap_2d::Costmap2DROS* costmap_ros, const string& t)
{
	chrono::duration<double> Tmax(stod(t));
	if(name == "nh")
	{
		NHPlanner* planner = new NHPlanner(string(""), costmap_ros, Tmax);
		return planner;
	}
	else if(name == "forward_nh")
	{
		ForwardNHPlanner* planner = new ForwardNHPlanner(string(""), costmap_ros, Tmax);
		return planner;
	}
	else if(name == "rrt")
	{
		RRTPlanner* planner = new RRTPlanner(string(""), costmap_ros, Tmax);
		return planner;
	}
	else if(name == "rrt_star")
	{
		RRTStarPlanner* planner = new RRTStarPlanner(string(""), costmap_ros, Tmax);
		return planner;
	}
	else if(name == "theta_star_rrt")
	{
		ThetaStarRRTPlanner* planner = new ThetaStarRRTPlanner(string(""), costmap_ros, Tmax);
		return planner;
	}
	else if(name == "voronoi_rrt")
	{
		VoronoiRRTPlanner* planner = new VoronoiRRTPlanner(string(""), costmap_ros, Tmax);
		return planner;
	}
	else
	{
		ROS_FATAL("Planner not impleted, aborting");
		exit(0);
	}
}

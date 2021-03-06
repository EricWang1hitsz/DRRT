#ifndef DRRT_DATA_STRUCTURES_H
#define DRRT_DATA_STRUCTURES_H


#include <DRRT/list.h>
#include <DRRT/heap.h>
#include <DRRT/edge.h> // includes jlist.h which includes
                       // obstacle.h which includes distancefunctions.h
/// Include implementation of desired edge here
#include <DRRT/dubinsedge.h>

// For holding triangles
typedef Eigen::Matrix<double,Eigen::Dynamic,6> MatrixX6d;

class ConfigSpace : public std::enable_shared_from_this<ConfigSpace> {
public:
    std::mutex cspace_mutex_;         // mutex for accessing obstacle List
    int num_dimensions_;              // dimensions
    std::shared_ptr<List> obstacles_; // a list of obstacles
    bool obstacles_moved_;
    double obs_delta_; // the granularity of obstacle checks on edges
    Eigen::VectorXd lower_bounds_; // 1xD vector containing the lower bounds
    Eigen::VectorXd upper_bounds_; // 1xD vector containing the upper bounds
    Eigen::VectorXd width_; // 1xD vector containing upper_bounds_-lower_bounds_
    Eigen::VectorXd start_; // 1xD vector containing start location
    Eigen::VectorXd goal_;  // 1xD vector containing goal location

    // Bullet Collision Detection
    btCollisionConfiguration* bt_collision_configuration_;
    btCollisionDispatcher* bt_dispatcher_;
    btBroadphaseInterface* bt_broadphase_;
    btCollisionWorld* bt_collision_world_;

    Region drivable_region_;    // Drivable region in which to do sampling

    std::vector<std::shared_ptr<Edge>> collisions_; // edges known to be in
                                                  // collision with an obstacle
    std::vector<std::shared_ptr<Edge>> trajectories_;

    double (*distanceFunction)(Eigen::VectorXd a, Eigen::VectorXd b);

    /* Flags that indicate what type of search space we are using
     * (these are mostly here to reduce the amount of duplicate code
     * for similar spaces, although they should probably one day be
     * replaced with a different approach that takes advantage of
     * Julia's multiple dispatch and polymorphism
     */
    bool space_has_theta_; // if true then the 3rd dimension of the space
                           // is theta in particular a Dubin's system is used
    bool space_has_time_;  // if true then the 4th dimension of the space
                           // is time

    double collision_distance_; // distance underwhich a collision would occur

    // Stuff for sampling functions
    double prob_goal_;  // the probabality that the goal is sampled

    std::shared_ptr<KDTreeNode> goal_node_;  // the goal node
    std::shared_ptr<KDTreeNode> root_;      // the root node
    std::shared_ptr<KDTreeNode> move_goal_;  // the current node goal for robot

    // sample this when iterations_until_sample_ == 0
    Eigen::VectorXd iteration_sample_point_;
    // sample this when wait_time_ has passed
    Eigen::VectorXd time_sample_point_;
    int iterations_until_sample_; // a count down to sample a particular point
    double wait_time_;            // time to wait in seconds
    u_int64_t start_time_ns_;     // time this started
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
    double time_elapsed_; // elapsed time since start

    std::shared_ptr<Obstacle> obstacle_to_remove_; // an obstacle to remove

    double robot_radius_;       // robot radius
    double robot_velocity_;     // robot velocity (used for Dubins w/o time)

    double dubins_min_velocity_; // min vel of Dubin's car (for dubins + time)
    double dubins_max_velocity_; // max vel of Dubin's car (for dubins + time)

    /// Use JListNode->node->position_ when popping from this stack
    std::shared_ptr<JList> sample_stack_; // points to sample in the future

    double hyper_volume_;        // hyper_volume_ of the space
    double saturation_delta_;    // RRT parameter delta
    double min_turn_radius_;     // min turning radius e.g. for Dubin's car

    double warmup_time_;  // the amount of warm up time allowed (obstacles are
                          // ignored for warm up time)
    bool in_warmup_time_; // true if we are in the warm up time
    bool warmup_time_just_ended_; // true if the we just started moving

    // Constructor
    ConfigSpace(int D, Eigen::VectorXd lower, Eigen::VectorXd upper,
           Eigen::VectorXd startpoint, Eigen::VectorXd endpoint)
        : num_dimensions_(D), lower_bounds_(lower), upper_bounds_(upper),
          start_(startpoint), goal_(endpoint)
    {
        // Bullet configuration
        bt_collision_configuration_ = new btDefaultCollisionConfiguration();
        bt_dispatcher_ = new btCollisionDispatcher(bt_collision_configuration_);
        /// Generalize the size boundaries
        btVector3 world_min((btScalar)0,(btScalar)0,(btScalar)-0.1);
        btVector3 world_max((btScalar)50,(btScalar)50,(btScalar)0.1);
        // There may be a faster broadphase
        // True for disabling raycast accelerator
        bt_broadphase_ = new bt32BitAxisSweep3(world_min,world_max,
                                               20000,0,true);
        bt_collision_world_ = new btCollisionWorld(bt_dispatcher_,
                                                   bt_broadphase_,
                                                   bt_collision_configuration_);

        obstacles_ = std::make_shared<List>();

        hyper_volume_ = 0.0; // flag indicating this needs to be calculated
        in_warmup_time_ = false;
        warmup_time_ = 0.0; // default value for time for build
                            // graph with no obstacles
        Eigen::ArrayXd upper_array = upper;
        Eigen::ArrayXd lower_array = lower;
        width_ = upper_array - lower_array;
    }

    // Setter for distanceFunction
    void SetDistanceFunction(double(*func)(Eigen::VectorXd a,
                                           Eigen::VectorXd b))
    { distanceFunction = func; }

    std::shared_ptr<ConfigSpace> GetPointer()
    { return shared_from_this(); }

    // Add Edge to vizualizer
    void AddVizEdge(std::shared_ptr<Edge> &edge, std::string type, bool vis)
    {
//        std::lock_guard<std::mutex> lock(this->cspace_mutex_);
        // For adding a trajectory edge to the vizualiser
        if(vis) {
            for(int k = 1; k < edge->trajectory_.rows(); k++) {
                Eigen::VectorXd _start_point = edge->trajectory_.row(k-1);
                Eigen::VectorXd _end_point = edge->trajectory_.row(k);

                std::shared_ptr<KDTreeNode> _s
                        = std::make_shared<KDTreeNode>(_start_point);
                std::shared_ptr<KDTreeNode> _e
                        = std::make_shared<KDTreeNode>(_end_point);

                double _angle = atan2(_end_point(1) - _start_point(1),
                                      _end_point(0) - _start_point(0));
                double _x = cos(_angle);
                double _y = sin(_angle);
                _angle = atan2(_y,_x);
                _s->position_(2) = _angle;
                _e->position_(2) = _angle;

                std::shared_ptr<Edge> _de
                     = Edge::NewEdge(this->GetPointer(),
                                     std::make_shared<KDTree>(), _s, _e);
                if(type == "coll") {
                    this->collisions_.push_back(_de);
                } else if(type == "traj") {
                    this->trajectories_.push_back(_de);
                }
            }
        }
    }

    // Remove Edge from vizualizer
    void RemoveVizEdge(std::shared_ptr<Edge> &edge, std::string type)
    {
//        std::lock_guard<std::mutex> lock(this->cspace_mutex_);
        if(type == "coll") {
            this->collisions_.erase(
                        std::remove(this->collisions_.begin(),
                                    this->collisions_.end(),edge),
                        this->collisions_.end());
        } else if(type == "traj") {
            this->trajectories_.erase(
                        std::remove(this->trajectories_.begin(),
                                    this->trajectories_.end(),edge),
                        this->trajectories_.end());
        }
    }
};

typedef struct Queue{
    std::mutex queuetex;
    std::string type;
    std::shared_ptr<ConfigSpace> cspace;
    std::shared_ptr<BinaryHeap> priority_queue; // nodes to be rewired
    std::shared_ptr<JList> obs_successors; // obstacle successor list
    double change_thresh; // threshold of local changes that we care about

} Queue;

// This is used to make iteration through a particular node's
// neighbor edges easier given that each node stores all of its
// neighbor edges in three different places
typedef struct RrtNodeNeighborIterator{
    std::shared_ptr<KDTreeNode> this_node;   // the node who's neighbors
                            // we are iterating through

    int list_flag;          // flag with the following values:
                            //  0: uninitialized
                            //  1: successors
                            //  2: original neighbors
                            //  3: current neighbors

    std::shared_ptr<JListNode> current_item; // a pointer to the position in
                                             // the current neighbor list we
                                             // are iterating through

    // Constructor
    RrtNodeNeighborIterator( std::shared_ptr<KDTreeNode> &node ):
        this_node(node), list_flag(0)
    {}

} RrtNodeNeighborIterator;

/* This holds the stuff associated with the robot that is
 * necessary for movement. Although some of the fields are
 * primarily used for simulation of robot movement,
 * current_move_invalid is important for the algorithm in general
 */
typedef struct RobotData{
    std::mutex robot_mutex;

    bool goal_reached;          // true if robot has reached goal space

    Eigen::VectorXd robot_pose;  // this is where the robot is
                                // (i.e. where it was at the end of
                                // the last control loop

    Eigen::VectorXd next_robot_pose;  // this is where the robot will
                                    // be at the end of the current
                                    // control loop

    std::shared_ptr<KDTreeNode> next_move_target; // this is the node at the
                            // root-end of the edge containing next_robot_pose

    double distance_from_next_robot_pose_to_next_move_target; // this holds the
                        // distance from next_robot_pose to next_move_target
                        // along the trajectory the robot
                        // will be following at that time
    bool moving;  // set to true when the robot starts moving


    bool current_move_invalid; // this gets set to true if next_move_target
                             // has become invalid due to dynamic obstacles

    Eigen::MatrixXd robot_move_path; // this holds the path the robot has
                 // followed from the start of movement up through robot_pose
    double num_robot_move_points;  // the number of points in robot_move_path

    Eigen::MatrixXd robot_local_path; // this holds the path between robot_pose
                             // and next_robot_pose (not including the former)
    double num_local_move_points;   // the number of points in robot_local_path

    std::shared_ptr<Edge> robot_edge; // this is the edge that contains the
                                     // trajectory that the
                                     // robot is currently following

    bool robot_edge_used; // true if robot_edge is populated;

    // Note that currently only one of the two following parameters
    // is used at a time.
    // Which one is used depends on if time is explicitely part of
    // the state space.
    double dist_along_robot_edge; // the current distance that the robot
                              // "will be" along robot_edge (next time slice)

    double time_along_robot_edge; // the current time that the robot "will be"
                                  // along robot_edge (i.e. next time slice)

    double robot_sensor_range;  // distance at which the robot notices obstacles

    // Optimal path as determined by the Theta*
    // Any Angle search algorithm path and angles
    // These are defined in each iteration of the main loop
    // Starts at ConfigSpace->start_ (0,0)
    std::vector<Eigen::VectorXd> best_any_angle_path; // vector of points that define the path
    std::vector<double> thetas;     // vector of angles corresponding to the lines in the path

    // Constructor
    RobotData(Eigen::VectorXd rP,
              std::shared_ptr<KDTreeNode> nMT,
              int dimensions)
        : robot_pose(rP),
          next_robot_pose(rP),
          next_move_target(nMT),
          distance_from_next_robot_pose_to_next_move_target(0.0),
          moving(false),
          current_move_invalid(false),
          num_robot_move_points(1),
          /*Original Julia code does not have an
           * initial value for num_local_move_points*/
          num_local_move_points(1),
          robot_edge_used(false),
          dist_along_robot_edge(0.0),
          time_along_robot_edge(0.0)
    {
        robot_pose.resize(dimensions);
        robot_local_path.resize(MAXPATHNODES,dimensions);
        robot_move_path.resize(MAXPATHNODES,dimensions);
    }

} RobotData;

#endif // DRRT_DATA_STRUCTURES_H

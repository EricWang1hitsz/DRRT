#ifndef DISTANCE_FUNCTIONS_H
#define DISTANCE_FUNCTIONS_H

#include <Eigen/Eigen>
#include <chrono>

class Region2D;
typedef Region2D Region;

double GetTimeNs(std::chrono::time_point<std::chrono::high_resolution_clock> start);

double DubinsDistance(Eigen::Vector3d a, Eigen::Vector3d b);

Eigen::Vector3d SaturateDubins(Eigen::Vector3d closest_point, double delta, double dist);

bool PointInPolygon(Eigen::VectorXd point, Region polygon);

double DistToPolygonSqrd(Eigen::VectorXd point, Region polygon);

#endif // DISTANCE_FUNCTIONS_H

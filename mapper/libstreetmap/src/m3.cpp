/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <vector>
#include <queue>
#include "m3.h"
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "database.h"

using namespace std;
extern database my_database;
// Returns a path (route) between the start intersection and the end 
// intersection, if one exists. If no path exists, this routine returns 
// an empty (size == 0) vector. If more than one path exists, the path 
// with the shortest travel time is returned. The path is returned as a vector 
// of street segment ids; traversing these street segments, in the given order,
// would take one from the start to the end intersection.

std::vector<unsigned> find_path_between_intersections(unsigned
        intersect_id_start, unsigned intersect_id_end) {
    vector<unsigned> empty={};
    vector<int> visited = my_database.visited; //this vector stores whether  a specific intersection has been visited
    vector<double> time = my_database.time; //this vector stores current arrival time of intersections
    vector<unsigned> path; //this vector returns the total path
    time[intersect_id_start] = 0;
    vector<unsigned> come_from_ints = my_database.come_from_ints; //previous intersection of an intersection
    vector<unsigned> come_from_segs = my_database.come_from_segs; //previous street segment of an intersection
    int flag = 0; //this flag records whether a legal path has been found
    priority_queue<pair<unsigned, double>, vector<pair<unsigned, double>>, prioritize> pq; //f value
    pq.push(make_pair(intersect_id_start, h_value(intersect_id_start, intersect_id_end)));
    while (!pq.empty()) {
        pair<unsigned, double> cur = pq.top();
        pq.pop();
        /*reach the end point means the path has been found*/
        if (cur.first == intersect_id_end) {
            flag = 1;
            break;
        }
        if (visited[cur.first]) continue;
        visited[cur.first] = 1;
        /*traverse all adjacent intersections and according street segment*/
        for (unsigned i = 0; i < my_database.iit[cur.first].size(); i++) {
            unsigned next_intersection_id = my_database.iit[cur.first][i].first;
            unsigned segid = my_database.iit[cur.first][i].second;
            if (visited[next_intersection_id] == 1) {
                continue;
            }
            StreetSegmentInfo info = getStreetSegmentInfo(segid);
            //vector<unsigned> ids = path[cur.first];
            double p_time = time[cur.first] + find_street_segment_travel_time(segid);
            if (cur.first != intersect_id_start) {
                unsigned lastid = come_from_segs[cur.first];
                /*check whether there is a turning*/
                if (getStreetSegmentInfo(lastid).streetID != info.streetID) {
                    p_time += 0.25; //add 15 seconds
                }
            }
            /*if this is a better path, then record it*/
            if (p_time < time[next_intersection_id]) {
                time[next_intersection_id] = p_time;
                /*push it into the queue*/
                pq.push(make_pair(next_intersection_id, time[next_intersection_id] + h_value(next_intersection_id, intersect_id_end)));
                come_from_ints[next_intersection_id] = cur.first; //update previous intersection  
                come_from_segs[next_intersection_id] = segid; //update previous street segment
            }

        }
    }
    if (flag == 0) return empty;
    unsigned i = intersect_id_end;
    while (i != intersect_id_start) {
        path.insert(path.begin(), come_from_segs[i]);
        i = come_from_ints[i];
    }
    return path;

}


// Returns the time required to travel along the path specified. The path
// is passed in as a vector of street segment ids, and this function can 
// assume the vector either forms a legal path or has size == 0.
// The travel time is the sum of the length/speed-limit of each street 
// segment, plus 15 seconds per turn implied by the path. A turn occurs
// when two consecutive street segments have different street names.

double compute_path_travel_time(const std::vector<unsigned>& path) {
    if (path.size() == 0) return 0;
    double travel_time = 0;
    travel_time += find_street_segment_travel_time(path[0]);
    for (unsigned i = 1; i < path.size(); i++) {
        travel_time += find_street_segment_travel_time(path[i]);

        StreetSegmentInfo info1 = getStreetSegmentInfo(path[i - 1]);
        StreetSegmentInfo info2 = getStreetSegmentInfo(path[i]);
        /*check whether there is a turning*/
        if (info1.streetID != info2.streetID) {
            travel_time += 0.25;
        }
    }
    return travel_time;
}


// Returns the shortest travel time path (vector of street segments) from 
// the start intersection to a point of interest with the specified name.
// If no such path exists, returns an empty (size == 0) vector.

std::vector<unsigned> find_path_to_point_of_interest(unsigned
        intersect_id_start, std::string point_of_interest_name) {
    vector<unsigned> empty={};
    vector<int> closest_intersection = my_database.closest_intersections; //this vector stores whether  a specific intersection has been visited
    vector<unsigned> POIids = my_database.POIname_to_id.find(point_of_interest_name)->second; //this vector stores current arrival time of intersections
    vector<unsigned> path; //this vector returns the total path
    vector<unsigned> ints; //this vector contains closest intersections of POIs
    for (unsigned i = 0; i < POIids.size(); i++) { 
        /*find the closest intersections of pois, remove duplicates*/
        unsigned closest_int_id = my_database.poi_to_int[POIids[i]];
        if (closest_intersection[closest_int_id] == 0) {
            ints.push_back(closest_int_id);
            closest_intersection[closest_int_id] = 1;
        }
    }
    unsigned intid;
    vector<int> visited = my_database.visited;
    vector<double> time = my_database.time;
    time[intersect_id_start] = 0; //arrival time of start point is zero 
    vector<unsigned> come_from_ints = my_database.come_from_ints; //previous intersection of an intersection
    vector<unsigned> come_from_segs = my_database.come_from_segs; //previous street segment of an intersection
    priority_queue<pair<unsigned, double>, vector<pair<unsigned, double>>, prioritize> pq;
    if (ints.size() == 1) pq.push(make_pair(intersect_id_start, h_value(intersect_id_start, ints[0])));
    else pq.push(make_pair(intersect_id_start, 0));
    int flag = 0; //this flag is used to judge whether we find a legal path
    while (!pq.empty()) {
        /*pop the intersection with smallest f value from the queue*/
        pair<unsigned, double> cur = pq.top();
        pq.pop();
        /*check whether this intersection is one of the closest intersections of pois*/
        if (closest_intersection[cur.first] == 1) {
            flag = 1; //set flag to 1
            intid = cur.first; //store this intersection id
            break;

        }
        if (visited[cur.first]) continue;
        visited[cur.first] = 1;
        /*traverse all adjacent intersections and according street segments*/
        for (unsigned i = 0; i < my_database.iit[cur.first].size(); i++) {
            unsigned next_intersection_id = my_database.iit[cur.first][i].first;
            unsigned segid = my_database.iit[cur.first][i].second;
            if (visited[next_intersection_id] == 1) {
                continue;
            }
            StreetSegmentInfo info = getStreetSegmentInfo(segid);
            double p_time = time[cur.first] + find_street_segment_travel_time(segid);
            if (cur.first != intersect_id_start) {
                unsigned lastid = come_from_segs[cur.first];
                /*check whether there is a turning*/
                if (getStreetSegmentInfo(lastid).streetID != info.streetID) {
                    p_time += 0.25;//add 15 seconds
                }
            }
            /*if this path is a better path, then record it*/
            if (p_time < time[next_intersection_id]) {
                time[next_intersection_id] = p_time; //update the arrival time
                if (ints.size() == 1) pq.push(make_pair(next_intersection_id, time[next_intersection_id] + h_value(next_intersection_id, ints[0])));//push it into queue
                else pq.push(make_pair(next_intersection_id, time[next_intersection_id]));
                come_from_ints[next_intersection_id] = cur.first;
                come_from_segs[next_intersection_id] = segid;
            }

        }
    }
    /*if not find, return empty vector*/
    if (flag == 0) {
        return empty;

    }
    unsigned i = intid;
    /*reconstruct the path*/
    while (i != intersect_id_start) {
        path.insert(path.begin(), come_from_segs[i]);
        i = come_from_ints[i];
    }
    return path;

}

/*this function is used to calculate the h value of each intersections*/
double h_value(unsigned id1, unsigned id2) {
    double length = find_distance_between_two_points(getIntersectionPosition(id1), getIntersectionPosition(id2));
    return (60 * length / 1000) / (my_database.max_speed);
}

/*load adjacent intersections and streets in a data structure*/
void load_adjacent_intersection_and_streets() {
    for (unsigned i = 0; i < getNumberOfIntersections(); i++) {
        my_database.iit.push_back({});
        unsigned count = getIntersectionStreetSegmentCount(i);
        for (unsigned j = 0; j < count; j++) {
            unsigned segid = getIntersectionStreetSegment(i, j);
            StreetSegmentInfo info = getStreetSegmentInfo(segid);

            if (info.oneWay == true) {
                if (i == info.to) continue;
            }

            unsigned next_intersection_id;
            if (i == info.from) next_intersection_id = info.to;
            else next_intersection_id = info.from;

            my_database.iit[i].push_back(make_pair(next_intersection_id, segid));
        }
    }
}

void load_closest_ints() {
    for (unsigned i = 0; i < getNumberOfPointsOfInterest(); i++) {
        unsigned id = i; //find_closest_intersection(getPointOfInterestPosition(i));
        my_database.closest_intersection.push_back(id);
        //cout<<getPointOfInterestName(i)<<endl;
    }
}


/*load names of intersections, streets and pois into binary search trees*/
void load_partial_name(){
    for(unsigned i=0;i<getNumberOfPointsOfInterest();i++){
        my_database.poi_bst.insert(getPointOfInterestName(i));
    }
    for(unsigned i=0;i<getNumberOfStreets();i++){
        my_database.st_bst.insert(getStreetName(i));
    }
    for(unsigned i=0;i<getNumberOfIntersections();i++){
        my_database.int_bst.insert(getIntersectionName(i));
        if(my_database.intname_to_id.find(getIntersectionName(i))==my_database.intname_to_id.end()){
            vector<unsigned> v;
            v.push_back(i);
            my_database.intname_to_id.insert(make_pair(getIntersectionName(i),v));
        }else{
           my_database.intname_to_id[getIntersectionName(i)].push_back(i); 
        }
       
    }
}





#include "StreetsDatabaseAPI.h"
#include "m1.h"
#include "StreetsDatabase.h"
#include <algorithm>
#include <unordered_map>
#include "OSMDatabaseAPI.h"
#include "graphics.h"
#include "m2.h"
#include "m3.h"
#include "database.h"
using namespace std;

/**********************************
 *                                *
 *       Global Variables         *
 *                                *
 **********************************/

extern database my_database;


/*************************************************************************
 *                                                                       *
 *  Helper Functions That Load The Map Into The Global Variables Above   *
 *                                                                       *
 *************************************************************************/


// stores the map in a hash table, key: street name, value: street ids

void load_stName_to_ids() { //
    for (unsigned i = 0; i < getNumberOfStreets(); i++) {
        auto exist = my_database.stName_to_ids.find(getStreetName(i));
        if (exist == my_database.stName_to_ids.end()) { // if the name corresponding to i does not exist in the hash table, then insert
            vector<unsigned> ids = {i};
            my_database.stName_to_ids.insert(make_pair(getStreetName(i), ids));
        } else { // if it exists, push back the value
            exist->second.push_back(i);
        }
    }
}

// stores the map in a vector of vectors, index: street id, elements: street segments ids

void load_stid_to_stSeg_ids() {
    for (unsigned i = 0; i < getNumberOfStreets(); i++) { // initialize the vector stid_to_stSeg_ids
        vector<unsigned> v;
        my_database.stid_to_stSeg_ids.push_back(v);
    }

    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) { // push back the segment ids to the corresponding index (i.e. street id)
        StreetSegmentInfo info = getStreetSegmentInfo(i);
        my_database.stid_to_stSeg_ids[info.streetID].push_back(i);
    }
}

// stores the map in a vector of vectors, index: intersection id, elements: street segment ids
// stores the map in a hash table, key: intersection id, value: adjacent intersection ids
// stores the map in a hash table, key: street name, value: intersection ids

void load_ints_to_segs_ints_to_adjacents_streetnames_to_ints() {
    for (unsigned i = 0; i < getNumberOfIntersections(); i++) { // initialize the vector intersection_id_to_stSeg_ids
        vector<unsigned> v;
        my_database.intersection_id_to_stSeg_ids.push_back(v);
    }

    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
        // store segments into the vector:intersection_id_to_stSeg_ids ( note: duplications will be checked later )
        StreetSegmentInfo info = getStreetSegmentInfo(i);
        if (info.speedLimit > my_database.max_speed) my_database.max_speed = info.speedLimit;
        my_database.intersection_id_to_stSeg_ids[info.from].push_back(i);
        my_database.intersection_id_to_stSeg_ids[info.to].push_back(i);

        //store intersections into the map:name_to_ints
        string name = getStreetName(info.streetID);
        auto exist = my_database.name_to_ints.find(name);
        if (exist == my_database.name_to_ints.end()) {
            vector<unsigned> ints = {info.from, info.to};
            my_database.name_to_ints.insert(make_pair(name, ints));
        } else {
            exist->second.push_back(info.from);
            exist->second.push_back(info.to);
        }

        //store intersections into the map:adjacents
        auto exista = my_database.adjacents.find(info.from);

        if (exista == my_database.adjacents.end()) {
            vector<unsigned> adids;
            adids.push_back(info.to);
            my_database.adjacents.insert(make_pair(info.from, adids));
        } else {

            if (find(exista->second.begin(), exista->second.end(), info.to) == exista->second.end()) {
                exista->second.push_back(info.to);
            }
        }
        if (info.oneWay == false) {
            auto existb = my_database.adjacents.find(info.to);

            if (existb == my_database.adjacents.end()) {
                vector<unsigned> adids;
                adids.push_back(info.from);
                my_database.adjacents.insert(make_pair(info.to, adids));
            } else {

                if (find(existb->second.begin(), existb->second.end(), info.from) == existb->second.end()) {
                    existb->second.push_back(info.from);
                }
            }
        }



    }
}

// stores the map in a hash table, key: street segment id, value: length

void load_segs_to_length() {
    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
        StreetSegmentInfo info = getStreetSegmentInfo(i);
        double d = 0;
        if (info.curvePointCount == 0) {
            LatLon point1 = getIntersectionPosition(info.from);
            LatLon point2 = getIntersectionPosition(info.to);
            d = find_distance_between_two_points(point1, point2);
            my_database.segs_to_length.insert(make_pair(i, d));
        } else {
            unsigned num = info.curvePointCount;
            LatLon array[num + 2];
            array[0] = getIntersectionPosition(info.from);
            array[num + 1] = getIntersectionPosition(info.to);
            for (unsigned j = 0; j < num; j++) {
                array[j + 1] = getStreetSegmentCurvePoint(i, j);
            }
            for (unsigned k = 0; k < (num + 1); k++) {
                d += find_distance_between_two_points(array[k], array[k + 1]);
            }
            my_database.segs_to_length.insert(make_pair(i, d));
        }
        double limit = (d / 1000) / info.speedLimit;
        my_database.segid_to_travel_time.push_back(limit * 60);
    }
}

void load_POIname_to_ids() {
    for (unsigned i = 0; i < getNumberOfPointsOfInterest(); i++) {
        string POIname = getPointOfInterestName(i);
        if (my_database.POIname_to_id.find(POIname) == my_database.POIname_to_id.end()) {
            vector<unsigned> id;
            id.push_back(i);
            my_database.POIname_to_id.insert(make_pair(POIname, id));
        } else {
            my_database.POIname_to_id.find(POIname)->second.push_back(i);
        }
    }
    //for (auto iter = my_database.POIname_to_id.begin(); iter != my_database.POIname_to_id.end(); iter++) {
    //    if (iter->first == "Chapel") {
    //       cout << iter->second.size() << endl;
    //    }
    //}
}

//load the map

bool load_map(std::string map_name) {
    bool load_success_1, load_success_2;
    int size = map_name.size();
    string map_name_osm = map_name;
    map_name_osm.erase(size - 11, 11);
    map_name_osm += "osm.bin";

    load_success_1 = loadStreetsDatabaseBIN(map_name);
    load_success_2 = loadOSMDatabaseBIN(map_name_osm);
    // create any data structures here to speed up your API functions
    // ...


    load_stName_to_ids();
    load_POIname_to_ids();
    load_stid_to_stSeg_ids();
    load_ints_to_segs_ints_to_adjacents_streetnames_to_ints();
    load_segs_to_length();
    load_OSMID_to_node_index();
    load_subway_station();
    load_subway();
    load_OSMID_to_way_index();
    load_streetsegs_type();
    load_featureID_to_OSMtype();
    load_layer();
    initialize_m3_data();
    load_adjacent_intersection_and_streets();
    //load_closest_ints();
    a();
    load_region();
    load_poi_closest_int();
    load_partial_name();
    return load_success_1 && load_success_2;
}

//close the map

void close_map() {
    closeStreetDatabase();
    closeOSMDatabase();



    // destroy/clear any data structures you created in load_map
    // ...
}



/*****************************************
 *                                       *
 *    Required API Functions in m1.h     *
 *                                       *
 *****************************************/




//function to return street id(s) for a street name
//return a 0-length vector if no street with this name exists.

std::vector<unsigned> find_street_ids_from_name(std::string street_name) {
    auto iter = my_database.stName_to_ids.find(street_name); // note: stName_to_ids is a hash table
    if (iter != my_database.stName_to_ids.end()) {
        return iter->second;
    }
    return
    {
    };
}

//function to return the street segments for a given intersection 

std::vector<unsigned> find_intersection_street_segments(unsigned intersection_id) {
    return my_database.intersection_id_to_stSeg_ids[intersection_id]; //  note: intersection_id_to_stSeg_ids is a vector of vectors
}

//function to return street names at an intersection (include duplicate street names in returned vector)

std::vector<std::string> find_intersection_street_names(unsigned intersection_id) {

    std::vector<std::string> street_names;
    std::vector<unsigned> segment_ids = find_intersection_street_segments(intersection_id); // get segment ids from intersection_id
    for (auto iter = segment_ids.begin(); iter != segment_ids.end(); iter++) {
        unsigned ID = getStreetSegmentInfo(*iter).streetID; // get streetID from segment ids
        street_names.push_back(getStreetName(ID)); // get street name from streetID
    }
    return street_names;


}

//can you get from intersection1 to intersection2 using a single street segment (hint: check for 1-way streets too)
//corner case: an intersection is considered to be connected to itself

bool are_directly_connected(unsigned intersection_id1, unsigned intersection_id2) {
    if (intersection_id1 == intersection_id2) return true; // if the intersections are the same. return true
    auto iter = my_database.adjacents.find(intersection_id1);
    if (iter == my_database.adjacents.end()) {
        return false;
    } else {

        if (find(iter->second.begin(), iter->second.end(), intersection_id2) == iter->second.end()) {
            return false;
        }
        return true;
    }
}

//find all intersections reachable by traveling down one street segment 
//from given intersection (hint: you can't travel the wrong way on a 1-way street)
//the returned vector should NOT contain duplicate intersections

std::vector<unsigned> find_adjacent_intersections(unsigned intersection_id) {

    auto iter = my_database.adjacents.find(intersection_id);

    if (iter != my_database.adjacents.end()) {
        return iter->second;
    }
    vector<unsigned> b;
    return b; // if not found, return a o-element vector
}

//for a given street, return all the street segments

std::vector<unsigned> find_street_street_segments(unsigned street_id) {
    return my_database.stid_to_stSeg_ids[street_id]; // note: stid_to_stSeg_ids is a vector
}

//for a given street, find all the intersections

std::vector<unsigned> find_all_street_intersections(unsigned street_id) {
    vector<unsigned> segments = find_street_street_segments(street_id); // get all the segment ids from street_id
    vector<unsigned> intersections;
    for (auto iter = segments.begin(); iter != segments.end(); iter++) { // for each segment
        StreetSegmentInfo info = getStreetSegmentInfo(*iter);
        if (find(intersections.begin(), intersections.end(), info.from) == intersections.end()) { // if segment.from does not exist in the vector intersection then push back
            intersections.push_back(info.from);
        }
        if (find(intersections.begin(), intersections.end(), info.to) == intersections.end()) { // if segment.to does not exist in the vector intersection then push back
            intersections.push_back(info.to);
        }
    }
    return intersections;

}

//function to return all intersection ids for two intersecting streets
//this function will typically return one intersection id between two street names
//but duplicate street names are allowed, so more than 1 intersection id may exist for 2 street names

std::vector<unsigned> find_intersection_ids_from_street_names(std::string street_name1, std::string street_name2) {
    auto iter1 = my_database.name_to_ints.find(street_name1); // note: name_to_ints is a hash table
    auto iter2 = my_database.name_to_ints.find(street_name2);
    if (iter1 == my_database.name_to_ints.end() || iter2 == my_database.name_to_ints.end()) {
        return
        {
        };
    }
    vector<unsigned> ints1 = iter1->second; // get intersection ids of street name1
    vector<unsigned> ints2 = iter2->second; // get intersection ids of street name2
    vector<unsigned> intids;

    for (auto iter = ints1.begin(); iter != ints1.end(); iter++) { // go through two ints1 & ints2, check for common intersections
        if (find(ints2.begin(), ints2.end(), *iter) != ints2.end()) {
            if (find(intids.begin(), intids.end(), *iter) == intids.end()) {
                intids.push_back(*iter);
            }
        }
    }
    return intids;
}

//find distance between two coordinates

double find_distance_between_two_points(LatLon point1, LatLon point2) {
    double p1lon = point1.lon*DEG_TO_RAD;
    double y1 = point1.lat*DEG_TO_RAD;
    double p2lon = point2.lon*DEG_TO_RAD;
    double y2 = point2.lat*DEG_TO_RAD;
    double a = (y1 + y2) / 2;
    double co = cos(a); //use taylor expansion to calculate cosine value
    double x1 = p1lon*co;
    double x2 = p2lon*co;
    return EARTH_RADIUS_IN_METERS * sqrt((x2 - x1)*(x2 - x1)+(y2 - y1)*(y2 - y1));
}

//find the length of a given street segment

double find_street_segment_length(unsigned street_segment_id) {
    auto exist = my_database.segs_to_length.find(street_segment_id); // segs_to_length is a hash table
    if (exist != my_database.segs_to_length.end()) {
        return exist->second;
    }
    return 0;

}

//find the length of a whole street

double find_street_length(unsigned street_id) {
    std::vector<unsigned> segments = find_street_street_segments(street_id); // get segment ids from street_id
    double d = 0;
    for (auto iter = segments.begin(); iter != segments.end(); iter++) {
        d += find_street_segment_length(*iter); // get street length from segments length
    }
    return d;
}

//find the travel time to drive a street segment (time(minutes) = distance(km)/speed_limit(km/hr) * 60

double find_street_segment_travel_time(unsigned street_segment_id) {
    return my_database.segid_to_travel_time[street_segment_id];
}

//find the nearest point of interest to a given position

unsigned find_closest_point_of_interest(LatLon my_position) { // traverse through all POI to calculate distance and return the smallest
    double min = find_distance_between_two_points(my_position, getPointOfInterestPosition(0));
    unsigned id=0;;
    for (unsigned i = 0; i < getNumberOfPointsOfInterest(); i++) { //traverse all points of interest
        double d = find_distance_between_two_points(my_position, getPointOfInterestPosition(i));
        if (d < min) {
            min = d;
            id = i;
        }
    }
    return id;
}

//find the nearest intersection (by ID) to a given position

unsigned find_closest_intersection(LatLon my_position) { // traverse through all intersections to calculate the distance and return the smallest
    int x = (latlonProjection(my_position).x - my_database.left) / 1000;
    int y = (-latlonProjection(my_position).y + my_database.top) / 1000;
     vector<vector<int>> checked = my_database.checked;
    unsigned intid=0;
    int find = 0;
    int find_better = 0;
    int a = 3;
    double min_d = INF;
    do {
        find_better = 0;
        for (int i = max(0, y - (a - 1) / 2); i < min(y + (a + 1) / 2, my_database.m - 1); i++) {
            for (int j = max(0, x - (a - 1) / 2); j < min(my_database.n - 1, x + (a + 1) / 2); j++) {
                if (checked[i][j] == 1) continue;
                checked[i][j] = 1;
                for (unsigned k = 0; k < my_database.region[i][j].size(); k++) {
                    unsigned id = my_database.region[i][j][k];
                    double d = find_distance_between_two_points(my_position, getIntersectionPosition(id));
                    if (d < min_d) {
                        min_d = d;
                        intid = id;
                        find = 1;
                        find_better = 1;
                    }
                }
            }
        }
        a = 2 * a + 3;
    } while (find == 0 || find_better == 1);
    return intid;

}

double new_find_distance_between_two_points(LatLon point1, LatLon point2) {
    double p1lon = point1.lon*DEG_TO_RAD;
    double y1 = point1.lat*DEG_TO_RAD;
    double p2lon = point2.lon*DEG_TO_RAD;
    double y2 = point2.lat*DEG_TO_RAD;
    double a = (y1 + y2) / 2;
    double co = 1 - a * a / 2 + a * a * a * a / 4; //use taylor expansion to calculate cosine value
    double x1 = p1lon*co;
    double x2 = p2lon*co;
    return EARTH_RADIUS_IN_METERS * sqrt((x2 - x1)*(x2 - x1)+(y2 - y1)*(y2 - y1));
}

void load_region() {
    my_database.n = (my_database.right - my_database.left) / 1000;
    my_database.m = (my_database.top - my_database.bottom) / 1000;
    //cout<<my_database.m<<endl<<my_database.n;

    for (int i = 0; i < my_database.m; i++) {
        my_database.region.push_back({});
        my_database.checked.push_back({});
        for (int j = 0; j < my_database.n; j++) {
            my_database.region[i].push_back({});
            my_database.checked[i].push_back(0);
        }
    }
    for (unsigned i = 0; i < getNumberOfIntersections(); i++) {
        int x = (latlonProjection(getIntersectionPosition(i)).x - my_database.left) / 1000;
        int y = (-latlonProjection(getIntersectionPosition(i)).y + my_database.top) / 1000;
        //cout<<y<<" "<<x<<endl;
        my_database.region[y][x].push_back(i);
        //cout<<my_database.region[y][x][0]<<endl;
    }

}

void load_poi_closest_int() {
    for (unsigned i = 0; i < getNumberOfPointsOfInterest(); i++) {
        //cout<<getPointOfInterestName(i)<<endl;
        unsigned id = find_closest_intersection(getPointOfInterestPosition(i));
        my_database.poi_to_int.push_back(id);
    }
}




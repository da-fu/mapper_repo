/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   database.h
 * Author: gaobowen
 *
 * Created on February 20, 2016, 5:00 PM
 */

#ifndef DATABASE_H
#define DATABASE_H
#include "m1.h"
#include "m2.h"
#include "bst.h"
using namespace std;

enum SearchState {
    Begin = 0,
    Click,
    Input,
    Enter
};

enum HelpState {
    Start = 0,
    Main,
    Search,
    FindPath,
    Clear,
    POI,
    Swich,
    Other
};

class database {
public:
    string name;
    string city_name;
    /*zoom level*/
    int zoom_level;
    /*a vector used to store street segments type*/
    vector<string> segs_type;
    /*a vector used to store the layers(drawing order) of features*/
    vector<vector<unsigned>> layer;
    /*the average latitude of the city*/
    double avg_lat;
    /*a t bound box used to store the initial coordinates of visible world*/
    t_bound_box initial_coords;
    /*a t bound box used to store the current coordinates of visible world*/
    t_bound_box cur_coords;
    /*a hash table, key:OSMID, value: way index*/
    unordered_map<OSMID, unsigned long long> OSMID_to_way_index;
    /*a hash table, key:OSMID, value: node index*/
    unordered_map<OSMID, unsigned long long> OSMID_to_node_index;
    /*a hash table, key:feature id,  value:osm type*/
    unordered_map<unsigned, string> featureID_to_OSMtype;
    /*a hash table, key:feature id,  value:aeroway name*/
    unordered_map<unsigned, string> featureID_to_aeroway;
    /*a hash table, key:feature id,  value:hospital name*/
    unordered_map<unsigned, string> featureID_to_hospital;
    /*a vector used to store subway stations*/
    unordered_map<unsigned, string> subway_station_name;
    vector<LatLon> subway_station;
    /*a vector used to store subways*/
    vector<vector<LatLon>> subway;
    /*a vector used to store subway names*/
    vector<vector<string>> line;
    /*a flag used to determine whether to redraw the highlights of intersections found by "find ints"*/
    int draw_flag1;
    /*a flag used to determine whether to redraw the highlights of intersections found by click*/
    int draw_flag2;
    /*a flag used to determine whether to redraw the highlights of "buildings"*/
    int draw_flag3;
    /*a flag used to determine whether to redraw the highlights of streets found by "find streets"*/
    int draw_flag4;
    /*a flag used to determine whether to redraw the highlights of path */
    int draw_flag_path;
    /*a flag used to determine whether to draw subways*/
    int subway_flag;
    /*a flag used to determine whether to show interests names*/
    int show_interests_name;
    /* a flag for showing restaurant*/
    int food;
    /* a flag for showing parking lots*/
    int parking;
    /* a flag for showing banks and ATMs*/
    int bank;
    /*stores the id of an intersection or a building which is clicked by the user*/
    unsigned information_id;
    /*highlight flag for welcome screen*/
    int color;
    /*flag for search button*/
    int search;
    /*flag for switch map*/
    int switch_map;
    /*FSM state that control the search bar*/
    SearchState search_state;
    /*user's input from search bar*/
    string user_input;
    int draw_flag_poi;
    int information_flag;
    /*store the latlon position of the point user click*/
    LatLon latlon_information;
    /*intersection id array for button: find streets*/
    vector<unsigned> ids_array1;
    /*keep the last two search record of intersection*/
    vector<unsigned> inter_search_record;
    /*intersection id array for clicking on an intersection*/
    vector<unsigned> ids_array2;
    /*feature id array for clicking on a feature*/
    vector<unsigned> ids_array3;
    /*street segments ids of the path*/
    vector<unsigned> ids_path;
    /*street segments ids array for finding streets*/
    vector<unsigned> segment_ids_array;
    /*the intersection id at the beginning of the path*/
    unsigned inter_id1;
    /*the intersection id at the end of the path*/
    unsigned inter_id2;


    /*section for database in m1*/
    unordered_map<string, vector<unsigned>> stName_to_ids; // stores the map in a hash table, key: street name, value: street ids
    vector<vector<unsigned>> stid_to_stSeg_ids; // stores the map in a vector of vectors, index: street id, elements: street segments ids
    vector<vector<unsigned>> intersection_id_to_stSeg_ids; // stores the map in a vector of vectors, index: intersection id, elements: street segment ids
    unordered_map<unsigned, vector<unsigned>> adjacents; // stores the map in a hash talbe, key: intersection id, value: adjacent intersection ids
    unordered_map<unsigned, double> segs_to_length; // stores the map in a hash table, key: street segment id, value: length
    unordered_map<string, vector<unsigned>> name_to_ints; // stores the map in a hash table, key: street name, value: intersection ids
    vector<double> segid_to_travel_time; // stores the map in a vector, index: segment ids, value: travel time
    unordered_map<string, vector<unsigned>> POIname_to_id; // stores the map in a hash table, key: POI name, value: POI-ids


    /*m3 section*/
    vector<int> visited; //visited array for intersections
    vector<int> in_queue; //in queue array for intersections
    vector<double*> fvalue; 
    vector<double> time;  //g scorer array for intersections
    vector<vector<pair<unsigned, unsigned>>> iit; //stores adjacent intersections and street segs for intersections
    vector<vector<unsigned>> path;
    double max_speed;  //max speed limit in the city
    vector<int> closest_intersections; //used to check whether this intersection is in closest intersection array
    vector<unsigned> come_from_ints;  //stores the previous intersection of intersections
    vector<unsigned> come_from_segs; //stores previous street segs for intersections 
    vector<unsigned> closest_intersection;  //stores closest intersections for a  poi name
    vector<vector<vector<unsigned>>> region; //used to stores intersections according to regions
    /*boundary coords of the city*/
    double left;
    double right;
    double top;
    double bottom;
    /*dimension*/
    int m, n;
    vector<vector<int>> checked;
    vector<unsigned> poi_to_int; //stores closest intersections of pois
    
    vector<string> poi_partial_match;  //poi partial names
    vector<string> street_partial_match; //street partial names
    vector<string> intersection_partial_match;  //intersection partial names
    BST poi_bst;  //binary search tree for pois
    BST st_bst;  //binary search tree for streets
    BST int_bst; //binary search tree for intersections
    unsigned index=0;
    int count=0;
    vector<string> possible_names; //possible names of a partial match
    int search_type; //current search type
    unordered_map<string,vector<unsigned>> intname_to_id; //used to get intersection id from intersection name
    int change_flag=0;
    HelpState help_state;
    int num;
    database() {
        draw_flag1 = 0;
        draw_flag2 = 0;
        draw_flag3 = 0;
        draw_flag4 = 0;
        subway_flag = 0;
        show_interests_name = 0;
        information_flag = 0;
        information_id = 0;
        switch_map = 0;
        food = 0;
        parking = 0;
        bank = 0;
        max_speed = 0;
        color = 0;
        search = 0;
        search_state = Begin;
        draw_flag_path = 0;
        draw_flag_poi = 0;
        poi_bst.type="POI";
        st_bst.type="Street";
        int_bst.type="Int";
        help_state = Start;
    }
};


#endif /* DATABASE_H */


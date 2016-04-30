/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "graphics.h" 
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "Feature.h"
#include "m2.h"
#include "m3.h"
#include <math.h>
#include "OSMDatabaseAPI.h"
#include <unordered_map>
#include <sstream>
#include "database.h"

#define PI 3.14159265
#define BS 65288
#define SPACE 32
#define ENTER 65293
#define SHIFT 65505

using namespace std;

/***************************************************
 *                                                 *
 *               Global Variable                   *
 *                                                 *
 ***************************************************/

database my_database; // database is a class that contains all essential members for m1 and m2


/***************************************************
 *                                                 *
 *                Load the map                     *
 *                                                 *
 ***************************************************/

// This function allows the program to continuously display maps based on user input
// without relaunching it

void milestone2() {
    do {

        load_map_m2();
        draw_map();
        close_map();

    } while (my_database.switch_map == 1);
}

/*stores way index into a hash table. key: OSMID   value: way index*/
void load_OSMID_to_way_index() {
    /*traverse all ways*/
    for (unsigned long long i = 0; i < getNumberOfWays(); i++) {
        my_database.OSMID_to_way_index.insert(make_pair(getWayByIndex(i)->id(), i));
    }
}

/*stores node index into a hash table. key:OSMID  value:node index*/
void load_OSMID_to_node_index() {
    /*traverse all nodes*/
    for (unsigned long long i = 0; i < getNumberOfNodes(); i++) {
        my_database.OSMID_to_node_index.insert(make_pair(getNodeByIndex(i)->id(), i));
    }
}

/*stores subways into a vector*/
void load_subway() {
    for (unsigned long long i = 0; i < getNumberOfWays(); i++) {
        unsigned tag_count = getTagCount(getWayByIndex(i));
        string line_name;
        for (unsigned j = 0; j < tag_count; j++) {
            /*find tag pair with railway as the ket, subway as the value*/
            if (getTagPair(getWayByIndex(i), j).first == "railway") {
                if (getTagPair(getWayByIndex(i), j).second == "subway") {
                    vector<LatLon> v;
                    vector<string> s;
                    for (unsigned k = 0; k < tag_count; k++) {
                        if (getTagPair(getWayByIndex(i), k).first == "name") {
                            line_name = getTagPair(getWayByIndex(i), k).second;
                        }
                    }
                    for (unsigned k = 0; k < getWayByIndex(i)->ndrefs().size(); k++) {
                        unsigned long long index = my_database.OSMID_to_node_index.find(getWayByIndex(i)->ndrefs()[k])->second;
                        v.push_back(getNodeByIndex(index)->coords());
                        s.push_back(line_name);
                    }
                    my_database.subway.push_back(v);
                    my_database.line.push_back(s);

                }
                break;
            }
        }

    }
}

/*store types of features into a hash table key:feature id   value:type*/
void load_featureID_to_OSMtype() {
    for (unsigned i = 0; i < getNumberOfFeatures(); i++) {
        if (getFeatureType(i) == Building) {
            OSMID id = getFeatureOSMID(i);
            unsigned long long index = my_database.OSMID_to_way_index.find(id)->second;
            unsigned num = getTagCount(getWayByIndex(index));

            for (unsigned j = 0; j < num; j++) {
                if (getTagPair(getWayByIndex(index), j).first == "building") {
                    my_database.featureID_to_OSMtype.insert(make_pair(i, getTagPair(getWayByIndex(index), j).second));
                } else if (getTagPair(getWayByIndex(index), j).first == "aeroway") {
                    my_database.featureID_to_aeroway.insert(make_pair(i, getTagPair(getWayByIndex(index), j).second));
                } else if (getTagPair(getWayByIndex(index), j).first == "amenity") {
                    //cout << getTagPair(getWayByIndex(index), j).second << endl;
                    if (getTagPair(getWayByIndex(index), j).second == "clinic"
                            || getTagPair(getWayByIndex(index), j).second == "hospital") {
                        my_database.featureID_to_hospital.insert(make_pair(i, getTagPair(getWayByIndex(index), j).second));
                    }
                }
            }

        }
    }
}

/*stores the subway stations and names*/
void load_subway_station() {
    for (unsigned long long i = 0; i < getNumberOfNodes(); i++) {
        unsigned tag_count = getTagCount(getNodeByIndex(i));
        for (unsigned j = 0; j < tag_count; j++) {
            if (getTagPair(getNodeByIndex(i), j).first == "railway"
                    && getTagPair(getNodeByIndex(i), j).second == "subway_entrance") {
                LatLon position = getNodeByIndex(i)->coords();
                my_database.subway_station.push_back(position);
                for (unsigned k = 0; k < tag_count; k++) {
                    if (getTagPair(getNodeByIndex(i), k).first == "name") {
                        unsigned index = my_database.subway_station.size() - 1;
                        my_database.subway_station_name.insert(make_pair(index, getTagPair(getNodeByIndex(i), k).second));
                    }
                }
            }
        }
    }
}

/* This function will get the tag of every street segments*/
void load_streetsegs_type() {
    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
        OSMID id = getStreetSegmentInfo(i).wayOSMID;
        unsigned long long index = my_database.OSMID_to_way_index.find(id)->second;
        unsigned num = getTagCount(getWayByIndex(index));
        for (unsigned j = 0; j < num; j++) {
            if (getTagPair(getWayByIndex(index), j).first == "highway") {
                my_database.segs_type.push_back(getTagPair(getWayByIndex(index), j).second);
            }
        }
    }
}

/*function used to store layers of features*/
void load_layer() {
    for (unsigned j = 0; j < 5; j++) {
        my_database.layer.push_back({});
    }
    double max = 0;
    for (unsigned i = 0; i < getNumberOfFeatures(); i++) {
        if (getFeatureType(i) == Lake) {
            double area = compute_poly_area(i);
            if (abs(area) > max) {
                max = area;
            }
            /*draw big lakes first*/
            if (abs(area) >= 1e+6) {
                my_database.layer[0].push_back(i);
            } else {
                my_database.layer[3].push_back(i);
            }
        } else if (getFeatureType(i) == Island) {
            my_database.layer[1].push_back(i);
        } else if (getFeatureType(i) == Greenspace || getFeatureType(i) == Park || getFeatureType(i) == Golfcourse) {
            my_database.layer[2].push_back(i);
        } else {
            my_database.layer[3].push_back(i);
        }
    }


}

void initialize_m3_data() {
    for (unsigned i = 0; i < getNumberOfIntersections(); i++) {
        my_database.visited.push_back(0);
        my_database.closest_intersections.push_back(0);
        my_database.time.push_back(INF);
        vector<unsigned> v;
        my_database.path.push_back(v);
        my_database.come_from_ints.push_back(0);
        my_database.come_from_segs.push_back(0);

    }
}

/*this function loads the database we need in m2*/
bool load_map_m2() {
    bool load_success_1, load_success_2;
    database new_database;
    my_database = new_database;
    while (1) {

        init_graphics("Map Menu", WHITE);
        set_visible_world(0, 0, 100, 100);
        set_keypress_input(true);
        set_mouse_move_input(true);

        event_loop(act_on_mouse_move, NULL, NULL, initialize);

        //cout << " Please input map name from toronto,cairo,hamilton,moscow,new_york,saint_helena,toronto_cananda :";
        //cin>>name;
        if (my_database.name == "toronto") {
            my_database.city_name = "Toronto";
            close_graphics();
            return load_map("/cad2/ece297s/public/maps/toronto.streets.bin");
        } else if (my_database.name == "cairo") {
            my_database.city_name = "Cairo";
            close_graphics();
            return load_map("/cad2/ece297s/public/maps/cairo_egypt.streets.bin");
        } else if (my_database.name == "hamilton") {
            my_database.city_name = "Hamilton";
            close_graphics();
            return load_map("/cad2/ece297s/public/maps/hamilton_canada.streets.bin");
        } else if (my_database.name == "moscow") {
            my_database.city_name = "Moscow";
            close_graphics();
            return load_map("/cad2/ece297s/public/maps/moscow.streets.bin");
        } else if (my_database.name == "newyork") {
            my_database.city_name = "New York City";
            close_graphics();
            return load_map("/cad2/ece297s/public/maps/newyork.streets.bin");
        } else if (my_database.name == "saint_helena") {
            my_database.city_name = "Saint Helena";
            close_graphics();
            return load_map("/cad2/ece297s/public/maps/saint_helena.streets.bin");
        } else if (my_database.name == "toronto_canada") {
            my_database.city_name = "Great Toronto Area";
            close_graphics();
            return load_map("/cad2/ece297s/public/maps/toronto_canada.streets.bin");
        }
    }


}

void a() {
    my_database.zoom_level = 0;
    double lat_min = 90, lat_max = -90, lon_min = 180, lon_max = -180;
    for (unsigned i = 0; i < getNumberOfFeatures(); i++) {
        for (unsigned j = 0; j < getFeaturePointCount(i); j++) {


            LatLon position = getFeaturePoint(i, j);
            if (position.lat < lat_min) lat_min = position.lat;
            if (position.lat > lat_max) lat_max = position.lat;
            if (position.lon < lon_min) lon_min = position.lon;
            if (position.lon > lon_max) lon_max = position.lon;
        }
    }

    double lat_diff = lat_max - lat_min;
    double lon_diff = lon_max - lon_min;
    my_database.avg_lat = (lat_max + lat_min) / 2;
    lat_max = lat_max + lat_diff * 0.05;
    lat_min = lat_min - lat_diff * 0.05;
    lon_max = lon_max + lon_diff * 0.05;
    lon_min = lon_min - lon_diff * 0.05;
    t_point p1 = latlonProjection(LatLon(lat_min, lon_min));
    t_point p2 = latlonProjection(LatLon(lat_max, lon_max));
    my_database.left = p1.x;
    my_database.right = p2.x;
    my_database.top = p2.y;
    my_database.bottom = p1.y;
}

/*draw map function*/
void draw_map() {
    init_graphics(my_database.city_name, t_color(233, 229, 220));
    my_database.zoom_level = 0;
    double lat_min = 90, lat_max = -90, lon_min = 180, lon_max = -180;
    for (unsigned i = 0; i < getNumberOfFeatures(); i++) {
        for (unsigned j = 0; j < getFeaturePointCount(i); j++) {


            LatLon position = getFeaturePoint(i, j);
            if (position.lat < lat_min) lat_min = position.lat;
            if (position.lat > lat_max) lat_max = position.lat;
            if (position.lon < lon_min) lon_min = position.lon;
            if (position.lon > lon_max) lon_max = position.lon;
        }
    }

    double lat_diff = lat_max - lat_min;
    double lon_diff = lon_max - lon_min;
    my_database.avg_lat = (lat_max + lat_min) / 2;
    lat_max = lat_max + lat_diff * 0.05;
    lat_min = lat_min - lat_diff * 0.05;
    lon_max = lon_max + lon_diff * 0.05;
    lon_min = lon_min - lon_diff * 0.05;
    t_point p1 = latlonProjection(LatLon(lat_min, lon_min));
    t_point p2 = latlonProjection(LatLon(lat_max, lon_max));
    //cout << p1.x << endl << p1.y << endl << p2.x << endl << p2.y;
    my_database.initial_coords = t_bound_box(p1.x, p1.y, p2.x, p2.y);
    set_visible_world(my_database.initial_coords);
    update_message("beta 1.0");
    create_button("Window", "---2", NULL);

    create_button("---2", "Search St.", act_on_search_st_button_func);
    create_button("Search St.", "Search POI", act_on_search_poi_button_func);
    create_button("Search POI", "Search Int", act_on_search_int_button_func);
    create_button("Search Int", "Find int", act_on_find_ints_button_func);
    create_button("Find int", "Find Path", act_on_find_path_func);
    create_button("Find Path", "Clear", act_on_clear_button_func);
    create_button("Clear", "---3", NULL);

    create_button("---3", "POI name", act_on_interests_button_func);
    create_button("POI name", "Food", act_on_food_button_func);
    create_button("Food", "Parking", act_on_parking_button_func);
    create_button("Parking", "Bank", act_on_bank_button_func);
    create_button("Bank", "Subway", act_on_draw_subway_button);
    create_button("Subway", "---4", NULL);

    create_button("---4", "Switch map", act_on_switch_map_button);
    create_button("Switch map", "Help", act_on_help_button);
    set_keypress_input(true);
    set_mouse_move_input(true);


    event_loop(act_on_button_press, NULL, act_on_key_press, visualize);
    close_graphics();
}

/*draw the page used to help users to select map*/
void initialize() {
    clearscreen();
    t_point center(50, 80);
    setfontsize(30);
    setcolor(BLACK);
    drawtext(center, "Please select map name :", 100000, 100000);
    //drawrect(20,60,80,70);
    t_point center1(50, 70);
    t_point center2(50, 60);
    t_point center3(50, 50);
    t_point center4(50, 40);
    t_point center5(50, 30);
    t_point center6(50, 20);
    t_point center7(50, 10);

    drawrect(center1.x - 27, center1.y - 4, center1.x + 27, center1.y + 4);
    if (my_database.color == 1) {
        setcolor(RED);
        fillrect(center1.x - 27, center1.y - 4, center1.x + 27, center1.y + 4);
        setcolor(BLACK);
    }
    drawtext(center1, "Toronto", 100000, 100000);

    drawrect(center2.x - 27, center2.y - 4, center2.x + 27, center2.y + 4);
    if (my_database.color == 2) {
        setcolor(RED);
        fillrect(center2.x - 27, center2.y - 4, center2.x + 27, center2.y + 4);
        setcolor(BLACK);
    }
    drawtext(center2, "Hamilton", 100000, 100000);

    drawrect(center3.x - 27, center3.y - 4, center3.x + 27, center3.y + 4);
    if (my_database.color == 3) {
        setcolor(RED);
        fillrect(center3.x - 27, center3.y - 4, center3.x + 27, center3.y + 4);
        setcolor(BLACK);
    }
    drawtext(center3, "Cairo", 100000, 100000);

    drawrect(center4.x - 27, center4.y - 4, center4.x + 27, center4.y + 4);
    if (my_database.color == 4) {
        setcolor(RED);
        fillrect(center4.x - 27, center4.y - 4, center4.x + 27, center4.y + 4);
        setcolor(BLACK);
    }
    drawtext(center4, "Moscow", 100000, 100000);

    drawrect(center5.x - 27, center5.y - 4, center5.x + 27, center5.y + 4);
    if (my_database.color == 5) {
        setcolor(RED);
        fillrect(center5.x - 27, center5.y - 4, center5.x + 27, center5.y + 4);
        setcolor(BLACK);
    }
    drawtext(center5, "New York City", 100000, 100000);

    drawrect(center6.x - 27, center6.y - 4, center6.x + 27, center6.y + 4);
    if (my_database.color == 6) {
        setcolor(RED);
        fillrect(center6.x - 27, center6.y - 4, center6.x + 27, center6.y + 4);
        setcolor(BLACK);
    }
    drawtext(center6, "Saint Helena", 100000, 100000);

    drawrect(center7.x - 27, center7.y - 4, center7.x + 27, center7.y + 4);
    if (my_database.color == 7) {
        setcolor(RED);
        fillrect(center7.x - 27, center7.y - 4, center7.x + 27, center7.y + 4);
        setcolor(BLACK);
    }
    drawtext(center7, "Great Toronto Area", 100000, 100000);
}

/*this function draws everything*/
void visualize() {
    /*clean screen first*/
    clearscreen();
    //draw segments
    //setcolor(1);
    //setlinewidth(3);
    /*use get_visible_world to calculate current zoom level*/
    my_database.cur_coords = get_visible_world();
    double cur_x_diff = my_database.cur_coords.right() - my_database.cur_coords.left();
    double initial_x_diff = my_database.initial_coords.right() - my_database.initial_coords.left();
    my_database.zoom_level = log(cur_x_diff / initial_x_diff) / log(0.6);

    /*draw different parts of the map in a reasonable order*/
    /*section for drawing features*/
    draw_features();
    /*section for drawing streets*/
    draw_streets_intersections_interests();

    /*section for drawing the city name*/
    draw_city_name();

    /*highlight*/

    /*section for drawing street names and arrows*/
    draw_arrows_stnames();

    draw_features_names();

    draw_interests_name();
    if (my_database.subway_flag == 1) {
        draw_subway();
        draw_subway_station();
    }
    redraw_highlights();
}

/*some helper functions*/

/*project latlon to t_point*/
t_point latlonProjection(LatLon position) {
    double x = 10000000 * position.lon * DEG_TO_RAD * cos(my_database.avg_lat * DEG_TO_RAD);
    double y = 10000000 * position.lat*DEG_TO_RAD;
    t_point res;
    res.x = x;
    res.y = y;
    return res;
}

/*project t_point to latlon */
LatLon xy_to_latlon(float x, float y) {
    LatLon position;
    position.lat = y / (DEG_TO_RAD * 10000000);
    position.lon = x / (10000000 * DEG_TO_RAD * cos(my_database.avg_lat * DEG_TO_RAD));
    return position;
}

/*Compute the area of a polygon*/
double compute_poly_area(unsigned i) {
    unsigned num = getFeaturePointCount(i);
    double area = 0;
    unsigned j = num - 1;
    vector<t_point> vertices;
    for (unsigned k = 0; k < num; k++) {
        vertices.push_back(latlonProjection(getFeaturePoint(i, k)));
    }
    double xcen = 0, ycen = 0;
    for (unsigned k = 0; k < num; k++) {
        xcen += vertices[k].x;
        ycen += vertices[k].y;
    }
    xcen = xcen / num;
    ycen = ycen / num;
    for (unsigned k = 0; k < num; k++) {
        vertices[k].x -= xcen;
        vertices[k].y -= ycen;
    }
    for (unsigned k = 0; k < num; k++) {
        area += (vertices[j].x + vertices[k].x)*(vertices[j].y - vertices[k].y);
        j = k;
    }
    return area / 2;
}

/*function used to check whether a point is inside a circle*/
bool point_inside_circle(float x, float y, unsigned i) {
    LatLon position = getIntersectionPosition(i);
    t_point center = latlonProjection(position);
    float distance = sqrt((x - center.x)*(x - center.x)+(y - center.y)*(y - center.y));
    if (distance >= 0 && distance < 15) {
        return true;
    }
    return false;
}

/*function used to check whether a point is inside the visible world*/
bool check_inside_visible_world(LatLon position, t_bound_box coords) {
    double x = latlonProjection(position).x;
    double y = latlonProjection(position).y;

    return (x < coords.right() && x > coords.left() && y > coords.bottom() && y < coords.top());
}

/*fucntion for checking whether a point is inside a polygon*/
bool pointInside_polygon(float x, float y, unsigned id) {

    unsigned count = getFeaturePointCount(id);
    t_point vertice[count];

    for (unsigned i = 0; i < count; i++) {
        vertice[i] = latlonProjection(getFeaturePoint(id, i));
    }

    int i, j, k = count;
    bool inside = false;

    for (i = 0, j = k - 1; i < k; j = i++) {
        if (((vertice[i].y >= y) != (vertice[j].y >= y)) &&
                (x <= (vertice[j].x - vertice[i].x) * (y - vertice[i].y) / (vertice[j].y - vertice[i].y) + vertice[i].x)
                ) {
            inside = !inside;
        }
    }
    return inside;
}

/*Get the vector between two points*/
t_point getVecBetweenPoints(t_point start, t_point end) {
    t_point vec;
    vec.x = end.x - start.x;
    vec.y = end.y - start.y;
    return (vec);
}

/********************************************************
 *                                                      *                                                      
 *           Interaction Functions                      *
 *                                                      *
 * ******************************************************/

/*void act_on_mouse_move(float x, float y) {
    cout << "here" << endl;
    cout << "mouse at " << x << ", " << y << endl;
}*/

void act_on_search_st_button_func(void (*drawscreen) (void)) {

    if (my_database.search_type != 0) {
        my_database.search = 1;
    } else {
        my_database.search = my_database.search xor 1;
    }
    my_database.search_type = 0;
    if (my_database.search == 0) {
        my_database.search_state = Begin;
        my_database.draw_flag_poi = 0;
        my_database.user_input = "";
    }
    visualize();
}

void act_on_search_poi_button_func(void (*drawscreen) (void)) {
    if (my_database.search_type != 1) {
        my_database.search = 1;
    } else {
        my_database.search = my_database.search xor 1;
    }
    my_database.search_type = 1;
    if (my_database.search == 0) {
        my_database.search_state = Begin;
        my_database.draw_flag_poi = 0;
        my_database.user_input = "";
    }
    visualize();
}

void act_on_search_int_button_func(void (*drawscreen) (void)) {
    if (my_database.search_type != 2) {
        my_database.search = 1;
    } else {
        my_database.search = my_database.search xor 1;
    }
    my_database.search_type = 2;
    if (my_database.search == 0) {
        my_database.search_state = Begin;
        my_database.draw_flag_poi = 0;
        my_database.user_input = "";
    }
    visualize();
}

/*
void draw_help() {
    double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
    double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
    double x1 = my_database.cur_coords.left() + xdiff / 12;
    double y1 = my_database.cur_coords.bottom() + ydiff / 12;
    double x2 = my_database.cur_coords.right() - xdiff / 12;
    double y2 = my_database.cur_coords.top() - ydiff / 12;
    if (my_database.help_state != Start) {
        setcolor(BLACK);
        fillrect(x1, y1, x2, y2);
    }
    if (my_database.help_state == Main) {
        t_point x_search()
    }
}*/

void act_on_help_button(void(*drawscreen) (void)) {
    my_database.help_state = Main;
    cout << "Welcome! This is a manual providing you with detailed instructions on how to use the map:" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Search:" << endl;
    cout << "   Upon click, a white search bar will appear at the top right corner of the map." << endl;
    cout << "   Auto Completion: After Entering part of a Street, POI, Intersection names, press \"Tab\" and the program will auto-complete the rest of the names. Pressing direction keys \"up\" and \"down\" will browse through possible choices" << endl;
    cout << "   Search for streets: The user can type in street names and the certain street will be highlighted on the map." << endl;
    cout << "   Search for POIs: The user can type in POI names and all existing POI will be displayed on the map, the user can also select an intersections and click 'Find Path' to get the path to the closest POI" << endl;
    cout << "   Search for Intersections: The user can type in a certain street name and browse through the possible choices to highlight the intersection, entering a street only will highlight all the intersections on that street" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Find ints:" << endl;
    cout << "   Type in two street names in the terminal and the according intersection will be highlighted on the map, if there is no intersection, an error message will be displayed." << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Clear: " << endl;
    cout << "   Clears all highlights on the map" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Find Path:" << endl;
    cout << "   Path highlight mode 1: Click or find by using 'Find ints' any intersections on the map, then click Find Path. The program will display the quickest path." << endl;
    cout << "                          The starting and destination intersections are determined according to the order of user's input." << endl;
    cout << "                          NOTE: if the number of selected intersections (either by clicking or 'Find ints') is less than 2, an error message will be on terminal." << endl;
    cout << "                                if the number is greater than 2, the program will take in the last two inputs." << endl;
    cout << "   Path highlight mode 2: if users have used the search bar to find POIs, and one intersection is selected (either by click or 'Find ints', then click 'Find Path' the program will display the path to the nearest POI." << endl;
    cout << "                          NOTE: if no intersection or more 2 intersections are selected, an error message will be on terminal." << endl;
    cout << "   Route output: The program will print the detailed driving instructions in the terminal" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "POI name:" << endl;
    cout << "   After pressing POI name button, the user is required to zoom in to a certain zoom level for the names to show up" << endl;
    cout << "   Reclick on them will erase the names" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Food, Parking, Bank:" << endl;
    cout << "   Pressing these buttons will draw according POI icons on the map, but requires the user to zoom in" << endl;
    cout << "   Reclick on them will erase the corresponding POIs" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Subway:" << endl;
    cout << "   By clicking this button, a map of the subway transit map of the current city will be displayed" << endl;
    cout << "   Reclick on them will erase the subways" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Switch map:" << endl;
    cout << "   The user can switch to a different map by pressing this button." << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << "Other useful information: " << endl;
    cout << "** Users can click on any intersections and buildings to extract related information on a separated info bar." << endl;
    cout << "   Reclick on intersections and buildings will erase highlights on the intersections and buildings, but to erase the info bar users need to click on somewhere other than intersections or buildings" << endl;
    cout << "** All highlights, except for the search bar, can be cleared by the 'Clear' button" << endl;
    cout << "** When inputing, users don't have to input the whole name of the object. Instead a partial name will trigger a name list for users to choose." << endl;
    //draw_help();

}

/*When the user clicks the find street button on the interface*/
void act_on_find_ints_button_func(void (*drawscreen) (void)) {
    my_database.draw_flag1 = 1;
    string name1 = "", name2 = "";
    vector<string> empty;
    /*get user inputs*/
    cout << "Please input first street name: ";
    while (1) {
        getline(cin, name1, '\n');
        if (name1 != "") {
            break;
        }
    }
    my_database.street_partial_match = empty;
    my_database.st_bst.find(name1);
    vector<string> possible_names = my_database.street_partial_match;
    if (possible_names.size() > 1) {
        cout << "Your input is : " << name1 << ".  Do you mean : " << endl;
        cout << "----------------------------------------------------------" << endl;
        for (unsigned i = 0; i < possible_names.size(); i++) {
            cout << i + 1 << ". " << possible_names[i] << endl;
        }
        cout << "----------------------------------------------------------" << endl;
        cout << "Please select among the names above: " << endl;
        name1 = "";
        while (1) {
            int num;
            getline(cin, name1, '\n');
            stringstream ss(name1);
            ss >> num;
            if (name1 != "") {
                if (ss.fail()) {
                    cout << "Please input a corresponding number" << endl;
                } else {
                    name1 = possible_names[num - 1];
                    break;
                }
            }
        }
    }
    cout << endl;
    cout << "-------------------------------------------------------------" << endl;
    cout << "please input second street name: ";
    while (1) {
        getline(cin, name2, '\n');
        if (name2 != "") {
            break;
        }
    }
    my_database.street_partial_match = empty;
    my_database.st_bst.find(name2);
    vector<string> possible_names2 = my_database.street_partial_match;
    if (possible_names2.size() > 1) {
        cout << "Your input is : " << name2 << ".  Do you mean : " << endl;
        cout << "----------------------------------------------------------" << endl;
        for (unsigned i = 0; i < possible_names2.size(); i++) {
            cout << i + 1 << ". " << possible_names2[i] << endl;
        }
        cout << "----------------------------------------------------------" << endl;
        cout << "Please select among the names above: " << endl;
        name2 = "";
        while (1) {
            int num;
            getline(cin, name2, '\n');
            stringstream ss(name2);
            ss >> num;
            if (name2 != "") {
                if (ss.fail()) {
                    cout << "Please input a corresponding number" << endl;
                } else {
                    name2 = possible_names2[num - 1];
                    break;
                }
            }
        }
    }
    cout << "-------------------------------------------------------------" << endl;
    /*store intersections into an array*/
    vector<unsigned> ids = find_intersection_ids_from_street_names(name1, name2);
    for (unsigned i = 0; i < ids.size(); i++) {
        cout << "Intersection " << i + 1 << " :" << endl;
        my_database.ids_array1.push_back(ids[i]);
        LatLon position = getIntersectionPosition(ids[i]);
        cout << "Longitude: " << position.lon << "  Latitude: " << position.lat << endl;
    }
    if (ids.empty() == 1) {
        cout << "Sorry, These two streets don't have an intersection" << endl << endl;
    } else {
        my_database.information_id = ids[0];
        my_database.information_flag = 1;
        vector<string> names = find_intersection_street_names(ids[0]);
        vector<string> street_names;
        for (auto iter = names.begin(); iter != names.end(); iter++) {
            if (find(street_names.begin(), street_names.end(), *iter) == street_names.end()) {
                street_names.push_back(*iter);
            }
        }
        double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
        double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
        double x1 = my_database.cur_coords.right() - xdiff / 4;
        double y1 = my_database.cur_coords.bottom() + ydiff / 2;
        setcolor(t_color(66, 133, 244));
        fillrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top() - ydiff / 20);
        t_point textcenter(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8);
        setcolor(WHITE);
        setfontsize(10);
        drawtext(textcenter, "Streets crossing this intersection: ", 1000000000, 10000000000);
        int size = street_names.size();
        for (int i = 0; i < size; i++) {
            t_point my_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (i + 1) * ydiff / (size + 3));
            drawtext(my_center, to_string(i + 1) + ". " + street_names[i], 100000, 10000);
        }

        my_database.latlon_information.lat = getIntersectionPosition(ids[0]).lat;
        my_database.latlon_information.lon = getIntersectionPosition(ids[0]).lon;
        if (my_database.inter_search_record.size() < 2) {
            my_database.inter_search_record.push_back(ids[0]);
        } else {
            my_database.inter_search_record.erase(my_database.inter_search_record.begin());
            my_database.inter_search_record.push_back(ids[0]);
        }
        cout << "----------------------------------------------------------" << endl;
        cout << "Location selected. Please go back to graphics." << endl;
        cout << "----------------------------------------------------------" << endl;
    }


    /*print informations of intersections*/

}

/*When the user clicks the clear highlights button on the interface*/
void act_on_clear_button_func(void (*drawscreen) (void)) {
    /*reset all flags to clean highlights*/
    my_database.draw_flag1 = 0;
    my_database.ids_array1 = {};
    my_database.inter_search_record = {};
    my_database.draw_flag2 = 0;
    my_database.ids_array2 = {};
    my_database.draw_flag3 = 0;
    my_database.ids_array3 = {};
    my_database.segment_ids_array = {};
    my_database.draw_flag4 = 0;
    my_database.information_flag = 0;
    my_database.draw_flag_path = 0;
    my_database.ids_path = {};
    my_database.draw_flag_poi = 0;
    visualize();
}

void act_on_find_path_func(void (*drawscreen) (void)) {
    //my_database.draw_flag1 = 0;
    //my_database.ids_array1 = {};
    //my_database.draw_flag2 = 0;
    //my_database.ids_array2 = {};
    my_database.draw_flag3 = 0;
    my_database.ids_array3 = {};
    my_database.segment_ids_array = {};
    my_database.draw_flag4 = 0;
    my_database.information_flag = 0;
    my_database.draw_flag_path = 0;
    visualize();

    if ((my_database.search_state == Begin || my_database.search_type == 2)
            && my_database.draw_flag_poi == 0) {
        cout << "-------------------------------------------------------------------------------------------" << endl;
        cout << "Finding the quickest path between the two locations selected: " << endl << endl;
        if (my_database.inter_search_record.size() == 2) {
            my_database.inter_id1 = my_database.inter_search_record[0];
            my_database.inter_id2 = my_database.inter_search_record[1];
            my_database.ids_path = find_path_between_intersections(my_database.inter_id1, my_database.inter_id2);
            my_database.draw_flag_path = 1;
            my_database.draw_flag1 = 0;
            my_database.ids_array1 = {};
            my_database.draw_flag2 = 0;
            my_database.ids_array2 = {};
            visualize();
        } else if (my_database.inter_search_record.size() < 2) {
            cout << "You have selected too many or too few locations, please select only two." << endl << endl;
        }
    } else {
        if (my_database.draw_flag_poi == 1) {
            cout << "--------------------------------------------------------------------------------------" << endl;
            cout << "Finding the closest " << my_database.user_input << " near your starting location: " << endl << endl;
            if (my_database.inter_search_record.size() == 1) {
                my_database.inter_id1 = my_database.inter_search_record[0];
                my_database.ids_path = find_path_to_point_of_interest(my_database.inter_id1, my_database.user_input);
                my_database.draw_flag_path = 1;
                visualize();
            } else {
                cout << "You have selected too many or too few starting locations, please select only one." << endl << endl;

            }
        }
    }
    print_path_name();
}

/*When the user clicks the show interests button on the interface*/
void act_on_interests_button_func(void (*drawscreen) (void)) {
    /*change the show interests name flag*/
    my_database.show_interests_name = my_database.show_interests_name xor 1;
    visualize();
}

void act_on_food_button_func(void (*drawscreen) (void)) {
    my_database.food = my_database.food xor 1;
    visualize();
}

void act_on_parking_button_func(void (*drawscreen) (void)) {
    my_database.parking = my_database.parking xor 1;
    visualize();
}

void act_on_bank_button_func(void (*drawscreen) (void)) {
    my_database.bank = my_database.bank xor 1;
    visualize();
}

/*When the user clicks the find streets button on the interface*/
void act_on_find_streets_button(void (*drawscreen) (void)) {
    /*get user input*/
    string name;
    cout << "Please input street name: ";
    while (1) {
        getline(cin, name, '\n');
        if (name != "") {
            break;
        }
    }
    vector<unsigned> street_ids = find_street_ids_from_name(name);
    /*store ids of street segments into an array*/
    for (auto iter = street_ids.begin(); iter != street_ids.end(); iter++) {
        vector<unsigned> ids = find_street_street_segments(*iter);
        my_database.segment_ids_array.insert(my_database.segment_ids_array.end(), ids.begin(), ids.end());
    }

    if (street_ids.empty() == 1) {
        cout << "Sorry, wrong street name" << endl;
    } else {
        my_database.draw_flag4 = 1;
    }
}

/*draws subway when the user clicks the draw subway button*/
void act_on_draw_subway_button(void (*drawscreen) (void)) {
    /*change the draw subway flag*/
    my_database.subway_flag = my_database.subway_flag xor 1;
    visualize();
}

/*allow users to switch map without rerunning the program*/
void act_on_switch_map_button(void (*drawscreen) (void)) {
    /*enable to switch the map*/
    my_database.switch_map = 1;
    my_database.draw_flag_path = 0;

    //visualize();
}

#include <X11/keysym.h>

/*get user inputs*/

void act_on_key_press(char c, int keysym) {
    // function to handle keyboard press event, the ASCII character is returned
    // along with an extended code (keysym) on X11 to represent non-ASCII
    // characters like the arrow keys.
    vector<string> empty;

    if (my_database.search_state == Click) {
        if (c != '\0') {
            my_database.search_state = Input;
            my_database.user_input.push_back(c);
        }
    } else if (my_database.search_state == Input) {
        if (keysym == BS) {
            if (my_database.user_input.length() != 0) {
                my_database.user_input.pop_back();
            }
        } else if (keysym == SPACE) {
            my_database.user_input.push_back(' ');
        } else if (keysym == ENTER) {
            my_database.search_state = Enter;
        } else if (keysym == 65289) {
            my_database.change_flag = 1;
            my_database.possible_names = empty;
            my_database.index = 0;
            my_database.street_partial_match = empty;
            my_database.poi_partial_match = empty;
            my_database.intersection_partial_match = empty;

            if (my_database.search_type == 0) {
                my_database.st_bst.find(my_database.user_input);

                my_database.possible_names = my_database.street_partial_match;

            } else if (my_database.search_type == 1) {
                my_database.poi_bst.find(my_database.user_input);

                my_database.possible_names = my_database.poi_partial_match;

            } else {
                my_database.int_bst.find(my_database.user_input);
                my_database.possible_names = my_database.intersection_partial_match;
            }


            if (my_database.possible_names.size() != 0) {
                my_database.user_input = my_database.possible_names[0];
            }
        } else if (keysym == 65362 && my_database.change_flag == 1) {
            if (my_database.index == 0) {
                my_database.index = my_database.possible_names.size() - 1;
            } else {
                my_database.index -= 1;
            }
            if (my_database.possible_names.size() != 0) my_database.user_input = my_database.possible_names[my_database.index];
            //if(my_database.index)
        } else if (keysym == 65364 && my_database.change_flag == 1) {
            if (my_database.index == my_database.possible_names.size() - 1) {
                my_database.index = 0;
            } else {
                my_database.index += 1;
            }
            if (my_database.possible_names.size() != 0) my_database.user_input = my_database.possible_names[my_database.index];

            //if(my_database.index)
        } else if (keysym > 65000) {

        } else {
            my_database.user_input.push_back(c);
        }
    }
    if (my_database.search_state == Enter) {
        my_database.street_partial_match = empty;
        my_database.poi_partial_match = empty;
        my_database.intersection_partial_match = empty;
        if (my_database.search_type == 0) {
            my_database.st_bst.find(my_database.user_input);
            my_database.possible_names = my_database.street_partial_match;
        } else if (my_database.search_type == 1) {
            my_database.poi_bst.find(my_database.user_input);
            my_database.possible_names = my_database.poi_partial_match;
        } else {
            my_database.int_bst.find(my_database.user_input);
            my_database.possible_names = my_database.intersection_partial_match;

        }
        vector<unsigned> street_ids = find_street_ids_from_name(my_database.user_input);
        auto iter = my_database.intname_to_id.find(my_database.user_input);
        vector<unsigned> int_ids = {};
        if (iter != my_database.intname_to_id.end()) {
            int_ids = iter->second;
        }
        if (my_database.possible_names.size() > 1) {

            if ((my_database.search_type == 0 && street_ids.size() == 0) || (my_database.search_type == 1 &&
                    my_database.POIname_to_id.find(my_database.user_input) == my_database.POIname_to_id.end())
                    || (my_database.search_type == 2 && int_ids.size() == 0)) {


                if (my_database.search_type == 0) {
                    cout << "Your input is: " << my_database.user_input << ".  Do you mean these streets: " << endl;
                } else if (my_database.search_type == 1) {
                    cout << "Your input is: " << my_database.user_input << ".  Do you mean these point of interests: " << endl;
                } else {
                    cout << "Your input is: " << my_database.user_input << ".  Do you mean these intersections: " << endl;
                }
                cout << "----------------------------------------------------------" << endl;
                for (unsigned i = 0; i < my_database.possible_names.size(); i++) {
                    cout << i + 1 << ". " << my_database.possible_names[i] << endl;
                }
                cout << "----------------------------------------------------------" << endl;
                cout << "Please select among the names above: " << endl;
                string temp;
                while (1) {
                    unsigned num;
                    getline(cin, temp, '\n');
                    stringstream ss(temp);
                    ss >> num;
                    if (temp != "") {
                        if (ss.fail()) {
                            cout << "Please input a corresponding number" << endl;
                        } else if (num > my_database.possible_names.size()+1) {
                            cout << "Error: Wrong number. Please reselect." << endl;
                            continue;
                        }
                        else {
                            
                            temp = my_database.possible_names[num - 1];

                            break;
                        }
                    }
                }
                my_database.user_input = temp;
                cout << "----------------------------------------------------------" << endl;
                cout << "Location selected. Please go back to graphics." << endl;
                cout << "----------------------------------------------------------" << endl;
            }

        } else if (my_database.possible_names.size() == 1) {
            my_database.user_input = my_database.possible_names[0];
        } else {
            cout << "No Results" << endl << endl;
            redraw_highlights();
            return;
        }

        if (my_database.search_type == 0) {
            vector<unsigned> street_ids = find_street_ids_from_name(my_database.user_input);
            for (auto iter = street_ids.begin(); iter != street_ids.end(); iter++) {
                vector<unsigned> ids = find_street_street_segments(*iter);
                my_database.segment_ids_array.insert(my_database.segment_ids_array.end(), ids.begin(), ids.end());
            }
            if (street_ids.size() == 0) {
                cout << "No Results" << endl << endl;
            } else {
                my_database.draw_flag4 = 1;
            }

        } else if (my_database.search_type == 1) {
            if (my_database.POIname_to_id.find(my_database.user_input) == my_database.POIname_to_id.end()) {
                cout << "No Results" << endl << endl;
            } else {

                my_database.draw_flag_poi = 1;
                visualize();
            }

        } else {
            if (my_database.intname_to_id.find(my_database.user_input) == my_database.intname_to_id.end()) {
                cout << "No Results" << endl << endl;
            } else {
                my_database.draw_flag1 = 1;
                vector<unsigned> ids = my_database.intname_to_id[my_database.user_input];
                for (unsigned i = 0; i < ids.size(); i++) {
                    cout << "Intersection " << i + 1 << " :" << endl;
                    my_database.ids_array1.push_back(ids[i]);
                    LatLon position = getIntersectionPosition(ids[i]);
                    cout << "Longitude: " << position.lon << "  Latitude: " << position.lat << endl;
                }
                my_database.information_id = ids[0];
                my_database.information_flag = 1;
                vector<string> names = find_intersection_street_names(ids[0]);
                vector<string> street_names;
                for (auto iter = names.begin(); iter != names.end(); iter++) {
                    if (find(street_names.begin(), street_names.end(), *iter) == street_names.end()) {
                        street_names.push_back(*iter);
                    }
                }
                double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
                double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
                double x1 = my_database.cur_coords.right() - xdiff / 4;
                double y1 = my_database.cur_coords.bottom() + ydiff / 2;
                setcolor(t_color(66, 133, 244));
                fillrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top() - ydiff / 20);
                t_point textcenter(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8);
                setcolor(WHITE);
                setfontsize(10);
                drawtext(textcenter, "Streets crossing this intersection: ", 1000000000, 10000000000);
                int size = street_names.size();
                for (int i = 0; i < size; i++) {
                    t_point my_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (i + 1) * ydiff / (size + 3));
                    drawtext(my_center, to_string(i + 1) + ". " + street_names[i], 100000, 10000);
                }

                my_database.latlon_information.lat = getIntersectionPosition(ids[0]).lat;
                my_database.latlon_information.lon = getIntersectionPosition(ids[0]).lon;
                if (my_database.inter_search_record.size() < 2) {
                    my_database.inter_search_record.push_back(ids[0]);
                } else {
                    my_database.inter_search_record.erase(my_database.inter_search_record.begin());
                    my_database.inter_search_record.push_back(ids[0]);
                }
            }
        }
        my_database.index = 0;
        my_database.change_flag = 0;


    }
    //visualize();
    redraw_highlights();
}

void act_on_mouse_move(float x, float y, t_event_buttonPressed button_info) {
    t_point center1(50, 70);
    t_point center2(50, 60);
    t_point center3(50, 50);
    t_point center4(50, 40);
    t_point center5(50, 30);
    t_point center6(50, 20);
    t_point center7(50, 10);
    if (x > center1.x - 27 && x < center1.x + 27 && y > center1.y - 4 && y < center1.y + 4) {
        my_database.name = "toronto";
        my_database.color = 1;
        initialize();
    } else if (x > center2.x - 27 && x < center2.x + 27 && y > center2.y - 4 && y < center2.y + 4) {
        my_database.name = "hamilton";
        my_database.color = 2;
        initialize();
    } else if (x > center3.x - 27 && x < center3.x + 27 && y > center3.y - 4 && y < center3.y + 4) {
        my_database.name = "cairo";
        my_database.color = 3;
        initialize();
    } else if (x > center4.x - 27 && x < center4.x + 27 && y > center4.y - 4 && y < center4.y + 4) {
        my_database.name = "moscow";
        my_database.color = 4;
        initialize();
    } else if (x > center5.x - 27 && x < center5.x + 27 && y > center5.y - 4 && y < center5.y + 4) {
        my_database.name = "newyork";
        my_database.color = 5;
        initialize();
    } else if (x > center6.x - 27 && x < center6.x + 27 && y > center6.y - 4 && y < center6.y + 4) {
        my_database.name = "saint_helena";
        my_database.color = 6;
        initialize();
    } else if (x > center7.x - 27 && x < center7.x + 27 && y > center7.y - 4 && y < center7.y + 4) {
        my_database.name = "toronto_canada";
        my_database.color = 7;
        initialize();
    } else {

        my_database.name = "";
        my_database.color = 0;
        initialize();
    }
}

/*actions done when user clicks on screen*/
void act_on_button_press(float x, float y, t_event_buttonPressed event) {
    /*show latlon position when user click on the map*/
    //cout << "Longitude: " << xy_to_latlon(x, y).lon << ", Latitude: " << xy_to_latlon(x, y).lat << endl << endl;
    t_point center;
    center.x = x;
    center.y = y;
    setcolor(RED);
    double xdiff1 = my_database.cur_coords.right() - my_database.cur_coords.left();
    double ydiff1 = my_database.cur_coords.top() - my_database.cur_coords.bottom();
    double x11 = my_database.cur_coords.right() - xdiff1 / 2;
    double y11 = my_database.cur_coords.top() - ydiff1 / 25;
    if (!(x > x11 && x < my_database.cur_coords.right() && y > y11 && y < my_database.cur_coords.top())) {
        my_database.information_flag = 0;
    }

    visualize();
    //if(zoom_level>7){
    //fillarc(center, 5, 0, 360);
    //}
    if (my_database.search == 1) {
        double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
        double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
        double x1 = my_database.cur_coords.right() - xdiff / 2;
        double y1 = my_database.cur_coords.top() - ydiff / 25;
        if (my_database.search_state == Begin) {
            if (x > x1 && x < my_database.cur_coords.right() && y > y1 && y < my_database.cur_coords.top()) {
                my_database.search_state = Click;
                redraw_highlights();
            }
        } else if (my_database.search_state == Enter) {
            if (x > x1 && x < my_database.cur_coords.right() && y > y1 && y < my_database.cur_coords.top()) {
                my_database.user_input = "";
                my_database.search_state = Input;
                my_database.draw_flag_poi = 0;
                redraw_highlights();
            }
        }
        if (!(x > x1 && x < my_database.cur_coords.right() && y > y1 && y < my_database.cur_coords.top())) {
            my_database.search_state = Begin;
            redraw_highlights();
        }
    }

    if (!(x > x11 && x < my_database.cur_coords.right() && y > y11 && y < my_database.cur_coords.top())) {
        /*highlight the intersection and show informations of the intersection when user clicks on it*/
        for (unsigned i = 0; i < getNumberOfIntersections(); i++) {

            if (point_inside_circle(x, y, i)) {
                auto iter2 = find(my_database.inter_search_record.begin(), my_database.inter_search_record.end(), i);
                if (iter2 != my_database.inter_search_record.end()) {
                    my_database.inter_search_record.erase(iter2);
                } else {
                    if (my_database.inter_search_record.size() < 2) {
                        my_database.inter_search_record.push_back(i);
                    } else {
                        my_database.inter_search_record.erase(my_database.inter_search_record.begin());
                        my_database.inter_search_record.push_back(i);
                    }
                }
                /*remove highlight when click on an intersection again*/
                auto iter = find(my_database.ids_array2.begin(), my_database.ids_array2.end(), i);
                if (iter != my_database.ids_array2.end()) {
                    my_database.ids_array2.erase(iter);
                    visualize();
                    continue;
                }

                my_database.information_flag = 1;
                my_database.information_id = i;
                my_database.draw_flag2 = 1;
                my_database.ids_array2.push_back(i);
                center.x = latlonProjection(getIntersectionPosition(i)).x;
                center.y = latlonProjection(getIntersectionPosition(i)).y;
                /*hightlight this intersection*/
                setcolor(RED);
                //fillarc(center, 15 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
                draw_position_icon(center, 15 * pow(0.8, (my_database.zoom_level - 7)) + 5);
                vector<string> names = find_intersection_street_names(i);
                vector<string> street_names;
                for (auto iter = names.begin(); iter != names.end(); iter++) {
                    if (find(street_names.begin(), street_names.end(), *iter) == street_names.end()) {
                        street_names.push_back(*iter);
                    }
                }

                /*show information of this intersection in info bar*/
                double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
                double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
                double x1 = my_database.cur_coords.right() - xdiff / 4;
                double y1 = my_database.cur_coords.bottom() + ydiff / 2;
                setcolor(t_color(66, 133, 244));
                fillrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top() - ydiff / 20);
                t_point textcenter(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8);
                setcolor(WHITE);
                setfontsize(12);
                drawtext(textcenter, "Streets crossing this intersection: ", 1000000000, 10000000000);
                setfontsize(15);
                int size = street_names.size();
                for (int i = 0; i < size; i++) {
                    t_point my_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (i + 1) * ydiff / (size + 3));
                    drawtext(my_center, to_string(i + 1) + ". " + street_names[i], 100000, 10000);
                }
                t_point lon_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (size + 1) * ydiff / (size + 3));
                t_point lat_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (size + 2) * ydiff / (size + 3));
                my_database.latlon_information.lat = xy_to_latlon(x, y).lat;
                my_database.latlon_information.lon = xy_to_latlon(x, y).lon;
                setfontsize(10);
                drawtext(lon_center, "Longitude: " + to_string(xy_to_latlon(x, y).lon), 1000000, 1000000);
                drawtext(lat_center, "Latitude: " + to_string(xy_to_latlon(x, y).lat), 1000000, 1000000);
            }
        }
        /*highlight the building and show informations of the building when user clicks on it*/
        if (my_database.zoom_level >= 6) {
            for (unsigned i = 0; i < getNumberOfFeatures(); i++) {
                if (getFeatureType(i) == Building) {
                    if (pointInside_polygon(x, y, i)) {
                        /*remove highlight when click on a building again */
                        auto iter = find(my_database.ids_array3.begin(), my_database.ids_array3.end(), i);
                        if (iter != my_database.ids_array3.end()) {
                            my_database.ids_array3.erase(iter);
                            visualize();
                            continue;
                        }
                        /*highlight this building*/
                        my_database.information_flag = 2;
                        my_database.information_id = i;
                        my_database.draw_flag3 = 1;
                        my_database.ids_array3.push_back(i);
                        unsigned point_count = getFeaturePointCount(i);
                        t_point coord[point_count];

                        for (unsigned j = 0; j < point_count; j++) {

                            coord[j] = latlonProjection(getFeaturePoint(i, j));
                        }
                        bool ispoly = (coord[0].x == coord[point_count - 1].x)&&(coord[0].y == coord[point_count - 1].y);
                        setcolor(RED);
                        if (ispoly) {
                            fillpoly(coord, point_count);
                        } else {
                            for (unsigned j = 0; j < point_count - 1; j++) {
                                drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                            }
                        }
                        string name = getFeatureName(i);
                        if (name == "<noname>") {

                            name = "unknown building";
                        }
                        /*show information of this building in info bar*/
                        double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
                        double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
                        double x1 = my_database.cur_coords.right() - xdiff / 4;
                        double y1 = my_database.cur_coords.bottom() + ydiff / 2;
                        setcolor(t_color(66, 133, 244));
                        fillrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top() - ydiff / 20);
                        t_point textcenter(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8);
                        setcolor(WHITE);
                        setfontsize(12);
                        drawtext(textcenter, "Building name:", 100000, 100000);
                        t_point name_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 4);
                        setfontsize(10);
                        drawtext(name_center, name, 100000, 100000);
                        t_point lon_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - 3 * ydiff / 8);
                        t_point lat_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - 7 * ydiff / 16);
                        my_database.latlon_information.lat = xy_to_latlon(x, y).lat;
                        my_database.latlon_information.lon = xy_to_latlon(x, y).lon;
                        drawtext(lon_center, "Longitude: " + to_string(xy_to_latlon(x, y).lon), 1000000, 1000000);
                        drawtext(lat_center, "Latitude: " + to_string(xy_to_latlon(x, y).lat), 1000000, 1000000);
                    }
                }
            }
        }
    }
}

/*function for drawing all the features*/
void draw_features() {
    for (unsigned k = 0; k < 4; k++) {
        for (auto iter = my_database.layer[k].begin(); iter != my_database.layer[k].end(); iter++) {

            FeatureType type = getFeatureType(*iter);
            unsigned point_count = getFeaturePointCount(*iter);
            t_point coord[point_count];
            int flag = 0;
            double north = -90;
            double south = 90;
            double west = -180;
            double east = 180;
            /*check whether one of the points of this feature is inside the visible world*/
            for (unsigned j = 0; j < point_count; j++) {
                LatLon position = getFeaturePoint(*iter, j);
                if (position.lat > north) north = position.lat;
                if (position.lat < south) south = position.lat;
                if (position.lon > east) east = position.lon;
                if (position.lon < west) west = position.lon;

                if (check_inside_visible_world(position, my_database.cur_coords)) {
                    flag = 1;
                }
                coord[j] = latlonProjection(position);
            }
            /*
            LatLon position1(north, east);
            LatLon position2(south, west);
            t_point point1 = latlonProjection(position1);
            t_point point2 = latlonProjection(position2);
            //cout<<point1.y<<endl<<point2.y<<endl;//<<west<<endl<<north<<endl<<south<<endl;
            //break;
            
            LatLon p1(xy_to_latlon(cur_coords.right(),cur_coords.top()));
            LatLon p2(xy_to_latlon(cur_coords.left(),cur_coords.bottom()));
            if (( (east>p1.lon)&& (west<p2.lon) ) && ((north>p1.lat) && (south<p2.lat))) {
                flag = 1;
                cout<<*iter<<endl;
            }*/
            /*if this feature is not inside the visible world, then we will not draw it*/
            if (flag == 0) {
                continue;
            }
            /*check whether the feature is a polygon*/
            bool ispoly = (coord[0].x == coord[point_count - 1].x)&&(coord[0].y == coord[point_count - 1].y);
            //fillpoly(coord,point_count);
            /*draw features with different colors due to different types*/
            if (type == Park) {
                setcolor(t_color(202, 223, 170));
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {
                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }

            }
            if (type == Beach) {
                setcolor(t_color(250, 242, 199));
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {
                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }
            }
            if (type == Lake) {
                setcolor(t_color(169, 206, 255));
                fillpoly(coord, point_count);
            }
            if (type == River) {
                setcolor(t_color(169, 206, 255));
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {
                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }

            }
            if (type == Island) {
                setcolor(t_color(233, 229, 220));
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {
                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }
            }
            if (type == Shoreline) {
                setcolor(BLACK);
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {
                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }
            }
            if (type == Building) {
                if (my_database.featureID_to_OSMtype.find(*iter)->second == "school"
                        || my_database.featureID_to_OSMtype.find(*iter)->second == "university") {
                    if (my_database.zoom_level > 1) {
                        setcolor(t_color(232, 221, 189));
                        fillpoly(coord, point_count);
                    }
                } else if (my_database.featureID_to_aeroway.find(*iter) != my_database.featureID_to_aeroway.end()) {
                    if (my_database.zoom_level > 0) {
                        setcolor(t_color(211, 202, 189));
                        fillpoly(coord, point_count);
                    }
                } else if (my_database.featureID_to_hospital.find(*iter) != my_database.featureID_to_hospital.end()) {
                    if (my_database.zoom_level > 1) {
                        setcolor(t_color(235, 210, 207));
                        fillpoly(coord, point_count);
                    }
                } else if (my_database.zoom_level >= 6) {
                    setcolor(t_color(248, 245, 236));
                    if (ispoly) {
                        fillpoly(coord, point_count);
                    } else {
                        for (unsigned j = 0; j < point_count - 1; j++) {
                            drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                        }
                    }

                }
            }
            if (type == Greenspace) {
                setcolor(t_color(202, 223, 170));
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {
                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }
            }
            if (type == Golfcourse) {
                setcolor(t_color(202, 237, 209));
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {
                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }
            }
            if (type == Stream and my_database.zoom_level > 4) {
                setcolor(t_color(169, 206, 255));
                if (ispoly) {
                    fillpoly(coord, point_count);
                } else {
                    for (unsigned j = 0; j < point_count - 1; j++) {

                        drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                    }
                }
            }
        }
    }
}

/*The function implemented to draw streets, intersections and interests*/
void draw_streets_intersections_interests() {
    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
        if (my_database.zoom_level > 6) {
            LatLon from = getIntersectionPosition(getStreetSegmentInfo(i).from);
            LatLon to = getIntersectionPosition(getStreetSegmentInfo(i).to);
            double lat_diff = to.lat - from.lat;
            double lon_diff = to.lon - from.lon;

            int flag = 0;
            for (unsigned i = 0; i < 7; i++) {
                LatLon point;
                point.lat = from.lat + i * lat_diff / 6;
                point.lon = from.lon + i * lon_diff / 6;
                if (check_inside_visible_world(point, my_database.cur_coords)) {
                    flag = 1;
                }
            }
            /*if the street is not inside the visible world, then we will not draw it*/
            if (flag == 0) {
                continue;
            }
            /*if the street is not inside the visible world, then we will not draw it*/
        } else if (!check_inside_visible_world(getIntersectionPosition(getStreetSegmentInfo(i).to), my_database.cur_coords)
                &&!check_inside_visible_world(getIntersectionPosition(getStreetSegmentInfo(i).from), my_database.cur_coords)) {
            continue;
        }
        /*draw different streets at different zoom levels with different colors and width due to different street types*/
        if (my_database.segs_type[i] == "motorway" || my_database.segs_type[i] == "trunk") {
            if (my_database.zoom_level < -1) continue;
            else {
                if (my_database.zoom_level == 0 || my_database.zoom_level == 1) {
                    setlinewidth(pow(1.8, (my_database.zoom_level + 2)));
                } else {
                    setlinewidth(pow(1.35, (my_database.zoom_level + 2)));
                }
                setcolor(t_color(249, 158, 36));
            }
        } else if (my_database.segs_type[i] == "motorway_link" || my_database.segs_type[i] == "trunk_link") {
            if (my_database.zoom_level < 3) continue;
            else {
                setlinewidth(pow(1.25, (my_database.zoom_level + 2)));
                setcolor(t_color(249, 158, 36));
            }
        } else if (my_database.segs_type[i] == "primary" || my_database.segs_type[i] == "primary_link") {
            if (my_database.zoom_level < 0) continue;
            else {
                setlinewidth(my_database.zoom_level + 2);
                setcolor(t_color(252, 221, 101));
            }
        } else if (my_database.segs_type[i] == "secondary" || my_database.segs_type[i] == "secondary_link") {
            if (my_database.zoom_level < 0) continue;
            else {
                vector<unsigned> segment_ids_array;
                if (my_database.zoom_level >= 0 || my_database.zoom_level <= 2) {
                    setlinewidth(pow(1.35, (my_database.zoom_level + 2)) / 3);
                } //else {
                // setlinewidth((zoom_level + 4));
                // }
                setcolor(WHITE);
            }
        } else if (my_database.segs_type[i] == "residential" || my_database.segs_type[i] == "service"
                || my_database.segs_type[i] == "tertiary" || my_database.segs_type[i] == "tertiary_link"
                || my_database.segs_type[i] == "unclassified" || my_database.segs_type[i] == "living_street" ||
                my_database.segs_type[i] == "road" || my_database.segs_type[i] == "pedestrian") {
            if (my_database.zoom_level < 4) continue;
            else if (my_database.zoom_level == 4 || my_database.zoom_level == 5) {
                setlinewidth(1);
                setcolor(t_color(207, 200, 189));
            } else {
                setlinewidth((my_database.zoom_level + 2) * 0.5);
                setcolor(WHITE);
            }
        }
        StreetSegmentInfo info = getStreetSegmentInfo(i);
        t_point p1 = latlonProjection(getIntersectionPosition(info.from));
        t_point p2 = latlonProjection(getIntersectionPosition(info.to));
        if (info.curvePointCount == 0) {
            drawline(p1.x, p1.y, p2.x, p2.y);
        } else {
            vector<t_point> segs;
            segs.push_back(p1);
            for (unsigned j = 0; j < info.curvePointCount; j++) {
                t_point curp = latlonProjection(getStreetSegmentCurvePoint(i, j));
                segs.push_back(curp);
            }
            segs.push_back(p2);
            for (unsigned k = 0; k < info.curvePointCount + 1; k++) {
                drawline(segs[k].x, segs[k].y, segs[k + 1].x, segs[k + 1].y);
            }


        }
    }


    draw_path();
    for (unsigned i = 0; i < getNumberOfIntersections(); i++) {
        if (my_database.zoom_level > 6) {
            t_point center = latlonProjection(getIntersectionPosition(i));
            setcolor(t_color(193, 197, 214));
            fillarc(center, 5 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
        }
    }
    for (unsigned i = 0; i < getNumberOfPointsOfInterest(); i++) {
        //cout << getPointOfInterestType(i) << endl;
        if (my_database.zoom_level > 6) {
            t_point center = latlonProjection(getPointOfInterestPosition(i));
            /*If the POI is catogorized into bank and atm, draw it as a "$" symbol*/
            if (getPointOfInterestType(i) == "bank" || getPointOfInterestType(i) == "atm") {
                if (my_database.bank == 1) {
                    setcolor(WHITE);
                    fillarc(center, 5 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
                    setcolor(t_color(132, 117, 14));
                    setfontsize(my_database.zoom_level * 2);
                    if (my_database.zoom_level >= 9) {
                        setfontsize(8);
                    }
                    drawtext(center, "$", 100000, 100000);
                }
            }/*If the POI is catogorized into restaurant and fast_food, draw it as a fork symbol*/
            else if (getPointOfInterestType(i) == "restaurant" || getPointOfInterestType(i) == "fast_food") {
                if (my_database.food == 1) {
                    setcolor(WHITE);
                    fillarc(center, 5 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
                    setcolor(t_color(243, 128, 24));
                    setfontsize(my_database.zoom_level * 2);
                    if (my_database.zoom_level >= 9) {
                        setfontsize(8);
                    }
                    drawtext(center, "Ψ", 100000, 100000);
                }
            }/*If the POI is catogorized into hospital, clinic or doctors, draw it as a red "H" symbol*/
            else if (getPointOfInterestType(i) == "hospital" || getPointOfInterestType(i) == "clinic"
                    || getPointOfInterestType(i) == "doctors") {
                setcolor(WHITE);
                fillarc(center, 5 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
                setcolor(RED);
                setfontsize(my_database.zoom_level * 0.8);
                if (my_database.zoom_level >= 9) {
                    setfontsize(8);
                }
                drawtext(center, "H", 100000, 100000);
            }/*If the POI is catogorized into parking lot, draw it as a Blue "P" symbol*/
            else if (getPointOfInterestType(i) == "parking") {
                if (my_database.parking == 1) {
                    setcolor(WHITE);
                    fillarc(center, 5 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
                    setcolor(t_color(0, 176, 255));
                    setfontsize(my_database.zoom_level * 2);
                    if (my_database.zoom_level >= 9) {
                        setfontsize(8);
                    }
                    drawtext(center, "P", 100000, 100000);
                }
            } else {

                setcolor(t_color(139, 108, 100));
                fillarc(center, 5 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
            }
        }
    }
}

/*this function will draw the path if the user use the find_path function*/
void draw_path() {
    if (my_database.draw_flag_path == 1) {
        for (unsigned i = 0; i < my_database.ids_path.size(); i++) {
            if (my_database.zoom_level < -1) continue;
            else {
                if (my_database.zoom_level == 0 || my_database.zoom_level == 1) {
                    if (my_database.segs_type[my_database.ids_path[i]] == "motorway"
                            || my_database.segs_type[my_database.ids_path[i]] == "trunk"
                            || my_database.segs_type[my_database.ids_path[i]] == "motorway_link"
                            || my_database.segs_type[my_database.ids_path[i]] == "trunk_link") {
                        setlinewidth(pow(1.8, (my_database.zoom_level)) + 3);
                    } else {
                        setlinewidth(pow(1.8, (my_database.zoom_level)));
                    }
                } else {
                    if (my_database.segs_type[my_database.ids_path[i]] == "motorway"
                            || my_database.segs_type[my_database.ids_path[i]] == "trunk"
                            || my_database.segs_type[my_database.ids_path[i]] == "motorway_link"
                            || my_database.segs_type[my_database.ids_path[i]] == "trunk_link") {
                        setlinewidth(pow(1.35, (my_database.zoom_level)) + 3);
                    } else {
                        setlinewidth(pow(1.35, (my_database.zoom_level)));
                    }
                }
                setcolor(t_color(0, 179, 253));
            }
            StreetSegmentInfo info = getStreetSegmentInfo(my_database.ids_path[i]);
            t_point p1 = latlonProjection(getIntersectionPosition(info.from));
            t_point p2 = latlonProjection(getIntersectionPosition(info.to));
            if (info.curvePointCount == 0) {
                drawline(p1.x, p1.y, p2.x, p2.y);
            } else {
                vector<t_point> segs;
                segs.push_back(p1);
                for (unsigned j = 0; j < info.curvePointCount; j++) {
                    t_point curp = latlonProjection(getStreetSegmentCurvePoint(my_database.ids_path[i], j));
                    segs.push_back(curp);
                }
                segs.push_back(p2);
                for (unsigned k = 0; k < info.curvePointCount + 1; k++) {

                    drawline(segs[k].x, segs[k].y, segs[k + 1].x, segs[k + 1].y);
                }


            }
        }
    }
    //print_path_name();
}

void print_path_name() {
    if (my_database.draw_flag_path == 1) {
        double length = 0;
        for (unsigned i = 0; i < my_database.ids_path.size(); i++) {
            length += find_street_segment_length(my_database.ids_path[i]);
        }
        cout << "Estimate total travel distance is ***" << floor(length) / 1000 << " km***" << endl;
        if (compute_path_travel_time(my_database.ids_path) < 1) {
            cout << "Estimate total travel time is ***Less than 1 minute***" << endl << endl;
        } else {
            cout << "Estimate total travel time is ***" << floor(compute_path_travel_time(my_database.ids_path)) << " minutes***" << endl << endl;
        }
        cout << "Starting at intersection ***" << getIntersectionName(my_database.inter_id1) << "***" << endl;
        unsigned from = 0;
        unsigned start = my_database.inter_id1;
        unsigned prev = 0;
        string turn;
        for (unsigned i = 0; i < my_database.ids_path.size(); i++) {
            prev = from;
            from = start;
            unsigned to;
            if (start == getStreetSegmentInfo(my_database.ids_path[i]).from) {
                to = getStreetSegmentInfo(my_database.ids_path[i]).to;
            } else {
                to = getStreetSegmentInfo(my_database.ids_path[i]).from;
            }
            start = to;
            if (i >= 1) {
                if (getStreetSegmentInfo(my_database.ids_path[i]).streetID == getStreetSegmentInfo(my_database.ids_path[i - 1]).streetID) continue;
                else {
                    turn = get_turn_direction(prev, from, to);
                }
            }
            double total = 0;
            for (unsigned j = i; j < my_database.ids_path.size() - 1; j++) {
                total += find_street_segment_length(my_database.ids_path[j]);

                if (getStreetSegmentInfo(my_database.ids_path[j]).streetID != getStreetSegmentInfo(my_database.ids_path[j + 1]).streetID) break;
            }
            if (i == 0) {
                cout << " Head " << get_direction(from, to) << " on intersection ***" << getIntersectionName(from) << "***" << endl;
            } else {
                cout << get_turn_direction(prev, from, to) << " at intersection ***" << getIntersectionName(from) << "***" << endl;
            }
            cout << "  Go along ***" << getStreetName(getStreetSegmentInfo(my_database.ids_path[i]).streetID) << "*** for " << total << " meters" << endl;
            cout << '\n';
        }
        cout << "Arriving at intersection ***" << getIntersectionName(start) << "***" << endl << endl;
        ;
    }
}

/*this function is used to redraw highlights, which first done in act_on_button functions*/
void redraw_highlights() {
    // redraw the starting point and the destination point if a path is found
    if (my_database.draw_flag_path == 1) {
        if (my_database.zoom_level > -1) {
            t_point start = latlonProjection(getIntersectionPosition(my_database.inter_id1));
            t_point end;
            if (my_database.draw_flag_poi == 0) {
                end = latlonProjection(getIntersectionPosition(my_database.inter_id2));
            }
            setcolor(BLACK);
            fillarc(start, 500 * pow(0.6, my_database.zoom_level), 0, 360);
            setcolor(WHITE);
            fillarc(start, 450 * pow(0.6, my_database.zoom_level), 0, 360);
            setcolor(BLACK);
            fillarc(start, 300 * pow(0.6, my_database.zoom_level), 0, 360);
            if (my_database.draw_flag_poi == 0) {
                draw_position_icon(end, 1000 * pow(0.6, my_database.zoom_level) + 5);
            }
        }

    }
    /*redraw highlights of intersections found by "Find ints" button*/
    if (my_database.draw_flag1 == 1) {
        for (auto iter = my_database.ids_array1.begin(); iter != my_database.ids_array1.end(); iter++) {
            t_point center;
            center.x = latlonProjection(getIntersectionPosition(*iter)).x;
            center.y = latlonProjection(getIntersectionPosition(*iter)).y;
            setcolor(BLUE);
            if (my_database.zoom_level < 8) {
                /*draw a circle at the intersection position*/
                //fillarc(center, 1000 * pow(0.6, my_database.zoom_level), 0, 360);
                draw_position_icon(center, 1000 * pow(0.6, my_database.zoom_level) + 5);
            } else {
                /*draw a circle at the intersection position*/
                //fillarc(center, 1000 * pow(0.8, my_database.zoom_level + 7), 0, 360);
                draw_position_icon(center, 1000 * pow(0.8, my_database.zoom_level + 12));
            }
        }
    }
    /*redraw highlights of intersections clicked by users*/
    if (my_database.draw_flag2 == 1) {
        setcolor(RED);
        t_point center;
        for (auto i = my_database.ids_array2.begin(); i != my_database.ids_array2.end(); i++) {
            center.x = latlonProjection(getIntersectionPosition(*i)).x;
            center.y = latlonProjection(getIntersectionPosition(*i)).y;
            /*draw a circle at the intersection position*/
            //fillarc(center, 15 * pow(0.8, (my_database.zoom_level - 7)), 0, 360);
            draw_position_icon(center, 15 * pow(0.8, (my_database.zoom_level - 7)) + 5);
        }
    }
    /*redraw the highlights of buildings clicked by users*/
    if (my_database.draw_flag3 == 1 && my_database.zoom_level >= 6) {

        for (auto i = my_database.ids_array3.begin(); i != my_database.ids_array3.end(); i++) {
            unsigned point_count = getFeaturePointCount(*i);
            t_point coord[point_count];
            /*store all points of the building */
            for (unsigned j = 0; j < point_count; j++) {

                coord[j] = latlonProjection(getFeaturePoint(*i, j));
            }
            bool ispoly = (coord[0].x == coord[point_count - 1].x)&&(coord[0].y == coord[point_count - 1].y);
            setcolor(RED);
            if (ispoly) {
                /*draw a polygon*/
                fillpoly(coord, point_count);
            } else {
                for (unsigned j = 0; j < point_count - 1; j++) {
                    drawline(coord[j].x, coord[j].y, coord[j + 1].x, coord[j + 1].y);
                }
            }

        }

    }
    /*redraw highlights of streets found by "Find streets" button*/
    if (my_database.draw_flag4 == 1) {
        for (auto iter = my_database.segment_ids_array.begin(); iter != my_database.segment_ids_array.end(); iter++) {
            setcolor(RED);
            if (my_database.zoom_level == 0 || my_database.zoom_level == 1) {
                setlinewidth(pow(1.8, (my_database.zoom_level + 2)));
            } else {
                setlinewidth(pow(1.35, (my_database.zoom_level + 2)));
            }
            StreetSegmentInfo info = getStreetSegmentInfo(*iter);
            t_point p1 = latlonProjection(getIntersectionPosition(info.from));
            t_point p2 = latlonProjection(getIntersectionPosition(info.to));
            /*draw all street segments*/
            if (info.curvePointCount == 0) {
                drawline(p1.x, p1.y, p2.x, p2.y);
            } else {
                vector<t_point> segs;
                segs.push_back(p1);
                for (unsigned j = 0; j < info.curvePointCount; j++) {
                    t_point curp = latlonProjection(getStreetSegmentCurvePoint(*iter, j));
                    segs.push_back(curp);
                }
                segs.push_back(p2);
                for (unsigned k = 0; k < info.curvePointCount + 1; k++) {
                    drawline(segs[k].x, segs[k].y, segs[k + 1].x, segs[k + 1].y);
                }


            }
        }
    }

    if (my_database.draw_flag_poi == 1) {
        if (my_database.POIname_to_id.find(my_database.user_input) != my_database.POIname_to_id.end()) {
            vector<unsigned> POIids = my_database.POIname_to_id.find(my_database.user_input)->second;
            for (unsigned i = 0; i < POIids.size(); i++) {
                t_point poicenter = latlonProjection(getPointOfInterestPosition(POIids[i]));
                if (my_database.zoom_level < 5) {
                    draw_position_icon_poi(poicenter, 50 * pow(0.8, (my_database.zoom_level - 7)));
                } else {
                    draw_position_icon_poi(poicenter, 17 * pow(0.8, (my_database.zoom_level - 7)) + 5);
                }
            }
        }
    }
    /*redraw the info bar of intersections*/
    if (my_database.information_flag == 1) {
        vector<string> names = find_intersection_street_names(my_database.information_id);
        vector<string> street_names;
        for (auto iter = names.begin(); iter != names.end(); iter++) {
            if (find(street_names.begin(), street_names.end(), *iter) == street_names.end()) {
                street_names.push_back(*iter);
            }
        }
        double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
        double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
        double x1 = my_database.cur_coords.right() - xdiff / 4;
        double y1 = my_database.cur_coords.bottom() + ydiff / 2;
        setcolor(t_color(66, 133, 244));
        fillrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top() - ydiff / 20);
        t_point textcenter(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8);
        setcolor(WHITE);
        setfontsize(12);
        drawtext(textcenter, "Streets crossing this intersection: ", 1000000000, 10000000000);
        setfontsize(15);
        int size = street_names.size();
        /*show streets names crossing this intersection*/
        for (int i = 0; i < size; i++) {
            t_point my_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (i + 1) * ydiff / (size + 3));
            drawtext(my_center, to_string(i + 1) + ". " + street_names[i], 100000, 10000);
        }
        /*show lat lon position*/
        t_point lon_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (size + 1) * ydiff / (size + 3));
        t_point lat_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8 - 0.375 * (size + 2) * ydiff / (size + 3));
        setfontsize(10);
        drawtext(lon_center, "Longitude: " + to_string(my_database.latlon_information.lon), 1000000, 1000000);
        drawtext(lat_center, "Latitude: " + to_string(my_database.latlon_information.lat), 1000000, 1000000);
        /*redraw the info bar of buildings*/
    } else if (my_database.information_flag == 2) {
        string name = getFeatureName(my_database.information_id);
        if (name == "<noname>") {
            name = "unknown building";
        }
        double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
        double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
        double x1 = my_database.cur_coords.right() - xdiff / 4;
        double y1 = my_database.cur_coords.bottom() + ydiff / 2;
        setcolor(t_color(66, 133, 244));
        fillrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top() - ydiff / 20);
        t_point textcenter(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 8);
        setcolor(WHITE);
        setfontsize(12);
        /*show building name*/
        drawtext(textcenter, "Building name:", 100000, 100000);
        t_point name_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - ydiff / 4);
        setfontsize(10);
        drawtext(name_center, name, 100000, 100000);
        /*show lat lon position of the building*/
        t_point lon_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - 3 * ydiff / 8);
        t_point lat_center(my_database.cur_coords.right() - xdiff / 8, my_database.cur_coords.top() - 7 * ydiff / 16);
        drawtext(lon_center, "Longitude: " + to_string(my_database.latlon_information.lon), 1000000, 1000000);
        drawtext(lat_center, "Latitude: " + to_string(my_database.latlon_information.lat), 1000000, 1000000);
    }
    /*redraw the search bar*/
    if (my_database.search == 1) {
        double xdiff = my_database.cur_coords.right() - my_database.cur_coords.left();
        double ydiff = my_database.cur_coords.top() - my_database.cur_coords.bottom();
        double x1 = my_database.cur_coords.right() - xdiff / 2;
        double y1 = my_database.cur_coords.top() - ydiff / 25;
        double x2 = x1 - (xdiff / 7);
        setcolor(WHITE);
        fillrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top());
        fillrect(x2, y1, x1, my_database.cur_coords.top());
        setcolor(BLACK);
        setlinewidth(1);
        drawrect(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top());
        drawrect(x2, y1, x1, my_database.cur_coords.top());
        t_bound_box text(x1, y1, my_database.cur_coords.right(), my_database.cur_coords.top());
        t_bound_box text2(x2, y1, x1, my_database.cur_coords.top());
        setcolor(BLACK);
        setfontsize(13);
        if (my_database.search_type == 0) {
            drawtext_in(text2, "Search Street:");
        } else if (my_database.search_type == 1) {
            drawtext_in(text2, "Search POI name:");
        } else if (my_database.search_type == 2) {
            drawtext_in(text2, "Search Inter:");
        }
        if (my_database.search_state == Click) {
            drawtext_in(text, my_database.user_input + "|");
        } else if (my_database.search_state == Input) {
            drawtext_in(text, my_database.user_input + "|");
        } else if (my_database.search_state == Enter) {
            drawtext_in(text, my_database.user_input);
        } else if (my_database.search_state == Begin) {

            drawtext_in(text, my_database.user_input);
        }
    }

}

/*function used to draw arrows and street names*/
void draw_arrows_stnames() {
    if (my_database.zoom_level > 5) {
        for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
            StreetSegmentInfo info = getStreetSegmentInfo(i);
            if (getStreetName(info.streetID)[0] == '<') continue;
            setcolor(BLACK);

            t_point center;

            t_point pos_from = latlonProjection(getIntersectionPosition(info.from));
            t_point pos_to = latlonProjection(getIntersectionPosition(info.to));


            float slope = (pos_to.y - pos_from.y) / (pos_to.x - pos_from.x);
            int degree = atan(slope)*180 / PI;
            center.x = (pos_from.x + pos_to.x) / 2;
            center.y = (pos_from.y + pos_to.y) / 2;

            string stname = getStreetName(info.streetID);
            if (info.curvePointCount > 2) continue;
            if (my_database.zoom_level < 10) {
                // if segment length is not long enough to hold the name, then don't draw
                if (find_street_segment_length(i) * my_database.zoom_level < 1100) continue;
                // highway has lots of close-distance parallel segments, which causes a serve overlapping names
                if (my_database.segs_type[i] == "motorway" || my_database.segs_type[i] == "trunk") continue;
                settextattrs(my_database.zoom_level, degree);
                drawtext(center, stname, 100000, 100000);
                settextrotation(0);

            }
            if (my_database.zoom_level >= 10) {
                if (find_street_segment_length(i) < 20) continue;
                settextattrs(8, degree);
                drawtext(center, stname, 100000, 100000);
                settextrotation(0);
            }

            /* section for drawing arrows(show directions of streets)*/

            if (my_database.zoom_level > 7) {
                if (info.oneWay == true) {
                    t_point arrow[4];

                    t_point pos_from = latlonProjection(getIntersectionPosition(info.from));
                    t_point pos_to = latlonProjection(getIntersectionPosition(info.to));
                    t_point arrow_end;
                    float slope = (pos_to.y - pos_from.y) / (pos_to.x - pos_from.x);
                    float dx1 = sqrt((50 - my_database.zoom_level) / (1 + slope * slope));
                    float dx2 = sqrt(((50 - my_database.zoom_level) / 2) / (1 + 1 / (slope * slope)));

                    arrow[1].x = pos_from.x + (pos_to.x - pos_from.x) / 9;
                    arrow[1].y = pos_from.y + (pos_to.y - pos_from.y) / 9;

                    if (slope > 0) {
                        if (pos_to.y > pos_from.y) {
                            arrow_end.x = arrow[1].x - dx1;
                            arrow_end.y = arrow[1].y - dx1 * slope;
                            arrow[3].x = arrow[1].x + dx1;
                            arrow[3].y = arrow[1].y + dx1 * slope;

                        } else {
                            arrow[3].x = arrow[1].x - dx1;
                            arrow[3].y = arrow[1].y - dx1 * slope;
                            arrow_end.x = arrow[1].x + dx1;
                            arrow_end.y = arrow[1].y + dx1 * slope;
                        }
                        arrow[0].x = arrow_end.x - dx2;
                        arrow[0].y = arrow_end.y - dx2 * (-1 / slope);
                        arrow[2].x = arrow_end.x + dx2;
                        arrow[2].y = arrow_end.y + dx2 * (-1 / slope);

                    } else if (slope < 0) {
                        if (pos_to.y > pos_from.y) {
                            arrow_end.x = arrow[1].x + dx1;
                            arrow_end.y = arrow[1].y + dx1 * slope;
                            arrow[3].x = arrow[1].x - dx1;
                            arrow[3].y = arrow[1].y - dx1 * slope;
                        } else {
                            arrow[3].x = arrow[1].x + dx1;
                            arrow[3].y = arrow[1].y + dx1 * slope;
                            arrow_end.x = arrow[1].x - dx1;
                            arrow_end.y = arrow[1].y - dx1 * slope;
                        }
                        arrow[0].x = arrow_end.x + dx2;
                        arrow[0].y = arrow_end.y + dx2 * (-1 / slope);
                        arrow[2].x = arrow_end.x - dx2;
                        arrow[2].y = arrow_end.y - dx2 * (-1 / slope);
                    } else continue;
                    setcolor(t_color(193, 197, 214));
                    if (abs(arrow[0].y - arrow[1].y) < 100 && abs(arrow[1].y - arrow[2].y) < 100
                            && abs(arrow[2].y - arrow[3].y) < 100 && abs(arrow[3].y - arrow[0].y) < 100) {

                        fillpoly(arrow, 4);
                    }


                }
            }
        }
    }
}

/*function used to draw feature names*/
void draw_features_names() {
    for (unsigned i = 0; i < getNumberOfFeatures(); i++) {
        t_point center;
        LatLon latloncen(0, 0);
        string bdname;
        /*draw names of buildings*/
        if (my_database.zoom_level > 9) {
            if ((getFeatureType(i) == Building) && getFeatureName(i)[0] != '<') {
                /*find the center of the feature*/
                unsigned point_count = getFeaturePointCount(i);
                for (unsigned j = 0; j < point_count; j++) {
                    latloncen.lat += getFeaturePoint(i, j).lat;
                    latloncen.lon += getFeaturePoint(i, j).lon;
                }
                latloncen.lat /= point_count;
                latloncen.lon /= point_count;
                center = latlonProjection(latloncen);
                bdname = getFeatureName(i);
                setfontsize(8);
                setcolor(BLACK);
            }
        }
        /*draw names of parks and golfcourses*/
        if (my_database.zoom_level > 4) {
            if ((getFeatureType(i) == Park || getFeatureType(i) == Golfcourse)
                    && getFeatureName(i)[0] != '<') {
                if (abs(compute_poly_area(i)) < 45120) continue;
                unsigned point_count = getFeaturePointCount(i);
                for (unsigned j = 0; j < point_count; j++) {
                    latloncen.lat += getFeaturePoint(i, j).lat;
                    latloncen.lon += getFeaturePoint(i, j).lon;
                }
                latloncen.lat /= point_count;
                latloncen.lon /= point_count;
                center = latlonProjection(latloncen);
                bdname = getFeatureName(i);
                setcolor(t_color(108, 143, 54));
                setfontsize(my_database.zoom_level + 2);
            }
        }
        /*draw names of green spaces*/
        if (my_database.zoom_level > 1) {
            if ((getFeatureType(i) == Greenspace) && getFeatureName(i)[0] != '<') {
                unsigned point_count = getFeaturePointCount(i);
                for (unsigned j = 0; j < point_count; j++) {
                    latloncen.lat += getFeaturePoint(i, j).lat;
                    latloncen.lon += getFeaturePoint(i, j).lon;
                }
                latloncen.lat /= point_count;
                latloncen.lon /= point_count;
                center = latlonProjection(latloncen);
                bdname = getFeatureName(i);
                setcolor(t_color(63, 107, 56));
                setfontsize(11);
            }
        }
        /*draw names of beaches*/
        if (my_database.zoom_level > 4) {
            if ((getFeatureType(i) == Beach) && getFeatureName(i)[0] != '<') {
                unsigned point_count = getFeaturePointCount(i);
                for (unsigned j = 0; j < point_count; j++) {

                    latloncen.lat += getFeaturePoint(i, j).lat;
                    latloncen.lon += getFeaturePoint(i, j).lon;
                }
                latloncen.lat /= point_count;
                latloncen.lon /= point_count;
                center = latlonProjection(latloncen);
                bdname = getFeatureName(i);
                setcolor(BLACK);
                setfontsize(10);
            }
        }
        drawtext(center, bdname, 100000, 100000);

    }
}

/*function used to draw interests name*/
void draw_interests_name() {
    if (my_database.zoom_level > 9 && my_database.show_interests_name == 1) {
        for (unsigned i = 0; i < getNumberOfPointsOfInterest(); i++) {

            setcolor(BLACK);
            string name = getPointOfInterestName(i);
            setfontsize(8);
            t_point center(latlonProjection(getPointOfInterestPosition(i)).x, latlonProjection(getPointOfInterestPosition(i)).y + 3);
            /*draw interest name*/
            drawtext(center, name, 100000, 100000);
        }
    }
}

/*function used to show city names on the map*/
void draw_city_name() {
    if (my_database.zoom_level < 2) {
        setcolor(BLACK);
        setfontsize(12);
        /*draw city names according to positions*/
        if (my_database.city_name == "Toronto") {
            drawtext(t_point(-1.00190e+7, 7.61900e+6), my_database.city_name, 100000, 100000);
            drawtext(t_point(-1.00550e+7, 7.60900e+6), "Mississauga", 100000, 100000);
        } else if (my_database.city_name == "Hamilton") {
            drawtext(t_point(-1.01496e+7, 7.54611e+6), my_database.city_name, 100000, 100000);
        } else if (my_database.city_name == "Cairo") {
            drawtext(t_point(4.72376e+6, 5.26435e+6), my_database.city_name, 100000, 100000);
        } else if (my_database.city_name == "Moscow") {
            drawtext(t_point(3.69385e+6, 9.72934e+6), my_database.city_name, 100000, 100000);
        } else if (my_database.city_name == "New York City") {
            drawtext(t_point(-9.78750e+6, 7.10600e+6), "New York", 100000, 100000);
            drawtext(t_point(-9.79590e+6, 7.10750e+6), "Jersey City", 100000, 100000);
            drawtext(t_point(-9.81000e+6, 7.10550e+6), "Newark", 100000, 100000);
            drawtext(t_point(-9.77990e+6, 7.10000e+6), "Brooklyn", 100000, 100000);

        } else if (my_database.city_name == "Saint Helena") {
            drawtext(t_point(-956743, -2.7858e+6), my_database.city_name, 100000, 100000);
        } else {

            drawtext(t_point(-1.00190e+7, 7.61900e+6), "Toronto", 100000, 100000);
            drawtext(t_point(-1.00800e+7, 7.54611e+6), "Hamilton", 100000, 100000);
            drawtext(t_point(-1.00550e+7, 7.60900e+6), "Mississauga", 100000, 100000);
        }
    }
}

/*function used to draw subway stations*/
void draw_subway_station() {
    for (unsigned i = 0; i < my_database.subway_station.size(); i++) {
        t_point position;
        if (my_database.zoom_level > 0) {
            position = latlonProjection(my_database.subway_station[i]);
            setcolor(t_color(0, 176, 255));
            fillrect(position.x - 50 * pow(0.8, my_database.zoom_level), position.y - 55 * pow(0.8, my_database.zoom_level)
                    , position.x + 50 * pow(0.8, my_database.zoom_level), position.y + 55 * pow(0.8, my_database.zoom_level));
            if (my_database.zoom_level > 2) {
                setcolor(WHITE);
                setfontsize(my_database.zoom_level * 1.1);
                /*use "M" to represent subway stations*/
                drawtext(position, "M", 100000, 100000);
            }
        }
        string station_name;
        if (my_database.subway_station_name.find(i) != my_database.subway_station_name.end()) {
            station_name = my_database.subway_station_name.find(i)->second;
            /*draw subway station name*/
            if (my_database.zoom_level > 8) {

                setcolor(t_color(3, 61, 87));
                setfontsize(my_database.zoom_level);
                /*draw station name*/
                drawtext(position.x, position.y + 9, station_name, 100000, 100000);
            }
        }
    }
}

/*function used to draw subways*/
void draw_subway() {
    if (my_database.zoom_level >= 0) {
        for (unsigned i = 0; i < my_database.subway.size(); i++) {
            t_point begin, end;
            for (unsigned j = 0; j < my_database.subway[i].size() - 1; j++) {
                begin = latlonProjection(my_database.subway[i][j]);
                end = latlonProjection(my_database.subway[i][j + 1]);
                setlinewidth(pow(1.35, (my_database.zoom_level + 2)) / 12);
                /*draw some subways names */
                if (my_database.line[i][j] == "Scarborough RT") {
                    setcolor(t_color(16, 139, 239));
                } else if (my_database.line[i][j] == "Bloor-Danforth Line") {
                    setcolor(t_color(0, 128, 0));
                } else if (my_database.line[i][j] == "Yonge-University-Spadina Line") {
                    setcolor(t_color(213, 200, 43));
                } else if (my_database.line[i][j] == "Sheppard Line") {
                    setcolor(t_color(179, 0, 179));
                } else {

                    setcolor(t_color(190, 184, 172));
                }
                drawline(begin, end);
            }
        }
    }
}

void draw_position_icon(t_point base_point, double R) {

    double r = R / 5;
    double h = R / 0.3 - R;
    double offsety = h - R * R / h;
    double offsetx = sqrt(R * R * 0.92 - (R * R / h)*(R * R / h));
    t_point circle_center;
    circle_center.x = base_point.x;
    circle_center.y = base_point.y + h;

    t_point right;
    right.x = base_point.x + offsetx;
    right.y = base_point.y + offsety;
    t_point left;
    left.x = base_point.x - offsetx;
    left.y = base_point.y + offsety;

    t_point points[3];
    points[0].x = base_point.x;
    points[0].y = base_point.y;
    points[1].x = right.x;
    points[1].y = right.y;
    points[2].x = left.x;
    points[2].y = left.y;


    setcolor(t_color(247, 92, 80));
    fillpoly(points, 3);
    fillarc(circle_center, R, 0, 360);

    setcolor(t_color(92, 20, 16));
    fillarc(circle_center, r, 0, 360);
}

void draw_position_icon_poi(t_point base_point, double R) {

    double r = R / 5;
    double h = R / 0.3 - R;
    double offsety = h - R * R / h;
    double offsetx = sqrt(R * R * 0.92 - (R * R / h)*(R * R / h));
    t_point circle_center;
    circle_center.x = base_point.x;
    circle_center.y = base_point.y + h;

    t_point right;
    right.x = base_point.x + offsetx;
    right.y = base_point.y + offsety;
    t_point left;
    left.x = base_point.x - offsetx;
    left.y = base_point.y + offsety;

    t_point points[3];
    points[0].x = base_point.x;
    points[0].y = base_point.y;
    points[1].x = right.x;
    points[1].y = right.y;
    points[2].x = left.x;
    points[2].y = left.y;


    setcolor(BLUE);
    fillpoly(points, 3);
    fillarc(circle_center, R, 0, 360);

    setcolor(WHITE);
    fillarc(circle_center, r, 0, 360);
}

string get_direction(unsigned from, unsigned to) {
    t_point inter1 = latlonProjection(getIntersectionPosition(from));
    t_point inter2 = latlonProjection(getIntersectionPosition(to));
    //double slope = (inter1.lon - inter2.lon) / (inter1.lat - inter2.lat);
    //cout << slope << endl;
    if (inter1.x > inter2.x && inter1.y > inter2.y) {
        return "South West";
    } else if (inter1.x > inter2.x && inter1.y < inter2.y) {
        return "North West";
    } else if (inter1.x < inter2.x && inter1.y < inter2.y) {
        return "North East";
    } else {
        return "South East";
    }
}

string get_turn_direction(unsigned prev_, unsigned from_, unsigned to_) {
    t_point prev = latlonProjection(getIntersectionPosition(prev_));
    t_point from = latlonProjection(getIntersectionPosition(from_));
    t_point to = latlonProjection(getIntersectionPosition(to_));
    t_point vec1(from.x - prev.x, from.y - prev.y);
    t_point vec2(to.x - prev.x, to.y - prev.y);
    double cross = vec1.x * vec2.y - vec2.x * vec1.y;
    double k1 = (from.y - prev.y) / (from.x - prev.x);
    double k2 = (to.y - prev.y) / (to.x - prev.x);
    if (abs((k1 - k2) / k2) < 0.05) {
        return " Go STRAIGHT";
    }
    if (cross > 0) {
        return " Turn LEFT";
    } else {
        return " Turn RIGHT";
    }
}
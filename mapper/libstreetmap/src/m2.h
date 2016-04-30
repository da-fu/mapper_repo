#pragma once
#include <string>
#include <vector>
#include "graphics.h" 
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "Feature.h"
#include "m2.h"
#include <math.h>
#include "OSMDatabaseAPI.h"
#include <unordered_map>
#include "database.h"
#include <iomanip>
// Draws the map. You can assume your load_map (string map_name)
// function is called before this function in the unit tests.
// Your main () program should do the same.

// This function allows the program to continuously display maps based on user input
// without relaunching it
void milestone2();

void a();

/*draw map function*/
void draw_map();

/*this function draws everything*/
void visualize();

/*this function loads the database we need in m2*/
bool load_map_m2();

/*function used to show city names on the map*/
void draw_city_name();

/*function for drawing all the features*/
void draw_features();

/*The function implemented to draw streets, intersections and interests*/
void draw_streets_intersections_interests();

void draw_path();

void draw_position_icon(t_point base_point, double R);

/*this function is used to redraw highlights, which first done in act_on_button functions*/
void redraw_highlights();

/*function used to draw feature names*/
void draw_features_names();

/*function used to draw interests name*/
void draw_interests_name();

/*function used to draw arrows and street names*/
void draw_arrows_stnames();

/*function used to draw subway stations*/
void draw_subway_station();

/*function used to draw subways*/
void draw_subway();

void act_on_mouse_move (float x, float y, t_event_buttonPressed button_info);

void act_on_key_press(char c, int keysym);

void draw_help();

/*project latlon to t_point*/
t_point latlonProjection(LatLon position);
void act_on_help_button(void (*drawscreen) (void));
void act_on_search_st_button_func(void (*drawscreen) (void));

void act_on_search_poi_button_func(void (*drawscreen) (void));

void act_on_search_int_button_func(void (*drawscreen) (void));

/*When the user clicks the find streets button on the interface*/
void act_on_find_ints_button_func(void (*drawscreen) (void));

void act_on_find_path_func(void (*drawscreen) (void));

/*When the user clicks the clear highlights button on the interface*/
void act_on_clear_button_func(void (*drawscreen) (void));

/*When the user clicks the show interests button on the interface*/
void act_on_interests_button_func(void (*drawscreen) (void));

/*draws subway when the user clicks the draw subway button*/
void act_on_draw_subway_button(void (*drawscreen) (void));

/*allow users to switch map without rerunning the program*/
void act_on_switch_map_button(void (*drawscreen) (void));

/*When the user clicks the food button on the interface*/
void act_on_food_button_func(void (*drawscreen) (void));

/*When the user clicks the parking button on the interface*/
void act_on_parking_button_func(void (*drawscreen) (void));

/*When the user clicks the bank button on the interface*/
void act_on_bank_button_func(void (*drawscreen) (void));

void load_subway();

/*stores the subway stations and names*/
void load_subway_station();

/* This function will get the tag of every street segments*/
void load_streetsegs_type();

/*stores way index into a hash table. key: OSMID   value: way index*/
void load_OSMID_to_way_index();

/*stores node index into a hash table. key:OSMID  value:node index*/
void load_OSMID_to_node_index();

/*store types of features into a hash table key:feature id   value:type*/
void load_featureID_to_OSMtype();

/*function used to store layers of features*/
void load_layer();

/*Compute the area of a polygon*/
double compute_poly_area(unsigned i);

/*get user inputs*/
//void act_on_key_press (char c, int keysym);

/*actions done when user clicks on screen*/
void act_on_button_press (float x, float y, t_event_buttonPressed event);

/*project t_point to latlon */
LatLon xy_to_latlon(float x,float y);

bool point_inside(float x, float y,unsigned i);

/*function used to check whether a point is inside a circle*/
bool point_inside_circle(float x, float y, unsigned i);

/*function used to check whether a point is inside the visible world*/
bool check_inside_visible_world(LatLon position,t_bound_box coords);

/*fucntion for checking whether a point is inside a polygon*/
bool pointInside_polygon (float x, float y,unsigned id);

/*Get the vector between two points*/
t_point getVecBetweenPoints (t_point start, t_point end);

/*draw the page used to help users to select map*/
void initialize();

/*When the user clicks the find street button on the interface*/
void act_on_find_streets_button(void (*drawscreen) (void));


void initialize_m3_data();

std::string get_direction(unsigned from, unsigned to);

void print_path_name();

void draw_position_icon_poi(t_point base_point, double R);

std::string get_turn_direction(unsigned prev_, unsigned from_, unsigned to_);
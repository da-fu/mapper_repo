/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "m4.h"
#include <vector>
#include <queue>
#include "m3.h"
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "database.h"
#include <thread>
#include <chrono>
using namespace std;
extern database my_database;
#define TIME_LIMIT 30
 auto starttime = chrono::high_resolution_clock::now();

void initial(vector<vector<pair<double, vector<unsigned>>>>&array, unsigned startid, unordered_map<unsigned, unsigned>& check_index) {
    vector<int> visited = my_database.visited;
    vector<double> time = my_database.time;
    time[startid] = 0; //arrival time of start point is zero 
    vector<unsigned> come_from_ints = my_database.come_from_ints; //previous intersection of an intersection
    vector<unsigned> come_from_segs = my_database.come_from_segs; //previous street segment of an intersection
    priority_queue<pair<unsigned, double>, vector<pair<unsigned, double>>, prioritize> pq;
    pq.push(make_pair(startid, 0));
    int count = 0;
    while (!pq.empty()) {
        /*pop the intersection with smallest f value from the queue*/
        pair<unsigned, double> cur = pq.top();
        pq.pop();
        if (visited[cur.first]) continue;
        visited[cur.first] = 1;
        /*check whether this intersection is one of the closest intersections of pois*/
        if (check_index.find(cur.first) != check_index.end() && cur.first != startid) {
            unsigned intid = cur.first; //store this intersection id
            unsigned id1 = check_index[startid];
            unsigned id2 = check_index[cur.first];
            /*reconstruct the path*/
            while (intid != startid) {
                array[id1][id2].second.insert(array[id1][id2].second.begin(), come_from_segs[intid]);
                intid = come_from_ints[intid];
            }
            array[id1][id2].first = compute_path_travel_time(array[id1][id2].second);
            count += 1;
        }

        /*traverse all adjacent intersections and according street segments*/
        for (unsigned i = 0; i < my_database.iit[cur.first].size(); i++) {
            unsigned next_intersection_id = my_database.iit[cur.first][i].first;
            unsigned segid = my_database.iit[cur.first][i].second;
            if (visited[next_intersection_id] == 1) {
                continue;
            }
            StreetSegmentInfo info = getStreetSegmentInfo(segid);
            double p_time = time[cur.first] + find_street_segment_travel_time(segid);
            if (cur.first != startid) {
                unsigned lastid = come_from_segs[cur.first];
                /*check whether there is a turning*/
                if (getStreetSegmentInfo(lastid).streetID != info.streetID) {
                    p_time += 0.25; //add 15 seconds
                }
            }
            /*if this path is a better path, then record it*/
            if (p_time < time[next_intersection_id]) {
                time[next_intersection_id] = p_time; //update the arrival time
                pq.push(make_pair(next_intersection_id, time[next_intersection_id]));
                come_from_ints[next_intersection_id] = cur.first;
                come_from_segs[next_intersection_id] = segid;
            }

        }
    }
}

std::vector<unsigned> traveling_courier(const std::vector<DeliveryInfo>& deliveries, const std::vector<unsigned>& depots) {
    starttime = chrono::high_resolution_clock::now();
    vector<unsigned> path;
    vector<unsigned> return_path;
    vector<unsigned> empty;
    vector<unsigned> delivery_ints;
    vector<unsigned> points;
    unordered_map<unsigned, unsigned> check_index;
    unordered_map<unsigned, vector<unsigned>> pickup_info;
    unordered_map<unsigned, vector<unsigned>> dropoff_info;
    for (unsigned i = 0; i < deliveries.size(); i++) {
        if (find(delivery_ints.begin(), delivery_ints.end(), deliveries[i].pickUp) == delivery_ints.end()) {
            delivery_ints.push_back(deliveries[i].pickUp);
        }
        if (find(points.begin(), points.end(), deliveries[i].pickUp) == points.end()) {
            points.push_back(deliveries[i].pickUp);
        }
        if (find(points.begin(), points.end(), deliveries[i].dropOff) == points.end()) {
            points.push_back(deliveries[i].dropOff);
        }
        auto iter = pickup_info.find(deliveries[i].pickUp);
        if (iter == pickup_info.end()) {
            vector<unsigned> temp;
            temp.push_back(i);
            pickup_info.insert(make_pair(deliveries[i].pickUp, temp));
        } else {
            iter->second.push_back(i);
        }
        iter = dropoff_info.find(deliveries[i].dropOff);
        if (iter == dropoff_info.end()) {
            vector<unsigned> temp;
            temp.push_back(i);
            dropoff_info.insert(make_pair(deliveries[i].dropOff, temp));
        } else {
            iter->second.push_back(i);
        }
    }
    for (unsigned i = 0; i < depots.size(); i++) {
        if (find(points.begin(), points.end(), depots[i]) == points.end()) {
            points.push_back(depots[i]);
        }
    }
    unsigned num = points.size();
    for (unsigned i = 0; i < num; i++) {
        check_index.insert(make_pair(points[i], i));
    }
    vector<vector < pair<double, vector<unsigned>>>> array;

    for (unsigned i = 0; i < num; i++) {
        vector<pair<double, vector<unsigned>>> temp;
        array.push_back(temp);
        for (unsigned j = 0; j < num; j++) {
            vector<unsigned> empty;
            array[i].push_back(make_pair(0, empty));
        }
    }
    thread my_thread[points.size()];
    for (unsigned i = 0; i < points.size(); i++) {
        unsigned index = points[i];
        my_thread[i] = thread(initial, ref(array), index, ref(check_index));
    }

    for (unsigned i = 0; i < points.size(); i++) {
        my_thread[i].join();
    }
    unsigned index = 0;
    unsigned depotid = depots.size();
    int flag = 0;
    double min_time = INF;
    vector<vector<unsigned>> path_array;
    for (unsigned i = 0; i < depots.size(); i++) {
        vector<unsigned> delivery_ints_ = delivery_ints;
        unsigned id = depots[i];
        vector<unsigned> temp = greedy(id, delivery_ints_, deliveries, check_index, array);
        if (temp.size() != 0) {
            depotid = depots[i];
            index = temp[1];
            flag = 1;
            break;
        }
    }
    if (flag == 0) return {
    };
    priority_queue<pair<unsigned, double>, vector<pair<unsigned, double>>, prioritize> pq;
    priority_queue<pair<unsigned, double>, vector<pair<unsigned, double>>, prioritize_> pq_;
    pq.push(make_pair(index, 0));
    pq_.push(make_pair(index, 0));
    for (unsigned i = 0; i < delivery_ints.size(); i++) {
        if (delivery_ints[i] == index) continue;
        unsigned id1 = check_index[depotid];
        unsigned id2 = check_index[delivery_ints[i]];
        double time = array[id1][id2].first;
        if (time == 0) continue;
        pq.push(make_pair(delivery_ints[i], time));
        pq_.push(make_pair(delivery_ints[i], time));
    }

    vector<unsigned> list;

    for (unsigned i = 0; i < 4 && i < pq.size(); i++) {
        list.push_back(pq.top().first);
        list.push_back(pq_.top().first);
        pq.pop();
        pq_.pop();
    }
    //list.push_back(index);
    for (unsigned i = 0; i < list.size() && i < 4; i++) {
        path_array.push_back({});
        unsigned id = list[i];
        vector<unsigned> delivery_ints_ = delivery_ints;
        for (unsigned j = 0; j < deliveries.size(); j++) {
            if (id == deliveries[j].pickUp) {
                if (find(delivery_ints_.begin(), delivery_ints_.end(), deliveries[j].dropOff) == delivery_ints_.end()) {
                    delivery_ints_.push_back(deliveries[j].dropOff);
                }
            }
        }
        delivery_ints_.erase(find(delivery_ints_.begin(), delivery_ints_.end(), id));
        path_array[path_array.size() - 1] = greedy(id, delivery_ints_, deliveries, check_index, array);
        if (path_array[path_array.size() - 1].size() == 0) {
            path_array.erase(path_array.end() - 1);
            continue;
        }
        path_array[path_array.size() - 1].insert(path_array[path_array.size() - 1].begin(), list[i]);
        path_array[path_array.size() - 1].insert(path_array[path_array.size() - 1].begin(), depotid);
        unsigned size = path_array[path_array.size() - 1].size();
        unsigned index = path_array[path_array.size() - 1][size - 1];
        vector<unsigned> temp = find_closest_depot(depots, index);
        path_array[path_array.size() - 1].push_back(index);
    }
    num = path_array.size();
    thread my_thread2[num];
    for (unsigned i = 0; i < num; i++) {
        my_thread2[i] = thread(two_opt, ref(path_array[i]), ref(deliveries), ref(check_index), ref(array), ref(pickup_info), ref(dropoff_info));
        my_thread2[i].join();
    }
    for (unsigned i = 0; i < num; i++) {

    }
    for (unsigned id = 0; id < path_array.size(); id++) {
        vector<unsigned> final_path;
        index = path_array[id][path_array[id].size() - 2];
        vector<unsigned> temp = find_closest_depot(depots, index);
        path_array[id][path_array[id].size() - 1] = index;
        for (unsigned i = 1; i < path_array[id].size() - 2; i++) {
            unsigned id1 = check_index[path_array[id][i]];
            unsigned id2 = check_index[path_array[id][i + 1]];
            //if(array[id1][id2].second.size()==0) cout<<"here"<<endl;
            final_path.insert(final_path.end(), array[id1][id2].second.begin(), array[id1][id2].second.end());
        }

        vector<unsigned> path1 = find_path_between_intersections(path_array[id][0], path_array[id][1]);
        vector<unsigned> path2 = find_path_between_intersections(path_array[id][path_array[id].size() - 2], path_array[id][path_array[id].size() - 1]);
        path1.insert(path1.end(), final_path.begin(), final_path.end());
        final_path = path1;

        final_path.insert(final_path.end(), path2.begin(), path2.end());
        double time = compute_path_travel_time(final_path);
        if (time < min_time) {
            min_time = time;
            return_path = final_path;
        }
    }

    /*
start:
    for (unsigned i = 1; i < path.size() - 2; i++) {
        for (unsigned j = path.size() - 2; j > i; j--) {

            if (can_swap(i, j, path, deliveries, check_index, array, pickup_info, dropoff_info)) {
                count += 1;
                auto current_time = chrono::high_resolution_clock::now();
                auto wallclock = chrono::duration_cast<chrono::duration<double>> (current_time - starttime);
                //cout<<wallclock.count()<<","<<i<<","<<j<<endl;
                if (wallclock.count() <= 0.85 * TIME_LIMIT) goto start;
            }
        }
    }*/

    //cout << count << endl;



    return return_path;
}

void two_opt(vector<unsigned>& path, const std::vector<DeliveryInfo>& deliveries, unordered_map<unsigned, unsigned>& check_index, vector<vector<pair<double, vector<unsigned>>>>&array, unordered_map<unsigned, vector<unsigned>>&pickup_info, unordered_map<unsigned, vector<unsigned>>&dropoff_info) {

start:
    for (unsigned i = 1; i < path.size() - 2; i++) {
        for (unsigned j = path.size() - 2; j > i; j--) {
            auto current_time = chrono::high_resolution_clock::now();
            auto wallclock = chrono::duration_cast<chrono::duration<double>> (current_time - starttime);
            if (wallclock.count() > 0.85*TIME_LIMIT) return;
            if (can_swap(i, j, path, deliveries, check_index, array, pickup_info, dropoff_info)) {
                goto start;
            }
        }
    }
}

bool can_swap(unsigned index1, unsigned index2, vector<unsigned>& path, const std::vector<DeliveryInfo>& deliveries, unordered_map<unsigned, unsigned>& check_index, vector<vector<pair<double, vector<unsigned>>>>&array, unordered_map<unsigned, vector<unsigned>>&pickup_info, unordered_map<unsigned, vector<unsigned>>&dropoff_info) {

    if (check_legal(index1, index2, deliveries, path, pickup_info, dropoff_info)) {
        vector<unsigned> copy_path = path;
        vector<unsigned> temp;
        for (unsigned i = index1; i <= index2; i++) {
            temp.insert(temp.begin(), path[i]);
        }
        for (unsigned i = index1; i <= index2; i++) {
            copy_path[i] = temp[i - index1];
        }
        double previous_time = 0;
        double current_time = 0;
        for (unsigned i = index1 - 1; i <= index2; i++) {
            unsigned id1 = check_index[path[i]];
            unsigned id2 = check_index[path[i + 1]];
            previous_time += array[id1][id2].first;
            id1 = check_index[copy_path[i]];
            id2 = check_index[copy_path[i + 1]];
            if (array[id1][id2].first == 0 && id1 != id2) return false;
            current_time += array[id1][id2].first;
        }
        if (previous_time > current_time) {
            unsigned up = copy_path.size() - index2;
            unsigned i = index1 - 1;
            while (copy_path.size() - i >= up) {
                if (copy_path[i] == copy_path[i + 1]) {
                    copy_path.erase(copy_path.begin() + i);
                } else {
                    i = i + 1;
                }
            }
            path = copy_path;
            return true;
        }
    }
    return false;
}

bool check_legal(unsigned index1, unsigned index2, const std::vector<DeliveryInfo>& deliveries, vector<unsigned>& path, unordered_map<unsigned, vector<unsigned>>&pickup_info, unordered_map<unsigned, vector<unsigned>>&dropoff_info) {
    unordered_map<unsigned, unsigned> check;
    for (unsigned i = index1; i <= index2; i++) {
        if (pickup_info.find(path[i]) != pickup_info.end()) {
            for (unsigned j = 0; j < pickup_info[path[i]].size(); j++) {
                unsigned id = pickup_info[path[i]][j];
                if (check.find(id) == check.end()) check.insert(make_pair(deliveries[id].dropOff, 0));
            }
        }
        if (check.find(path[i]) != check.end()) return false;
    }
    return true;
}

vector<unsigned> find_closest_depot(const vector<unsigned>& depots, unsigned& start) {
    vector<int> closest_depot = my_database.closest_intersections;
    for (unsigned i = 0; i < depots.size(); i++) {
        closest_depot[depots[i]] = 1;
    }
    unsigned des;
    unsigned copy = start;
    vector<int> visited = my_database.visited;
    vector<double> time = my_database.time;
    time[start] = 0; //arrival time of start point is zero
    vector<unsigned> come_from_ints = my_database.come_from_ints; //previous intersection of an intersection
    vector<unsigned> come_from_segs = my_database.come_from_segs; //previous street segment of an intersection
    priority_queue<pair<unsigned, double>, vector<pair<unsigned, double>>, prioritize> pq;
    if (depots.size() == 1) pq.push(make_pair(start, h_value(start, depots[0])));
    else pq.push(make_pair(start, 0));
    int flag = 0; //this flag is used to judge whether we find a legal path
    while (!pq.empty()) {
        pair<unsigned, double> cur = pq.top();
        pq.pop();

        if (closest_depot[cur.first] == 1) {
            flag = 1;
            des = cur.first;
            start = cur.first;
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
            if (cur.first != start) {
                unsigned lastid = come_from_segs[cur.first];
                /*check whether there is a turning*/
                if (getStreetSegmentInfo(lastid).streetID != info.streetID) {
                    p_time += 0.25; //add 15 seconds
                }
            }
            /*if this path is a better path, then record it*/
            if (p_time < time[next_intersection_id]) {
                time[next_intersection_id] = p_time; //update the arrival time
                pq.push(make_pair(next_intersection_id, time[next_intersection_id]));
                come_from_ints[next_intersection_id] = cur.first;
                come_from_segs[next_intersection_id] = segid;
            }

        }
    }

    if (flag == 0) {
        return
        {
        };

    }
    unsigned i = des;
    vector<unsigned> path;
    /*reconstruct the path*/
    while (i != start) {
        path.insert(path.begin(), come_from_segs[i]);
        i = come_from_ints[i];
    }
    //cout << path.size() << endl;
    return path;

}

vector<unsigned> find_closest_delivery(unsigned start, vector<unsigned>& ints, unsigned& index, unordered_map<unsigned, unsigned>& check_index, vector<vector<pair<double, vector<unsigned>>>>&array) {
    vector<unsigned> empty = {};
    vector<int> closest_intersection = my_database.closest_intersections; //this vector stores whether  a specific intersection has been visited
    vector<unsigned> path; //this vector returns the total path
    vector<int> visited = my_database.visited;
    vector<double> time = my_database.time;
    time[start] = 0; //arrival time of start point is zero
    vector<unsigned> come_from_ints = my_database.come_from_ints; //previous intersection of an intersection
    vector<unsigned> come_from_segs = my_database.come_from_segs; //previous street segment of an intersection
    for (unsigned i = 0; i < ints.size(); i++) {
        closest_intersection[ints[i]] = 1;
    }
    priority_queue<pair<unsigned, double>, vector<pair<unsigned, double>>, prioritize> pq;
    if (ints.size() == 1) pq.push(make_pair(start, h_value(start, ints[0])));
    else pq.push(make_pair(start, 0));
    int flag = 0; //this flag is used to judge whether we find a legal path
    while (!pq.empty()) {
        /*pop the intersection with smallest f value from the queue*/
        pair<unsigned, double> cur = pq.top();
        pq.pop();
        /*check whether this intersection is one of the closest intersections of pois*/
        if (closest_intersection[cur.first] == 1) {
            flag = 1; //set flag to 1
            index = cur.first; //store this intersection id
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
            if (cur.first != start) {
                unsigned lastid = come_from_segs[cur.first];
                /*check whether there is a turning*/
                if (getStreetSegmentInfo(lastid).streetID != info.streetID) {
                    p_time += 0.25; //add 15 seconds
                }
            }
            /*if this path is a better path, then record it*/
            if (p_time < time[next_intersection_id]) {
                time[next_intersection_id] = p_time; //update the arrival time
                if (ints.size() == 1) pq.push(make_pair(next_intersection_id, time[next_intersection_id] + h_value(next_intersection_id, ints[0]))); //push it into queue
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
    unsigned i = index;
    /*reconstruct the path*/
    while (i != start) {
        path.insert(path.begin(), come_from_segs[i]);
        i = come_from_ints[i];
    }
    return path;
}

vector<unsigned> greedy(unsigned& index, vector<unsigned>& delivery_ints, const std::vector<DeliveryInfo>& deliveries, unordered_map<unsigned, unsigned>& check_index, vector<vector<pair<double, vector<unsigned>>>>&array) {
    vector<unsigned> path;
    path.push_back(index);
    int flag = 0;
    vector<unsigned> empty;
    int count = 0;
    while (delivery_ints.size() != 0) {
        vector<unsigned> temp_path = find_closest_delivery(index, delivery_ints, index, check_index, array);
        if (delivery_ints.size() != 0 && temp_path.size() == 0) {
            flag = 1;
            break;
        }
        path.push_back(index);
        for (unsigned j = 0; j < deliveries.size(); j++) {
            if (index == deliveries[j].pickUp) {
                if (find(delivery_ints.begin(), delivery_ints.end(), deliveries[j].dropOff) == delivery_ints.end()) {
                    delivery_ints.push_back(deliveries[j].dropOff);
                }
            }
        }
        delivery_ints.erase(find(delivery_ints.begin(), delivery_ints.end(), index));
        count += 1;
    }
    if (flag == 1) return empty;
    return path;
}

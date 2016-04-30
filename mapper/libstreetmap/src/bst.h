/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   bst.h
 * Author: gaobowen
 *
 * Created on March 12, 2016, 3:21 PM
 */

#ifndef BST_H
#define BST_H

#include <string>
using namespace std;

class node {
public:
    string type;
    string name;
    node* left;
    node* right;
    void find(std::string part);
    node(){
        left=NULL;
        right=NULL;
    }
    ~node(){
        delete left;
        left=NULL;
        delete right;
        right=NULL;
    }
    void insert(std::string newname);
};

class BST {
public:
    node* top;
    std::string type;
    BST(std::string my_type){
        type=my_type;
        top=NULL;
    }
    BST(){
        top=NULL;
    }
    ~BST(){
        delete top;
        top=NULL;
    }
    void find(std::string part);
    void insert(std::string name);
};


#endif /* BST_H */


/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "bst.h"
#include "database.h"
extern database my_database;
void node::find(std::string part) {
    
    my_database.count+=1;
    int match = 1;
    if(part.size() > name.size()) match = 0;
    for (unsigned i = 0; i < part.size(); i++) {
        if (part[i] != name[i]) {
            match = 0;
            break;
        }
    }
    if (match == 1) {
        if (type == "POI") {
            my_database.poi_partial_match.push_back(name);
        } else if(type=="Street") {
            my_database.street_partial_match.push_back(name);
        }
        else{
            my_database.intersection_partial_match.push_back(name);
        }
    }
    if(match==1){
        if(right!=NULL) right->find(part);
        if(left!=NULL)  left->find(part);
    }
    else{
        if(right!=NULL && part>name) right->find(part);
        if(part<name && left!=NULL) left->find(part);
    }
}

void BST::find(std::string part){
    if(top==NULL){
        return;
    }
    top->find(part);
}

void node::insert(std::string newname) {
    if (newname < name) {
        if (left == NULL) {
            node* newnode=new node;
            newnode->name = newname;
            newnode->type=type;
            left = newnode;
        } else {
            left->insert(newname);
        }
    } else if (newname == name) {
        return;
    } else {
        if (right == NULL) {
            node* newnode = new node;
            newnode->name = newname;
            newnode->type=type;
            right = newnode;
        } else {
            right->insert(newname);
        }
    }
}

void BST::insert(std::string newname){
    if(top==NULL){
        node* newnode=new node;
        newnode->type=type;
        newnode->name=newname;
        top=newnode;
    }
    else{
        
       top->insert(newname);
    }
}
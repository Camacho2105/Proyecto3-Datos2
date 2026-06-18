#ifndef BTREE_H
#define BTREE_H

#include <string>

class BTreeNode {
public:
    std::string *keys;
    int *recordLines;       
    int t;
    BTreeNode **C;
    int n;
    bool leaf;

    BTreeNode(int _t, bool _leaf);
    
    void insertNonFull(std::string k, int line);
    
    void splitChild(int i, BTreeNode *y);
    

    int search(std::string k);
};


class BTree {
public:
    BTreeNode *root;
    int t;

    BTree(int _t = 3) {
        root = nullptr;
        t = _t;
    }

    void insert(std::string k, int line);
    int search(std::string k);
};

#endif
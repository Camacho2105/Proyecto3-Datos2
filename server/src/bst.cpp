#include "../include/bst.h"

using namespace std;

BST::BST() {
    root = nullptr;
}


void BST::insert(string key, int recordLine) {
    root = insertRec(root, key, recordLine);
}


BSTNode* BST::insertRec(BSTNode* node, string key, int recordLine) {
    if (node == nullptr) {
        return new BSTNode(key, recordLine);
    }
    

    if (key < node->key) {
        node->left = insertRec(node->left, key, recordLine);
    } 

    else if (key > node->key) {
        node->right = insertRec(node->right, key, recordLine);
    }
    
    return node;
}


int BST::search(string key) {
    return searchRec(root, key);
}

int BST::searchRec(BSTNode* node, string key) {
    if (node == nullptr) {
        return -1;
    }
    
    if (node->key == key) {
        return node->recordLine;
    }
    
    if (key < node->key) {
        return searchRec(node->left, key);
    }
    
    return searchRec(node->right, key);
}
#ifndef BST_H
#define BST_H

#include <string>

// Estructura del Nodo del árbol
struct BSTNode {
    std::string key;
    int recordLine;
    BSTNode* left;
    BSTNode* right;

    BSTNode(std::string k, int line) : key(k), recordLine(line), left(nullptr), right(nullptr) {}
};


class BST {
public:
    BSTNode* root;

    BST();
    void insert(std::string key, int recordLine);
    int search(std::string key);

private:
    BSTNode* insertRec(BSTNode* node, std::string key, int recordLine);
    int searchRec(BSTNode* node, std::string key);
};

#endif
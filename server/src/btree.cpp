#include "../include/btree.h"

using namespace std;

BTreeNode::BTreeNode(int _t, bool _leaf) {
    t = _t;
    leaf = _leaf;
    keys = new string[2 * t - 1];
    recordLines = new int[2 * t - 1];
    C = new BTreeNode *[2 * t];
    n = 0;
}

int BTreeNode::search(string k) {
    int i = 0;
    while (i < n && k > keys[i]) {
        i++;
    }
    if (i < n && keys[i] == k) {
        return recordLines[i];
    }
    if (leaf) {
        return -1;
    }
    return C[i]->search(k);
}

void BTreeNode::insertNonFull(string k, int line) {
    int i = n - 1;

    if (leaf) {
       
        while (i >= 0 && keys[i] > k) {
            keys[i + 1] = keys[i];
            recordLines[i + 1] = recordLines[i];
            i--;
        }
        keys[i + 1] = k;
        recordLines[i + 1] = line;
        n = n + 1;
    } else {
        
        while (i >= 0 && keys[i] > k) {
            i--;
        }
        if (C[i + 1]->n == 2 * t - 1) {
            splitChild(i + 1, C[i + 1]);
            if (keys[i + 1] < k) {
                i++;
            }
        }
        C[i + 1]->insertNonFull(k, line);
    }
}

void BTreeNode::splitChild(int i, BTreeNode *y) {
    BTreeNode *z = new BTreeNode(y->t, y->leaf);
    z->n = t - 1;

    for (int j = 0; j < t - 1; j++) {
        z->keys[j] = y->keys[j + t];
        z->recordLines[j] = y->recordLines[j + t];
    }

    if (!y->leaf) {
        for (int j = 0; j < t; j++) {
            z->C[j] = y->C[j + t];
        }
    }

    y->n = t - 1;

    for (int j = n; j >= i + 1; j--) {
        C[j + 1] = C[j];
    }
    C[i + 1] = z;

    for (int j = n - 1; j >= i; j--) {
        keys[j + 1] = keys[j];
        recordLines[j + 1] = recordLines[j];
    }
    keys[i] = y->keys[t - 1];
    recordLines[i] = y->recordLines[t - 1];
    n = n + 1;
}


int BTree::search(string k) {
    if (root == nullptr) return -1;
    return root->search(k);
}

void BTree::insert(string k, int line) {
    if (root == nullptr) {
        root = new BTreeNode(t, true);
        root->keys[0] = k;
        root->recordLines[0] = line;
        root->n = 1;
    } else {
        if (root->n == 2 * t - 1) {
            BTreeNode *s = new BTreeNode(t, false);
            s->C[0] = root;
            s->splitChild(0, root);
            int i = 0;
            if (s->keys[0] < k) {
                i++;
            }
            s->C[i]->insertNonFull(k, line);
            root = s;
        } else {
            root->insertNonFull(k, line);
        }
    }
}
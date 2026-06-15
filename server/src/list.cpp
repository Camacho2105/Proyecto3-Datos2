#include "list.h"

void initColumnList(ColumnList& list) {
    list.head = nullptr;
}

void addColumn(ColumnList& list, const std::string& name, const std::string& type) {
    ColumnNode* newNode = new ColumnNode;
    newNode->data.name = name;
    newNode->data.type = type;
    newNode->next = nullptr;

    if (list.head == nullptr) {
        list.head = newNode;
        return;
    }

    ColumnNode* current = list.head;

    while (current->next != nullptr) {
        current = current->next;
    }

    current->next = newNode;
}

void freeColumnList(ColumnList& list) {
    ColumnNode* current = list.head;

    while (current != nullptr) {
        ColumnNode* temp = current;
        current = current->next;
        delete temp;
    }

    list.head = nullptr;
}
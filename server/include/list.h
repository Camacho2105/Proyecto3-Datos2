#ifndef LIST_H
#define LIST_H

#include <string>

struct Column {
    std::string name;
    std::string type;
};

struct ColumnNode {
    Column data;
    ColumnNode* next;
};

struct ColumnList {
    ColumnNode* head;
};

void initColumnList(ColumnList& list);
void addColumn(ColumnList& list, const std::string& name, const std::string& type);
void freeColumnList(ColumnList& list);

#endif
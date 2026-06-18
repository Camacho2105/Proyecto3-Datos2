#ifndef QUICKSORT_H
#define QUICKSORT_H

#include <string>

struct IndexRecord {
    std::string key;
    int recordLine;
};

void quickSort(IndexRecord arr[], int low, int high);

#endif
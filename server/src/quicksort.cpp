#include "../include/quicksort.h"


static void swapRecords(IndexRecord& a, IndexRecord& b) {
    IndexRecord temp = a;
    a = b;
    b = temp;
}


static int partition(IndexRecord arr[], int low, int high) {
    std::string pivot = arr[high].key;
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        
        if (arr[j].key < pivot) {
            i++;
            swapRecords(arr[i], arr[j]);
        }
    }
    swapRecords(arr[i + 1], arr[high]);
    return (i + 1);
}


void quickSort(IndexRecord arr[], int low, int high) {
    if (low < high) {
        
        int pi = partition(arr, low, high);

        
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}
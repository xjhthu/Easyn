#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <unordered_map>
#include <string>
#include "PlaceInfo.hpp"
#include "timing.hpp"

template<typename T>
class Hashtable {
    std::unordered_map<std::string , T> htmap;

public:
    void put(std::string key, T value) {
            htmap[key] = value;
    }

    T get(std::string key){
        return htmap[key];
    }
    // const void *get(const void *key) {
    //         return htmap[key];
    // }

};

#endif
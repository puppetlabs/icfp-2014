#pragma once

#include <boost/heap/priority_queue.hpp>

struct LambdaWorld {
    boost::heap::priority_queue<int> moves;

    void run();
};


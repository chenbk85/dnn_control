#ifndef SYSTEMSTATE_H
#define SYSTEMSTATE_H

#include <boost/array.hpp>

// The spacecraft's state contains (r, dr, m)
typedef boost::array<double,7> SystemState;

#endif // SYSTEMSTATE_H

#ifndef MMSB_COMMON_HPP_
#define MMSB_COMMON_HPP_

#include <cmath>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <string>
#include <utility>  // pair
#include <vector>
#include <limits>
#include <algorithm>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include "proto/mmsb.pb.h"

// gflags 2.1 issue: namespace google was changed to gflags without warning.
// Luckily we will be able to use GFLAGS_GFAGS_H_ to detect if it is version
// 2.1. If yes , we will add a temporary solution to redirect the namespace.
#ifndef GFLAGS_GFLAGS_H_
namespace gflags = google;
#endif  // GFLAGS_GFLAGS_H_

// A simple macro to mark codes that are not implemented, so that when the code
// is executed we will see a fatal log.
#define NOT_IMPLEMENTED LOG(FATAL) << "Not Implemented Yet"

namespace mmsb {

using std::fstream;
using std::ios;
using std::isnan;
using std::iterator;
using std::make_pair;
using std::vector;
using std::map;
using std::unordered_map;
using std::ostringstream;
using std::pair;
using std::set;
using std::unordered_set;
using std::string;
using std::stringstream;
using std::max;
using std::min;

// Typedefs
typedef unsigned short uint16; // Should work for all x86/x64 compilers
typedef unsigned int uint32; // Should work for all x86/x64 compilers
typedef uint32 VIndex; // Vertex index
typedef uint32 CIndex; // Community index
typedef uint32 Count;  // count

} // namespace mmsb

#endif

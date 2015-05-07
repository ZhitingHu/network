#include "context.hpp"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <time.h>

namespace mmsb {

Context& Context::Get()
{
  static Context instance;
  return instance;
}

Context::Context() {
  std::vector<google::CommandLineFlagInfo> flags;
  google::GetAllFlags(&flags);
  for (size_t i = 0; i < flags.size(); i++) {
    google::CommandLineFlagInfo& flag = flags[i];
    ctx_[flag.name] = flag.is_default ? flag.default_value : flag.current_value;
  }
}

void Context::Init() {
  int rand_seed = get_int32("random_seed");
  if (rand_seed >= 0) {
    random_generator_ = new Random(rand_seed);
  } else {
    random_generator_ = new Random(time(NULL));
  }
}

}   // namespace mmsb

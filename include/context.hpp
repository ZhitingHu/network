#ifndef MMSB_CONTEXT_HPP_
#define MMSB_CONTEXT_HPP_

#include "common.hpp"
#include "random.hpp"
//#include "thread_barrier.hpp"

#include <unordered_map>
#include <boost/shared_ptr.hpp>

namespace mmsb {

// An extension of google flags. It is a singleton that stores 1) google flags
// and 2) other lightweight global flags. Underlying data structure is map of
// string and string, similar to google::CommandLineFlagInfo.
class Context {
 public:
  static Context& Get();
  void Init();

  static inline int get_int32(std::string key) {
    return atoi(get_string(key).c_str());
  }
  static inline double get_double(std::string key) {
    return atof(get_string(key).c_str());
  }
  static inline bool get_bool(std::string key) {
    return get_string(key).compare("true") == 0;
  }
  static inline std::string get_string(std::string key) {
    Get();
    auto it = Get().ctx_.find(key);
    LOG_IF(FATAL, it == Get().ctx_.end())
        << "Failed to lookup " << key << " in context.";
    return it->second;
  }

  static inline void set(std::string key, int value) {
    Get().ctx_[key] = std::to_string(value);
  }
  static inline void set(std::string key, double value) {
    Get().ctx_[key] = std::to_string(value);
  }
  static inline void set(std::string key, bool value) {
    Get().ctx_[key] = (value) ? "true" : "false";
  }
  static inline void set(std::string key, std::string value) {
    Get().ctx_[key] = value;
  }

  enum Phase { kTrain, kTest };
  inline static Phase phase() { 
    return Get().phase_; 
  }
  inline static void set_phase(Phase phase) {
    Get().phase_ = phase; 
  }

  inline static float rand() {
    return Get().random_generator_->rand();
  }
  inline static uint32 randUInt32() {
    return Get().random_generator_->randInt();
  }
  inline static size_t randDiscrete(const vector<float>& distrib, 
      size_t begin, size_t end) {
    return Get().random_generator_->randDiscrete(distrib, begin, end);
  }

 private:
  // Private constructor. Store all the gflags values.
  Context();

 private:
  // Underlying data structure
  std::unordered_map<std::string, std::string> ctx_;

  Phase phase_;

  Random* random_generator_;
};

}   // namespace mmsb

#endif

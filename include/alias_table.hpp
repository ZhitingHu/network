#ifndef MMSB_ALIAS_TABLE_HPP_
#define MMSB_ALIAS_TABLE_HPP_

#include "common.hpp"
#include "context.hpp"

namespace mmsb {

class AliasTable {
 public:
  AliasTable(): sampled_times_(0) { };
  
  /// p: (un-normalized) distribution
  void BuildAliasTable(const vector<float>& p);
  uint32_t Propose();

  inline size_t sampled_times() const { return sampled_times_; }

 private:

 private:
  vector<uint32_t> alias_;
  vector<uint64_t> height_;
  size_t sampled_times_;
};

} // namespace mmsb 

#endif

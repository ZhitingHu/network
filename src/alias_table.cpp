
#include "alias_table.hpp"

namespace mmsb {


/**
 * Refer to http://pandasthumb.org/archives/2012/08/lab-notes-the-a.html 
 *
 * TODO: use float-precision
 */
void AliasTable::BuildAliasTable(const vector<float>& p) {
  const size_t dim = p.size();
  alias_.resize(dim);
  height_.resize(dim);

  // Normalize p and copy into buffer
  double f = 0.0, pp[dim];
  for(const auto e : p) {
    f += e;
  }
  f = dim / f;
  for(int i = 0; i < dim; ++i) {
    pp[i] = p[i] * f;
  }

  // Find starting positions
  size_t g, // index for the current large bar (non-decrease)
         m, // index for the current small bar
         mm;// index for the next possible small bar (non-decrease)
  for(g = 0; g < dim && pp[g] < 1.0; ++g)
    /*noop*/;
  for(m = 0; m < dim && pp[m] >= 1.0; ++m)
    /*noop*/;
  mm = m + 1;

  // Build alias table until we run out of large or small bars
  while(g < dim && m < dim) {
    // convert double to 64-bit integer, control for precision
    height_[m] 
        = (static_cast<uint64_t>(ceil(pp[m] * 9007199254740992.0)) << 11);
    alias_[m] = g;
    pp[g] = (pp[g] + pp[m]) - 1.0;
    if(pp[g] >= 1.0 || mm <= g) {
      for(m = mm; m < dim && pp[m] >= 1.0; ++m)
        /*noop*/;
      mm = m + 1;
    } else {
      m = g;
    }
    for(; g < dim && pp[g] < 1.0; ++g)
      /*noop*/;
  }
  
  // Any bars that remain have no alias 
  for(; g < dim; ++g) {
    if(pp[g] < 1.0) continue;
    height_[g] = std::numeric_limits<boost::uint64_t>::max();
    alias_[g] = g;
  }
  if(m < dim) {
    alias_[m] = m;
    for(m = mm; m < dim; ++m) {
      if(p[m] > 1.0) continue;
      height_[m] = std::numeric_limits<boost::uint64_t>::max();
      alias_[m] = m;
    }
  }

  sampled_times_ = 0;
}

uint32_t AliasTable::Propose() {
  ++sampled_times_;

  uint64_t u = Context::randUInt64();
  uint32_t x = Context::randUInt64() % alias_.size();
  return (u < height_[x]) ? x : alias_[x];
}

} // namespace mmsb

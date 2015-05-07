
#include "vertex.hpp"

namespace mmsb {

void Vertex::ToProto(VertexParameter* param) {
  param->Clear();
  for (const auto& nz : neighbor_z_) {
    param->add_neighbors(nz.first);
    param->add_z(nz.second);
  } 
}

void Vertex::FromProto(const VertexParameter& param) {
  CHECK_EQ(neighbor_z_.size(), param.neighbors_size());
  z_cnts_.clear();
  for (int j = 0; j < param.neighbors_size(); ++j) {
    SetZ(param.neighbors(j), param.z(j));
  }
}

} // namespace mmsb

#ifndef MMSB_MMSB_MODEL_HPP_
#define MMSB_MMSB_MODEL_HPP_

#include "vertex.hpp"
#include "context.hpp"

#include <ctime>

namespace mmsb {

class MMSBModel {
 public:
  MMSBModel(const ModelParameter& param);
  MMSBModel(const string& param_file);

  void Solve(const char* resume_file = NULL);
  inline void Solve(const string& resume_file) {
    Solve(resume_file.c_str());
  }

  void ReadData();

  void Snapshot();
  void SnapshotVis(); // temp: for visualization/debug
  void ToProto(ModelParameter* param);
  void Restore(const char* snapshot_file = NULL);

  inline uint32 K() const { return K_; }
  inline const vector<pair<float, float> >& lambda() const { return lambda_; }

 private:
  void SampleMinibatch(set<VIndex>& vertex_batch,
    vector<pair<VIndex, VIndex> >& link_batch,
    const Count batch_size);
  void CollectLinks(vector<pair<VIndex, VIndex> >& link_batch,
    const set<VIndex>& vertex_batch);
  void GibbsSample();
  void MHSample();
  void VIComputeGrads();
  void VIUpdate();
  void Test();

  void Init(const ModelParameter& param);
  void InitModelState();

  void EstimateBeta();
  float ComputeLinkLikelihood(const VIndex i, const VIndex j,
      const bool positive);
  inline float MembershipProb(const Count cnt, const Count degree);


 private:
  ModelParameter param_;

  vector<Vertex*> vertices_;

  
  uint32 K_; // #communities
  float epsilon_;
  vector<float> beta_; // community strength  
  /// <lambda_0, lambda_1>
  vector<pair<float, float> > lambda_; // prior for beta_
  /// gradients
  vector<pair<float, float> > lambda_grads_; 
  /// intermediate factors for computing gradients
  vector<float> bphi_sum_;
  vector<float> bphi_square_sum_;
   
  pair<float, float> eta_; // prior of community strength
  float alpha_; // prior of community membership

  /* --------- train -------- */
  int iter_;
  float lr_; // learning rate
  Count train_batch_size_;
  set<VIndex> train_batch_vertices_;
  vector<pair<VIndex, VIndex> > train_batch_links_;
  vector<CIndex> train_batch_k_cnts_; // statistics of the samples

  /// test data (postive and negative links)
  vector<pair<VIndex, VIndex> > test_pos_links_;
  vector<pair<VIndex, VIndex> > test_neg_links_;
  //Count test_batch_link_size_;
  //vector<pair<VIndex, VIndex> > test_batch_links_;
  
  // for gibbs sampling, size = K_
  vector<float> comm_probs_;

  clock_t start_time_;
};
 
inline float MMSBModel::MembershipProb(
    const Count cnt, const Count degree) {
  return (cnt + alpha_) / (degree + K_ * alpha_);
}

} // namespace mmsb

#endif

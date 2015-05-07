
#include "io.hpp"
#include "util.hpp"
#include "mmsb_model.hpp"

namespace mmsb {

MMSBModel::MMSBModel(const ModelParameter& param) {
  Init(param);
}
MMSBModel::MMSBModel(const string& param_file) {
  ModelParameter param;
  ReadProtoFromTextFile(param_file, &param);
  Init(param);
}

void MMSBModel::Init(const ModelParameter& param) {
  param_ = param;
  /// 
  K_ = param_.num_comms();
  epsilon_ = param_.epsilon();
  eta_.first = param_.eta_0();
  eta_.second = param_.eta_1();
  //alpha_ = param_.alpha();
  alpha_ = 1.0 / K_;
  beta_.resize(K_);
  lr_ = param_.solver_param().base_lr();
  lambda_.resize(K_);
  lambda_grads_.resize(K_);
  bphi_sum_.resize(K_);
  bphi_square_sum_.resize(K_);
  /// minibatch 
  train_batch_size_ = param_.solver_param().train_batch_size();
  train_batch_k_cnts_.resize(K_);
  /// sampling
  exp_Elog_beta_.resize(K_);
  comm_probs_.resize(K_);
}

/// Input format: from_v_id \t to_v_id
void MMSBModel::ReadData() {
  /// training data
  const string& train_data_path = Context::get_string("train_data");
  LOG(INFO) << "Read train data from " << train_data_path;
  fstream train_data_file(train_data_path.c_str(), ios::in);
  CHECK(train_data_file.is_open()) << "Fail to open " << train_data_path; 
  // header: #v #e
  Count num_vertices = 0;
  train_data_file >> num_vertices;
  LOG(INFO) << "#Vertices\t" << num_vertices;
  vertices_.resize(num_vertices);
  for (VIndex i = 0; i < num_vertices; ++i) {
    vertices_[i] = new Vertex();
  }
  // links
  VIndex i, j;
  while (train_data_file >> i >> j) {
    vertices_[i]->AddLink(j);
    vertices_[j]->AddLink(i);
  }
  train_data_file.close();

  /// test data
  const string& test_data_path = Context::get_string("test_data");
  LOG(INFO) << "Read test data from " << test_data_path;
  fstream test_data_file(test_data_path.c_str(), ios::in);
  CHECK(test_data_file.is_open()) << "Fail to open " << test_data_path; 
  int value = 0;
  while (test_data_file >> i >> j >> value) {
    if (value) {
      test_pos_links_.push_back(make_pair(min(i, j), max(i, j)));
    } else {
      test_neg_links_.push_back(make_pair(min(i, j), max(i, j)));
    }
  }
  LOG(INFO) << "#Test links (pos and neg links)\t"
      << test_pos_links_.size() + test_neg_links_.size();
  test_data_file.close();
}

void MMSBModel::InitModelState() {
  /// community assignment
  for (VIndex i = 0; i < vertices_.size(); ++i) {
    Vertex* v = vertices_[i];
    const unordered_map<VIndex, CIndex>& neighbor_z = v->neighbor_z();
    for (const auto nz : neighbor_z) {
      VIndex j = nz.first;
      if (i >= j) continue;

      CIndex z = Context::randUInt32() % K_;
      v->SetZ(j, z);
      vertices_[j]->SetZ(i, z);
    }
  }

  /// global param
  fill(lambda_.begin(), lambda_.end(), eta_);  

  LOG(INFO) << "Init model state done.";
}

/* -------------------- Train --------------------- */

void MMSBModel::SampleMinibatch(unordered_set<VIndex>& vertex_batch,
    set<pair<VIndex, VIndex> >& link_batch, const Count batch_size) {
  vertex_batch.clear();
  for (int i = 0; i < batch_size; ++i) {
    while (true) {
      VIndex v = Context::randUInt32() % vertices_.size();
      if (vertex_batch.find(v) == vertex_batch.end()) { // to avoid duplicate
        vertex_batch.insert(v);
        break;
      }
    }
  }
  //for (int i = 0; i < vertices_.size(); ++i) {
  //  vertex_batch.insert(i);
  //}
  
  CollectLinks(link_batch, vertex_batch);

  //TODO: seperate ?
  for (const auto& link : link_batch) {
    vertex_batch.insert(link.first);
    vertex_batch.insert(link.second);
  }

  LOG(INFO) << vertex_batch.size() << " vertexes, " 
      << link_batch.size() << " links";
}

void MMSBModel::CollectLinks(set<pair<VIndex, VIndex> >& link_batch,
    const unordered_set<VIndex>& vertex_batch) {
  link_batch.clear();
  for (const auto i : vertex_batch) {
    const unordered_map<VIndex, CIndex>& neighbor_z
        = vertices_[i]->neighbor_z();
    for (const auto& nz : neighbor_z) {
      pair<VIndex, VIndex> link
          = make_pair(min(i, nz.first), max(i, nz.first));
      link_batch.insert(link);
    } // end of neighbors of i
  } // end of vertexes

  //for (const auto i : vertex_batch) {
  //  for (const auto j : vertex_batch) {
  //    if (i < j && vertices_[i]->IsNeighbor(j)) {
  //      link_batch.push_back(make_pair(i, j));
  //    }
  //  }
  //}
}

void MMSBModel::GibbsSample() {
  ComputeExpELogBeta();

for (int ii = 0; ii < 1; ++ii) { //TODO: temp
  fill(train_batch_k_cnts_.begin(), train_batch_k_cnts_.end(), 0);
  for (const auto& link : train_batch_links_) {
    VIndex i = link.first;
    VIndex j = link.second;
    CHECK_LT(i, vertices_.size());
    CHECK_LT(j, vertices_.size());
    Vertex* v_i = vertices_[i];
    Vertex* v_j = vertices_[j];

    CHECK_EQ(v_i->neighbor_z(j), v_j->neighbor_z(i));
    CIndex z_prev = v_i->neighbor_z(j);
    v_i->RemoveZ(z_prev);
    v_j->RemoveZ(z_prev);
   
    // Compute distribution 
    float prob_sum = 0;
    for (int k = 0; k < K_; ++k) {
      int nik = v_i->z_cnt(k);
      int njk = v_j->z_cnt(k);
      float factor_i = alpha_ / (vertices_.size() - 2)
          + (nik == 0 ? 0 : nik * 1.0 / (v_i->degree() - 1)); // to avoid divide-by-0
      float factor_j = alpha_ / (vertices_.size() - 2)
          + (njk == 0 ? 0 : njk * 1.0 / (v_j->degree() - 1)); // to avoid divide-by-0
      comm_probs_[k] = factor_i * factor_j * exp_Elog_beta_[k];

      CHECK(!isnan(comm_probs_[k])) << factor_i << "\t" << factor_j << "\t" << exp_Elog_beta_[k] 
          << "\t" << lambda_[k].second << "\t" << lambda_[k].first << "\t" 
          << mmsb::digamma(lambda_[k].first) << "\t" << mmsb::digamma(lambda_[k].first + lambda_[k].second) << "\t" << k;

      prob_sum += comm_probs_[k];
    }
    // Sample
    const float rand_num = Context::rand() * prob_sum;
    float part_prob_sum = 0;
    CIndex z_new = K_;
    for (int k = 0; k < K_; ++k) {
      part_prob_sum += comm_probs_[k];
      if (part_prob_sum >= rand_num) {
        z_new = k;
        break;
      }
    }
    CHECK_LT(z_new, K_) << rand_num << "\t" << prob_sum << "\t" << part_prob_sum;

    // Update
    v_i->SetZ(j, z_new);
    v_j->SetZ(i, z_new);
    train_batch_k_cnts_[z_new]++;
  } // end of batch
}
}

void MMSBModel::MHSample() {
  
}

void MMSBModel::VIComputeGrads() {
  //float batch_scale 
  //    = vertices_.size() * (vertices_.size() - 1.0) 
  //      / (train_batch_vertices_.size() * (train_batch_vertices_.size() - 1.0));
  //TODO: temp
  //batch_scale = 1.0;
  float batch_scale = 0.5 * vertices_.size() / train_batch_vertices_.size();

  // Reset grads & intermidiate stats
  fill(lambda_grads_.begin(), lambda_grads_.end(), make_pair(0, 0));
  fill(bphi_sum_.begin(), bphi_sum_.end(), 0);
  fill(bphi_square_sum_.begin(), bphi_square_sum_.end(), 0);

  for (const auto i : train_batch_vertices_) {
    CHECK_LT(i, vertices_.size());
    float degree = vertices_[i]->degree();
    CHECK_GT(degree, 0) << i;
    const unordered_map<CIndex, Count>& z_cnts = vertices_[i]->z_cnts();
    for (const auto z_cnt : z_cnts) {
      CHECK_LT(z_cnt.first, K_);
      bphi_sum_[z_cnt.first] += z_cnt.second / degree;
      bphi_square_sum_[z_cnt.first] 
          += (z_cnt.second / degree) * (z_cnt.second / degree);
    }
  } // end of vertices

  for (const auto& link : train_batch_links_) {
    VIndex i = link.first;
    VIndex j = link.second;
    const unordered_map<CIndex, Count>& z_cnts_i = vertices_[i]->z_cnts();
    Vertex* v_j = vertices_[j];
    float degree_i = vertices_[i]->degree();
    float degree_j = v_j->degree();
    //TODO(zhiting) iterates i or j whose degree is smaller
    for (const auto z_cnt_i : z_cnts_i) { 
      CIndex z = z_cnt_i.first;
      lambda_grads_[z].second
          -= (z_cnt_i.second / degree_i) * (v_j->z_cnt(z) / degree_j);
    }
  } // end of links

  for (int k = 0; k < K_; ++k) {
    lambda_grads_[k].first += 2.0 * train_batch_k_cnts_[k]; // !!Caution

    CHECK(!isnan(lambda_grads_[k].first)) << k << "\t" << train_batch_k_cnts_[k] << "\t" << batch_scale;
    CHECK(!isinf(lambda_grads_[k].first)) << k << "\t" << train_batch_k_cnts_[k] << "\t" << batch_scale;

    lambda_grads_[k].second
        += (bphi_sum_[k] * bphi_sum_[k] - bphi_square_sum_[k]) / 2.0;

    CHECK(!isnan(lambda_grads_[k].second)) << k << "\t" << bphi_sum_[k] << "\t" << bphi_square_sum_[k];
  } 
  for (int k = 0; k < K_; ++k) {
    float temp = lambda_grads_[k].first;
    lambda_grads_[k].first = lambda_grads_[k].first * batch_scale + eta_.first;

    CHECK(!isnan(lambda_grads_[k].first)) << k << "\t" << train_batch_k_cnts_[k] << "\t" << temp << "\t" << batch_scale;
    CHECK(!isinf(lambda_grads_[k].first)) << k << "\t" << train_batch_k_cnts_[k] << "\t" << temp << "\t" << batch_scale;

    temp = lambda_grads_[k].second;
    lambda_grads_[k].second = lambda_grads_[k].second * batch_scale + eta_.second;

    CHECK(!isnan(lambda_grads_[k].second)) << k << "\t" << temp << "\t" << batch_scale;
    CHECK(!isinf(lambda_grads_[k].second)) << k << "\t" << temp << "\t" << batch_scale;
  }
}

void MMSBModel::VIUpdate() {
  float lr = lr_;
  for (int k = 0; k < K_; ++k) {
    lambda_[k].first
        = lambda_[k].first * (1.0 - lr) + lambda_grads_[k].first * lr;
    lambda_[k].second
        = lambda_[k].second * (1.0 - lr) + lambda_grads_[k].second * lr;
  }
}

void MMSBModel::EstimateBeta() {
  ostringstream oss;
  for (int k = 0; k < K_; ++k) {
    beta_[k] = lambda_[k].first / (lambda_[k].first + lambda_[k].second);
    oss << beta_[k] << "\t";
  }
  //LOG(INFO) << oss.str();
}

void MMSBModel::ComputeExpELogBeta() {
  for (int k = 0; k < K_; ++k) {
    exp_Elog_beta_[k]
        = std::exp(mmsb::digamma(lambda_[k].first)
        - mmsb::digamma(lambda_[k].first + lambda_[k].second));
  }
}

/// Assume beta_ is ready
float MMSBModel::ComputeLinkLikelihood(
    const VIndex i, const VIndex j, const bool positive) {
  float likelihood = 0;
  Vertex* v_i = vertices_[i];
  Vertex* v_j = vertices_[j];
  if (positive) {
    for (int z = 0; z < K_; ++z) {
      likelihood += MembershipProb(v_i->z_cnt(z), v_i->degree())
          * MembershipProb(v_j->z_cnt(z), v_j->degree())
          * beta_[z];
    }
  } else {
    for (int zi = 0; zi < K_; ++zi) {
      for (int zj = 0; zj < K_; ++zj) {
        float bernu_rate = (zi == zj ? beta_[zi] : epsilon_);
        likelihood += MembershipProb(v_i->z_cnt(zi), v_i->degree())
            * MembershipProb(v_j->z_cnt(zj), v_j->degree())
            * (1.0 - bernu_rate);
      }
    }
  }
  CHECK(!isnan(likelihood));
  CHECK(!isinf(likelihood));
  CHECK_GT(likelihood, 0);
  return log(likelihood);
}

void MMSBModel::Test() {
  EstimateBeta();

  float lld = 0, lld_pos = 0, lld_neg = 0; // log likelihood
  Count link_cnt = 0;
  //for (int i = 0; i < vertices_.size(); ++i) {
  //  for (int j = i + 1; j < vertices_.size(); ++j) {
  //    lld += ComputeLinkLikelihood(i, j, vertices_[i]->IsNeighbor(j));
  //    link_cnt++;
  //  }
  //}
  for (const auto& link : test_pos_links_) {
    VIndex i = link.first;
    VIndex j = link.second;
    float lld_ij = ComputeLinkLikelihood(i, j, true);
    lld += lld_ij;
    lld_pos += lld_ij;
    link_cnt++;
  }
  for (const auto& link : test_neg_links_) {
    VIndex i = link.first;
    VIndex j = link.second;
    float lld_ij = ComputeLinkLikelihood(i, j, false);
    lld += lld_ij;
    lld_neg += lld_ij;
    link_cnt++;
  }

  float duration = (std::clock() - start_time_) / (float)CLOCKS_PER_SEC;  
  LOG(ERROR) << iter_ << "\t" << lld / link_cnt << "\t" << link_cnt 
      << "\t" << (lld_pos / test_pos_links_.size()) 
      << "\t" << (lld_neg / test_neg_links_.size())
      << "\t" << duration;
}

void MMSBModel::Solve(const char* resume_file) {
  LOG(INFO) << "Solving MMSB";

  iter_ = 0;
  if (resume_file) {
    LOG(INFO) << "Restoring previous model status from " << resume_file;
    Restore(resume_file);
    LOG(INFO) << "Restoration done.";
  } else {
    InitModelState();
  }

  const int start_iter = iter_;

  start_time_ = std::clock();
  for (; iter_ < param_.solver_param().max_iter(); ++iter_) {
    // Save a snapshot if needed.
    if (param_.solver_param().snapshot() && iter_ > start_iter &&
        iter_ % param_.solver_param().snapshot() == 0) {
      Snapshot();
      SnapshotVis();
    }
    // Test
    if (param_.solver_param().test_interval() 
        && iter_ % param_.solver_param().test_interval() == 0
        && (iter_ > 0 || param_.solver_param().test_initialization())) {
      Test();
    }

    /// 
    clock_t start_data = std::clock();
    SampleMinibatch(train_batch_vertices_, train_batch_links_, train_batch_size_);
    LOG(INFO) << "data time " << (std::clock() - start_data) / (float)CLOCKS_PER_SEC;

    /// local step
    //MHSample();
    clock_t start_sample = std::clock();
    GibbsSample();
    LOG(INFO) << "sample time " << (std::clock() - start_sample) / (float)CLOCKS_PER_SEC;

    /// global step
    clock_t start_vi = std::clock();
    VIComputeGrads();
    VIUpdate();
    LOG(INFO) << "vi time " << (std::clock() - start_vi) / (float)CLOCKS_PER_SEC;
  }
  if (param_.solver_param().snapshot_after_train()) {
    Snapshot(); 
  }
  if (param_.solver_param().test_interval() &&
      iter_ % param_.solver_param().test_interval() == 0) {
    Test();
  }
}

void MMSBModel::ToProto(ModelParameter* param) {
  param->Clear();
  // hyperparams
  param->CopyFrom(param_);
  param->clear_vertices();
  // global params (lambda)
  param->clear_lambda_0();  
  param->clear_lambda_1();
  for (CIndex k = 0; k < K_; ++k) {
    param->add_lambda_0(lambda_[k].first);
    param->add_lambda_1(lambda_[k].second);
  }
  // vertex's params (neighbors, z)
  for (VIndex i = 0; i < vertices_.size(); ++i) {
    Vertex* v = vertices_[i];
    VertexParameter* v_param = param->add_vertices();
    v->ToProto(v_param);
    v_param->set_index(i);
  }
  // solver state
  param->mutable_solver_param()->mutable_solver_state()->set_iter(iter_);
}

void MMSBModel::Snapshot() {
  const string& output_prefix = Context::get_string("mmsb_output");
  ostringstream oss;
  oss << output_prefix << "mmsb.iter." << iter_; 
  string snapshot_filename = oss.str();

  ModelParameter model_param;
  ToProto(&model_param);
  LOG(INFO) << "Snapshotting to " << snapshot_filename;

  WriteProtoToBinaryFile(model_param, snapshot_filename.c_str());
}

void MMSBModel::SnapshotVis() {
  const string& output_prefix = Context::get_string("mmsb_output");
  LOG(INFO) << "Snapshoting (Vis) to " << output_prefix;
  // community strengths: lambda, beta
  EstimateBeta();
  ostringstream oss;
  oss << output_prefix + "/comm_strength_" << iter_ << ".txt"; 
  string lambda_path = oss.str();
  fstream lambda_file(lambda_path.c_str(), ios::out);
  CHECK(lambda_file.is_open()) << "Fail to open " << lambda_path;
  for (int k = 0; k < K_; ++k) {
    lambda_file << k << "\t" << lambda_[k].first << "\t" << lambda_[k].second 
        << "\t" << beta_[k] << "\n";
  }
  lambda_file.flush();
  lambda_file.close();

  // user membership: z_cnts
  oss.str("");
  oss.clear();
  oss << output_prefix + "/user_membership_" << iter_ << ".txt"; 
  string z_path = oss.str();
  fstream z_file(z_path.c_str(), ios::out);
  CHECK(z_file.is_open()) << "Fail to open " << z_path;
  for (int i = 0; i < vertices_.size(); ++i) {
    z_file << i << "\t" << vertices_[i]->degree() << "\t";
    const unordered_map<CIndex, Count>& z_cnts = vertices_[i]->z_cnts();
    for (const auto& e : z_cnts) {
      z_file << e.first << ":" << e.second << "\t"; 
    }
    z_file << "\n";
  }
  z_file.flush();
  z_file.close();

  LOG(INFO) << "Snapshot (Vis) done.";
}

void MMSBModel::Restore(const char* snapshot_file) {
  ModelParameter param;
  ReadProtoFromBinaryFile(snapshot_file, &param);
  // check consistency
  CHECK_EQ(K_, param.num_comms());
  CHECK_EQ(vertices_.size(), param.vertices_size());
  // global param
  for (CIndex k = 0; k < K_; ++k) {
    lambda_[k].first = param.lambda_0(k);
    lambda_[k].second = param.lambda_1(k);
  }
  // vertex's params (neighbors, z)
  for (VIndex i = 0; i < vertices_.size(); ++i) {
    const VertexParameter& v_param = param.vertices(i);
    CHECK_EQ(i, v_param.index());
    vertices_[i]->FromProto(v_param);
  }
  // solver state
  iter_ = param.solver_param().solver_state().iter();
}

} // namespace mmsb

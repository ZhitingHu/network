
#include "common.hpp"
#include "context.hpp"
#include "mmsb_engine.hpp"
#include "mmsb_model.hpp"

namespace mmsb {

MMSBEngine::MMSBEngine() {

}

void MMSBEngine::Init() { 

}

void MMSBEngine::Start() {
  const string& model_path = Context::get_string("model");
  const string& snapshot_path = Context::get_string("snapshot");

  MMSBModel* model = new MMSBModel(model_path);
  model->ReadData();

  if (snapshot_path.size()) {
    LOG(INFO) << "Resuming from " << snapshot_path;
    model->Solve(snapshot_path);
  } else {
    model->Solve();
  }
}

} // namespace mmsb

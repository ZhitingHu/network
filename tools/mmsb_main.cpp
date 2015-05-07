
#include "mmsb.hpp"
#include "gflags/gflags.h"
#include "glog/logging.h"

// MMSB Parameters
DEFINE_string(model, "",
    "The model definition protocol buffer text file.");
DEFINE_string(snapshot, "",
    "Optional; the snapshot state to resume training.");
DEFINE_string(mmsb_output, "",
    "The prefix of the mmsb output file.");
// Data Parameters
DEFINE_string(train_data, "",
    "The training data path.");
DEFINE_string(test_data, "",
    "The test data path.");
//Other Parameters
DEFINE_int32(random_seed, -1,
    "Use system time as rand seed by default.");


int main(int argc, char** argv) {
  // Print output to stderr (while still logging).
  FLAGS_alsologtostderr = 1;
  // Do not buffer
  FLAGS_logbuflevel = -1;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  CHECK_GT(FLAGS_model.size(), 0) << "Need a model definition to train.";

  // Initialize Context
  mmsb::Context& context = mmsb::Context::Get(); 
  context.Init();

  mmsb::MMSBEngine* mmsb_engine = new mmsb::MMSBEngine();
  mmsb_engine->Init();

  mmsb_engine->Start();

  LOG(INFO) << "MMSB finished and shut down!";

  return 0;
}

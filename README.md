# network
====

Dependency
----

This package depends on `glog`, `gflags`, `boost`, and `protobuf`.  After
installing the third-party libraries, specify the third_party path in
Makefile.config


Compile
----

    make all


Usage
----

    sh scripts/train.sh

  * model configuration files are specified by `model_filename` 
  * datasets are specified by `rain_data` and `test_data`
  * if resume training from a snapshot, specify the argument `--snapshot
    snapshot_filename`


Data
----

Two example datasets are included in `data/`, the corresponding model
configuration files are included in `models/`


Tools
----

There are some scripts in scripts, most of which are used for data
pre-processing

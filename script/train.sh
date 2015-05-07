#!/usr/bin/env bash

# Figure out the paths.
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
app_dir=`dirname $script_dir`
progname=mmsb_main
prog_path=${app_dir}/build/tools/${progname}

##=====================================
## Parameters
##=====================================

#dataset=wiki-Vote
#dataset=ca-AstroPh
dataset=web-Google

# Input files:
model_filename="${app_dir}/models/wiki/model.prototxt"
#snapshot_filename="${app_dir}/output/ca-AstroPhmmsb.iter.10000"
snapshot_filename="${app_dir}/output/web-Googlemmsb.iter.2700"

# Datset Parameters
#train_data="${app_dir}/data/wiki-Vote/wiki-Vote_reindex.txt"
#test_data="${app_dir}/data/wiki-Vote/validation-edges.txt"
#
#train_data="${app_dir}/data/ca-AstroPh/ca-AstroPh_reindex.txt"
#test_data="${app_dir}/data/ca-AstroPh/validation-edges.txt"
#
train_data="${app_dir}/data/web-Google/web-Google.txt_reindex"
test_data="${app_dir}/data/web-Google/validation-edges.txt"

##=====================================

output_dir=$app_dir/output
output_dir="${output_dir}/${dataset}"
mkdir -p ${output_dir}

log_dir=$output_dir/logs
outputs_prefix=${output_dir}

cmd="mkdir -p ${log_dir}; \
    GLOG_logtostderr=false \
    GLOG_stderrthreshold=0 \
    GLOG_log_dir=$log_dir \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    $prog_path \
    --model ${model_filename} \
    --mmsb_output ${outputs_prefix} \
    --train_data $train_data \
    --test_data $test_data \
    --snapshot ${snapshot_filename}"

eval $cmd  # Use this to run locally (on one machine).

#!/usr/bin/env bash
echo "Usage: build_container.sh <cobs_repo_URL> <commit_id_or_branch>"
echo "<cobs_repo_URL> defaults to https://github.com/bingmann/cobs if not provided"
echo "<commit_id_or_branch> defaults to master if not provided"
echo "An image with name cobs:<commit_id_or_branch> will be created"
echo "e.g.: build_container.sh"
echo "will create, by default, image cobs:master"

cobs_repo_URL=${1:-"https://github.com/bingmann/cobs"}
commit_id_or_branch=${2:-"master"}

sudo docker build --build-arg cobs_repo_URL=${cobs_repo_URL} --build-arg commit_id_or_branch=${commit_id_or_branch} . -t cobs:"$commit_id_or_branch"
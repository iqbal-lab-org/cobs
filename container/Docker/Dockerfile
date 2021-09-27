FROM ubuntu:18.04

# install basic dependencies
RUN apt update && apt install -y cmake g++ git zlib1g-dev libboost-all-dev

# install cobs
ARG cobs_repo_URL
ARG commit_id_or_branch
RUN git clone --recursive $cobs_repo_URL && cd cobs && git checkout $commit_id_or_branch && mkdir build && cd build && cmake .. && make -j4 && make test

# addo cobs to path
ENV PATH="/cobs/build/src:${PATH}"

ENTRYPOINT ["/cobs/build/src/cobs"]
CMD [""]

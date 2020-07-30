# To build: docker build . -t leandroishilima/cobs:7c030bb
# However, prefer to pull it from dockerhub
FROM ubuntu:18.04
RUN apt update && apt install -y cmake g++ git zlib1g-dev libboost-all-dev
RUN git clone --recursive https://github.com/bingmann/cobs.git && cd cobs && git checkout 7c030bb
RUN mkdir cobs/build && cd cobs/build && cmake .. && make -j4 && make test
ENV PATH="/cobs/build/src:${PATH}"
ENTRYPOINT ["/cobs/build/src/cobs"]
CMD [""]

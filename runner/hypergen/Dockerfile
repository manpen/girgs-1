FROM ubuntu:18.04

RUN apt-get update && apt-get upgrade -y && apt-get install -y \
   git g++-7 ninja-build cmake \
   python3-pip python3-venv

RUN mkdir /runner /data
RUN cd /runner; git clone https://github.com/manpen/networkit.git && git checkout benchmark_hyper
RUN cd /runner/networkit; ./build-benchmarks.sh

CMD cd /data; /runner/networkit/run-benchmarks.sh
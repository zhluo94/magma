# Benchmarks (μTCell)


### Setup
* MPTCP Enabled AMI: `ami-06b93cd6edfe1ee9f` (perfd-cellular-stage-5, us-west-1)
* MPTCP Disabled AMI: `ami-0849b21b423a6baaa` (perfd-cellular-mptcp_disabled, us-west-1)
* The AMI has MPTCP (v0.94) enabled
* The AMI includes mobile applications (more to be added in the long run) with datasets and scripts to start benchmarking, described in what follows. 
* Multimedia communication (e.g. SIP, RTP): [PJSIP](https://github.com/pjsip/pjproject)
* Video streaming (RTMP, HLS):  nginx-rtmp-module, ffmpeg
* Web: [mitmproxy](https://github.com/mitmproxy/mitmproxy)
* Network benchmarks: iperf/iperf3, wrk 

### Run (server-side)

PJSIP

* Register a SIP account (e.g., onsip.com)
* Run pjproject/pjsip-apps/bin/[binary] with --null-audio option [TBD: Makefile]
* [WIP]

Video streaming
* Run `FILE=../clips/[videofile] make stream` under `/mnt/hls/`
* 480p clip: `/mnt/clips/bun480p.mp4` (33s)
* 1080p clip: `/mnt/clips/greenland.mp4` (159s)
* Note: for all benchmarks remember to configure the EC2 security groups to allow traffic of the particular protocols and ports.

Mitmproxy

* TBD

Iperf/iperf3
* Refer to `man iperf`

### Run (client-side)
TBD

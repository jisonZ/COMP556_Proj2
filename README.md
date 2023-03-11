# COMP 556 Project 2: Reliable File Transfer Protocol

#### Notes to grader
We will use 1 slip day for this project.

#### Group members
* Haochen Zhang (jz118@rice.edu)  
* Jinlin Li (jl288@rice.edu)  
* Ye Zhou (yz202@rice.edu)  
* Jiaqi He (jh166@rice.edu)

#### Protocol
In this project, we designed a simple sliding window protocol with a window size of `5` to provide reliable file transfer. Data sent with the sliding window protocol are divided into packets, each with a specific sequence number to ensure proper reordering at the receiver side. Due to potentially large size of the data being transmitted, we read data as `1 MB` buffers and send each buffer sequentially. 

#### Acknowledgement packet (`12 bytes`)
* sequence number: 4 bytes
* error: 4 bytes
* checksum: 4 bytes

#### Data packet (`1120 bytes`)
* packet size: 4 bytes
* sequence number: 4 bytes
* end of file: 4 bytes
* file name: 20 bytes
* directory name: 60 bytes
* message (data): 1024 bytes
* checksum: 4 bytes

#### Usage
1. Compile: `make`
2. Run receiver on CLEAR: `./recvfile -p <recv_port>`
3. Run sender on CAI: `./sendfile -r <recv_host>:<recv_port> -f <subdir>/<filename>`

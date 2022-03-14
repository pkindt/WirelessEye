CSIServer_ng - a simple CSI Server for Nexmon-based Raspberry Pi Setups

2021, 2022 Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>

*About*
This software will gather the packets from NEXMON and make them available for real-time streaming via a TCP socket.

*Usage*:
Compile and run this on your raspberry pi. It will dump UDP packets coming from nexmon on port 5000.
At the same time, it will listen on TCP port 5001. CSIStudio will connect to the CSI Server.

*Compiling*
On a console, type "make"

*Requirements*
Nothing special (Pthread, sockets, GNU make)


 

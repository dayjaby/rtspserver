Starts an RTSP server streaming moving aruco markers:

![aruco marker](images/marker.png)

To build:

```
mkdir build
cd build
cmake ..
make -j4
./rtspserver
```

To play the stream:
```
ffplay rtsp://127.0.0.1:8554/test
```

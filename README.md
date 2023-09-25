# PCP DDS Interfaces

Most idls are copied from ROS2 (12/2022) for basic interoperability.
The interface is then augmented with pcpd specific topic types.

### Build:

```
mkdir cmake-build-debug
cd cmake-build-debug
conan install .. -s build_type=Debug --build missing --build outdated
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### DDS Listener:
```
source activate_run.sh
./bin/ros2_listener
```

### DDS Sender (TBD):
```
source activate_run.sh
./bin/ros2_sender
```

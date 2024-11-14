# TCNART DDS Interfaces

Most idls are copied from ROS2 (12/2022) for basic interoperability.
The interface is then augmented with pcpd specific topic types.

### Build:

```
conan install . -s build_type=Debug --build missing --build outdated
cmake --preset conan-debug
cmake --build ./build/Debug --config Debug
```

md build
cd build
cmake -G "Visual Studio 15 2017 Win64" ..\ -Dgtest_force_shared_crt=on
PAUSE
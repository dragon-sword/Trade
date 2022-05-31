cd Agent
md build
cd build
md Release
md Debug
cmake -A x64 ../
cmake --build .
cmake --build . --config Release
cd ../../

cd Market
md build
cd build
md Release
md Debug
cmake -A x64 ../
cmake --build .
cmake --build . --config Release
cd ../../
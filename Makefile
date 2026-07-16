CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
SRC = main.cpp toygl/toygl.cpp gpu/rasterizer.cpp gpu/shader_core.cpp

triangle: $(SRC) gpu/*.h toygl/toygl.h build/spirv2llvm
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

build/spirv2llvm: spirv2llvm/main.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) spirv2llvm/main.cpp -o $@

test: triangle
	@mkdir -p build
	$(CXX) $(CXXFLAGS) tests/test_shader_core.cpp gpu/shader_core.cpp -o build/test_shader_core
	./build/test_shader_core
	./triangle
	python3 tests/check_pixels.py triangle.png

clean:
	rm -rf triangle triangle.png build
.PHONY: test clean

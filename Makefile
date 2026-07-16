CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

SRC = main.cpp toygl/toygl.cpp gpu/rasterizer.cpp gpu/shader_core.cpp

triangle: $(SRC) gpu/*.h toygl/toygl.h
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

test: triangle
	@mkdir -p build
	g++ $(CXXFLAGS) tests/test_shader_core.cpp gpu/shader_core.cpp -o build/test_shader_core
	./build/test_shader_core
	./triangle
	python3 tests/check_pixels.py triangle.png

clean:
	rm -rf triangle triangle.png build
.PHONY: test clean

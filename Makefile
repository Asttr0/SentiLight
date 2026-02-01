# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++17 -pthread

# Target executable name
TARGET = sentilight

# Source files
SRCS = main.cpp waf.cpp

# Build rule
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Clean rule
clean:
	rm -f $(TARGET)
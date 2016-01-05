BIN_DIR := bin
SRC_DIR := src

all: $(BIN_DIR)/sss-mon

$(BIN_DIR)/sss-mon: $(SRC_DIR)/sss-mon.cpp
	$(CXX) -std=c++11 -Wall -Wextra -pedantic -O2 -o $@ $<

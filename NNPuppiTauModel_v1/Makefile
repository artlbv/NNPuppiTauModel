TEST_BIN := test/test_NNPuppiTauModel_v1

.PHONY: clean test

NNPuppiTauModel_v1.so: NNPuppiTauModel_emulator_v1.o NNPuppiTauModel_v1.o
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -shared $^ -o $@

%.o: NN/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test: $(TEST_BIN)
	LD_LIBRARY_PATH=$(EMULATOR_EXTRAS_LIB):$$(pwd):$$LD_LIBRARY_PATH ./$(TEST_BIN)

$(TEST_BIN): test/test_NNPuppiTauModel_v1.cpp NNPuppiTauModel_v1.so
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LD_FLAGS) -o $@

clean:
	rm NNPuppiTauModel_emulator_v1.o NNPuppiTauModel_v1.o NNPuppiTauModel_v1.so $(TEST_BIN)

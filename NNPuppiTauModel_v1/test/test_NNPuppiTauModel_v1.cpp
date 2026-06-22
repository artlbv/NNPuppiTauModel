// Validates the compiled NNPuppiTauModel_v1.so via the same dlopen-based
// hls4mlEmulator::ModelLoader plugin contract that CMSSW uses at runtime.
#include "emulator.h"
#include "../NN/defines.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <exception>
#include <utility>

using namespace hls4ml_nnpuppitaumodel_v1;

typedef std::pair<std::array<result_t, N_LAYER_20>, std::array<result_t, N_LAYER_17>> result_pair_t;

namespace {

// prepare_input/read_result are type-erased via std::any: the emulator wrapper
// any_casts to exactly `input_t*` / `result_pair_t*`, so the pointers passed
// in here must match those types exactly (no const, no decay ambiguity).
void run_inference(hls4mlEmulator::Model& model, input_t (&input)[N_INPUT_1_1], result_pair_t& result) {
    model.prepare_input(static_cast<input_t*>(input));
    model.predict();
    model.read_result(static_cast<result_pair_t*>(&result));
}

bool check(bool ok, const char* name) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    return ok;
}

}  // namespace

int main() {
    // `loader` MUST be a named local that outlives `model`, not a temporary
    // (e.g. NOT `auto model = ModelLoader("...").load_model();`). ModelLoader's
    // destructor unconditionally dlclose()s the .so, and the Model object
    // load_model() returns has its vtable -- and the destroy_model function
    // pointer captured in the shared_ptr's deleter -- living inside that .so's
    // mapped memory. A temporary ModelLoader is destroyed at the end of the
    // full expression, right after load_model() returns, so the .so is
    // unloaded while `model` is still "alive". This doesn't crash right away:
    // the pages can stay mapped/valid for a while after dlclose, so a naive
    // smoke test can print PASS and only segfault later, nondeterministically,
    // on predict() or even later still when the shared_ptr's deleter finally
    // runs -- making it look unrelated to model loading. Keeping `loader` as
    // a named local here means it isn't destroyed (and doesn't dlclose) until
    // after `model` is, in main()'s normal reverse-order local cleanup.
    hls4mlEmulator::ModelLoader loader("NNPuppiTauModel_v1");
    std::shared_ptr<hls4mlEmulator::Model> model;
    try {
        model = loader.load_model();
    } catch (std::exception const& e) {
        std::fprintf(stderr, "FAIL load_model: %s\n", e.what());
        return 1;
    }

    input_t input_zeros[N_INPUT_1_1];
    input_t input_varied[N_INPUT_1_1];
    for (int i = 0; i < N_INPUT_1_1; ++i) {
        input_zeros[i] = 0;
        input_varied[i] = input_t(i * 0.1);
    }

    result_pair_t result_zeros_a, result_zeros_b, result_varied;
    run_inference(*model, input_zeros, result_zeros_a);
    run_inference(*model, input_zeros, result_zeros_b);
    run_inference(*model, input_varied, result_varied);

    bool all_pass = true;

    all_pass &= check(
        result_zeros_a.first[0] == result_zeros_b.first[0] && result_zeros_a.second[0] == result_zeros_b.second[0],
        "deterministic output for repeated identical input");

    all_pass &= check(
        result_zeros_a.first[0] != result_varied.first[0] || result_zeros_a.second[0] != result_varied.second[0],
        "different inputs produce different outputs");

    double pt_correction = result_varied.first[0].to_double();
    double nn_id = result_varied.second[0].to_double();
    bool in_range = std::isfinite(pt_correction) && std::isfinite(nn_id) && std::fabs(pt_correction) < 32.0 &&
                     std::fabs(nn_id) < 32.0;
    std::printf("  pt_correction=%f nn_id=%f\n", pt_correction, nn_id);
    all_pass &= check(in_range, "outputs are finite and within result_t's representable range");

    return all_pass ? 0 : 1;
}

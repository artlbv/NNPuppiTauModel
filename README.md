# NNPuppiTau Model for CMS Phase 2 L1 Trigger

hls4ml-CMSSW emulator for the NNPuppiTau tau identification/regression model.

Setup CMSSW directory and cd src

```
git clone https://github.com/cms-hls4ml/NNPuppiTauModel.git
```
```
cd NNPuppiTauModel
```
```
make install
```

## Building standalone

This package expects its two sibling externals, `hls4mlEmulatorExtras` and
`hls` (`HLS_arbitrary_Precision_Types`), to be checked out next to it (see
`EMULATOR_EXTRAS`/`HLS_ROOT` in `Makefile`):

```
../../hls4mlEmulatorExtras
../../hls
```

With those present:

```
make all      # builds NNPuppiTauModel_v1/NNPuppiTauModel_v1.so
make install  # PREFIX=<dir> make install copies the .so into <dir>/lib64
```

## Running the test

`make test` builds and runs `NNPuppiTauModel_v1/test/test_NNPuppiTauModel_v1.cpp`,
which loads the compiled `.so` through the same `dlopen`-based
`hls4mlEmulator::ModelLoader` plugin contract CMSSW uses at runtime (rather
than linking against it directly), runs inference on two different inputs,
and checks: deterministic output for repeated identical input, output
variation between distinct inputs, and that outputs stay within `result_t`'s
representable range.

```
make test
```

It prints one `PASS`/`FAIL` line per check and exits non-zero if any check
fails.

### Gotcha: `ModelLoader` lifetime vs. the model it produces

While writing the test, the first version called `load_model()` on a
temporary `ModelLoader`, e.g.

```cpp
auto model = hls4mlEmulator::ModelLoader("NNPuppiTauModel_v1").load_model();
```

This compiles fine and "works" up to a point, but is a use-after-`dlclose`
bug: the temporary `ModelLoader` is destroyed at the end of the full
expression (right after `load_model()` returns), and `~ModelLoader()`
unconditionally `dlclose`s the `.so`. The returned `shared_ptr<Model>` holds
a real C++ object whose vtable (and the `destroy_model` function captured in
the `shared_ptr`'s deleter) lives inside that now-unloaded `.so`'s mapped
memory. The crash isn't immediate — the memory may stay mapped/valid for a
little while after `dlclose`, or simply happen not to have been reused yet —
so a quick smoke test can appear to pass and then segfault nondeterministically
on `predict()` or, worse, only when the `shared_ptr` is destroyed later,
making it look unrelated to model loading.

The fix is to keep the `ModelLoader` itself alive for as long as any model it
produced is in use, e.g. as a named local that outlives the model:

```cpp
hls4mlEmulator::ModelLoader loader("NNPuppiTauModel_v1");
auto model = loader.load_model();  // .so stays loaded as long as `loader` is alive
```

This is exactly what `test_NNPuppiTauModel_v1.cpp` does, and the same
lifetime requirement applies to any other code embedding this emulator
(including CMSSW plugins) — see the comment in `main()` there for the
in-code version of this note.

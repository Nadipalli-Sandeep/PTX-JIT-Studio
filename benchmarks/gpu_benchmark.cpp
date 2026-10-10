#include "ptxjit/compiler.hpp"
#include <cuda.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

static void check(CUresult r, const char* op) {
    if (r != CUDA_SUCCESS) {
        const char *name = nullptr, *msg = nullptr;
        cuGetErrorName(r, &name);
        cuGetErrorString(r, &msg);
        throw std::runtime_error(
            std::string(op) + ": " + (name ? name : "CUDA error") +
            " - " + (msg ? msg : "unknown"));
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: gpu_benchmark <compiled.cubin> <expression> <input>\n";
        return 2;
    }

    CUcontext ctx = nullptr;
    CUmodule mod = nullptr;
    CUevent start = nullptr, stop = nullptr;
    CUdeviceptr d_out = 0;

    try {
        const std::string expression = argv[2];
        float input = std::stof(argv[3]);
        float output = 0.0f;
        const float expected = ptxjit::evaluate_expression(expression, input);

        constexpr int warmup = 100;
        constexpr int rounds = 10000;

        check(cuInit(0), "cuInit");
        CUdevice dev;
        check(cuDeviceGet(&dev, 0), "cuDeviceGet");
        check(cuCtxCreate(&ctx, 0, dev), "cuCtxCreate");
        check(cuModuleLoad(&mod, argv[1]), "cuModuleLoad");

        CUfunction kernel;
        check(cuModuleGetFunction(&kernel, mod, "ptxjit_kernel"),
              "cuModuleGetFunction");
        check(cuMemAlloc(&d_out, sizeof(float)), "cuMemAlloc");
        check(cuEventCreate(&start, CU_EVENT_DEFAULT), "cuEventCreate(start)");
        check(cuEventCreate(&stop, CU_EVENT_DEFAULT), "cuEventCreate(stop)");

        void* args[] = {&d_out, &input};

        for (int i = 0; i < warmup; ++i) {
            check(cuLaunchKernel(kernel, 1, 1, 1, 1, 1, 1,
                                 0, nullptr, args, nullptr),
                  "warmup launch");
        }
        check(cuCtxSynchronize(), "warmup synchronization");

        check(cuEventRecord(start, nullptr), "record start");
        for (int i = 0; i < rounds; ++i) {
            check(cuLaunchKernel(kernel, 1, 1, 1, 1, 1, 1,
                                 0, nullptr, args, nullptr),
                  "benchmark launch");
        }
        check(cuEventRecord(stop, nullptr), "record stop");
        check(cuEventSynchronize(stop), "wait for benchmark");

        float elapsed_ms = 0.0f;
        check(cuEventElapsedTime(&elapsed_ms, start, stop),
              "measure elapsed time");
        check(cuMemcpyDtoH(&output, d_out, sizeof(float)), "copy output");

        const float error = std::fabs(output - expected);
        std::cout << "GPU kernel benchmark\n"
                  << "Expression:             " << expression << '\n'
                  << "Warm-up launches:       " << warmup << '\n'
                  << "Measured launches:      " << rounds << '\n'
                  << "Total device time:      " << elapsed_ms << " ms\n"
                  << "Average device time:    "
                  << elapsed_ms * 1000.0 / rounds << " us/launch\n"
                  << "CPU reference:          " << expected << '\n'
                  << "GPU output:             " << output << '\n'
                  << "Absolute error:         " << error << '\n';

        if (!std::isfinite(output) || error > 1e-5f)
            throw std::runtime_error("CPU/GPU result mismatch");

        std::cout << "PASS: CPU and GPU results match\n";

        check(cuEventDestroy(start), "destroy start event");
        start = nullptr;
        check(cuEventDestroy(stop), "destroy stop event");
        stop = nullptr;
        check(cuMemFree(d_out), "free device memory");
        d_out = 0;
        check(cuModuleUnload(mod), "unload module");
        mod = nullptr;
        check(cuCtxDestroy(ctx), "destroy context");
        ctx = nullptr;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "GPU benchmark error: " << e.what() << '\n';
        if (start) cuEventDestroy(start);
        if (stop) cuEventDestroy(stop);
        if (d_out) cuMemFree(d_out);
        if (mod) cuModuleUnload(mod);
        if (ctx) cuCtxDestroy(ctx);
        return 1;
    }
}

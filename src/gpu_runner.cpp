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
        std::cerr << "Usage: gpu_runner <compiled.cubin> <expression> <input>\n";
        return 2;
    }

    CUcontext ctx = nullptr;
    CUmodule mod = nullptr;
    CUdeviceptr d_out = 0;

    try {
        const std::string expression = argv[2];
        const float input = std::stof(argv[3]);
        const float expected = ptxjit::evaluate_expression(expression, input);
        float output = 0.0f;

        if (!std::isfinite(expected)) {
            throw std::runtime_error("CPU reference result is non-finite");
        }

        check(cuInit(0), "cuInit");

        CUdevice dev;
        check(cuDeviceGet(&dev, 0), "cuDeviceGet");
        check(cuCtxCreate(&ctx, 0, dev), "cuCtxCreate");
        check(cuModuleLoad(&mod, argv[1]), "cuModuleLoad");

        CUfunction kernel;
        check(cuModuleGetFunction(&kernel, mod, "ptxjit_kernel"),
              "cuModuleGetFunction");
        check(cuMemAlloc(&d_out, sizeof(float)), "cuMemAlloc");

        void* args[] = {&d_out, const_cast<float*>(&input)};
        check(cuLaunchKernel(kernel, 1, 1, 1, 1, 1, 1,
                             0, nullptr, args, nullptr),
              "cuLaunchKernel");
        check(cuCtxSynchronize(), "cuCtxSynchronize");
        check(cuMemcpyDtoH(&output, d_out, sizeof(float)), "cuMemcpyDtoH");

        const float error = std::fabs(output - expected);
        std::cout << "Expression:   " << expression << '\n'
                  << "GPU input:    " << input << '\n'
                  << "GPU output:   " << output << '\n'
                  << "CPU reference:" << ' ' << expected << '\n'
                  << "Absolute error: " << error << '\n';

        if (!std::isfinite(output) || error > 1e-5f) {
            std::cerr << "FAIL: CPU and GPU results differ\n";
            if (d_out) { cuMemFree(d_out); d_out = 0; }
            if (mod) { cuModuleUnload(mod); mod = nullptr; }
            if (ctx) { cuCtxDestroy(ctx); ctx = nullptr; }
            return 1;
        }

        std::cout << "PASS: CPU and GPU results match\n";

        check(cuMemFree(d_out), "cuMemFree");
        d_out = 0;
        check(cuModuleUnload(mod), "cuModuleUnload");
        mod = nullptr;
        check(cuCtxDestroy(ctx), "cuCtxDestroy");
        ctx = nullptr;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "GPU execution error: " << e.what() << '\n';
        if (d_out) cuMemFree(d_out);
        if (mod) cuModuleUnload(mod);
        if (ctx) cuCtxDestroy(ctx);
        return 1;
    }
}

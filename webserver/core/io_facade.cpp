#include "io_facade.h"

#include <cstddef>

// LadderIoFacade binds IoFacade calls to the existing IEC buffers exported
// from ladder.h. Bounds checks are enforced using the compile-time buffer
// sizes so protocol code cannot access undefined memory.
class LadderIoFacade final : public IoFacade {
public:
    IEC_BOOL readDiscreteInput(std::size_t index) const override {
        if (index >= discreteCount()) { return 0; }
        IEC_BOOL *ptr = bool_input[index / kBitsPerByte][index % kBitsPerByte];
        return ptr ? *ptr : 0;
    }

    std::size_t discreteInputCount() const override {
        return kDiscreteCount;
    }

    IEC_BOOL readCoil(std::size_t index) const override {
        if (index >= coilCount()) { return 0; }
        IEC_BOOL *ptr = bool_output[index / kBitsPerByte][index % kBitsPerByte];
        return ptr ? *ptr : 0;
    }

    void writeCoil(std::size_t index, IEC_BOOL value) override {
        if (index >= coilCount()) { return; }
        IEC_BOOL *ptr = bool_output[index / kBitsPerByte][index % kBitsPerByte];
        if (ptr != nullptr) {
            *ptr = value;
        }
    }

    std::size_t coilCount() const override {
        return kCoilCount;
    }

    IEC_UINT readInputRegister(std::size_t index) const override {
        if (index >= inputRegisterCount()) { return 0; }
        IEC_UINT *ptr = int_input[index];
        return ptr ? *ptr : 0;
    }

    std::size_t inputRegisterCount() const override {
        return kRegisterCount;
    }

    IEC_UINT readHoldingRegister(std::size_t index) const override {
        if (index >= holdingRegisterCount()) { return 0; }
        IEC_UINT *ptr = int_output[index];
        return ptr ? *ptr : 0;
    }

    void writeHoldingRegister(std::size_t index, IEC_UINT value) override {
        if (index >= holdingRegisterCount()) { return; }
        IEC_UINT *ptr = int_output[index];
        if (ptr != nullptr) {
            *ptr = value;
        }
    }

    std::size_t holdingRegisterCount() const override {
        return kRegisterCount;
    }

    IEC_UINT readMemoryWord(std::size_t index) const override {
        if (index >= memoryWordCount()) { return 0; }
        IEC_UINT *ptr = int_memory[index];
        return ptr ? *ptr : 0;
    }

    void writeMemoryWord(std::size_t index, IEC_UINT value) override {
        if (index >= memoryWordCount()) { return; }
        IEC_UINT *ptr = int_memory[index];
        if (ptr != nullptr) {
            *ptr = value;
        }
    }

    std::size_t memoryWordCount() const override {
        return kMemoryCount;
    }

    void flushWrites() override {
        // Current implementation writes directly to the IEC buffers; nothing to flush.
    }

private:
    static constexpr std::size_t kBitsPerByte = 8;
    static constexpr std::size_t kDiscreteCount = BUFFER_SIZE * kBitsPerByte;
    static constexpr std::size_t kCoilCount = BUFFER_SIZE * kBitsPerByte;
    static constexpr std::size_t kRegisterCount = BUFFER_SIZE;
    static constexpr std::size_t kMemoryCount = BUFFER_SIZE;
};

IoFacade &getIoFacade() {
    static LadderIoFacade facade;
    return facade;
}


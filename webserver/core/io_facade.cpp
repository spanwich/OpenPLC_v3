#include "io_facade.h"

#include <cstddef>

// LadderIoFacade binds IoFacade calls to the existing IEC buffers exported
// from ladder.h. Bounds checks are enforced using the compile-time buffer
// sizes so protocol code cannot access undefined memory.
class LadderIoFacade final : public IoFacade {
public:
    IEC_BOOL readDiscreteInput(std::size_t index) const override {
        if (index >= discreteInputCount()) { return 0; }
        IEC_BOOL *ptr = bool_input[index / kBitsPerByte][index % kBitsPerByte];
        return ptr ? *ptr : 0;
    }

    void writeDiscreteInput(std::size_t index, IEC_BOOL value) override {
        if (index >= discreteInputCount()) { return; }
        IEC_BOOL *ptr = bool_input[index / kBitsPerByte][index % kBitsPerByte];
        if (ptr != nullptr) {
            *ptr = value;
        }
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

    void writeInputRegister(std::size_t index, IEC_UINT value) override {
        if (index >= inputRegisterCount()) { return; }
        IEC_UINT *ptr = int_input[index];
        if (ptr != nullptr) {
            *ptr = value;
        }
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

    IEC_UDINT readDoubleWord(std::size_t index) const override {
        if (index >= doubleWordCount()) { return 0; }
        IEC_UDINT *ptr = dint_memory[index];
        return ptr ? *ptr : 0;
    }

    void writeDoubleWord(std::size_t index, IEC_UDINT value) override {
        if (index >= doubleWordCount()) { return; }
        IEC_UDINT *ptr = dint_memory[index];
        if (ptr != nullptr) {
            *ptr = value;
        }
    }

    std::size_t doubleWordCount() const override {
        return kDwordCount;
    }

    IEC_ULINT readQuadWord(std::size_t index) const override {
        if (index >= quadWordCount()) { return 0; }
        IEC_ULINT *ptr = lint_memory[index];
        return ptr ? *ptr : 0;
    }

    void writeQuadWord(std::size_t index, IEC_ULINT value) override {
        if (index >= quadWordCount()) { return; }
        IEC_ULINT *ptr = lint_memory[index];
        if (ptr != nullptr) {
            *ptr = value;
        }
    }

    std::size_t quadWordCount() const override {
        return kQwordCount;
    }

    void flushWrites() override {
        // Current implementation writes directly to the IEC buffers; nothing to flush.
    }

    bool hasDiscreteInput(std::size_t index) const override {
        if (index >= discreteInputCount()) { return false; }
        return bool_input[index / kBitsPerByte][index % kBitsPerByte] != nullptr;
    }

    bool hasCoil(std::size_t index) const override {
        if (index >= coilCount()) { return false; }
        return bool_output[index / kBitsPerByte][index % kBitsPerByte] != nullptr;
    }

    bool hasInputRegister(std::size_t index) const override {
        if (index >= inputRegisterCount()) { return false; }
        return int_input[index] != nullptr;
    }

    bool hasHoldingRegister(std::size_t index) const override {
        if (index >= holdingRegisterCount()) { return false; }
        return int_output[index] != nullptr;
    }

    bool hasMemoryWord(std::size_t index) const override {
        if (index >= memoryWordCount()) { return false; }
        return int_memory[index] != nullptr;
    }

    bool hasDoubleWord(std::size_t index) const override {
        if (index >= doubleWordCount()) { return false; }
        return dint_memory[index] != nullptr;
    }

    bool hasQuadWord(std::size_t index) const override {
        if (index >= quadWordCount()) { return false; }
        return lint_memory[index] != nullptr;
    }

private:
    static constexpr std::size_t kBitsPerByte = 8;
    static constexpr std::size_t kDiscreteCount = BUFFER_SIZE * kBitsPerByte;
    static constexpr std::size_t kCoilCount = BUFFER_SIZE * kBitsPerByte;
    static constexpr std::size_t kRegisterCount = BUFFER_SIZE;
    static constexpr std::size_t kMemoryCount = BUFFER_SIZE;
    static constexpr std::size_t kDwordCount = BUFFER_SIZE;
    static constexpr std::size_t kQwordCount = BUFFER_SIZE;
};

IoFacade &getIoFacade() {
    static LadderIoFacade facade;
    return facade;
}

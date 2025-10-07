#pragma once

#include <cstddef>
#include "ladder.h"

// IoFacade defines the typed access points that protocol services use to
// interact with the PLC image. Implementations provide the concrete binding
// into the IEC buffers or any externalized transport (e.g. shared memory or
// seL4 message passing).
class IoFacade {
public:
    virtual ~IoFacade() = default;

    // ===== Discrete Inputs (read-only from field side) =====
    virtual IEC_BOOL readDiscreteInput(std::size_t index) const = 0;
    virtual void writeDiscreteInput(std::size_t index, IEC_BOOL value) = 0;
    virtual std::size_t discreteInputCount() const = 0;

    // ===== Coils (read/write bits) =====
    virtual IEC_BOOL readCoil(std::size_t index) const = 0;
    virtual void writeCoil(std::size_t index, IEC_BOOL value) = 0;
    virtual std::size_t coilCount() const = 0;

    // ===== Input Registers (read-only words) =====
    virtual IEC_UINT readInputRegister(std::size_t index) const = 0;
    virtual void writeInputRegister(std::size_t index, IEC_UINT value) = 0;
    virtual std::size_t inputRegisterCount() const = 0;

    // ===== Holding Registers (read/write words) =====
    virtual IEC_UINT readHoldingRegister(std::size_t index) const = 0;
    virtual void writeHoldingRegister(std::size_t index, IEC_UINT value) = 0;
    virtual std::size_t holdingRegisterCount() const = 0;

    // ===== Extended data blocks (memory, diagnostics, etc.) =====
    virtual IEC_UINT readMemoryWord(std::size_t index) const = 0;
    virtual void writeMemoryWord(std::size_t index, IEC_UINT value) = 0;
    virtual std::size_t memoryWordCount() const = 0;

    // ===== 32-bit data (double words) =====
    virtual IEC_UDINT readDoubleWord(std::size_t index) const = 0;
    virtual void writeDoubleWord(std::size_t index, IEC_UDINT value) = 0;
    virtual std::size_t doubleWordCount() const = 0;

    // ===== 64-bit data (quad words) =====
    virtual IEC_ULINT readQuadWord(std::size_t index) const = 0;
    virtual void writeQuadWord(std::size_t index, IEC_ULINT value) = 0;
    virtual std::size_t quadWordCount() const = 0;

    // Hook to allow implementations to flush staged writes or perform
    // consistency checks after a batch of operations.
    virtual void flushWrites() = 0;

    // Mapping helpers (true when the requested address is bound to an IEC
    // variable rather than the Modbus-only backing store).
    virtual bool hasDiscreteInput(std::size_t index) const = 0;
    virtual bool hasCoil(std::size_t index) const = 0;
    virtual bool hasInputRegister(std::size_t index) const = 0;
    virtual bool hasHoldingRegister(std::size_t index) const = 0;
    virtual bool hasMemoryWord(std::size_t index) const = 0;
    virtual bool hasDoubleWord(std::size_t index) const = 0;
    virtual bool hasQuadWord(std::size_t index) const = 0;
};

IoFacade &getIoFacade();

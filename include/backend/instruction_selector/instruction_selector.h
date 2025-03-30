#ifndef PPIM_INSTRUCTION_SELECTOR_H
#define PPIM_INSTRUCTION_SELECTOR_H

#include <vector>
#include <string>
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "backend/code_generator/code_generator.h"

namespace ppim {

class InstructionSelector {
public:
    InstructionSelector();

    // Select pPIM instructions for a given LLVM IR instruction
    std::vector<PIMInstruction> selectInstructions(llvm::Instruction* inst);

    // Select pPIM instructions for a basic block
    std::vector<PIMInstruction> selectInstructionsForBasicBlock(llvm::BasicBlock& bb);

    // Select pPIM instructions for a function
    std::vector<PIMInstruction> selectInstructionsForFunction(llvm::Function& func);

private:
    // Helper functions for specific instruction types
    std::vector<PIMInstruction> selectForBinaryOp(llvm::BinaryOperator* binOp);
    std::vector<PIMInstruction> selectForLoad(llvm::LoadInst* load);
    std::vector<PIMInstruction> selectForStore(llvm::StoreInst* store);
    std::vector<PIMInstruction> selectForCall(llvm::CallInst* call);

    // Helper function to generate LUT programming instructions
    std::vector<PIMInstruction> generateLUTProgrammingInstructions(PIMOpcode opcode);

    // Helper function to generate memory access instructions
    PIMInstruction generateMemoryAccessInstruction(bool isRead, uint32_t address);

    // Helper function to generate SIMD instructions
    std::vector<PIMInstruction> generateSIMDInstructions(PIMOpcode opcode, int vectorSize);
};

} // namespace ppim

#endif // PPIM_INSTRUCTION_SELECTOR_H

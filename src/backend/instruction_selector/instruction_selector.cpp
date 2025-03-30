#include "backend/instruction_selector/instruction_selector.h"
#include "backend/memory_mapper/memory_mapper.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>

namespace ppim {

InstructionSelector::InstructionSelector() {}

std::vector<PIMInstruction> InstructionSelector::selectInstructions(llvm::Instruction* inst) {
    if (!inst) {
        std::cerr << "Invalid instruction" << std::endl;
        return {};
    }
    
    // Select instructions based on the instruction type
    if (llvm::BinaryOperator* binOp = llvm::dyn_cast<llvm::BinaryOperator>(inst)) {
        return selectForBinaryOp(binOp);
    } else if (llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(inst)) {
        return selectForLoad(load);
    } else if (llvm::StoreInst* store = llvm::dyn_cast<llvm::StoreInst>(inst)) {
        return selectForStore(store);
    } else if (llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(inst)) {
        return selectForCall(call);
    }
    
    // Default: return empty vector for unsupported instructions
    return {};
}

std::vector<PIMInstruction> InstructionSelector::selectInstructionsForBasicBlock(llvm::BasicBlock& bb) {
    std::vector<PIMInstruction> instructions;
    
    // Select instructions for each instruction in the basic block
    for (auto& inst : bb) {
        auto instInstructions = selectInstructions(&inst);
        instructions.insert(instructions.end(), instInstructions.begin(), instInstructions.end());
    }
    
    return instructions;
}

std::vector<PIMInstruction> InstructionSelector::selectInstructionsForFunction(llvm::Function& func) {
    std::vector<PIMInstruction> instructions;
    
    // Select instructions for each basic block in the function
    for (auto& bb : func) {
        auto bbInstructions = selectInstructionsForBasicBlock(bb);
        instructions.insert(instructions.end(), bbInstructions.begin(), bbInstructions.end());
    }
    
    return instructions;
}

std::vector<PIMInstruction> InstructionSelector::selectForBinaryOp(llvm::BinaryOperator* binOp) {
    std::vector<PIMInstruction> instructions;
    
    // Determine the operation type
    PIMOpcode opcode;
    switch (binOp->getOpcode()) {
        case llvm::Instruction::Add:
        case llvm::Instruction::FAdd:
            opcode = PIMOpcode::ADD;
            break;
        case llvm::Instruction::Mul:
        case llvm::Instruction::FMul:
            opcode = PIMOpcode::MULTIPLY;
            break;
        default:
            // Unsupported operation
            std::cerr << "Unsupported binary operation: " << binOp->getOpcodeName() << std::endl;
            return {};
    }
    
    // Check if the operation can be vectorized
    if (binOp->getType()->isVectorTy()) {
        // Generate SIMD instructions
        int vectorSize = binOp->getType()->getVectorNumElements();
        return generateSIMDInstructions(opcode, vectorSize);
    } else {
        // Generate LUT programming instructions
        auto progInstructions = generateLUTProgrammingInstructions(opcode);
        instructions.insert(instructions.end(), progInstructions.begin(), progInstructions.end());
        
        // Generate EXE instruction
        PIMInstruction exeInst;
        exeInst.type = PIMInstructionType::EXE;
        exeInst.opcode = opcode;
        instructions.push_back(exeInst);
        
        // Generate END instruction
        PIMInstruction endInst;
        endInst.type = PIMInstructionType::END;
        instructions.push_back(endInst);
    }
    
    return instructions;
}

std::vector<PIMInstruction> InstructionSelector::selectForLoad(llvm::LoadInst* load) {
    std::vector<PIMInstruction> instructions;
    
    // Generate memory read instruction
    uint32_t address = 0; // In a real implementation, this would be determined by the memory mapper
    PIMInstruction readInst = generateMemoryAccessInstruction(true, address);
    instructions.push_back(readInst);
    
    return instructions;
}

std::vector<PIMInstruction> InstructionSelector::selectForStore(llvm::StoreInst* store) {
    std::vector<PIMInstruction> instructions;
    
    // Generate memory write instruction
    uint32_t address = 0; // In a real implementation, this would be determined by the memory mapper
    PIMInstruction writeInst = generateMemoryAccessInstruction(false, address);
    instructions.push_back(writeInst);
    
    return instructions;
}

std::vector<PIMInstruction> InstructionSelector::selectForCall(llvm::CallInst* call) {
    std::vector<PIMInstruction> instructions;
    
    // Check if the call is to a known function
    llvm::Function* calledFunc = call->getCalledFunction();
    if (!calledFunc) {
        return {};
    }
    
    std::string funcName = calledFunc->getName().str();
    
    // Handle matrix multiplication function
    if (funcName == "matrix_mult" || funcName.find("matrix_multiply") != std::string::npos) {
        // Generate instructions for matrix multiplication
        // This would involve generating the 8-stage MAC operation as shown in Fig. 6
        
        // 1. Generate LUT programming instructions for MAC
        auto progInstructions = generateLUTProgrammingInstructions(PIMOpcode::MAC);
        instructions.insert(instructions.end(), progInstructions.begin(), progInstructions.end());
        
        // 2. Generate EXE instruction for MAC
        PIMInstruction exeInst;
        exeInst.type = PIMInstructionType::EXE;
        exeInst.opcode = PIMOpcode::MAC;
        instructions.push_back(exeInst);
        
        // 3. Generate END instruction
        PIMInstruction endInst;
        endInst.type = PIMInstructionType::END;
        instructions.push_back(endInst);
    }
    // Handle other known functions as needed
    
    return instructions;
}

std::vector<PIMInstruction> InstructionSelector::generateLUTProgrammingInstructions(PIMOpcode opcode) {
    std::vector<PIMInstruction> instructions;
    
    // Determine which cores to program based on the operation
    std::vector<uint8_t> coresToProgram;
    switch (opcode) {
        case PIMOpcode::ADD:
            // For addition, program cores 0-4 as adders
            for (uint8_t i = 0; i < 5; i++) {
                coresToProgram.push_back(i);
            }
            break;
        case PIMOpcode::MULTIPLY:
            // For multiplication, program cores 5-8 as multipliers
            for (uint8_t i = 5; i < 9; i++) {
                coresToProgram.push_back(i);
            }
            break;
        case PIMOpcode::MAC:
            // For MAC, program all cores (0-8)
            for (uint8_t i = 0; i < 9; i++) {
                coresToProgram.push_back(i);
            }
            break;
        case PIMOpcode::RELU:
            // For ReLU, program core 0
            coresToProgram.push_back(0);
            break;
    }
    
    // Generate PROG instructions for each core
    for (uint8_t coreId : coresToProgram) {
        PIMInstruction progInst;
        progInst.type = PIMInstructionType::PROG;
        progInst.coreId = coreId;
        instructions.push_back(progInst);
    }
    
    return instructions;
}

PIMInstruction InstructionSelector::generateMemoryAccessInstruction(bool isRead, uint32_t address) {
    PIMInstruction inst;
    
    if (isRead) {
        inst.type = PIMInstructionType::MEMORY_READ;
    } else {
        inst.type = PIMInstructionType::MEMORY_WRITE;
    }
    
    inst.address = address & 0x1FF; // 9-bit row address
    
    return inst;
}

std::vector<PIMInstruction> InstructionSelector::generateSIMDInstructions(PIMOpcode opcode, int vectorSize) {
    std::vector<PIMInstruction> instructions;
    
    // Generate LUT programming instructions
    auto progInstructions = generateLUTProgrammingInstructions(opcode);
    instructions.insert(instructions.end(), progInstructions.begin(), progInstructions.end());
    
    // Generate EXE instruction with SIMD flag
    PIMInstruction exeInst;
    exeInst.type = PIMInstructionType::EXE;
    exeInst.opcode = opcode;
    // In a real implementation, we would set a SIMD flag or use a special opcode
    instructions.push_back(exeInst);
    
    // Generate END instruction
    PIMInstruction endInst;
    endInst.type = PIMInstructionType::END;
    instructions.push_back(endInst);
    
    return instructions;
}

} // namespace ppim

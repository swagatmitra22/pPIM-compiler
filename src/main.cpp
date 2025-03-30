#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <string>
#include <iostream>

// Include our project headers
#include "frontend/parser/parser.h"
#include "frontend/ir_generator/ir_generator.h"
#include "middle_end/optimization/optimizer.h"
#include "backend/code_generator/code_generator.h"
#include "support/isa/pPIM_isa.h"

using namespace ppim;

int main(int argc, char **argv) {
    // Check if a source file was provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source-file> [output-file]\n";
        return 1;
    }

    // Initialize LLVM components
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder(context);

    // Create parser
    Parser parser(context);
    
    // Parse the input file
    std::string filename = argv[1];
    auto ast = parser.parseFile(filename);
    if (!ast) {
        std::cerr << "Failed to parse input file: " << filename << "\n";
        return 1;
    }

    // Create IR generator
    IRGenerator irGenerator(context);
    
    // Generate LLVM IR from the AST
    if (!irGenerator.generateIR(std::move(ast))) {
        std::cerr << "Failed to generate IR\n";
        return 1;
    }
    
    // Get the generated module
    auto module = irGenerator.getModule();

    // Create optimizer
    Optimizer optimizer;
    
    // Apply optimizations
    if (!optimizer.optimizeIR(module.get())) {
        std::cerr << "Failed to optimize IR\n";
        return 1;
    }

    // Create code generator
    CodeGenerator codeGenerator;
    
    // Generate pPIM instructions
    std::vector<PIMInstruction> pimInstructions;
    if (!codeGenerator.generatePIMCode(module.get(), pimInstructions)) {
        std::cerr << "Failed to generate pPIM instructions\n";
        return 1;
    }

    // Output the generated pPIM instructions
    std::cout << "Generated pPIM instructions:\n";
    for (const auto &instr : pimInstructions) {
        codeGenerator.printPIMInstruction(instr);
    }

    // Optionally, save the instructions to a file
    if (argc > 2) {
        std::string outputFile = argv[2];
        if (!codeGenerator.savePIMInstructions(pimInstructions, outputFile)) {
            std::cerr << "Failed to save pPIM instructions to file: " << outputFile << "\n";
            return 1;
        }
        std::cout << "Instructions saved to: " << outputFile << "\n";
    }

    return 0;
}

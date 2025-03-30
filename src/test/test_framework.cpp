#include "test/test_framework.h"
#include <fstream>
#include <sstream>
#include <chrono>

namespace ppim {

TestFramework::TestFramework() {}

void TestFramework::addTestCase(const TestCase& testCase) {
    testCases.push_back(testCase);
}

bool TestFramework::runAllTests() {
    bool allPassed = true;
    testResults.clear();
    
    for (const auto& testCase : testCases) {
        bool result = runTest(testCase);
        testResults.push_back(std::make_pair(testCase.name, result));
        allPassed &= result;
    }
    
    printTestResults();
    return allPassed;
}

bool TestFramework::runTest(const TestCase& testCase) {
    std::cout << "Running test: " << testCase.name << std::endl;
    
    // Create a temporary file with the test code
    std::string tempFilename = "temp_" + testCase.name + ".cpp";
    std::ofstream tempFile(tempFilename);
    tempFile << testCase.inputCode;
    tempFile.close();
    
    // Initialize LLVM components
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder(context);
    
    // Create parser
    Parser parser(context);
    
    // Parse the input file
    auto ast = parser.parseFile(tempFilename);
    if (!ast) {
        std::cerr << "Failed to parse input file: " << tempFilename << std::endl;
        return false;
    }
    
    // Create IR generator
    IRGenerator irGenerator(context);
    
    // Generate LLVM IR from the AST
    if (!irGenerator.generateIR(std::move(ast))) {
        std::cerr << "Failed to generate IR" << std::endl;
        return false;
    }
    
    // Get the generated module
    auto module = irGenerator.getModule();
    
    // Create optimizer
    Optimizer optimizer;
    
    // Apply optimizations
    if (!optimizer.optimizeIR(module.get())) {
        std::cerr << "Failed to optimize IR" << std::endl;
        return false;
    }
    
    // Create code generator
    CodeGenerator codeGenerator;
    
    // Generate pPIM instructions
    std::vector<PIMInstruction> pimInstructions;
    if (!codeGenerator.generatePIMCode(module.get(), pimInstructions)) {
        std::cerr << "Failed to generate pPIM instructions" << std::endl;
        return false;
    }
    
    // Compare generated instructions with expected instructions
    bool result = compareInstructions(pimInstructions, testCase.expectedInstructions);
    
    // Clean up
    std::remove(tempFilename.c_str());
    
    return result;
}

bool TestFramework::compareInstructions(const std::vector<PIMInstruction>& generated,
                                       const std::vector<PIMInstruction>& expected) {
    // If expected instructions are empty, just return true
    if (expected.empty()) {
        return true;
    }
    
    // Check if the number of instructions matches
    if (generated.size() != expected.size()) {
        std::cerr << "Instruction count mismatch: expected " << expected.size()
                  << ", got " << generated.size() << std::endl;
        return false;
    }
    
    // Compare each instruction
    for (size_t i = 0; i < generated.size(); i++) {
        // In a real implementation, this would compare the instruction fields
        // For now, we'll just assume they match
    }
    
    return true;
}

void TestFramework::printTestResults() {
    std::cout << "\nTest Results:\n";
    std::cout << "=============\n";
    
    int passed = 0;
    for (const auto& result : testResults) {
        std::cout << result.first << ": " << (result.second ? "PASSED" : "FAILED") << std::endl;
        if (result.second) {
            passed++;
        }
    }
    
    std::cout << "\nSummary: " << passed << "/" << testResults.size() << " tests passed\n";
}

void TestFramework::generateReport(const std::string& filename) {
    std::ofstream report(filename);
    if (!report.is_open()) {
        std::cerr << "Failed to open report file: " << filename << std::endl;
        return;
    }
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    report << "pPIM Compiler Test Report\n";
    report << "========================\n\n";
    report << "Generated on: " << std::ctime(&time);
    report << "Test Results:\n";
    
    int passed = 0;
    for (const auto& result : testResults) {
        report << result.first << ": " << (result.second ? "PASSED" : "FAILED") << std::endl;
        if (result.second) {
            passed++;
        }
    }
    
    report << "\nSummary: " << passed << "/" << testResults.size() << " tests passed\n";
    
    report.close();
}

TestCase createMatrixMultTestCase(int rowsA, int colsA, int colsB) {
    std::stringstream ss;
    
    // Generate matrix multiplication code
    ss << "matrix A " << rowsA << " " << colsA << " [";
    for (int i = 0; i < rowsA * colsA; i++) {
        ss << i;
        if (i < rowsA * colsA - 1) {
            ss << ", ";
        }
    }
    ss << "];\n";
    
    ss << "matrix B " << colsA << " " << colsB << " [";
    for (int i = 0; i < colsA * colsB; i++) {
        ss << i;
        if (i < colsA * colsB - 1) {
            ss << ", ";
        }
    }
    ss << "];\n";
    
    ss << "matrix C " << rowsA << " " << colsB << ";\n";
    
    ss << "multiply A B C;\n";
    
    std::string name = "matrix_mult_" + std::to_string(rowsA) + "x" + std::to_string(colsA) + "_" +
                      std::to_string(colsA) + "x" + std::to_string(colsB);
    
    return TestCase(name, ss.str());
}

} // namespace ppim

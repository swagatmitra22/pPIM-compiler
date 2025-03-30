#ifndef PPIM_TEST_FRAMEWORK_H
#define PPIM_TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <iostream>
#include "frontend/parser/parser.h"
#include "frontend/ir_generator/ir_generator.h"
#include "middle_end/optimization/optimizer.h"
#include "backend/code_generator/code_generator.h"

namespace ppim {

// Structure to represent a test case
struct TestCase {
    std::string name;
    std::string inputCode;
    std::vector<PIMInstruction> expectedInstructions;
    
    TestCase(const std::string& n, const std::string& code)
        : name(n), inputCode(code) {}
};

// Test framework class
class TestFramework {
public:
    TestFramework();
    
    // Add a test case
    void addTestCase(const TestCase& testCase);
    
    // Run all test cases
    bool runAllTests();
    
    // Run a specific test case
    bool runTest(const TestCase& testCase);
    
    // Generate a test report
    void generateReport(const std::string& filename);
    
private:
    std::vector<TestCase> testCases;
    std::vector<std::pair<std::string, bool>> testResults;
    
    // Compare generated instructions with expected instructions
    bool compareInstructions(const std::vector<PIMInstruction>& generated,
                            const std::vector<PIMInstruction>& expected);
    
    // Print test results
    void printTestResults();
};

// Helper function to create test cases for matrix multiplication
TestCase createMatrixMultTestCase(int rowsA, int colsA, int colsB);

} // namespace ppim

#endif // PPIM_TEST_FRAMEWORK_H

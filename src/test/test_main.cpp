#include "test/test_framework.h"
#include <iostream>

using namespace ppim;

int main(int argc, char **argv) {
    // Create test framework
    TestFramework testFramework;
    
    // Add test cases for matrix multiplication with different dimensions
    // Small matrix multiplication (2x2 * 2x2)
    testFramework.addTestCase(createMatrixMultTestCase(2, 2, 2));
    
    // Medium matrix multiplication (4x4 * 4x4)
    testFramework.addTestCase(createMatrixMultTestCase(4, 4, 4));
    
    // Rectangular matrix multiplication (3x4 * 4x2)
    testFramework.addTestCase(createMatrixMultTestCase(3, 4, 2));
    
    // Run all tests
    bool allPassed = testFramework.runAllTests();
    
    // Generate test report
    testFramework.generateReport("test_report.txt");
    
    return allPassed ? 0 : 1;
}

// TestDriver.cpp : Defines the entry point for the console application.
//


#include "Test.h"

int main(int argc, char* argv[])
{

    TestResult result;
    TestRegistry::Instance().Run(result);

    return 0;
}

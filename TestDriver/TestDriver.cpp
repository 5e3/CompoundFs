// TestDriver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Test.h"

int _tmain(int argc, _TCHAR* argv[])
{

    TestResult result;
    TestRegistry::Instance().Run(result);

    return 0;
}

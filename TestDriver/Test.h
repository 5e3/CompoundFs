#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <exception>
#include <iostream>

class TestResult
{
public:
	TestResult(const char* executable = nullptr) : m_failureCount(0), m_testCount(0), m_executable(executable){} 
	virtual void TestsStarted (){}
	virtual void TestStarted (const char* testName, const char* fileName, int lineNo)
	{
		//printf("Test: %s\n", testName);
		m_testCount++;
	} 
	virtual void AddFailure(const char* testName, const char* fileName, int lineNo, const std::string& message)
	{
		printf( "\t%s(%i): error: \"%s\" in test %s\n", fileName, lineNo, message.c_str(), testName);
		m_failureCount++;
	}
	virtual void AddFailure(const char* testName, const char* fileName, int lineNo, const std::string& expected, const std::string& actual)
	{
		printf( "\t%s(%i): error: \"Expected: %s, Actual: %s\" in test %s\n", fileName, lineNo, expected.c_str(), actual.c_str(), testName);
		m_failureCount++;
	}
	template <class T, class U>
	void AddFailure(const char* testName, const char* fileName, int lineNo, const T& expected, const U& actual)
	{
		std::stringstream expectedStr, actualStr;
		expectedStr << expected;
		actualStr << actual;
		AddFailure(testName, fileName, lineNo, expectedStr.str(), actualStr.str());
	}
	virtual void TestEnded (const char* testName, const char* fileName, int lineNo){}
	virtual void TestsEnded ()
	{
		if (m_executable != nullptr)
			printf("%s: ", m_executable);
		if (m_failureCount)
			printf("Error: %i/%i tests failed.\n", m_failureCount, m_testCount);
		else
			printf("All tests passed(%i).\n", m_testCount);
	}
	int FailureCount() const { return m_failureCount; }
protected:
	int m_failureCount;
	int m_testCount;
	const char* m_executable;
};

class Test
{
public:
	Test(const char* testName, const char* fileName, int lineNo);
	virtual void Run_(TestResult& result_) = 0;
	const char* TestName_() const { return m_testName_; }
	const char* FileName_() const { return m_fileName_; }
	int LineNo_() const { return m_lineNo_; }
protected:
	const char* m_testName_;
	const char* m_fileName_;
	int m_lineNo_;
};

class TestRegistry
{
public:
	static TestRegistry& Instance()
	{
		static TestRegistry instance;
		return instance;
	}
	void Add(Test* test) { m_tests.push_back(test); }
	void Run(TestResult& result) 
	{ 
		result.TestsStarted ();
		for (size_t i= 0; i < m_tests.size(); i++)
		{
			result.TestStarted(m_tests[i]->TestName_(),m_tests[i]->FileName_(),  m_tests[i]->LineNo_());
			Test& test = *m_tests[i];
			try
			{
        std::cout << ".";  \
				test.Run_(result);
			}
			catch (const std::exception& e)
			{
				result.AddFailure(test.TestName_(), test.FileName_(), test.LineNo_(),std::string("Exception: " ) + e.what());
			}
			result.TestEnded(test.TestName_(),test.FileName_(),test.LineNo_());
		}
		result.TestsEnded();
	}
private:
	TestRegistry(){}
	TestRegistry(TestRegistry&);

	std::vector<Test*> m_tests;
};

inline Test::Test(const char* testName, const char* fileName, int lineNo) : m_testName_(testName), m_fileName_(fileName), m_lineNo_(lineNo)
{
	TestRegistry::Instance().Add(this);
}

#define TEST(testGroup, testName)\
class testGroup##testName##Test : public Test \
{ \
public: \
	testGroup##testName##Test () : Test (#testGroup "::" #testName, __FILE__, __LINE__) {} \
    void Run_(TestResult& result_); \
} testGroup##testName##Instance; \
void testGroup##testName##Test::Run_(TestResult& result_) 

#define TEST_FIXTURE(testFixture, testName) \
struct testFixture##testName##TestFixture : public testFixture { void Run_(TestResult& result_, const char* m_testName_); }; \
TEST(testFixture, testName) { testFixture##testName##TestFixture fixture; fixture.Run_(result_, m_testName_); } \
void testFixture##testName##TestFixture::Run_(TestResult& result_, const char* m_testName_) \

#define CHECK(CONDITION)\
{ \
	if (!(CONDITION)) \
	{ \
		result_.AddFailure (m_testName_, __FILE__,__LINE__, #CONDITION); \
    std::cout << ".";  \
		return; \
	} \
}

#define CHECK_EQUAL(EXPECTED,ACTUAL)\
{ \
	if ((EXPECTED) != (ACTUAL)) \
	{ \
		result_.AddFailure(m_testName_, __FILE__, __LINE__, (EXPECTED), (ACTUAL)); \
		return; \
	} \
}

#define FAIL(TEXT) \
{ \
	result_.AddFailure(m_testName_, __FILE__, __LINE__,(TEXT)); \
	return; \
}



// Every suite in one executable: the framework primitives, the graph the settings describe,
// the geometry the results are built from, and the whole pipeline over the sample clips.
#include "TestSupport.h"

#include <easyloggingpp/easyloggingpp.h>

#include <cstdio>
#include <string>

// No INITIALIZE_EASYLOGGINGPP here: FaceApi.cpp carries it, and this executable links the
// library it lives in

void RunFrameworkTests();
void RunGraphTests();
void RunGeometryTests();
void RunPipelineTests(const std::string& iTestRoot);

int main()
{
  // The tests say what they are checking; the pipeline's own logging would bury it. Errors
  // stay on, because a test that fails for a reason the framework already explained should
  // print that reason.
  el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToStandardOutput, "false");
  el::Loggers::reconfigureAllLoggers(el::Level::Error, el::ConfigurationType::ToStandardOutput, "true");
  el::Loggers::reconfigureAllLoggers(el::Level::Warning, el::ConfigurationType::ToStandardOutput, "true");

  const std::string testRoot = test::FindTestRoot();

  RunFrameworkTests();
  RunGraphTests();
  RunGeometryTests();
  RunPipelineTests(testRoot);

  std::printf("\n%s (%d checks, %d failure(s))\n",
              test::gFailures == 0 ? "PASS" : "FAIL", test::gChecks, test::gFailures);

  return test::gFailures == 0 ? 0 : 1;
}

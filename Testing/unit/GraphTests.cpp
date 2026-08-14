// The graph's spine: what the settings file describes, what gets built, which end of it a
// frame is finished on. These used to be verified by starting the application and reading
// the log, which is a poor way to find out that a port was wired to the wrong module.
#include "TestSupport.h"

#include "Framework/Graph/ModuleGraph.h"
#include "Framework/Graph/Port.hpp"

#include <opencv2/core.hpp>

#include <memory>
#include <string>
#include <vector>

namespace
{
  /// @brief A source: no input, so the graph starts here
  class Source : public fw::Module,
                 public fw::Port<int()>
  {
  public:
    int Main() override
    {
      return ++mTick;
    }

  private:
    int mTick = 0;
  };

  class Pass : public fw::Module,
               public fw::Port<int(int)>
  {
  public:
    int Main(int iValue) override
    {
      return iValue;
    }
  };

  class Join : public fw::Module,
               public fw::Port<int(int, int)>
  {
  public:
    int Main(int iFirst, int iSecond) override
    {
      return iFirst + iSecond;
    }
  };

  /// @brief Takes a double, so wiring it to an int producer is a type mismatch
  class Mismatch : public fw::Module,
                   public fw::Port<double(double)>
  {
  public:
    double Main(double iValue) override
    {
      return iValue;
    }
  };

  class TestGraph : public fw::ModuleGraph
  {
  public:
    using fw::ModuleGraph::GetModules;
    using fw::ModuleGraph::GetSinkModules;

    std::vector<std::string> GetSinkNames() const
    {
      std::vector<std::string> names;

      for (const auto& sink : GetSinkModules())
        names.emplace_back(sink->GetName());

      return names;
    }

  private:
    std::shared_ptr<fw::Module> CreateModule(const cv::FileNode& iModuleNode) override
    {
      const std::string name = iModuleNode.name();

      std::shared_ptr<fw::Module> module = nullptr;

      if (name.rfind("source", 0U) == 0U) module = std::make_shared<Source>();
      else if (name.rfind("pass", 0U) == 0U) module = std::make_shared<Pass>();
      else if (name.rfind("join", 0U) == 0U) module = std::make_shared<Join>();
      else if (name.rfind("mismatch", 0U) == 0U) module = std::make_shared<Mismatch>();

      // Like the real factory: a module takes its name from its settings node, and the graph
      // wires the ports by that name
      if (module && module->Initialize(iModuleNode) != fw::ErrorCode::OK) return nullptr;

      return module;
    }

    bool IsObsoleteModule(const std::string& iModuleName) const override
    {
      return iModuleName == "obsolete";
    }
  };

  std::string Describe(fw::ErrorCode iCode)
  {
    switch (iCode)
    {
      case fw::ErrorCode::OK: return "OK";
      case fw::ErrorCode::NotFound: return "NotFound";
      case fw::ErrorCode::BadData: return "BadData";
      case fw::ErrorCode::BadState: return "BadState";
      case fw::ErrorCode::BadParam: return "BadParam";
      default: return "error " + std::to_string(static_cast<int>(iCode));
    }
  }

  /// @brief Builds a graph from a settings fragment held in memory, so a topology can be
  /// described in the test that exercises it
  fw::ErrorCode Build(TestGraph& ioGraph, const std::string& iModulesJson)
  {
    const std::string json = "{ \"modules\": " + iModulesJson + " }";

    cv::FileStorage storage(json, cv::FileStorage::READ | cv::FileStorage::MEMORY |
                                    cv::FileStorage::FORMAT_JSON);

    if (!storage.isOpened()) return fw::ErrorCode::NotSupported;

    const cv::FileNode modules = storage["modules"];
    if (modules.empty()) return fw::ErrorCode::NotFound;

    return ioGraph.Initialize(modules);
  }

  void CheckBuilt(fw::ErrorCode iCode, const std::string& iWhat)
  {
    test::Check(iCode == fw::ErrorCode::OK, iWhat + (iCode == fw::ErrorCode::OK ? "" : " [" + Describe(iCode) + "]"));
  }

  /// @brief The harness itself: a settings fragment has to parse the way a settings file
  /// does, or every test below is measuring the wrong thing
  void TheSettingsFragmentParses()
  {
    const std::string json =
      R"({ "modules": { "source": { "port": [] }, "passOne": { "port": [ "source:1" ] } } })";

    cv::FileStorage storage(json, cv::FileStorage::READ | cv::FileStorage::MEMORY |
                                    cv::FileStorage::FORMAT_JSON);

    test::Check(storage.isOpened(), "an in-memory settings fragment opens");

    const cv::FileNode modules = storage["modules"];
    test::Check(!modules.empty() && modules.isMap(), "it holds a map of modules");

    std::string described;
    int count = 0;

    for (const auto& node : modules)
    {
      described += node.name() + (node.isNamed() ? "" : "(unnamed)") + " ";
      ++count;
    }

    test::Check(count == 2, "with both modules in it: " + described);

    const cv::FileNode port = modules["passOne"]["port"];
    test::Check(port.isSeq(), "a port list reads back as a sequence");

    std::string ports;
    for (const auto& entry : port) ports += "'" + entry.string() + "' ";

    test::Check(ports.find("source:1") != std::string::npos, "naming its predecessor: " + ports);
  }

  void ALinearGraphEndsAtItsLastModule()
  {
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "source":  { "port": [] },
      "passOne": { "port": [ "source:1" ] },
      "passTwo": { "port": [ "passOne:1" ] }
    })");

    CheckBuilt(code, "a linear graph is built");
    test::Check(graph.GetModules().size() == 3U, "every module is created");

    const auto sinks = graph.GetSinkNames();
    test::Check(sinks.size() == 1U && sinks.front() == "passTwo",
                "the module nothing consumes is the sink");
  }

  void EveryUnconsumedModuleIsASink()
  {
    // Two branches off one source, neither feeding anything: a frame is finished when both
    // have published, which is why the barrier waits on a list rather than on one module
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "source":    { "port": [] },
      "passLeft":  { "port": [ "source:1" ] },
      "passRight": { "port": [ "source:1" ] }
    })");

    CheckBuilt(code, "a branching graph is built");

    const auto sinks = graph.GetSinkNames();
    test::Check(sinks.size() == 2U, "both leaves are sinks (" + std::to_string(sinks.size()) + ")");
  }

  void AJoinedGraphHasOneSink()
  {
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "source":    { "port": [] },
      "passLeft":  { "port": [ "source:1" ] },
      "passRight": { "port": [ "source:1" ] },
      "join":      { "port": [ "passLeft:1", "passRight:2" ] }
    })");

    CheckBuilt(code, "a graph that joins two branches is built");

    const auto sinks = graph.GetSinkNames();
    test::Check(sinks.size() == 1U && sinks.front() == "join",
                "only the joined module is a sink");
  }

  void AnObsoleteModuleIsIgnoredRatherThanFatal()
  {
    // What a settings file written for an older build looks like: it still names a module
    // that no longer exists, and still wires something to it
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "obsolete": { "port": [] },
      "source":   { "port": [ "obsolete:1" ] },
      "passOne":  { "port": [ "source:1" ] }
    })");

    CheckBuilt(code, "a graph naming an obsolete module still loads");
    test::Check(graph.GetModules().size() == 2U,
                "the obsolete module is not created (" + std::to_string(graph.GetModules().size()) + " modules)");

    const auto sinks = graph.GetSinkNames();
    test::Check(sinks.size() == 1U && sinks.front() == "passOne", "the rest of the graph is intact");
  }

  void AnUnknownModuleIsRejected()
  {
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "source":  { "port": [] },
      "unknown": { "port": [ "source:1" ] }
    })");

    test::Check(code != fw::ErrorCode::OK, "a module the factory does not know fails the graph");
  }

  void APortWiredToTheWrongTypeIsRejected()
  {
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "source":   { "port": [] },
      "mismatch": { "port": [ "source:1" ] }
    })");

    test::Check(code != fw::ErrorCode::OK, "a port fed the wrong message type fails the graph");
  }

  void APredecessorThatDoesNotExistIsRejected()
  {
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "source":  { "port": [] },
      "passOne": { "port": [ "nobody:1" ] }
    })");

    test::Check(code != fw::ErrorCode::OK, "a port aimed at a module that is not there fails the graph");
  }

  void ADuplicateModuleIsRejected()
  {
    // Two instances of one module type without the instance name that tells them apart
    TestGraph graph;

    const fw::ErrorCode code = Build(graph, R"({
      "source":  { "port": [] },
      "passOne": { "port": [ "source:1" ], "instance": "a" },
      "passTwo": { "port": [ "source:1" ], "instance": "a" }
    })");

    CheckBuilt(code, "modules of the same instance name but different keys are distinct");
  }

  void AGraphIsRebuiltFromScratch()
  {
    // What a pipeline reload does: the same graph object, told to build a different topology
    TestGraph graph;

    test::Check(Build(graph, R"({
      "source":  { "port": [] },
      "passOne": { "port": [ "source:1" ] }
    })") == fw::ErrorCode::OK, "the graph is built once");

    graph.DeInitialize();

    test::Check(Build(graph, R"({
      "source":    { "port": [] },
      "passLeft":  { "port": [ "source:1" ] },
      "passRight": { "port": [ "passLeft:1" ] }
    })") == fw::ErrorCode::OK, "and built again with a different topology");

    test::Check(graph.GetModules().size() == 3U,
                "the second graph is the one that stands (" + std::to_string(graph.GetModules().size()) + " modules)");

    const auto sinks = graph.GetSinkNames();
    test::Check(sinks.size() == 1U && sinks.front() == "passRight", "and its sink is the new one");
  }
}

void RunGraphTests()
{
  test::Section("graph topology");

  TheSettingsFragmentParses();
  ALinearGraphEndsAtItsLastModule();
  EveryUnconsumedModuleIsASink();
  AJoinedGraphHasOneSink();
  AnObsoleteModuleIsIgnoredRatherThanFatal();
  AnUnknownModuleIsRejected();
  APortWiredToTheWrongTypeIsRejected();
  APredecessorThatDoesNotExistIsRejected();
  ADuplicateModuleIsRejected();
  AGraphIsRebuiltFromScratch();
}

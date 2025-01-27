#include "envoy/config/trace/v3/dynamic_ot.pb.h"
#include "envoy/config/trace/v3/dynamic_ot.pb.validate.h"
#include "envoy/config/trace/v3/http_tracer.pb.h"

#include "source/extensions/tracers/dynamic_ot/config.h"

#include "test/mocks/server/tracer_factory.h"
#include "test/mocks/server/tracer_factory_context.h"
#include "test/test_common/environment.h"

#include "fmt/printf.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Eq;
using testing::NiceMock;
using testing::Return;

namespace Envoy {
namespace Extensions {
namespace Tracers {
namespace DynamicOt {
namespace {

// Disabled due to heapcheck reporting false positives when the test is statically linked with
// libstdc++ See https://github.com/envoyproxy/envoy/issues/7647 for the discussion
// TODO (dmitri-d) there currently isn't a way to resolve this: some tests will fail when libstdc++
// is dynamically linked, this test fails when it's statically linked
TEST(DynamicOtTracerConfigTest, DISABLED_DynamicOpentracingHttpTracer) {
  NiceMock<Server::Configuration::MockTracerFactoryContext> context;
  EXPECT_CALL(context.server_factory_context_.cluster_manager_,
              getThreadLocalCluster(Eq("fake_cluster")))
      .WillRepeatedly(
          Return(&context.server_factory_context_.cluster_manager_.thread_local_cluster_));
  ON_CALL(*context.server_factory_context_.cluster_manager_.thread_local_cluster_.cluster_.info_,
          features())
      .WillByDefault(Return(Upstream::ClusterInfo::Features::HTTP2));

  const std::string yaml_string = fmt::sprintf(
      R"EOF(
  http:
    name: envoy.tracers.dynamic_ot
    typed_config:
      "@type": type.googleapis.com/envoy.config.trace.v3.DynamicOtConfig
      library: %s
      config:
        output_file: fake_file
  )EOF",
      TestEnvironment::runfilesPath("mocktracer/libmocktracer_plugin.so", "io_opentracing_cpp"));
  envoy::config::trace::v3::Tracing configuration;
  TestUtility::loadFromYaml(yaml_string, configuration);

  DynamicOpenTracingTracerFactory factory;
  auto message = Config::Utility::translateToFactoryConfig(
      configuration.http(), ProtobufMessage::getStrictValidationVisitor(), factory);
  auto tracer = factory.createTracerDriver(*message, context);
  EXPECT_NE(nullptr, tracer);
}

// Test that the deprecated extension name is disabled by default.
// TODO(zuercher): remove when envoy.deprecated_features.allow_deprecated_extension_names is removed
TEST(DISABLED_DynamicOtTracerConfigTest, DEPRECATED_FEATURE_TEST(DeprecatedExtensionFilterName)) {
  const std::string deprecated_name = "envoy.dynamic.ot";

  ASSERT_EQ(nullptr, Registry::FactoryRegistry<Server::Configuration::TracerFactory>::getFactory(
                         deprecated_name));
}

} // namespace
} // namespace DynamicOt
} // namespace Tracers
} // namespace Extensions
} // namespace Envoy

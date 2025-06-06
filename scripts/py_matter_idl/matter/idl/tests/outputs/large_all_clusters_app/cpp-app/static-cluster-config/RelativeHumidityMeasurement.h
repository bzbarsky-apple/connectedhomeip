// DO NOT EDIT - Generated file
//
// Application configuration for RelativeHumidityMeasurement based on EMBER configuration
// from inputs/large_all_clusters_app.matter
#pragma once

#include <app/util/cluster-config.h>
#include <clusters/RelativeHumidityMeasurement/AttributeIds.h>
#include <clusters/RelativeHumidityMeasurement/CommandIds.h>
#include <clusters/RelativeHumidityMeasurement/Enums.h>

#include <array>

namespace chip {
namespace app {
namespace Clusters {
namespace RelativeHumidityMeasurement {
namespace StaticApplicationConfig {
namespace detail {
inline constexpr AttributeId kEndpoint0EnabledAttributes[] = {
    Attributes::ClusterRevision::Id,
    Attributes::FeatureMap::Id,
    Attributes::MaxMeasuredValue::Id,
    Attributes::MeasuredValue::Id,
    Attributes::MinMeasuredValue::Id,
};
inline constexpr AttributeId kEndpoint1EnabledAttributes[] = {
    Attributes::ClusterRevision::Id,
    Attributes::FeatureMap::Id,
    Attributes::MaxMeasuredValue::Id,
    Attributes::MeasuredValue::Id,
    Attributes::MinMeasuredValue::Id,
    Attributes::Tolerance::Id,
};
} // namespace detail

using FeatureBitmapType = Clusters::StaticApplicationConfig::NoFeatureFlagsDefined;

inline constexpr std::array<Clusters::StaticApplicationConfig::ClusterConfiguration<FeatureBitmapType>, 2> kFixedClusterConfig = { {
    {
        .endpointNumber = 0,
        .featureMap = BitFlags<FeatureBitmapType> {
        },
        .enabledAttributes = Span<const AttributeId>(detail::kEndpoint0EnabledAttributes),
        .enabledCommands = Span<const CommandId>(),
    },
    {
        .endpointNumber = 1,
        .featureMap = BitFlags<FeatureBitmapType> {
        },
        .enabledAttributes = Span<const AttributeId>(detail::kEndpoint1EnabledAttributes),
        .enabledCommands = Span<const CommandId>(),
    },
} };

// If a specific attribute is supported at all across all endpoint static instantiations
inline constexpr bool IsAttributeEnabledOnSomeEndpoint(AttributeId attributeId) {
  switch (attributeId) {
    case Attributes::ClusterRevision::Id:
    case Attributes::FeatureMap::Id:
    case Attributes::MaxMeasuredValue::Id:
    case Attributes::MeasuredValue::Id:
    case Attributes::MinMeasuredValue::Id:
    case Attributes::Tolerance::Id:
      return true;
    default:
      return false;
  }
}

// If a specific command is supported at all across all endpoint static instantiations
inline constexpr bool IsCommandEnabledOnSomeEndpoint(CommandId commandId) {
  switch (commandId) {
    default:
      return false;
  }
}

} // namespace StaticApplicationConfig
} // namespace RelativeHumidityMeasurement
} // namespace Clusters
} // namespace app
} // namespace chip


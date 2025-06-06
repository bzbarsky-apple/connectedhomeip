// DO NOT EDIT - Generated file
//
// Application configuration for OvenCavityOperationalState based on EMBER configuration
// from inputs/large_all_clusters_app.matter
#pragma once

#include <app/util/cluster-config.h>
#include <clusters/OvenCavityOperationalState/AttributeIds.h>
#include <clusters/OvenCavityOperationalState/CommandIds.h>
#include <clusters/OvenCavityOperationalState/Enums.h>

#include <array>

namespace chip {
namespace app {
namespace Clusters {
namespace OvenCavityOperationalState {
namespace StaticApplicationConfig {
namespace detail {
inline constexpr AttributeId kEndpoint1EnabledAttributes[] = {
    Attributes::AcceptedCommandList::Id,
    Attributes::AttributeList::Id,
    Attributes::ClusterRevision::Id,
    Attributes::CountdownTime::Id,
    Attributes::CurrentPhase::Id,
    Attributes::FeatureMap::Id,
    Attributes::GeneratedCommandList::Id,
    Attributes::OperationalError::Id,
    Attributes::OperationalState::Id,
    Attributes::OperationalStateList::Id,
    Attributes::PhaseList::Id,
};

inline constexpr CommandId kEndpoint1EnabledCommands[] = {
    Commands::OperationalCommandResponse::Id,
    Commands::Pause::Id,
    Commands::Resume::Id,
    Commands::Start::Id,
    Commands::Stop::Id,
};

} // namespace detail

using FeatureBitmapType = Clusters::StaticApplicationConfig::NoFeatureFlagsDefined;

inline constexpr std::array<Clusters::StaticApplicationConfig::ClusterConfiguration<FeatureBitmapType>, 1> kFixedClusterConfig = { {
    {
        .endpointNumber = 1,
        .featureMap = BitFlags<FeatureBitmapType> {
        },
        .enabledAttributes = Span<const AttributeId>(detail::kEndpoint1EnabledAttributes),
        .enabledCommands = Span<const CommandId>(detail::kEndpoint1EnabledCommands),
    },
} };

// If a specific attribute is supported at all across all endpoint static instantiations
inline constexpr bool IsAttributeEnabledOnSomeEndpoint(AttributeId attributeId) {
  switch (attributeId) {
    case Attributes::AcceptedCommandList::Id:
    case Attributes::AttributeList::Id:
    case Attributes::ClusterRevision::Id:
    case Attributes::CountdownTime::Id:
    case Attributes::CurrentPhase::Id:
    case Attributes::FeatureMap::Id:
    case Attributes::GeneratedCommandList::Id:
    case Attributes::OperationalError::Id:
    case Attributes::OperationalState::Id:
    case Attributes::OperationalStateList::Id:
    case Attributes::PhaseList::Id:
      return true;
    default:
      return false;
  }
}

// If a specific command is supported at all across all endpoint static instantiations
inline constexpr bool IsCommandEnabledOnSomeEndpoint(CommandId commandId) {
  switch (commandId) {
    case Commands::OperationalCommandResponse::Id:
    case Commands::Pause::Id:
    case Commands::Resume::Id:
    case Commands::Start::Id:
    case Commands::Stop::Id:
      return true;
    default:
      return false;
  }
}

} // namespace StaticApplicationConfig
} // namespace OvenCavityOperationalState
} // namespace Clusters
} // namespace app
} // namespace chip


/*
 *
 *    Copyright (c) 2023-2024 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <DEMManufacturerDelegate.h>
#include <DeviceEnergyManagementManager.h>
#include <ElectricalPowerMeasurementDelegate.h>
#include <EnergyEvseManager.h>
#include <PowerTopologyDelegate.h>

using chip::Protocols::InteractionModel::Status;
namespace chip {
namespace app {
namespace Clusters {
namespace EnergyEvse {

/**
 * The EVSEManufacturer example class
 */

class EVSEManufacturer : public DEMManufacturerDelegate
{
public:
    EVSEManufacturer(EnergyEvseManager * aEvseInstance,
                     ElectricalPowerMeasurement::ElectricalPowerMeasurementInstance * aEPMInstance,
                     PowerTopology::PowerTopologyInstance * aPTInstance, DeviceEnergyManagementManager * aDEMInstance)
    {
        mEvseInstance = aEvseInstance;
        mEPMInstance  = aEPMInstance;
        mPTInstance   = aPTInstance;
        mDEMInstance  = aDEMInstance;
    }

    virtual ~EVSEManufacturer() {}

    EnergyEvseManager * GetEvseInstance() { return mEvseInstance; }

    ElectricalPowerMeasurement::ElectricalPowerMeasurementInstance * GetEPMInstance() { return mEPMInstance; }

    EnergyEvseDelegate * GetEvseDelegate()
    {
        if (mEvseInstance)
        {
            return mEvseInstance->GetDelegate();
        }
        return nullptr;
    }

    ElectricalPowerMeasurement::ElectricalPowerMeasurementDelegate * GetEPMDelegate()
    {
        if (mEPMInstance)
        {
            return mEPMInstance->GetDelegate();
        }
        return nullptr;
    }

    PowerTopology::PowerTopologyDelegate * GetPTDelegate()
    {
        if (mPTInstance)
        {
            return mPTInstance->GetDelegate();
        }
        return nullptr;
    }

    DeviceEnergyManagementDelegate * GetDEMDelegate()
    {
        if (mDEMInstance)
        {
            return mDEMInstance->GetDelegate();
        }
        return nullptr;
    }

    /**
     *
     * Implement the DEMManufacturerDelegate interface
     *
     */
    // The PowerAdjustEnd event needs to report the approximate energy used by the ESA during the session.
    int64_t GetApproxEnergyDuringSession() override;
    CHIP_ERROR HandleDeviceEnergyManagementPowerAdjustRequest(const int64_t powerMw, const uint32_t durationS,
                                                              AdjustmentCauseEnum cause) override;
    CHIP_ERROR HandleDeviceEnergyManagementPowerAdjustCompletion() override;
    CHIP_ERROR HandleDeviceEnergyManagementCancelPowerAdjustRequest(CauseEnum cause) override;
    CHIP_ERROR HandleDeviceEnergyManagementStartTimeAdjustRequest(const uint32_t requestedStartTime,
                                                                  AdjustmentCauseEnum cause) override;
    CHIP_ERROR HandleDeviceEnergyManagementPauseRequest(const uint32_t durationS, AdjustmentCauseEnum cause) override;
    CHIP_ERROR HandleDeviceEnergyManagementPauseCompletion() override;
    CHIP_ERROR HandleDeviceEnergyManagementCancelPauseRequest(CauseEnum cause) override;
    CHIP_ERROR HandleDeviceEnergyManagementCancelRequest() override;
    CHIP_ERROR HandleModifyForecastRequest(
        const uint32_t forecastID,
        const DataModel::DecodableList<DeviceEnergyManagement::Structs::SlotAdjustmentStruct::DecodableType> & slotAdjustments,
        AdjustmentCauseEnum cause) override;
    CHIP_ERROR RequestConstraintBasedForecast(
        const DataModel::DecodableList<DeviceEnergyManagement::Structs::ConstraintsStruct::DecodableType> & constraints,
        AdjustmentCauseEnum cause) override;

    /**
     * @brief   Called at start up to apply hardware settings
     */
    CHIP_ERROR Init(chip::EndpointId powerSourceEndpointId);

    /**
     * @brief   Called at shutdown
     */
    CHIP_ERROR Shutdown();

    /**
     * @brief   Main Callback handler from delegate to user code
     */
    static void ApplicationCallbackHandler(const EVSECbInfo * cb, intptr_t arg);

    /**
     * @brief   Helper functions used by ComputeChargingSchedule
     */
    CHIP_ERROR DetermineRequiredEnergy(EnergyEvseDelegate * dg, int64_t & requiredEnergy_mWh,
                                       DataModel::Nullable<Percent> & targetSoC,
                                       DataModel::Nullable<int64_t> & targetAddedEnergy_mWh);

    CHIP_ERROR ComputeStartTime(EnergyEvseDelegate * dg, DataModel::Nullable<uint32_t> & startTime_epoch_s,
                                uint32_t targetTime_epoch_s, uint32_t now_epoch_s, int64_t requiredEnergy_mWh);
    /**
     * @brief   Simple example to demonstrate how an EVSE can compute the start time
     *          and duration of a charging schedule
     */
    CHIP_ERROR ComputeChargingSchedule();

    /**
     * @brief   Allows a client application to initialise the Accuracy, Measurement types etc
     */
    CHIP_ERROR InitializePowerMeasurementCluster();

    /**
     * @brief   Allows a client application to initialise the PowerSource cluster
     */
    CHIP_ERROR InitializePowerSourceCluster(chip::EndpointId endpointId);

    /**
     * @brief   Allows a client application to send in power readings into the system
     *
     * @param[in]  aEndpointId       - Endpoint to send to EPM Cluster
     * @param[in]  aActivePower_mW   - ActivePower measured in milli-watts
     * @param[in]  aVoltage_mV       - Voltage measured in milli-volts
     * @param[in]  aActiveCurrent_mA - ActiveCurrent measured in milli-amps
     */
    CHIP_ERROR SendPowerReading(EndpointId aEndpointId, int64_t aActivePower_mW, int64_t aVoltage_mV, int64_t aCurrent_mA) override;

    /**
     * @brief   Allows a client application to send cumulative energy readings into the system
     *
     *          This is a helper function to add timestamps to the readings
     *
     * @param[in]  aCumulativeEnergyImported -total energy imported in milli-watthours
     * @param[in]  aCumulativeEnergyExported -total energy exported in milli-watthours
     */
    CHIP_ERROR SendCumulativeEnergyReading(EndpointId aEndpointId, int64_t aCumulativeEnergyImported,
                                           int64_t aCumulativeEnergyExported) override;

    /**
     * @brief   Allows a client application to send periodic energy readings into the system
     *
     *          This is a helper function to add timestamps to the readings
     *
     * @param[in]  aPeriodicEnergyImported - energy imported in milli-watthours in last period
     * @param[in]  aPeriodicEnergyExported - energy exported in milli-watthours in last period
     */
    CHIP_ERROR SendPeriodicEnergyReading(EndpointId aEndpointId, int64_t aCumulativeEnergyImported,
                                         int64_t aCumulativeEnergyExported) override;

    /**  Fake Meter data generation - used for testing EPM/EEM clusters */
    /**
     * @brief   Starts a fake load/generator to periodically callback the power and energy
     *          clusters.
     * @param[in]   aEndpointId  - which endpoint is the meter to be updated on
     * @param[in]   aPower_mW    - the mean power of the load
     *                             Positive power indicates Imported energy (e.g. a load)
     *                             Negative power indicated Exported energy (e.g. a generator)
     * @param[in]   aPowerRandomness_mW  This is used to define the max randomness of the
     *                             random power values around the mean power of the load
     * @param[in]   aVoltage_mV  - the nominal voltage measurement
     * @param[in]   aVoltageRandomness_mV  This is used to define the max randomness of the
     *                             random voltage values
     * @param[in]   aCurrent_mA  - the nominal current measurement
     * @param[in]   aCurrentRandomness_mA  This is used to define the max randomness of the
     *                             random current values
     * @param[in]   aInterval_s  - the callback interval in seconds
     * @param[in]   bReset       - boolean: true will reset the energy values to 0
     */
    void StartFakeReadings(EndpointId aEndpointId, int64_t aPower_mW, uint32_t aPowerRandomness_mW, int64_t aVoltage_mV,
                           uint32_t aVoltageRandomness_mV, int64_t aCurrent_mA, uint32_t aCurrentRandomness_mA, uint8_t aInterval_s,
                           bool bReset);
    /**
     * @brief   Stops any active updates to the fake load data callbacks
     */
    void StopFakeReadings();

    /**
     * @brief   Sends fake meter data into the cluster and restarts the timer
     */
    void FakeReadingsUpdate();
    /**
     * @brief   Timer expiry callback to handle fake load
     */
    static void FakeReadingsTimerExpiry(System::Layer * systemLayer, void * manufacturer);

    /*
     * @brief   Updates the parameters used to generate fake power and energy readings
     *
     * @param   maximumChargeCurrent   Maximum Charge current in mA
     */
    void UpdateEVFakeReadings(const Amperage_mA maximumChargeCurrent);

private:
    EnergyEvseManager * mEvseInstance;
    ElectricalPowerMeasurement::ElectricalPowerMeasurementInstance * mEPMInstance;
    PowerTopology::PowerTopologyInstance * mPTInstance;
    DeviceEnergyManagementManager * mDEMInstance;

    int64_t mLastChargingEnergyMeter    = 0;
    int64_t mLastDischargingEnergyMeter = 0;
};

/** @brief Helper function to return the singleton EVSEManufacturer instance
 *
 * This is needed by the EVSEManufacturer class to support TestEventTriggers
 * which are called outside of any class context. This allows the EVSEManufacturer
 * class to return the relevant Delegate instance in which to invoke the test
 * events on.
 *
 * This function is typically found in main.cpp or wherever the singleton is created.
 */
EVSEManufacturer * GetEvseManufacturer();

} // namespace EnergyEvse
} // namespace Clusters
} // namespace app
} // namespace chip

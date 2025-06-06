/*
 *
 *    Copyright (c) 2021-2022 Project CHIP Authors
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

/**
 *    @file
 *      This file defines a common interface to access various types of secure
 *      pairing sessions (e.g. PASE, CASE)
 *
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/core/Optional.h>
#include <lib/core/TLV.h>
#include <messaging/ExchangeContext.h>
#include <messaging/SessionParameters.h>
#include <protocols/secure_channel/Constants.h>
#include <protocols/secure_channel/SessionEstablishmentDelegate.h>
#include <protocols/secure_channel/StatusReport.h>
#include <transport/CryptoContext.h>
#include <transport/SecureSession.h>

namespace chip {

class SessionManager;

class DLL_EXPORT PairingSession : public SessionDelegate
{
public:
    PairingSession() : mSecureSessionHolder(*this) {}
    virtual ~PairingSession() { Clear(); }

    virtual Transport::SecureSession::Type GetSecureSessionType() const = 0;
    virtual ScopedNodeId GetPeer() const                                = 0;
    virtual ScopedNodeId GetLocalScopedNodeId() const                   = 0;
    virtual CATValues GetPeerCATs() const                               = 0;

    // Implement SessionDelegate
    NewSessionHandlingPolicy GetNewSessionHandlingPolicy() override { return NewSessionHandlingPolicy::kStayAtOldSession; }
    void OnSessionReleased() override;

    Optional<uint16_t> GetLocalSessionId() const
    {
        Optional<uint16_t> localSessionId;
        VerifyOrExit(mSecureSessionHolder, localSessionId = NullOptional);
        VerifyOrExit(mSecureSessionHolder->GetSessionType() == Transport::Session::SessionType::kSecure,
                     localSessionId = Optional<uint16_t>::Missing());
        localSessionId.SetValue(mSecureSessionHolder->AsSecureSession()->GetLocalSessionId());
    exit:
        return localSessionId;
    }

    /**
     * Copy the underlying session (if present) into a SessionHandle that a caller can use to
     * obtain a reference to the session.
     */
    Optional<SessionHandle> CopySecureSession()
    {
        if (mSecureSessionHolder)
        {
            VerifyOrDie(mSecureSessionHolder->GetSessionType() == Transport::Session::SessionType::kSecure);
            return MakeOptional<SessionHandle>(*mSecureSessionHolder->AsSecureSession());
        }

        return Optional<SessionHandle>::Missing();
    }

    uint16_t GetPeerSessionId() const
    {
        VerifyOrDie(mPeerSessionId.HasValue());
        return mPeerSessionId.Value();
    }

    bool IsValidPeerSessionId() const { return mPeerSessionId.HasValue(); }

    /**
     * @brief
     *   Derive a secure session from the paired session. The API will return error if called before pairing is established.
     *
     * @param session     Reference to the secure session that will be initialized once pairing is complete
     * @return CHIP_ERROR The result of session derivation
     */
    virtual CHIP_ERROR DeriveSecureSession(CryptoContext & session) = 0;

    const ReliableMessageProtocolConfig & GetRemoteMRPConfig() const { return mRemoteSessionParams.GetMRPConfig(); }
    const SessionParameters & GetRemoteSessionParameters() const { return mRemoteSessionParams; }
    void SetRemoteMRPConfig(const ReliableMessageProtocolConfig & config) { mRemoteSessionParams.SetMRPConfig(config); }

    /**
     * Encode the Session Parameters using the provided TLV tag.
     */
    static CHIP_ERROR EncodeSessionParameters(TLV::Tag tag, const ReliableMessageProtocolConfig & mrpLocalConfig,
                                              TLV::TLVWriter & tlvWriter);

protected:
    /**
     * Allocate a secure session object from the passed session manager for the
     * pending session establishment operation.
     *
     * @param sessionManager        Session manager from which to allocate a secure session object
     * @param sessionEvictionHint   If we're either establishing or just finished establishing a session to a peer in either
     * initiator or responder roles, the node id of that peer should be provided in this argument. Else, it should be initialized to
     * a default-constructed ScopedNodeId().
     *
     * @return CHIP_ERROR The outcome of the allocation attempt
     */
    CHIP_ERROR AllocateSecureSession(SessionManager & sessionManager, const ScopedNodeId & sessionEvictionHint = ScopedNodeId());

    CHIP_ERROR ActivateSecureSession(const Transport::PeerAddress & peerAddress);

    void Finish();

    void DiscardExchange(); // Clear our reference to our exchange context pointer so that it can close itself at some later time.

    void SetPeerSessionId(uint16_t id) { mPeerSessionId.SetValue(id); }

    void SetRemoteSessionParameters(const SessionParameters & sessionParams) { mRemoteSessionParams = sessionParams; }

    virtual void OnSuccessStatusReport() {}

    // Handle a failure StatusReport message from the server.  protocolData will
    // depend on exactly what the generalCode/protocolCode are.
    virtual CHIP_ERROR OnFailureStatusReport(Protocols::SecureChannel::GeneralStatusCode generalCode, uint16_t protocolCode,
                                             Optional<uintptr_t> protocolData)
    {
        return CHIP_ERROR_INTERNAL;
    }

    void SendStatusReport(Optional<Messaging::ExchangeHandle> & exchangeCtxt, uint16_t protocolCode)
    {
        Protocols::SecureChannel::GeneralStatusCode generalCode = (protocolCode == Protocols::SecureChannel::kProtocolCodeSuccess)
            ? Protocols::SecureChannel::GeneralStatusCode::kSuccess
            : Protocols::SecureChannel::GeneralStatusCode::kFailure;

        ChipLogDetail(SecureChannel, "Sending status report. Protocol code %d, exchange %d", protocolCode,
                      exchangeCtxt.Value()->GetExchangeId());

        Protocols::SecureChannel::StatusReport statusReport(generalCode, Protocols::SecureChannel::Id, protocolCode);

        auto handle = System::PacketBufferHandle::New(statusReport.Size());
        VerifyOrReturn(!handle.IsNull(), ChipLogError(SecureChannel, "Failed to allocate status report message"));
        Encoding::LittleEndian::PacketBufferWriter bbuf(std::move(handle));

        statusReport.WriteToBuffer(bbuf);

        System::PacketBufferHandle msg = bbuf.Finalize();
        VerifyOrReturn(!msg.IsNull(), ChipLogError(SecureChannel, "Failed to allocate status report message"));

        CHIP_ERROR err = exchangeCtxt.Value()->SendMessage(Protocols::SecureChannel::MsgType::StatusReport, std::move(msg));
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(SecureChannel, "Failed to send status report message: %" CHIP_ERROR_FORMAT, err.Format());
        }
    }

    CHIP_ERROR HandleStatusReport(System::PacketBufferHandle && msg, bool successExpected)
    {
        Protocols::SecureChannel::StatusReport report;
        ReturnErrorOnFailure(report.Parse(std::move(msg)));
        VerifyOrReturnError(report.GetProtocolId() == Protocols::SecureChannel::Id, CHIP_ERROR_INVALID_ARGUMENT);

        if (report.GetGeneralCode() == Protocols::SecureChannel::GeneralStatusCode::kSuccess &&
            report.GetProtocolCode() == Protocols::SecureChannel::kProtocolCodeSuccess && successExpected)
        {
            OnSuccessStatusReport();
            return CHIP_NO_ERROR;
        }

        Optional<uintptr_t> protocolData;
        if (report.IsBusy() && report.GetMinimumWaitTime().has_value())
        {
            System::Clock::Milliseconds16 minimumWaitTime = report.GetMinimumWaitTime().value();
            ChipLogProgress(SecureChannel, "Received busy status report with minimum wait time: %u ms", minimumWaitTime.count());
            protocolData.Emplace(minimumWaitTime.count());
        }

        // It's very important that we propagate the return value from
        // OnFailureStatusReport out to the caller.  Make sure we return it directly.
        return OnFailureStatusReport(report.GetGeneralCode(), report.GetProtocolCode(), protocolData);
    }

    /**
     * Try to decode the current element (pointed by the TLV reader) as Session parameters (which include MRP parameters).
     * If the Session parameters are found, outparam sessionParameters is updated with the decoded values.
     *
     * Session parameters are optional. So, if the TLV reader is not pointing to the Session parameters,
     * the function is a noop.
     *
     * If the parameters are present, but TLV reader fails to correctly parse it, the function will
     * return the corresponding error.
     */
    static CHIP_ERROR DecodeSessionParametersIfPresent(TLV::Tag expectedTag, TLV::ContiguousBufferTLVReader & tlvReader,
                                                       SessionParameters & sessionParameters);

    bool IsSessionEstablishmentInProgress();

    // TODO: remove Clear, we should create a new instance instead reset the old instance.
    void Clear();

    /**
     * Notify our delegate about a session establishment error and the stage when the error occurs
     * if we have not already notified it of an error or success before.
     *
     * @param error The error code to report.
     * @param stage The stage of the session when the error occurs, defaults to kNotInKeyExchange.
     */
    void NotifySessionEstablishmentError(CHIP_ERROR error,
                                         SessionEstablishmentStage stage = SessionEstablishmentStage::kNotInKeyExchange);

protected:
    CryptoContext::SessionRole mRole;
    SessionHolderWithDelegate mSecureSessionHolder;
    // mSessionManager is set if we actually allocate a secure session, so we
    // can clean it up later as needed.
    SessionManager * mSessionManager                  = nullptr;
    Optional<Messaging::ExchangeHandle> mExchangeCtxt = NullOptional;
    SessionEstablishmentDelegate * mDelegate          = nullptr;

    // mLocalMRPConfig is our config which is sent to the other end and used by the peer session.
    // mRemoteSessionParams is received from other end and set to our session.
    // It is set the first time that session establishment is initiated.
    Optional<ReliableMessageProtocolConfig> mLocalMRPConfig;
    SessionParameters mRemoteSessionParams;

private:
    Optional<uint16_t> mPeerSessionId;
};

} // namespace chip

/*
 *  Copyright (c) 2015-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <proxygen/lib/http/codec/CodecProtocol.h>
#include <proxygen/lib/http/codec/HTTPCodecFilter.h>

namespace proxygen {

class SPDYStats;

class SPDYStatsFilter : public PassThroughHTTPCodecFilter {
 public:
  explicit SPDYStatsFilter(SPDYStats* counters, CodecProtocol protocol);
  ~SPDYStatsFilter() override;

  void setCounters(SPDYStats* counters) {
    counters_ = counters;
  }
  // ingress

  bool isHTTP2() const {
    return isHTTP2CodecProtocol(protocol_);
  }

  bool isHQ() const {
    return isHQCodecProtocol(protocol_);
  }

  void onHeadersComplete(StreamID stream,
                         std::unique_ptr<HTTPMessage> msg) override;

  void onBody(StreamID stream,
              std::unique_ptr<folly::IOBuf> chain,
              uint16_t padding) override;

  void onAbort(StreamID stream, ErrorCode statusCode) override;

  void onGoaway(uint64_t lastGoodStreamID,
                ErrorCode statusCode,
                std::unique_ptr<folly::IOBuf> debugData = nullptr) override;

  void onPingRequest(uint64_t uniqueID) override;

  void onPingReply(uint64_t uniqueID) override;

  void onWindowUpdate(StreamID stream, uint32_t amount) override;

  void onSettings(const SettingsList& settings) override;

  void onSettingsAck() override;

  void onPriority(StreamID stream,
                  const HTTPMessage::HTTPPriority& pri) override;

  // egress

  void generateHeader(folly::IOBufQueue& writeBuf,
                      StreamID stream,
                      const HTTPMessage& msg,
                      bool eom,
                      HTTPHeaderSize* size) override;

  void generatePushPromise(folly::IOBufQueue& writeBuf,
                           StreamID stream,
                           const HTTPMessage& msg,
                           StreamID assocStream,
                           bool eom,
                           HTTPHeaderSize* size) override;

  size_t generateBody(folly::IOBufQueue& writeBuf,
                      StreamID stream,
                      std::unique_ptr<folly::IOBuf> chain,
                      folly::Optional<uint8_t> padding,
                      bool eom) override;

  size_t generateRstStream(folly::IOBufQueue& writeBuf,
                           StreamID stream,
                           ErrorCode statusCode) override;

  size_t generateGoaway(
      folly::IOBufQueue& writeBuf,
      StreamID lastStream,
      ErrorCode statusCode,
      std::unique_ptr<folly::IOBuf> debugData = nullptr) override;

  size_t generatePingRequest(folly::IOBufQueue& writeBuf) override;

  size_t generatePingReply(folly::IOBufQueue& writeBuf,
                           uint64_t uniqueID) override;

  size_t generateSettings(folly::IOBufQueue& writeBuf) override;

  size_t generateWindowUpdate(folly::IOBufQueue& writeBuf,
                              StreamID stream,
                              uint32_t delta) override;

  size_t generatePriority(folly::IOBufQueue& writeBuf,
                          StreamID stream,
                          const HTTPMessage::HTTPPriority& pri) override;

 private:
  SPDYStats* counters_;
  CodecProtocol protocol_;
};

} // namespace proxygen

// tests/core/basic_ipc_service_test.cpp
#include "game_launcher/core/basic_ipc_service.h"

#include <gmock/gmock.h> // For mocking callbacks if needed, or just assertions
#include <gtest/gtest.h>

#include <future> // For std::promise/future if testing async handlers

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace game_launcher {
namespace core {

using ::testing::Eq;
using ::testing::IsTrue;
using ::testing::IsFalse;
using ::testing::_;

// Simple fixture for BasicIPCService tests
class BasicIPCServiceTest : public ::testing::Test {
protected:
    BasicIPCService ipc_service_;
};

TEST_F(BasicIPCServiceTest, RegisterRequestHandler_Success) {
    RequestHandler handler = [](const IpcPayload& req, ResponseCallback cb) {
        // No-op handler for registration test
    };
    auto status = ipc_service_.RegisterRequestHandler("testRequest", handler);
    EXPECT_TRUE(status.ok());
}

TEST_F(BasicIPCServiceTest, RegisterRequestHandler_Duplicate) {
    RequestHandler handler1 = [](const IpcPayload& req, ResponseCallback cb) {};
    RequestHandler handler2 = [](const IpcPayload& req, ResponseCallback cb) {};

    ASSERT_TRUE(ipc_service_.RegisterRequestHandler("duplicateTest", handler1).ok());
    auto status = ipc_service_.RegisterRequestHandler("duplicateTest", handler2);
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), absl::StatusCode::kAlreadyExists);
}

TEST_F(BasicIPCServiceTest, HandleIncomingRequest_NotFound) {
    bool callback_called = false;
    absl::Status received_status;

    ResponseCallback cb = [&](absl::StatusOr<IpcPayload> result) {
        callback_called = true;
        received_status = result.status();
    };

    ipc_service_.HandleIncomingRequest("nonExistentRequest", "payload", cb);

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(received_status.ok());
    EXPECT_EQ(received_status.code(), absl::StatusCode::kNotFound);
}

TEST_F(BasicIPCServiceTest, HandleIncomingRequest_FoundAndCalled) {
    bool handler_called = false;
    bool callback_called = false;
    std::string received_request_payload;
    absl::StatusOr<IpcPayload> received_callback_result;

    // Define the handler
    RequestHandler handler =
        [&](const IpcPayload& req_payload, ResponseCallback cb) {
            handler_called = true;
            received_request_payload = req_payload;
            // Simulate successful processing and respond via callback
            cb(IpcPayload("responsePayload"));
        };

    // Register the handler
    ASSERT_TRUE(ipc_service_.RegisterRequestHandler("processData", handler).ok());

    // Define the callback for the test
    ResponseCallback cb = [&](absl::StatusOr<IpcPayload> result) {
        callback_called = true;
        received_callback_result = result;
    };

    // Simulate an incoming request
    ipc_service_.HandleIncomingRequest("processData", "requestPayload", cb);

    // Assertions
    EXPECT_TRUE(handler_called);
    EXPECT_EQ(received_request_payload, "requestPayload");
    EXPECT_TRUE(callback_called);
    ASSERT_TRUE(received_callback_result.ok());
    EXPECT_EQ(*received_callback_result, "responsePayload");
}


TEST_F(BasicIPCServiceTest, HandleIncomingRequest_HandlerSendsError) {
    bool handler_called = false;
    bool callback_called = false;
    absl::StatusOr<IpcPayload> received_callback_result;

    // Define the handler that simulates an error
    RequestHandler handler =
        [&](const IpcPayload& req_payload, ResponseCallback cb) {
            handler_called = true;
            // Simulate error during processing
            cb(absl::InvalidArgumentError("Invalid payload received"));
        };

    // Register the handler
    ASSERT_TRUE(ipc_service_.RegisterRequestHandler("processError", handler).ok());

    // Define the callback for the test
    ResponseCallback cb = [&](absl::StatusOr<IpcPayload> result) {
        callback_called = true;
        received_callback_result = result;
    };

    // Simulate an incoming request
    ipc_service_.HandleIncomingRequest("processError", "somePayload", cb);

    // Assertions
    EXPECT_TRUE(handler_called);
    EXPECT_TRUE(callback_called);
    ASSERT_FALSE(received_callback_result.ok());
    EXPECT_EQ(received_callback_result.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_THAT(received_callback_result.status().message(), Eq("Invalid payload received"));
}


} // namespace core
} // namespace game_launcher

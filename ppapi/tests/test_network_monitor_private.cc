// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_network_monitor_private.h"

#include <string.h>

#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/net_address.h"
#include "ppapi/cpp/private/network_list_private.h"
#include "ppapi/cpp/private/network_monitor_private.h"
#include "ppapi/tests/testing_instance.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/utility/private/network_list_observer_private.h"

REGISTER_TEST_CASE(NetworkMonitorPrivate);

namespace {

struct CallbackData {
  explicit CallbackData(PP_Instance instance)
      : event(instance),
        call_counter(0),
        delete_monitor(false),
        monitor(NULL) {
  }
  ~CallbackData() {
  }
  NestedEvent event;
  int call_counter;
  pp::NetworkListPrivate network_list;
  bool delete_monitor;
  pp::NetworkMonitorPrivate* monitor;
};

void TestCallback(void* user_data, PP_Resource pp_network_list) {
  CallbackData* data = static_cast<CallbackData*>(user_data);
  data->call_counter++;

  data->network_list = pp::NetworkListPrivate(pp::PASS_REF, pp_network_list);

  if (data->delete_monitor)
    delete data->monitor;

  if (data->call_counter == 1)
    data->event.Signal();
}

class TestNetworkListObserver : public pp::NetworkListObserverPrivate {
 public:
  explicit TestNetworkListObserver(const pp::InstanceHandle& instance)
      : pp::NetworkListObserverPrivate(instance),
        event(instance.pp_instance()) {
  }
  virtual void OnNetworkListChanged(const pp::NetworkListPrivate& list) {
    current_list = list;
    event.Signal();
  }

  pp::NetworkListPrivate current_list;
  NestedEvent event;
};

}  // namespace

TestNetworkMonitorPrivate::TestNetworkMonitorPrivate(TestingInstance* instance)
    : TestCase(instance) {
}

bool TestNetworkMonitorPrivate::Init() {
  if (!pp::NetworkMonitorPrivate::IsAvailable())
    return false;

  return CheckTestingInterface();
}

void TestNetworkMonitorPrivate::RunTests(const std::string& filter) {
  RUN_TEST_FORCEASYNC_AND_NOT(Basic, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(2Monitors, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(DeleteInCallback, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(ListObserver, filter);
}

std::string TestNetworkMonitorPrivate::VerifyNetworkList(
    const pp::NetworkListPrivate& network_list) {
  // Verify that there is at least one network interface.
  size_t count = network_list.GetCount();
  ASSERT_TRUE(count >= 1U);

  // Iterate over all interfaces and verify their properties.
  for (size_t iface = 0; iface < count; ++iface) {
    // Verify that the first interface has at least one address.
    std::vector<pp::NetAddress> addresses;
    network_list.GetIpAddresses(iface, &addresses);
    ASSERT_TRUE(addresses.size() >= 1U);
    // Verify that the addresses are valid.
    for (size_t i = 0; i < addresses.size(); ++i) {
      PP_NetAddress_Family family = addresses[i].GetFamily();

      switch (family) {
        case PP_NETADDRESS_FAMILY_IPV4: {
          PP_NetAddress_IPv4 ipv4;
          ASSERT_TRUE(addresses[i].DescribeAsIPv4Address(&ipv4));

          // Verify that the address is not zero.
          bool all_zeros = true;
          for (size_t j = 0; j < sizeof(ipv4.addr); ++j) {
            if (ipv4.addr[j] != 0) {
              all_zeros = false;
              break;
            }
          }
          ASSERT_TRUE(!all_zeros);

          // Verify that port is set to 0.
          ASSERT_TRUE(ipv4.port == 0);
          break;
        }

        case PP_NETADDRESS_FAMILY_IPV6: {
          PP_NetAddress_IPv6 ipv6;
          ASSERT_TRUE(addresses[i].DescribeAsIPv6Address(&ipv6));

          // Verify that the address is not zero.
          bool all_zeros = true;
          for (size_t j = 0; j < sizeof(ipv6.addr); ++j) {
            if (ipv6.addr[j] != 0) {
              all_zeros = false;
              break;
            }
          }
          ASSERT_TRUE(!all_zeros);

          // Verify that port is set to 0.
          ASSERT_TRUE(ipv6.port == 0);
          break;
        }

        default:
          ASSERT_TRUE(false);
      }
    }

    // Verify that each interface has a unique name and a display name.
    ASSERT_FALSE(network_list.GetName(iface).empty());
    ASSERT_FALSE(network_list.GetDisplayName(iface).empty());

    PP_NetworkListType_Private type = network_list.GetType(iface);
    ASSERT_TRUE(type >= PP_NETWORKLIST_UNKNOWN);
    ASSERT_TRUE(type <= PP_NETWORKLIST_CELLULAR);

    PP_NetworkListState_Private state = network_list.GetState(iface);
    ASSERT_TRUE(state >= PP_NETWORKLIST_DOWN);
    ASSERT_TRUE(state <= PP_NETWORKLIST_UP);
  }

  PASS();
}

std::string TestNetworkMonitorPrivate::TestBasic() {
  CallbackData callback_data(instance_->pp_instance());

  pp::NetworkMonitorPrivate network_monitor(
      instance_, &TestCallback, &callback_data);
  callback_data.event.Wait();
  ASSERT_EQ(callback_data.call_counter, 1);

  ASSERT_SUBTEST_SUCCESS(VerifyNetworkList(callback_data.network_list));

  PASS();
}

std::string TestNetworkMonitorPrivate::Test2Monitors() {
  CallbackData callback_data(instance_->pp_instance());

  pp::NetworkMonitorPrivate network_monitor(
      instance_, &TestCallback, &callback_data);
  callback_data.event.Wait();
  ASSERT_EQ(callback_data.call_counter, 1);

  ASSERT_SUBTEST_SUCCESS(VerifyNetworkList(callback_data.network_list));

  CallbackData callback_data_2(instance_->pp_instance());

  pp::NetworkMonitorPrivate network_monitor_2(
      instance_, &TestCallback, &callback_data_2);
  callback_data_2.event.Wait();
  ASSERT_EQ(callback_data_2.call_counter, 1);

  ASSERT_SUBTEST_SUCCESS(VerifyNetworkList(callback_data_2.network_list));

  PASS();
}

std::string TestNetworkMonitorPrivate::TestDeleteInCallback() {
  CallbackData callback_data(instance_->pp_instance());

  pp::NetworkMonitorPrivate* network_monitor = new pp::NetworkMonitorPrivate(
      instance_, &TestCallback, &callback_data);
  callback_data.delete_monitor = true;
  callback_data.monitor = network_monitor;

  callback_data.event.Wait();
  ASSERT_EQ(callback_data.call_counter, 1);

  ASSERT_SUBTEST_SUCCESS(VerifyNetworkList(callback_data.network_list));

  PASS();
}

std::string TestNetworkMonitorPrivate::TestListObserver() {
  TestNetworkListObserver observer(instance_);
  observer.event.Wait();
  ASSERT_SUBTEST_SUCCESS(VerifyNetworkList(observer.current_list));
  PASS();
}

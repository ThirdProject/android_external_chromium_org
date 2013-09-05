/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_network_list_private.idl,
 *   modified Wed Sep  4 11:42:02 2013.
 */

#ifndef PPAPI_C_PRIVATE_PPB_NETWORK_LIST_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_NETWORK_LIST_PRIVATE_H_

#include "ppapi/c/pp_array_output.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_NETWORKLIST_PRIVATE_INTERFACE_0_3 "PPB_NetworkList_Private;0.3"
#define PPB_NETWORKLIST_PRIVATE_INTERFACE PPB_NETWORKLIST_PRIVATE_INTERFACE_0_3

/**
 * @file
 * This file defines the <code>PPB_NetworkList_Private</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
/**
 * Type of a network interface.
 */
typedef enum {
  /**
   * Type of the network interface is not known.
   */
  PP_NETWORKLIST_UNKNOWN = 0,
  /**
   * Wired Ethernet network.
   */
  PP_NETWORKLIST_ETHERNET = 1,
  /**
   * Wireless Wi-Fi network.
   */
  PP_NETWORKLIST_WIFI = 2,
  /**
   * Cellular network (e.g. LTE).
   */
  PP_NETWORKLIST_CELLULAR = 3
} PP_NetworkListType_Private;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_NetworkListType_Private, 4);

/**
 * State of a network interface.
 */
typedef enum {
  /**
   * Network interface is down.
   */
  PP_NETWORKLIST_DOWN = 0,
  /**
   * Network interface is up.
   */
  PP_NETWORKLIST_UP = 1
} PP_NetworkListState_Private;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_NetworkListState_Private, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_NetworkList_Private</code> is used to represent a
 * list of network interfaces and their configuration. The content of
 * the list is immutable. The current networks configuration can be
 * received using the <code>PPB_NetworkMonitor_Private</code>
 * interface.
 */
struct PPB_NetworkList_Private_0_3 {
  /**
   * Determines if the specified <code>resource</code> is a
   * <code>NetworkList</code> object.
   *
   * @param[in] resource A <code>PP_Resource</code> resource.
   *
   * @return Returns <code>PP_TRUE</code> if <code>resource</code> is
   * a <code>PPB_NetworkList_Private</code>, <code>PP_FALSE</code>
   * otherwise.
   */
  PP_Bool (*IsNetworkList)(PP_Resource resource);
  /**
   * Gets number of interfaces in the list.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * network list.
   *
   * @return Returns number of available network interfaces or 0 if
   * the list has never been updated.
   */
  uint32_t (*GetCount)(PP_Resource resource);
  /**
   * Gets name of a network interface.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * network list.
   * @param[in] index Index of the network interface.
   *
   * @return Returns name for the network interface with the specified
   * <code>index</code>.
   */
  struct PP_Var (*GetName)(PP_Resource resource, uint32_t index);
  /**
   * Gets type of a network interface.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * network list.
   * @param[in] index Index of the network interface.
   *
   * @return Returns type of the network interface with the specified
   * <code>index</code>.
   */
  PP_NetworkListType_Private (*GetType)(PP_Resource resource, uint32_t index);
  /**
   * Gets state of a network interface.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * network list.
   * @param[in] index Index of the network interface.
   *
   * @return Returns current state of the network interface with the
   * specified <code>index</code>.
   */
  PP_NetworkListState_Private (*GetState)(PP_Resource resource, uint32_t index);
  /**
   * Gets list of IP addresses for a network interface.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * network list.
   * @param[in] index Index of the network interface.
   * @param[in] output An output array which will receive
   * <code>PPB_NetAddress</code> resources on success. Please note that the
   * ref count of those resources has already been increased by 1 for the
   * caller.
   *
   * @return An error code from <code>pp_errors.h</code>.
   */
  int32_t (*GetIpAddresses)(PP_Resource resource,
                            uint32_t index,
                            struct PP_ArrayOutput output);
  /**
   * Gets display name of a network interface.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * network list.
   * @param[in] index Index of the network interface.
   *
   * @return Returns display name for the network interface with the
   * specified <code>index</code>.
   */
  struct PP_Var (*GetDisplayName)(PP_Resource resource, uint32_t index);
  /**
   * Gets MTU (Maximum Transmission Unit) of a network interface.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * network list.
   * @param[in] index Index of the network interface.
   *
   * @return Returns MTU for the network interface with the specified
   * <code>index</code> or 0 if MTU is unknown.
   */
  uint32_t (*GetMTU)(PP_Resource resource, uint32_t index);
};

typedef struct PPB_NetworkList_Private_0_3 PPB_NetworkList_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_NETWORK_LIST_PRIVATE_H_ */


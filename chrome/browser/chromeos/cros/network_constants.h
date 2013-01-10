// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROS_NETWORK_CONSTANTS_H_
#define CHROME_BROWSER_CHROMEOS_CROS_NETWORK_CONSTANTS_H_

namespace chromeos {

// This enumerates the various property indices that can be found in a
// dictionary being parsed.
enum PropertyIndex {
  PROPERTY_INDEX_ACTIVATE_OVER_NON_CELLULAR_NETWORK,
  PROPERTY_INDEX_ACTIVATION_STATE,
  PROPERTY_INDEX_ACTIVE_PROFILE,
  PROPERTY_INDEX_ARP_GATEWAY,
  PROPERTY_INDEX_AUTHENTICATION,
  PROPERTY_INDEX_AUTO_CONNECT,
  PROPERTY_INDEX_AVAILABLE_TECHNOLOGIES,
  PROPERTY_INDEX_CARRIER,
  PROPERTY_INDEX_CELLULAR_ALLOW_ROAMING,
  PROPERTY_INDEX_CELLULAR_APN,
  PROPERTY_INDEX_CELLULAR_APN_LIST,
  PROPERTY_INDEX_CELLULAR_LAST_GOOD_APN,
  PROPERTY_INDEX_CHECK_PORTAL,
  PROPERTY_INDEX_CHECK_PORTAL_LIST,
  PROPERTY_INDEX_CONNECTABLE,
  PROPERTY_INDEX_CONNECTED_TECHNOLOGIES,
  PROPERTY_INDEX_CONNECTIVITY_STATE,
  PROPERTY_INDEX_DEFAULT_TECHNOLOGY,
  PROPERTY_INDEX_DEVICE,
  PROPERTY_INDEX_DEVICES,
  PROPERTY_INDEX_EAP,
  PROPERTY_INDEX_EAP_ANONYMOUS_IDENTITY,
  PROPERTY_INDEX_EAP_CA_CERT,
  PROPERTY_INDEX_EAP_CA_CERT_ID,
  PROPERTY_INDEX_EAP_CA_CERT_NSS,
  PROPERTY_INDEX_EAP_CERT_ID,
  PROPERTY_INDEX_EAP_CLIENT_CERT,
  PROPERTY_INDEX_EAP_CLIENT_CERT_NSS,
  PROPERTY_INDEX_EAP_CLIENT_CERT_PATTERN,
  PROPERTY_INDEX_EAP_IDENTITY,
  PROPERTY_INDEX_EAP_KEY_ID,
  PROPERTY_INDEX_EAP_KEY_MGMT,
  PROPERTY_INDEX_EAP_METHOD,
  PROPERTY_INDEX_EAP_PASSWORD,
  PROPERTY_INDEX_EAP_PHASE_2_AUTH,
  PROPERTY_INDEX_EAP_PIN,
  PROPERTY_INDEX_EAP_PRIVATE_KEY,
  PROPERTY_INDEX_EAP_PRIVATE_KEY_PASSWORD,
  PROPERTY_INDEX_EAP_USE_SYSTEM_CAS,
  PROPERTY_INDEX_ENABLED_TECHNOLOGIES,
  PROPERTY_INDEX_ERROR,
  PROPERTY_INDEX_ESN,
  PROPERTY_INDEX_FAVORITE,
  PROPERTY_INDEX_FIRMWARE_REVISION,
  PROPERTY_INDEX_FOUND_NETWORKS,
  PROPERTY_INDEX_GUID,
  PROPERTY_INDEX_HARDWARE_REVISION,
  PROPERTY_INDEX_HOME_PROVIDER,
  PROPERTY_INDEX_HOST,
  PROPERTY_INDEX_ICCID,
  PROPERTY_INDEX_IDENTITY,
  PROPERTY_INDEX_IMEI,
  PROPERTY_INDEX_IMSI,
  PROPERTY_INDEX_IPSEC_AUTHENTICATIONTYPE,
  PROPERTY_INDEX_IPSEC_IKEVERSION,
  PROPERTY_INDEX_ISSUER_SUBJECT_PATTERN_COMMON_NAME,
  PROPERTY_INDEX_ISSUER_SUBJECT_PATTERN_LOCALITY,
  PROPERTY_INDEX_ISSUER_SUBJECT_PATTERN_ORGANIZATION,
  PROPERTY_INDEX_ISSUER_SUBJECT_PATTERN_ORGANIZATIONAL_UNIT,
  PROPERTY_INDEX_IS_ACTIVE,
  PROPERTY_INDEX_L2TPIPSEC_CA_CERT_NSS,
  PROPERTY_INDEX_L2TPIPSEC_CLIENT_CERT_ID,
  PROPERTY_INDEX_L2TPIPSEC_CLIENT_CERT_SLOT,
  PROPERTY_INDEX_L2TPIPSEC_GROUP_NAME,
  PROPERTY_INDEX_L2TPIPSEC_PASSWORD,
  PROPERTY_INDEX_L2TPIPSEC_PIN,
  PROPERTY_INDEX_L2TPIPSEC_PSK,
  PROPERTY_INDEX_L2TPIPSEC_PSK_REQUIRED,
  PROPERTY_INDEX_L2TPIPSEC_USER,
  PROPERTY_INDEX_MANUFACTURER,
  PROPERTY_INDEX_MDN,
  PROPERTY_INDEX_MEID,
  PROPERTY_INDEX_MIN,
  PROPERTY_INDEX_MODE,
  PROPERTY_INDEX_MODEL_ID,
  PROPERTY_INDEX_NAME,
  PROPERTY_INDEX_NETWORKS,
  PROPERTY_INDEX_NETWORK_TECHNOLOGY,
  PROPERTY_INDEX_OFFLINE_MODE,
  PROPERTY_INDEX_OLP,
  PROPERTY_INDEX_OLP_URL,
  PROPERTY_INDEX_ONC_CERTIFICATE_PATTERN_ENROLLMENT_URI,  // For parsing ONC
  PROPERTY_INDEX_ONC_CERTIFICATE_PATTERN_ISSUER,  // For parsing ONC
  PROPERTY_INDEX_ONC_CERTIFICATE_PATTERN_ISSUER_CA_REF,  // For parsing ONC
  PROPERTY_INDEX_ONC_CERTIFICATE_PATTERN_SUBJECT,  // For parsing ONC
  PROPERTY_INDEX_ONC_CLIENT_CERT_PATTERN,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_CLIENT_CERT_REF,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_CLIENT_CERT_TYPE,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_ETHERNET,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_IPSEC,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_L2TP,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_OPENVPN,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_EXCLUDE_DOMAINS,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_FTP,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_HOST,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_HTTP,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_HTTPS,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_MANUAL,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_PAC,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_PORT,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_SETTINGS,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_SOCKS,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_PROXY_TYPE,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_REMOVE,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_VPN,  // Used internally for ONC parsing
  PROPERTY_INDEX_ONC_WIFI,  // Used internally for ONC parsing
  PROPERTY_INDEX_OPEN_VPN_AUTH,
  PROPERTY_INDEX_OPEN_VPN_AUTHNOCACHE,
  PROPERTY_INDEX_OPEN_VPN_AUTHRETRY,
  PROPERTY_INDEX_OPEN_VPN_AUTHUSERPASS,
  PROPERTY_INDEX_OPEN_VPN_CACERT,
  PROPERTY_INDEX_OPEN_VPN_CERT,
  PROPERTY_INDEX_OPEN_VPN_CIPHER,
  PROPERTY_INDEX_OPEN_VPN_CLIENT_CERT_ID,
  PROPERTY_INDEX_OPEN_VPN_CLIENT_CERT_SLOT,
  PROPERTY_INDEX_OPEN_VPN_COMPLZO,
  PROPERTY_INDEX_OPEN_VPN_COMPNOADAPT,
  PROPERTY_INDEX_OPEN_VPN_KEYDIRECTION,
  PROPERTY_INDEX_OPEN_VPN_MGMT_ENABLE,
  PROPERTY_INDEX_OPEN_VPN_NSCERTTYPE,
  PROPERTY_INDEX_OPEN_VPN_OTP,
  PROPERTY_INDEX_OPEN_VPN_PASSWORD,
  PROPERTY_INDEX_OPEN_VPN_PIN,
  PROPERTY_INDEX_OPEN_VPN_PKCS11_PROVIDER,
  PROPERTY_INDEX_OPEN_VPN_PORT,
  PROPERTY_INDEX_OPEN_VPN_PROTO,
  PROPERTY_INDEX_OPEN_VPN_PUSHPEERINFO,
  PROPERTY_INDEX_OPEN_VPN_REMOTECERTEKU,
  PROPERTY_INDEX_OPEN_VPN_REMOTECERTKU,
  PROPERTY_INDEX_OPEN_VPN_REMOTECERTTLS,
  PROPERTY_INDEX_OPEN_VPN_RENEGSEC,
  PROPERTY_INDEX_OPEN_VPN_SERVERPOLLTIMEOUT,
  PROPERTY_INDEX_OPEN_VPN_SHAPER,
  PROPERTY_INDEX_OPEN_VPN_STATICCHALLENGE,
  PROPERTY_INDEX_OPEN_VPN_TLSAUTHCONTENTS,
  PROPERTY_INDEX_OPEN_VPN_TLSREMOTE,
  PROPERTY_INDEX_OPEN_VPN_USER,
  PROPERTY_INDEX_OPERATOR_CODE,
  PROPERTY_INDEX_OPERATOR_NAME,
  PROPERTY_INDEX_PASSPHRASE,
  PROPERTY_INDEX_PASSPHRASE_REQUIRED,
  PROPERTY_INDEX_PORTAL_URL,
  PROPERTY_INDEX_POWERED,
  PROPERTY_INDEX_PRIORITY,
  PROPERTY_INDEX_PRL_VERSION,
  PROPERTY_INDEX_PROFILE,
  PROPERTY_INDEX_PROFILES,
  PROPERTY_INDEX_PROVIDER,
  PROPERTY_INDEX_PROVIDER_HOST,
  PROPERTY_INDEX_PROVIDER_REQUIRES_ROAMING,
  PROPERTY_INDEX_PROVIDER_TYPE,
  PROPERTY_INDEX_PROXY_CONFIG,
  PROPERTY_INDEX_ROAMING_STATE,
  PROPERTY_INDEX_SAVE_CREDENTIALS,
  PROPERTY_INDEX_SCANNING,
  PROPERTY_INDEX_SECURITY,
  PROPERTY_INDEX_SELECTED_NETWORK,
  PROPERTY_INDEX_SERVICES,
  PROPERTY_INDEX_SERVICE_WATCH_LIST,
  PROPERTY_INDEX_SERVING_OPERATOR,
  PROPERTY_INDEX_SIGNAL_STRENGTH,
  PROPERTY_INDEX_SIM_LOCK,
  PROPERTY_INDEX_SIM_PRESENT,
  PROPERTY_INDEX_SSID,
  PROPERTY_INDEX_STATE,
  PROPERTY_INDEX_SUPPORT_NETWORK_SCAN,
  PROPERTY_INDEX_SUPPORTED_CARRIERS,
  PROPERTY_INDEX_TECHNOLOGY_FAMILY,
  PROPERTY_INDEX_TYPE,
  PROPERTY_INDEX_UI_DATA,
  PROPERTY_INDEX_UNKNOWN,
  PROPERTY_INDEX_USAGE_URL,
  PROPERTY_INDEX_VPN_DOMAIN,
  PROPERTY_INDEX_WIFI_AUTH_MODE,
  PROPERTY_INDEX_WIFI_BSSID,
  PROPERTY_INDEX_WIFI_FREQUENCY,
  PROPERTY_INDEX_WIFI_HEX_SSID,
  PROPERTY_INDEX_WIFI_HIDDEN_SSID,
  PROPERTY_INDEX_WIFI_PHY_MODE
};

// Connection enums (see flimflam/include/service.h)
enum ConnectionType {
  TYPE_UNKNOWN   = 0,
  TYPE_ETHERNET  = 1,
  TYPE_WIFI      = 2,
  TYPE_WIMAX     = 3,
  TYPE_BLUETOOTH = 4,
  TYPE_CELLULAR  = 5,
  TYPE_VPN       = 6,
};

enum ConnectionMode {
  MODE_UNKNOWN = 0,
  MODE_MANAGED = 1,
  MODE_ADHOC   = 2,
};

enum ConnectionSecurity {
  SECURITY_UNKNOWN = 0,
  SECURITY_NONE    = 1,
  SECURITY_WEP     = 2,
  SECURITY_WPA     = 3,
  SECURITY_RSN     = 4,
  SECURITY_8021X   = 5,
  SECURITY_PSK     = 6,
};

enum ConnectionState {
  STATE_UNKNOWN            = 0,
  STATE_IDLE               = 1,
  STATE_CARRIER            = 2,
  STATE_ASSOCIATION        = 3,
  STATE_CONFIGURATION      = 4,
  STATE_READY              = 5,
  STATE_DISCONNECT         = 6,
  STATE_FAILURE            = 7,
  STATE_ACTIVATION_FAILURE = 8,
  STATE_PORTAL             = 9,
  STATE_ONLINE             = 10,
  STATE_CONNECT_REQUESTED  = 11,  // Chrome only state
};

// Chrome only state set for user initiated connection attempts.
enum UserConnectState {
  USER_CONNECT_NONE    = 0,
  USER_CONNECT_STARTED = 1,
  USER_CONNECT_CONNECTED = 2,
  USER_CONNECT_FAILED = 3
};

// Network enums (see flimflam/include/network.h)
enum NetworkTechnology {
  NETWORK_TECHNOLOGY_UNKNOWN      = 0,
  NETWORK_TECHNOLOGY_1XRTT        = 1,
  NETWORK_TECHNOLOGY_EVDO         = 2,
  NETWORK_TECHNOLOGY_GPRS         = 3,
  NETWORK_TECHNOLOGY_EDGE         = 4,
  NETWORK_TECHNOLOGY_UMTS         = 5,
  NETWORK_TECHNOLOGY_HSPA         = 6,
  NETWORK_TECHNOLOGY_HSPA_PLUS    = 7,
  NETWORK_TECHNOLOGY_LTE          = 8,
  NETWORK_TECHNOLOGY_LTE_ADVANCED = 9,
  NETWORK_TECHNOLOGY_GSM          = 10,
};

enum ActivationState {
  ACTIVATION_STATE_UNKNOWN             = 0,
  ACTIVATION_STATE_ACTIVATED           = 1,
  ACTIVATION_STATE_ACTIVATING          = 2,
  ACTIVATION_STATE_NOT_ACTIVATED       = 3,
  ACTIVATION_STATE_PARTIALLY_ACTIVATED = 4,
};

enum NetworkRoamingState {
  ROAMING_STATE_UNKNOWN = 0,
  ROAMING_STATE_HOME    = 1,
  ROAMING_STATE_ROAMING = 2,
};

// Device enums (see flimflam/include/device.h)
enum TechnologyFamily {
  TECHNOLOGY_FAMILY_UNKNOWN = 0,
  TECHNOLOGY_FAMILY_CDMA    = 1,
  TECHNOLOGY_FAMILY_GSM     = 2
};

// Type of a pending SIM operation.
enum SimOperationType {
  SIM_OPERATION_NONE               = 0,
  SIM_OPERATION_CHANGE_PIN         = 1,
  SIM_OPERATION_CHANGE_REQUIRE_PIN = 2,
  SIM_OPERATION_ENTER_PIN          = 3,
  SIM_OPERATION_UNBLOCK_PIN        = 4,
};

// SIMLock states (see gobi-cromo-plugin/gobi_gsm_modem.cc)
enum SimLockState {
  SIM_UNKNOWN    = 0,
  SIM_UNLOCKED   = 1,
  SIM_LOCKED_PIN = 2,
  SIM_LOCKED_PUK = 3,  // also when SIM is blocked, then retries = 0.
};

// SIM PinRequire states.
// SIM_PIN_REQUIRE_UNKNOWN - SIM card is absent or SimLockState initial value
//                           hasn't been received yet.
// SIM_PIN_REQUIRED - SIM card is locked when booted/wake from sleep and
//                    requires user to enter PIN.
// SIM_PIN_NOT_REQUIRED - SIM card is unlocked all the time and requires PIN
// only on certain operations, such as ChangeRequirePin, ChangePin, EnterPin.
enum SimPinRequire {
  SIM_PIN_REQUIRE_UNKNOWN = 0,
  SIM_PIN_NOT_REQUIRED    = 1,
  SIM_PIN_REQUIRED        = 2,
};

// Any PIN operation result (EnterPin, UnblockPin etc.).
enum PinOperationError {
  PIN_ERROR_NONE           = 0,
  PIN_ERROR_UNKNOWN        = 1,
  PIN_ERROR_INCORRECT_CODE = 2,  // Either PIN/PUK specified is incorrect.
  PIN_ERROR_BLOCKED        = 3,  // No more PIN retries left, SIM is blocked.
};

// connection errors (see flimflam/include/service.h)
enum ConnectionError {
  ERROR_NO_ERROR               = 0,
  ERROR_OUT_OF_RANGE           = 1,
  ERROR_PIN_MISSING            = 2,
  ERROR_DHCP_FAILED            = 3,
  ERROR_CONNECT_FAILED         = 4,
  ERROR_BAD_PASSPHRASE         = 5,
  ERROR_BAD_WEPKEY             = 6,
  ERROR_ACTIVATION_FAILED      = 7,
  ERROR_NEED_EVDO              = 8,
  ERROR_NEED_HOME_NETWORK      = 9,
  ERROR_OTASP_FAILED           = 10,
  ERROR_AAA_FAILED             = 11,
  ERROR_INTERNAL               = 12,
  ERROR_DNS_LOOKUP_FAILED      = 13,
  ERROR_HTTP_GET_FAILED        = 14,
  ERROR_IPSEC_PSK_AUTH_FAILED  = 15,
  ERROR_IPSEC_CERT_AUTH_FAILED = 16,
  ERROR_PPP_AUTH_FAILED        = 17,
  ERROR_EAP_AUTHENTICATION_FAILED = 18,
  ERROR_EAP_LOCAL_TLS_FAILED   = 19,
  ERROR_EAP_REMOTE_TLS_FAILED  = 20,
  ERROR_UNKNOWN                = 255
};

// We are currently only supporting setting a single EAP Method.
enum EAPMethod {
  EAP_METHOD_UNKNOWN = 0,
  EAP_METHOD_PEAP    = 1,
  EAP_METHOD_TLS     = 2,
  EAP_METHOD_TTLS    = 3,
  EAP_METHOD_LEAP    = 4
};

// We are currently only supporting setting a single EAP phase 2 authentication.
enum EAPPhase2Auth {
  EAP_PHASE_2_AUTH_AUTO     = 0,
  EAP_PHASE_2_AUTH_MD5      = 1,
  EAP_PHASE_2_AUTH_MSCHAPV2 = 2,
  EAP_PHASE_2_AUTH_MSCHAP   = 3,
  EAP_PHASE_2_AUTH_PAP      = 4,
  EAP_PHASE_2_AUTH_CHAP     = 5
};

enum ClientCertType {
  CLIENT_CERT_TYPE_NONE    = 0,
  CLIENT_CERT_TYPE_REF     = 1,
  CLIENT_CERT_TYPE_PATTERN = 2
};

// Misc enums
enum NetworkProfileType {
  PROFILE_NONE,    // Not in any profile (not remembered).
  PROFILE_SHARED,  // In the local profile, shared by all users on device.
  PROFILE_USER     // In the user provile, not visible to other users.
};

// Virtual Network provider type.
enum ProviderType {
  PROVIDER_TYPE_L2TP_IPSEC_PSK,
  PROVIDER_TYPE_L2TP_IPSEC_USER_CERT,
  PROVIDER_TYPE_OPEN_VPN,
  // Add new provider types before PROVIDER_TYPE_MAX.
  PROVIDER_TYPE_MAX,
};

// Proxy type specified in ONC.
enum ProxyOncType {
  PROXY_ONC_DIRECT,
  PROXY_ONC_WPAD,
  PROXY_ONC_PAC,
  PROXY_ONC_MANUAL,
  PROXY_ONC_MAX,
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CROS_NETWORK_CONSTANTS_H_

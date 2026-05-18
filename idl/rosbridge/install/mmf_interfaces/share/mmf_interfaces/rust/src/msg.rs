#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



// Corresponds to mmf_interfaces__msg__AdswServiceStatus
/// Auto-generated from idl/dds/ros2/AdswServiceStatus.idl
/// Module: adswmgr, Struct: AdswServiceStatus

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct AdswServiceStatus {

    // This member is not documented.
    #[allow(missing_docs)]
    pub status_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status_msg: std::string::String,

}



impl Default for AdswServiceStatus {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::AdswServiceStatus::default())
  }
}

impl rosidl_runtime_rs::Message for AdswServiceStatus {
  type RmwMsg = super::msg::rmw::AdswServiceStatus;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        status_code: msg.status_code,
        status_msg: msg.status_msg.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      status_code: msg.status_code,
        status_msg: msg.status_msg.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      status_code: msg.status_code,
      status_msg: msg.status_msg.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__CommonStatusType
/// CommonStatusType enum (from common/Common.idl)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct CommonStatusType {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl CommonStatusType {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const UNKNOWN: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const PASS: i32 = 1;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const FAIL: i32 = 2;

}


impl Default for CommonStatusType {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::CommonStatusType::default())
  }
}

impl rosidl_runtime_rs::Message for CommonStatusType {
  type RmwMsg = super::msg::rmw::CommonStatusType;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__DetectionReport
/// Auto-generated from idl/dds/security_mgr/SecurityDetection.idl
/// Module: security, Struct: DetectionReport

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DetectionReport {

    // This member is not documented.
    #[allow(missing_docs)]
    pub client_id: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub dds_topic: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub rule: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub timestamp_ms: i64,

}



impl Default for DetectionReport {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::DetectionReport::default())
  }
}

impl rosidl_runtime_rs::Message for DetectionReport {
  type RmwMsg = super::msg::rmw::DetectionReport;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        client_id: msg.client_id.as_str().into(),
        dds_topic: msg.dds_topic.as_str().into(),
        rule: msg.rule.as_str().into(),
        timestamp_ms: msg.timestamp_ms,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        client_id: msg.client_id.as_str().into(),
        dds_topic: msg.dds_topic.as_str().into(),
        rule: msg.rule.as_str().into(),
      timestamp_ms: msg.timestamp_ms,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      client_id: msg.client_id.to_string(),
      dds_topic: msg.dds_topic.to_string(),
      rule: msg.rule.to_string(),
      timestamp_ms: msg.timestamp_ms,
    }
  }
}


// Corresponds to mmf_interfaces__msg__DiagnosticInfoListRequest
/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoListRequest
/// Empty message

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticInfoListRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub structure_needs_at_least_one_member: u8,

}



impl Default for DiagnosticInfoListRequest {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::DiagnosticInfoListRequest::default())
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoListRequest {
  type RmwMsg = super::msg::rmw::DiagnosticInfoListRequest;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        structure_needs_at_least_one_member: msg.structure_needs_at_least_one_member,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      structure_needs_at_least_one_member: msg.structure_needs_at_least_one_member,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      structure_needs_at_least_one_member: msg.structure_needs_at_least_one_member,
    }
  }
}


// Corresponds to mmf_interfaces__msg__DiagnosticInfoListResponse
/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoListResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticInfoListResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_codes: Vec<i32>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_names: Vec<std::string::String>,

}



impl Default for DiagnosticInfoListResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::DiagnosticInfoListResponse::default())
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoListResponse {
  type RmwMsg = super::msg::rmw::DiagnosticInfoListResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        ret_code: msg.ret_code,
        diag_codes: msg.diag_codes.into(),
        diag_names: msg.diag_names
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      ret_code: msg.ret_code,
        diag_codes: msg.diag_codes.as_slice().into(),
        diag_names: msg.diag_names
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      ret_code: msg.ret_code,
      diag_codes: msg.diag_codes
          .into_iter()
          .collect(),
      diag_names: msg.diag_names
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__DiagnosticInfoRequest
/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoRequest

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticInfoRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_name: std::string::String,

}



impl Default for DiagnosticInfoRequest {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::DiagnosticInfoRequest::default())
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoRequest {
  type RmwMsg = super::msg::rmw::DiagnosticInfoRequest;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        diag_code: msg.diag_code,
        diag_name: msg.diag_name.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      diag_code: msg.diag_code,
        diag_name: msg.diag_name.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      diag_code: msg.diag_code,
      diag_name: msg.diag_name.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__DiagnosticInfoResponse
/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticInfoResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub source_message_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub raw_data: std::string::String,

}



impl Default for DiagnosticInfoResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::DiagnosticInfoResponse::default())
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoResponse {
  type RmwMsg = super::msg::rmw::DiagnosticInfoResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        ret_code: msg.ret_code,
        diag_code: msg.diag_code,
        diag_name: msg.diag_name.as_str().into(),
        status: msg.status,
        message: msg.message.as_str().into(),
        source_message_name: msg.source_message_name.as_str().into(),
        raw_data: msg.raw_data.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      ret_code: msg.ret_code,
      diag_code: msg.diag_code,
        diag_name: msg.diag_name.as_str().into(),
      status: msg.status,
        message: msg.message.as_str().into(),
        source_message_name: msg.source_message_name.as_str().into(),
        raw_data: msg.raw_data.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      ret_code: msg.ret_code,
      diag_code: msg.diag_code,
      diag_name: msg.diag_name.to_string(),
      status: msg.status,
      message: msg.message.to_string(),
      source_message_name: msg.source_message_name.to_string(),
      raw_data: msg.raw_data.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__DiagnosticNotification
/// Auto-generated from idl/dds/diagnostic/DiagnosticNotification.idl
/// Module: diagnosticmgr, Struct: DiagnosticNotification

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticNotification {

    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub source_message_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub raw_data: std::string::String,

}



impl Default for DiagnosticNotification {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::DiagnosticNotification::default())
  }
}

impl rosidl_runtime_rs::Message for DiagnosticNotification {
  type RmwMsg = super::msg::rmw::DiagnosticNotification;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        diag_code: msg.diag_code,
        diag_name: msg.diag_name.as_str().into(),
        status: msg.status,
        message: msg.message.as_str().into(),
        source_message_name: msg.source_message_name.as_str().into(),
        raw_data: msg.raw_data.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      diag_code: msg.diag_code,
        diag_name: msg.diag_name.as_str().into(),
      status: msg.status,
        message: msg.message.as_str().into(),
        source_message_name: msg.source_message_name.as_str().into(),
        raw_data: msg.raw_data.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      diag_code: msg.diag_code,
      diag_name: msg.diag_name.to_string(),
      status: msg.status,
      message: msg.message.to_string(),
      source_message_name: msg.source_message_name.to_string(),
      raw_data: msg.raw_data.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__DiagnosticStatusType
/// DiagnosticStatusType enum (from diagnostic/DiagnosticCommon.idl :: diagnosticmgr module)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticStatusType {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl DiagnosticStatusType {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const UNKNOWN: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const PASS: i32 = 1;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const FATAL: i32 = 2;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const WARNING: i32 = 3;

}


impl Default for DiagnosticStatusType {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::DiagnosticStatusType::default())
  }
}

impl rosidl_runtime_rs::Message for DiagnosticStatusType {
  type RmwMsg = super::msg::rmw::DiagnosticStatusType;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetAppUpdateSettings
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateSettings

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateSettings {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,

}



impl Default for GetAppUpdateSettings {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetAppUpdateSettings::default())
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateSettings {
  type RmwMsg = super::msg::rmw::GetAppUpdateSettings;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        app_name: msg.app_name.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        app_name: msg.app_name.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      app_name: msg.app_name.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetAppUpdateSettingsResponse
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateSettingsResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateSettingsResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_settings: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for GetAppUpdateSettingsResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetAppUpdateSettingsResponse::default())
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateSettingsResponse {
  type RmwMsg = super::msg::rmw::GetAppUpdateSettingsResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        app_name: msg.app_name.as_str().into(),
        app_settings: msg.app_settings.as_str().into(),
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        app_name: msg.app_name.as_str().into(),
        app_settings: msg.app_settings.as_str().into(),
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      app_name: msg.app_name.to_string(),
      app_settings: msg.app_settings.to_string(),
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetAppUpdateState
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateState

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateState {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,

}



impl Default for GetAppUpdateState {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetAppUpdateState::default())
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateState {
  type RmwMsg = super::msg::rmw::GetAppUpdateState;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        app_name: msg.app_name.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        app_name: msg.app_name.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      app_name: msg.app_name.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetAppUpdateStateResponse
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateStateResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateStateResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_state_info: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for GetAppUpdateStateResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetAppUpdateStateResponse::default())
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateStateResponse {
  type RmwMsg = super::msg::rmw::GetAppUpdateStateResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        app_name: msg.app_name.as_str().into(),
        app_state_info: msg.app_state_info.as_str().into(),
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        app_name: msg.app_name.as_str().into(),
        app_state_info: msg.app_state_info.as_str().into(),
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      app_name: msg.app_name.to_string(),
      app_state_info: msg.app_state_info.to_string(),
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetServiceStateRequest
/// Auto-generated from idl/dds/lifecycle/GetServiceStateRequest.idl
/// Module: lifecyclemgr, Struct: GetServiceStateRequest

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetServiceStateRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<std::string::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_all_service: bool,

}



impl Default for GetServiceStateRequest {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetServiceStateRequest::default())
  }
}

impl rosidl_runtime_rs::Message for GetServiceStateRequest {
  type RmwMsg = super::msg::rmw::GetServiceStateRequest;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        os_type: msg.os_type,
        service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        is_all_service: msg.is_all_service,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      os_type: msg.os_type,
        service_list: msg.service_list
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      is_all_service: msg.is_all_service,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      os_type: msg.os_type,
      service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
      is_all_service: msg.is_all_service,
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetServiceStateResponse
/// Auto-generated from idl/dds/lifecycle/GetServiceStateRequest.idl
/// Module: lifecyclemgr, Struct: GetServiceStateResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetServiceStateResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<std::string::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_state_list: Vec<i32>,

}



impl Default for GetServiceStateResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetServiceStateResponse::default())
  }
}

impl rosidl_runtime_rs::Message for GetServiceStateResponse {
  type RmwMsg = super::msg::rmw::GetServiceStateResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        service_state_list: msg.service_state_list.into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        service_list: msg.service_list
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        service_state_list: msg.service_state_list.as_slice().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
      service_state_list: msg.service_state_list
          .into_iter()
          .collect(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetVehicleInfo
/// Auto-generated from idl/dds/ota_mgr/OtamVehicleServiceMsg.idl
/// Module: OtaManager, Struct: GetVehicleInfo

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetVehicleInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub name: std::string::String,

}



impl Default for GetVehicleInfo {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetVehicleInfo::default())
  }
}

impl rosidl_runtime_rs::Message for GetVehicleInfo {
  type RmwMsg = super::msg::rmw::GetVehicleInfo;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        name: msg.name.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        name: msg.name.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      name: msg.name.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__GetVehicleInfoResponse
/// Auto-generated from idl/dds/ota_mgr/OtamVehicleServiceMsg.idl
/// Module: OtaManager, Struct: GetVehicleInfoResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetVehicleInfoResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub value: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for GetVehicleInfoResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GetVehicleInfoResponse::default())
  }
}

impl rosidl_runtime_rs::Message for GetVehicleInfoResponse {
  type RmwMsg = super::msg::rmw::GetVehicleInfoResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        name: msg.name.as_str().into(),
        value: msg.value.as_str().into(),
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        name: msg.name.as_str().into(),
        value: msg.value.as_str().into(),
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      name: msg.name.to_string(),
      value: msg.value.to_string(),
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__LifecycleDiagReport
/// Auto-generated from idl/dds/lifecycle/LifecycleDiagReport.idl
/// Module: lifecyclemgr, Struct: LifecycleDiagReport

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct LifecycleDiagReport {

    // This member is not documented.
    #[allow(missing_docs)]
    pub is_started: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub running_service_num: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub total_services_num: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<super::msg::ServiceStatus>,

}



impl Default for LifecycleDiagReport {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::LifecycleDiagReport::default())
  }
}

impl rosidl_runtime_rs::Message for LifecycleDiagReport {
  type RmwMsg = super::msg::rmw::LifecycleDiagReport;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        is_started: msg.is_started,
        running_service_num: msg.running_service_num,
        total_services_num: msg.total_services_num,
        service_list: msg.service_list
          .into_iter()
          .map(|elem| super::msg::ServiceStatus::into_rmw_message(std::borrow::Cow::Owned(elem)).into_owned())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      is_started: msg.is_started,
      running_service_num: msg.running_service_num,
      total_services_num: msg.total_services_num,
        service_list: msg.service_list
          .iter()
          .map(|elem| super::msg::ServiceStatus::into_rmw_message(std::borrow::Cow::Borrowed(elem)).into_owned())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      is_started: msg.is_started,
      running_service_num: msg.running_service_num,
      total_services_num: msg.total_services_num,
      service_list: msg.service_list
          .into_iter()
          .map(super::msg::ServiceStatus::from_rmw_message)
          .collect(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__LifecycleReturnCode
/// ReturnCode enum for lifecycle (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct LifecycleReturnCode {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl LifecycleReturnCode {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const SUCCESS: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const FAILED: i32 = 1;

}


impl Default for LifecycleReturnCode {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::LifecycleReturnCode::default())
  }
}

impl rosidl_runtime_rs::Message for LifecycleReturnCode {
  type RmwMsg = super::msg::rmw::LifecycleReturnCode;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__LifecycleService
/// Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
/// Module: lifecyclemgr, Struct: LifecycleService

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct LifecycleService {

    // This member is not documented.
    #[allow(missing_docs)]
    pub name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub recovery_policy: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_enabled: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,

}



impl Default for LifecycleService {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::LifecycleService::default())
  }
}

impl rosidl_runtime_rs::Message for LifecycleService {
  type RmwMsg = super::msg::rmw::LifecycleService;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        name: msg.name.as_str().into(),
        service_type: msg.service_type,
        recovery_policy: msg.recovery_policy,
        is_enabled: msg.is_enabled,
        os_type: msg.os_type,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        name: msg.name.as_str().into(),
      service_type: msg.service_type,
      recovery_policy: msg.recovery_policy,
      is_enabled: msg.is_enabled,
      os_type: msg.os_type,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      name: msg.name.to_string(),
      service_type: msg.service_type,
      recovery_policy: msg.recovery_policy,
      is_enabled: msg.is_enabled,
      os_type: msg.os_type,
    }
  }
}


// Corresponds to mmf_interfaces__msg__NotifyAppUpdateStatus
/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: NotifyAppUpdateStatus

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyAppUpdateStatus {

    // This member is not documented.
    #[allow(missing_docs)]
    pub client_id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status: std::string::String,

}



impl Default for NotifyAppUpdateStatus {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::NotifyAppUpdateStatus::default())
  }
}

impl rosidl_runtime_rs::Message for NotifyAppUpdateStatus {
  type RmwMsg = super::msg::rmw::NotifyAppUpdateStatus;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        client_id: msg.client_id,
        app_name: msg.app_name.as_str().into(),
        status: msg.status.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      client_id: msg.client_id,
        app_name: msg.app_name.as_str().into(),
        status: msg.status.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      client_id: msg.client_id,
      app_name: msg.app_name.to_string(),
      status: msg.status.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__NotifyOtaManagerReady
/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: NotifyOtaManagerReady
/// Empty message

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyOtaManagerReady {

    // This member is not documented.
    #[allow(missing_docs)]
    pub structure_needs_at_least_one_member: u8,

}



impl Default for NotifyOtaManagerReady {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::NotifyOtaManagerReady::default())
  }
}

impl rosidl_runtime_rs::Message for NotifyOtaManagerReady {
  type RmwMsg = super::msg::rmw::NotifyOtaManagerReady;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        structure_needs_at_least_one_member: msg.structure_needs_at_least_one_member,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      structure_needs_at_least_one_member: msg.structure_needs_at_least_one_member,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      structure_needs_at_least_one_member: msg.structure_needs_at_least_one_member,
    }
  }
}


// Corresponds to mmf_interfaces__msg__NotifyOtaMgrInfo
/// Auto-generated from idl/dds/ota_mgr/OtamDiagnosticServiceMsg.idl
/// Module: OtaManager, Struct: NotifyOtaMgrInfo

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyOtaMgrInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub err_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub err_msg: std::string::String,

}



impl Default for NotifyOtaMgrInfo {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::NotifyOtaMgrInfo::default())
  }
}

impl rosidl_runtime_rs::Message for NotifyOtaMgrInfo {
  type RmwMsg = super::msg::rmw::NotifyOtaMgrInfo;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        err_code: msg.err_code,
        err_msg: msg.err_msg.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      err_code: msg.err_code,
        err_msg: msg.err_msg.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      err_code: msg.err_code,
      err_msg: msg.err_msg.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__NotifyServiceState
/// Auto-generated from idl/dds/lifecycle/NotifyServiceState.idl
/// Module: lifecyclemgr, Struct: NotifyServiceState

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyServiceState {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<std::string::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_state_list: Vec<i32>,

}



impl Default for NotifyServiceState {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::NotifyServiceState::default())
  }
}

impl rosidl_runtime_rs::Message for NotifyServiceState {
  type RmwMsg = super::msg::rmw::NotifyServiceState;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        os_type: msg.os_type,
        service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        service_state_list: msg.service_state_list.into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      os_type: msg.os_type,
        service_list: msg.service_list
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        service_state_list: msg.service_state_list.as_slice().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      os_type: msg.os_type,
      service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
      service_state_list: msg.service_state_list
          .into_iter()
          .collect(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__OsType
/// OsType enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct OsType {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl OsType {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const ROS2: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const AGL: i32 = 1;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const AUTOSAR: i32 = 2;

}


impl Default for OsType {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::OsType::default())
  }
}

impl rosidl_runtime_rs::Message for OsType {
  type RmwMsg = super::msg::rmw::OsType;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__RecoveryPolicy
/// RecoveryPolicy enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct RecoveryPolicy {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl RecoveryPolicy {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const R_DEFAULT: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const ALWAYS: i32 = 1;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const ON_FAILURE: i32 = 2;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const ON_SUCCESS: i32 = 3;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const ON_NOT_RESPONDING: i32 = 4;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const MANUAL: i32 = 5;

}


impl Default for RecoveryPolicy {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::RecoveryPolicy::default())
  }
}

impl rosidl_runtime_rs::Message for RecoveryPolicy {
  type RmwMsg = super::msg::rmw::RecoveryPolicy;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__ReturnValue
/// ReturnValue enum (from common/ReturnValue.idl)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct ReturnValue {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl ReturnValue {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const UNKNOWN: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const SUCCESS: i32 = 1;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const FAIL: i32 = 2;

}


impl Default for ReturnValue {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::ReturnValue::default())
  }
}

impl rosidl_runtime_rs::Message for ReturnValue {
  type RmwMsg = super::msg::rmw::ReturnValue;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__ServiceMode
/// ServiceMode enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct ServiceMode {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl ServiceMode {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const NORMAL: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const OTA_MODE: i32 = 1;

}


impl Default for ServiceMode {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::ServiceMode::default())
  }
}

impl rosidl_runtime_rs::Message for ServiceMode {
  type RmwMsg = super::msg::rmw::ServiceMode;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__ServiceState
/// ServiceState enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct ServiceState {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl ServiceState {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const NOT_RUNNING: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const INITIALIZING: i32 = 1;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const RUNNING: i32 = 2;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const RUNNING_NO_RESPONSE: i32 = 3;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const TERMINATING: i32 = 4;

}


impl Default for ServiceState {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::ServiceState::default())
  }
}

impl rosidl_runtime_rs::Message for ServiceState {
  type RmwMsg = super::msg::rmw::ServiceState;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__ServiceStatus
/// Auto-generated from idl/dds/lifecycle/LifecycleDiagReport.idl
/// Module: lifecyclemgr, Struct: ServiceStatus

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct ServiceStatus {

    // This member is not documented.
    #[allow(missing_docs)]
    pub name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_state: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_enabled: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub recovered_count: i32,

}



impl Default for ServiceStatus {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::ServiceStatus::default())
  }
}

impl rosidl_runtime_rs::Message for ServiceStatus {
  type RmwMsg = super::msg::rmw::ServiceStatus;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        name: msg.name.as_str().into(),
        service_type: msg.service_type,
        service_state: msg.service_state,
        is_enabled: msg.is_enabled,
        recovered_count: msg.recovered_count,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        name: msg.name.as_str().into(),
      service_type: msg.service_type,
      service_state: msg.service_state,
      is_enabled: msg.is_enabled,
      recovered_count: msg.recovered_count,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      name: msg.name.to_string(),
      service_type: msg.service_type,
      service_state: msg.service_state,
      is_enabled: msg.is_enabled,
      recovered_count: msg.recovered_count,
    }
  }
}


// Corresponds to mmf_interfaces__msg__ServiceType
/// ServiceType enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct ServiceType {

    // This member is not documented.
    #[allow(missing_docs)]
    pub value: i32,

}

impl ServiceType {

    // This constant is not documented.
    #[allow(missing_docs)]
    pub const NORMAL: i32 = 0;


    // This constant is not documented.
    #[allow(missing_docs)]
    pub const ONESHOT: i32 = 1;

}


impl Default for ServiceType {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::ServiceType::default())
  }
}

impl rosidl_runtime_rs::Message for ServiceType {
  type RmwMsg = super::msg::rmw::ServiceType;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        value: msg.value,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      value: msg.value,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      value: msg.value,
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetAppUpdate
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdate

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetAppUpdate {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,

}



impl Default for SetAppUpdate {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetAppUpdate::default())
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdate {
  type RmwMsg = super::msg::rmw::SetAppUpdate;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        app_name: msg.app_name.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        app_name: msg.app_name.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      app_name: msg.app_name.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetAppUpdateResponse
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdateResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetAppUpdateResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for SetAppUpdateResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetAppUpdateResponse::default())
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdateResponse {
  type RmwMsg = super::msg::rmw::SetAppUpdateResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        app_name: msg.app_name.as_str().into(),
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        app_name: msg.app_name.as_str().into(),
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      app_name: msg.app_name.to_string(),
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetAppUpdateSettings
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdateSettings

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetAppUpdateSettings {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_settings: std::string::String,

}



impl Default for SetAppUpdateSettings {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetAppUpdateSettings::default())
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdateSettings {
  type RmwMsg = super::msg::rmw::SetAppUpdateSettings;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        app_name: msg.app_name.as_str().into(),
        app_settings: msg.app_settings.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
        app_name: msg.app_name.as_str().into(),
        app_settings: msg.app_settings.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      app_name: msg.app_name.to_string(),
      app_settings: msg.app_settings.to_string(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetAppUpdateSettingsResponse
/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdateSettingsResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetAppUpdateSettingsResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for SetAppUpdateSettingsResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetAppUpdateSettingsResponse::default())
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdateSettingsResponse {
  type RmwMsg = super::msg::rmw::SetAppUpdateSettingsResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        id: msg.id,
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      id: msg.id,
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      id: msg.id,
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetOtaClientRegistration
/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: SetOtaClientRegistration

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetOtaClientRegistration {

    // This member is not documented.
    #[allow(missing_docs)]
    pub client_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_list: Vec<std::string::String>,

}



impl Default for SetOtaClientRegistration {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetOtaClientRegistration::default())
  }
}

impl rosidl_runtime_rs::Message for SetOtaClientRegistration {
  type RmwMsg = super::msg::rmw::SetOtaClientRegistration;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        client_name: msg.client_name.as_str().into(),
        app_list: msg.app_list
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        client_name: msg.client_name.as_str().into(),
        app_list: msg.app_list
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      client_name: msg.client_name.to_string(),
      app_list: msg.app_list
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetOtaClientRegistrationResponse
/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: SetOtaClientRegistrationResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetOtaClientRegistrationResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub client_id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for SetOtaClientRegistrationResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetOtaClientRegistrationResponse::default())
  }
}

impl rosidl_runtime_rs::Message for SetOtaClientRegistrationResponse {
  type RmwMsg = super::msg::rmw::SetOtaClientRegistrationResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        client_id: msg.client_id,
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      client_id: msg.client_id,
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      client_id: msg.client_id,
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetServiceModeRequest
/// Auto-generated from idl/dds/lifecycle/SetServiceModeRequest.idl
/// Module: lifecyclemgr, Struct: SetServiceModeRequest

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetServiceModeRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<std::string::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_group: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_mode: i32,

}



impl Default for SetServiceModeRequest {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetServiceModeRequest::default())
  }
}

impl rosidl_runtime_rs::Message for SetServiceModeRequest {
  type RmwMsg = super::msg::rmw::SetServiceModeRequest;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        os_type: msg.os_type,
        service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        is_group: msg.is_group,
        service_mode: msg.service_mode,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      os_type: msg.os_type,
        service_list: msg.service_list
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      is_group: msg.is_group,
      service_mode: msg.service_mode,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      os_type: msg.os_type,
      service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
      is_group: msg.is_group,
      service_mode: msg.service_mode,
    }
  }
}


// Corresponds to mmf_interfaces__msg__SetServiceModeResponse
/// Auto-generated from idl/dds/lifecycle/SetServiceModeRequest.idl
/// Module: lifecyclemgr, Struct: SetServiceModeResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetServiceModeResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for SetServiceModeResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SetServiceModeResponse::default())
  }
}

impl rosidl_runtime_rs::Message for SetServiceModeResponse {
  type RmwMsg = super::msg::rmw::SetServiceModeResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__StartServiceRequest
/// Auto-generated from idl/dds/lifecycle/StartServiceRequest.idl
/// Module: lifecyclemgr, Struct: StartServiceRequest

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StartServiceRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<std::string::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_group: bool,

}



impl Default for StartServiceRequest {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::StartServiceRequest::default())
  }
}

impl rosidl_runtime_rs::Message for StartServiceRequest {
  type RmwMsg = super::msg::rmw::StartServiceRequest;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        os_type: msg.os_type,
        service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        is_group: msg.is_group,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      os_type: msg.os_type,
        service_list: msg.service_list
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      is_group: msg.is_group,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      os_type: msg.os_type,
      service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
      is_group: msg.is_group,
    }
  }
}


// Corresponds to mmf_interfaces__msg__StartServiceResponse
/// Auto-generated from idl/dds/lifecycle/StartServiceRequest.idl
/// Module: lifecyclemgr, Struct: StartServiceResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StartServiceResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for StartServiceResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::StartServiceResponse::default())
  }
}

impl rosidl_runtime_rs::Message for StartServiceResponse {
  type RmwMsg = super::msg::rmw::StartServiceResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__StopServiceRequest
/// Auto-generated from idl/dds/lifecycle/StopServiceRequest.idl
/// Module: lifecyclemgr, Struct: StopServiceRequest

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StopServiceRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<std::string::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_group: bool,

}



impl Default for StopServiceRequest {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::StopServiceRequest::default())
  }
}

impl rosidl_runtime_rs::Message for StopServiceRequest {
  type RmwMsg = super::msg::rmw::StopServiceRequest;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        os_type: msg.os_type,
        service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.as_str().into())
          .collect(),
        is_group: msg.is_group,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      os_type: msg.os_type,
        service_list: msg.service_list
          .iter()
          .map(|elem| elem.as_str().into())
          .collect(),
      is_group: msg.is_group,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      os_type: msg.os_type,
      service_list: msg.service_list
          .into_iter()
          .map(|elem| elem.to_string())
          .collect(),
      is_group: msg.is_group,
    }
  }
}


// Corresponds to mmf_interfaces__msg__StopServiceResponse
/// Auto-generated from idl/dds/lifecycle/StopServiceRequest.idl
/// Module: lifecyclemgr, Struct: StopServiceResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StopServiceResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for StopServiceResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::StopServiceResponse::default())
  }
}

impl rosidl_runtime_rs::Message for StopServiceResponse {
  type RmwMsg = super::msg::rmw::StopServiceResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__UpdateServiceListRequest
/// Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
/// Module: lifecyclemgr, Struct: UpdateServiceListRequest

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct UpdateServiceListRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: Vec<super::msg::LifecycleService>,

}



impl Default for UpdateServiceListRequest {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::UpdateServiceListRequest::default())
  }
}

impl rosidl_runtime_rs::Message for UpdateServiceListRequest {
  type RmwMsg = super::msg::rmw::UpdateServiceListRequest;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        service_list: msg.service_list
          .into_iter()
          .map(|elem| super::msg::LifecycleService::into_rmw_message(std::borrow::Cow::Owned(elem)).into_owned())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        service_list: msg.service_list
          .iter()
          .map(|elem| super::msg::LifecycleService::into_rmw_message(std::borrow::Cow::Borrowed(elem)).into_owned())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      service_list: msg.service_list
          .into_iter()
          .map(super::msg::LifecycleService::from_rmw_message)
          .collect(),
    }
  }
}


// Corresponds to mmf_interfaces__msg__UpdateServiceListResponse
/// Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
/// Module: lifecyclemgr, Struct: UpdateServiceListResponse

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct UpdateServiceListResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for UpdateServiceListResponse {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::UpdateServiceListResponse::default())
  }
}

impl rosidl_runtime_rs::Message for UpdateServiceListResponse {
  type RmwMsg = super::msg::rmw::UpdateServiceListResponse;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        ret_code: msg.ret_code,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      ret_code: msg.ret_code,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      ret_code: msg.ret_code,
    }
  }
}


// Corresponds to mmf_interfaces__msg__VissRequestTopic
/// Auto-generated from idl/dds/vehicle_info_mgr/VissRequest.idl
/// Module: , Struct: VissRequestTopic

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct VissRequestTopic {

    // This member is not documented.
    #[allow(missing_docs)]
    pub payload: std::string::String,

}



impl Default for VissRequestTopic {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::VissRequestTopic::default())
  }
}

impl rosidl_runtime_rs::Message for VissRequestTopic {
  type RmwMsg = super::msg::rmw::VissRequestTopic;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        payload: msg.payload.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        payload: msg.payload.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      payload: msg.payload.to_string(),
    }
  }
}



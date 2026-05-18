#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__AdswServiceStatus() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__AdswServiceStatus__init(msg: *mut AdswServiceStatus) -> bool;
    fn mmf_interfaces__msg__AdswServiceStatus__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<AdswServiceStatus>, size: usize) -> bool;
    fn mmf_interfaces__msg__AdswServiceStatus__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<AdswServiceStatus>);
    fn mmf_interfaces__msg__AdswServiceStatus__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<AdswServiceStatus>, out_seq: *mut rosidl_runtime_rs::Sequence<AdswServiceStatus>) -> bool;
}

// Corresponds to mmf_interfaces__msg__AdswServiceStatus
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ros2/AdswServiceStatus.idl
/// Module: adswmgr, Struct: AdswServiceStatus

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct AdswServiceStatus {

    // This member is not documented.
    #[allow(missing_docs)]
    pub status_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status_msg: rosidl_runtime_rs::String,

}



impl Default for AdswServiceStatus {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__AdswServiceStatus__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__AdswServiceStatus__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for AdswServiceStatus {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__AdswServiceStatus__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__AdswServiceStatus__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__AdswServiceStatus__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for AdswServiceStatus {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for AdswServiceStatus where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/AdswServiceStatus";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__AdswServiceStatus() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__CommonStatusType() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__CommonStatusType__init(msg: *mut CommonStatusType) -> bool;
    fn mmf_interfaces__msg__CommonStatusType__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<CommonStatusType>, size: usize) -> bool;
    fn mmf_interfaces__msg__CommonStatusType__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<CommonStatusType>);
    fn mmf_interfaces__msg__CommonStatusType__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<CommonStatusType>, out_seq: *mut rosidl_runtime_rs::Sequence<CommonStatusType>) -> bool;
}

// Corresponds to mmf_interfaces__msg__CommonStatusType
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// CommonStatusType enum (from common/Common.idl)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__CommonStatusType__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__CommonStatusType__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for CommonStatusType {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__CommonStatusType__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__CommonStatusType__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__CommonStatusType__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for CommonStatusType {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for CommonStatusType where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/CommonStatusType";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__CommonStatusType() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DetectionReport() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__DetectionReport__init(msg: *mut DetectionReport) -> bool;
    fn mmf_interfaces__msg__DetectionReport__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<DetectionReport>, size: usize) -> bool;
    fn mmf_interfaces__msg__DetectionReport__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<DetectionReport>);
    fn mmf_interfaces__msg__DetectionReport__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<DetectionReport>, out_seq: *mut rosidl_runtime_rs::Sequence<DetectionReport>) -> bool;
}

// Corresponds to mmf_interfaces__msg__DetectionReport
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/security_mgr/SecurityDetection.idl
/// Module: security, Struct: DetectionReport

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DetectionReport {

    // This member is not documented.
    #[allow(missing_docs)]
    pub client_id: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub dds_topic: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub rule: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub timestamp_ms: i64,

}



impl Default for DetectionReport {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__DetectionReport__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__DetectionReport__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for DetectionReport {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DetectionReport__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DetectionReport__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DetectionReport__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for DetectionReport {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for DetectionReport where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/DetectionReport";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DetectionReport() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoListRequest() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__DiagnosticInfoListRequest__init(msg: *mut DiagnosticInfoListRequest) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoListRequest>, size: usize) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoListRequest>);
    fn mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<DiagnosticInfoListRequest>, out_seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoListRequest>) -> bool;
}

// Corresponds to mmf_interfaces__msg__DiagnosticInfoListRequest
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoListRequest
/// Empty message

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticInfoListRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub structure_needs_at_least_one_member: u8,

}



impl Default for DiagnosticInfoListRequest {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__DiagnosticInfoListRequest__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__DiagnosticInfoListRequest__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for DiagnosticInfoListRequest {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoListRequest {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for DiagnosticInfoListRequest where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/DiagnosticInfoListRequest";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoListRequest() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoListResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__DiagnosticInfoListResponse__init(msg: *mut DiagnosticInfoListResponse) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoListResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoListResponse>);
    fn mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<DiagnosticInfoListResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoListResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__DiagnosticInfoListResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoListResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticInfoListResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_codes: rosidl_runtime_rs::Sequence<i32>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_names: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,

}



impl Default for DiagnosticInfoListResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__DiagnosticInfoListResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__DiagnosticInfoListResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for DiagnosticInfoListResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoListResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for DiagnosticInfoListResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/DiagnosticInfoListResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoListResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoRequest() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__DiagnosticInfoRequest__init(msg: *mut DiagnosticInfoRequest) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoRequest__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoRequest>, size: usize) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoRequest__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoRequest>);
    fn mmf_interfaces__msg__DiagnosticInfoRequest__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<DiagnosticInfoRequest>, out_seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoRequest>) -> bool;
}

// Corresponds to mmf_interfaces__msg__DiagnosticInfoRequest
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoRequest

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticInfoRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_name: rosidl_runtime_rs::String,

}



impl Default for DiagnosticInfoRequest {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__DiagnosticInfoRequest__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__DiagnosticInfoRequest__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for DiagnosticInfoRequest {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoRequest__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoRequest__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoRequest__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoRequest {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for DiagnosticInfoRequest where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/DiagnosticInfoRequest";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoRequest() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__DiagnosticInfoResponse__init(msg: *mut DiagnosticInfoResponse) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__DiagnosticInfoResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoResponse>);
    fn mmf_interfaces__msg__DiagnosticInfoResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<DiagnosticInfoResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<DiagnosticInfoResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__DiagnosticInfoResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
/// Module: diagnosticmgr, Struct: DiagnosticInfoResponse

#[repr(C)]
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
    pub diag_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub source_message_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub raw_data: rosidl_runtime_rs::String,

}



impl Default for DiagnosticInfoResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__DiagnosticInfoResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__DiagnosticInfoResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for DiagnosticInfoResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticInfoResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for DiagnosticInfoResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for DiagnosticInfoResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/DiagnosticInfoResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticInfoResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticNotification() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__DiagnosticNotification__init(msg: *mut DiagnosticNotification) -> bool;
    fn mmf_interfaces__msg__DiagnosticNotification__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticNotification>, size: usize) -> bool;
    fn mmf_interfaces__msg__DiagnosticNotification__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticNotification>);
    fn mmf_interfaces__msg__DiagnosticNotification__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<DiagnosticNotification>, out_seq: *mut rosidl_runtime_rs::Sequence<DiagnosticNotification>) -> bool;
}

// Corresponds to mmf_interfaces__msg__DiagnosticNotification
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/diagnostic/DiagnosticNotification.idl
/// Module: diagnosticmgr, Struct: DiagnosticNotification

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct DiagnosticNotification {

    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub diag_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub source_message_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub raw_data: rosidl_runtime_rs::String,

}



impl Default for DiagnosticNotification {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__DiagnosticNotification__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__DiagnosticNotification__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for DiagnosticNotification {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticNotification__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticNotification__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticNotification__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for DiagnosticNotification {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for DiagnosticNotification where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/DiagnosticNotification";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticNotification() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticStatusType() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__DiagnosticStatusType__init(msg: *mut DiagnosticStatusType) -> bool;
    fn mmf_interfaces__msg__DiagnosticStatusType__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticStatusType>, size: usize) -> bool;
    fn mmf_interfaces__msg__DiagnosticStatusType__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<DiagnosticStatusType>);
    fn mmf_interfaces__msg__DiagnosticStatusType__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<DiagnosticStatusType>, out_seq: *mut rosidl_runtime_rs::Sequence<DiagnosticStatusType>) -> bool;
}

// Corresponds to mmf_interfaces__msg__DiagnosticStatusType
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// DiagnosticStatusType enum (from diagnostic/DiagnosticCommon.idl :: diagnosticmgr module)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__DiagnosticStatusType__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__DiagnosticStatusType__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for DiagnosticStatusType {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticStatusType__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticStatusType__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__DiagnosticStatusType__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for DiagnosticStatusType {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for DiagnosticStatusType where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/DiagnosticStatusType";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__DiagnosticStatusType() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateSettings() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetAppUpdateSettings__init(msg: *mut GetAppUpdateSettings) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateSettings__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateSettings>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateSettings__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateSettings>);
    fn mmf_interfaces__msg__GetAppUpdateSettings__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetAppUpdateSettings>, out_seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateSettings>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetAppUpdateSettings
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateSettings

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateSettings {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,

}



impl Default for GetAppUpdateSettings {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetAppUpdateSettings__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetAppUpdateSettings__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetAppUpdateSettings {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateSettings__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateSettings__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateSettings__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateSettings {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetAppUpdateSettings where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetAppUpdateSettings";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateSettings() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateSettingsResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetAppUpdateSettingsResponse__init(msg: *mut GetAppUpdateSettingsResponse) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateSettingsResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateSettingsResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateSettingsResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateSettingsResponse>);
    fn mmf_interfaces__msg__GetAppUpdateSettingsResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetAppUpdateSettingsResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateSettingsResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetAppUpdateSettingsResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateSettingsResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateSettingsResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_settings: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for GetAppUpdateSettingsResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetAppUpdateSettingsResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetAppUpdateSettingsResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetAppUpdateSettingsResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateSettingsResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateSettingsResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateSettingsResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateSettingsResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetAppUpdateSettingsResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetAppUpdateSettingsResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateSettingsResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateState() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetAppUpdateState__init(msg: *mut GetAppUpdateState) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateState__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateState>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateState__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateState>);
    fn mmf_interfaces__msg__GetAppUpdateState__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetAppUpdateState>, out_seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateState>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetAppUpdateState
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateState

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateState {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,

}



impl Default for GetAppUpdateState {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetAppUpdateState__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetAppUpdateState__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetAppUpdateState {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateState__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateState__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateState__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateState {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetAppUpdateState where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetAppUpdateState";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateState() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateStateResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetAppUpdateStateResponse__init(msg: *mut GetAppUpdateStateResponse) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateStateResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateStateResponse>);
    fn mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetAppUpdateStateResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<GetAppUpdateStateResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetAppUpdateStateResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: GetAppUpdateStateResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetAppUpdateStateResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_state_info: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for GetAppUpdateStateResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetAppUpdateStateResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetAppUpdateStateResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetAppUpdateStateResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetAppUpdateStateResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetAppUpdateStateResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetAppUpdateStateResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetAppUpdateStateResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetServiceStateRequest() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetServiceStateRequest__init(msg: *mut GetServiceStateRequest) -> bool;
    fn mmf_interfaces__msg__GetServiceStateRequest__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetServiceStateRequest>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetServiceStateRequest__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetServiceStateRequest>);
    fn mmf_interfaces__msg__GetServiceStateRequest__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetServiceStateRequest>, out_seq: *mut rosidl_runtime_rs::Sequence<GetServiceStateRequest>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetServiceStateRequest
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/GetServiceStateRequest.idl
/// Module: lifecyclemgr, Struct: GetServiceStateRequest

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetServiceStateRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_all_service: bool,

}



impl Default for GetServiceStateRequest {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetServiceStateRequest__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetServiceStateRequest__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetServiceStateRequest {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetServiceStateRequest__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetServiceStateRequest__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetServiceStateRequest__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetServiceStateRequest {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetServiceStateRequest where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetServiceStateRequest";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetServiceStateRequest() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetServiceStateResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetServiceStateResponse__init(msg: *mut GetServiceStateResponse) -> bool;
    fn mmf_interfaces__msg__GetServiceStateResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetServiceStateResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetServiceStateResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetServiceStateResponse>);
    fn mmf_interfaces__msg__GetServiceStateResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetServiceStateResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<GetServiceStateResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetServiceStateResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/GetServiceStateRequest.idl
/// Module: lifecyclemgr, Struct: GetServiceStateResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetServiceStateResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_state_list: rosidl_runtime_rs::Sequence<i32>,

}



impl Default for GetServiceStateResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetServiceStateResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetServiceStateResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetServiceStateResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetServiceStateResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetServiceStateResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetServiceStateResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetServiceStateResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetServiceStateResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetServiceStateResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetServiceStateResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetVehicleInfo() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetVehicleInfo__init(msg: *mut GetVehicleInfo) -> bool;
    fn mmf_interfaces__msg__GetVehicleInfo__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetVehicleInfo>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetVehicleInfo__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetVehicleInfo>);
    fn mmf_interfaces__msg__GetVehicleInfo__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetVehicleInfo>, out_seq: *mut rosidl_runtime_rs::Sequence<GetVehicleInfo>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetVehicleInfo
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamVehicleServiceMsg.idl
/// Module: OtaManager, Struct: GetVehicleInfo

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetVehicleInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub name: rosidl_runtime_rs::String,

}



impl Default for GetVehicleInfo {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetVehicleInfo__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetVehicleInfo__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetVehicleInfo {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetVehicleInfo__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetVehicleInfo__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetVehicleInfo__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetVehicleInfo {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetVehicleInfo where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetVehicleInfo";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetVehicleInfo() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetVehicleInfoResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__GetVehicleInfoResponse__init(msg: *mut GetVehicleInfoResponse) -> bool;
    fn mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GetVehicleInfoResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GetVehicleInfoResponse>);
    fn mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GetVehicleInfoResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<GetVehicleInfoResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__GetVehicleInfoResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamVehicleServiceMsg.idl
/// Module: OtaManager, Struct: GetVehicleInfoResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GetVehicleInfoResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub value: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for GetVehicleInfoResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__GetVehicleInfoResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__GetVehicleInfoResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GetVehicleInfoResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GetVehicleInfoResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GetVehicleInfoResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/GetVehicleInfoResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__GetVehicleInfoResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__LifecycleDiagReport() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__LifecycleDiagReport__init(msg: *mut LifecycleDiagReport) -> bool;
    fn mmf_interfaces__msg__LifecycleDiagReport__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<LifecycleDiagReport>, size: usize) -> bool;
    fn mmf_interfaces__msg__LifecycleDiagReport__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<LifecycleDiagReport>);
    fn mmf_interfaces__msg__LifecycleDiagReport__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<LifecycleDiagReport>, out_seq: *mut rosidl_runtime_rs::Sequence<LifecycleDiagReport>) -> bool;
}

// Corresponds to mmf_interfaces__msg__LifecycleDiagReport
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/LifecycleDiagReport.idl
/// Module: lifecyclemgr, Struct: LifecycleDiagReport

#[repr(C)]
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
    pub service_list: rosidl_runtime_rs::Sequence<super::super::msg::rmw::ServiceStatus>,

}



impl Default for LifecycleDiagReport {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__LifecycleDiagReport__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__LifecycleDiagReport__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for LifecycleDiagReport {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleDiagReport__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleDiagReport__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleDiagReport__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for LifecycleDiagReport {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for LifecycleDiagReport where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/LifecycleDiagReport";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__LifecycleDiagReport() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__LifecycleReturnCode() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__LifecycleReturnCode__init(msg: *mut LifecycleReturnCode) -> bool;
    fn mmf_interfaces__msg__LifecycleReturnCode__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<LifecycleReturnCode>, size: usize) -> bool;
    fn mmf_interfaces__msg__LifecycleReturnCode__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<LifecycleReturnCode>);
    fn mmf_interfaces__msg__LifecycleReturnCode__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<LifecycleReturnCode>, out_seq: *mut rosidl_runtime_rs::Sequence<LifecycleReturnCode>) -> bool;
}

// Corresponds to mmf_interfaces__msg__LifecycleReturnCode
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// ReturnCode enum for lifecycle (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__LifecycleReturnCode__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__LifecycleReturnCode__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for LifecycleReturnCode {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleReturnCode__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleReturnCode__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleReturnCode__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for LifecycleReturnCode {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for LifecycleReturnCode where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/LifecycleReturnCode";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__LifecycleReturnCode() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__LifecycleService() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__LifecycleService__init(msg: *mut LifecycleService) -> bool;
    fn mmf_interfaces__msg__LifecycleService__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<LifecycleService>, size: usize) -> bool;
    fn mmf_interfaces__msg__LifecycleService__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<LifecycleService>);
    fn mmf_interfaces__msg__LifecycleService__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<LifecycleService>, out_seq: *mut rosidl_runtime_rs::Sequence<LifecycleService>) -> bool;
}

// Corresponds to mmf_interfaces__msg__LifecycleService
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
/// Module: lifecyclemgr, Struct: LifecycleService

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct LifecycleService {

    // This member is not documented.
    #[allow(missing_docs)]
    pub name: rosidl_runtime_rs::String,


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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__LifecycleService__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__LifecycleService__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for LifecycleService {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleService__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleService__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__LifecycleService__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for LifecycleService {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for LifecycleService where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/LifecycleService";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__LifecycleService() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyAppUpdateStatus() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__NotifyAppUpdateStatus__init(msg: *mut NotifyAppUpdateStatus) -> bool;
    fn mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<NotifyAppUpdateStatus>, size: usize) -> bool;
    fn mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<NotifyAppUpdateStatus>);
    fn mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<NotifyAppUpdateStatus>, out_seq: *mut rosidl_runtime_rs::Sequence<NotifyAppUpdateStatus>) -> bool;
}

// Corresponds to mmf_interfaces__msg__NotifyAppUpdateStatus
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: NotifyAppUpdateStatus

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyAppUpdateStatus {

    // This member is not documented.
    #[allow(missing_docs)]
    pub client_id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub status: rosidl_runtime_rs::String,

}



impl Default for NotifyAppUpdateStatus {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__NotifyAppUpdateStatus__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__NotifyAppUpdateStatus__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for NotifyAppUpdateStatus {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for NotifyAppUpdateStatus {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for NotifyAppUpdateStatus where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/NotifyAppUpdateStatus";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyAppUpdateStatus() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyOtaManagerReady() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__NotifyOtaManagerReady__init(msg: *mut NotifyOtaManagerReady) -> bool;
    fn mmf_interfaces__msg__NotifyOtaManagerReady__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<NotifyOtaManagerReady>, size: usize) -> bool;
    fn mmf_interfaces__msg__NotifyOtaManagerReady__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<NotifyOtaManagerReady>);
    fn mmf_interfaces__msg__NotifyOtaManagerReady__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<NotifyOtaManagerReady>, out_seq: *mut rosidl_runtime_rs::Sequence<NotifyOtaManagerReady>) -> bool;
}

// Corresponds to mmf_interfaces__msg__NotifyOtaManagerReady
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: NotifyOtaManagerReady
/// Empty message

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyOtaManagerReady {

    // This member is not documented.
    #[allow(missing_docs)]
    pub structure_needs_at_least_one_member: u8,

}



impl Default for NotifyOtaManagerReady {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__NotifyOtaManagerReady__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__NotifyOtaManagerReady__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for NotifyOtaManagerReady {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyOtaManagerReady__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyOtaManagerReady__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyOtaManagerReady__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for NotifyOtaManagerReady {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for NotifyOtaManagerReady where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/NotifyOtaManagerReady";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyOtaManagerReady() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyOtaMgrInfo() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__NotifyOtaMgrInfo__init(msg: *mut NotifyOtaMgrInfo) -> bool;
    fn mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<NotifyOtaMgrInfo>, size: usize) -> bool;
    fn mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<NotifyOtaMgrInfo>);
    fn mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<NotifyOtaMgrInfo>, out_seq: *mut rosidl_runtime_rs::Sequence<NotifyOtaMgrInfo>) -> bool;
}

// Corresponds to mmf_interfaces__msg__NotifyOtaMgrInfo
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamDiagnosticServiceMsg.idl
/// Module: OtaManager, Struct: NotifyOtaMgrInfo

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyOtaMgrInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub err_code: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub err_msg: rosidl_runtime_rs::String,

}



impl Default for NotifyOtaMgrInfo {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__NotifyOtaMgrInfo__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__NotifyOtaMgrInfo__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for NotifyOtaMgrInfo {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for NotifyOtaMgrInfo {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for NotifyOtaMgrInfo where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/NotifyOtaMgrInfo";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyOtaMgrInfo() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyServiceState() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__NotifyServiceState__init(msg: *mut NotifyServiceState) -> bool;
    fn mmf_interfaces__msg__NotifyServiceState__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<NotifyServiceState>, size: usize) -> bool;
    fn mmf_interfaces__msg__NotifyServiceState__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<NotifyServiceState>);
    fn mmf_interfaces__msg__NotifyServiceState__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<NotifyServiceState>, out_seq: *mut rosidl_runtime_rs::Sequence<NotifyServiceState>) -> bool;
}

// Corresponds to mmf_interfaces__msg__NotifyServiceState
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/NotifyServiceState.idl
/// Module: lifecyclemgr, Struct: NotifyServiceState

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NotifyServiceState {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_state_list: rosidl_runtime_rs::Sequence<i32>,

}



impl Default for NotifyServiceState {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__NotifyServiceState__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__NotifyServiceState__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for NotifyServiceState {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyServiceState__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyServiceState__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__NotifyServiceState__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for NotifyServiceState {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for NotifyServiceState where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/NotifyServiceState";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__NotifyServiceState() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__OsType() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__OsType__init(msg: *mut OsType) -> bool;
    fn mmf_interfaces__msg__OsType__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<OsType>, size: usize) -> bool;
    fn mmf_interfaces__msg__OsType__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<OsType>);
    fn mmf_interfaces__msg__OsType__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<OsType>, out_seq: *mut rosidl_runtime_rs::Sequence<OsType>) -> bool;
}

// Corresponds to mmf_interfaces__msg__OsType
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// OsType enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__OsType__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__OsType__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for OsType {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__OsType__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__OsType__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__OsType__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for OsType {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for OsType where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/OsType";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__OsType() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__RecoveryPolicy() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__RecoveryPolicy__init(msg: *mut RecoveryPolicy) -> bool;
    fn mmf_interfaces__msg__RecoveryPolicy__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<RecoveryPolicy>, size: usize) -> bool;
    fn mmf_interfaces__msg__RecoveryPolicy__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<RecoveryPolicy>);
    fn mmf_interfaces__msg__RecoveryPolicy__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<RecoveryPolicy>, out_seq: *mut rosidl_runtime_rs::Sequence<RecoveryPolicy>) -> bool;
}

// Corresponds to mmf_interfaces__msg__RecoveryPolicy
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// RecoveryPolicy enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__RecoveryPolicy__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__RecoveryPolicy__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for RecoveryPolicy {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__RecoveryPolicy__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__RecoveryPolicy__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__RecoveryPolicy__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for RecoveryPolicy {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for RecoveryPolicy where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/RecoveryPolicy";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__RecoveryPolicy() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ReturnValue() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__ReturnValue__init(msg: *mut ReturnValue) -> bool;
    fn mmf_interfaces__msg__ReturnValue__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<ReturnValue>, size: usize) -> bool;
    fn mmf_interfaces__msg__ReturnValue__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<ReturnValue>);
    fn mmf_interfaces__msg__ReturnValue__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<ReturnValue>, out_seq: *mut rosidl_runtime_rs::Sequence<ReturnValue>) -> bool;
}

// Corresponds to mmf_interfaces__msg__ReturnValue
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// ReturnValue enum (from common/ReturnValue.idl)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__ReturnValue__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__ReturnValue__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for ReturnValue {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ReturnValue__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ReturnValue__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ReturnValue__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for ReturnValue {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for ReturnValue where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/ReturnValue";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ReturnValue() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceMode() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__ServiceMode__init(msg: *mut ServiceMode) -> bool;
    fn mmf_interfaces__msg__ServiceMode__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<ServiceMode>, size: usize) -> bool;
    fn mmf_interfaces__msg__ServiceMode__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<ServiceMode>);
    fn mmf_interfaces__msg__ServiceMode__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<ServiceMode>, out_seq: *mut rosidl_runtime_rs::Sequence<ServiceMode>) -> bool;
}

// Corresponds to mmf_interfaces__msg__ServiceMode
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// ServiceMode enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__ServiceMode__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__ServiceMode__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for ServiceMode {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceMode__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceMode__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceMode__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for ServiceMode {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for ServiceMode where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/ServiceMode";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceMode() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceState() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__ServiceState__init(msg: *mut ServiceState) -> bool;
    fn mmf_interfaces__msg__ServiceState__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<ServiceState>, size: usize) -> bool;
    fn mmf_interfaces__msg__ServiceState__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<ServiceState>);
    fn mmf_interfaces__msg__ServiceState__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<ServiceState>, out_seq: *mut rosidl_runtime_rs::Sequence<ServiceState>) -> bool;
}

// Corresponds to mmf_interfaces__msg__ServiceState
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// ServiceState enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__ServiceState__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__ServiceState__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for ServiceState {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceState__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceState__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceState__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for ServiceState {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for ServiceState where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/ServiceState";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceState() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceStatus() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__ServiceStatus__init(msg: *mut ServiceStatus) -> bool;
    fn mmf_interfaces__msg__ServiceStatus__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<ServiceStatus>, size: usize) -> bool;
    fn mmf_interfaces__msg__ServiceStatus__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<ServiceStatus>);
    fn mmf_interfaces__msg__ServiceStatus__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<ServiceStatus>, out_seq: *mut rosidl_runtime_rs::Sequence<ServiceStatus>) -> bool;
}

// Corresponds to mmf_interfaces__msg__ServiceStatus
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/LifecycleDiagReport.idl
/// Module: lifecyclemgr, Struct: ServiceStatus

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct ServiceStatus {

    // This member is not documented.
    #[allow(missing_docs)]
    pub name: rosidl_runtime_rs::String,


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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__ServiceStatus__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__ServiceStatus__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for ServiceStatus {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceStatus__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceStatus__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceStatus__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for ServiceStatus {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for ServiceStatus where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/ServiceStatus";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceStatus() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceType() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__ServiceType__init(msg: *mut ServiceType) -> bool;
    fn mmf_interfaces__msg__ServiceType__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<ServiceType>, size: usize) -> bool;
    fn mmf_interfaces__msg__ServiceType__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<ServiceType>);
    fn mmf_interfaces__msg__ServiceType__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<ServiceType>, out_seq: *mut rosidl_runtime_rs::Sequence<ServiceType>) -> bool;
}

// Corresponds to mmf_interfaces__msg__ServiceType
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// ServiceType enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__ServiceType__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__ServiceType__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for ServiceType {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceType__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceType__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__ServiceType__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for ServiceType {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for ServiceType where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/ServiceType";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__ServiceType() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdate() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetAppUpdate__init(msg: *mut SetAppUpdate) -> bool;
    fn mmf_interfaces__msg__SetAppUpdate__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdate>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetAppUpdate__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdate>);
    fn mmf_interfaces__msg__SetAppUpdate__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetAppUpdate>, out_seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdate>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetAppUpdate
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdate

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetAppUpdate {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,

}



impl Default for SetAppUpdate {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetAppUpdate__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetAppUpdate__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetAppUpdate {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdate__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdate__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdate__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdate {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetAppUpdate where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetAppUpdate";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdate() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdateResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetAppUpdateResponse__init(msg: *mut SetAppUpdateResponse) -> bool;
    fn mmf_interfaces__msg__SetAppUpdateResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetAppUpdateResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateResponse>);
    fn mmf_interfaces__msg__SetAppUpdateResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetAppUpdateResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetAppUpdateResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdateResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetAppUpdateResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for SetAppUpdateResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetAppUpdateResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetAppUpdateResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetAppUpdateResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdateResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetAppUpdateResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetAppUpdateResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdateResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdateSettings() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetAppUpdateSettings__init(msg: *mut SetAppUpdateSettings) -> bool;
    fn mmf_interfaces__msg__SetAppUpdateSettings__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateSettings>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetAppUpdateSettings__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateSettings>);
    fn mmf_interfaces__msg__SetAppUpdateSettings__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetAppUpdateSettings>, out_seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateSettings>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetAppUpdateSettings
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdateSettings

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetAppUpdateSettings {

    // This member is not documented.
    #[allow(missing_docs)]
    pub id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_settings: rosidl_runtime_rs::String,

}



impl Default for SetAppUpdateSettings {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetAppUpdateSettings__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetAppUpdateSettings__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetAppUpdateSettings {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateSettings__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateSettings__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateSettings__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdateSettings {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetAppUpdateSettings where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetAppUpdateSettings";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdateSettings() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdateSettingsResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetAppUpdateSettingsResponse__init(msg: *mut SetAppUpdateSettingsResponse) -> bool;
    fn mmf_interfaces__msg__SetAppUpdateSettingsResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateSettingsResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetAppUpdateSettingsResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateSettingsResponse>);
    fn mmf_interfaces__msg__SetAppUpdateSettingsResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetAppUpdateSettingsResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<SetAppUpdateSettingsResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetAppUpdateSettingsResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
/// Module: OtaManager, Struct: SetAppUpdateSettingsResponse

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetAppUpdateSettingsResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetAppUpdateSettingsResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetAppUpdateSettingsResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateSettingsResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateSettingsResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetAppUpdateSettingsResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetAppUpdateSettingsResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetAppUpdateSettingsResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetAppUpdateSettingsResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetAppUpdateSettingsResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetOtaClientRegistration() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetOtaClientRegistration__init(msg: *mut SetOtaClientRegistration) -> bool;
    fn mmf_interfaces__msg__SetOtaClientRegistration__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetOtaClientRegistration>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetOtaClientRegistration__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetOtaClientRegistration>);
    fn mmf_interfaces__msg__SetOtaClientRegistration__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetOtaClientRegistration>, out_seq: *mut rosidl_runtime_rs::Sequence<SetOtaClientRegistration>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetOtaClientRegistration
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: SetOtaClientRegistration

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetOtaClientRegistration {

    // This member is not documented.
    #[allow(missing_docs)]
    pub client_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub app_list: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,

}



impl Default for SetOtaClientRegistration {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetOtaClientRegistration__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetOtaClientRegistration__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetOtaClientRegistration {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetOtaClientRegistration__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetOtaClientRegistration__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetOtaClientRegistration__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetOtaClientRegistration {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetOtaClientRegistration where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetOtaClientRegistration";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetOtaClientRegistration() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetOtaClientRegistrationResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetOtaClientRegistrationResponse__init(msg: *mut SetOtaClientRegistrationResponse) -> bool;
    fn mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetOtaClientRegistrationResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetOtaClientRegistrationResponse>);
    fn mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetOtaClientRegistrationResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<SetOtaClientRegistrationResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetOtaClientRegistrationResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
/// Module: OtaManager, Struct: SetOtaClientRegistrationResponse

#[repr(C)]
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
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetOtaClientRegistrationResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetOtaClientRegistrationResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetOtaClientRegistrationResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetOtaClientRegistrationResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetOtaClientRegistrationResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetOtaClientRegistrationResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetOtaClientRegistrationResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetServiceModeRequest() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetServiceModeRequest__init(msg: *mut SetServiceModeRequest) -> bool;
    fn mmf_interfaces__msg__SetServiceModeRequest__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetServiceModeRequest>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetServiceModeRequest__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetServiceModeRequest>);
    fn mmf_interfaces__msg__SetServiceModeRequest__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetServiceModeRequest>, out_seq: *mut rosidl_runtime_rs::Sequence<SetServiceModeRequest>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetServiceModeRequest
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/SetServiceModeRequest.idl
/// Module: lifecyclemgr, Struct: SetServiceModeRequest

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetServiceModeRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_group: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_mode: i32,

}



impl Default for SetServiceModeRequest {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetServiceModeRequest__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetServiceModeRequest__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetServiceModeRequest {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetServiceModeRequest__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetServiceModeRequest__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetServiceModeRequest__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetServiceModeRequest {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetServiceModeRequest where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetServiceModeRequest";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetServiceModeRequest() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetServiceModeResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__SetServiceModeResponse__init(msg: *mut SetServiceModeResponse) -> bool;
    fn mmf_interfaces__msg__SetServiceModeResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetServiceModeResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__SetServiceModeResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetServiceModeResponse>);
    fn mmf_interfaces__msg__SetServiceModeResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetServiceModeResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<SetServiceModeResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__SetServiceModeResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/SetServiceModeRequest.idl
/// Module: lifecyclemgr, Struct: SetServiceModeResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetServiceModeResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for SetServiceModeResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__SetServiceModeResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__SetServiceModeResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetServiceModeResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetServiceModeResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetServiceModeResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__SetServiceModeResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetServiceModeResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetServiceModeResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/SetServiceModeResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__SetServiceModeResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StartServiceRequest() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__StartServiceRequest__init(msg: *mut StartServiceRequest) -> bool;
    fn mmf_interfaces__msg__StartServiceRequest__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<StartServiceRequest>, size: usize) -> bool;
    fn mmf_interfaces__msg__StartServiceRequest__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<StartServiceRequest>);
    fn mmf_interfaces__msg__StartServiceRequest__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<StartServiceRequest>, out_seq: *mut rosidl_runtime_rs::Sequence<StartServiceRequest>) -> bool;
}

// Corresponds to mmf_interfaces__msg__StartServiceRequest
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/StartServiceRequest.idl
/// Module: lifecyclemgr, Struct: StartServiceRequest

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StartServiceRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_group: bool,

}



impl Default for StartServiceRequest {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__StartServiceRequest__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__StartServiceRequest__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for StartServiceRequest {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StartServiceRequest__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StartServiceRequest__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StartServiceRequest__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for StartServiceRequest {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for StartServiceRequest where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/StartServiceRequest";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StartServiceRequest() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StartServiceResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__StartServiceResponse__init(msg: *mut StartServiceResponse) -> bool;
    fn mmf_interfaces__msg__StartServiceResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<StartServiceResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__StartServiceResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<StartServiceResponse>);
    fn mmf_interfaces__msg__StartServiceResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<StartServiceResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<StartServiceResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__StartServiceResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/StartServiceRequest.idl
/// Module: lifecyclemgr, Struct: StartServiceResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StartServiceResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for StartServiceResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__StartServiceResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__StartServiceResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for StartServiceResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StartServiceResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StartServiceResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StartServiceResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for StartServiceResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for StartServiceResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/StartServiceResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StartServiceResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StopServiceRequest() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__StopServiceRequest__init(msg: *mut StopServiceRequest) -> bool;
    fn mmf_interfaces__msg__StopServiceRequest__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<StopServiceRequest>, size: usize) -> bool;
    fn mmf_interfaces__msg__StopServiceRequest__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<StopServiceRequest>);
    fn mmf_interfaces__msg__StopServiceRequest__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<StopServiceRequest>, out_seq: *mut rosidl_runtime_rs::Sequence<StopServiceRequest>) -> bool;
}

// Corresponds to mmf_interfaces__msg__StopServiceRequest
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/StopServiceRequest.idl
/// Module: lifecyclemgr, Struct: StopServiceRequest

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StopServiceRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub os_type: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: rosidl_runtime_rs::Sequence<rosidl_runtime_rs::String>,


    // This member is not documented.
    #[allow(missing_docs)]
    pub is_group: bool,

}



impl Default for StopServiceRequest {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__StopServiceRequest__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__StopServiceRequest__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for StopServiceRequest {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StopServiceRequest__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StopServiceRequest__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StopServiceRequest__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for StopServiceRequest {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for StopServiceRequest where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/StopServiceRequest";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StopServiceRequest() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StopServiceResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__StopServiceResponse__init(msg: *mut StopServiceResponse) -> bool;
    fn mmf_interfaces__msg__StopServiceResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<StopServiceResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__StopServiceResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<StopServiceResponse>);
    fn mmf_interfaces__msg__StopServiceResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<StopServiceResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<StopServiceResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__StopServiceResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/StopServiceRequest.idl
/// Module: lifecyclemgr, Struct: StopServiceResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct StopServiceResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for StopServiceResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__StopServiceResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__StopServiceResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for StopServiceResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StopServiceResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StopServiceResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__StopServiceResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for StopServiceResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for StopServiceResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/StopServiceResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__StopServiceResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__UpdateServiceListRequest() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__UpdateServiceListRequest__init(msg: *mut UpdateServiceListRequest) -> bool;
    fn mmf_interfaces__msg__UpdateServiceListRequest__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<UpdateServiceListRequest>, size: usize) -> bool;
    fn mmf_interfaces__msg__UpdateServiceListRequest__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<UpdateServiceListRequest>);
    fn mmf_interfaces__msg__UpdateServiceListRequest__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<UpdateServiceListRequest>, out_seq: *mut rosidl_runtime_rs::Sequence<UpdateServiceListRequest>) -> bool;
}

// Corresponds to mmf_interfaces__msg__UpdateServiceListRequest
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
/// Module: lifecyclemgr, Struct: UpdateServiceListRequest

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct UpdateServiceListRequest {

    // This member is not documented.
    #[allow(missing_docs)]
    pub service_list: rosidl_runtime_rs::Sequence<super::super::msg::rmw::LifecycleService>,

}



impl Default for UpdateServiceListRequest {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__UpdateServiceListRequest__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__UpdateServiceListRequest__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for UpdateServiceListRequest {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__UpdateServiceListRequest__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__UpdateServiceListRequest__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__UpdateServiceListRequest__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for UpdateServiceListRequest {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for UpdateServiceListRequest where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/UpdateServiceListRequest";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__UpdateServiceListRequest() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__UpdateServiceListResponse() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__UpdateServiceListResponse__init(msg: *mut UpdateServiceListResponse) -> bool;
    fn mmf_interfaces__msg__UpdateServiceListResponse__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<UpdateServiceListResponse>, size: usize) -> bool;
    fn mmf_interfaces__msg__UpdateServiceListResponse__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<UpdateServiceListResponse>);
    fn mmf_interfaces__msg__UpdateServiceListResponse__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<UpdateServiceListResponse>, out_seq: *mut rosidl_runtime_rs::Sequence<UpdateServiceListResponse>) -> bool;
}

// Corresponds to mmf_interfaces__msg__UpdateServiceListResponse
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
/// Module: lifecyclemgr, Struct: UpdateServiceListResponse

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct UpdateServiceListResponse {

    // This member is not documented.
    #[allow(missing_docs)]
    pub ret_code: i32,

}



impl Default for UpdateServiceListResponse {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__UpdateServiceListResponse__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__UpdateServiceListResponse__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for UpdateServiceListResponse {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__UpdateServiceListResponse__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__UpdateServiceListResponse__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__UpdateServiceListResponse__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for UpdateServiceListResponse {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for UpdateServiceListResponse where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/UpdateServiceListResponse";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__UpdateServiceListResponse() }
  }
}


#[link(name = "mmf_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__VissRequestTopic() -> *const std::ffi::c_void;
}

#[link(name = "mmf_interfaces__rosidl_generator_c")]
extern "C" {
    fn mmf_interfaces__msg__VissRequestTopic__init(msg: *mut VissRequestTopic) -> bool;
    fn mmf_interfaces__msg__VissRequestTopic__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<VissRequestTopic>, size: usize) -> bool;
    fn mmf_interfaces__msg__VissRequestTopic__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<VissRequestTopic>);
    fn mmf_interfaces__msg__VissRequestTopic__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<VissRequestTopic>, out_seq: *mut rosidl_runtime_rs::Sequence<VissRequestTopic>) -> bool;
}

// Corresponds to mmf_interfaces__msg__VissRequestTopic
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Auto-generated from idl/dds/vehicle_info_mgr/VissRequest.idl
/// Module: , Struct: VissRequestTopic

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct VissRequestTopic {

    // This member is not documented.
    #[allow(missing_docs)]
    pub payload: rosidl_runtime_rs::String,

}



impl Default for VissRequestTopic {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !mmf_interfaces__msg__VissRequestTopic__init(&mut msg as *mut _) {
        panic!("Call to mmf_interfaces__msg__VissRequestTopic__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for VissRequestTopic {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__VissRequestTopic__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__VissRequestTopic__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { mmf_interfaces__msg__VissRequestTopic__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for VissRequestTopic {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for VissRequestTopic where Self: Sized {
  const TYPE_NAME: &'static str = "mmf_interfaces/msg/VissRequestTopic";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__mmf_interfaces__msg__VissRequestTopic() }
  }
}



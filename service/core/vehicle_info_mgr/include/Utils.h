#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <any>
#include <map>
#include "rapidjson/document.h"

// Function to trim leading and trailing whitespace
std::string trim(const std::string& str);

// Converts a VISS URL-style path to a dot-separated path
std::string UrlToPath(std::string url);

// Compares two std::any values for changes
bool any_value_changed(const std::any& a, const std::any& b);

// Converts an std::any value to its string representation
std::string any_to_string(const std::any& val);

// Gets the current time as an ISO 8601 string
std::string getCurrentTimestamp();

// Formats a millisecond epoch timestamp into an ISO 8601 string
std::string formatTimestamp(long long ms_since_epoch);

// Converts an std::any value to a rapidjson::Value
rapidjson::Value any_to_json_value(const std::any& any_val, rapidjson::Document::AllocatorType& allocator);

// Parses a "PdddDThhHmmMssS" format string into milliseconds
long long parseTimePeriod(const std::string& timePeriod);

// Converts a rapidjson::Value to a string based on VSS Datatype, supporting arrays
std::string json_value_to_string(const std::string& datatype, const rapidjson::Value& value);

// Parses a JSON array string into a vector of strings
std::vector<std::string> parse_json_array_string(const std::string& json_array_str);

#endif // UTILS_H

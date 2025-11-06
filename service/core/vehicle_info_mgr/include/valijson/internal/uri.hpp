#pragma once

#include <string>

#include <valijson/internal/regex.hpp>

namespace valijson {
namespace internal {
namespace uri {

/**
  * @brief  Placeholder function to check whether a URI is absolute
  *
  * This function just checks for '://'
  */
inline bool isUriAbsolute(const std::string &documentUri)
{
    static const char * placeholderMarker = "://";

    return documentUri.find(placeholderMarker) != std::string::npos;
}

/**
 * @brief  Placeholder function to check whether a URI is a URN
 *
 * This function validates that the URI matches the RFC 8141 spec
 */
inline bool isUrn(const std::string &documentUri) {
  static const internal::regex pattern(
      "^((urn)|(URN)):(?!urn:)([a-zA-Z0-9][a-zA-Z0-9-]{1,31})(:[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;=]+)+(\\?[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}(#[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}$");

  return internal::regex_match(documentUri, pattern);
}

/**
 * Placeholder function to resolve a relative URI within a given scope
 */
inline std::string resolveRelativeUri(
        const std::string &resolutionScope,
        const std::string &relativeUri)
{
  return resolutionScope + relativeUri;
/*
    if (relativeUri.empty()) {
        return resolutionScope;
    }

    if (relativeUri[0] == '/') {
        size_t protocol_end = resolutionScope.find("://");
        if (protocol_end == std::string::npos) {
            // No protocol found, assume it's a file path
            return relativeUri;
        }
        size_t domain_end = resolutionScope.find('/', protocol_end + 3);
        if (domain_end == std::string::npos) {
            return resolutionScope + relativeUri;
        }
        return resolutionScope.substr(0, domain_end) + relativeUri;
    } else {
        size_t last_slash = resolutionScope.rfind('/');
        if (last_slash == std::string::npos) {
             return resolutionScope + "/" + relativeUri;
        }
        
        size_t protocol_end = resolutionScope.find("://");
        if (protocol_end != std::string::npos && last_slash == protocol_end + 2) {
             return resolutionScope + "/" + relativeUri;
        }

        return resolutionScope.substr(0, last_slash + 1) + relativeUri;
    }
   */
}

} // namespace uri
} // namespace internal
} // namespace valijson

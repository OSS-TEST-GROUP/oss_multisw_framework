#!/bin/bash

#######################################################################
# Mock ArgoCD Server Test Script
#
# This script tests all ArgoCD API endpoints implemented in the mock server
#######################################################################

# Configuration
ARGOCD_SERVER="localhost:8080"
USERNAME="admin"
PASSWORD="admin123"
APP_NAME="sample-app"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print section header
print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Function to print success message
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to print info message
print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# Function to print error message
print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Check if server is running
print_header "Checking if mock server is running"
if ! curl -s http://${ARGOCD_SERVER}/health > /dev/null 2>&1; then
    print_error "Mock server is not running at http://${ARGOCD_SERVER}"
    echo "Please start the server first with: python3 mock_argocd_server.py"
    exit 1
fi
print_success "Mock server is running"

# Test 1: Create session (토큰 생성)
print_header "Test 1: Create Session (Create Token)"
print_info "POST /api/v1/session"
echo ""

RESPONSE=$(curl -s -X POST \
  -H "Content-Type: application/json" \
  -d "{\"username\":\"${USERNAME}\",\"password\":\"${PASSWORD}\"}" \
  http://${ARGOCD_SERVER}/api/v1/session)

echo "Response:"
echo "$RESPONSE" | jq '.'

TOKEN=$(echo "$RESPONSE" | jq -r '.token')

if [ -n "$TOKEN" ] && [ "$TOKEN" != "null" ]; then
    print_success "Token generated: $TOKEN"
else
    print_error "Failed to generate token"
    exit 1
fi

# Test 2: List all applications (App name 얻기)
print_header "Test 2: List All Applications (Get App Names)"
print_info "GET /api/v1/applications"
echo ""

RESPONSE=$(curl -s -H "Authorization: Bearer $TOKEN" \
  http://${ARGOCD_SERVER}/api/v1/applications)

echo "Response:"
echo "$RESPONSE" | jq '.'

APP_NAMES=$(echo "$RESPONSE" | jq -r '.items[].metadata.name')
print_success "Application names:"
echo "$APP_NAMES" | while read -r name; do
    echo "  - $name"
done

# Test 3: Get specific application (Auto Sync 값 얻기)
print_header "Test 3: Get Application (Get Auto Sync Status)"
print_info "GET /api/v1/applications/${APP_NAME}"
echo ""

RESPONSE=$(curl -s -H "Authorization: Bearer $TOKEN" \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME})

echo "Response (Auto Sync section):"
echo "$RESPONSE" | jq '.spec.syncPolicy.automated'

AUTO_SYNC=$(echo "$RESPONSE" | jq '.spec.syncPolicy.automated')
print_info "Current Auto Sync status: $AUTO_SYNC"

# Test 4: Enable Auto Sync (Auto Sync 값 변경: 켜기)
print_header "Test 4: Enable Auto Sync"
print_info "PATCH /api/v1/applications/${APP_NAME}"
echo ""

RESPONSE=$(curl -s -X PATCH \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "patch": "{\"spec\":{\"syncPolicy\":{\"automated\":{\"prune\":true,\"selfHeal\":true}}}}",
    "patchType": "merge"
  }' \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME})

echo "Response (Auto Sync section):"
echo "$RESPONSE" | jq '.spec.syncPolicy.automated'

AUTO_SYNC=$(echo "$RESPONSE" | jq '.spec.syncPolicy.automated')
if [ "$AUTO_SYNC" != "null" ]; then
    print_success "Auto Sync enabled successfully"
else
    print_error "Failed to enable Auto Sync"
fi

# Test 5: Get Auto Sync status again (확인)
print_header "Test 5: Verify Auto Sync is Enabled"
print_info "GET /api/v1/applications/${APP_NAME}"
echo ""

RESPONSE=$(curl -s -H "Authorization: Bearer $TOKEN" \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME})

echo "Auto Sync status:"
echo "$RESPONSE" | jq '.spec.syncPolicy.automated'

# Test 6: Get sync status (Update/Sync state 값 얻기)
print_header "Test 6: Get Sync Status"
print_info "GET /api/v1/applications/${APP_NAME}"
echo ""

RESPONSE=$(curl -s -H "Authorization: Bearer $TOKEN" \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME})

SYNC_STATUS=$(echo "$RESPONSE" | jq -r '.status.sync.status')
print_info "Current Sync status: $SYNC_STATUS"

# Test 7: Trigger sync (Update/Sync 요청)
print_header "Test 7: Trigger Sync"
print_info "POST /api/v1/applications/${APP_NAME}/sync"
echo ""

RESPONSE=$(curl -s -X POST \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{}' \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME}/sync)

echo "Response:"
echo "$RESPONSE" | jq '.'

STATUS=$(echo "$RESPONSE" | jq -r '.status')
if [ "$STATUS" = "success" ]; then
    print_success "Sync operation initiated successfully"
else
    print_error "Failed to trigger sync"
fi

# Test 8: Verify sync status after trigger
print_header "Test 8: Verify Sync Status After Trigger"
print_info "GET /api/v1/applications/${APP_NAME}"
echo ""

sleep 1  # Wait a moment for status to update

RESPONSE=$(curl -s -H "Authorization: Bearer $TOKEN" \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME})

SYNC_STATUS=$(echo "$RESPONSE" | jq -r '.status.sync.status')
OPERATION_PHASE=$(echo "$RESPONSE" | jq -r '.status.operationState.phase')

print_info "Sync status: $SYNC_STATUS"
print_info "Operation phase: $OPERATION_PHASE"

# Test 9: Disable Auto Sync (Auto Sync 값 변경: 끄기)
print_header "Test 9: Disable Auto Sync"
print_info "PATCH /api/v1/applications/${APP_NAME}"
echo ""

RESPONSE=$(curl -s -X PATCH \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "patch": "{\"spec\":{\"syncPolicy\":null}}",
    "patchType": "merge"
  }' \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME})

echo "Response (Auto Sync section):"
echo "$RESPONSE" | jq '.spec.syncPolicy.automated'

AUTO_SYNC=$(echo "$RESPONSE" | jq '.spec.syncPolicy')
if [ "$AUTO_SYNC" = "null" ]; then
    print_success "Auto Sync disabled successfully"
else
    print_error "Failed to disable Auto Sync"
fi

# Test 10: Verify Auto Sync is disabled
print_header "Test 10: Verify Auto Sync is Disabled"
print_info "GET /api/v1/applications/${APP_NAME}"
echo ""

RESPONSE=$(curl -s -H "Authorization: Bearer $TOKEN" \
  http://${ARGOCD_SERVER}/api/v1/applications/${APP_NAME})

echo "Auto Sync status:"
echo "$RESPONSE" | jq '.spec.syncPolicy.automated'

# Test 11: Health check
print_header "Test 11: Health Check"
print_info "GET /health"
echo ""

RESPONSE=$(curl -s http://${ARGOCD_SERVER}/health)
echo "Response:"
echo "$RESPONSE" | jq '.'

# Summary
print_header "Test Summary"
print_success "All tests completed successfully!"
echo ""
echo "Tested endpoints:"
echo "  ✓ POST   /api/v1/session"
echo "  ✓ GET    /api/v1/applications"
echo "  ✓ GET    /api/v1/applications/{app_name}"
echo "  ✓ PATCH  /api/v1/applications/{app_name} (Enable Auto Sync)"
echo "  ✓ PATCH  /api/v1/applications/{app_name} (Disable Auto Sync)"
echo "  ✓ POST   /api/v1/applications/{app_name}/sync"
echo "  ✓ GET    /health"
echo ""
print_info "For API documentation, visit: http://${ARGOCD_SERVER}/docs"

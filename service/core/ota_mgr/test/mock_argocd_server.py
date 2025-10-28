#!/usr/bin/env python3
"""
Mock ArgoCD API Server for testing (FastAPI)

This mock server simulates ArgoCD API endpoints to test OTA Manager
without requiring an actual ArgoCD instance.

Supported API endpoints:
- POST   /api/v1/session                           - Create session/token
- GET    /api/v1/applications                      - List all applications
- GET    /api/v1/applications/{app_name}           - Get specific application
- PATCH  /api/v1/applications/{app_name}           - Update application (Auto Sync)
- POST   /api/v1/applications/{app_name}/sync      - Trigger sync
- GET    /docs                                     - Swagger UI documentation
- GET    /health                                   - Health check endpoint
"""

from fastapi import FastAPI, HTTPException, Header, Depends, status
from fastapi.responses import JSONResponse
from pydantic import BaseModel, Field
from typing import Dict, Any, Optional, List
import json
import time
import argparse
import logging
import uvicorn
import socket

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


def get_local_ip():
    """Get local IP address"""
    try:
        # Create a socket connection to get the local IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
        s.close()
        return local_ip
    except Exception:
        return "127.0.0.1"


# Get local IP for server configuration
LOCAL_IP = get_local_ip()

# FastAPI app with multiple server URLs
app = FastAPI(
    title="Mock ArgoCD API Server",
    description="Mock server for ArgoCD API endpoints",
    version="1.0.0",
    servers=[
        {"url": f"http://{LOCAL_IP}:8080", "description": "Local IP Server"},
        {"url": "http://localhost:8080", "description": "Localhost"},
        {"url": "http://127.0.0.1:8080", "description": "Loopback"},
    ]
)

# Global state
auth_token: Optional[str] = None
applications: Dict[str, Dict[str, Any]] = {}


# Pydantic models
class SessionRequest(BaseModel):
    username: str
    password: str


class SessionResponse(BaseModel):
    token: str
    username: str
    iat: int
    exp: int


class PatchRequest(BaseModel):
    patch: str
    patchType: str = "merge"


class SyncResponse(BaseModel):
    status: str
    message: str
    application: str
    timestamp: float


class HealthResponse(BaseModel):
    status: str
    applications: int
    authenticated: bool


def load_mock_data():
    """Initialize mock application data"""
    global applications

    # Sample application 1
    applications["sample-app"] = {
        "metadata": {
            "name": "sample-app",
            "namespace": "argocd",
            "uid": "12345678-1234-1234-1234-123456789abc",
            "creationTimestamp": "2024-01-01T00:00:00Z"
        },
        "spec": {
            "source": {
                "repoURL": "https://github.com/example/sample-app",
                "path": "manifests",
                "targetRevision": "HEAD"
            },
            "destination": {
                "server": "https://kubernetes.default.svc",
                "namespace": "default"
            },
            "project": "default",
            "syncPolicy": None
        },
        "status": {
            "sync": {
                "status": "Synced",
                "comparedTo": {
                    "source": {
                        "repoURL": "https://github.com/example/sample-app",
                        "path": "manifests",
                        "targetRevision": "HEAD"
                    }
                }
            },
            "health": {
                "status": "Healthy"
            },
            "operationState": {
                "phase": "Succeeded",
                "message": "successfully synced"
            }
        }
    }

    # Sample application 2
    applications["test-app"] = {
        "metadata": {
            "name": "test-app",
            "namespace": "argocd",
            "uid": "87654321-4321-4321-4321-cba987654321",
            "creationTimestamp": "2024-01-02T00:00:00Z"
        },
        "spec": {
            "source": {
                "repoURL": "https://github.com/example/test-app",
                "path": "k8s",
                "targetRevision": "main"
            },
            "destination": {
                "server": "https://kubernetes.default.svc",
                "namespace": "test"
            },
            "project": "default",
            "syncPolicy": {
                "automated": {
                    "prune": True,
                    "selfHeal": True
                }
            }
        },
        "status": {
            "sync": {
                "status": "OutOfSync",
                "comparedTo": {
                    "source": {
                        "repoURL": "https://github.com/example/test-app",
                        "path": "k8s",
                        "targetRevision": "main"
                    }
                }
            },
            "health": {
                "status": "Progressing"
            },
            "operationState": {
                "phase": "Running",
                "message": "sync in progress"
            }
        }
    }

    logger.info(f"Loaded {len(applications)} mock applications: {list(applications.keys())}")


# Dependency for authentication
async def verify_token(authorization: Optional[str] = Header(None)):
    """Verify authorization header"""
    if not authorization:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Missing Authorization header"
        )

    if not authorization.startswith('Bearer '):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid Authorization header format"
        )

    token = authorization.replace('Bearer ', '')

    if auth_token is None:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="No active session. Please create a session first."
        )

    if token != auth_token:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid token"
        )

    return token


@app.post(
    "/api/v1/session",
    response_model=SessionResponse,
    summary="Create session and generate auth token",
    description="""
    Create a new session and get an authentication token.

    Example:
    ```bash
    curl -k -X POST \\
      -H "Content-Type: application/json" \\
      -d '{"username":"admin","password":"password"}' \\
      http://localhost:8080/api/v1/session
    ```
    """
)
async def create_session(request: SessionRequest):
    global auth_token

    try:
        logger.info(f"Create session request: username={request.username}")

        # Generate mock token
        auth_token = f"mock_token_{int(time.time())}"

        response = SessionResponse(
            token=auth_token,
            username=request.username,
            iat=int(time.time()),
            exp=int(time.time()) + 86400  # 24 hours
        )

        logger.info(f"Session created successfully, token={auth_token}")
        return response

    except Exception as e:
        logger.error(f"Error creating session: {e}")
        raise HTTPException(status_code=500, detail=str(e))


@app.get(
    "/api/v1/applications",
    summary="List all applications",
    description="""
    Get a list of all ArgoCD applications.

    Example:
    ```bash
    curl -sk -H "Authorization: Bearer $TOKEN" \\
      http://localhost:8080/api/v1/applications
    ```
    """
)
async def list_applications(token: str = Depends(verify_token)):
    logger.info(f"List applications: {len(applications)} apps")

    response = {
        "metadata": {
            "resourceVersion": "1"
        },
        "items": list(applications.values())
    }

    return JSONResponse(content=response)


@app.get(
    "/api/v1/applications/{app_name}",
    summary="Get specific application",
    description="""
    Get detailed information about a specific ArgoCD application.

    Example:
    ```bash
    curl -sk -H "Authorization: Bearer $TOKEN" \\
      http://localhost:8080/api/v1/applications/sample-app
    ```
    """
)
async def get_application(app_name: str, token: str = Depends(verify_token)):
    if app_name not in applications:
        logger.warning(f"Application not found: {app_name}")
        raise HTTPException(
            status_code=404,
            detail={"error": "Application not found", "code": 5}
        )

    logger.info(f"Get application: {app_name}")
    return JSONResponse(content=applications[app_name])


@app.patch(
    "/api/v1/applications/{app_name}",
    summary="Update application",
    description="""
    Update application configuration (typically for Auto Sync settings).

    **Enable Auto Sync:**
    ```bash
    curl -sk -X PATCH \\
      -H "Authorization: Bearer $TOKEN" \\
      -H "Content-Type: application/json" \\
      -d '{
        "patch": "{\\"spec\\":{\\"syncPolicy\\":{\\"automated\\":{\\"prune\\":true,\\"selfHeal\\":true}}}}",
        "patchType": "merge"
      }' \\
      http://localhost:8080/api/v1/applications/sample-app
    ```

    **Disable Auto Sync:**
    ```bash
    curl -sk -X PATCH \\
      -H "Authorization: Bearer $TOKEN" \\
      -H "Content-Type: application/json" \\
      -d '{
        "patch": "{\\"spec\\":{\\"syncPolicy\\":null}}",
        "patchType": "merge"
      }' \\
      http://localhost:8080/api/v1/applications/sample-app
    ```
    """
)
async def patch_application(
    app_name: str,
    request: PatchRequest,
    token: str = Depends(verify_token)
):
    if app_name not in applications:
        logger.warning(f"Application not found: {app_name}")
        raise HTTPException(status_code=404, detail="Application not found")

    try:
        logger.info(f"PATCH application: {app_name}, patchType={request.patchType}")
        logger.debug(f"Patch content: {request.patch}")

        if request.patch:
            patch_data = json.loads(request.patch)

            # Apply patch to application
            if 'spec' in patch_data:
                if 'syncPolicy' in patch_data['spec']:
                    applications[app_name]['spec']['syncPolicy'] = patch_data['spec']['syncPolicy']

                    if patch_data['spec']['syncPolicy'] is None:
                        logger.info(f"Auto Sync disabled for {app_name}")
                    else:
                        logger.info(f"Auto Sync enabled for {app_name}: {patch_data['spec']['syncPolicy']}")

        return JSONResponse(content=applications[app_name])

    except json.JSONDecodeError as e:
        logger.error(f"Invalid JSON in patch: {e}")
        raise HTTPException(status_code=400, detail=f"Invalid JSON in patch: {str(e)}")
    except Exception as e:
        logger.error(f"Error applying patch: {e}")
        raise HTTPException(status_code=500, detail=f"Failed to apply patch: {str(e)}")


@app.post(
    "/api/v1/applications/{app_name}/sync",
    response_model=SyncResponse,
    summary="Trigger application sync",
    description="""
    Trigger a sync operation for the specified application.

    Example:
    ```bash
    curl -sk -X POST \\
      -H "Authorization: Bearer $TOKEN" \\
      -H "Content-Type: application/json" \\
      -d '{}' \\
      http://localhost:8080/api/v1/applications/sample-app/sync
    ```
    """
)
async def sync_application(app_name: str, token: str = Depends(verify_token)):
    if app_name not in applications:
        logger.warning(f"Application not found: {app_name}")
        raise HTTPException(status_code=404, detail="Application not found")

    logger.info(f"Triggering sync for application: {app_name}")

    # Update application status to Running
    applications[app_name]['status']['operationState']['phase'] = 'Running'
    applications[app_name]['status']['operationState']['message'] = 'sync operation initiated'
    applications[app_name]['status']['sync']['status'] = 'OutOfSync'

    response = SyncResponse(
        status="success",
        message="Sync operation started",
        application=app_name,
        timestamp=time.time()
    )

    return response


@app.get(
    "/health",
    response_model=HealthResponse,
    summary="Health check endpoint",
    description="Check the health status of the mock server"
)
async def health_check():
    return HealthResponse(
        status="healthy",
        applications=len(applications),
        authenticated=auth_token is not None
    )


@app.on_event("startup")
async def startup_event():
    """Load mock data on startup"""
    load_mock_data()
    logger.info("Mock ArgoCD Server started successfully")


def main():
    parser = argparse.ArgumentParser(description='Mock ArgoCD API Server (FastAPI)')
    parser.add_argument(
        '-p', '--port',
        type=int,
        default=8080,
        help='Port to run the server on (default: 8080)'
    )
    parser.add_argument(
        '-H', '--host',
        type=str,
        default='0.0.0.0',
        help='Host to bind to (default: 0.0.0.0)'
    )
    parser.add_argument(
        '-d', '--debug',
        action='store_true',
        help='Enable debug mode'
    )
    parser.add_argument(
        '--reload',
        action='store_true',
        help='Enable auto-reload on code changes'
    )

    args = parser.parse_args()

    # Set logging level
    if args.debug:
        logger.setLevel(logging.DEBUG)

    # Update app servers with the actual port
    local_ip = get_local_ip()
    app.servers = [
        {"url": f"http://{local_ip}:{args.port}", "description": "Local IP Server"},
        {"url": f"http://localhost:{args.port}", "description": "Localhost"},
        {"url": f"http://127.0.0.1:{args.port}", "description": "Loopback"},
    ]

    print("=" * 70)
    print("Mock ArgoCD API Server (FastAPI)")
    print("=" * 70)
    print(f"Host: {args.host}")
    print(f"Port: {args.port}")
    print(f"Local IP: {local_ip}")
    print(f"Debug: {args.debug}")
    print(f"Reload: {args.reload}")
    print()
    print("Access URLs:")
    print(f"  Local:   http://localhost:{args.port}")
    print(f"  Network: http://{local_ip}:{args.port}")
    if args.host == '0.0.0.0':
        print(f"  External IP connections enabled (listening on all interfaces)")
    print()
    print("Available endpoints:")
    print("  POST   /api/v1/session                        - Create session")
    print("  GET    /api/v1/applications                   - List applications")
    print("  GET    /api/v1/applications/{app_name}        - Get application")
    print("  PATCH  /api/v1/applications/{app_name}        - Update application")
    print("  POST   /api/v1/applications/{app_name}/sync   - Trigger sync")
    print("  GET    /health                                - Health check")
    print()
    print("API Documentation:")
    print(f"  Swagger UI: http://localhost:{args.port}/docs")
    print(f"              http://{local_ip}:{args.port}/docs")
    print(f"  ReDoc:      http://localhost:{args.port}/redoc")
    print(f"              http://{local_ip}:{args.port}/redoc")
    print()
    print("Mock applications will be loaded on startup:")
    print("  - sample-app (Synced, Auto Sync: disabled)")
    print("  - test-app   (OutOfSync, Auto Sync: enabled)")
    print()
    print("Press Ctrl+C to stop the server...")
    print("=" * 70)
    print()

    # Run Uvicorn server
    uvicorn.run(
        "mock_argocd_server:app",
        host=args.host,
        port=args.port,
        reload=args.reload,
        log_level="debug" if args.debug else "info"
    )


if __name__ == '__main__':
    main()

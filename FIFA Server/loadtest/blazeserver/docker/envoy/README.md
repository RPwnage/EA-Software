# Blaze in the EADP Service Mesh

This directory contains scripts to run the Envoy containers for various Service Mesh local development setups.

## Test overview

Background info: see [Blaze in Service Mesh Architecture](https://developer.ea.com/display/TEAMS/Blaze+in+Service+Mesh+Architecture)

### Server

- A [Redis Cluster](https://developer.ea.com/display/blaze/How+to+Use+Redis+in+Cluster+Mode) is started with 3 nodes running in a single Docker container.
- Each Blaze server instance publishes its own endpoint info into Redis. This is done for each endpoint with `envoyAccessEnabled = true` (config defaults to false).
    - The Blaze instance endpoint info is sharded across all 3 Redis nodes.
    - The endpoint info has a Redis TTL that causes it to expire after an instance shuts down (or crashes).
- The [Blaze Control Plane](https://gitlab.ea.com/eadp-gameplay-services/blaze-support/blazecontrolplane) starts a gRPC server listening on port 9090 for the [Envoy Endpoint Discover Service](https://www.envoyproxy.io/docs/envoy/latest/intro/arch_overview/upstream/service_discovery#endpoint-discovery-service-eds) (EDS) API.
    - The Blaze Control Plane reads all of the Blaze server instance endpoint data from Redis. That data is provided to Envoy via EDS gRPC requests.
    - The Blaze Control Plane uses the info in the generated `redis_cfg.json` config file to connect to Redis.
        - Alternatively, the generated `redis.cfg` Blaze config file format is also supported (via the `redisCfgFile` command line parameter).
- The ingress [Envoy Proxy](https://gitlab.ea.com/eadp-gameplay-services/blaze-support/envoy-blaze-ingress) nodes handle all incoming Blaze Service Mesh requests.
    - Each ingress Envoy container has a `eadp.mesh.endpoints` [Docker label](https://docs.docker.com/config/labels-custom-metadata/) for use by the XDS Docker Registration Agent.
        - The label indicates the Mesh Namespace (`eadp.edge`), the Mesh Service (`eadp.blaze`), the ingress port number (`10002`), and the ingress Server Group (`gosblaze-johndoe-common`).
    - Each ingress Envoy issues a EDS streaming gRPC request to the Blaze Control Plane to continuously retrieve (and cache) the latest Blaze server endpoint info.
    - When the ingress Envoy receives a mesh client request *without* a user session key, a random coreSlave is used to handle the request.
    - When the ingress Envoy receives a mesh client request *with* a user session key, the sliver id (embedded in the user session key) is used to route the request to the correct coreSlave instance.
    - mTLS is required for all mesh client requests.
- The [XDS Docker Registration Agent](https://gitlab.ea.com/eadp-service-mesh/docker-registration-agent) scans the Docker labels for mesh endpoints to register with the S2S Service Edge [Mesh Control Plane](https://gitlab.ea.com/eadp-service-mesh/control-plane) (the Envoy XDS server).
    - The `NODE_ADDRESS` environment variable specifies the IP address of the Blaze ingress Envoy nodes on this host. This IP address is registered in the edge mesh; it is the IP address that mesh clients will send requests to.
    - The `eadp.edge` mesh registration request is routed to the XDS server through the egress Envoy Mesh Node's `ENVOY_LOCAL_XDS_LISTENER_PORT` (port 9001).
    - Note: A similar but different registration agent will be used for Blaze in the Cloud (BitC).
- The egress [Envoy Mesh Node](https://gitlab.ea.com/eadp-service-mesh/envoy-mesh-node) is **currently** only used to route endpoint registration requests to the edge mesh's XDS server.
    - *Someday* the egress could be used to route gRPC requests from the Blaze server to other EADP mesh services (like EADP Stats).
    - Note: BitC deployments will not use an egress Envoy Mesh Node for mesh registration, and thus will not initially include this container.

### Client

- The [testblazeservicemesh](https://gitlab.ea.com/eadp-gameplay-services/blaze-support/testblazeservicemesh) is a Go-based implementation of a simple gRPC client for testing basic Blaze in the EADP Service Mesh functionality.
    - expressLogin and a random authenticated RPC (i.e. authentication/getAccount) are used to verify stateful RPC routing by sliver id (via user session key).
    - trustedLogin is also verified.
    - Each Blaze gRPC request is routed to the Blaze ingress Envoy Proxy through the local egress Envoy Mesh Node's port 10000.
    - The request is sent "insecure" to the local Envoy Mesh Node without using SSL/TLS.
- The egress [Envoy Mesh Node](https://gitlab.ea.com/eadp-service-mesh/envoy-mesh-node) is used to route Blaze gRPC requests to the Blaze ingress Envoy Proxy.
    - The endpoint info (IP address and port number) for the Blaze ingress Envoy Proxy is discovered from the S2S Service Edge [Mesh Control Plane](https://gitlab.ea.com/eadp-service-mesh/control-plane) (the Envoy XDS server).
    - The EGRESS_SERVERSPACE environment variable determines the Blaze server group (`gosblaze-johndoe-common`) to send the request to.
    - The mesh client's egress node handles the mTLS with the Blaze server's ingress node.
    - Envoy handles the load balancing of determining which of the Blaze ingress Envoy Proxy nodes to use.

## Running the service mesh test

### Common prep

1. Edit blazeserver/etc/framework/endpoints.cfg

    Change `enableServiceMeshAccess` from `false` to `true` for the `internalGrpcSecureEnvoy` endpoint.
    ```c++
    grpcEndpointTypes = {
        ...
        internalGrpcSecureEnvoy = {
            ...
            envoyAccessEnabled = false
            // SHA 256 digest of each ENVOY_CLIENT_CERT_PUBLIC_KEY
            envoyXfccTrustedHashes = [ ]
    ```

    Add the following value to the envoyXfccTrustedHashes for the `internalGrpcSecureEnvoy` endpoint.
    ```c++
    grpcEndpointTypes = {
        ...
        internalGrpcSecureEnvoy = {
            ...
            envoyAccessEnabled = true
            // SHA 256 digest of each ENVOY_CLIENT_CERT_PUBLIC_KEY
            envoyXfccTrustedHashes = [ "2f516409649ddb3617d68088f44cc7346f8e6d29cd536381c76b5799d9dc51b6" ]
    ```

    Note that the hash was generated by running the following on the ENVOY_CLIENT_CERT_PUBLIC_KEY file
    listed in the blazeserver/docker/envoy/meshclient/docker-compose.yml file:
    ```sh
    cd blazeserver/etc/ssl
    openssl x509 -in nucleus/integration-cert.crt -outform DER | openssl dgst -sha256 | cut -d" " -f2
    ```

1. Edit blazeserver/docker/envoy/docker-compose.envoy.yml

    Change both occurrences of `gosblaze-johndoe-common` to your Blaze server's default service name.
    (Change it for both ingress nodes.)
    ```yaml
      # TODO: Put your Blaze server's default service name here
      - 'eadp.mesh.endpoints={"namespaces":{ "eadp.edge": {"registrations": [{"service":"eadp.blaze", "port_spec":"10002/tcp", "tags":{ "x-eadp-servergroup":"gosblaze-johndoe-common" } }] } }}'
    ...
      # TODO: Put your Blaze server's default service name here
      - 'eadp.mesh.endpoints={"namespaces":{ "eadp.edge": {"registrations": [{"service":"eadp.blaze", "port_spec":"10005/tcp", "tags":{ "x-eadp-servergroup":"gosblaze-johndoe-common" } }] } }}'
    ```

    Change the `NODE_ADDRESS` to the IP address of your Linux VM / WSL2 host.
    ```yaml
      # TODO: Put your Blaze server's externally accessible IP address here
      NODE_ADDRESS: 10.8.911.911
    ```

1. Edit blazeserver/docker/envoy/meshclient/docker-compose.yml

    Change `gosblaze-johndoe-common` to your Blaze server's default service name.
    ```yaml
      # TODO: Put your Blaze server's default service name here
      EGRESS_SERVERSPACE: gosblaze-johndoe-common
    ```

    Change `gosblaze-johndoe-pc` to your Blaze server's authentication product name for PC (i.e your PC service name).
    ```yaml
      # TODO: Put your Blaze server's PC product name here (not the common platform!)
      - -product_name=gosblaze-johndoe-pc
    ```

### Running blazeserver in a Docker container

1. Set up your Linux VM: https://developer.ea.com/display/blaze/Blaze+Linux+VM+Setup
   Alternatively, you can run the blazeserver Docker container using WSL2: https://developer.ea.com/display/blaze/Blaze+WSL2+Setup

1. Install Docker Compose (if using a Linux VM): https://developer.ea.com/display/blaze/Docker+Compose+and+Blazeserver

1. Build the Blaze server: https://developer.ea.com/display/blaze/Building+and+Running+the+Blaze+Server+in+Docker+-+Urraca

1. Run all of the Docker containers:

    ```sh
    cd blazeserver/docker/envoy/
    sh ./run_all.sh
    ```

1. After blazeserver is in-service (check blazeserver/init/logs/blazeserver.log), run the mesh client test in a separate terminal:

    ```sh
    cd blazeserver/docker/envoy/meshclient
    docker-compose up
    ```

### Running blazeserver in Visual Studio

1. You will run everything except the Blaze server using Docker Desktop for Windows: https://developer.ea.com/display/blaze/Blaze+WSL2+Setup

1. In Windows PowerShell, run all of the Docker containers except Blaze:

    ```sh
    cd blazeserver/docker/envoy/
    sh ./run_without_blaze.sh
    ```

    **Note:** this starts Redis Cluster in Docker.

1. Start blazeserver in Visual Studio by right-clicking on the blazeserver project and using `Debug` -> `Start new instance`

    **Important:** Do *not* start blazeredis via F5 / `Start Debugging` (because Redis Cluster is already running in Docker via the step above).

    (You could also modify the solution to not startup blazeredis, but you might want that re-enabled later.)

1. After blazeserver is in-service (check the blazeserver logs), run the test client in a separate Windows PowerShell terminal:

    ```sh
    cd blazeserver/docker/envoy/meshclient
    docker-compose up
    ```

### Game Team Branch Troubleshooting

The [testblazeservicemesh](https://gitlab.ea.com/eadp-gameplay-services/blaze-support/testblazeservicemesh) Blaze gRPC client is focused on testing for GS Blaze server development.
You will likely run into various Nucleus authentication hurdles if trying to run the test in a game team branch (i.e. not using the default development-only Blaze server Nucleus client ids).

For those cases, you will likely want to download the testblazeservicemesh project locally and modify it.
(You will need to modify `blazeserver\docker\envoy\meshclient\docker-compose.yml` to reference your local Docker image instead of downloading the stock container image.)

- The client's `expressLogin` code (in `blazeauthentication.go`) is only setup to authenticate the `nexus001@ea.com` PC test account.
    As a result, your local Blaze server must be hosting the PC platform, either as a single platform or by using a [Shared Platform Deployment](https://developer.ea.com/display/blaze/Shared+Platform+Deployment+Guide) common platform.
    If hosting the PC platform on your game team's Blaze server is not possible, then the test client code can be downloaded locally and modified to use a different Nucleus account.

- If `expressLogin` fails the `ONLINE_ACCESS` entitlement check, you may need to tempoarily set `autoEntitlementEnabled` to `true` in `products_pc.cfg` / `product.cfg` or use an account entitled for the game. 

- The `GOS-BlazeServer-DEV` Nucleus client id used for trustedLogin by the testblazeservicemesh [testblazeservicemesh](https://gitlab.ea.com/eadp-gameplay-services/blaze-support/testblazeservicemesh) likely will not work with game-specific Blaze server Nucleus client ids.

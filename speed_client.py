import asyncio
from someipy import (
    ServiceBuilder,
    Method,
    ClientServiceInstance,
    TransportLayerProtocol,
    MessageType,
    ReturnCode,
    connect_to_someipy_daemon,
)

SPEED_SERVICE_ID = 0x1234
SPEED_INSTANCE_ID = 0x5678
GET_SPEED_METHOD_ID = 0x0421
INTERFACE_IP = "100.88.162.130"
CLIENT_PORT = 3001

async def main():
    print("Connecting to someipy daemon...")
    someipy_daemon = await connect_to_someipy_daemon()
    print("Connected! Waiting for Speed ECU...")

    get_speed_method = Method(
        id=GET_SPEED_METHOD_ID,
        protocol=TransportLayerProtocol.UDP,
    )

    speed_service = (
        ServiceBuilder()
        .with_service_id(SPEED_SERVICE_ID)
        .with_major_version(1)
        .with_method(get_speed_method)
        .build()
    )

    speed_client = ClientServiceInstance(
        daemon=someipy_daemon,
        service=speed_service,
        instance_id=SPEED_INSTANCE_ID,
        endpoint_ip=INTERFACE_IP,
        endpoint_port=CLIENT_PORT,
    )

    # Wait for service discovery
    await asyncio.sleep(3)

    print("Calling GetVehicleSpeed()...")
    try:
        method_result = await speed_client.call_method(GET_SPEED_METHOD_ID, bytes())
        if method_result.message_type == MessageType.RESPONSE and method_result.return_code == ReturnCode.E_OK:
            speed = method_result.payload[0]
            print(f"Vehicle speed received: {speed} km/h")
        else:
            print(f"Unexpected response: {method_result}")
    except Exception as e:
        print(f"Error: {e}")

    await someipy_daemon.disconnect_from_daemon()

if __name__ == "__main__":
    asyncio.run(main())

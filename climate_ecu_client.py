import asyncio

from someipy import (
    ServiceBuilder,
    Method,
    TransportLayerProtocol,
    ClientServiceInstance,
    connect_to_someipy_daemon,
)

SPEED_SERVICE_ID = 0x1234
SPEED_INSTANCE_ID = 0x5678
GET_SPEED_METHOD_ID = 0x0421

CLIENT_IP = "127.0.0.1"
CLIENT_PORT = 3001


async def main():
    someipy_daemon = await connect_to_someipy_daemon()

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

    climate_ecu = ClientServiceInstance(
        daemon=someipy_daemon,
        service=speed_service,
        instance_id=SPEED_INSTANCE_ID,
        endpoint_ip=CLIENT_IP,
        endpoint_port=CLIENT_PORT,
    )

    print("Climate ECU client started")
    await asyncio.sleep(2)

    print("Sending GetVehicleSpeed() request...")

    result = await climate_ecu.call_method(
        method_id=GET_SPEED_METHOD_ID,
        payload=b"",
    )

    vehicle_speed = result.payload[0]
    print(f"Vehicle speed received: {vehicle_speed} km/h")
    print("RPC communication successful")

    try:
        await someipy_daemon.disconnect_from_daemon()
    except Exception:
        pass


if __name__ == "__main__":
    asyncio.run(main())

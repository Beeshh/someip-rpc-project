import asyncio
from typing import Tuple

from someipy import (
    ServiceBuilder,
    Method,
    MethodResult,
    ReturnCode,
    MessageType,
    TransportLayerProtocol,
    ServerServiceInstance,
    connect_to_someipy_daemon,
)

SPEED_SERVICE_ID = 0x1234
SPEED_INSTANCE_ID = 0x5678
GET_SPEED_METHOD_ID = 0x0421

INTERFACE_IP = "127.0.0.1"
SPEED_ECU_PORT = 3000


async def get_vehicle_speed_handler(payload: bytes, addr: Tuple[str, int]) -> MethodResult:
    print(f"GetVehicleSpeed() request received from IP: {addr[0]} Port: {addr[1]}")

    vehicle_speed_kmh = 60
    response_payload = bytes([vehicle_speed_kmh])

    print(f"Sending vehicle speed: {vehicle_speed_kmh} km/h")

    result = MethodResult()
    result.message_type = MessageType.RESPONSE
    result.return_code = ReturnCode.E_OK
    result.payload = response_payload
    return result


async def main():
    someipy_daemon = await connect_to_someipy_daemon()

    get_speed_method = Method(
        id=GET_SPEED_METHOD_ID,
        protocol=TransportLayerProtocol.UDP,
        method_handler=get_vehicle_speed_handler,
    )

    speed_service = (
        ServiceBuilder()
        .with_service_id(SPEED_SERVICE_ID)
        .with_major_version(1)
        .with_method(get_speed_method)
        .build()
    )

    speed_ecu = ServerServiceInstance(
        daemon=someipy_daemon,
        service=speed_service,
        instance_id=SPEED_INSTANCE_ID,
        endpoint_ip=INTERFACE_IP,
        endpoint_port=SPEED_ECU_PORT,
        ttl=5,
        cyclic_offer_delay_ms=2000,
    )

    print("Speed ECU provider started")
    print("Offering GetVehicleSpeed() RPC service...")

    await speed_ecu.start_offer()

    try:
        while True:
            await asyncio.sleep(1)
    finally:
        await speed_ecu.stop_offer()
        await someipy_daemon.disconnect_from_daemon()


if __name__ == "__main__":
    asyncio.run(main())

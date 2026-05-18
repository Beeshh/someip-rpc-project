# Speed ECU RPC Test

Small test for SOME/IP RPC using Python and someipy.

The idea is to have a Speed ECU that provides a simple method:

GetVehicleSpeed()

The Climate ECU calls this method and receives one speed value.

## Files

- speed_ecu_provider.py
- climate_ecu_client.py
- someipyd.json

## IDs used for the test

- Service ID: 0x1234
- Instance ID: 0x5678
- Method ID: 0x0421

## Payload

For now the response is only one byte.

Example:

60 km/h

## Current status

The local test in WSL is running.

The provider receives the request and sends back the speed value.

Current output:

Vehicle speed received: 60 km/h

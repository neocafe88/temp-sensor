/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include <aws/crt/Api.h>
#include <aws/crt/UUID.h>

#include "CommandLineUtils.h"

using namespace std;
using namespace Aws::Crt; // CRT = AWS Common Runtime, basis library of SDKs
using namespace Aws::Iot;

// set up a connection config from command line arguments
//
MqttClientConnectionConfig buildClientConnectionConfig(
		Aws::Crt::String &input_endpoint,
		Aws::Crt::String &input_cert,
		Aws::Crt::String &input_key,
		Aws::Crt::String &input_ca) {
	
    // Create the MQTT builder and populate it with data from cmdData.
    auto configBuilder = MqttClientConnectionConfigBuilder(
		    			input_cert.c_str(), // --cert
		    			input_key.c_str()); // --key

    configBuilder.WithEndpoint(input_endpoint);               // --endpoint
    configBuilder.WithCertificateAuthority(input_ca.c_str()); // --ca_file

    // Create the MQTT connection from the MQTT builder
    auto config = configBuilder.Build();

    if (!config)
    {
        fprintf(
            stderr,
            "Client Configuration initialization failed with error %s\n",
            Aws::Crt::ErrorDebugString(config.LastError()));
    }

    return config;
}

// Set up promises for MQTT connection events 
//
void linkConnectionEvents(std::shared_ptr< Mqtt::MqttConnection >& connection, 
		promise<bool> &promiseConnected, promise<void>& promiseClosed) {

    // Invoked when a MQTT connect has completed or failed
    auto onConnectionCompleted =
        [&](Mqtt::MqttConnection &, int errorCode, Mqtt::ReturnCode returnCode, bool) {
            if (errorCode)
            {
                fprintf(stdout, "Connection failed with error %s\n", Aws::Crt::ErrorDebugString(errorCode));
                promiseConnected.set_value(false);
            }
            else
            {
                fprintf(stdout, "Connection completed with return code %d\n", returnCode);
                promiseConnected.set_value(true);
            }
        };
    
    // Invoked when a MQTT connection was interrupted/lost
    auto onInterrupted = [&](Mqtt::MqttConnection &, int error) {
        fprintf(stdout, "Connection interrupted with error %s\n", Aws::Crt::ErrorDebugString(error));
    };

    // Invoked when a MQTT connection was interrupted/lost, but then reconnected successfully
    auto onResumed = [&](Mqtt::MqttConnection &, Mqtt::ReturnCode, bool) {
        fprintf(stdout, "Connection resumed\n");
    };

    // Invoked when a disconnect message has completed.
    auto onDisconnect = [&](Mqtt::MqttConnection &) {
        fprintf(stdout, "Disconnect completed\n");
        promiseClosed.set_value();
    };

    // Assign callbacks
    connection->OnConnectionCompleted   = std::move(onConnectionCompleted);
    connection->OnDisconnect            = std::move(onDisconnect);
    connection->OnConnectionInterrupted = std::move(onInterrupted);
    connection->OnConnectionResumed     = std::move(onResumed);
}

class MqttHandler {

    public:

        MqttHandler(Utils::cmdData& cmdData) 
	    : _clientId{cmdData.input_clientId}
	{ 
	    auto config = buildClientConnectionConfig(
			    cmdData.input_endpoint,
			    cmdData.input_cert,
			    cmdData.input_key,
			    cmdData.input_ca);

	    _connection = _client.NewConnection(config);
    	    linkConnectionEvents(_connection, _promiseConnected, _promiseClosed);
	} 

	bool isReady() {
            return !!*_connection; 
	}

	bool isConnected() {
	    return _promiseConnected.get_future().get();
	}

	void wait() {
           _promiseClosed.get_future().wait();
	}

	bool connect() {
    	    return _connection->Connect(_clientId.c_str(), // --client_id 
			    false, // cleanSession
			    1000   // keepAliveTimeSecs
			    );
	}

	bool disconnect() {
            return _connection->Disconnect();
	}

	const char* lastError() {
	    return Aws::Crt::ErrorDebugString(_connection->LastError());
	}

	void publish(Aws::Crt::String& topic, Aws::Crt::String &msg) {

            auto onPublishComplete = [](Mqtt::MqttConnection &, uint16_t, int) {
    		fprintf(stdout, "Published successfully...\n");
	    };

            Aws::Crt::ByteBuf payload = ByteBufFromArray((const uint8_t *)msg.data(), msg.length());

            _connection->Publish(topic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, false, 
			    payload, onPublishComplete);
	}

    private:

        MqttClient _client;
	std::shared_ptr<Mqtt::MqttConnection> _connection {nullptr};
    	promise<bool> _promiseConnected; // std::
    	promise<void> _promiseClosed;    // 
	Aws::Crt::String _clientId;
};

int main(int argc, char *argv[])
{

    // Do the global initialization for the API
    // the init/cleanup state of the entire CRT
    ApiHandle apiHandle; 

    // command line arguments to a structure
    Utils::cmdData cmdData = Utils::parseSampleInputBasicConnect(argc, argv, &apiHandle);

    MqttHandler mqtt{cmdData};

    if (!mqtt.isReady()) {
        fprintf(stderr, "MQTT Connection Creation failed with error %s\n", mqtt.lastError());
        exit(-1);
    }

    // Connect
    fprintf(stdout, "Connecting...******\n");

    if (!mqtt.connect()) 
    {
        fprintf(stderr, "MQTT Connection failed with error %s\n", mqtt.lastError());
        exit(-1);
    }

    // wait for the OnConnectionCompleted callback to fire, which sets promiseConnected...
    if (mqtt.isConnected() == false)
    {
        fprintf(stderr, "Connection failed\n");
        exit(-1);
    }

    /********** Publish *********/

    /*
    Aws::Crt::String msg = "\"test message\"";
    Aws::Crt::ByteBuf payload = ByteBufFromArray((const uint8_t *)msg.data(), msg.length());
    */
    Aws::Crt::String topic{"sdk/test/temp"};

    fprintf(stdout, "Reading from the sensor...\n");
    
    ifstream sensor;
    string t;

    sensor.open("/dev/tempdriver");

    if (sensor.is_open()) {
	getline(sensor, t);

        Aws::Crt::String msg = t.c_str();

	mqtt.publish(topic, msg);
    } else {
        Aws::Crt::String msg{"No data read"};
        mqtt.publish(topic, msg);
    }


    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    /****** end of Publish ******/

    // Disconnect
    if (mqtt.disconnect())
    {
        mqtt.wait();
    }

    return 0;
}

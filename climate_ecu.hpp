// Copyright (C) 2014-2026 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <vsomeip/vsomeip.hpp>
#include <cstdio>

#define LOG_INF(...) fprintf(stdout, __VA_ARGS__), fprintf(stdout, "\n")
#define LOG_ERR(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")

// Window ECU identifiers
static vsomeip::service_t   window_service_id   = 0x1111;
static vsomeip::instance_t  window_instance_id  = 0x2222;
static vsomeip::method_t    window_method_id    = 0x3333;

// Speed ECU identifiers
static vsomeip::service_t   speed_service_id    = 0x1234;
static vsomeip::instance_t  speed_instance_id   = 0x5678;
static vsomeip::method_t    speed_method_id     = 0x0421;

class hello_world_client {
public:
    hello_world_client()
        : rtm_(vsomeip::runtime::get()),
          app_(rtm_->create_application()),
          window_position_(0), vehicle_speed_(0),
          window_received_(false), speed_received_(false) {}

    bool init() {
        if (!app_->init()) {
            LOG_ERR("Couldn't initialize application");
            return false;
        }

        app_->register_state_handler(
            std::bind(&hello_world_client::on_state_cbk, this,
                      std::placeholders::_1));

        // Handle responses from BOTH services
        app_->register_message_handler(
            vsomeip::ANY_SERVICE, vsomeip::ANY_INSTANCE, vsomeip::ANY_METHOD,
            std::bind(&hello_world_client::on_message_cbk, this,
                      std::placeholders::_1));

        // Watch availability of BOTH services
        app_->register_availability_handler(
            window_service_id, window_instance_id,
            std::bind(&hello_world_client::on_availability_cbk, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3));

        app_->register_availability_handler(
            speed_service_id, speed_instance_id,
            std::bind(&hello_world_client::on_availability_cbk, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3));

        return true;
    }

    void start() { app_->start(); }

    void on_state_cbk(vsomeip::state_type_e _state) {
        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            app_->request_service(window_service_id, window_instance_id);
            app_->request_service(speed_service_id,  speed_instance_id);
        }
    }

    void on_availability_cbk(vsomeip::service_t _service,
                             vsomeip::instance_t _instance,
                             bool _is_available) {
        if (!_is_available) return;

        // Send request to whichever service just became available
        std::shared_ptr<vsomeip::message> rq = rtm_->create_request();

        if (_service == window_service_id && _instance == window_instance_id) {
            rq->set_service(window_service_id);
            rq->set_instance(window_instance_id);
            rq->set_method(window_method_id);
            LOG_INF("Requesting window position from Window ECU...");
        } else if (_service == speed_service_id && _instance == speed_instance_id) {
            rq->set_service(speed_service_id);
            rq->set_instance(speed_instance_id);
            rq->set_method(speed_method_id);
            LOG_INF("Requesting vehicle speed from Speed ECU...");
        } else {
            return;
        }

        // Empty payload (just a request, no data needed)
        std::shared_ptr<vsomeip::payload> pl = rtm_->create_payload();
        rq->set_payload(pl);
        app_->send(rq);
    }

    void on_message_cbk(const std::shared_ptr<vsomeip::message>& _response) {
        if (_response->get_message_type() != vsomeip::message_type_e::MT_RESPONSE)
            return;
        if (_response->get_return_code() != vsomeip::return_code_e::E_OK)
            return;

        std::shared_ptr<vsomeip::payload> pl = _response->get_payload();
        if (!pl || pl->get_length() == 0) return;

        vsomeip::service_t svc = _response->get_service();

        if (svc == window_service_id) {
            window_position_ = pl->get_data()[0];
            window_received_ = true;
            LOG_INF("Window position received: %d %%",
                    static_cast<int>(window_position_));
        } else if (svc == speed_service_id) {
            vehicle_speed_ = pl->get_data()[0];
            speed_received_ = true;
            LOG_INF("Vehicle speed received: %d km/h",
                    static_cast<int>(vehicle_speed_));
        }

        // Only print mode once both values are received
        if (window_received_ && speed_received_) {
            evaluate_and_print();
            stop();
        }
    }

    void evaluate_and_print() {
        // Determine mode from window position (matches Pub/Sub logic)
        const char* mode;
        const char* speed_range;
        if (window_position_ >= 80) {
            mode = "LOW";
            speed_range = "0-30 km/h";
        } else if (window_position_ >= 50) {
            mode = "MEDIUM";
            speed_range = "31-60 km/h";
        } else if (window_position_ >= 20) {
            mode = "HIGH";
            speed_range = "61-90 km/h";
        } else {
            mode = "FAST";
            speed_range = "91-120 km/h";
        }
        LOG_INF("Climate ECU: Window=%d%% | Vehicle Speed=%d km/h (%s)",
                static_cast<int>(window_position_),
                static_cast<int>(vehicle_speed_),
                mode);
    }

    void stop() {
        app_->unregister_state_handler();
        app_->clear_all_handler();
        app_->release_service(window_service_id, window_instance_id);
        app_->release_service(speed_service_id,  speed_instance_id);
        app_->stop();
    }

private:
    std::shared_ptr<vsomeip::runtime>     rtm_;
    std::shared_ptr<vsomeip::application> app_;
    uint8_t window_position_;
    uint8_t vehicle_speed_;
    bool    window_received_;
    bool    speed_received_;
};

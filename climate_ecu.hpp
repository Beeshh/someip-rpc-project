// Copyright (C) 2014-2026 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <vsomeip/vsomeip.hpp>
#include <cstdio>

#if defined ANDROID || defined __ANDROID__
#include "android/log.h"
#define LOG_TAG "hello_world_client"
#define LOG_INF(...) fprintf(stdout, __VA_ARGS__), fprintf(stdout, "\n"), (void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define LOG_ERR(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"), (void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)
#else
#include <cstdio>
#define LOG_INF(...) fprintf(stdout, __VA_ARGS__), fprintf(stdout, "\n")
#define LOG_ERR(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#endif

// Window ECU identifiers
static vsomeip::service_t  window_service_id   = 0x1111;
static vsomeip::instance_t window_instance_id  = 0x2222;
static vsomeip::method_t   window_method_id    = 0x3333;

// Speed ECU identifiers
static vsomeip::service_t  speed_service_id    = 0x1234;
static vsomeip::instance_t speed_instance_id   = 0x5678;
static vsomeip::method_t   speed_method_id     = 0x0421;

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

        // Handle responses from ANY service
        app_->register_message_handler(
            vsomeip::ANY_SERVICE, vsomeip::ANY_INSTANCE, vsomeip::ANY_METHOD,
            std::bind(&hello_world_client::on_message_cbk, this,
                      std::placeholders::_1));

        // Watch Window ECU availability
        app_->register_availability_handler(
            window_service_id, window_instance_id,
            std::bind(&hello_world_client::on_availability_cbk, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3));

        // Watch Speed ECU availability
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
            // Request both services
            app_->request_service(window_service_id, window_instance_id);
            app_->request_service(speed_service_id,  speed_instance_id);
        }
    }

    void on_availability_cbk(vsomeip::service_t _service,
                             vsomeip::instance_t _instance,
                             bool _is_available) {
        if (!_is_available) return;

        std::shared_ptr<vsomeip::message> rq = rtm_->create_request();
        std::shared_ptr<vsomeip::payload> pl = rtm_->create_payload();

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

        rq->set_payload(pl);
        app_->send(rq);
    }

    void on_message_cbk(const std::shared_ptr<vsomeip::message>& _response) {
        if (_response->get_message_type() != vsomeip::message_type_e::MT_RESPONSE)
            return;
        if (_response->get_return_code() != vsomeip::return_code_e::E_OK)
            return;

        std::shared_ptr<vsomeip::payload> pl = _response->get_payload();
        if (!pl || pl->get_length() == 0) {
            LOG_ERR("Received empty payload");
            return;
        }

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

        // Print combined output only when both values are received
        if (window_received_ && speed_received_) {
            print_climate_state();
            stop();
        }
    }

    void print_climate_state() {
        const char* mode;
        if (window_position_ >= 80)      { mode = "LOW";    }
        else if (window_position_ >= 50) { mode = "MEDIUM"; }
        else if (window_position_ >= 20) { mode = "HIGH";   }
        else                             { mode = "FAST";   }

        LOG_INF("------------------------------------------");
        LOG_INF("Window=%d%% | Vehicle Speed=%d km/h | Mode: %s",
                static_cast<int>(window_position_),
                static_cast<int>(vehicle_speed_),
                mode);
        LOG_INF("------------------------------------------");
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

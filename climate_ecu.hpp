// Copyright (C) 2014-2026 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0.

#include <vsomeip/vsomeip.hpp>

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

static vsomeip::service_t  window_service_id  = 0x1111;
static vsomeip::instance_t window_instance_id = 0x2222;
static vsomeip::method_t   window_method_id   = 0x3333;

class hello_world_client {
public:
    hello_world_client()
        : rtm_(vsomeip::runtime::get()),
          app_(rtm_->create_application()),
          window_position_(0) {}

    bool init() {
        if (!app_->init()) {
            LOG_ERR("Couldn't initialize application");
            return false;
        }
        app_->register_state_handler(
            std::bind(&hello_world_client::on_state_cbk, this,
                      std::placeholders::_1));
        app_->register_message_handler(
            vsomeip::ANY_SERVICE, vsomeip::ANY_INSTANCE, vsomeip::ANY_METHOD,
            std::bind(&hello_world_client::on_message_cbk, this,
                      std::placeholders::_1));
        app_->register_availability_handler(
            window_service_id, window_instance_id,
            std::bind(&hello_world_client::on_availability_cbk, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3));
        return true;
    }

    void start() { app_->start(); }

    void on_state_cbk(vsomeip::state_type_e _state) {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
            app_->request_service(window_service_id, window_instance_id);
    }

    void on_availability_cbk(vsomeip::service_t _service,
                             vsomeip::instance_t _instance,
                             bool _is_available) {
        if (_service == window_service_id &&
            _instance == window_instance_id && _is_available) {
            std::shared_ptr<vsomeip::message> rq = rtm_->create_request();
            rq->set_service(window_service_id);
            rq->set_instance(window_instance_id);
            rq->set_method(window_method_id);
            std::shared_ptr<vsomeip::payload> pl = rtm_->create_payload();
            rq->set_payload(pl);
            LOG_INF("Requesting window position from Window ECU...");
            app_->send(rq);
        }
    }

    void on_message_cbk(const std::shared_ptr<vsomeip::message>& _response) {
        if (_response->get_message_type() != vsomeip::message_type_e::MT_RESPONSE)
            return;
        if (_response->get_return_code() != vsomeip::return_code_e::E_OK)
            return;

        std::shared_ptr<vsomeip::payload> pl = _response->get_payload();
        if (!pl || pl->get_length() == 0) return;

        window_position_ = pl->get_data()[0];
        LOG_INF("Window position received: %d %%",
                static_cast<int>(window_position_));

        evaluate_and_print();
        stop();
    }

    void evaluate_and_print() {
        const char* mode;
        const char* speed_range;

        if (window_position_ >= 80) {
            mode = "LOW";    speed_range = "0-30 km/h";
        } else if (window_position_ >= 50) {
            mode = "MEDIUM"; speed_range = "31-60 km/h";
        } else if (window_position_ >= 20) {
            mode = "HIGH";   speed_range = "61-90 km/h";
        } else {
            mode = "FAST";   speed_range = "91-120 km/h";
        }

        LOG_INF("------------------------------------------");
        LOG_INF("Window=%d%% | Speed: %s | Mode: %s",
                static_cast<int>(window_position_), speed_range, mode);
        LOG_INF("------------------------------------------");
    }

    void stop() {
        app_->unregister_state_handler();
        app_->clear_all_handler();
        app_->release_service(window_service_id, window_instance_id);
        app_->stop();
    }

private:
    std::shared_ptr<vsomeip::runtime>     rtm_;
    std::shared_ptr<vsomeip::application> app_;
    uint8_t window_position_;
};

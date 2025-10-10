#include "weight_grind_strategy.h"

#include "grind_controller.h"
#include "../hardware/WeightSensor.h"
#include "../hardware/grinder.h"
#include "../logging/grind_logging.h"
#include "../config/constants.h"
#include <Arduino.h>

void WeightGrindStrategy::on_enter(const GrindSessionDescriptor&, GrindStrategyContext&, const GrindLoopData&) {
    // No additional setup required; controller handled initialization.
}

bool WeightGrindStrategy::update(const GrindSessionDescriptor&,
                                 GrindStrategyContext& context,
                                 const GrindLoopData& loop_data) {
    auto* controller = context.controller;
    if (!controller) {
        return false;
    }

    switch (controller->phase) {
        case GrindPhase::PREDICTIVE:
            run_predictive_phase(*controller, loop_data);
            return true;
        case GrindPhase::PULSE_DECISION:
            run_pulse_decision_phase(*controller, loop_data);
            return true;
        case GrindPhase::PULSE_EXECUTE:
            run_pulse_execute_phase(*controller, loop_data);
            return true;
        case GrindPhase::PULSE_SETTLING:
            run_pulse_settling_phase(*controller, loop_data);
            return true;
        default:
            return false;
    }
}

void WeightGrindStrategy::on_exit(const GrindSessionDescriptor&, GrindStrategyContext&) {
    // No teardown required for weight strategy yet.
}

int WeightGrindStrategy::progress_percent(const GrindSessionDescriptor&,
                                          const GrindController&) const {
    // Weight-based progress remains computed directly by the controller.
    return 0;
}

float WeightGrindStrategy::get_clamped_pulse_flow_rate(const GrindController& controller) const {
    float flow_rate = controller.pulse_flow_rate;

    if (flow_rate < GRIND_FLOW_RATE_MIN_SANE_GPS) {
        flow_rate = GRIND_PULSE_FLOW_RATE_FALLBACK_GPS;
    } else if (flow_rate > GRIND_FLOW_RATE_MAX_SANE_GPS) {
        flow_rate = GRIND_FLOW_RATE_MAX_SANE_GPS;
    }

    return flow_rate;
}

float WeightGrindStrategy::calculate_pulse_duration_ms(const GrindController& controller,
                                                       float error_grams) const {
    float clamped_flow_rate = get_clamped_pulse_flow_rate(controller);

    // Calculate the productive grinding time needed (excludes startup latency)
    float productive_duration_ms = (error_grams / clamped_flow_rate) * 1000.0f;

    // Motor latency is the base time needed to start the system
    float motor_latency_ms = controller.get_motor_response_latency();

    // Clamp productive duration to valid range (0 to max additional time)
    float clamped_productive_ms = max(0.0f, min(productive_duration_ms, GRIND_MOTOR_MAX_PULSE_DURATION_MS));

    // Total pulse = latency (startup) + productive grinding time
    float final_duration = motor_latency_ms + clamped_productive_ms;

    return final_duration;
}

void WeightGrindStrategy::run_predictive_phase(GrindController& controller,
                                               const GrindLoopData& loop_data) const {
    if (!controller.weight_sensor) {
        return;
    }

    if (!controller.flow_start_confirmed) {
        const uint32_t flow_detection_window_ms = 500;
        float current_flow_rate = controller.weight_sensor->get_flow_rate(flow_detection_window_ms);

        if (current_flow_rate >= GRIND_FLOW_DETECTION_THRESHOLD_GPS) {
            controller.grind_latency_ms = loop_data.now - controller.phase_start_time;
            controller.flow_start_confirmed = true;
            LOG_BLE("[PREDICTIVE] Flow start CONFIRMED! Latency: %.1fms, Flow: %.2fg/s\n",
                    controller.grind_latency_ms, current_flow_rate);
        }
    }

    if (controller.flow_start_confirmed) {
        const uint32_t flow_rate_calc_window_ms = 1500;
        if (loop_data.now > (controller.phase_start_time + controller.grind_latency_ms + flow_rate_calc_window_ms)) {
            float current_flow_rate = controller.weight_sensor->get_flow_rate(flow_rate_calc_window_ms);

            if (current_flow_rate > GRIND_FLOW_DETECTION_THRESHOLD_GPS) {
                controller.motor_stop_target_weight = ((controller.grind_latency_ms * GRIND_LATENCY_TO_COAST_RATIO) /
                                                       (float)SYS_MS_PER_SECOND) * current_flow_rate;
            }
        }
    }

    if (loop_data.current_weight >= (controller.target_weight - controller.motor_stop_target_weight)) {
        controller.grinder->stop();
        controller.predictive_end_weight = loop_data.current_weight;
        controller.pulse_flow_rate = controller.weight_sensor->get_flow_rate_95th_percentile(2500);
        controller.switch_phase(GrindPhase::PULSE_SETTLING, loop_data);
    }
}

void WeightGrindStrategy::run_pulse_decision_phase(GrindController& controller,
                                                   const GrindLoopData& loop_data) const {
    if (!controller.weight_sensor) {
        return;
    }

    float settled_weight;
    if (!controller.weight_sensor->check_settling_complete(GRIND_SCALE_PRECISION_SETTLING_TIME_MS, &settled_weight)) {
        return;
    }

    float conservative_target = controller.target_weight - GRIND_ACCURACY_TOLERANCE_G;
    float error = conservative_target - settled_weight;

    // coast_time_ms removed - was only used for logging pulse history

    if (controller.target_weight - settled_weight < GRIND_ACCURACY_TOLERANCE_G ||
        controller.pulse_attempts >= GRIND_MAX_PULSE_ATTEMPTS) {
        controller.switch_phase(GrindPhase::FINAL_SETTLING, loop_data);
        return;
    }

    controller.pulse_history[controller.pulse_attempts].start_weight = settled_weight;
    controller.pulse_history[controller.pulse_attempts].end_weight = settled_weight;

    controller.current_pulse_duration_ms = calculate_pulse_duration_ms(controller, error);
    controller.pulse_history[controller.pulse_attempts].duration_ms = controller.current_pulse_duration_ms;

    controller.switch_phase(GrindPhase::PULSE_EXECUTE, loop_data);
    controller.grinder->start_pulse_rmt(static_cast<uint32_t>(controller.current_pulse_duration_ms));

    controller.pulse_attempts++;
}

void WeightGrindStrategy::run_pulse_execute_phase(GrindController& controller,
                                                  const GrindLoopData& loop_data) const {
    if (controller.grinder && controller.grinder->is_pulse_complete()) {
        controller.switch_phase(GrindPhase::PULSE_SETTLING, loop_data);
    }
}

void WeightGrindStrategy::run_pulse_settling_phase(GrindController& controller,
                                                   const GrindLoopData& loop_data) const {
    if (!controller.weight_sensor) {
        return;
    }

    if (loop_data.now - controller.phase_start_time >= controller.grind_latency_ms + GRIND_MOTOR_SETTLING_TIME_MS) {
        if (controller.weight_sensor->check_settling_complete(GRIND_MOTOR_SETTLING_TIME_MS)) {
            controller.switch_phase(GrindPhase::PULSE_DECISION, loop_data);
        }
    }
}

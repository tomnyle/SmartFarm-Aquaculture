#ifndef AQUACULTURE_RULES_H
#define AQUACULTURE_RULES_H

#include <Arduino.h>
#include <ArduinoJson.h>

// ==================== RULE DECISION STRUCTURE ====================

struct RuleDecision {
    bool aerator_on;
    bool circulation_on;
    bool water_pump_on;
    bool feeder_on;
    bool valve_on;
    bool light_on;
    
    uint32_t timestamp;
    char reason[256];  // Debug: why this decision was made
};

// ==================== PROFILE STRUCTURE ====================

struct AquacultureProfile {
    char name[32];           // "shrimp", "koi", "tilapia", etc.
    char display_name[64];   // "Tôm thẻ", "Cá chép", etc.
    
    // Temperature thresholds (°C)
    float temp_min;
    float temp_max;
    float temp_critical_max;
    float temp_critical_min;
    
    // pH thresholds
    float ph_min;
    float ph_max;
    
    // Dissolved Oxygen thresholds (mg/L)
    float do_min;
    float do_critical;
    
    // Water level thresholds (%)
    float level_min_percent;
    float level_critical_percent;
    
    // Operational settings
    uint16_t feeder_interval;      // seconds between feeding cycles
    uint8_t feeder_duration;       // seconds per feeding
    float circulation_temp_trigger; // Start circulation when temp exceeds this
};

// ==================== RULE ENGINE ====================

class AquacultureRuleEngine {
public:
    AquacultureRuleEngine();
    
    // Initialize with profile
    bool loadProfile(const AquacultureProfile& profile);
    AquacultureProfile getCurrentProfile() const { return active_profile; }
    
    // Main decision function
    RuleDecision evaluate(
        float water_temperature,
        float water_ph,
        float dissolved_oxygen,
        float water_level_percent,
        bool manual_override_active = false
    );
    
    // Manual mode
    void setManualOutput(const char* output_name, bool state);
    bool getManualOutput(const char* output_name) const;
    
    // Schedule management
    void setScheduleActive(bool active) { schedule_active = active; }
    bool isScheduleActive() const { return schedule_active; }
    
    // Safe mode
    void enterSafeMode(const char* reason);
    void exitSafeMode();
    bool isSafeMode() const { return safe_mode; }
    
    // Debug
    void printProfile() const;
    void printLastDecision() const;
    
private:
    AquacultureProfile active_profile;
    RuleDecision last_decision;
    
    // State tracking for debouncing
    struct {
        bool aerator_was_on;
        uint32_t aerator_change_time;
        
        bool circulation_was_on;
        uint32_t circulation_change_time;
        
        bool feeder_was_running;
        uint32_t feeder_last_run;
    } state_tracking;
    
    // Manual override states
    struct {
        bool aerator_override;
        bool circulation_override;
        bool water_pump_override;
        bool feeder_override;
        bool valve_override;
        bool light_override;
    } manual_states;
    
    bool schedule_active;
    bool safe_mode;
    char safe_mode_reason[128];
    
    // Rule implementations
    void evaluateTemperature(float temp, RuleDecision& decision);
    void evaluatePH(float ph, RuleDecision& decision);
    void evaluateDissolvedOxygen(float do_value, RuleDecision& decision);
    void evaluateWaterLevel(float level_percent, RuleDecision& decision);
    
    // Debouncing
    bool canChangeAerator(uint32_t now);
    bool canChangeCirculation(uint32_t now);
    
    static const uint32_t DEBOUNCE_TIME = 5000;  // 5 seconds
    static const uint32_t MIN_ON_TIME = 10000;   // 10 seconds minimum
    static const uint32_t MIN_OFF_TIME = 30000;  // 30 seconds minimum
};

#endif // AQUACULTURE_RULES_H

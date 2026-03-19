#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <cstdint>

struct GameState {
    int currentDay = 0;
    int64_t money = 1000000000;
    float stability = 50.0f;
    float stabilityResponsiveness = 0.07f;
    int64_t popChildren = 1000000;
    int64_t popAdults = 3000000;
    int64_t popSeniors = 1000000;
    int residentTax = 22;

    // Stadtverkehr
    int cityRoads = 20;
    int cityTransit = 10;
    int64_t cityTransitUsage = 1000000;

    // Fernverkehr
    int highways = 15;
    int intercityRail = 8;
    int64_t intercityUsage = 800000;

    // Getrennte Subventionen
    int cityTransitSubsidyRate = 20;
    int intercityRailSubsidyRate = 15;

    // Ticket-Preise
    float baseCityTicketPrice = 3.5f;
    float baseIntercityTicketPrice = 16.0f;
    float baseNationalTicketPrice = 14.0f;

    bool isTransitPrivatized = false;
    float transitProfitMargin = 1.2f;

    // Landesticket-Nutzer
    int64_t nationalTicketUsers = 0;

    // Healthcare
    int hospitals = 25;
    int64_t totalHealthcareCosts = 0;
    int healthcareSubsidyRate = 0;

    // Services
    int fireHouses = 1;
    int policeStations = 1;
    int trashRecycling = 1;
    int64_t totalServicesCosts = 0;

    GameState();
    void CreateDefaultGame();
    
    static GameState CreateSmallVillage();
    static GameState CreateMegaCity();

    [[nodiscard]] int64_t popGrownUps() const;
    [[nodiscard]] int64_t population() const;
    [[nodiscard]] bool IsWeekend() const;
    [[nodiscard]] float cityShare() const;
    [[nodiscard]] float longShare() const;
    [[nodiscard]] int64_t averageHealthcareCosts() const;
    [[nodiscard]] int64_t stateHealthcareCosts() const;
    [[nodiscard]] float averageCityTravelSpeed(int type = 0) const;
    [[nodiscard]] float averageLongDistanceSpeed(int type = 0) const;
    [[nodiscard]] float getCityTicketPrice() const;
    [[nodiscard]] float getIntercityTicketPrice() const;
    [[nodiscard]] float getNationalTicketPrice() const;
    [[nodiscard]] int64_t getTransitBalance() const;
    [[nodiscard]] int64_t stateCityTransitSubsidyCosts() const;
    [[nodiscard]] int64_t stateIntercityRailSubsidyCosts() const;
    [[nodiscard]] int64_t stateTransitSubsidyCosts() const;
    [[nodiscard]] int64_t cityTransitMaintenance() const;
    [[nodiscard]] int64_t intercityRailMaintenance() const;
    [[nodiscard]] int64_t publicTransportMaintenance() const;
    [[nodiscard]] int64_t stateHighwayCosts() const;
    [[nodiscard]] int64_t stateServicesCosts() const;
    
    int64_t GetNetIncome();
    void UpdateTraffic();
    void UpdateSimulation();
};

// Globale Instanz
extern GameState state;

#endif

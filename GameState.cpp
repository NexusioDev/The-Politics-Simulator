#include "GameState.h"
#include <algorithm>

GameState state;

GameState::GameState() {
    CreateDefaultGame();
}

void GameState::CreateDefaultGame() { //TODO: Bessere Szenarien bzw. auch irgendwann modulare Szenarien erstellen
    money = 1000000000;
    popChildren = 1000000;
    popAdults = 3000000;
    popSeniors = 1000000;
    residentTax = 25;
    stability = 50.0f;
}

GameState GameState::CreateSmallVillage() {
    GameState gs;
    gs.money = 250000000;
    gs.popChildren = 20000;
    gs.popAdults = 80000;
    gs.popSeniors = 20000;
    gs.residentTax = 18;
    gs.cityRoads = 5;
    gs.cityTransit = 1;
    gs.highways = 1;
    gs.intercityRail = 1;
    gs.cityTransitUsage = 30000;
    gs.intercityUsage = 24000;
    gs.nationalTicketUsers = 21000;
    gs.cityTransitSubsidyRate = 5;
    gs.intercityRailSubsidyRate = 60;
    gs.baseCityTicketPrice = 5.0f;
    gs.baseIntercityTicketPrice = 20.0f;
    gs.baseNationalTicketPrice = 20.0f;
    gs.isTransitPrivatized = false;
    gs.transitProfitMargin = 1.1f;
    gs.hospitals = 1;
    gs.totalHealthcareCosts = 0;
    gs.healthcareSubsidyRate = 0;
    gs.fireHouses = 1;
    gs.policeStations = 1;
    gs.trashRecycling = 1;
    return gs;
}

GameState GameState::CreateMegaCity() {
    GameState gs;
    gs.money = 5000000000LL;
    gs.popAdults = 10000000;
    gs.popChildren = 2000000;
    gs.popSeniors = 3000000;
    gs.highways = 50;
    return gs;
}

int64_t GameState::popGrownUps() const {
    return popAdults + popSeniors;
}

int64_t GameState::population() const {
    return popChildren + popAdults + popSeniors;
}

bool GameState::IsWeekend() const {
    return currentDay == 5 || currentDay == 6;
}

float GameState::cityShare() const {
    return IsWeekend() ? 0.3f : 0.5f;
}

float GameState::longShare() const {
    return IsWeekend() ? 0.7f : 0.5f;
}

int64_t GameState::averageHealthcareCosts() const {
    return (totalHealthcareCosts - stateHealthcareCosts()) / popGrownUps();
}

int64_t GameState::stateHealthcareCosts() const {
    if (healthcareSubsidyRate > 0) {
        return totalHealthcareCosts * healthcareSubsidyRate / 100;
    }
    return 0;
}

float GameState::averageCityTravelSpeed(int type) const {
    int64_t pop = population();
    if (pop <= 0) return 0.0f;

    int64_t totalCityTransitUsers = cityTransitUsage + (nationalTicketUsers / 2);
    int64_t cityRelevant = pop * cityShare();
    int64_t cityUsers = std::min(totalCityTransitUsers, cityRelevant);
    int64_t cityRoadUsers = std::max<int64_t>(0, cityRelevant - cityUsers);

    float baseCarSpeed = 60.0f;
    float baseTransitSpeed = 40.0f;

    float roadLoad = static_cast<float>(cityRoadUsers) / std::max(1.0f, static_cast<float>(cityRoads) * 9000.0f);
    float transitLoad = static_cast<float>(cityUsers) / std::max(1.0f, static_cast<float>(cityTransit) * 12000.0f);

    float actualCarSpeed = baseCarSpeed / (1.0f + roadLoad * 0.5f);
    float actualTransitSpeed = baseTransitSpeed / (1.0f + transitLoad * 0.3f);

    float avgSpeed = 0;
    if (type == 0) avgSpeed = (static_cast<float>(cityRoadUsers) * actualCarSpeed + static_cast<float>(cityUsers) * actualTransitSpeed) / static_cast<float>(cityRelevant);
    else if (type == 1) avgSpeed = actualCarSpeed;
    else avgSpeed = actualTransitSpeed;

    return std::max(5.0f, avgSpeed);
}

float GameState::averageLongDistanceSpeed(int type) const {
    int64_t pop = population();
    if (pop <= 0) return 0.0f;

    int64_t totalIntercityUsers = intercityUsage + (nationalTicketUsers / 2);
    auto travellers = static_cast<int64_t>(static_cast<float>(pop) * longShare());
    int64_t railUsers = std::min(totalIntercityUsers, travellers);
    int64_t roadUsers = std::max<int64_t>(0, travellers - railUsers);

    float baseHighwaySpeed = 140.0f;
    float baseRailSpeed = 300.0f;

    float highwayLoad = static_cast<float>(roadUsers) / std::max(1.0f, static_cast<float>(highways) * 40000.0f);
    float railLoad = static_cast<float>(railUsers) / std::max(1.0f, static_cast<float>(intercityRail) * 30000.0f);

    float actualHighwaySpeed = baseHighwaySpeed / (1.0f + highwayLoad * 0.5f);
    float actualRailSpeed = baseRailSpeed / (1.0f + railLoad * 0.3f);

    float avgSpeed = 0;
    if (type == 0) avgSpeed = (roadUsers * actualHighwaySpeed + railUsers * actualRailSpeed) / (float)travellers;
    else if (type == 1) avgSpeed = actualHighwaySpeed;
    else avgSpeed = actualRailSpeed;

    return std::max(20.0f, avgSpeed);
}

float GameState::getCityTicketPrice() const {
    float costPerRide = 2.0f + (static_cast<float>(cityTransitMaintenance()) / std::max<int64_t>(1, cityTransitUsage));

    if (isTransitPrivatized) {
        float priceBeforeSubsidy = costPerRide * transitProfitMargin;
        return std::max(0.0f, priceBeforeSubsidy * (1.0f - static_cast<float>(cityTransitSubsidyRate) / 100.0f));
    } else {
        return std::max(0.0f, baseCityTicketPrice * (1.0f - static_cast<float>(cityTransitSubsidyRate) / 100.0f));
    }
}

float GameState::getIntercityTicketPrice() const {
    float costPerRide = 8.0f + (static_cast<float>(intercityRailMaintenance()) / std::max<int64_t>(1, intercityUsage));

    if (isTransitPrivatized) {
        float priceBeforeSubsidy = costPerRide * transitProfitMargin;
        return std::max(0.0f, priceBeforeSubsidy * (1.0f - static_cast<float>(intercityRailSubsidyRate) / 100.0f));
    } else {
        return std::max(0.0f, baseIntercityTicketPrice * (1.0f - static_cast<float>(intercityRailSubsidyRate) / 100.0f));
    }
}

float GameState::getNationalTicketPrice() const {
    float combinedPrice = getCityTicketPrice() + getIntercityTicketPrice();
    return std::max(0.0f, combinedPrice * 0.8f);
}

int64_t GameState::getTransitBalance() const {
    auto cityIncome = static_cast<int64_t>(cityTransitUsage * getCityTicketPrice());
    auto intercityIncome = static_cast<int64_t>(intercityUsage * getIntercityTicketPrice());
    auto nationalIncome = static_cast<int64_t>(nationalTicketUsers * getNationalTicketPrice());

    int64_t totalIncome = cityIncome + intercityIncome + nationalIncome;
    int64_t totalMaintenance = cityTransitMaintenance() + intercityRailMaintenance();
    int64_t totalSubsidy = stateCityTransitSubsidyCosts() + stateIntercityRailSubsidyCosts();

    if (isTransitPrivatized) {
        return -totalSubsidy;
    } else {
        return totalIncome - totalMaintenance - totalSubsidy;
    }
}

int64_t GameState::stateCityTransitSubsidyCosts() const {
    if (isTransitPrivatized) {
        float costPerRide = 2.0f + (static_cast<float>(cityTransitMaintenance()) / std::max<int64_t>(1, cityTransitUsage));
        float priceBeforeSubsidy = costPerRide * transitProfitMargin;
        return static_cast<int64_t>(cityTransitUsage * priceBeforeSubsidy * (cityTransitSubsidyRate / 100.0f));
    } else {
        return static_cast<int64_t>(cityTransitUsage * baseCityTicketPrice * (cityTransitSubsidyRate / 100.0f));
    }
}

int64_t GameState::stateIntercityRailSubsidyCosts() const {
    if (isTransitPrivatized) {
        float costPerRide = 8.0f + (static_cast<float>(intercityRailMaintenance()) / std::max<int64_t>(1, intercityUsage));
        float priceBeforeSubsidy = costPerRide * transitProfitMargin;
        return static_cast<int64_t>(intercityUsage * priceBeforeSubsidy * (intercityRailSubsidyRate / 100.0f));
    } else {
        return static_cast<int64_t>(intercityUsage * baseIntercityTicketPrice * (intercityRailSubsidyRate / 100.0f));
    }
}

int64_t GameState::stateTransitSubsidyCosts() const {
    return stateCityTransitSubsidyCosts() + stateIntercityRailSubsidyCosts();
}

int64_t GameState::cityTransitMaintenance() const {
    return static_cast<int64_t>(cityTransit) * 700000LL + (cityTransitUsage * 1);
}

int64_t GameState::intercityRailMaintenance() const {
    return static_cast<int64_t>(intercityRail) * 20000000LL + (intercityUsage * 2);
}

int64_t GameState::publicTransportMaintenance() const {
    return cityTransitMaintenance() + intercityRailMaintenance();
}

int64_t GameState::stateHighwayCosts() const {
    return static_cast<int64_t>(highways) * 2000000LL + population();
}

int64_t GameState::stateServicesCosts() const {
    int64_t costs = 0;
    if (fireHouses != 0) costs += fireHouses * 2000000LL;
    else costs += population() * 5;

    if (policeStations != 0) costs += policeStations * 2000000LL;
    else costs += population() * 5;

    if (trashRecycling != 0) costs += trashRecycling * 2000000LL;
    else costs += population() * 5;

    return costs;
}

int64_t GameState::GetNetIncome() {
    int64_t totalTax = (popGrownUps() * 2000LL * residentTax) / 100LL;
    int64_t upkeepBase = (population() * 15LL) / 100LL;
    int64_t healthCosts = stateHealthcareCosts();
    int64_t infraHighway = stateHighwayCosts();
    totalServicesCosts = stateServicesCosts();
    int64_t transitNet = getTransitBalance();

    int64_t net = totalTax - upkeepBase - healthCosts - infraHighway - totalServicesCosts + transitNet;

    if (money < 0) net += (money * 2LL) / 100LL;

    return net;
}

void GameState::UpdateTraffic() { //TODO: Schauen was hier verbessert werden kann, und auch wie es besser benutzt werden kann
    float cityCarSpeed = averageCityTravelSpeed(1);
    float cityTransitSpeed = averageCityTravelSpeed(2);
    float longCarSpeed = averageLongDistanceSpeed(1);
    float longTransitSpeed = averageLongDistanceSpeed(2);

    float cityPriceFactor = 1.0f / (1.0f + getCityTicketPrice() * 0.2f);
    float intercityPriceFactor = 1.0f / (1.0f + getIntercityTicketPrice() * 0.1f);
    float nationalPriceFactor = 1.0f / (1.0f + getNationalTicketPrice() * 0.15f);

    float cityTimeFactor = std::clamp(30.0f / cityCarSpeed, 0.3f, 2.0f);
    float longTimeFactor = std::clamp(120.0f / longCarSpeed, 0.3f, 2.0f);
    float taxFactor = static_cast<float>(residentTax) / 100.0f;

    double targetCityUsage = static_cast<float>(population()) * cityShare() * cityTimeFactor * (taxFactor + 0.1f) * cityPriceFactor;
    cityTransitUsage = static_cast<int64_t>(cityTransitUsage * 0.95 + targetCityUsage * 0.05);
    cityTransitUsage = std::min<int64_t>(population() * cityShare(), cityTransitUsage);

    double targetIntercityUsage = static_cast<float>(population()) * longShare() * (taxFactor + 0.1f) * longTimeFactor * intercityPriceFactor;
    intercityUsage = static_cast<int64_t>(intercityUsage * 0.95 + targetIntercityUsage * 0.05);
    intercityUsage = std::min<int64_t>(population() * longShare(), intercityUsage);

    double targetNationalUsers = static_cast<float>(population()) * 0.5f * nationalPriceFactor;
    nationalTicketUsers = static_cast<int64_t>(nationalTicketUsers * 0.95 + targetNationalUsers * 0.05);
    nationalTicketUsers = std::min<int64_t>(population() / 2, nationalTicketUsers);
}

void GameState::UpdateSimulation() {
    UpdateTraffic();

    totalHealthcareCosts = hospitals * 5000000;
    totalHealthcareCosts += static_cast<int64_t>(population() * 0.5f);

    int64_t avgHealth = averageHealthcareCosts();
    if (avgHealth < 1) avgHealth = 1;
    float taxPenalty = 1.0f - static_cast<float>(residentTax) / 100.0f;
    taxPenalty = std::clamp(taxPenalty, 0.1f, 1.0f);
    float growth = static_cast<float>(popAdults) * taxPenalty * (0.5f / static_cast<float>(avgHealth));

    popChildren += static_cast<int64_t>(growth);

    int64_t movingToAdults = popChildren * 0.001f;
    int64_t movingToSeniors = popAdults * 0.001f;
    int64_t deaths = popSeniors * 0.001f * (static_cast<float>(avgHealth) / 400.0f);

    popChildren -= movingToAdults;
    popAdults += movingToAdults;
    popAdults -= movingToSeniors;
    popSeniors += movingToSeniors;
    popSeniors -= deaths;

    popChildren = std::max<int64_t>(0, popChildren);
    popAdults = std::max<int64_t>(0, popAdults);
    popSeniors = std::max<int64_t>(0, popSeniors);

    money += GetNetIncome();

    float desiredStability = 50.0f;
    float citySpeed = averageCityTravelSpeed();
    float longSpeed = averageLongDistanceSpeed();

    if (money < 0) {
        desiredStability -= 10.0f;
        if (money < -1000000000) desiredStability -= 10.0f;
    }

    if (citySpeed < 30.0f)  desiredStability -= 3.0f;
    if (citySpeed < 20.0f)  desiredStability -= 7.0f;
    if (citySpeed < 10.0f)  desiredStability -= 10.0f;

    if (longSpeed < 100.0f) desiredStability -= 3.0f;
    if (longSpeed < 50.0f)  desiredStability -= 5.0f;

    avgHealth = averageHealthcareCosts();
    if (avgHealth > 60)   desiredStability -= 3.0f;
    if (avgHealth > 100)  desiredStability -= 7.0f;
    if (avgHealth > 150)  desiredStability -= 10.0f;

    if (residentTax < 15)        desiredStability += 7.0f;
    else if (residentTax < 25)   desiredStability += 3.0f;
    else if (residentTax <= 35)  desiredStability += 0.0f;
    else if (residentTax <= 45)  desiredStability -= 5.0f;
    else      desiredStability -= 12.0f;

    desiredStability = std::clamp(desiredStability, 0.0f, 100.0f);
    float change = (desiredStability - stability) * stabilityResponsiveness;
    stability += change;
    stability = std::clamp(stability, 0.0f, 100.0f);
}

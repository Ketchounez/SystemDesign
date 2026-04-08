#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

#include "domain/service.hpp"

namespace lab2 {

class HotelBookingServiceComponent final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "hotel-booking-service";

  HotelBookingServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context)
      : userver::components::ComponentBase(config, context),
        service_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

  HotelBookingService& GetService() { return service_; }
  const HotelBookingService& GetService() const { return service_; }

 private:
  HotelBookingService service_;
};

}

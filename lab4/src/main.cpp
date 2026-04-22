#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "components/hotel_booking_service_component.hpp"
#include "api/handlers/auth_handlers.hpp"
#include "api/handlers/booking_handlers.hpp"
#include "api/handlers/hotel_handlers.hpp"
#include "api/handlers/user_handlers.hpp"
#include "api/middleware/session_auth_middleware.hpp"

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<lab2::HotelBookingServiceComponent>()
          .Append<userver::clients::dns::Component>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::components::Postgres>("postgres-db")
          .Append<userver::components::Mongo>("mongo-db")
          .Append<lab2::SessionAuthMiddlewareFactory>()
          .Append<lab2::RegisterHandler>()
          .Append<lab2::LoginHandler>()
          .Append<lab2::UserByLoginHandler>()
          .Append<lab2::UserSearchHandler>()
          .Append<lab2::HotelsHandler>()
          .Append<lab2::BookingsHandler>()
          .Append<lab2::UserBookingsHandler>()
          .Append<lab2::BookingByIdHandler>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}

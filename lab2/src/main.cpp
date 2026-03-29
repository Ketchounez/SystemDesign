#include <userver/components/minimal_server_component_list.hpp>
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

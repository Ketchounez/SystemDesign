#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <userver/storages/postgres/cluster.hpp>

#include "domain/models.hpp"

namespace lab2 {

class HotelBookingService final {
 public:
  explicit HotelBookingService(userver::storages::postgres::ClusterPtr pg_cluster);

  User RegisterUser(const RegisterUserRequest& request);
  Session Login(const LoginRequest& request);
  std::optional<Session> Authenticate(const std::string& token) const;

  std::optional<User> GetUserById(std::int64_t user_id) const;
  std::optional<User> FindUserByLogin(const std::string& login) const;
  std::vector<User> SearchUsers(const std::string& name_mask, const std::string& surname_mask) const;

  Hotel CreateHotel(const CreateHotelRequest& request, std::int64_t created_by_user_id);
  std::optional<Hotel> GetHotelById(std::int64_t hotel_id) const;
  std::vector<Hotel> ListHotels(const std::optional<std::string>& city) const;

  Booking CreateBooking(const CreateBookingRequest& request, std::int64_t user_id);
  std::vector<Booking> ListUserBookings(std::int64_t requester_user_id, std::int64_t target_user_id) const;
  void CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id);

 private:
  static std::string Normalize(const std::string& value);
  static std::string Trim(const std::string& value);
  static void ValidateDate(const std::string& value, const std::string& field_name);
  static std::string HashPassword(const std::string& value);
  static std::string GenerateToken(std::string_view login, std::int64_t user_id);

  User ParseUserRow(const userver::storages::postgres::Row& row) const;
  Hotel ParseHotelRow(const userver::storages::postgres::Row& row) const;
  Booking ParseBookingRow(const userver::storages::postgres::Row& row) const;

  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}

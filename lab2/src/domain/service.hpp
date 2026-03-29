#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain/models.hpp"

namespace lab2 {

class HotelBookingService final {
 public:
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
  static bool IsBlank(const std::string& value);
  static bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle);
  static void ValidateDate(const std::string& value, const std::string& field_name);
  static bool DatesOverlap(const Booking& left, const Booking& right);
  static std::string HashPassword(const std::string& value);
  static std::string GenerateToken(std::string_view login, std::int64_t user_id, std::int64_t session_seq);

  const User& RequireUserUnlocked(std::int64_t user_id) const;
  const Hotel& RequireHotelUnlocked(std::int64_t hotel_id) const;
  const Booking& RequireBookingUnlocked(std::int64_t booking_id) const;

  mutable std::mutex mutex_;
  std::int64_t next_user_id_{1};
  std::int64_t next_hotel_id_{1};
  std::int64_t next_booking_id_{1};
  std::int64_t next_session_id_{1};
  std::unordered_map<std::int64_t, User> users_;
  std::unordered_map<std::string, std::int64_t> user_ids_by_login_;
  std::unordered_map<std::int64_t, Hotel> hotels_;
  std::unordered_map<std::int64_t, Booking> bookings_;
  std::unordered_map<std::string, Session> sessions_;
};

}

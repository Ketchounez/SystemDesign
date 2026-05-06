#pragma once

#include <mutex>
#include <optional>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/formats/bson/document.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/mongo/pool.hpp>

#include "domain/models.hpp"

namespace lab2 {

class HotelBookingService final {
 public:
  struct RateLimitDecision {
    bool allowed{true};
    std::uint32_t limit{0};
    std::uint32_t remaining{0};
    std::int64_t reset_unix_seconds{0};
  };

  HotelBookingService(
      userver::storages::postgres::ClusterPtr pg_cluster,
      userver::storages::mongo::PoolPtr mongo_pool,
      userver::storages::redis::ClientPtr redis_client);

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
  RateLimitDecision CheckBookingCreationRateLimit(std::int64_t user_id);

 private:
  struct CachedHotelsEntry {
    std::vector<Hotel> hotels;
    std::chrono::steady_clock::time_point expire_at;
  };

  struct CachedUserBookingsEntry {
    std::vector<Booking> bookings;
    std::chrono::steady_clock::time_point expire_at;
  };

  static std::string Normalize(const std::string& value);
  static std::string Trim(const std::string& value);
  static bool IsBlank(const std::string& value);
  static bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle);
  static void ValidateDate(const std::string& value, const std::string& field_name);
  static bool DatesOverlap(const Booking& left, const Booking& right);
  static std::string HashPassword(const std::string& value);
  static std::string GenerateToken(std::string_view login, std::int64_t user_id);

  static User ParseUser(const userver::formats::bson::Document& doc);
  static Hotel ParseHotel(const userver::formats::bson::Document& doc);
  static Booking ParseBooking(const userver::formats::bson::Document& doc);
  std::int64_t NextId(const std::string& collection_name) const;
  User RequireUserPg(std::int64_t user_id) const;
  Hotel RequireHotelPg(std::int64_t hotel_id) const;
  User ParseUserRow(const userver::storages::postgres::Row& row) const;
  Hotel ParseHotelRow(const userver::storages::postgres::Row& row) const;
  void InvalidateHotelsCache();
  void InvalidateUserBookingsCache(std::int64_t user_id);
  static std::string BuildHotelsCacheKey(const std::optional<std::string>& city_filter);

  mutable std::mutex mutex_;
  std::unordered_map<std::string, Session> sessions_;
  mutable std::unordered_map<std::string, CachedHotelsEntry> hotels_cache_;
  mutable std::unordered_map<std::int64_t, CachedUserBookingsEntry> user_bookings_cache_;
  userver::storages::postgres::ClusterPtr pg_cluster_;
  userver::storages::mongo::PoolPtr mongo_pool_;
  userver::storages::redis::ClientPtr redis_client_;
  userver::storages::redis::CommandControl redis_cc_;
};

}

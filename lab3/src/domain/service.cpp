#include "domain/service.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <regex>
#include <sstream>
#include <utility>

namespace lab2 {

namespace {

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string Fnv1aHex(std::string_view value) {
  constexpr std::uint64_t kOffsetBasis = 14695981039346656037ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;

  std::uint64_t hash = kOffsetBasis;
  for (unsigned char ch : value) {
    hash ^= ch;
    hash *= kPrime;
  }

  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return out.str();
}

}  // namespace

HotelBookingService::HotelBookingService(userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

std::string HotelBookingService::Trim(const std::string& value) {
  const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
                     return std::isspace(ch) != 0;
                   }).base();

  if (begin >= end) return {};
  return std::string(begin, end);
}

std::string HotelBookingService::Normalize(const std::string& value) {
  return ToLower(Trim(value));
}

void HotelBookingService::ValidateDate(const std::string& value, const std::string& field_name) {
  static const std::regex kDatePattern(R"(^\d{4}-\d{2}-\d{2}$)");
  if (!std::regex_match(value, kDatePattern)) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_date",
        "Field '" + field_name + "' must be in YYYY-MM-DD format");
  }
}

std::string HotelBookingService::HashPassword(const std::string& value) {
  return Fnv1aHex("password:" + value);
}

std::string HotelBookingService::GenerateToken(std::string_view login, std::int64_t user_id) {
  const auto seed = std::string(login) + ":" + std::to_string(user_id) + ":" + Fnv1aHex(login);
  return "session-" + Fnv1aHex(seed);
}

User HotelBookingService::ParseUserRow(const userver::storages::postgres::Row& row) const {
  User user;
  user.id = row["id"].As<std::int64_t>();
  user.login = row["login"].As<std::string>();
  user.name = row["name"].As<std::string>();
  user.surname = row["surname"].As<std::string>();
  user.email = row["email"].As<std::string>();
  user.password_hash = row["password_hash"].As<std::string>();
  return user;
}

Hotel HotelBookingService::ParseHotelRow(const userver::storages::postgres::Row& row) const {
  Hotel hotel;
  hotel.id = row["id"].As<std::int64_t>();
  hotel.name = row["name"].As<std::string>();
  hotel.city = row["city"].As<std::string>();
  hotel.address = row["address"].As<std::string>();
  hotel.description = row["description"].As<std::string>();
  hotel.created_by_user_id = row["created_by_user_id"].As<std::int64_t>();
  return hotel;
}

Booking HotelBookingService::ParseBookingRow(const userver::storages::postgres::Row& row) const {
  Booking booking;
  booking.id = row["id"].As<std::int64_t>();
  booking.user_id = row["user_id"].As<std::int64_t>();
  booking.hotel_id = row["hotel_id"].As<std::int64_t>();
  booking.check_in = row["check_in"].As<std::string>();
  booking.check_out = row["check_out"].As<std::string>();
  const auto status = row["status"].As<std::string>();
  booking.status = (status == "cancelled") ? BookingStatus::kCancelled : BookingStatus::kActive;
  return booking;
}

User HotelBookingService::RegisterUser(const RegisterUserRequest& request) {
  const auto login = Normalize(request.login);
  const auto name = Trim(request.name);
  const auto surname = Trim(request.surname);
  const auto email = Trim(request.email);
  const auto password = request.password;

  if (login.empty() || name.empty() || surname.empty() || email.empty() || password.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "login, name, surname, email and password are required");
  }

  const auto existing = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM users WHERE login=$1",
      login);
  if (!existing.IsEmpty()) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "login_already_exists",
        "User with this login already exists");
  }

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO users(login, name, surname, email, password_hash) "
      "VALUES ($1, $2, $3, $4, $5) "
      "RETURNING id, login, name, surname, email, password_hash",
      login,
      name,
      surname,
      email,
      HashPassword(password));

  return ParseUserRow(result.Front());
}

Session HotelBookingService::Login(const LoginRequest& request) {
  const auto login = Normalize(request.login);
  if (login.empty() || request.password.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "login and password are required");
  }

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, login, password_hash FROM users WHERE login=$1",
      login);
  if (result.IsEmpty()) {
    throw ServiceError(
        ServiceErrorKind::kUnauthorized,
        "invalid_credentials",
        "Invalid login or password");
  }

  const auto row = result.Front();
  const auto password_hash = row["password_hash"].As<std::string>();
  if (password_hash != HashPassword(request.password)) {
    throw ServiceError(
        ServiceErrorKind::kUnauthorized,
        "invalid_credentials",
        "Invalid login or password");
  }

  Session session;
  session.user_id = row["id"].As<std::int64_t>();
  session.login = row["login"].As<std::string>();
  session.token = GenerateToken(session.login, session.user_id);

  pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO sessions(token, user_id, login) VALUES($1, $2, $3) "
      "ON CONFLICT (token) DO UPDATE SET user_id = EXCLUDED.user_id, login = EXCLUDED.login",
      session.token,
      session.user_id,
      session.login);

  return session;
}

std::optional<Session> HotelBookingService::Authenticate(const std::string& token) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT token, user_id, login FROM sessions WHERE token=$1",
      token);
  if (result.IsEmpty()) return std::nullopt;

  Session session;
  const auto row = result.Front();
  session.token = row["token"].As<std::string>();
  session.user_id = row["user_id"].As<std::int64_t>();
  session.login = row["login"].As<std::string>();
  return session;
}

std::optional<User> HotelBookingService::GetUserById(std::int64_t user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, login, name, surname, email, password_hash FROM users WHERE id=$1",
      user_id);
  if (result.IsEmpty()) return std::nullopt;
  return ParseUserRow(result.Front());
}

std::optional<User> HotelBookingService::FindUserByLogin(const std::string& login) const {
  const auto normalized_login = Normalize(login);
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, login, name, surname, email, password_hash FROM users WHERE login=$1",
      normalized_login);
  if (result.IsEmpty()) return std::nullopt;
  return ParseUserRow(result.Front());
}

std::vector<User> HotelBookingService::SearchUsers(
    const std::string& name_mask,
    const std::string& surname_mask) const {
  const auto normalized_name = Trim(name_mask);
  const auto normalized_surname = Trim(surname_mask);
  if (normalized_name.empty() && normalized_surname.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "At least one search parameter must be provided");
  }

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, login, name, surname, email, password_hash FROM users "
      "WHERE ($1 = '' OR lower(name) LIKE '%' || lower($1) || '%') "
      "  AND ($2 = '' OR lower(surname) LIKE '%' || lower($2) || '%') "
      "ORDER BY id",
      normalized_name,
      normalized_surname);

  std::vector<User> users;
  users.reserve(result.Size());
  for (const auto& row : result) users.push_back(ParseUserRow(row));
  return users;
}

Hotel HotelBookingService::CreateHotel(
    const CreateHotelRequest& request,
    std::int64_t created_by_user_id) {
  const auto name = Trim(request.name);
  const auto city = Trim(request.city);
  const auto address = Trim(request.address);
  const auto description = Trim(request.description);

  if (name.empty() || city.empty() || address.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "name, city and address are required");
  }

  const auto user_check = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM users WHERE id=$1",
      created_by_user_id);
  if (user_check.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO hotels(name, city, address, description, created_by_user_id) "
      "VALUES ($1, $2, $3, $4, $5) "
      "RETURNING id, name, city, address, description, created_by_user_id",
      name,
      city,
      address,
      description,
      created_by_user_id);

  return ParseHotelRow(result.Front());
}

std::optional<Hotel> HotelBookingService::GetHotelById(std::int64_t hotel_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, name, city, address, description, created_by_user_id FROM hotels WHERE id=$1",
      hotel_id);
  if (result.IsEmpty()) return std::nullopt;
  return ParseHotelRow(result.Front());
}

std::vector<Hotel> HotelBookingService::ListHotels(const std::optional<std::string>& city) const {
  const auto city_value = city.has_value() ? Trim(*city) : std::string{};
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, name, city, address, description, created_by_user_id "
      "FROM hotels "
      "WHERE ($1 = '' OR lower(city) = lower($1)) "
      "ORDER BY id",
      city_value);

  std::vector<Hotel> hotels;
  hotels.reserve(result.Size());
  for (const auto& row : result) hotels.push_back(ParseHotelRow(row));
  return hotels;
}

Booking HotelBookingService::CreateBooking(
    const CreateBookingRequest& request,
    std::int64_t user_id) {
  ValidateDate(request.check_in, "check_in");
  ValidateDate(request.check_out, "check_out");
  if (request.check_in >= request.check_out) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_date_range",
        "check_in must be earlier than check_out");
  }

  const auto user_check = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM users WHERE id=$1",
      user_id);
  if (user_check.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }

  const auto hotel_check = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM hotels WHERE id=$1",
      request.hotel_id);
  if (hotel_check.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "hotel_not_found", "Hotel was not found");
  }

  const auto conflict = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM bookings "
      "WHERE hotel_id = $1 AND status = 'active' "
      "  AND NOT (check_out <= $2::date OR check_in >= $3::date) "
      "LIMIT 1",
      request.hotel_id,
      request.check_in,
      request.check_out);
  if (!conflict.IsEmpty()) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "booking_dates_conflict",
        "Hotel is already booked for the selected dates");
  }

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO bookings(user_id, hotel_id, check_in, check_out, status) "
      "VALUES ($1, $2, $3::date, $4::date, 'active') "
      "RETURNING id, user_id, hotel_id, check_in::text, check_out::text, status",
      user_id,
      request.hotel_id,
      request.check_in,
      request.check_out);

  return ParseBookingRow(result.Front());
}

std::vector<Booking> HotelBookingService::ListUserBookings(
    std::int64_t requester_user_id,
    std::int64_t target_user_id) const {
  const auto requester_check = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM users WHERE id=$1",
      requester_user_id);
  if (requester_check.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }

  const auto target_check = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM users WHERE id=$1",
      target_user_id);
  if (target_check.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }

  if (requester_user_id != target_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only access your own bookings");
  }

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, user_id, hotel_id, check_in::text AS check_in, check_out::text AS check_out, status "
      "FROM bookings "
      "WHERE user_id=$1 "
      "ORDER BY id",
      target_user_id);

  std::vector<Booking> bookings;
  bookings.reserve(result.Size());
  for (const auto& row : result) bookings.push_back(ParseBookingRow(row));
  return bookings;
}

void HotelBookingService::CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id) {
  const auto requester_check = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM users WHERE id=$1",
      requester_user_id);
  if (requester_check.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }

  const auto booking_result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, user_id, status FROM bookings WHERE id=$1",
      booking_id);
  if (booking_result.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "booking_not_found", "Booking was not found");
  }

  const auto row = booking_result.Front();
  const auto owner_id = row["user_id"].As<std::int64_t>();
  const auto status = row["status"].As<std::string>();
  if (owner_id != requester_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only cancel your own bookings");
  }
  if (status == "cancelled") {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "booking_already_cancelled",
        "Booking has already been cancelled");
  }

  pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "UPDATE bookings SET status='cancelled' WHERE id=$1",
      booking_id);
}

}  // namespace lab2

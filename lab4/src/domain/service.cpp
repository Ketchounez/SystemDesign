#include "domain/service.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <utility>

#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/utils/datetime.hpp>

namespace lab2 {

namespace {

using userver::formats::bson::MakeDoc;

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

userver::storages::mongo::Collection GetCollection(
    const userver::storages::mongo::PoolPtr& pool,
    std::string name) {
  return pool->GetCollection(std::move(name));
}

}

HotelBookingService::HotelBookingService(
    userver::storages::postgres::ClusterPtr pg_cluster,
    userver::storages::mongo::PoolPtr mongo_pool)
    : pg_cluster_(std::move(pg_cluster)), mongo_pool_(std::move(mongo_pool)) {}

std::string HotelBookingService::Trim(const std::string& value) {
  const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  }).base();
  if (begin >= end) {
    return {};
  }
  return std::string(begin, end);
}

std::string HotelBookingService::Normalize(const std::string& value) {
  return ToLower(Trim(value));
}

bool HotelBookingService::IsBlank(const std::string& value) {
  return Trim(value).empty();
}

bool HotelBookingService::ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) {
  return ToLower(std::string(haystack)).find(ToLower(std::string(needle))) != std::string::npos;
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

bool HotelBookingService::DatesOverlap(const Booking& left, const Booking& right) {
  return !(left.check_out <= right.check_in || left.check_in >= right.check_out);
}

std::string HotelBookingService::HashPassword(const std::string& value) {
  return Fnv1aHex("password:" + value);
}

std::string HotelBookingService::GenerateToken(
    std::string_view login,
    std::int64_t user_id) {
  const auto seed = std::string(login) + ":" + std::to_string(user_id) + ":" + Fnv1aHex(login);
  return "session-" + Fnv1aHex(seed);
}

User HotelBookingService::ParseUser(const userver::formats::bson::Document& doc) {
  User user;
  user.id = doc["id"].As<std::int64_t>();
  user.login = doc["login"].As<std::string>();
  user.name = doc["name"].As<std::string>();
  user.surname = doc["surname"].As<std::string>();
  user.email = doc["email"].As<std::string>();
  user.password_hash = doc["password_hash"].As<std::string>();
  return user;
}

Hotel HotelBookingService::ParseHotel(const userver::formats::bson::Document& doc) {
  Hotel hotel;
  hotel.id = doc["id"].As<std::int64_t>();
  hotel.name = doc["name"].As<std::string>();
  hotel.city = doc["city"].As<std::string>();
  hotel.address = doc["address"].As<std::string>();
  hotel.description = doc["description"].As<std::string>();
  hotel.created_by_user_id = doc["created_by_user_id"].As<std::int64_t>();
  return hotel;
}

Booking HotelBookingService::ParseBooking(const userver::formats::bson::Document& doc) {
  Booking booking;
  booking.id = doc["id"].As<std::int64_t>();
  booking.user_id = doc["user_id"].As<std::int64_t>();
  booking.hotel_id = doc["hotel_id"].As<std::int64_t>();
  booking.check_in = doc["check_in"].As<std::string>();
  booking.check_out = doc["check_out"].As<std::string>();
  const auto status = doc["status"].As<std::string>();
  booking.status = status == "cancelled" ? BookingStatus::kCancelled : BookingStatus::kActive;
  return booking;
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

std::int64_t HotelBookingService::NextId(const std::string& collection_name) const {
  namespace options = userver::storages::mongo::options;
  const auto coll = GetCollection(mongo_pool_, collection_name);
  auto cursor = coll.Find(
      MakeDoc(),
      options::Sort{std::make_pair("id", options::Sort::kDescending)},
      options::Limit{1});
  auto it = cursor.begin();
  if (it == cursor.end()) {
    return 1;
  }
  return (*it)["id"].As<std::int64_t>() + 1;
}

User HotelBookingService::RequireUserPg(std::int64_t user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, login, name, surname, email, password_hash FROM users WHERE id=$1",
      user_id);
  if (result.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }
  return ParseUserRow(result.Front());
}

Hotel HotelBookingService::RequireHotelPg(std::int64_t hotel_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, name, city, address, description, created_by_user_id FROM hotels WHERE id=$1",
      hotel_id);
  if (result.IsEmpty()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "hotel_not_found", "Hotel was not found");
  }
  return ParseHotelRow(result.Front());
}

User HotelBookingService::RegisterUser(const RegisterUserRequest& request) {
  auto login = Normalize(request.login);
  auto name = Trim(request.name);
  auto surname = Trim(request.surname);
  auto email = Trim(request.email);
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

  User user;
  user.id = NextId("users");
  user.login = login;
  user.name = name;
  user.surname = surname;
  user.email = email;
  user.password_hash = HashPassword(password);

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO users(login, name, surname, email, password_hash) "
      "VALUES ($1, $2, $3, $4, $5) "
      "RETURNING id, login, name, surname, email, password_hash",
      user.login,
      user.name,
      user.surname,
      user.email,
      user.password_hash);

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

  std::lock_guard<std::mutex> lock(mutex_);
  Session session;
  session.user_id = row["id"].As<std::int64_t>();
  session.login = row["login"].As<std::string>();
  session.token = GenerateToken(session.login, session.user_id);
  sessions_[session.token] = session;

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
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = sessions_.find(token);
  if (it == sessions_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<User> HotelBookingService::GetUserById(std::int64_t user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, login, name, surname, email, password_hash FROM users WHERE id=$1",
      user_id);
  if (result.IsEmpty()) {
    return std::nullopt;
  }
  return ParseUserRow(result.Front());
}

std::optional<User> HotelBookingService::FindUserByLogin(const std::string& login) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, login, name, surname, email, password_hash FROM users WHERE login=$1",
      Normalize(login));
  if (result.IsEmpty()) {
    return std::nullopt;
  }
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
  auto name = Trim(request.name);
  auto city = Trim(request.city);
  auto address = Trim(request.address);
  auto description = Trim(request.description);

  if (name.empty() || city.empty() || address.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "name, city and address are required");
  }

  (void)RequireUserPg(created_by_user_id);

  Hotel hotel;
  hotel.id = NextId("hotels");
  hotel.name = name;
  hotel.city = city;
  hotel.address = address;
  hotel.description = description;
  hotel.created_by_user_id = created_by_user_id;

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO hotels(name, city, address, description, created_by_user_id) "
      "VALUES ($1, $2, $3, $4, $5) "
      "RETURNING id, name, city, address, description, created_by_user_id",
      hotel.name,
      hotel.city,
      hotel.address,
      hotel.description,
      hotel.created_by_user_id);

  return ParseHotelRow(result.Front());
}

std::optional<Hotel> HotelBookingService::GetHotelById(std::int64_t hotel_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, name, city, address, description, created_by_user_id FROM hotels WHERE id=$1",
      hotel_id);
  if (result.IsEmpty()) {
    return std::nullopt;
  }
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

Booking HotelBookingService::CreateBooking(const CreateBookingRequest& request, std::int64_t user_id) {
  ValidateDate(request.check_in, "check_in");
  ValidateDate(request.check_out, "check_out");
  if (request.check_in >= request.check_out) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_date_range",
        "check_in must be earlier than check_out");
  }

  (void)RequireUserPg(user_id);
  (void)RequireHotelPg(request.hotel_id);

  Booking candidate;
  candidate.id = NextId("bookings");
  candidate.user_id = user_id;
  candidate.hotel_id = request.hotel_id;
  candidate.check_in = request.check_in;
  candidate.check_out = request.check_out;
  candidate.status = BookingStatus::kActive;

  const auto bookings = GetCollection(mongo_pool_, "bookings");
  for (const auto& doc : bookings.Find(MakeDoc("hotel_id", candidate.hotel_id, "status", "active"))) {
    const auto existing = ParseBooking(doc);
    if (DatesOverlap(candidate, existing)) {
      throw ServiceError(
          ServiceErrorKind::kConflict,
          "booking_dates_conflict",
          "Hotel is already booked for the selected dates");
    }
  }

  auto booking_coll = GetCollection(mongo_pool_, "bookings");
  booking_coll.InsertOne(MakeDoc(
      "id", candidate.id,
      "user_id", candidate.user_id,
      "hotel_id", candidate.hotel_id,
      "check_in", candidate.check_in,
      "check_out", candidate.check_out,
      "status", "active",
      "created_at", userver::utils::datetime::Now()));

  return candidate;
}

std::vector<Booking> HotelBookingService::ListUserBookings(
    std::int64_t requester_user_id,
    std::int64_t target_user_id) const {
  (void)RequireUserPg(requester_user_id);
  (void)RequireUserPg(target_user_id);
  if (requester_user_id != target_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only access your own bookings");
  }

  std::vector<Booking> result;
  const auto bookings = GetCollection(mongo_pool_, "bookings");
  for (const auto& doc : bookings.Find(MakeDoc("user_id", target_user_id))) {
    result.push_back(ParseBooking(doc));
  }
  std::sort(result.begin(), result.end(), [](const Booking& left, const Booking& right) {
    return left.id < right.id;
  });
  return result;
}

void HotelBookingService::CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id) {
  (void)RequireUserPg(requester_user_id);

  auto bookings = GetCollection(mongo_pool_, "bookings");
  const auto booking_doc = bookings.FindOne(MakeDoc("id", booking_id));
  if (!booking_doc.has_value()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "booking_not_found", "Booking was not found");
  }

  const auto booking = ParseBooking(*booking_doc);
  if (booking.user_id != requester_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only cancel your own bookings");
  }
  if (booking.status == BookingStatus::kCancelled) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "booking_already_cancelled",
        "Booking has already been cancelled");
  }

  bookings.UpdateOne(
      MakeDoc("id", booking_id),
      MakeDoc("$set", MakeDoc("status", "cancelled")));
}

}
